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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/service/ServiceFactory.kt
 */

package org.gnunet.gnunetmessenger.service

import android.content.Context
import org.gnunet.gnunetmessenger.service.boundimpl.GnunetChatBoundService
import org.gnunet.gnunetmessenger.service.mock.GnunetChatMock


object ServiceFactory {
    fun create(context: android.content.Context, useMock: Boolean): GnunetChat {
        return if (useMock) {
            GnunetChatMock()
        } else {
            // Delegator, der per AIDL auf den Remote-Service ruft
            GnunetChatBoundService(context.applicationContext)
        }
    }
}