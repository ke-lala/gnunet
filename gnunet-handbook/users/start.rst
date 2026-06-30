Getting started
---------------

Prior to using any GNUnet-based application, one has to start a node:

::

   $ gnunet-arm -s

To stop GNUnet:

::

   $ gnunet-arm -e

You can usually find the logs under ``~/.cache/gnunet`` and all files
such as databases and private keys in ``~/.local/share/gnunet``.

The list of running services can be displayed using the ``-I`` option.
It should look similar to this example:

::

   $ gnunet-arm -I
   Running services:
   topology (gnunet-daemon-topology)
   nat (gnunet-service-nat)
   vpn (gnunet-service-vpn)
   gns (gnunet-service-gns)
   cadet (gnunet-service-cadet)
   namecache (gnunet-service-namecache)
   hostlist (gnunet-daemon-hostlist)
   revocation (gnunet-service-revocation)
   zonemaster (gnunet-service-zonemaster)
   zonemaster-monitor (gnunet-service-zonemaster-monitor)
   dht (gnunet-service-dht)
   namestore (gnunet-service-namestore)
   set (gnunet-service-set)
   statistics (gnunet-service-statistics)
   nse (gnunet-service-nse)
   fs (gnunet-service-fs)
   peerstore (gnunet-service-peerstore)
   core (gnunet-service-core)
   rest (gnunet-rest-server)
   transport (gnunet-service-transport)
   datastore (gnunet-service-datastore)

For the **multi-user** setup first the system services need to be
started as the system user, i.e. the user gnunet needs to execute
``gnunet-arm -s``. This should be done by the system’s init system. Then
the user who wants to start GNUnet applications has to run
``gnunet-arm -s``, too. It is recommended to automate this, e.g. using
the user’s crontab.

You can check directly connected peers with:

::

   $ gnunet-core --connection-status

This should return (at least) one established connection peer.
Otherwise, again, there is likely a problem with your network
configuration.

You can display your own current peer identity with:

::

  $ gnunet-core --show-identity
