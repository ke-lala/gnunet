package org.gnunet.gnunetmessenger.service.mock

import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.MessageKind
import org.gnunet.gnunetmessenger.model.ChatUri
import org.gnunet.gnunetmessenger.model.GnunetReturnValue
import org.gnunet.gnunetmessenger.model.ChatContact
import org.gnunet.gnunetmessenger.model.ChatGroup
import org.junit.Assert.assertEquals
import org.junit.Test

class GnunetChatMockTest {

    @Test
    fun iterateAccountsReturnsTheHardcodedAccountNames() {
        val mock = GnunetChatMock()

        val fakeHandle = ChatHandle(1)

        val resultingNames = mutableListOf<String>()

        mock.iterateAccounts(fakeHandle) { account ->
            resultingNames.add(account.name)
        }

        val expectedNames = listOf("Alice", "Bob", "Charlie")

        assertEquals(3, resultingNames.size)
        assertEquals(expectedNames, resultingNames)
    }

    @Test
    fun getMessageTimestampReturnsTheHardcodedTimestamp() {
        val mock = GnunetChatMock()

        val fakeMessage = ChatMessage(ChatContext(null, null, false, false), "", 0, null, MessageKind.TEXT, null)
        val resultTimestamp = mock.getMessageTimestamp(fakeMessage)

        assertEquals(123456789L, resultTimestamp)
    }

    @Test
    fun parseUriReturnsAChatUriObjectWithTheCorrectUri() {
        val mock = GnunetChatMock()
        val testUriString = "gnunet://example-uri/12345"

        val resultChatUri = mock.parseUri(testUriString)

        assertEquals(testUriString, resultChatUri.error)
    }

    @Test
    fun isMessageRecentAlwaysReturnsOk() {
        val mock = GnunetChatMock()

        val fakeMessage = ChatMessage(ChatContext(null, null, false, false), "", 0, null, MessageKind.TEXT, null)

        val result = mock.isMessageRecent(fakeMessage)

        assertEquals(GnunetReturnValue.OK, result)
    }

    @Test
    fun getMessageKindAlwaysReturnsText() {
        val mock = GnunetChatMock()

        val fakeMessage = ChatMessage(ChatContext(null, null, false, false), "", 0, null, MessageKind.TEXT, null)

        val result = mock.getMessageKind(fakeMessage)

        assertEquals(MessageKind.TEXT, result)
    }

    @Test
    fun destroyUriSetsTheLastDestroyedUriProperty() {
        val mock = GnunetChatMock()
        val fakeUri = ChatUri("gnunet://test-uri-to-destroy")

        mock.destroyUri(fakeUri)

        assertEquals(fakeUri, mock.lastDestroyedUri)
    }

    @Test
    fun getContactUserPointerReturnsAHardcodedString() {
        val mock = GnunetChatMock()

        val fakeContact = ChatContact(ChatContext(null, null, false, false), "")

        val result = mock.getContactUserPointer(fakeContact)

        assertEquals("fake-user-pointer-123", result)
    }

    @Test
    fun setContactUserPointerSetsTheLastSetUserPointerProperty() {
        val mock = GnunetChatMock()
        val fakeContact = ChatContact(ChatContext(null, null, false, false), "test-contact")
        val fakePointer = "test-pointer-456"

        mock.setContactUserPointer(fakeContact, fakePointer)

        assertEquals(Pair(fakeContact, fakePointer), mock.lastSetUserPointer)
    }

    @Test
    fun getGroupUserPointerReturnsAHardcodedString() {
        val mock = GnunetChatMock()

        val fakeGroup = ChatGroup(ChatContext(null, null, false, false), "")

        val result = mock.getGroupUserPointer(fakeGroup)

        assertEquals("fake-group-pointer-789", result)
    }

    @Test
    fun setGroupUserPointerSetsTheLastSetGroupPointerProperty() {
        val mock = GnunetChatMock()
        val fakeGroup = ChatGroup(ChatContext(null, null, false, false), "test-group")
        val fakePointer = "test-group-pointer-xyz"

        mock.setGroupUserPointer(fakeGroup, fakePointer)

        assertEquals(Pair(fakeGroup, fakePointer), mock.lastSetGroupPointer)
    }

    @Test
    fun getMessageForGroupContactReturnsAHardcodedChatMessage() {

        val mock = GnunetChatMock()
        val fakeGroup = ChatGroup(ChatContext(null, null, false, false), "test-group")
        val fakeContact = ChatContact(ChatContext(null, null, false, false), "test-contact")

        val resultMessage = mock.getMessageForGroupContact(fakeGroup, fakeContact)

        assertEquals("fake-message-for-group", resultMessage.text)
        assertEquals(MessageKind.TEXT, resultMessage.kind)
        assertEquals(fakeContact, resultMessage.sender) // Check it attached the contact
    }

    @Test
    fun setMessageForGroupContactSetsTheLastSetMessageForGroupContactProperty() {
        val mock = GnunetChatMock()
        val fakeGroup = ChatGroup(ChatContext(null, null, false, false), "test-group")
        val fakeContact = ChatContact(ChatContext(null, null, false, false), "test-contact")
        val fakeMessage = ChatMessage(ChatContext(null, null, false, false), "test-msg", 0, null, MessageKind.TEXT, null)


        mock.setMessageForGroupContact(fakeGroup, fakeContact, fakeMessage)

        assertEquals(Triple(fakeGroup, fakeContact, fakeMessage), mock.lastSetMessageForGroupContact)
    }
}