#!/usr/bin/env python3
"""
This file is part of Ascension.
Copyright (C) 2019 GNUnet e.V.

Ascension is free software: you can redistribute it and/or modify it
under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

Ascension is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

SPDX-License-Identifier: AGPL3.0-or-later

Author rexxnor
"""

import subprocess
import unittest
from unittest import TestCase
from ascension import ascension

class TestAscender(TestCase):
    """
    Short Unit Tests for WebArchiver
    """
    @classmethod
    def setUpClass(cls):
        """
        Prepare data for tests
        """
        cls.ascender = ascension.Ascender("notsopretty.fantasy",
                                          transferns="ns1.example.com",
                                          port=8000)

    def test_constructor(self):
        """
        Tests constructor
        """
        self.assertEqual("ns1.example.com", self.ascender.transferns)
        self.assertEqual(8000, self.ascender.port)

    @classmethod
    def tearDownClass(cls):
        """
        Removes all zones and cleans up testing environment
        """
        subprocess.run(['gnunet-identity', '-D', 'pretty.fantasy'])
        subprocess.run(['gnunet-identity', '-D', 'notsopretty.fantasy'])

if __name__ == "__main__":
    unittest.main()
