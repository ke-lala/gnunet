
.. index:: 
   double: subsystem; NAMESTORE

.. _NAMESTORE-Subsystem-Dev:

NAMESTORE
=========

To interact with NAMESTORE clients first connect to the NAMESTORE
service using the ``GNUNET_NAMESTORE_connect`` passing a configuration
handle. As a result they obtain a NAMESTORE handle, they can use for
operations, or NULL is returned if the connection failed.

To disconnect from NAMESTORE, clients use
``GNUNET_NAMESTORE_disconnect`` and specify the handle to disconnect.

NAMESTORE internally uses the private key to refer to zones. These
private keys can be obtained from the IDENTITY subsystem. Here *egos*
*can be used to refer to zones or the default ego assigned to the GNS
subsystem can be used to obtained the master zone's private key.*

.. _Editing-Zone-Information:

Editing Zone Information
^^^^^^^^^^^^^^^^^^^^^^^^

NAMESTORE provides functions to lookup records stored under a label in a
zone and to store records under a label in a zone.

To store (and delete) records, the client uses the
``GNUNET_NAMESTORE_records_store`` function and has to provide namestore
handle to use, the private key of the zone, the label to store the
records under, the records and number of records plus an callback
function. After the operation is performed NAMESTORE will call the
provided callback function with the result GNUNET_SYSERR on failure
(including timeout/queue drop/failure to validate), GNUNET_NO if content
was already there or not found GNUNET_YES (or other positive value) on
success plus an additional error message.

In addition, ``GNUNET_NAMESTORE_records_store2`` can be used to store multiple
record sets using a single API call. This allows the caller to import
a large number of (label, records) tuples in a single database transaction.
This is useful for large zone imports.

Records are deleted by using the store command with 0 records to store.
It is important to note, that records are not merged when records exist
with the label. So a client has first to retrieve records, merge with
existing records and then store the result.

To perform a lookup operation, the client uses the
``GNUNET_NAMESTORE_records_lookup`` function. Here it has to pass the
namestore handle, the private key of the zone and the label. It also has
to provide a callback function which will be called with the result of
the lookup operation: the zone for the records, the label, and the
records including the number of records included.

A special operation is used to set the preferred nickname for a zone.
This nickname is stored with the zone and is automatically merged with
all labels and records stored in a zone. Here the client uses the
``GNUNET_NAMESTORE_set_nick`` function and passes the private key of the
zone, the nickname as string plus a the callback with the result of the
operation.

.. _Transactional-Namestore-API:

Transactional operations
^^^^^^^^^^^^^^^^^^^^^^^^

All API calls by default are mapped to implicit single transactions in the
database backends.
This happends automatically, as most databases support implicit transactions
including the databases supported by NAMESTORE.

However, implicit transactions have two drawbacks:

  1. When storing or deleting a lot of records individual transactions cause
     a significant overhead in the database.
  2. Storage and deletion of records my multiple clients concurrently can lead
     to inconsistencies.

This is why NAMESTORE supports explicit transactions in order to efficiently
handle large amounds of zone data as well as keep then NAMESTORE consistent
when the client thinks this is necessary.

When the client wants to start a transaction, ``GNUNET_NAMESTORE_transaction_begin``
is called.
After this call, ``GNUNET_NAMESTORE_records_lookup`` or ``GNUNET_NAMESTORE_records_store(2)``
can be called successively.
The operations will only be commited to the database (and monitors such as ZONEMASTER
notified of the changes) when ``GNUNET_NAMESTORE_transaction_commit`` is used
to finalize the transaction.
Alternatively, the transaction can be aborted using ``GNUNET_NAMESTORE_transaction_rollback``.
Should the client disconnect after calling ``GNUNET_NAMESTORE_transaction_begin``
any running transaction will automatically be rolled-back.



.. _Iterating-Zone-Information:

Iterating Zone Information
^^^^^^^^^^^^^^^^^^^^^^^^^^

A client can iterate over all information in a zone or all zones managed
by NAMESTORE. Here a client uses one of the
``GNUNET_NAMESTORE_zone_iteration_start(2)`` functions and passes the
namestore handle, the zone to iterate over and a callback function to
call with the result. To iterate over all the zones, it is possible to
pass NULL for the zone. A ``GNUNET_NAMESTORE_ZoneIterator`` handle is
returned to be used to continue iteration.

NAMESTORE calls the callback for every result and expects the client to
call ``GNUNET_NAMESTORE_zone_iterator_next`` to continue to iterate or
``GNUNET_NAMESTORE_zone_iterator_stop`` to interrupt the iteration. When
NAMESTORE reached the last item it will call the callback with a NULL
value to indicate.

.. _Monitoring-Zone-Information:

Monitoring Zone Information
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Clients can also monitor zones to be notified about changes. Here the
clients uses one of the ``GNUNET_NAMESTORE_zone_monitor_start(2)`` functions and
passes the private key of the zone and and a callback function to call
with updates for a zone. The client can specify to obtain zone
information first by iterating over the zone and specify a
synchronization callback to be called when the client and the namestore
are synced.

On an update, NAMESTORE will call the callback with the private key of
the zone, the label and the records and their number.

To stop monitoring, the client calls
``GNUNET_NAMESTORE_zone_monitor_stop`` and passes the handle obtained
from the function to start the monitoring.
