#
# This file is part of GNUnet.
# Copyright (C) 2023--2024 GNUnet e.V.
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
    'messenger-gtk',
    'c',
    license: 'AGPL3.0-or-later',
    version: run_command('contrib/get_version.sh').stdout().strip(),
    meson_version: '>= 1.1.0',
    default_options: [
        'use_libportal=true',
        'use_eventfd=true'
    ]
)

meson.add_dist_script('contrib/distribute_version.sh', meson.project_version())

use_libportal = get_option('use_libportal')
use_eventfd = get_option('use_eventfd')

messenger_gtk_id = 'org.gnunet.Messenger'

gnome = import('gnome')

src_resources = include_directories('src', 'resources')

subdir('submodules')

messenger_gtk_deps = [
    dependency('gnunetchat'),
    dependency('gnunetutil'),
    dependency('glib-2.0'),
    dependency('gtk+-3.0'),
    dependency('libhandy-1'),
    dependency('gstreamer-1.0'),
    dependency('gstreamer-rtp-1.0'),
    dependency('libnotify'),
    dependency('libqrencode'),
    dependency('libpipewire-0.3'),
    dependency('libsecret-1'),
    declare_dependency(link_args: '-lunistring'),
]

messenger_gtk_args = [
    '-DENABLE_NLS=1',
    '-DMESSENGER_APPLICATION_BINARY="@0@"'.format(meson.project_name()),
    '-DMESSENGER_APPLICATION_ID="@0@"'.format(messenger_gtk_id),
    '-DMESSENGER_APPLICATION_VERSION="@0@"'.format(meson.project_version()),
]

if use_libportal
    messenger_gtk_deps += [
        dependency('libportal'),
        dependency('libportal-gtk3'),
    ]
else
    messenger_gtk_args += [
        '-DMESSENGER_APPLICATION_NO_PORTAL=1',
    ]
endif

if use_eventfd
else
    messenger_gtk_args += [
        '-DMESSENGER_APPLICATION_NO_EVENT_FD=1',
    ]
endif

subdir('po')
subdir('resources')
subdir('src')

messenger_gtk_exec = executable(
    meson.project_name(),
    messenger_gtk_resources + messenger_gtk_sources,
    c_args: messenger_gtk_args,
    install: true,
    dependencies: messenger_gtk_deps,
    extra_files: submodules_headers,
    include_directories: [
        src_resources, 
        submodules_includes,
    ],
)

gnome.post_install(
    gtk_update_icon_cache: true,
    update_desktop_database: true,
)
