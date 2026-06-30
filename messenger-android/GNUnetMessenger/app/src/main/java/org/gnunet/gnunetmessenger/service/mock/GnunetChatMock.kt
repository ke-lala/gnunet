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
 * @file GNUnetMessenger/app/src/main/java/org/gnunet/gnunetmessenger/service/mock/GnunetChatMock.kt
 */

package org.gnunet.gnunetmessenger.service.mock

import org.gnunet.gnunetmessenger.model.ChatAccount
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatContextType
import org.gnunet.gnunetmessenger.model.ChatGroup
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.ChatUri
import org.gnunet.gnunetmessenger.model.GnunetReturnValue
import org.gnunet.gnunetmessenger.model.MessageKind
import org.gnunet.gnunetmessenger.model.MessengerApp
import org.gnunet.gnunetmessenger.service.GnunetChat
import java.util.UUID

class GnunetChatMock : GnunetChat {

    private lateinit var messageCallback: (ChatContext, ChatMessage) -> Unit
    private var uuidCounter: Long = 0

    var lastDestroyedUri: ChatUri? = null
        private set

    var lastSetUserPointer: Pair<ChatContact?, String?>? = null
        private set
    var lastSetGroupPointer: Pair<ChatGroup?, String?>? = null
        private set

    var lastSetMessageForGroupContact: Triple<ChatGroup?, ChatContact?, ChatMessage?>? = null
        private set

    override suspend fun awaitReady(handle: ChatHandle) {
        return
    }

    override fun startChat(
        messengerApp: MessengerApp,
        callback: (ChatContext, ChatMessage) -> Unit
    ): ChatHandle {
        println("start chat")
        messageCallback = callback
        return ChatHandle(1)
    }

    override fun iterateAccounts(handle: ChatHandle, callback: (ChatAccount) -> Unit) {
        val mockAccounts = listOf(
            ChatAccount(1, "Alice", ""),
            ChatAccount(2, "Bob", ""),
            ChatAccount(3, "Charlie", "")
        )

        for (account in mockAccounts) {
            callback(account)
        }
    }

    override suspend fun listAccounts(handle: ChatHandle): List<ChatAccount> {
        return listOf(
            ChatAccount(1, "Alice", ""),
            ChatAccount(2, "Bob", ""),
            ChatAccount(3, "Charlie", "")
        )
    }

    override suspend fun createAccount(handle: ChatHandle, name: String): GnunetReturnValue {
        println("create account")
        return GnunetReturnValue.OK
    }

    override suspend fun connect(handle: ChatHandle, account: ChatAccount) {
        println("connect")
        messageCallback(
            ChatContext(ChatContextType.UNKNOWN, UUID.randomUUID().toString(), false, false),
            ChatMessage(
                ChatContext(ChatContextType.UNKNOWN, null, true, false),
                "",
                0,
                null,
                MessageKind.LOGIN,
                null
            )
        )
    }

    override suspend fun disconnect(handle: ChatHandle) {
        println("disconnect")
        uuidCounter = 0
        messageCallback(
            ChatContext(ChatContextType.UNKNOWN, UUID.randomUUID().toString(), false, false),
            ChatMessage(
                ChatContext(ChatContextType.UNKNOWN, null, false, false),
                "",
                0,
                null,
                MessageKind.LOGOUT,
                null
            )
        )
    }

    override suspend fun stopChat(handle: ChatHandle) {
        println("stopChat")
    }

    override suspend fun getProfileName(handle: ChatHandle): String {
        return "somename" + (1000..9999).random()
    }

    override suspend fun setProfileName(handle: ChatHandle, name: String) {
        println("set profile name")
    }

    override fun getProfileKey(handle: ChatHandle): String {
        return "somekey1337"
    }

    override fun isContactBlocked(contact: ChatContact): Boolean {
        return true
    }

    override fun setContactBlocked(contact: ChatContact, isBlocked: Boolean) {
        println("isblocked:$isBlocked")
    }

    override fun setAttribute(handle: ChatHandle, key: String, value: String) {
        println("setting Attribute withj key: ${key} and value ${value}")
    }

    override fun getAttributes(handle: ChatHandle, callback: (String, String) -> Unit) {
        val mockAttributes = listOf(
            "nickname" to "Alice",
            "location" to "Berlin",
            "status" to "Online"
        )

        for ((key, value) in mockAttributes) {
            callback(key, value)
        }
    }

    override fun lobbyOpen(handle: ChatHandle, callback: (String) -> Unit) {
        callback("000G006K2TJNMD9VTCYRX7BRVV3HAEPS15E6NHDXKPJA1KAJJEG9AFF884")
    }

    override suspend fun lobbyJoin(handle: ChatHandle, uri: String) {
        println("join lobby")
    }

