#!/usr/bin/env python
"""
Full-fleet A/B regression comparator for two JSBSim builds (e.g. a feature
branch vs. sim-main baseline).

Runs every scripts/*.xml against Release/JSBSim.exe (or build/src/JSBSim on
Linux) in each of two repo checkouts/worktrees, captures every CSV each
script produces plus its exit code, then diffs each pair cell-by-cell.

This complements (does not replace) `make -f Makefile.sim test-regression[-full]`:
that target runs check_cases/ against reference data baked into the repo and
a small set of our own scripts/test_*.xml aircraft regressions; this script
instead diffs the *entire* scripts/ fleet (all aircraft with a runnable
script) directly against a second build, which is the shape of check needed
for "this C++ change must be a no-op for every aircraft that doesn't opt in"
acceptance criteria (see docs/requirements/REQ-003-fggearbox-core-class.md).

Usage:
    python admin/compare_fleet_regression.py <repo_a> <repo_b> [outdir] [tol]

    <repo_a>, <repo_b>  paths to two JSBSim checkouts/worktrees, each already
                        built (Release/JSBSim.exe present on Windows, or
                        build/src/JSBSim on Linux).
    [outdir]            where to stash per-script CSVs/logs from both runs
                        (default: ./_fleet_regression next to this script's
                        cwd). Safe to delete afterwards.
    [tol]               absolute per-cell tolerance for the CSV diff
                        (default 1e-9 -- effectively bit-identical; use
                        1e-5 to mirror the check_cases/RunCheckCases.py
                        convention).

Exit code is 0 iff every script's exit code and every produced CSV matches
between the two builds within tolerance.
"""
import csv
import glob
import json
import os
import shutil
import subprocess
import sys
import time

def jsbsim_bin(repo):
    win = os.path.join(repo, "Release", "JSBSim.exe")
    if os.path.exists(win):
        return win
    nix = os.path.join(repo, "build", "src", "JSBSim")
    if os.path.exists(nix):
        return nix
    raise SystemExit(f"compare_fleet_regression: no JSBSim binary found under {repo} "
                      f"(looked for Release/JSBSim.exe and build/src/JSBSim) -- build first")


def run_fleet(repo, outdir, timeout_s=180):
    exe = jsbsim_bin(repo)
    scripts = sorted(glob.glob(os.path.join(repo, "scripts", "*.xml")))
    os.makedirs(outdir, exist_ok=True)
    results = {}
    for i, script in enumerate(scripts):
        name = os.path.basename(script)
        stem = os.path.splitext(name)[0]
        for f in glob.glob(os.path.join(repo, "*.csv")):
            os.remove(f)
        t0 = time.time()
        try:
            proc = subprocess.run(
                [exe, "--root=./", f"--script=scripts/{name}"],
                cwd=repo, capture_output=True, text=True, timeout=timeout_s,
            )
            rc, log, timedout = proc.returncode, (proc.stdout or "") + "\n" + (proc.stderr or ""), False
        except subprocess.TimeoutExpired:
            rc, log, timedout = None, "TIMEOUT", True
        dt = time.time() - t0

        dest_dir = os.path.join(outdir, stem)
        os.makedirs(dest_dir, exist_ok=True)
        csv_names = []
        for f in glob.glob(os.path.join(repo, "*.csv")):
            bn = os.path.basename(f)
            shutil.move(f, os.path.join(dest_dir, bn))
            csv_names.append(bn)
        with open(os.path.join(dest_dir, "run.log"), "w", encoding="utf-8", errors="replace") as lf:
            lf.write(log)

        results[stem] = {"rc": rc, "timedout": timedout, "csvs": csv_names, "seconds": round(dt, 2)}
        print(f"[{i+1}/{len(scripts)}] {name}: rc={rc} timedout={timedout} csvs={csv_names} t={dt:.1f}s")

    with open(os.path.join(outdir, "_summary.json"), "w") as f:
        json.dump(results, f, indent=2)
    return results


def load_csv(path):
    with open(path, newline="") as f:
        rows = list(csv.reader(f))
    if not rows:
        return [], []
    return [c.strip() for c in rows[0]], rows[1:]


def compare(feat_dir, main_dir, feat, main, tol):
    all_stems = sorted(set(feat) | set(main))
    n_pass = n_fail = 0
    fail_report = []
    for stem in all_stems:
        f, m = feat.get(stem), main.get(stem)
        if f is None or m is None:
            fail_report.append((stem, "MISSING on one side", f"a={f is not None} b={m is not None}"))
            n_fail += 1
            continue
        issues = []
        if f["rc"] != m["rc"]:
            issues.append(f"rc mismatch: a={f['rc']} b={m['rc']}")
        if set(f["csvs"]) != set(m["csvs"]):
            issues.append(f"csv set mismatch: a={f['csvs']} b={m['csvs']}")
        else:
            for csvname in f["csvs"]:
                fhdr, frows = load_csv(os.path.join(feat_dir, stem, csvname))
                mhdr, mrows = load_csv(os.path.join(main_dir, stem, csvname))
                if fhdr != mhdr:
                    issues.append(f"{csvname}: header mismatch")
                    continue
                if len(frows) != len(mrows):
                    issues.append(f"{csvname}: row count {len(frows)} vs {len(mrows)}")
                    continue
                bad = 0
                worst = 0.0
                for r, c in zip(frows, mrows):
                    for a, b in zip(r, c):
                        try:
                            x, y = float(a), float(b)
                        except ValueError:
                            if a.strip() != b.strip():
                                bad += 1
                            continue
                        d = abs(x - y)
                        worst = max(worst, d)
                        if d > tol:
                            bad += 1
                if bad:
                    issues.append(f"{csvname}: {bad} cells differ > tol={tol} (worst {worst:g})")
        if issues:
            n_fail += 1
            fail_report.append((stem, "; ".join(issues), f"a_rc={f['rc']} b_rc={m['rc']}"))
        else:
            n_pass += 1
    return n_pass, n_fail, fail_report


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        raise SystemExit(2)
    repo_a, repo_b = sys.argv[1], sys.argv[2]
    outdir = sys.argv[3] if len(sys.argv) > 3 else "./_fleet_regression"
    tol = float(sys.argv[4]) if len(sys.argv) > 4 else 1e-9

    out_a = os.path.join(outdir, "a")
    out_b = os.path.join(outdir, "b")
    feat = run_fleet(repo_a, out_a)
    main = run_fleet(repo_b, out_b)
    n_pass, n_fail, fail_report = compare(out_a, out_b, feat, main, tol)

    print(f"\nTOTAL scripts compared: {n_pass + n_fail}")
    print(f"PASS (identical within tol {tol}): {n_pass}")
    print(f"FAIL: {n_fail}")
    for stem, issue, extra in fail_report:
        print(f"FAIL: {stem}\n    {issue}\n    {extra}")

    raise SystemExit(1 if n_fail else 0)


if __name__ == "__main__":
    main()
