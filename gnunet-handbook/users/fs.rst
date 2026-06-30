File-sharing
------------

This chapter documents the GNUnet file-sharing application. The original
file-sharing implementation for GNUnet was designed to provide anonymous
file-sharing. However, over time, we have also added support for
non-anonymous file-sharing (which can provide better performance).
Anonymous and non-anonymous file-sharing are quite integrated in GNUnet
and, except for routing, share most of the concepts and implementation.
There are three primary file-sharing operations: publishing, searching
and downloading. For each of these operations, the user specifies an
anonymity level. If both the publisher and the searcher/downloader
specify “no anonymity”, non-anonymous file-sharing is used. If either
user specifies some desired degree of anonymity, anonymous file-sharing
will be used.

After a short introduction, we will first look at the various concepts
in GNUnet’s file-sharing implementation. Then, we will discuss specifics
as to how they impact users that publish, search or download files.

Searching
~~~~~~~~~

The command ``gnunet-search`` can be used to search for content on
GNUnet. The format is:

::

   $ gnunet-search [-t TIMEOUT] KEYWORD

The ``-t`` option specifies that the query should timeout after
approximately ``TIMEOUT`` seconds. A value of zero (“0”) is interpreted
as no timeout, which is the default. In this case, gnunet-search will
never terminate (unless you press CTRL-C).

If multiple words are passed as keywords, they will all be considered
optional. Prefix keywords with a “+” to make them mandatory.

Note that searching using:

::

   $ gnunet-search Das Kapital

is not the same as searching for

::

   $ gnunet-search "Das Kapital"

as the first will match files shared under the keywords “Das” or
“Kapital” whereas the second will match files shared under the keyword
“Das Kapital”.

Search results are printed like this:

::

   #15:
   gnunet-download -o "COPYING" gnunet://fs/chk/PGK8M...3EK130.75446

The whole line is the command you would have to enter to download the
file. The first argument passed to ``-o`` is the suggested filename (you
may change it to whatever you like). It is followed by the key for
decrypting the file, the query for searching the file, a checksum (in
hexadecimal) finally the size of the file in bytes.

Downloading
~~~~~~~~~~~

In order to download a file, you need the whole line returned by
gnunet-search. You can then use the tool ``gnunet-download`` to obtain
the file:

::

   $ gnunet-download -o <FILENAME> <GNUNET-URL>

``FILENAME`` specifies the name of the file where GNUnet is supposed to
write the result. Existing files are overwritten. If the existing file
contains blocks that are identical to the desired download, those blocks
will not be downloaded again (automatic resume).

If you want to download the GPL from the previous example, you do the
following:

::

   $ gnunet-download -o "COPYING" gnunet://fs/chk/PGK8M...3EK130.75446

If you ever have to abort a download, you can continue it at any time by
re-issuing gnunet-download with the same filename. In that case, GNUnet
will **not** download blocks again that are already present.

GNUnet’s file-encoding mechanism will ensure file integrity, even if the
existing file was not downloaded from GNUnet in the first place.

You may want to use the ``-V`` switch to turn on verbose reporting. In
this case, gnunet-download will print the current number of bytes
downloaded whenever new data was received.

Publishing
~~~~~~~~~~

The command ``gnunet-publish`` can be used to add content to the
network. The basic format of the command is:

::

   $ gnunet-publish [-n] [-k KEYWORDS]* [-m TYPE:VALUE] FILENAME

For example:

::

   $ gnunet-publish -m "description:GNU License" -k gpl -k test -m "mimetype:text/plain" COPYING

The option ``-k`` is used to specify keywords for the file that should
be inserted. You can supply any number of keywords, and each of the
keywords will be sufficient to locate and retrieve the file. Please note
that you must use the ``-k`` option more than once – one for each
expression you use as a keyword for the filename.

The ``-m`` option is used to specify meta-data, such as descriptions.
You can use ``-m`` multiple times. The ``TYPE`` passed must be from the
list of meta-data types known to libextractor. You can obtain this list
by running ``extract -L``. Use quotes around the entire meta-data
argument if the value contains spaces. The meta-data is displayed to
other users when they select which files to download. The meta-data and
the keywords are optional and may be inferred using GNU libextractor.

``gnunet-publish`` has a few additional options to handle namespaces and
directories. Refer to the man-page for details.

