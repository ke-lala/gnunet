**********
C Tutorial
**********

This tutorials explains how to install GNUnet on a GNU/Linux system and
gives an introduction on how GNUnet can be used to develop a
Peer-to-Peer application. Detailed installation instructions for various
operating systems and a detailed list of all dependencies can be found
on our website at https://docs.gnunet.org/#Installation and in our
Reference Documentation (GNUnet Handbook).

Please read this tutorial carefully since every single step is
important, and do not hesitate to contact the GNUnet team if you have
any questions or problems! Visit this link in your webbrowser to learn
how to contact the GNUnet team: https://gnunet.org/en/contact.html

.. _Introduction-to-GNUnet-Architecture:

Introduction to GNUnet Architecture
===================================

GNUnet is organized in layers and services. Each service is composed of
a main service implementation and a client library for other programs to
use the service's functionality, described by an API. Some services
provide an additional command line tool to enable the user to interact
with the service.

Very often it is other GNUnet services that will use these APIs to build
the higher layers of GNUnet on top of the lower ones. Each layer expands
or extends the functionality of the service below (for instance, to
build a mesh on top of a DHT).

The main service implementation runs as a standalone process in the
Operating System and the client code runs as part of the client program,
so crashes of a client do not affect the service process or other
clients. The service and the clients communicate via a message protocol
to be defined and implemented by the programmer.

.. _First-Steps-with-GNUnet:

First Steps with GNUnet
=======================

.. _Configure-your-peer:

Configure your peer
-------------------

First of all we need to configure your peer. Each peer is started with a
configuration containing settings for GNUnet itself and its services.
This configuration is based on the default configuration shipped with
GNUnet and can be modified. The default configuration is located in the
``$PREFIX/share/gnunet/config.d`` directory. When starting a peer, you
can specify a customized configuration using the the ``-c`` command line
switch when starting the ARM service and all other services. When using
a modified configuration the default values are loaded and only values
specified in the configuration file will replace the default values.

Since we want to start additional peers later, we need some
modifications from the default configuration. We need to create a
separate service home and a file containing our modifications for this
peer:

.. code-block:: text

   $ mkdir ~/gnunet1/
   $ touch peer1.conf

Now add the following lines to ``peer1.conf`` to use this directory. For
simplified usage we want to prevent the peer to connect to the GNUnet
network since this could lead to confusing output. This modifications
will replace the default settings:

.. code-block:: text

   [PATHS]
   # Use this directory to store GNUnet data
   GNUNET_HOME = ~/gnunet1/
   [hostlist]
   # prevent bootstrapping
   SERVERS =

.. _Start-a-peer:

Start a peer
------------

Each GNUnet instance (called peer) has an identity (peer ID) based on a
cryptographic public private key pair. The peer ID is the printable hash
of the public key.

GNUnet services are controlled by a master service, the so called
Automatic Restart Manager (ARM). ARM starts, stops and even restarts
services automatically or on demand when a client connects. You interact
with the ARM service using the ``gnunet-arm`` tool. GNUnet can then be
started with ``gnunet-arm -s`` and stopped with ``gnunet-arm -e``. An
additional service not automatically started can be started using
``gnunet-arm -i <service name>`` and stopped using
``gnunet-arm -k <servicename>``.

Once you have started your peer, you can use many other GNUnet commands
to interact with it. For example, you can run:

.. code-block:: text

   $ gnunet-core -i

to obtain the public key of your peer.

You should see an output containing the peer ID similar to:

.. code-block:: text

   I am peer `0PA02UVRKQTS2C .. JL5Q78F6H0B1ACPV1CJI59MEQUMQCC5G'.

.. _Monitor-a-peer:

Monitor a peer
--------------

In this section, we will monitor the behaviour of our peer's DHT service
with respect to a specific key. First we will start GNUnet and then
start the DHT service and use the DHT monitor tool to monitor the PUT
and GET commands we issue ussing the ``gnunet-dht-put`` and
``gnunet-dht-get`` commands. Using the "monitor" line given below, you
can observe the behavior of your own peer's DHT with respect to the
specified KEY:

.. code-block:: text

   # start gnunet with all default services:
   $ gnunet-arm -c ~/peer1.conf -s
   # start DHT service:
   $ gnunet-arm -c ~/peer1.conf -i dht
   $ gnunet-dht-monitor -c ~/peer1.conf -k KEY

Now open a separate terminal and:

.. code-block:: text

   # put VALUE under KEY in the DHT:
   $ gnunet-dht-put -c ~/peer1.conf -k KEY -d VALUE
   # get key KEY from the DHT:
   $ gnunet-dht-get -c ~/peer1.conf -k KEY
   # print statistics about current GNUnet state:
   $ gnunet-statistics -c ~/peer1.conf
   # print statistics about DHT service:
   $ gnunet-statistics -c ~/peer1.conf -s dht

.. _Starting-Two-Peers-by-Hand:

Starting Two Peers by Hand
--------------------------

This section describes how to start two peers on the same machine by
hand. The process is rather painful, but the description is somewhat
instructive. In practice, you might prefer the automated method (see
`Starting Peers Using the Testbed
Service <#Starting-Peers-Using-the-Testbed-Service>`__).

.. _Setup-a-second-peer:

Setup a second peer
~~~~~~~~~~~~~~~~~~~

We will now start a second peer on your machine. For the second peer,
you will need to manually create a modified configuration file to avoid
conflicts with ports and directories. A peers configuration file is by
default located in ``~/.config/gnunet.conf``. This file is typically
very short or even empty as only the differences to the defaults need to
be specified. The defaults are located in many files in the
``$PREFIX/share/gnunet/config.d`` directory.

To configure the second peer, use the files
``$PREFIX/share/gnunet/config.d`` as a template for your main
configuration file:

