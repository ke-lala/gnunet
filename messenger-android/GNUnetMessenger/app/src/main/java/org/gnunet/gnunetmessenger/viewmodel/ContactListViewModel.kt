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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/viewmodel/ContactListViewModel.kt
 */

package org.gnunet.gnunetmessenger.viewmodel

import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import org.gnunet.gnunetmessenger.model.ChatContact

class ContactListViewModel : ViewModel() {

    private val _contacts = MutableLiveData<List<ChatContact>>()
    val contacts: LiveData<List<ChatContact>> = _contacts

    fun setContacts(newContacts: List<ChatContact>) {
        _contacts.value = newContacts
    }

    fun clearModel(){
        _contacts.value = emptyList()
    }
}