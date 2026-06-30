
.. _GNUnet-Developer-Handbook:

#########################
Developer Handbook
#########################

This book is intended to be an introduction for programmers that want to
extend the GNUnet framework. GNUnet is more than a simple peer-to-peer
application.

For developers, GNUnet is:

* developed by a community that believes in the GNU philosophy
* Free Software (Free as in Freedom), licensed under the 
  `GNU Affero General Public License`_
* A set of standards, including coding conventions and architectural rules
* A set of layered protocols, both specifying the communication
  between peers as well as the communication between components
  of a single peer
* A set of libraries with well-defined APIs suitable for
  writing extensions

In particular, the architecture specifies that a peer consists of many
processes communicating via protocols. Processes can be written in almost
any language.
``C``, ``Java`` and ``Guile`` APIs exist for accessing existing
services and for writing extensions.
It is possible to write extensions in other languages by
implementing the necessary IPC protocols.

GNUnet can be extended and improved along many possible dimensions, and
anyone interested in Free Software and Freedom-enhancing Networking is
welcome to join the effort. This Developer Handbook attempts to provide
an initial introduction to some of the key design choices and central
components of the system.

This part of the GNUnet documentation is far from complete,
and we welcome informed contributions, be it in the form of
new chapters, sections or insightful comments.

.. toctree::
   :maxdepth: 2

   contributing.rst
   style.rst

   util.rst
   architecture.rst
   cryptographic-material.rst
   stability.rst
   apis/index.rst
   rest-api/index.rst
   tutorial
   livingstandards
   doxygen

.. _GNU Affero General Public License: https://www.gnu.org/licenses/licenses.html#AGPL