.. code-block:: text

   $ cat $PREFIX/share/gnunet/config.d/*.conf > peer2.conf

Now you have to edit ``peer2.conf`` and change:

-  ``GNUNET\_TEST\_HOME`` under ``PATHS``

-  Every (uncommented) value for "\ ``PORT``\ " (add 10000) in any
   section (the option may be commented out if ``PORT`` is prefixed by
   \"\#\", in this case, UNIX domain sockets are used and the PORT
   option does not need to be touched)

-  Every value for "\ ``UNIXPATH``\ " in any section (e.g. by adding a
   \"-p2\" suffix)

to a fresh, unique value. Make sure that the PORT numbers stay below
65536. From now on, whenever you interact with the second peer, you need
to specify ``-c peer2.conf`` as an additional command line argument.

Now, generate the 2nd peer's private key:

.. code-block:: text

   $ gnunet-core -i -c peer2.conf

This may take a while, generate entropy using your keyboard or mouse as
needed. Also, make sure the output is different from the gnunet-core
output for the first peer (otherwise you made an error in the
configuration).

.. _Start-the-second-peer-and-connect-the-peers:

Start the second peer and connect the peers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Then, you can start a second peer using:

.. code-block:: text

   $ gnunet-arm -c peer2.conf -s
   $ gnunet-arm -c peer2.conf -i dht
   $ ~/gnunet/src/dht/gnunet-dht-put -c peer2.conf -k KEY -d VALUE
   $ ~/gnunet/src/dht/gnunet-dht-get -c peer2.conf -k KEY

If you want the two peers to connect, you have multiple options:

-  UDP neighbour discovery (automatic)

-  Setup a bootstrap server

-  Connect manually

To setup peer 1 as bootstrapping server change the configuration of the
first one to be a hostlist server by adding the following lines to
``peer1.conf`` to enable bootstrapping server:

.. code-block:: text

   [hostlist]
   OPTIONS = -p

Then change ``peer2.conf`` and replace the "\ ``SERVERS``\ " line in the
"\ ``[hostlist]``\ " section with "\ ``http://localhost:8080/``\ ".
Restart both peers using:

.. code-block:: text

   # stop first peer
   $ gnunet-arm -c peer1.conf -e
   # start first peer
   $ gnunet-arm -c peer1.conf -s
   # start second peer
   $ gnunet-arm -c peer2.conf -s

Note that if you start your peers without changing these settings, they
will use the "global" hostlist servers of the GNUnet P2P network and
likely connect to those peers. At that point, debugging might become
tricky as you're going to be connected to many more peers and would
likely observe traffic and behaviors that are not explicitly controlled
by you.

.. _How-to-connect-manually:

How to connect manually
~~~~~~~~~~~~~~~~~~~~~~~

If you want to use the ``peerinfo`` tool to connect your peers, you
should:

-  Set ``IMMEDIATE_START = NO`` in section ``hostlist`` (to not connect
   to the global GNUnet)

-  Start both peers running ``gnunet-arm -c peer1.conf -s`` and
   ``gnunet-arm -c peer2.conf -s``

-  Get ``HELLO`` message of the first peer running
   ``gnunet-hello -c peer1.conf --export-hello``

-  Give the output to the second peer by running
   ``echo "gnunet://hello/..." | gnunet-hello -c peer2.conf --import-hello``

Check that they are connected using ``gnunet-core -s -c peer1.conf``, which
should give you the other peer's peer identity:


.. code-block:: text

   $ gnunet-core -s -c peer1.conf
   Peer `9TVUCS8P5A7ILLBGO6 [...shortened...] 1KNBJ4NGCHP3JPVULDG'

.. _Starting-Peers-Using-the-Testbed-Service:

Starting Peers Using the Testbed Service
----------------------------------------

GNUnet's testbed service is used for testing scenarios where a number of
peers are to be started. The testbed can manage peers on a single host
or on multiple hosts in a distributed fashion. On a single affordable
computer, it should be possible to run around tens of peers without
drastically increasing the load on the system.

The testbed service can be access through its API
``include/gnunet\_testbed\_service.h``. The API provides many routines
for managing a group of peers. It also provides a helper function
``GNUNET\_TESTBED\_test\_run()`` to quickly setup a minimalistic testing
environment on a single host.

This function takes a configuration file which will be used as a
template configuration for the peers. The testbed takes care of
modifying relevant options in the peers' configuration such as
``SERVICEHOME``, ``PORT``, ``UNIXPATH`` to unique values so that peers
run without running into conflicts. It also checks and assigns the ports
in configurations only if they are free.

Additionally, the testbed service also reads its options from the same
configuration file. Various available options and details about them can
be found in the testbed default configuration file
``src/testbed/testbed.conf``.

With the testbed API, a sample test case can be structured as follows:

.. code-block:: text

   #include <unistd.h>
   #include <gnunet/platform.h>
   #include <gnunet/gnunet_util_lib.h>
   #include <gnunet/gnunet_testbed_service.h>
   #include <gnunet/gnunet_dht_service.h>

   #define NUM_PEERS 20

   static struct GNUNET_TESTBED_Operation *dht_op;

   static struct GNUNET_DHT_Handle *dht_handle;


   struct MyContext
   {
     int ht_len;
   } ctxt;


   static int result;


   static void
   shutdown_task (void *cls)
   {
     if (NULL != dht_op)
     {
       GNUNET_TESTBED_operation_done (dht_op);
       dht_op = NULL;
       dht_handle = NULL;
     }
     result = GNUNET_OK;
   }


   static void
   service_connect_comp (void *cls,
                         struct GNUNET_TESTBED_Operation *op,
                         void *ca_result,
                         const char *emsg)
   {
     GNUNET_assert (op == dht_op);
     dht_handle = ca_result;
     // Do work here...
     GNUNET_SCHEDULER_shutdown ();
   }


   static void *
   dht_ca (void *cls, const struct GNUNET_CONFIGURATION_Handle *cfg)
   {
     struct MyContext *ctxt = cls;

     dht_handle = GNUNET_DHT_connect (cfg, ctxt->ht_len);
     return dht_handle;
   }


   static void
   dht_da (void *cls, void *op_result)
   {
     struct MyContext *ctxt = cls;

     GNUNET_DHT_disconnect ((struct GNUNET_DHT_Handle *) op_result);
     dht_handle = NULL;
   }


   static void
   test_master (void *cls,
                struct GNUNET_TESTBED_RunHandle *h,
                unsigned int num_peers,
                struct GNUNET_TESTBED_Peer **peers,
                unsigned int links_succeeded,
                unsigned int links_failed)
   {
     ctxt.ht_len = 10;
     dht_op = GNUNET_TESTBED_service_connect
         (NULL, peers[0], "dht",
          &service_connect_comp, NULL,
          &dht_ca, &dht_da, &ctxt);
     GNUNET_SCHEDULER_add_shutdown (&shutdown_task, NULL);
   }


   int
   main (int argc, char **argv)
   {
     int ret;

     result = GNUNET_SYSERR;
     ret = GNUNET_TESTBED_test_run
         ("awesome-test", "template.conf",
          NUM_PEERS, 0LL,
          NULL, NULL, &test_master, NULL);
     if ( (GNUNET_OK != ret) || (GNUNET_OK != result) )
       return 1;
     return 0;
   }

The source code for the above listing can be found at
https://git.gnunet.org/gnunet.git/tree/doc/documentation/testbed_test.c
or in the ``doc/`` folder of your repository check-out. After installing
GNUnet, the above source code can be compiled as:

.. code-block:: text

   $ export CPPFLAGS="-I/path/to/gnunet/headers"
   $ export LDFLAGS="-L/path/to/gnunet/libraries"
   $ gcc $CPPFLAGS $LDFLAGS -o testbed-test testbed_test.c \
    -lgnunettestbed -lgnunetdht -lgnunetutil
   # Generate (empty) configuration
   $ touch template.conf
   # run it (press CTRL-C to stop)
   $ ./testbed-test

The ``CPPFLAGS`` and ``LDFLAGS`` are necessary if GNUnet is installed
into a different directory other than ``/usr/local``.

All of testbed API's peer management functions treat management actions
as operations and return operation handles. It is expected that the
operations begin immediately, but they may get delayed (to balance out
load on the system). The program using the API then has to take care of
marking the operation as "done" so that its associated resources can be
freed immediately and other waiting operations can be executed.
Operations will be canceled if they are marked as "done" before their
completion.

An operation is treated as completed when it succeeds or fails.
Completion of an operation is either conveyed as events through
controller event callback or through respective operation completion
callbacks. In functions which support completion notification through
both controller event callback and operation completion callback, first
the controller event callback will be called. If the operation is not
marked as done in that callback or if the callback is given as NULL when
creating the operation, the operation completion callback will be
called. The API documentation shows which event are to be expected in
the controller event notifications. It also documents any exceptional
behaviour.

Once the peers are started, test cases often need to connect some of the
peers' services. Normally, opening a connect to a peer's service
requires the peer's configuration. While using testbed, the testbed
automatically generates per-peer configuration. Accessing those
configurations directly through file system is discouraged as their
locations are dynamically created and will be different among various
runs of testbed. To make access to these configurations easy, testbed
API provides the function ``GNUNET\_TESTBED\_service\_connect()``. This
function fetches the configuration of a given peer and calls the Connect
Adapter. In the example code, it is the ``dht\_ca``. A connect adapter
is expected to open the connection to the needed service by using the
provided configuration and return the created service connection handle.
Successful connection to the needed service is signaled through
``service\_connect\_comp\_cb``.

A dual to connect adapter is the Disconnect Adapter. This callback is
called after the connect adapter has been called when the operation from
``GNUNET\_TESTBED\_service\_connect()`` is marked as "done". It has to
disconnect from the service with the provided service handle
(``op\_result``).

Exercise: Find out how many peers you can run on your system.

Exercise: Find out how to create a 2D torus topology by changing the
options in the configuration file. See section "The GNUnet Reference
Documentation" in The GNUnet Reference Documentation, then use the DHT
API to store and retrieve values in the network.

.. _Developing-Applications:

Developing Applications
=======================

.. _gnunet_002dext:

gnunet-ext
----------

To develop a new peer-to-peer application or to extend GNUnet we provide
a template build system for writing GNUnet extensions in C. It can be
obtained as follows:

.. code-block:: text

   $ git clone https://git.gnunet.org/gnunet-ext.git
   $ cd gnunet-ext/
   $ ./bootstrap
   $ ./configure --prefix=$PREFIX --with-gnunet=$PREFIX
   $ make
   $ make install
   $ make check

The GNUnet ext template includes examples and a working buildsystem for
a new GNUnet service. A common GNUnet service consists of the following
parts which will be discussed in detail in the remainder of this
document. The functionality of a GNUnet service is implemented in:

-  the GNUnet service (gnunet-ext/src/ext/gnunet-service-ext.c)

-  the client API (gnunet-ext/src/ext/ext_api.c)

-  the client application using the service API
   (gnunet-ext/src/ext/gnunet-ext.c)

The interfaces for these entities are defined in:

-  client API interface (gnunet-ext/src/ext/ext.h)

-  the service interface
   (gnunet-ext/src/include/gnunet_service_SERVICE.h)

-  the P2P protocol (gnunet-ext/src/include/gnunet_protocols_ext.h)

In addition the ext systems provides:

-  a test testing the API (gnunet-ext/src/ext/test_ext_api.c)

-  a configuration template for the service
   (gnunet-ext/src/ext/ext.conf.in)

.. _Adapting-the-Template:

Adapting the Template
---------------------

The first step for writing any extension with a new service is to ensure
that the ``ext.conf.in`` file contains entries for the ``UNIXPATH``,
``PORT`` and ``BINARY`` for the service in a section named after the
service.

If you want to adapt the template rename the ``ext.conf.in`` to match
your services name, you have to modify the ``AC\_OUTPUT`` section in
``configure.ac`` in the ``gnunet-ext`` root.

.. _Writing-a-Client-Application:

Writing a Client Application
----------------------------

When writing any client application (for example, a command-line tool),
the basic structure is to start with the ``GNUNET\_PROGRAM\_run``
function. This function will parse command-line options, setup the
scheduler and then invoke the ``run`` function (with the remaining
non-option arguments) and a handle to the parsed configuration (and the
configuration file name that was used, which is typically not needed):

.. code-block:: text

   #include <gnunet/platform.h>
   #include <gnunet/gnunet_util_lib.h>

   static int ret;

   static void
   run (void *cls,
        char *const *args,
        const char *cfgfile,
        const struct GNUNET_CONFIGURATION_Handle *cfg)
   {
     // main code here
     ret = 0;
   }

   int
   main (int argc, char *const *argv)
   {
     struct GNUNET_GETOPT_CommandLineOption options[] = {
       GNUNET_GETOPT_OPTION_END
     };
     return (GNUNET_OK ==
             GNUNET_PROGRAM_run (argc,
                                 argv,
                                 "binary-name",
                                 gettext_noop ("binary description text"),
                                 options, &run, NULL)) ? ret : 1;
   }

.. _Handling-command_002dline-options:

Handling command-line options
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Options can then be added easily by adding global variables and
expanding the ``options`` array. For example, the following would add a
string-option and a binary flag (defaulting to ``NULL`` and
``GNUNET\_NO`` respectively):

.. code-block:: text

   static char *string_option;
   static int a_flag;

   // ...
     struct GNUNET_GETOPT_CommandLineOption options[] = {
       GNUNET_GETOPT_option_string ('s', "name", "SOMESTRING",
        gettext_noop ("text describing the string_option NAME"),
        &string_option},
       GNUNET_GETOPT_option_flag ('f', "flag",
        gettext_noop ("text describing the flag option"), 
        &a_flag),
       GNUNET_GETOPT_OPTION_END
     };
     string_option = NULL;
     a_flag = GNUNET_SYSERR;
   // ...

Issues such as displaying some helpful text describing options using the
``--help`` argument and error handling are taken care of when using this
approach. Other ``GNUNET\_GETOPT\_``-functions can be used to obtain
integer value options, increment counters, etc. You can even write
custom option parsers for special circumstances not covered by the
available handlers. To check if an argument was specified by the user
you initialize the variable with a specific value (e.g. NULL for a
string and GNUNET\_SYSERR for a integer) and check after parsing
happened if the values were modified.

Inside the ``run`` method, the program would perform the
application-specific logic, which typically involves initializing and
using some client library to interact with the service. The client
library is supposed to implement the IPC whereas the service provides
more persistent P2P functions.

Exercise: Add a few command-line options and print them inside of
``run``. What happens if the user gives invalid arguments?

.. _Writing-a-Client-Library:

Writing a Client Library
~~~~~~~~~~~~~~~~~~~~~~~~

The first and most important step in writing a client library is to
decide on an API for the library. Typical API calls include connecting
to the service, performing application-specific requests and cleaning
up. Many examples for such service APIs can be found in the
``gnunet/src/include/gnunet\_*\_service.h`` files.

Then, a client-service protocol needs to be designed. This typically
involves defining various message formats in a header that will be
included by both the service and the client library (but is otherwise
not shared and hence located within the service's directory and not
installed by ``make install``). Each message must start with a
``struct GNUNET\_MessageHeader`` and must be shorter than 64k. By
convention, all fields in IPC (and P2P) messages must be in big-endian
format (and thus should be read using ``ntohl`` and similar functions
and written using ``htonl`` and similar functions). Unique message types
must be defined for each message struct in the ``gnunet\_protocols.h``
header (or an extension-specific include file).

.. _Connecting-to-the-Service:

Connecting to the Service
^^^^^^^^^^^^^^^^^^^^^^^^^

Before a client library can implement the application-specific protocol
with the service, a connection must be created:

.. code-block:: text

   struct GNUNET_MQ_MessageHandlers handlers[] = {
       // ...
     GNUNET_MQ_handler_end ()
   };
   struct GNUNET_MQ_Handle *mq;

   mq = GNUNET_CLIENT_connect (cfg,
                               "service-name",
                               handlers,
                               &error_cb,
                               NULL);

As a result a ``GNUNET\_MQ\_Handle`` is returned which can to used
henceforth to transmit messages to the service. The complete MQ API can
be found in ``gnunet\_mq\_lib.h``. The ``handlers`` array in the example
above is incomplete. Here is where you will define which messages you
expect to receive from the service, and which functions handle them. The
``error\_cb`` is a function that is to be called whenever there are
errors communicating with the service.

.. _Sending-messages:

Sending messages
^^^^^^^^^^^^^^^^

In GNUnet, messages are always sent beginning with a
``struct GNUNET\_MessageHeader`` in big endian format. This header
defines the size and the type of the message, the payload follows after
this header.

.. code-block:: text

   struct GNUNET_MessageHeader
   {
     uint16_t size GNUNET_PACKED;
     uint16_t type GNUNET_PACKED;
   };

Existing message types are defined in ``gnunet\_protocols.h``. A common
way to create a message is with an envelope:

.. code-block:: text

   struct GNUNET_MQ_Envelope *env;
   struct GNUNET_MessageHeader *msg;

   env = GNUNET_MQ_msg_extra (msg, payload_size, GNUNET_MY_MESSAGE_TYPE);
   GNUNET_memcpy (&msg[1],
                  &payload,
                  payload_size);
   // Send message via message queue 'mq'
   GNUNET_mq_send (mq, env);

Exercise: Define a message struct that includes a 32-bit unsigned
integer in addition to the standard GNUnet MessageHeader. Add a C struct
and define a fresh protocol number for your message. Protocol numbers in
gnunet-ext are defined in
``gnunet-ext/src/include/gnunet_protocols_ext.h``

Exercise: Find out how you can determine the number of messages in a
message queue.

Exercise: Find out how you can determine when a message you have queued
was actually transmitted.

Exercise: Define a helper function to transmit a 32-bit unsigned integer
(as payload) to a service using some given client handle.

.. _Receiving-Replies-from-the-Service:

Receiving Replies from the Service
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Clients can receive messages from the service using the handlers
specified in the ``handlers`` array we specified when connecting to the
service. Entries in the the array are usually created using one of two
macros, depending on whether the message is fixed size or variable size.
Variable size messages are managed using two callbacks, one to check
that the message is well-formed, the other to actually process the
message. Fixed size messages are fully checked by the MQ-logic, and thus
only need to provide the handler to process the message. Note that the
prefixes ``check\_`` and ``handle\_`` are mandatory.

.. code-block:: text

   static void
   handle_fix (void *cls, const struct MyMessage *msg)
   {
     // process 'msg'
   }

   static int
   check_var (void *cls, const struct MyVarMessage *msg)
   {
     // check 'msg' is well-formed
     return GNUNET_OK;
   }

   static void
   handle_var (void *cls, const struct MyVarMessage *msg)
   {
     // process 'msg'
   }

   struct GNUNET_MQ_MessageHandler handlers[] = {
     GNUNET_MQ_hd_fixed_size (fix,
                             GNUNET_MESSAGE_TYPE_MY_FIX,
                             struct MyMessage,
                             NULL),
     GNUNET_MQ_hd_fixed_size (var,
                             GNUNET_MESSAGE_TYPE_MY_VAR,
                             struct MyVarMessage,
                             NULL),

     GNUNET_MQ_handler_end ()
   };

Exercise: Expand your helper function to receive a response message (for
example, containing just the ``struct GNUnet MessageHeader`` without any
payload). Upon receiving the service's response, you should call a
callback provided to your helper function's API.

Exercise: Figure out where you can pass values to the closures
(``cls``).

.. _Writing-a-user-interface:

Writing a user interface
~~~~~~~~~~~~~~~~~~~~~~~~

Given a client library, all it takes to access a service now is to
combine calls to the client library with parsing command-line options.

Exercise: Call your client API from your ``run()`` method in your client
application to send a request to the service. For example, send a 32-bit
integer value based on a number given at the command-line to the
service.

.. _Writing-a-Service:

Writing a Service
-----------------

Before you can test the client you've written so far, you'll need to
also implement the corresponding service.

.. _Code-Placement:

Code Placement
~~~~~~~~~~~~~~

New services are placed in their own subdirectory under ``gnunet/src``.
This subdirectory should contain the API implementation file
``SERVICE\_api.c``, the description of the client-service protocol
``SERVICE.h`` and P2P protocol ``SERVICE\_protocol.h``, the
implementation of the service itself ``gnunet-service-SERVICE.h`` and
several files for tests, including test code and configuration files.

.. _Starting-a-Service:

Starting a Service
~~~~~~~~~~~~~~~~~~

The key API definition for creating a service is the
``GNUNET\_SERVICE\_MAIN`` macro:

.. code-block:: text

   GNUNET_SERVICE_MAIN
   ("service-name",
    GNUNET_SERVICE_OPTION_NONE,
    &run,
    &client_connect_cb,
    &client_disconnect_cb,
    NULL,
    GNUNET_MQ_hd_fixed_size (...),
    GNUNET_MQ_hd_var_size (...),
    GNUNET_MQ_handler_end ());

In addition to the service name and flags, the macro takes three
functions, typically called ``run``, ``client\_connect\_cb`` and
``client\_disconnect\_cb`` as well as an array of message handlers that
will be called for incoming messages from clients.

A minimal version of the three central service functions would look like
this:

.. code-block:: text

   static void
   run (void *cls,
        const struct GNUNET_CONFIGURATION_Handle *c,
        struct GNUNET_SERVICE_Handle *service)
   {
   }

   static void *
   client_connect_cb (void *cls,
                      struct GNUNET_SERVICE_Client *c,
                      struct GNUNET_MQ_Handle *mq)
   {
     return c;
   }

   static void
   client_disconnect_cb (void *cls,
                         struct GNUNET_SERVICE_Client *c,
                         void *internal_cls)
   {
     GNUNET_assert (c == internal_cls);
   }

Exercise: Write a stub service that processes no messages at all in your
code. Create a default configuration for it, integrate it with the build
system and start the service from ``gnunet-service-arm`` using
``gnunet-arm -i NAME``.

Exercise: Figure out how to set the closure (``cls``) for handlers of a
service.

Exercise: Figure out how to send messages from the service back to the
client.

Each handler function in the service **must** eventually (possibly in
some asynchronous continuation) call
``GNUNET\_SERVICE\_client\_continue()``. Only after this call additional
messages from the same client may be processed. This way, the service
can throttle processing messages from the same client.

Exercise: Change the service to "handle" the message from your client
(for now, by printing a message). What happens if you forget to call
``GNUNET\_SERVICE\_client\_continue()``?

.. _Interacting-directly-with-other-Peers-using-the-CORE-Service:

Interacting directly with other Peers using the CORE Service
------------------------------------------------------------

FIXME: This section still needs to be updated to the latest API!

One of the most important services in GNUnet is the ``CORE`` service
managing connections between peers and handling encryption between
peers.

One of the first things any service that extends the P2P protocol
typically does is connect to the ``CORE`` service using:

.. code-block:: text

   #include <gnunet/gnunet_core_service.h>

   struct GNUNET_CORE_Handle *
   GNUNET_CORE_connect (const struct GNUNET_CONFIGURATION_Handle *cfg,
                        void *cls,
                        GNUNET_CORE_StartupCallback init,
                        GNUNET_CORE_ConnectEventHandler connects,
                        GNUNET_CORE_DisconnectEventHandler disconnects,
                        const struct GNUNET_MQ_MessageHandler *handlers);

.. _New-P2P-connections:

New P2P connections
~~~~~~~~~~~~~~~~~~~

Before any traffic with a different peer can be exchanged, the peer must
be known to the service. This is notified by the ``CORE`` ``connects``
callback, which communicates the identity of the new peer to the
service:

.. code-block:: text

   void *
   connects (void *cls,
             const struct GNUNET_PeerIdentity *peer,
             struct GNUNET_MQ_Handle *mq)
   {
     return mq;
   }

Note that whatever you return from ``connects`` is given as the ``cls``
argument to the message handlers for messages from the respective peer.

Exercise: Create a service that connects to the ``CORE``. Then start
(and connect) two peers and print a message once your connect callback
is invoked.

.. _Receiving-P2P-Messages:

Receiving P2P Messages
~~~~~~~~~~~~~~~~~~~~~~

To receive messages from ``CORE``, you pass the desired ``handlers`` to
the ``GNUNET\_CORE\_connect()`` function, just as we showed for
services.

It is your responsibility to process messages fast enough or to
implement flow control. If an application does not process CORE messages
fast enough, CORE will randomly drop messages to not keep a very long
queue in memory.

Exercise: Start one peer with a new service that has a message handler
and start a second peer that only has your "old" service without message
handlers. Which "connect" handlers are invoked when the two peers are
connected? Why?

.. _Sending-P2P-Messages:

Sending P2P Messages
~~~~~~~~~~~~~~~~~~~~

You can transmit messages to other peers using the ``mq`` you were given
during the ``connect`` callback. Note that the ``mq`` automatically is
released upon ``disconnect`` and that you must not use it afterwards.

It is your responsibility to not over-fill the message queue, GNUnet
will send the messages roughly in the order given as soon as possible.

Exercise: Write a service that upon connect sends messages as fast as
possible to the other peer (the other peer should run a service that
"processes" those messages). How fast is the transmission? Count using
the STATISTICS service on both ends. Are messages lost? How can you
transmit messages faster? What happens if you stop the peer that is
receiving your messages?

.. _End-of-P2P-connections:

End of P2P connections
~~~~~~~~~~~~~~~~~~~~~~

If a message handler returns ``GNUNET\_SYSERR``, the remote peer shuts
down or there is an unrecoverable network disconnection, CORE notifies
the service that the peer disconnected. After this notification no more
messages will be received from the peer and the service is no longer
allowed to send messages to the peer. The disconnect callback looks like
the following:

.. code-block:: text

   void
   disconnects (void *cls,
                const struct GNUNET_PeerIdentity * peer)
   {
       /* Remove peer's identity from known peers */
       /* Make sure no messages are sent to peer from now on */
   }

