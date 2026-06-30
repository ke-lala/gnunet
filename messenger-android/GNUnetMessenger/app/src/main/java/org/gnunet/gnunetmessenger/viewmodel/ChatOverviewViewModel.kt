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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/ui/viewmodel/ChatOverviewViewModel.kt
 */

package org.gnunet.gnunetmessenger.viewmodel

import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.ChatSummary

class ChatOverviewViewModel : ViewModel() {

    val chats = MutableLiveData<List<ChatSummary>>(emptyList())


    fun setChats(newChats: List<ChatSummary>) {
        chats.value = newChats
    }

    fun updateChatMessage(chatContext: ChatContext, chatMessage: ChatMessage) {
        val updated = chats.value.orEmpty().map {
            if (it.chatContext.userPointer == chatContext.userPointer)
                it.copy(lastMessagePreview = chatMessage.text)
            else
                it
        }
        chats.value = updated
    }

    fun addOrUpdateChat(chat: ChatSummary) {
        val current = chats.value.orEmpty().toMutableList()
        val index = current.indexOfFirst { it.chatContext.userPointer == chat.chatContext.userPointer }
        if (index >= 0) {
            current[index] = chat
        } else {
            current.add(chat)
        }
        chats.value = current
    }

    fun clearModel(){
        chats.value = emptyList()
    }

    fun removeChat(chat: ChatSummary) {
        chats.value = chats.value?.filter { it.chatContext.userPointer != chat.chatContext.userPointer }
    }
}