
.. _CADET-Subsystem-Dev:

.. index::
   double: CADET; subsystem

CADET
=====


The CADET API (defined in ``gnunet_cadet_service.h``) is the messaging
API used by P2P applications built using GNUnet. It provides
applications the ability to send and receive encrypted messages to any
peer participating in GNUnet. The API is heavily based on the CORE API.

CADET delivers messages to other peers in \"channels\". A channel is a
permanent connection defined by a destination peer (identified by its
public key) and a port number. Internally, CADET tunnels all channels
towards a destination peer using one session key and relays the data on
multiple \"connections\", independent from the channels.

Each channel has optional parameters, the most important being the
reliability flag. Should a message get lost on TRANSPORT/CORE level, if
a channel is created with as reliable, CADET will retransmit the lost
message and deliver it in order to the destination application.

GNUNET_CADET_connect

.. .. doxygenfunction:: GNUNET_CADET_connect

To communicate with other peers using CADET, it is necessary to first
connect to the service using ``GNUNET_CADET_connect``. This function
takes several parameters in form of callbacks, to allow the client to
react to various events, like incoming channels or channels that
terminate, as well as specify a list of ports the client wishes to
listen to (at the moment it is not possible to start listening on
further ports once connected, but nothing prevents a client to connect
several times to CADET, even do one connection per listening port). The
function returns a handle which has to be used for any further
interaction with the service.

GNUNET_CADET_channel_create

.. .. doxygenfunction:: GNUNET_CADET_channel_create

To connect to a remote peer, a client has to call the
``GNUNET_CADET_channel_create`` function. The most important parameters
given are the remote peer's identity (it public key) and a port, which
specifies which application on the remote peer to connect to, similar to
TCP/UDP ports. CADET will then find the peer in the GNUnet network and
establish the proper low-level connections and do the necessary key
exchanges to assure and authenticated, secure and verified
communication. Similar to
``GNUNET_CADET_connect``,\ ``GNUNET_CADET_create_channel`` returns a
handle to interact with the created channel.

GNUNET_CADET_notify_transmit_ready

.. .. doxygenfunction:: GNUNET_CADET_notify_transmit_ready

For every message the client wants to send to the remote application,
``GNUNET_CADET_notify_transmit_ready`` must be called, indicating the
channel on which the message should be sent and the size of the message
(but not the message itself!). Once CADET is ready to send the message,
the provided callback will fire, and the message contents are provided
to this callback.

Please note the CADET does not provide an explicit notification of when
a channel is connected. In loosely connected networks, like big wireless
mesh networks, this can take several seconds, even minutes in the worst
case. To be alerted when a channel is online, a client can call
``GNUNET_CADET_notify_transmit_ready`` immediately after
``GNUNET_CADET_create_channel``. When the callback is activated, it
means that the channel is online. The callback can give 0 bytes to CADET
if no message is to be sent, this is OK.

GNUNET_CADET_notify_transmit_cancel

.. .. doxygenfunction:: GNUNET_CADET_notify_transmit_cancel

If a transmission was requested but before the callback fires it is no
longer needed, it can be canceled with
``GNUNET_CADET_notify_transmit_ready_cancel``, which uses the handle
given back by ``GNUNET_CADET_notify_transmit_ready``. As in the case of
CORE, only one message can be requested at a time: a client must not
call ``GNUNET_CADET_notify_transmit_ready`` again until the callback is
called or the request is canceled.

GNUNET_CADET_channel_destroy

.. .. doxygenfunction:: GNUNET_CADET_notify_channel_destroy

When a channel is no longer needed, a client can call
``GNUNET_CADET_channel_destroy`` to get rid of it. Note that CADET will
try to transmit all pending traffic before notifying the remote peer of
the destruction of the channel, including retransmitting lost messages
if the channel was reliable.

Incoming channels, channels being closed by the remote peer, and traffic
on any incoming or outgoing channels are given to the client when CADET
executes the callbacks given to it at the time of
``GNUNET_CADET_connect``.

GNUNET_CADET_disconnect

.. .. doxygenfunction:: GNUNET_CADET_disconnect

Finally, when an application no longer wants to use CADET, it should
call ``GNUNET_CADET_disconnect``, but first all channels and pending
transmissions must be closed (otherwise CADET will complain).