Indexing vs Inserting
~~~~~~~~~~~~~~~~~~~~~

By default, GNUnet indexes a file instead of making a full copy. This is
much more efficient, but requires the file to stay unaltered at the
location where it was when it was indexed. If you intend to move, delete
or alter a file, consider using the option ``-n`` which will force
GNUnet to make a copy of the file in the database.

Since it is much less efficient, this is strongly discouraged for large
files. When GNUnet indexes a file (default), GNUnet does **not** create
an additional encrypted copy of the file but just computes a summary (or
index) of the file. That summary is approximately two percent of the
size of the original file and is stored in GNUnet’s database. Whenever a
request for a part of an indexed file reaches GNUnet, this part is
encrypted on-demand and send out. This way, there is no need for an
additional encrypted copy of the file to stay anywhere on the drive.
This is different from other systems, such as Freenet, where each file
that is put online must be in Freenet’s database in encrypted format,
doubling the space requirements if the user wants to preserve a directly
accessible copy in plaintext.

Thus indexing should be used for all files where the user will keep
using this file (at the location given to gnunet-publish) and does not
want to retrieve it back from GNUnet each time. If you want to remove a
file that you have indexed from the local peer, use the tool
gnunet-unindex to un-index the file.

The option ``-n`` may be used if the user fears that the file might be
found on their drive (assuming the computer comes under the control of
an adversary). When used with the ``-n`` flag, the user has a much
better chance of denying knowledge of the existence of the file, even if
it is still (encrypted) on the drive and the adversary is able to crack
the encryption (e.g. by guessing the keyword).

.. _fs_002dConcepts:

Concepts
~~~~~~~~

For better results with filesharing it is useful to understand the
following concepts. In addition to anonymous routing GNUnet attempts to
give users a better experience in searching for content. GNUnet uses
cryptography to safely break content into smaller pieces that can be
obtained from different sources without allowing participants to corrupt
files. GNUnet makes it difficult for an adversary to send back bogus
search results. GNUnet enables content providers to group related
content and to establish a reputation. Furthermore, GNUnet allows
updates to certain content to be made available. This section is
supposed to introduce users to the concepts that are used to achieve
these goals.

.. _Files:

Files
^^^^^

A file in GNUnet is just a sequence of bytes. Any file-format is allowed
and the maximum file size is theoretically :math:`2^64 - 1` bytes,
except that it would take an impractical amount of time to share such a
file. GNUnet itself never interprets the contents of shared files,
except when using GNU libextractor to obtain keywords.

.. _Keywords:

Keywords
^^^^^^^^

Keywords are the most simple mechanism to find files on GNUnet. Keywords
are **case-sensitive** and the search string must always match
**exactly** the keyword used by the person providing the file. Keywords
are never transmitted in plaintext. The only way for an adversary to
determine the keyword that you used to search is to guess it (which then
allows the adversary to produce the same search request). Since
providing keywords by hand for each shared file is tedious, GNUnet uses
GNU libextractor to help automate this process. Starting a keyword
search on a slow machine can take a little while since the keyword
search involves computing a fresh RSA key to formulate the request.

.. _Directories:

Directories
^^^^^^^^^^^

A directory in GNUnet is a list of file identifiers with meta data. The
file identifiers provide sufficient information about the files to allow
downloading the contents. Once a directory has been created, it cannot
be changed since it is treated just like an ordinary file by the
network. Small files (of a few kilobytes) can be inlined in the
directory, so that a separate download becomes unnecessary.

Directories are shared just like ordinary files. If you download a
directory with ``gnunet-download``, you can use ``gnunet-directory`` to
list its contents. The canonical extension for GNUnet directories when
stored as files in your local file-system is \".gnd\". The contents of a
directory are URIs and meta data. The URIs contain all the information
required by ``gnunet-download`` to retrieve the file. The meta data
typically includes the mime-type, description, a filename and other meta
information, and possibly even the full original file (if it was small).

.. _Egos-and-File_002dSharing:

Egos and File-Sharing
^^^^^^^^^^^^^^^^^^^^^

When sharing files, it is sometimes desirable to build a reputation as a
source for quality information. With egos, publishers can
(cryptographically) sign files, thereby demonstrating that various files
were published by the same entity. An ego thus allows users to link
different publication events, thereby deliberately reducing anonymity to
pseudonymity.