Exercise: Fix your service to handle peer disconnects.

.. _Storing-peer_002dspecific-data-using-the-PEERSTORE-service:

Storing peer-specific data using the PEERSTORE service
------------------------------------------------------

GNUnet's PEERSTORE service offers a persistorage for arbitrary
peer-specific data. Other GNUnet services can use the PEERSTORE to
store, retrieve and monitor data records. Each data record stored with
PEERSTORE contains the following fields:

-  subsystem: Name of the subsystem responsible for the record.

-  peerid: Identity of the peer this record is related to.

-  key: a key string identifying the record.

-  value: binary record value.

-  expiry: record expiry date.

The first step is to start a connection to the PEERSTORE service:

.. code-block:: text

   #include "gnunet_peerstore_service.h"

   peerstore_handle = GNUNET_PEERSTORE_connect (cfg);

The service handle ``peerstore_handle`` will be needed for all
subsequent PEERSTORE operations.

.. _Storing-records:

Storing records
~~~~~~~~~~~~~~~

To store a new record, use the following function:

.. code-block:: text

   struct GNUNET_PEERSTORE_StoreContext *
   GNUNET_PEERSTORE_store (struct GNUNET_PEERSTORE_Handle *h,
                           const char *sub_system,
                           const struct GNUNET_PeerIdentity *peer,
                           const char *key,
                           const void *value,
                           size_t size,
                           struct GNUNET_TIME_Absolute expiry,
                           enum GNUNET_PEERSTORE_StoreOption options,
                           GNUNET_PEERSTORE_Continuation cont,
                           void *cont_cls);

