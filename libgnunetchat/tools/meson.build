#
# This file is part of GNUnet.
# Copyright (C) 2024--2025 GNUnet e.V.
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

libgnunetchat_uml = executable(
  'libgnunetchat_uml',
  [ 'gnunet_chat_lib_uml.c' ],
  dependencies: [
    dependency('gnunetutil')
  ],
  link_with: gnunetchat_lib,
  include_directories: tools_include,
)

messenger_uml = executable(
  'messenger_uml',
  [ 'gnunet_messenger_uml.c' ],
  dependencies: [
    dependency('gnunetidentity'),
    dependency('gnunetmessenger'),
    dependency('gnunetutil')
  ],
)

messenger_ping = executable(
  'messenger_ping',
  [ 'gnunet_messenger_ping.c' ],
  dependencies: [
    dependency('gnunetidentity'),
    dependency('gnunetmessenger'),
    dependency('gnunetutil')
  ],
  link_args: '-lm'
)