Egos used in GNUnet's file-sharing for such pseudonymous publishing also
correspond to the egos used to identify and sign zones in the GNU Name
System. However, if the same ego is used for file-sharing and for a GNS
zone, this will weaken the privacy assurances provided by the anonymous
file-sharing protocol.

Note that an ego is NOT bound to a GNUnet peer. There can be multiple
egos for a single user, and users could (theoretically) share the
private keys of an ego by copying the respective private keys.

.. _Namespaces:

Namespaces
^^^^^^^^^^

A namespace is a set of files that were signed by the same ego. Today,
namespaces are implemented independently of GNS zones, but in the future
we plan to merge the two such that a GNS zone can basically contain
files using a file-sharing specific record type.

Files (or directories) that have been signed and placed into a namespace
can be updated. Updates are identified as authentic if the same secret
key was used to sign the update.

.. _Advertisements:

Advertisements
^^^^^^^^^^^^^^

Advertisements are used to notify other users about the existence of a
namespace. Advertisements are propagated using the normal keyword
search. When an advertisement is received (in response to a search), the
namespace is added to the list of namespaces available in the
namespace-search dialogs of gnunet-fs-gtk and printed by
``gnunet-identity``. Whenever a namespace is created, an appropriate
advertisement can be generated. The default keyword for the advertising
of namespaces is \"namespace\".

.. _Anonymity-level:

Anonymity level
^^^^^^^^^^^^^^^

The anonymity level determines how hard it should be for an adversary to
determine the identity of the publisher or the searcher/downloader. An
anonymity level of zero means that anonymity is not required. The
default anonymity level of \"1\" means that anonymous routing is
desired, but no particular amount of cover traffic is necessary. A
powerful adversary might thus still be able to deduce the origin of the
traffic using traffic analysis. Specifying higher anonymity levels
increases the amount of cover traffic required.

The specific numeric value (for anonymity levels above 1) is simple:
Given an anonymity level L (above 1), each request FS makes on your
behalf must be hidden in L-1 equivalent requests of cover traffic
(traffic your peer routes for others) in the same time-period. The
time-period is twice the average delay by which GNUnet artificially
delays traffic.

While higher anonymity levels may offer better privacy, they can also
significantly hurt performance.

.. _Content-Priority:

Content Priority
^^^^^^^^^^^^^^^^

Depending on the peer's configuration, GNUnet peers migrate content
between peers. Content in this sense are individual blocks of a file,
not necessarily entire files. When peers run out of space (due to local
publishing operations or due to migration of content from other peers),
blocks sometimes need to be discarded. GNUnet first always discards
expired blocks (typically, blocks are published with an expiration of
about two years in the future; this is another option). If there is
still not enough space, GNUnet discards the blocks with the lowest
priority. The priority of a block is decided by its popularity (in terms
of requests from peers we trust) and, in case of blocks published
locally, the base-priority that was specified by the user when the block
was published initially.

.. _Replication:

Replication
^^^^^^^^^^^

When peers migrate content to other systems, the replication level of a
block is used to decide which blocks need to be migrated most urgently.
GNUnet will always push the block with the highest replication level
into the network, and then decrement the replication level by one. If
all blocks reach replication level zero, the selection is simply random.

.. _Namespace-Management:

Namespace Management
~~~~~~~~~~~~~~~~~~~~

The ``gnunet-identity`` tool can be used to create egos. By default,
``gnunet-identity --display`` simply lists all locally available egos.

.. _Creating-Egos:

Creating Egos
^^^^^^^^^^^^^

With the ``--create=NICK`` option it can also be used to create a new
ego. An ego is the virtual identity of the entity in control of a
namespace or GNS zone. Anyone can create any number of egos. The
provided NICK name automatically corresponds to a GNU Name System domain
name. Thus, henceforth name resolution for any name ending in ".NICK"
will use the NICK's zone. You should avoid using NICKs that collide with
well-known DNS names.

Currently, the IDENTITY subsystem supports two types of identity keys:
ECDSA and EdDSA. By default, ECDSA identities are creates with ECDSA
keys. In order to create an identity with EdDSA keys, you can use the
``--eddsa`` flag.

.. _Deleting-Egos:

Deleting Egos
^^^^^^^^^^^^^

