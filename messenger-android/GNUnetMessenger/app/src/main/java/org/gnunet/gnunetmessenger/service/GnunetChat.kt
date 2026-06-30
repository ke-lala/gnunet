package org.gnunet.gnunetmessenger.service

import org.gnunet.gnunetmessenger.model.ChatAccount
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatGroup
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.ChatUri
import org.gnunet.gnunetmessenger.model.GnunetReturnValue
import org.gnunet.gnunetmessenger.model.MessageKind
import org.gnunet.gnunetmessenger.model.MessengerApp

interface GnunetChat {
    fun startChat(
        messengerApp: MessengerApp,
        callback: (ChatContext, ChatMessage) -> Unit
    ): ChatHandle

    suspend fun awaitReady(handle: ChatHandle)

    suspend fun reset()

    fun iterateAccounts(handle: ChatHandle, callback: (ChatAccount) -> Unit)
    suspend fun listAccounts(handle: ChatHandle): List<ChatAccount>

    suspend fun createAccount(handle: ChatHandle, name: String): GnunetReturnValue
    suspend fun connect(handle: ChatHandle, account: ChatAccount)
    suspend fun disconnect(handle: ChatHandle)
    suspend fun stopChat(handle: ChatHandle)
    suspend fun getProfileName(handle: ChatHandle): String
    suspend fun setProfileName(handle: ChatHandle, name: String)

    fun getProfileKey(handle: ChatHandle): String
    fun isContactBlocked(contact: ChatContact): Boolean
    fun setContactBlocked(contact: ChatContact, isBlocked: Boolean)
    fun setAttribute(handle: ChatHandle, key: String, value: String)
    fun getAttributes(handle: ChatHandle, callback: (String, String) -> Unit)

    fun lobbyOpen(handle: ChatHandle, callback: (String) -> Unit)
    suspend fun lobbyJoin(handle: ChatHandle, uri: String)

    fun setGroupName(group: ChatGroup, name: String)
    fun createGroup(handle: ChatHandle, topic: String): ChatGroup

    fun parseUri(uri: String): ChatUri
    fun destroyUri(uri: ChatUri)

    fun inviteContactToGroup(group: ChatGroup, contact: ChatContact)
    fun getUserPointerForContext(context: ChatContext): String?
    fun setUserPointerForContext(context: ChatContext, userPointer: String)

    fun getSenderFromMessage(message: ChatMessage): ChatContact
    fun getGroupFromContext(context: ChatContext): ChatGroup?
    fun getMessageForGroupContact(group: ChatGroup, contact: ChatContact): ChatMessage
    fun getMessageKind(message: ChatMessage): MessageKind
    fun isMessageRecent(message: ChatMessage): GnunetReturnValue
    fun getMessageTimestamp(message: ChatMessage): Long
    fun setMessageForGroupContact(group: ChatGroup, contact: ChatContact, message: ChatMessage)

    fun iterateContacts(handle: ChatHandle, callback: (ChatContact) -> Int)
    fun iterateGroups(handle: ChatHandle, callback: (ChatGroup) -> Int)
    suspend fun listContacts(handle: ChatHandle): List<ChatContact>
    suspend fun listGroups(handle: ChatHandle): List<ChatGroup>

    fun getContactContext(chatContact: ChatContact): ChatContext
    fun getGroupContext(chatGroup: ChatGroup): ChatContext
    fun getContactUserPointer(chatContact: ChatContact): String
    fun setContactUserPointer(chatContact: ChatContact, userPointer: String)
    fun getGroupUserPointer(chatGroup: ChatGroup): String
    fun setGroupUserPointer(chatGroup: ChatGroup, userPointer: String)

    fun sendText(chatContext: ChatContext, text: String)
    fun getContactKey(chatContact: ChatContact): String
    fun getContextContact(context: ChatContext): ChatContact
    fun deleteContact(chatContact: ChatContact)
    fun isGroup(context: ChatContext): Boolean
    fun isPlatform(context: ChatContext): Boolean

    fun iterateGroupContacts(chatGroup: ChatGroup, callback: (ChatGroup, ChatContact) -> Int)
    suspend fun listGroupContacts(group: ChatGroup): List<ChatContact>

    fun randomUUID(): String
    fun getContactAttributes(contact: ChatContact, callback: (String, String) -> Unit)
    fun shareAttributes(handle: ChatHandle, contact: ChatContact, key: String)
    fun unshareAttributes(handle: ChatHandle, contact: ChatContact, key: String)

    suspend fun iterateContextMessages(context: ChatContext): List<ChatMessage>
}