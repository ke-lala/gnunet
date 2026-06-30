package org.gnunet.gnunetmessenger.ipc

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.core.app.ApplicationProvider
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.test.runTest
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeout
import org.gnunet.gnunetmessenger.model.ChatAccount
import org.gnunet.gnunetmessenger.model.ChatContext
import org.gnunet.gnunetmessenger.model.ChatMessage
import org.gnunet.gnunetmessenger.model.ChatHandle
import org.gnunet.gnunetmessenger.model.ChatGroup
import org.gnunet.gnunetmessenger.model.GnunetReturnValue
import org.gnunet.gnunetmessenger.model.MessengerApp
import org.gnunet.gnunetmessenger.service.GnunetChat
import org.gnunet.gnunetmessenger.service.boundimpl.GnunetChatBoundService
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assert.assertNotNull
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import kotlinx.coroutines.delay

@RunWith(AndroidJUnit4::class)
class GnunetChatRemoteTest {

    private val appContext = ApplicationProvider.getApplicationContext<android.content.Context>()
    private val gnunetChat: GnunetChatBoundService = GnunetChatBoundService(appContext)
    private var messageLog = mutableListOf<Pair<ChatContext, ChatMessage>>()

    @Before
    fun setUp() {
        messageLog.clear()
    }

    @After
    fun tearDown() = runTest {
        // sauber vom Service abmelden
        gnunetChat.unbind()
        // Wait for unbind to complete and clean up
        delay(1000)
    }

    @Test
    fun startChat_and_getProfileName_works() = runTest {
        // 1. Chat starten – callback ignorieren wir erstmal
        val handle: ChatHandle = gnunetChat.startChat(
            messengerApp = MessengerApp()
        ) { ctx: ChatContext, msg: ChatMessage ->
            // Messages sammeln für Tests
            messageLog.add(ctx to msg)
        }

        // 2. Wait for handle to be initialized with pointer from async operation
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }
        
        // 3. Verify handle is ready
        assertTrue("Handle.pointer sollte != 0 sein", handle.pointer != 0L)

        // 4. Remote-Call auf die Server-App: getProfileName()
        val profileName = gnunetChat.getProfileName(handle)

