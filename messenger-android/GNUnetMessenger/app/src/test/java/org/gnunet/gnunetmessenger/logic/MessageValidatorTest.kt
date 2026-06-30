package org.gnunet.gnunetmessenger.logic

// Import the tools we need from the JUnit testing library
import org.junit.Assert.assertEquals
import org.junit.Test

class MessageValidatorTest {

    // The @Test annotation tells the system this is a test function
    @Test
    fun `isValid returns true for a normal message`() {
        // 1. Arrange: Set up your test
        val validator = MessageValidator()
        val message = "Hello, world!"

        // 2. Act: Call the function you want to test
        val result = validator.isValid(message)

        // 3. Assert: Check if the result is what you expected
        // We expect 'true' because the message is valid
        assertEquals(true, result)
    }

    // Test case 2: Check an empty message
    @Test
    fun `isValid returns false for an empty message`() {
        // Arrange
        val validator = MessageValidator()
        val message = "" // Empty string

        // Act
        val result = validator.isValid(message)

        // Assert
        // This time, we expect the result to be 'false'
        assertEquals(false, result)
    }
}