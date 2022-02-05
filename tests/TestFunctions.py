# TestFunctions.py
#
# Test that the <fcs_function> component is functional.
#
# Copyright (c) 2018 Bertrand Coconnier
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

import math
import numpy as np
import xml.etree.ElementTree as et

from JSBSim_utils import JSBSimTestCase, RunTest, FlightModel
from jsbsim import BaseError


class TestFunctions(JSBSimTestCase):
    def test_functions(self):
        # Containers for the sequence of random numbers issued by test/random
        # and test/urandom
        random = []
        urandom = []
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('function.xml')
        fdm = tripod.start()

        self.assertAlmostEqual(fdm['test/sum-values'], -1.5)
        self.assertAlmostEqual(fdm['test/sum-values-inverted-input-sign'], 0.0)
        self.assertAlmostEqual(fdm['test/sum-value-property'], 1.0)
        self.assertAlmostEqual(fdm['test/sum-properties'], 0.0)
        self.assertAlmostEqual(fdm['test/summer'], 0.0)
        self.assertAlmostEqual(fdm['test/summer-with-bias'], 1.0)
        self.assertAlmostEqual(fdm['test/product-values'], 2.0*math.pi)
        self.assertAlmostEqual(fdm['test/product-value-property'], 0.0)
        self.assertAlmostEqual(fdm['test/product-as-a-no-op'], 1.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], 0.0)
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/reference'] = 1.5
        fdm.run()

        self.assertAlmostEqual(fdm['test/sum-values'], -1.5)
        self.assertAlmostEqual(fdm['test/sum-values-inverted-input-sign'], 0.0)
        self.assertAlmostEqual(fdm['test/sum-value-property'], 2.5)
        self.assertAlmostEqual(fdm['test/sum-properties'], 1.5)
        self.assertAlmostEqual(fdm['test/summer'], 1.5)
        self.assertAlmostEqual(fdm['test/summer-with-bias'], 2.5)
        self.assertAlmostEqual(fdm['test/product-values'], 2.0*math.pi)
        self.assertAlmostEqual(fdm['test/product-value-property'], 2.25)
        self.assertAlmostEqual(fdm['test/product-as-a-no-op'], 2.5)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], 0.0)
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = -1.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/sum-values-inverted-input-sign'], -1.5)
        self.assertAlmostEqual(fdm['test/sum-properties'], 0.5)
        self.assertAlmostEqual(fdm['test/summer'], 0.5)
        self.assertAlmostEqual(fdm['test/interpolate1d'], -1.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(-1.0))
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/sum-values-inverted-input-sign'], 0.0)
        self.assertAlmostEqual(fdm['test/interpolate1d'], -1.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], 0.0)
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = 0.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/sum-values-inverted-input-sign'],
                               0.75)
        self.assertAlmostEqual(fdm['test/interpolate1d'], -0.5)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(0.5))
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = 1.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(1.0))
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = 1.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(1.5))
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = 3.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 0.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(3.0))
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = 3.5
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 1.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(3.5))
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = 4.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 2.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(4.0))
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        fdm['test/input'] = 5.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/interpolate1d'], 2.0)
        self.assertAlmostEqual(fdm['test/sin-value'], 0.5*math.sqrt(2.0))
        self.assertAlmostEqual(fdm['test/sin-property'], math.sin(5.0))
        random.append(fdm['test/random'])
        random.append(fdm['test/random2'])
        urandom.append(fdm['test/urandom'])

        # Since a set() removes duplicates, the length of "urandom" should be
        # equal to the number of steps if test/urandom has successfully issued
        # different numbers at each time step.
        self.assertEqual(len(set(urandom)), fdm['simulation/frame']+1)
        # Since the set "random" has cumulated the results of tests/random and
        # tests/random2 then its size must be equal to twice the number of time
        # steps.
        self.assertEqual(len(set(random)), 2*(fdm['simulation/frame']+1))

    def test_rotations(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('function.xml')
        fdm = tripod.start()

        for a in range(36):
            alpha = (a-17)*10
            for c in range(36):
                gamma = (c-17)*10
                fdm['test/alpha'] = alpha
                fdm['test/gamma'] = gamma
                fdm.run()
                self.assertAlmostEqual(fdm['test/alpha_local'], alpha)
                self.assertAlmostEqual(fdm['test/gamma_local'], gamma)

        fdm['test/alpha'] = 0.

        for b in range(17):
            beta = (b-8)*10
            for c in range(36):
                gamma = (c-17)*10
                fdm['test/beta'] = beta
                fdm['test/gamma'] = gamma
                fdm.run()
                self.assertAlmostEqual(fdm['test/beta_local'], beta)
                self.assertAlmostEqual(fdm['test/gamma_local'], gamma)

        fdm['test/alpha'] = 10.
        fdm['test/beta'] = 0.
        fdm['test/theta'] = -10.
        for c in range(36):
            gamma = (c-17)*10
            fdm['test/gamma'] = gamma
            fdm.run()
            self.assertAlmostEqual(fdm['test/alpha_local'], 0.0)
            self.assertAlmostEqual(fdm['test/beta_local'], 0.0)
            self.assertAlmostEqual(fdm['test/gamma_local'], gamma)

        fdm['test/alpha'] = 0.
        fdm['test/beta'] = 10.
        fdm['test/theta'] = 0.
        fdm['test/psi'] = 10.
        for c in range(36):
            gamma = (c-17)*10
            fdm['test/gamma'] = gamma
            fdm.run()
            self.assertAlmostEqual(fdm['test/alpha_local'], 0.0)
            self.assertAlmostEqual(fdm['test/beta_local'], 0.0)
            self.assertAlmostEqual(fdm['test/gamma_local'], gamma)

        alpha = 10.
        fdm['test/alpha'] = alpha
        fdm['test/beta'] =  5.
        fdm['test/phi'] = 0.
        fdm['test/psi'] = 0.
        for t in range(18):
            theta = (t-8)*10
            fdm['test/theta'] = theta
            for c in range(36):
                gamma = (c-17)*10
                fdm['test/gamma'] = gamma
                fdm.run()
                self.assertAlmostEqual(fdm['test/alpha_local'], alpha+theta)
                self.assertAlmostEqual(fdm['test/beta_local'], 5.0)
                self.assertAlmostEqual(fdm['test/gamma_local'], gamma)

    def CheckMatrix(self,fdm, M, transform):
        fdm['test/rx'] = 1.0
        fdm['test/ry'] = 0.0
        fdm['test/rz'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/'+transform+'/x'], M[0,0])
        self.assertAlmostEqual(fdm['test/'+transform+'/y'], M[1,0])
        self.assertAlmostEqual(fdm['test/'+transform+'/z'], M[2,0])

        fdm['test/rx'] = 0.0
        fdm['test/ry'] = 1.0
        fdm['test/rz'] = 0.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/'+transform+'/x'], M[0,1])
        self.assertAlmostEqual(fdm['test/'+transform+'/y'], M[1,1])
        self.assertAlmostEqual(fdm['test/'+transform+'/z'], M[2,1])

        fdm['test/rx'] = 0.0
        fdm['test/ry'] = 0.0
        fdm['test/rz'] = 1.0
        fdm.run()
        self.assertAlmostEqual(fdm['test/'+transform+'/x'], M[0,2])
        self.assertAlmostEqual(fdm['test/'+transform+'/y'], M[1,2])
        self.assertAlmostEqual(fdm['test/'+transform+'/z'], M[2,2])

    def test_wf_to_bf(self):
        tripod = FlightModel(self, 'tripod')
        tripod.include_system_test_file('function.xml')
        fdm = tripod.start()

        alpha = 10.5
        beta = -8.6
        gamma = 45.0

        a = alpha*math.pi/180.
        b = beta*math.pi/180.
        c = gamma*math.pi/180.

        Ma = np.matrix([[ math.cos(a), 0.0, math.sin(a)],
                        [              0.0, 1.0,     0.0],
                        [ -math.sin(a), 0.0,  math.cos(a)]])
        Mb = np.matrix([[  math.cos(b), math.sin(b), 0.0],
                        [ -math.sin(b), math.cos(b), 0.0],
                        [          0.0,         0.0, 1.0]])
        Mc = np.matrix([[ 1.0,         0.0,          0.0],
                        [ 0.0, math.cos(c), -math.sin(c)],
                        [ 0.0, math.sin(c),  math.cos(c)]])

        fdm['test/alpha'] = alpha
        self.CheckMatrix(fdm, Ma, 'bf_to_wf')
        self.CheckMatrix(fdm, Ma.T, 'wf_to_bf')

        fdm['test/alpha'] = 0.0
        fdm['test/beta'] = beta
        self.CheckMatrix(fdm, Mb, 'bf_to_wf')
        self.CheckMatrix(fdm, Mb.T, 'wf_to_bf')

        fdm['test/beta'] = 0.0
        fdm['test/gamma'] = gamma
        self.CheckMatrix(fdm, Mc, 'bf_to_wf')
        self.CheckMatrix(fdm, Mc.T, 'wf_to_bf')

        fdm['test/alpha'] = alpha
        fdm['test/beta'] = beta
        fdm['test/gamma'] = 0.0
        self.CheckMatrix(fdm, Mb*Ma, 'bf_to_wf')
        self.CheckMatrix(fdm, (Mb*Ma).T, 'wf_to_bf')

        fdm['test/alpha'] = alpha
        fdm['test/beta'] = 0.0
        fdm['test/gamma'] = gamma
        self.CheckMatrix(fdm, Mc*Ma, 'bf_to_wf')
        self.CheckMatrix(fdm, (Mc*Ma).T, 'wf_to_bf')

        fdm['test/alpha'] = 0.0
        fdm['test/beta'] = beta
        fdm['test/gamma'] = gamma
        self.CheckMatrix(fdm, Mc*Mb, 'bf_to_wf')
        self.CheckMatrix(fdm, (Mc*Mb).T, 'wf_to_bf')

        fdm['test/alpha'] = alpha
        fdm['test/beta'] = beta
        fdm['test/gamma'] = gamma
        self.CheckMatrix(fdm, Mc*Mb*Ma, 'bf_to_wf')
        self.CheckMatrix(fdm, (Mc*Mb*Ma).T, 'wf_to_bf')

    def test_table_exception(self):
        tripod = FlightModel(self, 'tripod')
        function_file = self.sandbox.path_to_jsbsim_file('tests/function.xml')
        tree = et.parse(function_file)
        root = tree.getroot()
        table_tag = root.find('./channel/fcs_function/function/table')
        table_tag.attrib['type'] = 'qsdfghjkl'
        function_file = self.sandbox('function.xml')
        tree.write(function_file)
        tripod.include_system_test_file(function_file)
        with self.assertRaises(BaseError):
            tripod.start()


RunTest(TestFunctions)
