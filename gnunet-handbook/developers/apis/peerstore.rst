
.. index::
   double: subsystem; PEERSTORE

.. _PEERSTORE-Subsystem-Dev:

PEERSTORE
=========

libgnunetpeerstore
------------------

libgnunetpeerstore is the library containing the PEERSTORE API.
Subsystems wishing to communicate with the PEERSTORE service use this
API to open a connection to PEERSTORE. This is done by calling
``GNUNET_PEERSTORE_connect`` which returns a handle to the newly created
connection. This handle has to be used with any further calls to the
API.

To store a new record, the function ``GNUNET_PEERSTORE_store`` is to be
used which requires the record fields and a continuation function that
will be called by the API after the STORE request is sent to the
PEERSTORE service. Note that calling the continuation function does not
mean that the record is successfully stored, only that the STORE request
has been successfully sent to the PEERSTORE service.
``GNUNET_PEERSTORE_store_cancel`` can be called to cancel the STORE
request only before the continuation function has been called.

To iterate over stored records, the function
``GNUNET_PEERSTORE_iterate`` is to be used. *peerid* and *key* can be
set to NULL. An iterator callback function will be called with each
matching record found and a NULL record at the end to signal the end of
result set. ``GNUNET_PEERSTORE_iterate_cancel`` can be used to cancel
the ITERATE request before the iterator callback is called with a NULL
record.

To be notified with new values stored under a (subsystem, peerid, key)
combination, the function ``GNUNET_PEERSTORE_watch`` is to be used. This
will register the watcher with the PEERSTORE service, any new records
matching the given combination will trigger the callback function passed
to ``GNUNET_PEERSTORE_watch``. This continues until
``GNUNET_PEERSTORE_watch_cancel`` is called or the connection to the
service is destroyed.

After the connection is no longer needed, the function
``GNUNET_PEERSTORE_disconnect`` can be called to disconnect from the
PEERSTORE service. Any pending ITERATE or WATCH requests will be
destroyed. If the ``sync_first`` flag is set to ``GNUNET_YES``, the API
will delay the disconnection until all pending STORE requests are sent
to the PEERSTORE service, otherwise, the pending STORE requests will be
destroyed as well.


