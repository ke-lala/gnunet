/*
   This file is part of GNUnet.
   Copyright (C) 2021--2025 GNUnet e.V.

   GNUnet is free software: you can redistribute it and/or modify it
   under the terms of the GNU Affero General Public License as published
   by the Free Software Foundation, either version 3 of the License,
   or (at your option) any later version.

   GNUnet is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   SPDX-License-Identifier: AGPL3.0-or-later
 */
/*
 * @author t3sserakt
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/model/MessageKind.kt
 */

package org.gnunet.gnunetmessenger.model

enum class MessageKind(val code: Int) {
    UNKOWN (code = 0),WARNING(1), REFRESH(2), LOGIN(3), LOGOUT(4),
    CREATED_ACCOUNT(5), DELETE_ACCOUNT(code = 6), UPDATE_ACCOUNT(7),
    UPDATE_CONTEXT(8), JOIN(9), LEAVE(10),
    CONTACT(11),
    INVITATION(12), TEXT(13), FILE(14),
    DELETION(15), TAG(16), ATTRIBUTES(17), SHARED_ATTRIBUTES(18),
    DISCOURSE(19), DATA(20);

    companion object {
        fun fromCode(code: Int): MessageKind =
            entries.firstOrNull { it.code == code } ?: WARNING
    }
}
