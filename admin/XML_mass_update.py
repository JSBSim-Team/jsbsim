#!/usr/bin/python
#
# This script adds the attribute `negated_crossproduct_inertia="true"` to
# all the aircraft models of JSBSim that have at least one non zero cross
# product inertia.

import os
import xml.etree.ElementTree as et

def CheckXMLFile(f, header):
    # Is f an XML file ?
    try:
        tree = et.parse(f)
    except et.ParseError:
        return False

    # Check the file header
    return tree.getroot().tag.upper() == header.upper()

def recursive_scan_XML(dir_name, header):
    for d in os.scandir(dir_name):
        if d.is_file():
            fname = os.path.join(dir_name,d.name)
            if CheckXMLFile(fname, header):
                yield fname
            continue
        if d.is_dir():
            for f in recursive_scan_XML(os.path.join(dir_name, d.name), header):
                yield f

shift = len('mass_balance')
for fname in recursive_scan_XML('.', 'fdm_config'):
    tree = et.parse(fname)
    mass_balance_tag = tree.getroot().find('./mass_balance')
    if mass_balance_tag is None:
        continue

    # Check if there is a non zero cross product inertia
    ixy_tag = mass_balance_tag.find('ixy')
    if ixy_tag is None or float(ixy_tag.text) == 0.0:
        ixz_tag = mass_balance_tag.find('ixz')
        if ixz_tag is None or float(ixz_tag.text) == 0.0:
            iyz_tag = mass_balance_tag.find('iyz')
            if iyz_tag is None or float(iyz_tag.text) == 0.0:
                continue

    with open(fname, 'r') as f:
        lines = f.readlines()

        for i, l in enumerate(lines):
            index = l.find('mass_balance')
            if index == -1:
                continue
            index += shift
            lines[i] = l[:index]+' negated_crossproduct_inertia="true"'+l[index:]
            break

    with open(fname, 'w') as f:
        f.writelines(lines)