The ``options`` parameter can either be
``GNUNET_PEERSTORE_STOREOPTION_MULTIPLE`` which means that multiple
values can be stored under the same key combination (subsystem, peerid,
key), or ``GNUNET_PEERSTORE_STOREOPTION_REPLACE`` which means that
PEERSTORE will replace any existing values under the given key
combination (subsystem, peerid, key) with the new given value.

The continuation function ``cont`` will be called after the store
request is successfully sent to the PEERSTORE service. This does not
guarantee that the record is successfully stored, only that it was
received by the service.

The ``GNUNET_PEERSTORE_store`` function returns a handle to the store
operation. This handle can be used to cancel the store operation only
before the continuation function is called:

.. code-block:: text

   void
   GNUNET_PEERSTORE_store_cancel (struct GNUNET_PEERSTORE_StoreContext
                                  *sc);

.. _Retrieving-records:

Retrieving records
~~~~~~~~~~~~~~~~~~

To retrieve stored records, use the following function:

.. code-block:: text

   struct GNUNET_PEERSTORE_IterateContext *
   GNUNET_PEERSTORE_iterate (struct GNUNET_PEERSTORE_Handle *h,
                             const char *sub_system,
                             const struct GNUNET_PeerIdentity *peer,
                             const char *key,
                             GNUNET_PEERSTORE_Processor callback,
                             void *callback_cls);

