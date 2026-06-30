.. _GNUnet-libgnunetutil:

*************
libgnunetutil
*************

.. This section of the docs is particularly amenable to
   integration with the doxygen output, as it is a library and not
   an individual program.

libgnunetutil is the fundamental library that all GNUnet code builds upon.
Ideally, this library should contain most of the platform dependent code
(except for user interfaces and really special needs that only few
applications have). It is also supposed to offer basic services that most
if not all GNUnet binaries require. The code of libgnunetutil is in the
``src/util/`` directory. The public interface to the library is in the
``gnunet_util.h`` header. 

The functions provided by libgnunetutil fall
roughly into the following categories (in roughly the order of importance
for new developers):


* logging (common_logging.c)
* memory allocation (common_allocation.c)
* endianness conversion (common_endian.c)
* internationalization (common_gettext.c)
* String manipulation (string.c)
* file access (disk.c)
* buffered disk IO (bio.c)
* time manipulation (time.c)
* configuration parsing (configuration.c)
* command-line handling (getopt*.c)
* cryptography (crypto_*.c)
* data structures (container_*.c)
* CPS-style scheduling (scheduler.c)
* Program initialization (program.c)
* Networking (network.c, client.c, server*.c, service.c)
* message queuing (mq.c)
* bandwidth calculations (bandwidth.c)
* Other OS-related (os*.c, plugin.c, signal.c)
* Pseudonym management (pseudonym.c)

It should be noted that only developers that fully understand this entire
API will be able to write good GNUnet code.

Ideally, porting GNUnet should only require porting the gnunetutil
library. More testcases for the gnunetutil APIs are therefore a great
way to make porting of GNUnet easier.
