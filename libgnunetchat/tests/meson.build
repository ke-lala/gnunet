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

test_deps = [
    check_dep,
    dependency('gnunetutil'),
]

test_header = '../test_gnunet_chat.h'

subdir('attribute')
subdir('discourse')
subdir('file')
subdir('group')
subdir('handle')
subdir('lobby')
subdir('message')
subdir('tag')

test('test_gnunet_chat_handle_init', test_gnunet_chat_handle_init, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_handle_accounts', test_gnunet_chat_handle_accounts, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_handle_no_secret', test_gnunet_chat_handle_no_secret, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_handle_connection', test_gnunet_chat_handle_connection, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_handle_update', test_gnunet_chat_handle_update, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_handle_rename', test_gnunet_chat_handle_rename, depends: gnunetchat_lib, is_parallel : false)

test('test_gnunet_chat_group_open', test_gnunet_chat_group_open, depends: gnunetchat_lib, is_parallel : false)

test('test_gnunet_chat_message_text', test_gnunet_chat_message_text, depends: gnunetchat_lib, is_parallel : false)

test('test_gnunet_chat_file_send', test_gnunet_chat_file_send, depends: gnunetchat_lib, is_parallel : false)

test('test_gnunet_chat_lobby_open', test_gnunet_chat_lobby_open, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_lobby_join', test_gnunet_chat_lobby_join, depends: gnunetchat_lib, is_parallel : false)

test('test_gnunet_chat_attribute_check', test_gnunet_chat_attribute_check, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_attribute_share', test_gnunet_chat_attribute_share, depends: gnunetchat_lib, is_parallel : false)

test('test_gnunet_chat_discourse_open', test_gnunet_chat_discourse_open, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_discourse_write', test_gnunet_chat_discourse_write, depends: gnunetchat_lib, is_parallel : false)

test('test_gnunet_chat_tag_contact', test_gnunet_chat_tag_contact, depends: gnunetchat_lib, is_parallel : false)
test('test_gnunet_chat_tag_message', test_gnunet_chat_tag_message, depends: gnunetchat_lib, is_parallel : false)