The values of ``peer`` and ``key`` can be ``NULL``. This allows the
iteration over values stored under any of the following key
combinations:

-  (subsystem)

-  (subsystem, peerid)

-  (subsystem, key)

-  (subsystem, peerid, key)

The ``callback`` function will be called once with each retrieved record
and once more with a ``NULL`` record to signal the end of results.

The ``GNUNET_PEERSTORE_iterate`` function returns a handle to the
iterate operation. This handle can be used to cancel the iterate
operation only before the callback function is called with a ``NULL``
record.

.. _Monitoring-records:

Monitoring records
~~~~~~~~~~~~~~~~~~

PEERSTORE offers the functionality of monitoring for new records stored
under a specific key combination (subsystem, peerid, key). To start the
monitoring, use the following function:

.. code-block:: text

   struct GNUNET_PEERSTORE_WatchContext *
   GNUNET_PEERSTORE_watch (struct GNUNET_PEERSTORE_Handle *h,
                           const char *sub_system,
                           const struct GNUNET_PeerIdentity *peer,
                           const char *key,
                           GNUNET_PEERSTORE_Processor callback,
                           void *callback_cls);

Whenever a new record is stored under the given key combination, the
``callback`` function will be called with this new record. This will
continue until the connection to the PEERSTORE service is broken or the
watch operation is canceled:

.. code-block:: text

   void
   GNUNET_PEERSTORE_watch_cancel (struct GNUNET_PEERSTORE_WatchContext
                                  *wc);

.. _Disconnecting-from-PEERSTORE:

Disconnecting from PEERSTORE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the connection to the PEERSTORE service is no longer needed,
disconnect using the following function:

.. code-block:: text

   void
   GNUNET_PEERSTORE_disconnect (struct GNUNET_PEERSTORE_Handle *h,
                                int sync_first);

If the ``sync_first`` flag is set to ``GNUNET_YES``, the API will delay
the disconnection until all store requests are received by the PEERSTORE
service. Otherwise, it will disconnect immediately.

.. _Using-the-DHT:

Using the DHT
-------------

The DHT allows to store data so other peers in the P2P network can
access it and retrieve data stored by any peers in the network. This
section will explain how to use the DHT. Of course, the first thing to
do is to connect to the DHT service:

.. code-block:: text

   dht_handle = GNUNET_DHT_connect (cfg, parallel_requests);

The second parameter indicates how many requests in parallel to expect.
It is not a hard limit, but a good approximation will make the DHT more
efficient.

.. _Storing-data-in-the-DHT:

Storing data in the DHT
~~~~~~~~~~~~~~~~~~~~~~~

Since the DHT is a dynamic environment (peers join and leave frequently)
the data that we put in the DHT does not stay there indefinitely. It is
important to "refresh" the data periodically by simply storing it again,
in order to make sure other peers can access it.

The put API call offers a callback to signal that the PUT request has
been sent. This does not guarantee that the data is accessible to others
peers, or even that is has been stored, only that the service has
requested to a neighboring peer the retransmission of the PUT request
towards its final destination. Currently there is no feedback about
whether or not the data has been successfully stored or where it has
been stored. In order to improve the availablilty of the data and to
compensate for possible errors, peers leaving and other unfavorable
events, just make several PUT requests!

.. code-block:: text

   message_sent_cont (void *cls,
                      const struct GNUNET_SCHEDULER_TaskContext *tc)
   {
     // Request has left local node
   }

   struct GNUNET_DHT_PutHandle *
   GNUNET_DHT_put (struct GNUNET_DHT_Handle *handle,
                   const struct GNUNET_HashCode *key,
                   uint32_t desired_replication_level,
                   enum GNUNET_DHT_RouteOption options,
                   enum GNUNET_BLOCK_Type type,
                   size_t size,
                   const void *data,
                   struct GNUNET_TIME_Absolute exp,
                   struct GNUNET_TIME_Relative timeout,
                   GNUNET_DHT_PutContinuation cont, void *cont_cls)

Exercise: Store a value in the DHT periodically to make sure it is
available over time. You might consider using the function
``GNUNET\_SCHEDULER\_add\_delayed`` and call ``GNUNET\_DHT\_put`` from
inside a helper function.

.. _Obtaining-data-from-the-DHT:

Obtaining data from the DHT
~~~~~~~~~~~~~~~~~~~~~~~~~~~

As we saw in the previous example, the DHT works in an asynchronous
mode. Each request to the DHT is executed "in the background" and the
API calls return immediately. In order to receive results from the DHT,
the API provides a callback. Once started, the request runs in the
service, the service will try to get as many results as possible
(filtering out duplicates) until the timeout expires or we explicitly
stop the request. It is possible to give a "forever" timeout with
``GNUNET\_TIME\_UNIT\_FOREVER\_REL``.

If we give a route option ``GNUNET\_DHT\_RO\_RECORD\_ROUTE`` the
callback will get a list of all the peers the data has travelled, both
on the PUT path and on the GET path.

.. code-block:: text

   static void
   get_result_iterator (void *cls, struct GNUNET_TIME_Absolute expiration,
                        const struct GNUNET_HashCode *key,
                        const struct GNUNET_PeerIdentity *get_path,
                        unsigned int get_path_length,
                        const struct GNUNET_PeerIdentity *put_path,
                        unsigned int put_path_length,
                        enum GNUNET_BLOCK_Type type, size_t size,
                        const void *data)
   {
     // Optionally:
     GNUNET_DHT_get_stop (get_handle);
   }

   get_handle =
         GNUNET_DHT_get_start (dht_handle,
                               block_type,
                               &key,
                               replication,
                               GNUNET_DHT_RO_NONE,
                               NULL,
                               0,
                               &get_result_iterator,
                               cls)

Exercise: Store a value in the DHT and after a while retrieve it. Show
the IDs of all the peers the requests have gone through. In order to
convert a peer ID to a string, use the function ``GNUNET\_i2s``. Pay
attention to the route option parameters in both calls!

.. _Implementing-a-block-plugin:

Implementing a block plugin
~~~~~~~~~~~~~~~~~~~~~~~~~~~

In order to store data in the DHT, it is necessary to provide a block
plugin. The DHT uses the block plugin to ensure that only well-formed
requests and replies are transmitted over the network.

The block plugin should be put in a file ``plugin\_block\_SERVICE.c`` in
the service's respective directory. The mandatory functions that need to
be implemented for a block plugin are described in the following
sections.

.. _Validating-requests-and-replies:

Validating requests and replies
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The evaluate function should validate a reply or a request. It returns a
``GNUNET\_BLOCK\_EvaluationResult``, which is an enumeration. All
possible answers are in ``gnunet\_block\_lib.h``. The function will be
called with a ``reply\_block`` argument of ``NULL`` for requests. Note
that depending on how ``evaluate`` is called, only some of the possible
return values are valid. The specific meaning of the ``xquery`` argument
is application-specific. Applications that do not use an extended query
should check that the ``xquery\_size`` is zero. The block group is
typically used to filter duplicate replies.

.. code-block:: text

   static enum GNUNET_BLOCK_EvaluationResult
   block_plugin_SERVICE_evaluate (void *cls,
                                 enum GNUNET_BLOCK_Type type,
                                 struct GNUNET_BlockGroup *bg,
                                 const GNUNET_HashCode *query,
                                 const void *xquery,
                                 size_t xquery_size,
                                 const void *reply_block,
                                 size_t reply_block_size)
   {
     // Verify type, block and bg
   }

