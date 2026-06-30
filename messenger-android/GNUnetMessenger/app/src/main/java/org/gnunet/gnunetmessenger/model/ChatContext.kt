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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/model/ChatContext.kt
 */

package org.gnunet.gnunetmessenger.model

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class ChatContext (
    val chatContextType: ChatContextType?,
    var userPointer: String?,
    val isGroup: Boolean,
    val isPlatform: Boolean,
    // Native GNUNET_CHAT_Context* (as decimal string) provided by the daemon.
    // Stays stable even if [userPointer] is rewritten client-side for keying.
    var nativeContextPointer: String? = null
): Parcelable

