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

theme = 'hicolor'
theme_dir = get_option('datadir') / 'icons' / theme
theme_icon_id = messenger_gtk_id
theme_icon_sizes = [
    512,
    256,
    128,
    64,
    32,
]

foreach icon_size : theme_icon_sizes
    icon_size_dir = icon_size.to_string() + 'x' + icon_size.to_string()
    install_data(
        'full_color_' + icon_size.to_string() + '.png',
        install_dir: theme_dir,
        rename: icon_size_dir / 'apps' / theme_icon_id + '.png',
    )
endforeach

install_data(
    'full_color.svg',
    install_dir: theme_dir,
    rename: 'scalable/apps' / theme_icon_id + '.svg',
)
