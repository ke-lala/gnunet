#
# This file is part of GNUnet.
# Copyright (C) 2024 GNUnet e.V.
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

test_gnunet_chat_tag_contact = executable(
    'test_gnunet_chat_tag_contact.test',
    'test_gnunet_chat_tag_contact.c',
    dependencies: test_deps,
    link_with: gnunetchat_lib,
    include_directories: tests_include,
    extra_files: test_header,
)

test_gnunet_chat_tag_message = executable(
    'test_gnunet_chat_tag_message.test',
    'test_gnunet_chat_tag_message.c',
    dependencies: test_deps,
    link_with: gnunetchat_lib,
    include_directories: tests_include,
    extra_files: test_header,
)