    override fun setGroupName(group: ChatGroup, name: String) {
        group.name = name
    }

    override fun createGroup(handle: ChatHandle, topic: String): ChatGroup {
        return ChatGroup(
            ChatContext(ChatContextType.GROUP, null, true, false),
            topic
        )
    }

    override fun parseUri(uri: String): ChatUri {
        return ChatUri(uri)
    }

    override fun destroyUri(uri: ChatUri) {
        lastDestroyedUri = uri
    }

    override fun inviteContactToGroup(group: ChatGroup, contact: ChatContact) {
        println("contact ${contact.name} invited")
    }

    override fun getUserPointerForContext(context: ChatContext): String? {
        return context.userPointer
    }

    override fun setUserPointerForContext(context: ChatContext, userPointer: String) {
        context.userPointer = userPointer
    }

    override fun getSenderFromMessage(message: ChatMessage): ChatContact {
        return ChatContact(
            ChatContext(ChatContextType.CONTACT, null, true, false),
            message.sender?.name ?: ""
        )
    }

    override fun getGroupFromContext(context: ChatContext): ChatGroup? {
        if ("3" == context.userPointer) {
            val contextDev = ChatContext(ChatContextType.GROUP, null, true, false)
            return ChatGroup(contextDev, name = "Dev Team")
        }
        val contextDev = ChatContext(ChatContextType.GROUP, null, true, false)
        return ChatGroup(contextDev, name = "GNUnet Team")
    }

    override fun getMessageForGroupContact(group: ChatGroup, contact: ChatContact): ChatMessage {
        return ChatMessage(
            ChatContext(null, null, false, false),
            "fake-message-for-group",
            12345L,
            contact,
            MessageKind.TEXT,
            null
        )
    }

    override fun getMessageKind(message: ChatMessage): MessageKind {
        return MessageKind.TEXT
    }

    override fun isMessageRecent(message: ChatMessage): GnunetReturnValue {
        return GnunetReturnValue.OK
    }

    override fun getMessageTimestamp(message: ChatMessage): Long {
        return 123456789L
    }

    override fun setMessageForGroupContact(
        group: ChatGroup,
        contact: ChatContact,
        message: ChatMessage
    ) {
        lastSetMessageForGroupContact = Triple(group, contact, message)
    }

    override fun iterateContacts(handle: ChatHandle, callback: (ChatContact) -> Int) {
        val contextAlice = ChatContext(ChatContextType.CONTACT, null, false, false)
        val contextBob = ChatContext(ChatContextType.CONTACT, null, false, false)
        val contacts = listOf(
            ChatContact(contextAlice, name = "Alice"),
            ChatContact(contextBob, name = "Bob")
        )
        for (contact in contacts) {
            callback(contact)
        }
    }

    override fun iterateGroups(handle: ChatHandle, callback: (ChatGroup) -> Int) {
        println("iterate groups")
        val contextDev = ChatContext(ChatContextType.GROUP, null, true, false)
        val contextFriends = ChatContext(ChatContextType.GROUP, null, true, false)
        val groups = listOf(
            ChatGroup(contextDev, name = "Dev Team"),
            ChatGroup(contextFriends, name = "Friends")
        )
        for (group in groups) {
            callback(group)
        }

        messageCallback(
            ChatContext(ChatContextType.CONTACT, null, true, false),
            ChatMessage(
                ChatContext(ChatContextType.CONTACT, null, true, false),
                "",
                0,
                ChatContact(ChatContext(ChatContextType.CONTACT, null, true, false), "Mallory"),
                MessageKind.JOIN,
                null
            )
        )

        messageCallback(
            ChatContext(ChatContextType.CONTACT, "5", true, false),
            ChatMessage(
                ChatContext(ChatContextType.CONTACT, null, true, false),
                "Hi, I am Mallory!",
                0,
                ChatContact(ChatContext(ChatContextType.CONTACT, "6", true, false), "Mallory"),
                MessageKind.TEXT,
                null
            )
        )

        messageCallback(
            ChatContext(ChatContextType.GROUP, "3", true, false),
            ChatMessage(
                ChatContext(ChatContextType.GROUP, null, true, false),
                "",
                0,
                ChatContact(ChatContext(ChatContextType.CONTACT, null, true, false), "Flo"),
                MessageKind.JOIN,
                null
            )
        )

        messageCallback(
            ChatContext(ChatContextType.GROUP, "3", true, false),
            ChatMessage(
                ChatContext(ChatContextType.GROUP, null, true, false),
                "Hi, I am Flo!",
                0,
                ChatContact(ChatContext(ChatContextType.CONTACT, "7", true, false), "Flo"),
                MessageKind.TEXT,
                null
            )
        )

        messageCallback(
            ChatContext(ChatContextType.GROUP, "3", true, false),
            ChatMessage(
                ChatContext(ChatContextType.GROUP, null, true, false),
                "",
                0,
                ChatContact(ChatContext(ChatContextType.CONTACT, "1", true, false), "Alice"),
                MessageKind.JOIN,
                null
            )
        )

        messageCallback(
            ChatContext(ChatContextType.GROUP, "3", true, false),
            ChatMessage(
                ChatContext(ChatContextType.GROUP, null, true, false),
                "Hi, I am Alice!",
                0,
                ChatContact(ChatContext(ChatContextType.CONTACT, "1", true, false), "Alice"),
                MessageKind.TEXT,
                null
            )
        )
    }

