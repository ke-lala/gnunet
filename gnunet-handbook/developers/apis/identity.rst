
.. index::
   double: IDENTITY; subsystem 

.. _IDENTITY-Subsystem-Dev:

IDENTITY
========

.. _Connecting-to-the-identity-service:

Connecting to the service
^^^^^^^^^^^^^^^^^^^^^^^^^

First, typical clients connect to the identity service using
``GNUNET_IDENTITY_connect``. This function takes a callback as a
parameter. If the given callback parameter is non-null, it will be
invoked to notify the application about the current state of the
identities in the system.

-  First, it will be invoked on all known egos at the time of the
   connection. For each ego, a handle to the ego and the user's name for
   the ego will be passed to the callback. Furthermore, a ``void **``
   context argument will be provided which gives the client the
   opportunity to associate some state with the ego.

-  Second, the callback will be invoked with NULL for the ego, the name
   and the context. This signals that the (initial) iteration over all
   egos has completed.

-  Then, the callback will be invoked whenever something changes about
   an ego. If an ego is renamed, the callback is invoked with the ego
   handle of the ego that was renamed, and the new name. If an ego is
   deleted, the callback is invoked with the ego handle and a name of
   NULL. In the deletion case, the application should also release
   resources stored in the context.

-  When the application destroys the connection to the identity service
   using ``GNUNET_IDENTITY_disconnect``, the callback is again invoked
   with the ego and a name of NULL (equivalent to deletion of the egos).
   This should again be used to clean up the per-ego context.

The ego handle passed to the callback remains valid until the callback
is invoked with a name of NULL, so it is safe to store a reference to
the ego's handle.

.. _Operations-on-Egos:

Operations on Egos
^^^^^^^^^^^^^^^^^^

Given an ego handle, the main operations are to get its associated
private key using ``GNUNET_IDENTITY_ego_get_private_key`` or its
associated public key using ``GNUNET_IDENTITY_ego_get_public_key``.

The other operations on egos are pretty straightforward. Using
``GNUNET_IDENTITY_create``, an application can request the creation of
an ego by specifying the desired name. The operation will fail if that
name is already in use. Using ``GNUNET_IDENTITY_rename`` the name of an
existing ego can be changed. Finally, egos can be deleted using
``GNUNET_IDENTITY_delete``. All of these operations will trigger updates
to the callback given to the ``GNUNET_IDENTITY_connect`` function of all
applications that are connected with the identity service at the time.
``GNUNET_IDENTITY_cancel`` can be used to cancel the operations before
the respective continuations would be called. It is not guaranteed that
the operation will not be completed anyway, only the continuation will
no longer be called.

.. _The-anonymous-Ego:

The anonymous Ego
^^^^^^^^^^^^^^^^^

A special way to obtain an ego handle is to call
``GNUNET_IDENTITY_ego_get_anonymous``, which returns an ego for the
\"anonymous\" user --- anyone knows and can get the private key for this
user, so it is suitable for operations that are supposed to be anonymous
but require signatures (for example, to avoid a special path in the
code). The anonymous ego is always valid and accessing it does not
require a connection to the identity service.

.. _Convenience-API-to-lookup-a-single-ego:

Convenience API to lookup a single ego
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As applications commonly simply have to lookup a single ego, there is a
convenience API to do just that. Use ``GNUNET_IDENTITY_ego_lookup`` to
lookup a single ego by name. Note that this is the user's name for the
ego, not the service function. The resulting ego will be returned via a
callback and will only be valid during that callback. The operation can
be canceled via ``GNUNET_IDENTITY_ego_lookup_cancel`` (cancellation is
only legal before the callback is invoked).

.. _The-IDENTITY-Client_002dService-Protocol:

The IDENTITY Client-Service Protocol
------------------------------------

A client connecting to the identity service first sends a message with
type ``GNUNET_MESSAGE_TYPE_IDENTITY_START`` to the service. After that,
the client will receive information about changes to the egos by
receiving messages of type ``GNUNET_MESSAGE_TYPE_IDENTITY_UPDATE``.
Those messages contain the private key of the ego and the user's name of
the ego (or zero bytes for the name to indicate that the ego was
deleted). A special bit ``end_of_list`` is used to indicate the end of
the initial iteration over the identity service's egos.

The client can trigger changes to the egos by sending ``CREATE``,
``RENAME`` or ``DELETE`` messages. The CREATE message contains the
private key and the desired name. The RENAME message contains the old
name and the new name. The DELETE message only needs to include the name
of the ego to delete. The service responds to each of these messages
with a ``RESULT_CODE`` message which indicates success or error of the
operation, and possibly a human-readable error message.

Finally, the client can bind the name of a service function to an ego by
sending a ``SET_DEFAULT`` message with the name of the service function
and the private key of the ego. Such bindings can then be resolved using
a ``GET_DEFAULT`` message, which includes the name of the service
function. The identity service will respond to a GET_DEFAULT request
with a SET_DEFAULT message containing the respective information, or
with a RESULT_CODE to indicate an error.


