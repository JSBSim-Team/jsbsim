import os, csv, sys, string
import jsbsim

path_to_jsbsim_files = os.path.join(os.path.relpath(sys.argv[1], os.getcwd()),
                                    'check_cases', 'orbit')

def InitFDM():
    _fdm = jsbsim.FGFDMExec(root_dir="./")
    _fdm.set_aircraft_path(path_to_jsbsim_files+"/aircraft")
    _fdm.set_engine_path(path_to_jsbsim_files+"/engine")
    _fdm.set_systems_path(path_to_jsbsim_files+"/systems")
    return _fdm

def ReadCSV(filename):
    lines = []
    file_csv = open(filename,'r')
    first_line = True
    for line in csv.reader(file_csv, delimiter=','):
        if first_line:
            first_line = False
            line = map(string.strip, line)
        else:
            line = map(float, line)
        lines += [line]

        columns = {}
        for i in xrange(len(lines[0])):
            header = lines[0][i]
            columns[header] = []
            for j in xrange(1, len(lines)):
                columns[header] += [lines[j][i]]
    return columns

fdm = InitFDM()
fdm.load_script(path_to_jsbsim_files+"/scripts/ball_orbit.xml")
fdm.run_ic()

while fdm.run():
    pass

ref_columns = ReadCSV(path_to_jsbsim_files+"/logged_data/BallOut.csv")
current_columns = ReadCSV('BallOut.csv')

deltas = {}
failed = False
report = False

for key in ref_columns.keys():
    if not current_columns.has_key(key):
        if not report:
            report = True
            print "{:^30} | {:^11} | {:^15} | {:^15}".format("Property","delta","ref value","value")
            print "-"*31+"+"+"-"*13+"+"+"-"*17+"+"+"-"*16
        print "{:<30} | {:^47}".format(key, "*** has not been exported ***")
        continue

    deltas[key] = [0.0, -1]

    for row in xrange(min(len(ref_columns[key]), len(current_columns[key]))):
        delta = abs(current_columns[key][row]-ref_columns[key][row])
        if delta > deltas[key][0]:
            deltas[key] = [delta, row]

    delta, row = deltas[key]
    if delta > 1E-5:
        if not report:
            report = True
            print "{:^30} | {:^11} | {:^15} | {:^15}".format("Property","delta","ref value","value")
            print "-"*31+"+"+"-"*13+"+"+"-"*17+"+"+"-"*16
        print "{:<30} | {:.5e} | {:>15.7f} | {:>15.7f}".format(key, delta, ref_columns[key][row],
                                                           current_columns[key][row])
        failed = True


if failed:
  sys.exit(-1) # 'make test' will report the test failed.

sys.exit(0) # Needed for 'make test' to report the test passed.
