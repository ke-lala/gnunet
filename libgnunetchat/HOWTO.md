
# libgnunetchat

This library defines a client-side interface for modern chat- or messenger-applications utilizing multiple services from GNUnet, a decentralized networking framework. The following information shall give a short overview how to use this library and how it is designed.

## Handle

The handle represents the state of your client application as a whole managing all the following resources. You can create a new handle with the `GNUNET_CHAT_start()` function, providing the callback for any message your client application receives. More about that under [Messages](#Messages).

If you have created a handle for your application, you need to destroy it before exiting your application with the function `GNUNET_CHAT_stop()`. This will ensure that all resources get freed in the proper order and deletions of resources will taken care of.

## Accounts

A handle alone doesn't provide much functionality without an account. Your application will need to use an account to enter chats, send or receive messages. So your application can either list all existing accounts via `GNUNET_CHAT_iterate_accounts()` and choose one from them or create a new one with `GNUNET_CHAT_account_create()`.

If necessary your application can also delete an account with `GNUNET_CHAT_account_delete()`. But this can also be done using the IDENTITY service from GNUnet itself.

This is the case because accounts in this library represent so called EGOs from the GNUnet IDENTITY service. Those allow storing records in the local (or via network shared) namestore and signing your messages to allow others verifying that you are indeed the original sender.

To use an account with this library you need to call `GNUNET_CHAT_connect()` with it once. You can switch the account at any time with the same function or if you don't want to have any account selected anymore, you can call `GNUNET_CHAT_disconnect()` to be sure. The account which is currently in use by your handle can be accessed directly via `GNUNET_CHAT_get_connected()`.

Now we are coming to some of the features from the GNUnet Messenger service. We can rename our account at any time using `GNUNET_CHAT_set_name()` once it is connected to our handle but we can replace our EGO keeping the same name as well, using the function `GNUNET_CHAT_update()`. This will generate a new EGO internally, delete the old one and signal all connected chats about this process in one go. So all old signatures from our old messages keep valid as well as new ones despite we are using new cryptographic keys to sign them from now on.

## Lobbies

Lobbies are designed to invite other people into a chat with you without them being part of your contacts already. So for example when you know other people who don't have any application using libgnunetchat installed or they have it running but you are not part of any common group.

You can open a new lobby with the function `GNUNET_CHAT_lobby_open()`. This will publish a record in a public namestore which can be resolved using GNS. The record won't be published in your personal namestore but using a new identity independent of your account instead. This is for multiple reasons: 1.) For example you could change your EGO key while the lobby is open. 2.) Other people could identify you as owner of the lobby without joining which might be a privacy issue.

Just keep in mind when opening a lobby that each lobby will represent an individual chat. So if you want to establish a group chat with your lobby, you have to share its URI between multiple people. Otherwise you want to generate a new lobby each time you want to open a direct chat with it. More about that under [Contacts](#Contacts).

Once a lobby is opened and published in the network your callback will receive the URI which is required to resolve the actual record using GNS and to join the lobby with `GNUNET_CHAT_lobby_join()`. Other people which do not have access to the information captured in that URI won't be able to join the chat. You can share this URI in text form or even as QR code. That is up to your application implementing the exchange.

In case you want to forcefully close a lobby or cancel its opening process, you can call `GNUNET_CHAT_lobby_close()`. However this should not be necessary since a opening a lobby expects you to provide a delay after which it will be closed automatically.

Closing a lobby will not close its chat once other people have joined neither will it delete its published record. The delay however will take care of the record deletion. So it is recommended to always provide a reachable amount of time.

## Groups

Groups represent all chats with more than two members (one being the active user themselves). You can list all current groups with `GNUNET_CHAT_iterate_groups()`, create a new group with `GNUNET_CHAT_group_create()` and leave any group with `GNUNET_CHAT_group_leave()`.

Most capabilities like sending text messages, files and such will be handled by the context of a group. That simplifies the API a lot because direct chats with your contacts will also use contexts for those capabilities. More information about that will be under [Contexts](#Contexts).

There are two functionalities which differentiate between direct chats and group chats. Groups allow you to list all of their current members with `GNUNET_CHAT_group_iterate_contacts()` and you can invite any contact using `GNUNET_CHAT_group_invite_contact()`. Inviting people via your contacts will automatically send them an encrypted invite message through the common chat with least amount of other members (this is likely to be the direct chat with that given contact). But even if the invitation will be sent via another common group chat, you can be sure that it is encrypted and others won't be able to read it.

If the recipient of an invitation rejects it, you will receive a message to be notified about it. However it's application and user specific whether a contact needs to accept or reject any invitation.

## Contacts

Anyone in your contacts needs to share at least one chat with you. This can either be a group chat or a direct chat. All contacts can be listed via `GNUNET_CHAT_iterate_contacts()`. If you want to get rid of a specific contact you can call `GNUNET_CHAT_contact_delete()` which will at least close the direct chat with them. But you would need to drop all common group chats to remove them fully from your contacts list.

Each contact will provide a context for a direct chat with them. But it's recommend to check its status first. If a contact is only part of your contacts list because of a common group, there is no direct chat established. That means you will first need to invite them to a direct chat. The function `GNUNET_CHAT_context_request()` will take care of that and change the contexts status after success and depending on the acceptance of your invitation. Keep in mind that all requests and invitations for chats with other people can be ignored by them. Any chat is designed opt-in.

If you don't like to receive further messages from any contact, it is possible to block them via `GNUNET_CHAT_contact_set_blocked()`. This will mark the contact in every common chat as blocked and it's fully reversible at any time. Additionally you can tag or untag contacts via `GNUNET_CHAT_contact_tag()` and `GNUNET_CHAT_contact_untag()` respectively. This is a feature to individually group contacts.

## Contexts

As mentioned under [Contacts](#Contacts) each context will have a status if it's ready to be used. If that is possible a context provides many features typical for a modern chat application. For example you can send text messages, files and read receipts for other messages. A context will also list you all received and sent messages with `GNUNET_CHAT_context_iterate_messages()` as well as files via `GNUNET_CHAT_context_iterate_files()` accordingly.

### Messages

Messages can represent more than just sent portions of text. Your application will take care of all messages independent of being sent or received successfully via the provided callback starting your handle. Most of the time you want to tell which kind of message you got with `GNUNET_CHAT_message_get_kind()`. After that it is far easier to combine it with your applications logic. Most other attributes of messages won't leave any questions open.

One thing to mention is that read receipts will use another callback instead of just reporting its status via `GNUNET_CHAT_message_get_read_receipt()`. This is because there is no specific message for read receipts internally in the GNUnet Messenger service. A read receipt is just another interpretation of any other message received from one member in a chat afterwards (at least in the service because of its message history integrity).

This also means that a user can not disable to receiving "read receipts". But they can decide whether they want to send them via `GNUNET_CHAT_context_send_read_receipt()`. Internally this will just send an empty text message. But if you wanted to send text, you would use `GNUNET_CHAT_context_send_text()` instead.

A special feature of the GNUnet Messenger service is that messages can be deleted despite the decentralized networking structure behind the front-end. An application can request the deletion of any message with a custom delay to operate using `GNUNET_CHAT_message_delete()`. This will operate differently depending on the original sender of the message. If the current account is the original sender, a deletion message will be sent into the chat requesting all members to delete its content from storage. Obviously it is trust required that other systems will enforce this demand on permission but the default implementation in GNUnet will just do as said as long the permission to delete the message by the sender can be verified.

If you want to tag any message in a chat, you can do that via `GNUNET_CHAT_context_send_tag()` which will send a specific message to add a tag to another message. That tag can be deleted later on as any other generic message.

Last thing to add about messages is that there will be kinds of messages which do not have any counterpart in the GNUnet Messenger service. They will usually represent internal events or information of the API. This allows the application to deal with warnings, refresh- or login-events in a similar way as with other received information from the chats.

### Files

Files can be sent, received and shared. The difference between sending a file and sharing it is mostly to efficiency. Sending a file will require the use of `GNUNET_CHAT_context_send_file()` which copies a local file from your system, generates and uses a random key for it to encrypt its content and finally upload it to the GNUnet network via the GNUnet FS module. Sharing on the other hand will reuse a previously received file information and its key. So it won't require a new upload procedure. Your application can just call `GNUNET_CHAT_context_share_file()` and its done. But both ways will create valid file messages which will be received and perceived the same way.

After receiving a message containing a file you can start its download via `GNUNET_CHAT_file_start_download()`. This is because the GNUnet Messenger service won't actually send files but send the information to download it from the GNUnet network via the GNUnet FS module. In addition shared files are encrypted and the key will be sent via the same message. So only the members of the chat you received the file message in will have all the information together to download and decrypt its content.

Once a file has completed its download, you can use the function `GNUNET_CHAT_file_open_preview()` to get a local file path pointing to a decrypted copy of the downloaded file. This allows any application to easily work with received files from the chat and until you call this function each file will only be stored encrypted on your device. The decrypted copy will be deleted again when you call `GNUNET_CHAT_file_close_preview()` and it is recommended to do that before exiting your application.

If you want to delete any downloaded file completely from your system, you can use `GNUNET_CHAT_file_unindex()` to do that even tracking its status. This will utilize the GNUNET FS module as well and it should be safe to download a deleted file later on again if desired.
