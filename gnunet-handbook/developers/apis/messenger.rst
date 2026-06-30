.. index::
   double: subsystem; MESSENGER

.. _MESSENGER-Subsystem-Dev:

MESSENGER
=========

The MESSENGER API (defined in ``gnunet_messenger_service.h``) allows P2P
applications built using GNUnet to communicate with specified kinds of
messages in a group. It provides applications the ability to send and
receive encrypted messages to any group of peers participating in GNUnet
in a decentralized way ( without even knowing all peers's identities).

MESSENGER delivers messages to other peers in \"rooms\". A room uses a
variable amount of CADET \"channels\" which will all be used for message
distribution. Each channel can represent an outgoing connection opened
by entering a room with ``GNUNET_MESSENGER_enter_room`` or an incoming
connection if the room was opened before via
``GNUNET_MESSENGER_open_room``.

|messenger_room|

To enter a room you have to specify the \"door\" (peer's identity of a
peer which has opened the room) and the key of the room (which is
identical to a CADET \"port\"). To open a room you have to specify only
the key to use. When opening a room you automatically distribute a
PEER-message sharing your peer's identity in the room.

Entering or opening a room can also be combined in any order. In any
case you will automatically get a unique member ID and send a
JOIN-message notifying others about your entry and your public key
derived from your selected private key.

The private key can be selected in combination with a name using
``GNUNET_MESSENGER_connect`` besides setting a (message-)callback which
gets called every time a message gets sent or received in the room. Once
the handle is initialized you can check your used key pair with
``GNUNET_MESSENGER_get_key`` providing only its public key. The function
returns NULL if the anonymous key pair is used. If the key pair should
be replaced with a different one, you can use
``GNUNET_MESSENGER_set_key`` to ensure proper chaining of used private
keys.

This will automatically cause the handle to send a KEY-message which
introduces the change of key pair to all other members in the rooms you
have entered or opened. Your sessions will therefore stay valid while
your old key pair gets replaced, signing the exchange.

Also once the handle is initialized you can check your current name
with ``GNUNET_MESSENGER_get_name`` and potentially change or set a name
via ``GNUNET_MESSENGER_set_name``. Any change in name will automatically
be distributed in all entered or opened rooms with a NAME-message.

In case you have adjusted your name separately in a specific room of
choice by sending a NAME-message manually, that room will not be
affected by the change of your handle's name.

To send a message a message inside of a room you can use
``GNUNET_MESSENGER_send_message``. If you specify a selected contact as
receiver, the message gets encrypted automatically and will be sent as
PRIVATE- message instead.

To request a potentially missed message or to get a specific message
after its original call of the message-callback, you can use
``GNUNET_MESSENGER_get_message``. Additionally once a message was
distributed to application level and the message-callback got called,
you can get the contact respresenting a message's sender respectively
with ``GNUNET_MESSENGER_get_sender``. This allows getting name and the
public key of any sender currently in use with
``GNUNET_MESSENGER_contact_get_name`` and
``GNUNET_MESSENGER_contact_get_key``. It is also possible to iterate
through all current members of a room with
``GNUNET_MESSENGER_iterate_members`` using a callback.

To leave a room you can use ``GNUNET_MESSENGER_close_room`` which will
also close the rooms connections once all applications on the same peer
have left the room. Leaving a room will also send a LEAVE-message
closing a member session on all connected peers before any connection
will be closed. Leaving a room is however not required for any
application to keep your member session open between multiple sessions
of the actual application.

Finally, when an application no longer wants to use CADET, it should
call ``GNUNET_MESSENGER_disconnect``. You don't have to explicitly close
the used rooms or leave them.

Messages
^^^^^^^^

Here is a little summary to the kinds of messages you can send manually:

.. _NAME_002dmessage:

NAME-message
------------

NAME-messages can be used to change the name (or nick) of your identity
inside a room. The selected name can differ from the identifier used to
select your private key for signing messages.

.. _INVITE_002dmessage:

INVITE-message
--------------

INVITE-messages can be used to invite other members in a room to a
different room, sharing one potential door and the required key to enter
the room. This kind of message is typically sent as encrypted
PRIVATE-message to selected members because it doesn't make much sense
to invite all members from one room to another considering a rooms key
doesn't specify its usage.

.. _TEXT_002dmessage:

TEXT-message
------------

TEXT-messages can be used to send simple text-based messages and should
be considered as being in readable form without complex decoding. The
text has to end with a NULL-terminator character and should be in UTF-8
encoding for most compatibility.

.. _FILE_002dmessage:

FILE-message
------------

FILE-messages can be used to share files inside of a room. They do not
contain the actual file being shared but its original hash, filename
and URI to download the file.

It is recommended to use the FS subsystem and the FILE-messages in
combination.

.. _DELETE_002dmessage:

DELETE-message
--------------

DELETE-messages can be send via the separate function
``GNUNET_MESSENGER_delete_message`` which will handle linked deletions
of messages automatically. Messages can be linked in cases the content
of one message requires another message to co-exist.

DELETE-messages can be used to delete messages selected with its hash.
You can also select any custom delay relative to the time of sending the
DELETE-message. Deletion will only be processed on each peer in a room
if the sender is authorized.

The only information of a deleted message which being kept will be the
chained hashes connecting the message graph for potential traversion.
For example the check for completion of a member session requires this
information.

.. _TICKET_002dmessage:

TICKET-message
--------------

TICKET-messages can be send privately to other members in the room. The
member will be able to consume the received ticket via
``GNUNET_RECLAIM_ticket_consume`` to gain access to selected attributes
and their stored values.

TICKET-messages require the usage of the RECLAIM service of GNUnet to
issue, revoke and consume tickets. Revoking tickets is independant of
deletions inside the MESSENGER API.

.. _TAG_002dmessage:

TAG-message
-----------

TAG-messages can be used to tag or reject other messages in a
communicative way. Depending on the level of publication the message
fulfills different terms of functionality. For example if sent in a
private way using your own public key as audience, it might be used to
tag messages in a sharable way between different devices.

Public tagging or private tagging shared with only one contact might
reflect the communication of an own assignment or decision. For example
it can be used to reject an invitation from another contact.

Rejection can be expressed via a TAG-message that does not contain a
tag or in other words the tag is an empty string. This is a special case
for applications because it might translate into the rejection of
contacts or blocking of them.

Tagging or blocking of contacts depends on the message which is the
target of a TAG-message. If the tagged message is the latest
JOIN-message of another contact, the tag will be interpreted as tagging
of the regarding contact. The same way a rejection of such a
JOIN-message will be interpreted as block of the contact.

Unblocking or untagging can be done via deletion of the selected
TAG-message.

.. _SUBSCRIBE_002dmessage:

SUBSCRIBE-message
-----------------

SUBSCRIBE-messages can be used to subscribe to a selected discourse in
a room. With the ``GNUNET_MESSENGER_FLAG_SUBSCRIPTION_KEEP_ALIVE`` flag
this subscription will be extended automatically by the client API of
the MESSENGER service.

A subscription can be ended manually via SUBSCRIBE-message with the flag
``GNUNET_MESSENGER_FLAG_SUBSCRIPTION_UNSUBSCRIBE`` or it will end
automatically when closing the room or disconnecting the client, letting
the subscription run out.

Such an active subscription is necessary to receive TALK-messages from
a discourse. SUBSCRIBE-messages will not be stored locally.

.. _TALK_002dmessage:

TALK-message
------------

TALK-messages can be send while subscribed to a selected discourse. The
messages can contain any sort of binary data. Applications need to work
out format compatibility on their own. It is possible to link formats
to the discourse id used to identify a given discourse inside a room.

Only members that have an active subscription to the given discourse
will receive the TALK-messages. These messages will not be stored
locally.

.. _Member-sessions:

Member sessions
^^^^^^^^^^^^^^^

A member session is a triple of the room key, the member ID and the
public key of the member's key pair. Member sessions allow that a member
can change their ID or their private key once at a time without losing
the ability to delete old messages or identifying the original sender
of a message. On every change of ID or private key a session will be
marked as closed. So every session chain will only contain one open
session with the current ID and public key.

If a session is marked as closed the MESSENGER service will check from
the first message opening a session to its last one closing the session
for completion. If a the service can confirm that there is no message
still missing which was sent from the closed member session, it will be
marked as completed.

A completed member session is not able to verify any incoming message to
ensure forward secrecy preventing others from using old stolen private
keys.

.. |messenger_room| image:: /images/messenger_room.png
