# Dummy module that mimics the behavior of fpectl for platforms where fpectl
# cannot (yet) be built.
#
# Copyright (c) 2020 Bertrand Coconnier
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

TURNON = False

def turnon_sigfpe():
    global TURNON
    TURNON = True

def turnoff_sigfpe():
    global TURNON
    TURNON = False

def test_sigfpe():
    global TURNON
    if TURNON:
        raise FloatingPointError
