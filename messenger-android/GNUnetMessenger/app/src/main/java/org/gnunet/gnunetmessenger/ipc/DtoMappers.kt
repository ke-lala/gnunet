package org.gnunet.gnunetmessenger.ipc

import org.gnunet.gnunetmessenger.model.*

// DTO -> Local Model Mappers
fun ChatAccountDto.toLocal(): ChatAccount {
    return ChatAccount(
        key = this.key ?: "",
        name = this.name ?: "",
        pointer = 0
    )
}

fun ChatContactDto.toLocal(): ChatContact {
    return ChatContact(
        chatContext = this.chatContext?.toLocal() ?: ChatContext(ChatContextType.CONTACT, null, false, false),
        name = this.name ?: "",
        key = this.key ?: "",
        blocked = this.isBlocked,
        userPointer = this.userPointer?.takeIf { it.isNotEmpty() }
    )
}

fun ChatGroupDto.toLocal(): ChatGroup {
    return ChatGroup(
        chatContext = this.chatContext?.toLocal() ?: ChatContext(ChatContextType.GROUP, null, true, false),
        name = this.name ?: "",
        userPointer = this.userPointer?.takeIf { it.isNotEmpty() }
    )
}

fun ChatContextDto.toLocal(): ChatContext {
    val type = ChatContextType.fromCode(chatContextType)
    return ChatContext(
        chatContextType = type,
        userPointer = userPointer?.takeIf { it.isNotEmpty() },
        isGroup = isGroup,
        isPlatform = isPlatform,
        nativeContextPointer = nativeContextPointer?.takeIf { it.isNotEmpty() }
            ?: userPointer?.takeIf { it.isNotEmpty() }
    )
}

fun ChatMessageDto.toLocal(ctx: ChatContext): ChatMessage {
    val kindEnum = MessageKind.fromCode(kind)
    val typeEnum = if (type < 0) null else ChatMessageType.fromCode(type)
    val senderContact = if (!senderName.isNullOrEmpty() || !senderKey.isNullOrEmpty()) {
        ChatContact(
            chatContext = ChatContext(ChatContextType.CONTACT, null, false, false),
            name = senderName ?: "",
            key = senderKey ?: ""
        )
    } else {
        null
    }
    return ChatMessage(
        chatContext = ctx,
        text = text ?: "",
        timestamp = timestamp,
        sender = senderContact,
        kind = kindEnum,
        type = typeEnum
    )
}

fun ChatUriDto.toLocal(): ChatUri {
    return ChatUri(
        error = if (isValid) "" else (uri ?: "Invalid URI")
    )
}

// Local Model -> DTO Mappers
fun ChatAccount.toDto(): ChatAccountDto =
    ChatAccountDto().apply {
        key = this@toDto.key
        name = this@toDto.name
    }

fun ChatContact.toDto(): ChatContactDto =
    ChatContactDto().apply {
        chatContext = this@toDto.chatContext.toDto()
        name = this@toDto.name
        key = this@toDto.key
        isBlocked = this@toDto.blocked
        userPointer = this@toDto.userPointer
    }

fun ChatGroup.toDto(): ChatGroupDto =
    ChatGroupDto().apply {
        chatContext = this@toDto.chatContext.toDto()
        name = this@toDto.name
        userPointer = this@toDto.userPointer
    }

fun ChatContext.toDto(): ChatContextDto =
    ChatContextDto().apply {
        chatContextType = this@toDto.chatContextType?.code ?: 0
        userPointer = this@toDto.userPointer
        isGroup = this@toDto.isGroup
        isPlatform = this@toDto.isPlatform
        nativeContextPointer = this@toDto.nativeContextPointer
    }

fun ChatMessage.toDto(): ChatMessageDto =
    ChatMessageDto().apply {
        chatContext = this@toDto.chatContext.toDto()
        text = this@toDto.text
        timestamp = this@toDto.timestamp
        senderKey = this@toDto.sender?.key
        senderName = this@toDto.sender?.name
        kind = this@toDto.kind.code
        type = this@toDto.type?.code ?: -1
    }

fun ChatUri.toDto(): ChatUriDto =
    ChatUriDto().apply {
        uri = if (error.isEmpty()) "valid" else error
        isValid = error.isEmpty()
    }
