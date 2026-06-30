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

subdir('chat')
subdir('ui')

messenger_gtk_sources = files([
    'account.c', 'account.h',
    'application.c', 'application.h',
    'contact.c', 'contact.h',
    'discourse.c', 'discourse.h',
    'event.c', 'event.h',
    'file.c', 'file.h',
    'media.c', 'media.h',
    'request.c', 'request.h',
    'resources.c', 'resources.h',
    'schedule.c', 'schedule.h',
    'secret.c', 'secret.h',
    'ui.c', 'ui.h',
    'util.c', 'util.h',
    'messenger_gtk.c',
]) + messenger_gtk_chat_sources + messenger_gtk_ui_sources
