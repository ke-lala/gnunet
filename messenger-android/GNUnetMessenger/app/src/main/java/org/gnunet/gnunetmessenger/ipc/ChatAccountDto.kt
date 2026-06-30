package org.gnunet.gnunetmessenger.ipc

import android.os.Parcelable
import kotlinx.parcelize.Parcelize

@Parcelize
data class ChatAccountDto(
    var key: String? = null,
    var name: String? = null
) : Parcelable