Note that it is mandatory to detect duplicate replies in this function
and return the respective status code. Duplicate detection is typically
done using the Bloom filter block group provided by
``libgnunetblockgroup.so``. Failure to do so may cause replies to circle
in the network.

.. _Deriving-a-key-from-a-reply:

Deriving a key from a reply
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The DHT can operate more efficiently if it is possible to derive a key
from the value of the corresponding block. The ``get\_key`` function is
used to obtain the key of a block --- for example, by means of hashing.
If deriving the key is not possible, the function should simply return
``GNUNET\_SYSERR`` (the DHT will still work just fine with such blocks).

.. code-block:: text

   static int
   block_plugin_SERVICE_get_key (void *cls, enum GNUNET_BLOCK_Type type,
                                const void *block, size_t block_size,
                                struct GNUNET_HashCode *key)
   {
     // Store the key in the key argument, return GNUNET_OK on success.
   }

.. _Initialization-of-the-plugin:

Initialization of the plugin
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The plugin is realized as a shared C library. The library must export an
initialization function which should initialize the plugin. The
initialization function specifies what block types the plugin cares
about and returns a struct with the functions that are to be used for
validation and obtaining keys (the ones just defined above).

.. code-block:: text

   void *
   libgnunet_plugin_block_SERVICE_init (void *cls)
   {
     static enum GNUNET_BLOCK_Type types[] =
     {
       GNUNET_BLOCK_TYPE_SERVICE_BLOCKYPE,
       GNUNET_BLOCK_TYPE_ANY
     };
     struct GNUNET_BLOCK_PluginFunctions *api;

     api = GNUNET_new (struct GNUNET_BLOCK_PluginFunctions);
     api->evaluate = &block_plugin_SERICE_evaluate;
     api->get_key = &block_plugin_SERVICE_get_key;
     api->types = types;
     return api;
   }

.. _Shutdown-of-the-plugin:

Shutdown of the plugin
^^^^^^^^^^^^^^^^^^^^^^

Following GNUnet's general plugin API concept, the plugin must export a
second function for cleaning up. It usually does very little.

.. code-block:: text

   void *
   libgnunet_plugin_block_SERVICE_done (void *cls)
   {
     struct GNUNET_TRANSPORT_PluginFunctions *api = cls;

     GNUNET_free (api);
     return NULL;
   }

.. _Integration-of-the-plugin-with-the-build-system:

Integration of the plugin with the build system
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In order to compile the plugin, the ``Makefile.am`` file for the service
SERVICE should contain a rule similar to this:

.. code-block:: text

     plugindir = $(libdir)/gnunet

     plugin_LTLIBRARIES = \
             libgnunet_plugin_block_ext.la
     libgnunet_plugin_block_ext_la_SOURCES = \
             plugin_block_ext.c
     libgnunet_plugin_block_ext_la_LIBADD = \
             $(prefix)/lib/libgnunethello.la \
             $(prefix)/lib/libgnunetblock.la \
             $(prefix)/lib/libgnunetutil.la
     libgnunet_plugin_block_ext_la_LDFLAGS = \
             $(GN_PLUGIN_LDFLAGS)
     libgnunet_plugin_block_ext_la_DEPENDENCIES = \
             $(prefix)/lib/libgnunetblock.la

Exercise: Write a block plugin that accepts all queries and all replies
but prints information about queries and replies when the respective
validation hooks are called.

.. _Monitoring-the-DHT:

Monitoring the DHT
~~~~~~~~~~~~~~~~~~

It is possible to monitor the functioning of the local DHT service. When
monitoring the DHT, the service will alert the monitoring program of any
events, both started locally or received for routing from another peer.
The are three different types of events possible: a GET request, a PUT
request or a response (a reply to a GET).

Since the different events have different associated data, the API gets
3 different callbacks (one for each message type) and optional type and
key parameters, to allow for filtering of messages. When an event
happens, the appropriate callback is called with all the information
about the event.

.. code-block:: text

   static void
   get_callback (void *cls,
                 enum GNUNET_DHT_RouteOption options,
                 enum GNUNET_BLOCK_Type type,
                 uint32_t hop_count,
                 uint32_t desired_replication_level,
                 unsigned int path_length,
                 const struct GNUNET_PeerIdentity *path,
                 const struct GNUNET_HashCode * key)
   {
   }


   static void
   get_resp_callback (void *cls,
                      enum GNUNET_BLOCK_Type type,
                      const struct GNUNET_PeerIdentity *get_path,
                      unsigned int get_path_length,
                      const struct GNUNET_PeerIdentity *put_path,
                      unsigned int put_path_length,
                      struct GNUNET_TIME_Absolute exp,
                      const struct GNUNET_HashCode * key,
                      const void *data,
                      size_t size)
   {
   }


   static void
   put_callback (void *cls,
                 enum GNUNET_DHT_RouteOption options,
                 enum GNUNET_BLOCK_Type type,
                 uint32_t hop_count,
                 uint32_t desired_replication_level,
                 unsigned int path_length,
                 const struct GNUNET_PeerIdentity *path,
                 struct GNUNET_TIME_Absolute exp,
                 const struct GNUNET_HashCode * key,
                 const void *data,
                 size_t size)
   {
   }


   monitor_handle = GNUNET_DHT_monitor_start (dht_handle,
                                             block_type,
                                             key,
                                             &get_callback,
                                             &get_resp_callback,
                                             &put_callback,
                                             cls);

.. _Debugging-with-gnunet_002darm:

Debugging with gnunet-arm
-------------------------

Even if services are managed by ``gnunet-arm``, you can start them with
``gdb`` or ``valgrind``. For example, you could add the following lines
to your configuration file to start the DHT service in a ``gdb`` session
in a fresh ``xterm``:

.. code-block:: text

   [dht]
   PREFIX=xterm -e gdb --args

Alternatively, you can stop a service that was started via ARM and run
it manually:

.. code-block:: text

   $ gnunet-arm -k dht
   $ gdb --args gnunet-service-dht -L DEBUG
   $ valgrind gnunet-service-dht -L DEBUG

Assuming other services are well-written, they will automatically
re-integrate the restarted service with the peer.

GNUnet provides a powerful logging mechanism providing log levels
``ERROR``, ``WARNING``, ``INFO`` and ``DEBUG``. The current log level is
configured using the ``$GNUNET_FORCE_LOG`` environmental variable. The
``DEBUG`` level is only available if ``--enable-logging=verbose`` was
used when running ``configure``. More details about logging can be found
under https://docs.gnunet.org/#Logging.

You should also probably enable the creation of core files, by setting
``ulimit``, and echo'ing ``1`` into
``/proc/sys/kernel/core\_uses\_pid``. Then you can investigate the core
dumps with ``gdb``, which is often the fastest method to find simple
errors.

Exercise: Add a memory leak to your service and obtain a trace pointing
to the leak using ``valgrind`` while running the service from
``gnunet-service-arm``.

.. _GNU-Free-Documentation-License:

GNU Free Documentation License
==============================

license, GNU Free Documentation License
Version 1.3, 3 November 2008

.. code-block:: text

   Copyright © 2000, 2001, 2002, 2007, 2008 Free Software Foundation, Inc.
   http://fsf.org/

   Everyone is permitted to copy and distribute verbatim copies
   of this license document, but changing it is not allowed.

1.  PREAMBLE

    The purpose of this License is to make a manual, textbook, or other
    functional and useful document free in the sense of freedom: to
    assure everyone the effective freedom to copy and redistribute it,
    with or without modifying it, either commercially or
    noncommercially. Secondarily, this License preserves for the author
    and publisher a way to get credit for their work, while not being
    considered responsible for modifications made by others.

    This License is a kind of "copyleft", which means that derivative
    works of the document must themselves be free in the same sense. It
    complements the GNU General Public License, which is a copyleft
    license designed for free software.

    We have designed this License in order to use it for manuals for
    free software, because free software needs free documentation: a
    free program should come with manuals providing the same freedoms
    that the software does. But this License is not limited to software
    manuals; it can be used for any textual work, regardless of subject
    matter or whether it is published as a printed book. We recommend
    this License principally for works whose purpose is instruction or
    reference.

