package org.gnunet.gnunetmessenger.logic

// This is the simple class we want to test.
// Its job is to check if a message is valid before sending.
class MessageValidator {

    fun isValid(message: String): Boolean {
        // A simple rule: the message cannot be blank.
        return message.isNotBlank()
    }

}