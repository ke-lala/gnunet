
.. index::
  double: subsystem; REST

.. _REST-Subsystem-Dev:

REST
====

.. todo:: Define REST

Using the REST subsystem, you can expose REST-based APIs or services.
The REST service is designed as a pluggable architecture. To create a
new REST endpoint, simply add a library in the form "plugin_rest_*". The
REST service will automatically load all REST plugins on startup.

.. _Namespace-considerations:

Namespace considerations
------------------------

The ``gnunet-rest-service`` will load all plugins that are installed. As
such it is important that the endpoint namespaces do not clash.

For example, plugin X might expose the endpoint "/xxx" while plugin Y
exposes endpoint "/xxx/yyy". This is a problem if plugin X is also
supposed to handle a call to "/xxx/yyy". Currently the REST service will
not complain or warn about such clashes, so please make sure that
endpoints are unambiguous.

.. _Endpoint-documentation:

Endpoint documentation
----------------------

This is WIP. Endpoints should be documented appropriately. Preferably
using annotations.
