.. _Using-the-GNUnet-Messenger:

Messenger
---------

The GNUnet Messenger subsystem allows decentralized message-based
communication inside of so called rooms. Each room can be hosted by a
variable amount of peers. Every member of a room has the possibility to
host the room on its own peer. A peer allows any amount of members to
join a room. The amount of members in a room is not restricted.

Messages in a room will be distributed between all peers hosting the
room or being internally (in context of the messenger service) connected
to a hosting peer. All received or sent messages will be stored on any
peer locally which is hosting the respective room or is internally
connected to such a hosting peer.

The Messenger service is built on the CADET subsystem to make internal
connections between peers using a reliable and encrypted transmission.
Additionally the service uses a discrete padding to few different sizes.
So kinds of messages and potential content can't be identified by the
size of traffic from any attacker being unable to break the encryption
of the transmission layer.

Another feature is additional end-to-end encryption for selected
messages which uses the public key of another member (the receiver) to
encrypt the message. Therefore it is ensured that only the selected
member can read its content. This will also use additional padding.

.. _Current-state:

Current state
~~~~~~~~~~~~~

Currently there is only a simplistic CLI application available to use
the messenger service. You can use this application with the
``gnunet-messenger`` command.

This application was designed for testing purposes and it does not
provide full functionality in the current state. It is planned to
replace this CLI application in later stages with a fully featured one
using a client-side library designed for messenger applications.

.. _Entering-a-room:

Entering a room
~~~~~~~~~~~~~~~

You can enter any room by its ROOMKEY and any PEERIDENTITY of a hosting
peer. Optionally you can provide any IDENTITY which can represent a
local ego by its name. This will automatically select that ego's private
key to sign your messages with.

::

   $ gnunet-messenger [-e IDENTITY] -d PEERIDENTITY -r ROOMKEY

A PEERIDENTITY gets entered in encoded form. You can get your own peer
ID by using the ``gnunet-core`` command:

::

   $ gnunet-core -i

A ROOMKEY gets entered in readable text form. The service will then hash
the entered ROOMKEY and use the result as shared secret for transmission
through the CADET submodule. You can also optionally leave out the '-r'
parameter and the ROOMKEY to use the zeroed hash instead.

If no IDENTITY is provided you will not send any name to others, you
will be referred as \"anonymous\" instead and use the anonymous ego (a
shared key pair known to all peers). If you provide any IDENTITY a
matching ego will be used to sign your messages. If there is no matching
ego you will use the anonymous ego instead. The provided IDENTITY will
be distributed as your name for the service in any case.

.. _Opening-a-room:

Opening a room
~~~~~~~~~~~~~~

You can open any room in a similar way to entering it. You just have to
leave out the '-d' parameter and the PEERIDENTITY of the hosting peer.

::

   $ gnunet-messenger [-e IDENTITY] -r ROOMKEY

Providing ROOMKEY and IDENTITY is identical to entering a room. Opening
a room will also make your peer to a host of this room. So others can
enter the room through your peer if they have the required ROOMKEY and
your peer ID.

If you want to use the zeroed hash as shared secret key for the room you
can also leave it out as well:

::

   $ gnunet-messenger

.. _Messaging-in-a-room:

Messaging in a room
~~~~~~~~~~~~~~~~~~~

Once joined a room by entering it or opening it you can write text-based
messages which will be distributed between all internally conntected
peers. All sent messages will be displayed in the same way as received
messages.

This relates to the internal handling of sent and received messages
being mostly identical on application layer. Every handled message will
be represented visually depending on its kind, content and sender. A
sender can usually be identified by the encoded member ID or their name.

.. code-block:: text

   [17X37K] * 'anonymous' says: "hey"

.. _Private-messaging:

Private messaging
~~~~~~~~~~~~~~~~~

As referred in the introduction the service allows sending private
messages with additional end-to-end encryption. These messages will be
visually represented by messages of the kind 'PRIVATE' in case they
can't be decrypted with your accessible stored keys. Members who can't
decrypt the message can potentially only identify its sender but they
can't identify its receiver. This prevents other members from collecting
more metadata than necessary about you.

.. code-block:: text

   [17X37K] ~ message: PRIVATE

If they can be decrypted they will appear as their secret message
instead but marked visually.

.. code-block:: text

   [17X37K] ** 'anonymous' says: "hey"

Currently you can only activate sending such encrypted text messages
instead of usual text messages by adding the '-p' parameter:

.. code-block:: shell

   $ gnunet-messenger [-e IDENTITY] -d PEERIDENTITY -r ROOMKEY -p

Notice that you can only send such encrypted messages to members who use
a private key which is not publicly known via the anonymous ego to ensure
transparency. If any user could decrypt these messages they would not be
private. So as receiver of such messages the IDENTITY is required and it
has to match a local ego.
