#
# This file is part of GNUnet.
# Copyright (C) 2023 GNUnet e.V.
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

applications_dir = get_option('datadir') / 'applications'
appdata_dir = get_option('datadir') / 'appdata'
application_id = messenger_gtk_id

subdir('icon')

css_resources = gnome.compile_resources(
    'css',
    'css.gresource.xml',
)

ui_resources = gnome.compile_resources(
    'ui',
    'ui.gresource.xml',
)

messenger_gtk_resources = [
    css_resources,
    ui_resources,
]

install_data(
    application_id + '.desktop',
    install_dir: applications_dir,
)

install_data(
    application_id + '.appdata.xml',
    install_dir: appdata_dir,
)
