
.. _System-Architecture:

*******************
System Architecture
*******************

.. todo:: 
   FIXME: For those irritated by the textflow, we are missing images here,
   in the short term we should add them back, in the long term this should
   work without images or have images with alt-text.

.. todo:: Adjust image sizes so that they are less obtrusive.

GNUnet developers like LEGOs. The blocks are indestructible, can be
stacked together to construct complex buildings and it is generally easy
to swap one block for a different one that has the same shape. GNUnet's
architecture is based on LEGOs:

|service_lego_block|

This chapter documents the GNUnet LEGO system, also known as GNUnet's
system architecture.

The most common GNUnet component is a service. Services offer an API (or
several, depending on what you count as \"an API\") which is implemented
as a library. The library communicates with the main process of the
service using a service-specific network protocol. The main process of
the service typically doesn't fully provide everything that is needed
--- it has holes to be filled by APIs to other services.

A special kind of component in GNUnet are user interfaces and daemons.
Like services, they have holes to be filled by APIs of other services.
Unlike services, daemons do not implement their own network protocol and
they have no API:

|daemon_lego_block|

The GNUnet system provides a range of services, daemons and user
interfaces, which are then combined into a layered GNUnet instance (also
known as a peer).

|service_stack|

Note that while it is generally possible to swap one service for another
compatible service, there is often only one implementation. However,
during development we often have a \"new\" version of a service in
parallel with an \"old\" version. While the \"new\" version is not
working, developers working on other parts of the service can continue
their development by simply using the \"old\" service. Alternative
design ideas can also be easily investigated by swapping out individual
components. This is typically achieved by simply changing the name of
the \"BINARY\" in the respective configuration section.

Key properties of GNUnet services are that they must be separate
processes and that they must protect themselves by applying tight error
checking against the network protocol they implement (thereby achieving
a certain degree of robustness).

On the other hand, the APIs are implemented to tolerate failures of the
service, isolating their host process from errors by the service. If the
service process crashes, other services and daemons around it should not
also fail, but instead wait for the service process to be restarted by
ARM.

.. todo::
   
   Missing subsystems:
      * TOPOLOGY, FRAGMENTATION
      * VPN and support services (DNS, PT, EXIT)
      * DATASTORE (only used by FS?)
      * MULTICAST and social services (PSYC, PSYCSTORE, SOCIAL)
      * GNS support services/applications (GNSRECORD, ZONEMASTER)
      * Set-based applications (SCALARPRODUCT, SECRETSHARING, CONSENSUS, VOTING)

.. |service_lego_block| image:: /images/service_lego_block.png
.. |daemon_lego_block| image:: /images/daemon_lego_block.png
.. |service_stack| image:: /images/service_stack.png
