
.. index::
   double: STATISTICS; subsystem

.. _STATISTICS-Subsystem-Dev:

STATISTICS
==========

**libgnunetstatistics** is the library containing the API for the
STATISTICS subsystem. Any process requiring to use STATISTICS should use
this API by to open a connection to the STATISTICS service. This is done
by calling the function ``GNUNET_STATISTICS_create()``. This function
takes the subsystem's name which is trying to use STATISTICS and a
configuration. All values written to STATISTICS with this connection
will be placed in the section corresponding to the given subsystem's
name. The connection to STATISTICS can be destroyed with the function
``GNUNET_STATISTICS_destroy()``. This function allows for the connection
to be destroyed immediately or upon transferring all pending write
requests to the service.

Note: STATISTICS subsystem can be disabled by setting ``DISABLE = YES``
under the ``[STATISTICS]`` section in the configuration. With such a
configuration all calls to ``GNUNET_STATISTICS_create()`` return
``NULL`` as the STATISTICS subsystem is unavailable and no other
functions from the API can be used.

.. _Statistics-retrieval:

Statistics retrieval
^^^^^^^^^^^^^^^^^^^^

Once a connection to the statistics service is obtained, information
about any other system which uses statistics can be retrieved with the
function GNUNET_STATISTICS_get(). This function takes the connection
handle, the name of the subsystem whose information we are interested in
(a ``NULL`` value will retrieve information of all available subsystems
using STATISTICS), the name of the statistic we are interested in (a
``NULL`` value will retrieve all available statistics), a continuation
callback which is called when all of requested information is retrieved,
an iterator callback which is called for each parameter in the retrieved
information and a closure for the aforementioned callbacks. The library
then invokes the iterator callback for each value matching the request.

Call to ``GNUNET_STATISTICS_get()`` is asynchronous and can be canceled
with the function ``GNUNET_STATISTICS_get_cancel()``. This is helpful
when retrieving statistics takes too long and especially when we want to
shutdown and cleanup everything.

.. _Setting-statistics-and-updating-them:

Setting statistics and updating them
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

So far we have seen how to retrieve statistics, here we will learn how
we can set statistics and update them so that other subsystems can
retrieve them.

A new statistic can be set using the function
``GNUNET_STATISTICS_set()``. This function takes the name of the
statistic and its value and a flag to make the statistic persistent. The
value of the statistic should be of the type ``uint64_t``. The function
does not take the name of the subsystem; it is determined from the
previous ``GNUNET_STATISTICS_create()`` invocation. If the given
statistic is already present, its value is overwritten.

An existing statistics can be updated, i.e its value can be increased or
decreased by an amount with the function ``GNUNET_STATISTICS_update()``.
The parameters to this function are similar to
``GNUNET_STATISTICS_set()``, except that it takes the amount to be
changed as a type ``int64_t`` instead of the value.

The library will combine multiple set or update operations into one
message if the client performs requests at a rate that is faster than
the available IPC with the STATISTICS service. Thus, the client does not
have to worry about sending requests too quickly.

.. _Watches:

Watches
^^^^^^^

As interesting feature of STATISTICS lies in serving notifications
whenever a statistic of our interest is modified. This is achieved by
registering a watch through the function ``GNUNET_STATISTICS_watch()``.
The parameters of this function are similar to those of
``GNUNET_STATISTICS_get()``. Changes to the respective statistic's value
will then cause the given iterator callback to be called. Note: A watch
can only be registered for a specific statistic. Hence the subsystem
name and the parameter name cannot be ``NULL`` in a call to
``GNUNET_STATISTICS_watch()``.

A registered watch will keep notifying any value changes until
``GNUNET_STATISTICS_watch_cancel()`` is called with the same parameters
that are used for registering the watch.

.. _The-STATISTICS-Client_002dService-Protocol:

The STATISTICS Client-Service Protocol
--------------------------------------

.. _Statistics-retrieval2:

Statistics retrieval
^^^^^^^^^^^^^^^^^^^^

To retrieve statistics, the client transmits a message of type
``GNUNET_MESSAGE_TYPE_STATISTICS_GET`` containing the given subsystem
name and statistic parameter to the STATISTICS service. The service
responds with a message of type ``GNUNET_MESSAGE_TYPE_STATISTICS_VALUE``
for each of the statistics parameters that match the client request for
the client. The end of information retrieved is signaled by the service
by sending a message of type ``GNUNET_MESSAGE_TYPE_STATISTICS_END``.

.. _Setting-and-updating-statistics:

Setting and updating statistics
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The subsystem name, parameter name, its value and the persistence flag
are communicated to the service through the message
``GNUNET_MESSAGE_TYPE_STATISTICS_SET``.

When the service receives a message of type
``GNUNET_MESSAGE_TYPE_STATISTICS_SET``, it retrieves the subsystem name
and checks for a statistic parameter with matching the name given in the
message. If a statistic parameter is found, the value is overwritten by
the new value from the message; if not found then a new statistic
parameter is created with the given name and value.

In addition to just setting an absolute value, it is possible to perform
a relative update by sending a message of type
``GNUNET_MESSAGE_TYPE_STATISTICS_SET`` with an update flag
(``GNUNET_STATISTICS_SETFLAG_RELATIVE``) signifying that the value in
the message should be treated as an update value.

.. _Watching-for-updates:

Watching for updates
^^^^^^^^^^^^^^^^^^^^

The function registers the watch at the service by sending a message of
type ``GNUNET_MESSAGE_TYPE_STATISTICS_WATCH``. The service then sends
notifications through messages of type
``GNUNET_MESSAGE_TYPE_STATISTICS_WATCH_VALUE`` whenever the statistic
parameter's value is changed.


