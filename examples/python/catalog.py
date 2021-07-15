# Originally developed by JSBSim Team
# Modified by Guilherme A. L. da Silva - aerothermalsolutions.co

# List properties of a script
# This is good to know which variable to read and write

# Instructions
# Select a string to search
# See the list of properties containing that string
# And its status
# R - Read
# W - Write

# GNU Lesser General Public License v2.1

# To install jsbsim module in Python
# pip install jsbsim
# Or conda install jsbsim (if you have anaconda, this is the best way)

import jsbsim
import os

# Put your path here
PATH_TO_JSBSIM_FILES="/Users/gasilva/Documents/GitHub/jsbsim" #my is a macbook
# Object with JSB executable
fdm = jsbsim.FGFDMExec(PATH_TO_JSBSIM_FILES)
# Load script
fdm.load_script(os.path.join(fdm.get_root_dir(), 'aircraft/global5000/scripts/trim-cruise_ap.xml')) 

# Provide an empty string to get the complete list of properties.
# Provide a string to seach properites that contains the string
string4Search="throttle"
print(fdm.query_property_catalog(string4Search))  
