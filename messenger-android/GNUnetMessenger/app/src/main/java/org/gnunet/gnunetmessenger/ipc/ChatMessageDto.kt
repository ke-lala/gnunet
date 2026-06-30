package org.gnunet.gnunetmessenger.ipc

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class ChatMessageDto(
    var chatContext: ChatContextDto? = null,
    var text: String? = null,
    var timestamp: Long = 0L,
    var senderKey: String? = null,
    var senderName: String? = null,
    var kind: Int = 0,
    var type: Int = -1,
    var userPointer: String? = null
) : Parcelable