2.  APPLICABILITY AND DEFINITIONS

    This License applies to any manual or other work, in any medium,
    that contains a notice placed by the copyright holder saying it can
    be distributed under the terms of this License. Such a notice grants
    a world-wide, royalty-free license, unlimited in duration, to use
    that work under the conditions stated herein. The "Document", below,
    refers to any such manual or work. Any member of the public is a
    licensee, and is addressed as "you". You accept the license if you
    copy, modify or distribute the work in a way requiring permission
    under copyright law.

    A "Modified Version" of the Document means any work containing the
    Document or a portion of it, either copied verbatim, or with
    modifications and/or translated into another language.

    A "Secondary Section" is a named appendix or a front-matter section
    of the Document that deals exclusively with the relationship of the
    publishers or authors of the Document to the Document's overall
    subject (or to related matters) and contains nothing that could fall
    directly within that overall subject. (Thus, if the Document is in
    part a textbook of mathematics, a Secondary Section may not explain
    any mathematics.) The relationship could be a matter of historical
    connection with the subject or with related matters, or of legal,
    commercial, philosophical, ethical or political position regarding
    them.

    The "Invariant Sections" are certain Secondary Sections whose titles
    are designated, as being those of Invariant Sections, in the notice
    that says that the Document is released under this License. If a
    section does not fit the above definition of Secondary then it is
    not allowed to be designated as Invariant. The Document may contain
    zero Invariant Sections. If the Document does not identify any
    Invariant Sections then there are none.

    The "Cover Texts" are certain short passages of text that are
    listed, as Front-Cover Texts or Back-Cover Texts, in the notice that
    says that the Document is released under this License. A Front-Cover
    Text may be at most 5 words, and a Back-Cover Text may be at most 25
    words.

    A "Transparent" copy of the Document means a machine-readable copy,
    represented in a format whose specification is available to the
    general public, that is suitable for revising the document
    straightforwardly with generic text editors or (for images composed
    of pixels) generic paint programs or (for drawings) some widely
    available drawing editor, and that is suitable for input to text
    formatters or for automatic translation to a variety of formats
    suitable for input to text formatters. A copy made in an otherwise
    Transparent file format whose markup, or absence of markup, has been
    arranged to thwart or discourage subsequent modification by readers
    is not Transparent. An image format is not Transparent if used for
    any substantial amount of text. A copy that is not "Transparent" is
    called "Opaque".

    Examples of suitable formats for Transparent copies include plain
    ASCII without markup, Texinfo input format, LaTeX input format, SGML
    or XML using a publicly available DTD, and standard-conforming
    simple HTML, PostScript or PDF designed for human modification.
    Examples of transparent image formats include PNG, XCF and JPG.
    Opaque formats include proprietary formats that can be read and
    edited only by proprietary word processors, SGML or XML for which
    the DTD and/or processing tools are not generally available, and the
    machine-generated HTML, PostScript or PDF produced by some word
    processors for output purposes only.

    The "Title Page" means, for a printed book, the title page itself,
    plus such following pages as are needed to hold, legibly, the
    material this License requires to appear in the title page. For
    works in formats which do not have any title page as such, "Title
    Page" means the text near the most prominent appearance of the
    work's title, preceding the beginning of the body of the text.

    The "publisher" means any person or entity that distributes copies
    of the Document to the public.

    A section "Entitled XYZ" means a named subunit of the Document whose
    title either is precisely XYZ or contains XYZ in parentheses
    following text that translates XYZ in another language. (Here XYZ
    stands for a specific section name mentioned below, such as
    "Acknowledgements", "Dedications", "Endorsements", or "History".) To
    "Preserve the Title" of such a section when you modify the Document
    means that it remains a section "Entitled XYZ" according to this
    definition.

    The Document may include Warranty Disclaimers next to the notice
    which states that this License applies to the Document. These
    Warranty Disclaimers are considered to be included by reference in
    this License, but only as regards disclaiming warranties: any other
    implication that these Warranty Disclaimers may have is void and has
    no effect on the meaning of this License.

3.  VERBATIM COPYING

    You may copy and distribute the Document in any medium, either
    commercially or noncommercially, provided that this License, the
    copyright notices, and the license notice saying this License
    applies to the Document are reproduced in all copies, and that you
    add no other conditions whatsoever to those of this License. You may
    not use technical measures to obstruct or control the reading or
    further copying of the copies you make or distribute. However, you
    may accept compensation in exchange for copies. If you distribute a
    large enough number of copies you must also follow the conditions in
    section 3.

    You may also lend copies, under the same conditions stated above,
    and you may publicly display copies.

4.  COPYING IN QUANTITY

    If you publish printed copies (or copies in media that commonly have
    printed covers) of the Document, numbering more than 100, and the
    Document's license notice requires Cover Texts, you must enclose the
    copies in covers that carry, clearly and legibly, all these Cover
    Texts: Front-Cover Texts on the front cover, and Back-Cover Texts on
    the back cover. Both covers must also clearly and legibly identify
    you as the publisher of these copies. The front cover must present
    the full title with all words of the title equally prominent and
    visible. You may add other material on the covers in addition.
    Copying with changes limited to the covers, as long as they preserve
    the title of the Document and satisfy these conditions, can be
    treated as verbatim copying in other respects.

    If the required texts for either cover are too voluminous to fit
    legibly, you should put the first ones listed (as many as fit
    reasonably) on the actual cover, and continue the rest onto adjacent
    pages.

    If you publish or distribute Opaque copies of the Document numbering
    more than 100, you must either include a machine-readable
    Transparent copy along with each Opaque copy, or state in or with
    each Opaque copy a computer-network location from which the general
    network-using public has access to download using public-standard
    network protocols a complete Transparent copy of the Document, free
    of added material. If you use the latter option, you must take
    reasonably prudent steps, when you begin distribution of Opaque
    copies in quantity, to ensure that this Transparent copy will remain
    thus accessible at the stated location until at least one year after
    the last time you distribute an Opaque copy (directly or through
    your agents or retailers) of that edition to the public.

    It is requested, but not required, that you contact the authors of
    the Document well before redistributing any large number of copies,
    to give them a chance to provide you with an updated version of the
    Document.

5.  MODIFICATIONS

    You may copy and distribute a Modified Version of the Document under
    the conditions of sections 2 and 3 above, provided that you release
    the Modified Version under precisely this License, with the Modified
    Version filling the role of the Document, thus licensing
    distribution and modification of the Modified Version to whoever
    possesses a copy of it. In addition, you must do these things in the
    Modified Version:

    A. Use in the Title Page (and on the covers, if any) a title
       distinct from that of the Document, and from those of previous
       versions (which should, if there were any, be listed in the
       History section of the Document). You may use the same title as a
       previous version if the original publisher of that version gives
       permission.

    B. List on the Title Page, as authors, one or more persons or
       entities responsible for authorship of the modifications in the
       Modified Version, together with at least five of the principal
       authors of the Document (all of its principal authors, if it has
       fewer than five), unless they release you from this requirement.

    C. State on the Title page the name of the publisher of the Modified
       Version, as the publisher.

    D. Preserve all the copyright notices of the Document.

    E. Add an appropriate copyright notice for your modifications
       adjacent to the other copyright notices.

    F. Include, immediately after the copyright notices, a license
       notice giving the public permission to use the Modified Version
       under the terms of this License, in the form shown in the
       Addendum below.

    G. Preserve in that license notice the full lists of Invariant
       Sections and required Cover Texts given in the Document's license
       notice.

    H. Include an unaltered copy of this License.

    I. Preserve the section Entitled "History", Preserve its Title, and
       add to it an item stating at least the title, year, new authors,
       and publisher of the Modified Version as given on the Title Page.
       If there is no section Entitled "History" in the Document, create
       one stating the title, year, authors, and publisher of the
       Document as given on its Title Page, then add an item describing
       the Modified Version as stated in the previous sentence.

    J. Preserve the network location, if any, given in the Document for
       public access to a Transparent copy of the Document, and likewise
       the network locations given in the Document for previous versions
       it was based on. These may be placed in the "History" section.
       You may omit a network location for a work that was published at
       least four years before the Document itself, or if the original
       publisher of the version it refers to gives permission.

    K. For any section Entitled "Acknowledgements" or "Dedications",
       Preserve the Title of the section, and preserve in the section
       all the substance and tone of each of the contributor
       acknowledgements and/or dedications given therein.

    L. Preserve all the Invariant Sections of the Document, unaltered in
       their text and in their titles. Section numbers or the equivalent
       are not considered part of the section titles.

    M. Delete any section Entitled "Endorsements". Such a section may
       not be included in the Modified Version.

    N. Do not retitle any existing section to be Entitled "Endorsements"
       or to conflict in title with any Invariant Section.

    O. Preserve any Warranty Disclaimers.

    If the Modified Version includes new front-matter sections or
    appendices that qualify as Secondary Sections and contain no
    material copied from the Document, you may at your option designate
    some or all of these sections as invariant. To do this, add their
    titles to the list of Invariant Sections in the Modified Version's
    license notice. These titles must be distinct from any other section
    titles.

    You may add a section Entitled "Endorsements", provided it contains
    nothing but endorsements of your Modified Version by various
    parties---for example, statements of peer review or that the text
    has been approved by an organization as the authoritative definition
    of a standard.

    You may add a passage of up to five words as a Front-Cover Text, and
    a passage of up to 25 words as a Back-Cover Text, to the end of the
    list of Cover Texts in the Modified Version. Only one passage of
    Front-Cover Text and one of Back-Cover Text may be added by (or
    through arrangements made by) any one entity. If the Document
    already includes a cover text for the same cover, previously added
    by you or by arrangement made by the same entity you are acting on
    behalf of, you may not add another; but you may replace the old one,
    on explicit permission from the previous publisher that added the
    old one.

    The author(s) and publisher(s) of the Document do not by this
    License give permission to use their names for publicity for or to
    assert or imply endorsement of any Modified Version.

