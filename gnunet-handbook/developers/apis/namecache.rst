
.. index::
   single: GNS; name cache
   double: subsystem; NAMECACHE

.. _GNS-Namecache-Dev:

NAMECACHE
=========

The NAMECACHE API consists of five simple functions. First, there is
``GNUNET_NAMECACHE_connect`` to connect to the NAMECACHE service. This
returns the handle required for all other operations on the NAMECACHE.
Using ``GNUNET_NAMECACHE_block_cache`` clients can insert a block into
the cache. ``GNUNET_NAMECACHE_lookup_block`` can be used to lookup
blocks that were stored in the NAMECACHE. Both operations can be
canceled using ``GNUNET_NAMECACHE_cancel``. Note that canceling a
``GNUNET_NAMECACHE_block_cache`` operation can result in the block being
stored in the NAMECACHE --- or not. Cancellation primarily ensures that
the continuation function with the result of the operation will no
longer be invoked. Finally, ``GNUNET_NAMECACHE_disconnect`` closes the
connection to the NAMECACHE.

The maximum size of a block that can be stored in the NAMECACHE is
``GNUNET_NAMECACHE_MAX_VALUE_SIZE``, which is defined to be 63 kB.

.. _The-NAMECACHE-Client_002dService-Protocol:

The NAMECACHE Client-Service Protocol
-------------------------------------

All messages in the NAMECACHE IPC protocol start with the
``struct GNUNET_NAMECACHE_Header`` which adds a request ID (32-bit
integer) to the standard message header. The request ID is used to match
requests with the respective responses from the NAMECACHE, as they are
allowed to happen out-of-order.

.. _Lookup:

Lookup
^^^^^^

The ``struct LookupBlockMessage`` is used to lookup a block stored in
the cache. It contains the query hash. The NAMECACHE always responds
with a ``struct LookupBlockResponseMessage``. If the NAMECACHE has no
response, it sets the expiration time in the response to zero.
Otherwise, the response is expected to contain the expiration time, the
ECDSA signature, the derived key and the (variable-size) encrypted data
of the block.

.. _Store:

Store
^^^^^

The ``struct BlockCacheMessage`` is used to cache a block in the
NAMECACHE. It has the same structure as the
``struct LookupBlockResponseMessage``. The service responds with a
``struct BlockCacheResponseMessage`` which contains the result of the
operation (success or failure). In the future, we might want to make it
possible to provide an error message as well.

.. _The-NAMECACHE-Plugin-API:

The NAMECACHE Plugin API
------------------------

The NAMECACHE plugin API consists of two functions, ``cache_block`` to
store a block in the database, and ``lookup_block`` to lookup a block in
the database.

.. _Lookup2:

Lookup2
^^^^^^^

The ``lookup_block`` function is expected to return at most one block to
the iterator, and return ``GNUNET_NO`` if there were no non-expired
results. If there are multiple non-expired results in the cache, the
lookup is supposed to return the result with the largest expiration
time.

.. _Store2:

Store2
^^^^^^

The ``cache_block`` function is expected to try to store the block in
the database, and return ``GNUNET_SYSERR`` if this was not possible for
any reason. Furthermore, ``cache_block`` is expected to implicitly
perform cache maintenance and purge blocks from the cache that have
expired. Note that ``cache_block`` might encounter the case where the
database already has another block stored under the same key. In this
case, the plugin must ensure that the block with the larger expiration
time is preserved. Obviously, this can done either by simply adding new
blocks and selecting for the most recent expiration time during lookup,
or by checking which block is more recent during the store operation.

