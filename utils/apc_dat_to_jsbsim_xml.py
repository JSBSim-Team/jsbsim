#!/usr/bin/env python3
"""
Convert APC PER3 propeller performance .dat files to JSBSim propeller XML.

APC distributes wind-tunnel data in "PER3" format, organised as a sequence of
RPM blocks.  Each block has a header line "PROP RPM = <n>" followed by column
headers and data rows: V  J  Pe  Ct  Cp  PWR  ...

The script picks one baseline RPM (default 10000) for the J-dependent C_THRUST
and C_POWER tables, then sweeps all RPM blocks at J=0 (static) to build the
Reynolds-number correction tables CP_RPM_FACTOR and CT_RPM_FACTOR, both
normalised to 1.0 at the baseline RPM.

Performance data for APC propellers can be downloaded from here:
https://www.apcprop.com/technical-information/performance-data

Usage
-----
    python apc_dat_to_jsbsim_xml.py PER3_18x8E.dat [options]

Options
-------
    --output FILE       Output XML filename (default: APC_<name>.xml in same dir)
    --rpm    INT        Baseline RPM for C_THRUST/C_POWER tables (default: 10000)
    --mass   FLOAT      Propeller mass in grams – computes ixx automatically
                        using (1/12)*m*D² (blades as uniform rods), output in KG*M2
    --ixx    FLOAT      Override: moment of inertia in slug*ft² (ignored if --mass given)
    --blades INT        Number of blades (default: 2)
    --no-rpm-factors    Omit CP_RPM_FACTOR and CT_RPM_FACTOR tables

HISTORY
--------------------------------------------------------------------------------
06/2026  Andreas Kanzler    Created
"""

import argparse
import re
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

def parse_apc_dat(filepath: Path) -> dict:
    """Return a dict with keys: name, diameter_in, pitch_in, rpm_sections.

    rpm_sections is {rpm_int: [(J, CT, CP), ...]} sorted by J ascending.

    APC PER3 format (v2022+):
      Line 0  : "<name>   (<name>.dat)"
      Sections: "PROP RPM =  <n>"
                "  V     J     Pe    Ct    Cp    PWR  ..."   <- column header
                "  (mph) (Adv_Ratio) -     -     -   ..."   <- units row (skip)
                data rows with col-indices: 0=V 1=J 2=Pe 3=Ct 4=Cp ...
    """
    text = filepath.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()

    # --- Prop name from first non-empty token on line 0 ---
    first_tokens = lines[0].split() if lines else []
    prop_name: str = first_tokens[0] if first_tokens else filepath.stem

    # --- Diameter and pitch from name pattern "DDxPP[letter]" e.g. "18x8E" ---
    diameter_in: float | None = None
    pitch_in: float | None = None
    m = re.match(r"([\d.]+)[xX]([\d.]+)", prop_name)
    if m:
        diameter_in = float(m.group(1))
        pitch_in = float(m.group(2))

    # --- Walk lines, collecting RPM sections ---
    rpm_sections: dict[int, list[tuple[float, float, float]]] = {}
    current_rpm: int | None = None
    skip_units_row = False
    in_data = False

    for line in lines:
        # Detect new RPM block: "PROP RPM =  <n>"
        m = re.match(r"\s*PROP\s+RPM\s*=\s*([\d.]+)", line, re.I)
        if m:
            current_rpm = round(float(m.group(1)))
            rpm_sections[current_rpm] = []
            skip_units_row = False
            in_data = False
            continue

        if current_rpm is None:
            continue

        # Column-header row contains "V", "J", "Ct" (case-sensitive in APC files)
        if not in_data and re.search(r"\bV\b.*\bJ\b.*\bCt\b", line):
            skip_units_row = True  # units row "(mph) (Adv_Ratio) ..." comes next
            continue

        # Skip the units row that follows the column header
        if skip_units_row:
            skip_units_row = False
            in_data = True
            continue

        # Parse data rows: col 0=V, 1=J, 2=Pe, 3=Ct, 4=Cp
        if in_data:
            parts = line.split()
            if len(parts) >= 5:
                try:
                    j = float(parts[1])
                    ct = float(parts[3])
                    cp = float(parts[4])
                    rpm_sections[current_rpm].append((j, ct, cp))
                except ValueError:
                    pass

    # Remove empty sections and sort rows by J.
    rpm_sections = {
        rpm: sorted(rows, key=lambda r: r[0])
        for rpm, rows in rpm_sections.items()
        if rows
    }

    return {
        "name": prop_name,
        "diameter_in": diameter_in,
        "pitch_in": pitch_in,
        "rpm_sections": rpm_sections,
    }


# ---------------------------------------------------------------------------
# XML generation
# ---------------------------------------------------------------------------

def _nearest_rpm(available: list[int], target: int) -> int:
    return min(available, key=lambda r: abs(r - target))


