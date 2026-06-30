.. index:: 
   double: File sharing; subsystem
   see: FS; File sharing

.. _FS-Subsystem-Dev:

FS
==

.. _Namespace-Advertisements:

Namespace Advertisements
^^^^^^^^^^^^^^^^^^^^^^^^

.. todo:: FIXME: all zeroses -> ?

An ``SBlock`` with identifier all zeros is a signed advertisement for a
namespace. This special ``SBlock`` contains metadata describing the
content of the namespace. Instead of the name of the identifier for a
potential update, it contains the identifier for the root of the
namespace. The URI should always be empty. The ``SBlock`` is signed with
the content provider's RSA private key (just like any other SBlock).
Peers can search for ``SBlock``\ s in order to find out more about a
namespace.

.. _KSBlocks:

KSBlocks
^^^^^^^^

GNUnet implements ``KSBlocks`` which are ``KBlocks`` that, instead of
encrypting a CHK and metadata, encrypt an ``SBlock`` instead. In other
words, ``KSBlocks`` enable GNUnet to find ``SBlocks`` using the global
keyword search. Usually the encrypted ``SBlock`` is a namespace
advertisement. The rationale behind ``KSBlock``\ s and ``SBlock``\ s is
to enable peers to discover namespaces via keyword searches, and, to
associate useful information with namespaces. When GNUnet finds
``KSBlocks`` during a normal keyword search, it adds the information to
an internal list of discovered namespaces. Users looking for interesting
namespaces can then inspect this list, reducing the need for out-of-band
discovery of namespaces. Naturally, namespaces (or more specifically,
namespace advertisements) can also be referenced from directories, but
``KSBlock``\ s should make it easier to advertise namespaces for the
owner of the pseudonym since they eliminate the need to first create a
directory.

Collections are also advertised using ``KSBlock``\ s.

.. https://old.gnunet.org/sites/default/files/ecrs.pdf
.. What is this? - WGL

.. _File_002dsharing-persistence-directory-structure:

File-sharing persistence directory structure
--------------------------------------------

This section documents how the file-sharing library implements
persistence of file-sharing operations and specifically the resulting
directory structure. This code is only active if the
``GNUNET_FS_FLAGS_PERSISTENCE`` flag was set when calling
``GNUNET_FS_start``. In this case, the file-sharing library will try
hard to ensure that all major operations (searching, downloading,
publishing, unindexing) are persistent, that is, can live longer than
the process itself. More specifically, an operation is supposed to live
until it is explicitly stopped.

If ``GNUNET_FS_stop`` is called before an operation has been stopped, a
``SUSPEND`` event is generated and then when the process calls
``GNUNET_FS_start`` next time, a ``RESUME`` event is generated.
Additionally, even if an application crashes (segfault, SIGKILL, system
crash) and hence ``GNUNET_FS_stop`` is never called and no ``SUSPEND``
events are generated, operations are still resumed (with ``RESUME``
events). This is implemented by constantly writing the current state of
the file-sharing operations to disk. Specifically, the current state is
always written to disk whenever anything significant changes (the
exception are block-wise progress in publishing and unindexing, since
those operations would be slowed down significantly and can be resumed
cheaply even without detailed accounting). Note that if the process
crashes (or is killed) during a serialization operation, FS does not
guarantee that this specific operation is recoverable (no strict
transactional semantics, again for performance reasons). However, all
other unrelated operations should resume nicely.

Since we need to serialize the state continuously and want to recover as
much as possible even after crashing during a serialization operation,
we do not use one large file for serialization. Instead, several
directories are used for the various operations. When
``GNUNET_FS_start`` executes, the master directories are scanned for
files describing operations to resume. Sometimes, these operations can
refer to related operations in child directories which may also be
resumed at this point. Note that corrupted files are cleaned up
automatically. However, dangling files in child directories (those that
are not referenced by files from the master directories) are not
automatically removed.

Persistence data is kept in a directory that begins with the
\"STATE_DIR\" prefix from the configuration file (by default,
\"$SERVICEHOME/persistence/\") followed by the name of the client as
given to ``GNUNET_FS_start`` (for example, \"gnunet-gtk\") followed by
the actual name of the master or child directory.

The names for the master directories follow the names of the operations:

-  \"search\"

-  \"download\"

-  \"publish\"

-  \"unindex\"

Each of the master directories contains names (chosen at random) for
each active top-level (master) operation. Note that a download that is
associated with a search result is not a top-level operation.

In contrast to the master directories, the child directories are only
consulted when another operation refers to them. For each search, a
subdirectory (named after the master search synchronization file)
contains the search results. Search results can have an associated
download, which is then stored in the general \"download-child\"
directory. Downloads can be recursive, in which case children are stored
in subdirectories mirroring the structure of the recursive download
(either starting in the master \"download\" directory or in the
\"download-child\" directory depending on how the download was
initiated). For publishing operations, the \"publish-file\" directory
contains information about the individual files and directories that are
part of the publication. However, this directory structure is flat and
does not mirror the structure of the publishing operation. Note that
unindex operations cannot have associated child operations.