    override suspend fun listContacts(handle: ChatHandle): List<ChatContact> {
        val contextAlice = ChatContext(ChatContextType.CONTACT, null, false, false)
        val contextBob = ChatContext(ChatContextType.CONTACT, null, false, false)
        return listOf(
            ChatContact(contextAlice, name = "Alice"),
            ChatContact(contextBob, name = "Bob")
        )
    }

    override suspend fun listGroups(handle: ChatHandle): List<ChatGroup> {
        val contextDev = ChatContext(ChatContextType.GROUP, null, true, false)
        val contextFriends = ChatContext(ChatContextType.GROUP, null, true, false)
        return listOf(
            ChatGroup(contextDev, name = "Dev Team"),
            ChatGroup(contextFriends, name = "Friends")
        )
    }

    override fun getContactContext(chatContact: ChatContact): ChatContext {
        return chatContact.chatContext
    }

    override fun getGroupContext(chatGroup: ChatGroup): ChatContext {
        return chatGroup.chatContext
    }

    override fun getContactUserPointer(chatContact: ChatContact): String {
        return "fake-user-pointer-123"
    }

    override fun setContactUserPointer(chatContact: ChatContact, userPointer: String) {
        lastSetUserPointer = Pair(chatContact, userPointer)
    }

    override fun getGroupUserPointer(chatGroup: ChatGroup): String {
        return "fake-group-pointer-789"
    }

    override fun setGroupUserPointer(chatGroup: ChatGroup, userPointer: String) {
        lastSetGroupPointer = Pair(chatGroup, userPointer)
    }

    override fun sendText(chatContext: ChatContext, text: String) {
        println("send text: $text")
    }

    override fun getContactKey(chatContact: ChatContact): String {
        return "otherkey42"
    }

    override fun getContextContact(context: ChatContext): ChatContact {
        println("get contact for context")
        return ChatContact(context, "test")
    }

    override fun deleteContact(chatContact: ChatContact) {
        println("delete contact")
    }

    override fun isGroup(context: ChatContext): Boolean {
        return context.isGroup
    }

    override fun isPlatform(context: ChatContext): Boolean {
        return context.isPlatform
    }

    override fun iterateGroupContacts(
        chatGroup: ChatGroup,
        callback: (ChatGroup, ChatContact) -> Int
    ) {
        val contextCharlie = ChatContext(ChatContextType.CONTACT, null, true, false)
        val contacts = listOf(
            ChatContact(contextCharlie, name = "Charlie")
        )
        for (contact in contacts) {
            callback(chatGroup, contact)
        }
    }

    override suspend fun listGroupContacts(group: ChatGroup): List<ChatContact> {
        val contextCharlie = ChatContext(ChatContextType.CONTACT, null, true, false)
        return listOf(
            ChatContact(contextCharlie, name = "Charlie")
        )
    }

    override fun randomUUID(): String {
        return uuidCounter++.toString()
    }

    override fun getContactAttributes(
        contact: ChatContact,
        callback: (String, String) -> Unit
    ) {
        val mockAttributes = listOf(
            "nickname" to "Alice",
            "location" to "Berlin",
            "status" to "Online"
        )

        for ((key, value) in mockAttributes) {
            callback(key, value)
        }
    }

    override fun shareAttributes(handle: ChatHandle, contact: ChatContact, key: String) {
        println("share ${key} for contact ${contact.name}")
    }

    override fun unshareAttributes(handle: ChatHandle, contact: ChatContact, key: String) {
        println("unshare ${key} for contact ${contact.name}")
    }

    override suspend fun iterateContextMessages(context: ChatContext): List<ChatMessage> {
        return emptyList()
    }

    override suspend fun reset() {
        if (!org.gnunet.gnunetmessenger.BuildConfig.ALLOW_RESET) {
            println("reset: BLOCKED - Reset not allowed in production builds")
            throw SecurityException("Reset operation is not allowed in production")
        }

        println("reset mock service - clearing all state")
        uuidCounter = 0
        lastDestroyedUri = null
        lastSetUserPointer = null
        lastSetGroupPointer = null
        lastSetMessageForGroupContact = null
        messageCallback = { _, _ -> }
    }
}