_J_STATIC_TOL = 1e-6   # J values at or below this are treated as exact zero
_J_STATIC_MAX = 0.10   # blocks whose minimum J exceeds this are skipped


def _static_at_j0(
    rows: list[tuple[float, float, float]],
) -> tuple[float, float] | None:
    """Return (CT, CP) at J=0, extrapolating linearly when needed.

    APC datasets do not always start at J=0.0 for every RPM block.  Comparing
    each block at its own minimum-J would mix static and lightly-dynamic
    conditions, corrupting the Reynolds-number correction ratios.  This
    function pins every block to the same reference point:

      * J <= _J_STATIC_TOL  →  use the row directly (exact zero).
      * J in (tol, _J_STATIC_MAX]  →  linearly extrapolate from the two
        lowest-J rows down to J=0.
      * J > _J_STATIC_MAX or fewer than 2 rows available for extrapolation
        →  return None (caller should skip this block with a warning).
    """
    if not rows:
        return None

    # rows are already sorted by J ascending
    j0, ct0, cp0 = rows[0]

    if j0 <= _J_STATIC_TOL:
        return ct0, cp0

    if j0 > _J_STATIC_MAX or len(rows) < 2:
        return None

    j1, ct1, cp1 = rows[1]
    dj = j1 - j0
    if abs(dj) < 1e-9:           # duplicate J values – fall back to first row
        return ct0, cp0

    # Linear extrapolation: value_at_0 = value_at_j0 - j0 * slope
    slope_ct = (ct1 - ct0) / dj
    slope_cp = (cp1 - cp0) / dj
    return ct0 - j0 * slope_ct, cp0 - j0 * slope_cp


def calc_ixx_kgm2(mass_g: float, diameter_in: float) -> float:
    """Moment of inertia about spin axis using thin-rod blade approximation.

    I = (1/12) * m * D²  (equivalent to (1/3)*m*r² for two opposing rods)
    Valid for any blade count - blades are assumed uniformly distributed.
    """
    m_kg = mass_g / 1000.0
    d_m = diameter_in * 0.0254
    return (1.0 / 12.0) * m_kg * d_m ** 2


