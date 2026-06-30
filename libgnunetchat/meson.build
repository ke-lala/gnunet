#
# This file is part of GNUnet.
# Copyright (C) 2023--2026 GNUnet e.V.
#
# GNUnet is free software: you can redistribute it and/or modify it
# under the terms of the GNU Affero General Public License as published
# by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# GNUnet is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: AGPL3.0-or-later
#

project(
    'libgnunetchat', 
    'c',
    license: 'AGPL3.0-or-later',
    version: run_command('contrib/get_version.sh').stdout().strip(),
    meson_version: '>= 1.1.0',
)

meson.add_dist_script('contrib/distribute_version.sh', meson.project_version())

pkg = import('pkgconfig')

src_include = include_directories('src', 'include/gnunet')
tests_include = include_directories('tests', 'include')
tools_include = include_directories('include')

gnunetchat_deps = [
    dependency('gnunetarm'),
    dependency('gnunetfs'),
    dependency('gnunetgns'),
    dependency('gnunetgnsrecord'),
    dependency('gnunethello'),
    dependency('gnunetidentity'),
    dependency('gnunetmessenger'),
    dependency('gnunetnamestore'),
    dependency('gnunetpils'),
    dependency('gnunetreclaim'),
    dependency('gnunetregex'),
    dependency('gnunetutil'),
]

subdir('include')
subdir('src')

gnunetchat_lib = library(
    'gnunetchat',
    gnunetchat_sources,
    install: true,
    dependencies: gnunetchat_deps,
    include_directories: src_include,
    extra_files: gnunetchat_internal,
)

pkg.generate(
    gnunetchat_lib,
    description: 'A client-side library for applications to use the Messenger service of GNUnet',
)

check_dep = dependency('check', required: false)

if check_dep.found()
  subdir('tests')
endif

subdir('tools')

run_target(
    'docs', 
    command: 'doc/generate.sh',
)
