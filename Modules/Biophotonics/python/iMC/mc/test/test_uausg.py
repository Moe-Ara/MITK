'''
Created on Sep 8, 2015

@author: wirkert
'''

import unittest

from mc.usuag import Ua, UsG


class test_ua(unittest.TestCase):

    def setUp(self):
        self.ua = Ua()
        self.ua532 = self.ua(532.*10 ** -9) / 100.
        self.ua800 = self.ua(800.*10 ** -9) / 100.

    def test_uA532(self):
        self.assertTrue(3. < self.ua532 < 4., "test if calculated ua takes " +
                        "reasonable values " +
                        "(according to \"Determination of optical" +
                        " properties of normal and adenomatous human colon " +
                        "tissues in vitro using integrating sphere " +
                        "techniques\")")

    def test_uA800(self):
        self.assertTrue(0.05 < self.ua800 < 0.15, "test if calculated ua " +
                        "takes reasonable values " +
                        "(according to \"Differences in" +
                        " optical properties between healthy and " +
                        "pathological human colon tissues using a Ti:sapphire" +
                        " laser: an in vitro study using the Monte Carlo " +
                        "inversion technique\")")

    def test_saO2_makes_difference(self):
        self.ua.saO2 = 1.0
        self.assertNotAlmostEqual(self.ua532,
                                  self.ua(532.*10 ** -9) / 100.,
                                  msg="changing oxygenation changes result")


class test_us(unittest.TestCase):

    def setUp(self):
        usg = UsG()
        usg.dsp = 0.015
        self.usg470 = usg(470.*10 ** -9)
        self.usg700 = usg(700.*10 ** -9)

    def test_us470nm(self):
        reduced_us470nm = self.usg470[0] * (1 - self.usg470[1]) / 100.
        self.assertTrue(5. < reduced_us470nm < 10.,
                        "test if calculated us' can take reasonable values"
                        + "(according to \"Ex Vivo Optical Properties of"
                        + " Human Colon Tissue\"). Calculated value is " +
                        str(reduced_us470nm) + "1 / cm")


    def test_us700nm(self):
        reduced_us700nm = self.usg700[0] * (1 - self.usg700[1]) / 100.
        self.assertTrue(5. < reduced_us700nm < 10.,
                        "test if calculated us' can take reasonable values" +
                        "(according to \"Ex Vivo Optical Properties of " +
                        "Human Colon Tissue\")." +
                        " Calculated value is " + str(reduced_us700nm) +
                        "1 / cm")