6.  COMBINING DOCUMENTS

    You may combine the Document with other documents released under
    this License, under the terms defined in section 4 above for
    modified versions, provided that you include in the combination all
    of the Invariant Sections of all of the original documents,
    unmodified, and list them all as Invariant Sections of your combined
    work in its license notice, and that you preserve all their Warranty
    Disclaimers.

    The combined work need only contain one copy of this License, and
    multiple identical Invariant Sections may be replaced with a single
    copy. If there are multiple Invariant Sections with the same name
    but different contents, make the title of each such section unique
    by adding at the end of it, in parentheses, the name of the original
    author or publisher of that section if known, or else a unique
    number. Make the same adjustment to the section titles in the list
    of Invariant Sections in the license notice of the combined work.

    In the combination, you must combine any sections Entitled "History"
    in the various original documents, forming one section Entitled
    "History"; likewise combine any sections Entitled
    "Acknowledgements", and any sections Entitled "Dedications". You
    must delete all sections Entitled "Endorsements."

7.  COLLECTIONS OF DOCUMENTS

    You may make a collection consisting of the Document and other
    documents released under this License, and replace the individual
    copies of this License in the various documents with a single copy
    that is included in the collection, provided that you follow the
    rules of this License for verbatim copying of each of the documents
    in all other respects.

    You may extract a single document from such a collection, and
    distribute it individually under this License, provided you insert a
    copy of this License into the extracted document, and follow this
    License in all other respects regarding verbatim copying of that
    document.

8.  AGGREGATION WITH INDEPENDENT WORKS

    A compilation of the Document or its derivatives with other separate
    and independent documents or works, in or on a volume of a storage
    or distribution medium, is called an "aggregate" if the copyright
    resulting from the compilation is not used to limit the legal rights
    of the compilation's users beyond what the individual works permit.
    When the Document is included in an aggregate, this License does not
    apply to the other works in the aggregate which are not themselves
    derivative works of the Document.

    If the Cover Text requirement of section 3 is applicable to these
    copies of the Document, then if the Document is less than one half
    of the entire aggregate, the Document's Cover Texts may be placed on
    covers that bracket the Document within the aggregate, or the
    electronic equivalent of covers if the Document is in electronic
    form. Otherwise they must appear on printed covers that bracket the
    whole aggregate.

9.  TRANSLATION

    Translation is considered a kind of modification, so you may
    distribute translations of the Document under the terms of section
    4. Replacing Invariant Sections with translations requires special
    permission from their copyright holders, but you may include
    translations of some or all Invariant Sections in addition to the
    original versions of these Invariant Sections. You may include a
    translation of this License, and all the license notices in the
    Document, and any Warranty Disclaimers, provided that you also
    include the original English version of this License and the
    original versions of those notices and disclaimers. In case of a
    disagreement between the translation and the original version of
    this License or a notice or disclaimer, the original version will
    prevail.

    If a section in the Document is Entitled "Acknowledgements",
    "Dedications", or "History", the requirement (section 4) to Preserve
    its Title (section 1) will typically require changing the actual
    title.

10. TERMINATION

    You may not copy, modify, sublicense, or distribute the Document
    except as expressly provided under this License. Any attempt
    otherwise to copy, modify, sublicense, or distribute it is void, and
    will automatically terminate your rights under this License.

    However, if you cease all violation of this License, then your
    license from a particular copyright holder is reinstated (a)
    provisionally, unless and until the copyright holder explicitly and
    finally terminates your license, and (b) permanently, if the
    copyright holder fails to notify you of the violation by some
    reasonable means prior to 60 days after the cessation.

    Moreover, your license from a particular copyright holder is
    reinstated permanently if the copyright holder notifies you of the
    violation by some reasonable means, this is the first time you have
    received notice of violation of this License (for any work) from
    that copyright holder, and you cure the violation prior to 30 days
    after your receipt of the notice.

    Termination of your rights under this section does not terminate the
    licenses of parties who have received copies or rights from you
    under this License. If your rights have been terminated and not
    permanently reinstated, receipt of a copy of some or all of the same
    material does not give you any rights to use it.

11. FUTURE REVISIONS OF THIS LICENSE

    The Free Software Foundation may publish new, revised versions of
    the GNU Free Documentation License from time to time. Such new
    versions will be similar in spirit to the present version, but may
    differ in detail to address new problems or concerns. See
    http://www.gnu.org/copyleft/.

    Each version of the License is given a distinguishing version
    number. If the Document specifies that a particular numbered version
    of this License "or any later version" applies to it, you have the
    option of following the terms and conditions either of that
    specified version or of any later version that has been published
    (not as a draft) by the Free Software Foundation. If the Document
    does not specify a version number of this License, you may choose
    any version ever published (not as a draft) by the Free Software
    Foundation. If the Document specifies that a proxy can decide which
    future versions of this License can be used, that proxy's public
    statement of acceptance of a version permanently authorizes you to
    choose that version for the Document.

12. RELICENSING

    "Massive Multiauthor Collaboration Site" (or "MMC Site") means any
    World Wide Web server that publishes copyrightable works and also
    provides prominent facilities for anybody to edit those works. A
    public wiki that anybody can edit is an example of such a server. A
    "Massive Multiauthor Collaboration" (or "MMC") contained in the site
    means any set of copyrightable works thus published on the MMC site.

    "CC-BY-SA" means the Creative Commons Attribution-Share Alike 3.0
    license published by Creative Commons Corporation, a not-for-profit
    corporation with a principal place of business in San Francisco,
    California, as well as future copyleft versions of that license
    published by that same organization.

    "Incorporate" means to publish or republish a Document, in whole or
    in part, as part of another Document.

    An MMC is "eligible for relicensing" if it is licensed under this
    License, and if all works that were first published under this
    License somewhere other than this MMC, and subsequently incorporated
    in whole or in part into the MMC, (1) had no cover texts or
    invariant sections, and (2) were thus incorporated prior to November
    1, 2008.

    The operator of an MMC Site may republish an MMC contained in the
    site under CC-BY-SA on the same site at any time before August 1,
    2009, provided the MMC is eligible for relicensing.

**ADDENDUM: How to use this License for your documents**

To use this License in a document you have written, include a copy of
the License in the document and put the following copyright and license
notices just after the title page:

.. code-block:: text

     Copyright (C)  year  your name.
     Permission is granted to copy, distribute and/or modify this document
     under the terms of the GNU Free Documentation License, Version 1.3
     or any later version published by the Free Software Foundation;
     with no Invariant Sections, no Front-Cover Texts, and no Back-Cover
     Texts.  A copy of the license is included in the section entitled ``GNU
     Free Documentation License''.

If you have Invariant Sections, Front-Cover Texts and Back-Cover Texts,
replace the "with...Texts." line with this:

.. code-block:: text

       with the Invariant Sections being list their titles, with
       the Front-Cover Texts being list, and with the Back-Cover Texts
       being list.

If you have Invariant Sections without Cover Texts, or some other
combination of the three, merge those two alternatives to suit the
situation.

If your document contains nontrivial examples of program code, we
recommend releasing these examples in parallel under your choice of free
software license, such as the GNU General Public License, to permit
their use in free software.
