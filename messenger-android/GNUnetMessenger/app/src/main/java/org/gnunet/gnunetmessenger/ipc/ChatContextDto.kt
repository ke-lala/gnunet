package org.gnunet.gnunetmessenger.ipc

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class ChatContextDto(
    var chatContextType: Int = 0,
    var userPointer: String? = null,
    var isGroup: Boolean = false,
    var isPlatform: Boolean = false,
    // Stable pointer to the native GNUNET_CHAT_Context*, kept separate from
    // userPointer so the client can overwrite userPointer with a UUID for its
    // own chat-keying without breaking IPC ops that need the native context
    // handle (sendText, iterateContextMessages, etc.).
    var nativeContextPointer: String? = null
) : Parcelable
