package org.gnunet.gnunetmessenger.ipc

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class ChatGroupDto(
    var chatContext: ChatContextDto? = null,
    var name: String? = null,
    var userPointer: String? = null
) : Parcelable
