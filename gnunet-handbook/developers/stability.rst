.. _Subsystem-stability:

*******************
Subsystem stability
*******************

This page documents the current stability of the various GNUnet
subsystems. Stability here describes the expected degree of
compatibility with future versions of GNUnet. 

For each subsystem we distinguish between compatibility on the P2P 
network level (communication protocol between peers), the IPC level 
(communication between the service and the service library) and the 
API level (stability of the API). 

P2P compatibility is relevant in terms of which applications are likely 
going to be able to communicate with future versions of the network. 
IPC communication is relevant for the implementation of language bindings 
that re-implement the IPC messages. Finally, API compatibility is relevant 
to developers that hope to be able to avoid changes to applications built 
on top of the APIs of the framework.

The following table summarizes our current view of the stability of the
respective protocols or APIs:

.. todo:: Make table automatically generated individual pages?

+-----------------+-----------------+-----------------+-----------------+
| Subsystem       | P2P             | IPC             | C API           |
+=================+=================+=================+=================+
| util            | n/a             | n/a             | stable          |
+-----------------+-----------------+-----------------+-----------------+
| arm             | n/a             | stable          | stable          |
+-----------------+-----------------+-----------------+-----------------+
| block           | n/a             | n/a             | stable          |
+-----------------+-----------------+-----------------+-----------------+
| cadet           | testing         | testing         | testing         |
+-----------------+-----------------+-----------------+-----------------+
| consensus       | experimental    | experimental    | experimental    |
+-----------------+-----------------+-----------------+-----------------+
| core            | unstable        | unstable        | unstable        |
+-----------------+-----------------+-----------------+-----------------+
| datacache       | n/a             | n/a             | unstable        |
+-----------------+-----------------+-----------------+-----------------+
| datastore       | n/a             | unstable        | unstable        |
+-----------------+-----------------+-----------------+-----------------+
| dht             | testing         | testing         | testing         |
+-----------------+-----------------+-----------------+-----------------+
| dns             | stable          | stable          | stable          |
+-----------------+-----------------+-----------------+-----------------+
| dv              | testing         | testing         | n/a             |
+-----------------+-----------------+-----------------+-----------------+
| exit            | testing         | n/a             | n/a             |
+-----------------+-----------------+-----------------+-----------------+
| fragmentation   | stable          | n/a             | stable          |
+-----------------+-----------------+-----------------+-----------------+
| fs              | unstable        | unstable        | unstable        |
+-----------------+-----------------+-----------------+-----------------+
| gns             | stable          | stable          | stable          |
+-----------------+-----------------+-----------------+-----------------+
| hello           | n/a             | n/a             | testing         |
+-----------------+-----------------+-----------------+-----------------+
| hostlist        | unstable        | unstable        | n/a             |
+-----------------+-----------------+-----------------+-----------------+
| identity        | testing         | testing         | n/a             |
+-----------------+-----------------+-----------------+-----------------+
| multicast       | experimental    | experimental    | experimental    |
+-----------------+-----------------+-----------------+-----------------+
| namestore       | n/a             | testing         | testing         |
+-----------------+-----------------+-----------------+-----------------+
| nat             | n/a             | n/a             | unstable        |
+-----------------+-----------------+-----------------+-----------------+
| nse             | stable          | stable          | stable          |
+-----------------+-----------------+-----------------+-----------------+
| pt              | n/a             | n/a             | n/a             |
+-----------------+-----------------+-----------------+-----------------+
| regex           | stable          | stable          | stable          |
+-----------------+-----------------+-----------------+-----------------+
| revocation      | stable          | stable          | stable          |
+-----------------+-----------------+-----------------+-----------------+
| social          | experimental    | experimental    | experimental    |
+-----------------+-----------------+-----------------+-----------------+
| statistics      | n/a             | stable          | stable          |
+-----------------+-----------------+-----------------+-----------------+
| testbed         | n/a             | testing         | testing         |
+-----------------+-----------------+-----------------+-----------------+
| testing         | n/a             | n/a             | testing         |
+-----------------+-----------------+-----------------+-----------------+
| topology        | n/a             | n/a             | n/a             |
+-----------------+-----------------+-----------------+-----------------+
| transport       | experimental    | experimental    | experimental    |
+-----------------+-----------------+-----------------+-----------------+
| tun             | n/a             | n/a             | stable          |
+-----------------+-----------------+-----------------+-----------------+
| vpn             | testing         | n/a             | n/a             |
+-----------------+-----------------+-----------------+-----------------+

Here is a rough explanation of the values:

.. todo:: 0.10.x is outdated - rewrite "\ ``stable``\ " to reflect 
          a time-independent meaning.

'\ ``stable``\ '
   No incompatible changes are planned at this time; for IPC/APIs, if
   there are incompatible changes, they will be minor and might only
   require minimal changes to existing code; for P2P, changes will be
   avoided if at all possible for the current major version.

'\ ``testing``\ '
   No incompatible changes are planned at this time, but the code is
   still known to be in flux; so while we have no concrete plans, our
   expectation is that there will still be minor modifications; for P2P,
   changes will likely be extensions that should not break existing code

'\ ``unstable``\ '
   Changes are planned and will happen; however, they will not be
   totally radical and the result should still resemble what is there
   now; nevertheless, anticipated changes will break protocol/API
   compatibility

'\ ``experimental``\ '
   Changes are planned and the result may look nothing like what the
   API/protocol looks like today

'\ ``unknown``\ '
   Someone should think about where this subsystem headed

'\ ``n/a``\ '
   This subsystem does not implement a corresponding API/protocol