With the ``-D NICK`` option egos can be deleted. Once the ego has been
deleted it is impossible to add content to the corresponding namespace
or zone. However, the existing GNS zone data is currently not dropped.
This may change in the future.

Deleting the pseudonym does not make the namespace or any content in it
unavailable.

.. _File_002dSharing-URIs:

File-Sharing URIs
~~~~~~~~~~~~~~~~~

GNUnet (currently) uses four different types of URIs for file-sharing.
They all begin with \"gnunet://fs/\". This section describes the four
different URI types in detail.

For FS URIs empty KEYWORDs are not allowed. Quotes are allowed to denote
whitespace between words. Keywords must contain a balanced number of
double quotes. Doubles quotes can not be used in the actual keywords.
This means that the string '\"\"foo bar\"\"' will be turned into two
OR-ed keywords 'foo' and 'bar', not into '\"foo bar\"'.

.. _Encoding-of-hash-values-in-URIs:

Encoding of hash values in URIs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Most URIs include some hash values. Hashes are encoded using base32hex
(RFC 2938).

chk-uri
.. _Content-Hash-Key-_0028chk_0029:

Content Hash Key (chk)
^^^^^^^^^^^^^^^^^^^^^^

A chk-URI is used to (uniquely) identify a file or directory and to
allow peers to download the file. Files are stored in GNUnet as a tree
of encrypted blocks. The chk-URI thus contains the information to
download and decrypt those blocks. A chk-URI has the format
\"gnunet://fs/chk/KEYHASH.QUERYHASH.SIZE\". Here, \"SIZE\" is the size
of the file (which allows a peer to determine the shape of the tree),
KEYHASH is the key used to decrypt the file (also the hash of the
plaintext of the top block) and QUERYHASH is the query used to request
the top-level block (also the hash of the encrypted block).

loc-uri
.. _Location-identifiers-_0028loc_0029:

Location identifiers (loc)
^^^^^^^^^^^^^^^^^^^^^^^^^^

For non-anonymous file-sharing, loc-URIs are used to specify which peer
is offering the data (in addition to specifying all of the data from a
chk-URI). Location identifiers include a digital signature of the peer
to affirm that the peer is truly the origin of the data. The format is
\"gnunet://fs/loc/KEYHASH.QUERYHASH.SIZE.PEER.SIG.EXPTIME\". Here,
\"PEER\" is the public key of the peer (in GNUnet format in base32hex),
SIG is the RSA signature (in GNUnet format in base32hex) and EXPTIME
specifies when the signature expires (in milliseconds after 1970).

ksk-uri
.. _Keyword-queries-_0028ksk_0029:

Keyword queries (ksk)
^^^^^^^^^^^^^^^^^^^^^

A keyword-URI is used to specify that the desired operation is the
search using a particular keyword. The format is simply
\"gnunet://fs/ksk/KEYWORD\". Non-ASCII characters can be specified using
the typical URI-encoding (using hex values) from HTTP. \"+\" can be used
to specify multiple keywords (which are then logically \"OR\"-ed in the
search, results matching both keywords are given a higher rank):
\"gnunet://fs/ksk/KEYWORD1+KEYWORD2\". ksk-URIs must not begin or end
with the plus ('+') character. Furthermore they must not contain '++'.

sks-uri
.. _Namespace-content-_0028sks_0029:

Namespace content (sks)
^^^^^^^^^^^^^^^^^^^^^^^

**Please note that the text in this subsection is outdated and needs**
**to be rewritten for version 0.10!** **This especially concerns the
terminology of Pseudonym/Ego/Identity.**

Namespaces are sets of files that have been approved by some (usually
pseudonymous) user --- typically by that user publishing all of the
files together. A file can be in many namespaces. A file is in a
namespace if the owner of the ego (aka the namespace's private key)
signs the CHK of the file cryptographically. An SKS-URI is used to
search a namespace. The result is a block containing meta data, the CHK
and the namespace owner's signature. The format of a sks-URI is
\"gnunet://fs/sks/NAMESPACE/IDENTIFIER\". Here, \"NAMESPACE\" is the
public key for the namespace. \"IDENTIFIER\" is a freely chosen keyword
(or password!). A commonly used identifier is \"root\" which by
convention refers to some kind of index or other entry point into the
namespace.


