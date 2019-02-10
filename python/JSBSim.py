#! /usr/bin/python
#
# JSBSim.py
#
# Standalone version of JSBSim in Python language.
#
# Copyright (c) 2019 Bertrand Coconnier
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, see <http://www.gnu.org/licenses/>
#

import xml.etree.ElementTree as et
import argparse, sys, os
import jsbsim

parser = argparse.ArgumentParser()
parser.add_argument("input", nargs='?', help="script file name")
parser.add_argument("--version", action="version",
                    version="%(prog)s {}".format(jsbsim.FGJSBBase().get_version()))
parser.add_argument("--outputlogfile", action="append", metavar="<filename>",
                    help="sets (overrides) the name of a data output file")
parser.add_argument("--logdirectivefile", action="append", metavar="<filename>",
                    help="specifies the name of a data logging directives file")
parser.add_argument("--root", default='.', metavar="<path>",
                    help="specifies the JSBSim root directory (where aircraft/, engine/, etc. reside)")
parser.add_argument("--aircraft", metavar="<filename>",
                    help="specifies the name of the aircraft to be modeled")
parser.add_argument("--script", metavar="<filename>",
                    help="specifies a script to run")
parser.add_argument("--initfile", metavar="<filename>",
                    help="specifies an initialization file")
parser.add_argument("--end", type=float, default=1E99, metavar="<time (double)>",
                    help="specifies the sim end time")
args = parser.parse_args()

def CheckXMLFile(f):
    # Is f a file ?
    if not os.path.isfile(f):
        return None

    # Is f an XML file ?
    try:
        tree = et.parse(f)
    except et.ParseError:
        return None

    return tree

if args.input:
    tree = CheckXMLFile(args.input)
    if not tree:
        print('The argument "{}" cannot be interpreted as a file name.'.format(args.input))
        sys.exit(1)

    root = tree.getroot()

    if root.tag == 'runscript':
        if args.script:
            print('Two script files are specified.')
            sys.exit(1)
        else:
            args.script = args.input

    if root.tag == 'output':
        if args.logdirectivefile:
            args.logdirectivefile += [args.input]
        else:
            args.logdirectivefile = [args.input]

    if root.tag == 'fdm_config':
        if args.aircraft:
            print('Two aircraft files are specified.')
            sys.exit(1)
        else:
            args.aircraft = args.input

fdm = jsbsim.FGFDMExec(args.root, None)

if args.script:
    fdm.load_script(args.script)
elif args.aircraft:
    fdm.load_model(args.aircraft, False)

if args.logdirectivefile:
    for f in args.logdirectivefile:
        if not fdm.set_output_directive(f):
            print("Output directives not properly set in file {}".format(f))
            sys.exit(1)

if args.outputlogfile:
    for n,f in enumerate(args.outputlogfile):
        old_filename = fdm.get_output_filename(n)
        if not fdm.set_output_filename(n, f):
            print("Output filename could not be set")
        else:
            print("Output filename change from {} from aircraft configuration file to {} specified on command line.".format(old_filename, f))

fdm.run_ic()
fdm.print_simulation_configuration()

while fdm.run() and fdm.get_sim_time() <= args.end:
    pass
