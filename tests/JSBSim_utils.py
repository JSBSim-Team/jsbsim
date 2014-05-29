# JSBSim_utils.py
#
# Some utilities to help developing Python scripts with JSBSim.
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

import os, sys, csv, string
import jsbsim

path_to_jsbsim_files = os.path.relpath(sys.argv[1], os.getcwd())

def InitFDM():
    _fdm = jsbsim.FGFDMExec(root_dir=os.path.join('.',''))
    _fdm.set_aircraft_path(os.path.join(path_to_jsbsim_files, 'aircraft'))
    _fdm.set_engine_path(os.path.join(path_to_jsbsim_files, 'engine'))
    _fdm.set_systems_path(os.path.join(path_to_jsbsim_files, 'systems'))
    return _fdm

def delete_csv_files():
    files = os.listdir(".")
    for f in files:
        if f[-4:] == '.csv':
            os.remove(f)

def ExecuteUntil(_fdm, end_time):
    while _fdm.run():
        if _fdm.get_sim_time() > end_time:
            return

class MismatchError(Exception):
    pass

class Table:
    def __init__(self, headers=[]):
        if headers:
            self._lines = [headers]
        else:
            self._lines = []
        self._missing = []
    def ReadCSV(self, filename):
        self._lines = []
        self.missing = []

        file_csv = open(filename,'r')
        first_line = True
        for line in csv.reader(file_csv, delimiter=','):
            if first_line:
                first_line = False
                line = map(string.strip, line)
            else:
                line = map(float, line)
            self._lines += [line]
    
        file_csv.close()
    def add_line(self, line):
        if len(line) != len(self._lines[0]):
            raise MismatchError
        self._lines += [line]
    def get_column(self, col):
        column = []

        if type(col) == type(0):
            if col < 0 or col >= len(self._lines[0]):
                raise AttributeError
        elif type(col) == type(''):
            header = string.strip(col)
            for col in xrange(len(self._lines[0])):
                if header == self._lines[0][col]:
                    break
            else:
                raise AttributeError
        else:
            raise TypeError

        for line in self._lines:
            column += [line[col]]
        return column

    def compare(self, other, precision=1E-5):
        result = Table(['Property','delta','Time','ref value','value'])

        if len(self._lines) != len(other._lines):
            raise MismatchError

        for row in xrange(1, len(self._lines)):
            if abs(self._lines[row][0] - other._lines[row][0]) > 1E-10:
                raise MismatchError

        for col, key in enumerate(self._lines[0][1:]):
            if not key in other._lines[0]:
                result._missing += [key]

            comparison = [key, 0.0]
            for row, line in enumerate(self._lines[1:]):
                delta = abs(line[col] - other._lines[row+1][col])
                if delta > comparison[1]:
                    comparison = [key, delta, line[0], line[col],
                                  other._lines[row+1][col]]

            if comparison[1] > precision:
                result.add_line(comparison)

            if len(result._lines) == 1:
                return Table()

        return result

    def empty(self):
        return len(self._lines) == 0

    def __repr__(self):
        col_width = [max(len(str(item)) for item in col) for col in zip(*self._lines)]
        output = ''
        for line in self._lines:
            output += "|" + "|".join("{:{}}".format(str(item), col_width[i]) for i, item in enumerate(line)) + "|\n"
        return output
