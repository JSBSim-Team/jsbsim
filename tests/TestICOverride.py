# TestICOverride.py
#
# A regression test that checks that IC loaded from a file by a script can be
# overridden.
#
# Copyright (c) 2014 Bertrand Coconnier
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

import sys
import xml.etree.ElementTree as et
from JSBSim_utils import Table, CreateFDM, ExecuteUntil, SandBox

sandbox = SandBox()
fpstokts = 0.592484

# Run the script c1724.xml
script_path = sandbox.path_to_jsbsim_file('scripts', 'c1724.xml')

fdm = CreateFDM(sandbox)
fdm.load_script(script_path)

vt0 = fdm.get_property_value('ic/vt-kts')

fdm.run_ic()
ExecuteUntil(fdm, 1.0)

# Check that the total velocity exported in the output file matches the IC
# defined in the initialization file
ref = Table()
ref.ReadCSV(sandbox('JSBout172B.csv'))

for col, title in enumerate(ref._lines[0]):
    if title == 'V_{Total} (ft/s)':
        if abs(ref._lines[1][col] - (vt0 / fpstokts)) > 1E-5:
            print "Original script %s" % (script_path,)
            print "The total velocity is %f. The value %f was expected" % (ref._lines[1][col], vt0 / fpstokts)
            sys.exit(-1)
        break
else:
    print "The total velocity is not exported in %s" % (script_path,)
    sys.exit(-1)

# Now, we will re-run the same test but the IC will be overridden in the scripts
# The initial total velocity is increased by 1 ft/s
vt0 += 1.0

# The script c1724.xml is loaded and the following line is added in it:
#    <property value="..."> ic/vt-kts </property>
# The modified script is then saved with the named 'c1724_0.xml'
tree = et.parse(sandbox.elude(script_path))
run_tag = tree.getroot().find("./run")
property = et.SubElement(run_tag, 'property')
property.text = 'ic/vt-kts'
property.attrib['value'] = str(vt0)
tree.write(sandbox('c1724_0.xml'))

# Re-run the same check than above. This time we are making sure than the total
# initial velocity is increased by 1 ft/s
sandbox.delete_csv_files()

fdm = CreateFDM(sandbox)
fdm.load_script('c1724_0.xml')

if abs(fdm.get_property_value('ic/vt-kts') - vt0) > 1E-5:
    print "Modified script %s" % (sandbox('JSBout172B.csv'),)
    print "The total velocity in the IC (%f) is different from %f" % (fdm.get_property_value('ic/vt-kts'), vt0)
    sys.exit(-1)

fdm.run_ic()
ExecuteUntil(fdm, 1.0)

mod = Table()
mod.ReadCSV(sandbox('JSBout172B.csv'))

for col, title in enumerate(mod._lines[0]):
    if title == 'V_{Total} (ft/s)':
        if abs(mod._lines[1][col] - (vt0 / fpstokts)) > 1E-5:
            print "Modified script %s" % (sandbox('JSBout172B.csv'),)
            print "The total velocity is %f. The value %f was expected" % (mod._lines[1][col], vt0 / fpstokts)
            sys.exit(-1)
        break
else:
    print "The total velocity is not exported in %s" % (sandbox('JSBout172B.csv'),)
    sys.exit(-1)

sandbox.erase()