def build_xml(
    input_file: str,
    data: dict,
    baseline_rpm: int,
    mass_g: float | None,
    ixx_slug_ft2: float,
    blades: int,
    include_rpm_factors: bool,
) -> str:
    rpm_sections = data["rpm_sections"]
    available = sorted(rpm_sections.keys())

    if not available:
        raise ValueError("No usable RPM sections found in the file.")

    actual_baseline = _nearest_rpm(available, baseline_rpm)
    if actual_baseline != baseline_rpm:
        print(
            f"  Note: RPM {baseline_rpm} not found – using nearest: {actual_baseline}",
            file=sys.stderr,
        )

    baseline_rows = rpm_sections[actual_baseline]
    ct_rows = [(j, ct) for j, ct, _ in baseline_rows]
    cp_rows = [(j, cp) for j, _, cp in baseline_rows]

    # RPM correction factors (normalised to baseline static value).
    cp_factor_rows: list[tuple[int, float]] = []
    ct_factor_rows: list[tuple[int, float]] = []

    if include_rpm_factors:
        base_result = _static_at_j0(baseline_rows)
        if base_result is None:
            print(
                "  Warning: baseline RPM block has no usable J=0 data"
                f" (min J > {_J_STATIC_MAX}) – skipping RPM factors.",
                file=sys.stderr,
            )
        else:
            base_ct0, base_cp0 = base_result
            if base_ct0 == 0.0 or base_cp0 == 0.0:
                print(
                    "  Warning: baseline static CT or CP is zero - skipping RPM factors.",
                    file=sys.stderr,
                )
            else:
                for rpm in available:
                    result = _static_at_j0(rpm_sections[rpm])
                    if result is None:
                        print(
                            f"  Warning: RPM {rpm} block has no usable J=0 data"
                            f" (min J > {_J_STATIC_MAX}) - skipping from RPM factors.",
                            file=sys.stderr,
                        )
                        continue
                    ct0, cp0 = result
                    ct_factor_rows.append((rpm, ct0 / base_ct0))
                    cp_factor_rows.append((rpm, cp0 / base_cp0))

    # ---- Format helpers -------------------------------------------------- #

    def table_2col(rows: list[tuple[float, float]], indent: int = 6) -> str:
        pad = " " * indent
        return "\n".join(f"{pad}{j:8.4f}   {v:8.4f}" for j, v in rows)

    def table_rpm(rows: list[tuple[int, float]], indent: int = 6) -> str:
        pad = " " * indent
        return "\n".join(f"{pad}{rpm:6d}   {factor:.3f}" for rpm, factor in rows)

    # ---- Build XML -------------------------------------------------------- #

    name = data["name"]
    diam = data["diameter_in"] or 0.0

    # ixx: prefer mass-based calculation (KG*M2) over manual slug·ft² override.
    if mass_g is not None:
        ixx_val = calc_ixx_kgm2(mass_g, diam)
        ixx_tag = f'  <ixx unit="KG*M2"> {ixx_val:.3e} </ixx>'
        ixx_tag += (
            f'  <!-- (1/12) * {mass_g:.1f}e-3 * ({diam:.4g}*0.0254)^2 -->'
        )
    else:
        ixx_tag = f'  <ixx>{ixx_slug_ft2}</ixx>'

    out: list[str] = []
    out.append('<?xml version="1.0"?>')
    out.append('')
    out.append(f'<!-- Generated by {Path(__file__).name} from {input_file} -->')
    out.append('')
    out.append(f'<propeller name="{name}">')
    out.append(ixx_tag)
    out.append(f'  <diameter unit="IN">{diam:.4g}</diameter>')
    out.append(f'  <numblades>{blades}</numblades>')
    out.append(f'  <constspeed>0</constspeed>')
    out.append('')
    out.append(
        f'  <!-- C_THRUST and C_POWER from APC PER3 measurement data'
        f' at {actual_baseline} RPM -->'
    )
    out.append('')
    out.append('  <table name="C_THRUST" type="internal">')
    out.append('    <tableData>')
    out.append(table_2col(ct_rows))
    out.append('    </tableData>')
    out.append('  </table>')
    out.append('')
    out.append('  <table name="C_POWER" type="internal">')
    out.append('    <tableData>')
    out.append(table_2col(cp_rows))
    out.append('    </tableData>')
    out.append('  </table>')

    if ct_factor_rows:
        out.append('')
        out.append(
            '  <!-- Reynolds-number correction for off-design RPM operation.'
        )
        out.append(
            f'       Normalised to 1.0 at the baseline {actual_baseline} RPM data set above.'
        )
        out.append(
            '       Derived from APC PER3 static (J=0) values across RPM sweep.'
        )
        out.append('  -->')
        out.append('  <table name="CP_RPM_FACTOR" type="internal">')
        out.append('    <tableData>')
        out.append(table_rpm(cp_factor_rows))
        out.append('    </tableData>')
        out.append('  </table>')
        out.append('  <table name="CT_RPM_FACTOR" type="internal">')
        out.append('    <tableData>')
        out.append(table_rpm(ct_factor_rows))
        out.append('    </tableData>')
        out.append('  </table>')

    out.append('')
    out.append('</propeller>')

    return "\n".join(out) + "\n"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Convert an APC PER3 .dat file to a JSBSim propeller XML.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("input", type=Path, help="APC PER3 .dat file")
    parser.add_argument(
        "--output", "-o", type=Path, default=None,
        help="Output XML file (default: APC_<name>.xml in the same directory as input)",
    )
    parser.add_argument(
        "--rpm", type=int, default=10000, metavar="INT",
        help="Baseline RPM for C_THRUST/C_POWER tables (default: 10000)",
    )
    ixx_group = parser.add_mutually_exclusive_group()
    ixx_group.add_argument(
        "--mass", type=float, default=None, metavar="GRAMS",
        help="Propeller mass in grams; ixx computed as (1/12)*m*D² [KG*M2]",
    )
    ixx_group.add_argument(
        "--ixx", type=float, default=0.001, metavar="FLOAT",
        help="Moment of inertia in slug·ft² (used when --mass is not given; default: 0.001)",
    )
    parser.add_argument(
        "--blades", type=int, default=2, metavar="INT",
        help="Number of blades (default: 2)",
    )
    parser.add_argument(
        "--no-rpm-factors", action="store_true",
        help="Omit CP_RPM_FACTOR and CT_RPM_FACTOR tables",
    )

    args = parser.parse_args()

    if not args.input.exists():
        parser.error(f"File not found: {args.input}")

    print(f"Parsing  : {args.input}")
    data = parse_apc_dat(args.input)

    output_path: Path = args.output or args.input.parent / f"APC_{data['name']}.xml"

    available = sorted(data["rpm_sections"].keys())
    print(f"Prop     : {data['name']}")
    print(f"Diameter : {data['diameter_in']} in  |  Pitch: {data['pitch_in']} in")
    print(f"RPM data : {available}")

    if args.mass is not None:
        ixx_kgm2 = calc_ixx_kgm2(args.mass, data["diameter_in"] or 0.0)
        print(f"ixx      : {ixx_kgm2:.3e} kg·m²  (from --mass {args.mass} g)")
    else:
        print(f"ixx      : {args.ixx} slug·ft²  (manual --ixx)")

    xml = build_xml(
        args.input,
        data,
        baseline_rpm=args.rpm,
        mass_g=args.mass,
        ixx_slug_ft2=args.ixx,
        blades=args.blades,
        include_rpm_factors=not args.no_rpm_factors,
    )

    output_path.write_text(xml, encoding="utf-8")
    print(f"Written  : {output_path}")


if __name__ == "__main__":
    main()
