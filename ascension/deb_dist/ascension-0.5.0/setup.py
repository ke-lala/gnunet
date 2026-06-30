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

import setuptools

with open("README", "r") as fh:
    long_description = fh.read()

setuptools.setup(
    name="ascension",
    version="0.5.0",
    author="rexxnor",
    author_email="rexxnor+gnunet@brief.li",
    description="Tool to migrate DNS Zones to the GNU Name System",
    long_description=long_description,
    url="https://gnunet.org/git/ascension.git/",
    packages=['ascension'],
    data_files=[('man/man1', ['ascension.1'])],
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    entry_points={
        'console_scripts': [
            'ascension=ascension.ascension:main',
        ],
    },
    install_requires=[
        'coverage',
        'dnspython',
        'docopt',
        'mock',
        'pbr',
        'six',
    ],
)
