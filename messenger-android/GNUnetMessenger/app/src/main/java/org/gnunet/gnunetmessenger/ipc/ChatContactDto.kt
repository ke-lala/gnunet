package org.gnunet.gnunetmessenger.ipc

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class ChatContactDto(
    var chatContext: ChatContextDto? = null,
    var name: String? = null,
    var key: String? = null,
    var isBlocked: Boolean = false,
    var userPointer: String? = null
) : Parcelable
