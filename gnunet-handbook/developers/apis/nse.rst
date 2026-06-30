.. index::
   single: subsystem; Network size estimation
   see: NSE; Network size estimation

.. _NSE-Subsystem-Dev:

NSE
===

The NSE subsystem has the simplest API of all services, with only two
calls: ``GNUNET_NSE_connect`` and ``GNUNET_NSE_disconnect``.

The connect call gets a callback function as a parameter and this
function is called each time the network agrees on an estimate. This
usually is once per round, with some exceptions: if the closest peer has
a late local clock and starts spreading its ID after everyone else
agreed on a value, the callback might be activated twice in a round, the
second value being always bigger than the first. The default round time
is set to 1 hour.

The disconnect call disconnects from the NSE subsystem and the callback
is no longer called with new estimates.

.. _Results:

Results
^^^^^^^

The callback provides two values: the average and the `standard
deviation <http://en.wikipedia.org/wiki/Standard_deviation>`__ of the
last 64 rounds. The values provided by the callback function are
logarithmic, this means that the real estimate numbers can be obtained
by calculating 2 to the power of the given value (2average). From a
statistics point of view this means that:

-  68% of the time the real size is included in the interval
   [(2average-stddev), 2]

-  95% of the time the real size is included in the interval
   [(2average-2*stddev, 2^average+2*stddev]

-  99.7% of the time the real size is included in the interval
   [(2average-3*stddev, 2average+3*stddev]

The expected standard variation for 64 rounds in a network of stable
size is 0.2. Thus, we can say that normally:

-  68% of the time the real size is in the range [-13%, +15%]

-  95% of the time the real size is in the range [-24%, +32%]

-  99.7% of the time the real size is in the range [-34%, +52%]

As said in the introduction, we can be quite sure that usually the real
size is between one third and three times the estimate. This can of
course vary with network conditions. Thus, applications may want to also
consider the provided standard deviation value, not only the average (in
particular, if the standard variation is very high, the average maybe
meaningless: the network size is changing rapidly).

.. _libgnunetnse-_002d-Examples:

Examples
^^^^^^^^

Let's close with a couple examples.

Average: 10, std dev: 1 Here the estimate would be
   2^10 = 1024 peers. (The range in which we can be 95% sure is: [2^8,
   2^12] = [256, 4096]. We can be very (>99.7%) sure that the network is
   not a hundred peers and absolutely sure that it is not a million
   peers, but somewhere around a thousand.)

Average 22, std dev: 0.2 Here the estimate would be
   2^22 = 4 Million peers. (The range in which we can be 99.7% sure is:
   [2^21.4, 2^22.6] = [2.8M, 6.3M]. We can be sure that the network size
   is around four million, with absolutely way of it being 1 million.)

To put this in perspective, if someone remembers the LHC Higgs boson
results, were announced with \"5 sigma\" and \"6 sigma\" certainties. In
this case a 5 sigma minimum would be 2 million and a 6 sigma minimum,
1.8 million.

.. _The-NSE-Client_002dService-Protocol:

The NSE Client-Service Protocol
-------------------------------

As with the API, the client-service protocol is very simple, only has 2
different messages, defined in ``src/nse/nse.h``:

-  ``GNUNET_MESSAGE_TYPE_NSE_START`` This message has no parameters and
   is sent from the client to the service upon connection.

-  ``GNUNET_MESSAGE_TYPE_NSE_ESTIMATE`` This message is sent from the
   service to the client for every new estimate and upon connection.
   Contains a timestamp for the estimate, the average and the standard
   deviation for the respective round.

When the ``GNUNET_NSE_disconnect`` API call is executed, the client
simply disconnects from the service, with no message involved.

NSE Peer-to-Peer Protocol
.. _The-NSE-Peer_002dto_002dPeer-Protocol:

The NSE Peer-to-Peer Protocol
-----------------------------

GNUNET_MESSAGE_TYPE_NSE_P2P_FLOOD
The NSE subsystem only has one message in the P2P protocol, the
``GNUNET_MESSAGE_TYPE_NSE_P2P_FLOOD`` message.

This message key contents are the timestamp to identify the round
(differences in system clocks may cause some peers to send messages way
too early or way too late, so the timestamp allows other peers to
identify such messages easily), the `proof of
work <http://en.wikipedia.org/wiki/Proof-of-work_system>`__ used to make
it difficult to mount a `Sybil
attack <http://en.wikipedia.org/wiki/Sybil_attack>`__, and the public
key, which is used to verify the signature on the message.

Every peer stores a message for the previous, current and next round.
The messages for the previous and current round are given to peers that
connect to us. The message for the next round is simply stored until our
system clock advances to the next round. The message for the current
round is what we are flooding the network with right now. At the
beginning of each round the peer does the following:

-  calculates its own distance to the target value

-  creates, signs and stores the message for the current round (unless
   it has a better message in the \"next round\" slot which came early
   in the previous round)

-  calculates, based on the stored round message (own or received) when
   to start flooding it to its neighbors

Upon receiving a message the peer checks the validity of the message
(round, proof of work, signature). The next action depends on the
contents of the incoming message:

-  if the message is worse than the current stored message, the peer
   sends the current message back immediately, to stop the other peer
   from spreading suboptimal results

-  if the message is better than the current stored message, the peer
   stores the new message and calculates the new target time to start
   spreading it to its neighbors (excluding the one the message came
   from)

-  if the message is for the previous round, it is compared to the
   message stored in the \"previous round slot\", which may then be
   updated

-  if the message is for the next round, it is compared to the message
   stored in the \"next round slot\", which again may then be updated

Finally, when it comes to send the stored message for the current round
to the neighbors there is a random delay added for each neighbor, to
avoid traffic spikes and minimize cross-messages.