        // Aktuell sollte der Default-Name aus deiner Session-Struktur kommen
        assertEquals("GNUnet", profileName)
    }

    @Test
    fun createAccount_then_iterateAccounts_sees_it() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }

        val name = "MyTestAccount"

        val result = gnunetChat.createAccount(handle, name)
        assertEquals(GnunetReturnValue.OK, result)

        val accounts = mutableListOf<ChatAccount>()

        // iterateAccounts ist nicht suspend -> wir warten mit Timeout,
        // bis mindestens ein Account mit passendem Namen im Callback war.
        gnunetChat.iterateAccounts(handle) { acc ->
            accounts += acc
        }

        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(3_000) {
                while (accounts.none { it.name == name }) {
                    delay(50)
                }
            }
        }

        assertTrue(accounts.any { it.name == name })
    }

    @Test
    fun testMultipleAccountCreation() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }

        val accountNames = listOf("Account1", "Account2", "Account3")
        
        // Create multiple accounts
        accountNames.forEach { name ->
            val result = gnunetChat.createAccount(handle, name)
            assertEquals("Failed to create account: $name", GnunetReturnValue.OK, result)
        }

        // Verify all accounts are returned
        val accounts = mutableListOf<ChatAccount>()
        gnunetChat.iterateAccounts(handle) { acc ->
            accounts += acc
        }

        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (accounts.size < accountNames.size) {
                    delay(50)
                }
            }
        }

        accountNames.forEach { name ->
            assertTrue("Account $name not found in iteration", 
                accounts.any { it.name == name })
        }
    }

    @Test
    fun testProfileNameUpdate() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }

        val newName = "TestProfileName"
        
        // Update profile name
        gnunetChat.setProfileName(handle, newName)
        
        // Verify update
        val retrievedName = gnunetChat.getProfileName(handle)
        assertEquals(newName, retrievedName)
    }

    @Test
    fun testGroupCreation() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }

        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(30_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }
        val groupName = "TestGroup"
        
        // Create group
        val group = gnunetChat.createGroup(handle, groupName)
        assertNotNull("Group should not be null", group)
        assertEquals(groupName, group.name)
        
        // Verify group appears in iteration
        val groups = mutableListOf<ChatGroup>()
        gnunetChat.iterateGroups(handle) { grp ->
            groups += grp
            0  // Return GNUNet_OK (Int as required by interface)
        }
        
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(3_000) {
                while (groups.none { it.name == groupName }) {
                    delay(50)
                }
            }
        }
        
        assertTrue("Group $groupName not found in iteration", 
            groups.any { it.name == groupName })
    }

    @Test
    fun testContactsIteration() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }
        
        // Wait a bit for any initial messages to clear
        delay(500)

        val contacts = mutableListOf<String>()
        
        gnunetChat.iterateContacts(handle) { contact ->
            contacts.add(contact.name ?: "Unknown")
            0  // Return GNUNet_OK (Int as required by interface)
        }
        
        // Wait longer for contacts to arrive - server needs to process the iteration
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                var attempts = 0
                while (contacts.isEmpty() && attempts < 100) {
                    delay(50)
                    attempts++
                }
            }
        }
        
        assertTrue("Should receive at least one contact, got: $contacts", contacts.isNotEmpty())
    }

    @Test
    fun testAttributeOperations() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }

        // Set attribute
        val testKey = "test_key"
        val testValue = "test_value"
        gnunetChat.setAttribute(handle, testKey, testValue)
        
        // Get attributes and verify
        val attributes = mutableListOf<Pair<String, String>>()
        gnunetChat.getAttributes(handle) { key, value ->
            attributes.add(key to value)
        }
        
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(3_000) {
                while (attributes.none { it.first == testKey }) {
                    delay(50)
                }
            }
        }
        
        assertTrue("Attribute $testKey not found", 
            attributes.any { it.first == testKey && it.second == testValue })
    }

    @Test
    fun testMessageReception() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }

        // Create account which should trigger message
        val accountName = "MessageTestAccount"
        gnunetChat.createAccount(handle, accountName)
        
        // Wait for message to be received
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(3_000) {
                while (messageLog.isEmpty()) {
                    delay(50)
                }
            }
        }
        
        assertTrue("Should receive at least one message", messageLog.isNotEmpty())
        val (ctx, msg) = messageLog.first()
        assertNotNull("Message context should not be null", ctx)
        assertNotNull("Message should not be null", msg)
        assertNotNull("Message text should not be null", msg.text)
    }

    @Test
    fun testLobbyOperations() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }

        var lobbyUri = ""
        
        gnunetChat.lobbyOpen(handle) { uri ->
            lobbyUri = uri
        }
        
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(3_000) {
                while (lobbyUri.isEmpty()) {
                    delay(50)
                }
            }
        }
        
        assertTrue("Lobby URI should not be empty", lobbyUri.isNotEmpty())
    }

    @Test
    fun testResetClearsAllData() = runTest {
        val handle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (handle.pointer == 0L) {
                    delay(100)
                }
            }
        }

        // Create some test data
        val accountName = "TestAccountReset"
        val groupName = "TestGroupReset"
        val profileName = "TestProfileReset"
        
        // Create account
        val createResult = gnunetChat.createAccount(handle, accountName)
        assertEquals("Account creation should succeed", GnunetReturnValue.OK, createResult)
        
        // Create group
        val group = gnunetChat.createGroup(handle, groupName)
        assertNotNull("Group should be created", group)
        assertEquals(groupName, group.name)
        
        // Set profile name
        gnunetChat.setProfileName(handle, profileName)
        assertEquals("Profile name should be set", profileName, gnunetChat.getProfileName(handle))
        
        // Verify data exists before reset
        val accountsBefore = mutableListOf<ChatAccount>()
        gnunetChat.iterateAccounts(handle) { acc ->
            accountsBefore += acc
        }
        
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(3_000) {
                while (accountsBefore.none { it.name == accountName }) {
                    delay(50)
                }
            }
        }
        
        assertTrue("Account should exist before reset", 
            accountsBefore.any { it.name == accountName })
        
        val groupsBefore = mutableListOf<ChatGroup>()
        gnunetChat.iterateGroups(handle) { grp ->
            groupsBefore += grp
            0
        }
        
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(3_000) {
                while (groupsBefore.none { it.name == groupName }) {
                    delay(50)
                }
            }
        }
        
        assertTrue("Group should exist before reset", 
            groupsBefore.any { it.name == groupName })
        
        // Perform reset
        gnunetChat.reset()
        
        // Note: The local ChatHandle object's pointer is not automatically set to 0
        // because it's a local variable. The reset clears server state, but we need
        // to start a new session with a fresh handle.
        
        // Start a new session after reset
        val newHandle = gnunetChat.startChat(MessengerApp()) { ctx, msg ->
            messageLog.add(ctx to msg)
        }
        
        // Wait for new handle to be initialized
        withContext(Dispatchers.Default.limitedParallelism(1)) {
            withTimeout(5_000) {
                while (newHandle.pointer == 0L) {
                    delay(100)
                }
            }
        }
        
        // Verify old data is gone
        val accountsAfter = mutableListOf<ChatAccount>()
        gnunetChat.iterateAccounts(newHandle) { acc ->
            accountsAfter += acc
        }
        
        // Give time for iteration to complete
        delay(500)
        
        // The created account should be gone (only default mock accounts remain)
        assertTrue("Created account should not exist after reset", 
            !accountsAfter.any { it.name == accountName })
        
        // Profile should be back to default
        val defaultProfile = gnunetChat.getProfileName(newHandle)
        assertTrue("Profile should be reset to default", profileName != defaultProfile)
        assertEquals("Profile should be default 'GNUnet'", "GNUnet", defaultProfile)
    }
}
