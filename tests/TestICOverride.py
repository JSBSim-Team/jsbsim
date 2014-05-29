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

import os, sys
import xml.etree.ElementTree as et
from JSBSim_utils import path_to_jsbsim_files, Table, InitFDM, delete_csv_files, ExecuteUntil
import jsbsim

delete_csv_files()
fpstokts = 0.592484

# Run the script c1724.xml
script_full_path = os.path.join(path_to_jsbsim_files, 'scripts', 'c1724.xml')

fdm = InitFDM()
fdm.load_script(script_full_path)

vt0 = fdm.get_property_value('ic/vt-kts')

fdm.run_ic()
ExecuteUntil(fdm, 1.0)

# Check that the total velocity exported in the output file matches the IC
# defined in the initialization file
ref = Table()
ref.ReadCSV('JSBout172B.csv')

for col, title in enumerate(ref._lines[0]):
    if title == 'V_{Total} (ft/s)':
        if abs(ref._lines[1][col] - (vt0 / fpstokts)) > 1E-5:
            sys.exit(-1)
        break
else:
    sys.exit(-1)

# Now, we will re-run the same test but the IC will be overridden in the scripts
# The initial total velocity is increased by 1 ft/s
vt0 += 1.0

# The script c1724.xml is loaded and the following line is added in it:
#    <property value="..."> ic/vt-kts </property>
# The modified script is then saved with the named 'c1724_0.xml'
tree = et.parse(script_full_path)
run_tag = tree.getroot().find("./run")
property = et.SubElement(run_tag, 'property')
property.text = 'ic/vt-kts'
property.attrib['value'] = str(vt0)
tree.write('c1724_0.xml')

# Re-run the same check than above. This time we are making sure than the total
# initial velocity is increased by 1 ft/s
delete_csv_files()

fdm = InitFDM()
fdm.load_script('c1724_0.xml')

if abs(fdm.get_property_value('ic/vt-kts') - vt0) > 1E-5:
  sys.exit(-1)

fdm.run_ic()
ExecuteUntil(fdm, 1.0)

mod = Table()
mod.ReadCSV('JSBout172B.csv')

for col, title in enumerate(mod._lines[0]):
    if title == 'V_{Total} (ft/s)':
        if abs(mod._lines[1][col] - (vt0 / fpstokts)) > 1E-5:
            sys.exit(-1)
        break
else:
    sys.exit(-1)

sys.exit(0)
