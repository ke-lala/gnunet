package org.gnunet.gnunetmessenger.ipc

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class ChatUriDto(
    var uri: String? = null,
    var isValid: Boolean = false,
    var userPointer: String? = null
) : Parcelable
