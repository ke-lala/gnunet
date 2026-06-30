.. _The-graphical-configuration-interface:

The graphical configuration interface
-------------------------------------

.. Inserted from Installation Handbook in original "order".

.. original FIXME said to move to the user's handbook...
   WGL thinks somewhere else...

If you also would like to use ``gnunet-gtk`` and ``gnunet-setup``
(highly recommended for beginners), do:

.. _Configuring-your-peer:

Configuring your peer
~~~~~~~~~~~~~~~~~~~~~

This chapter will describe the various configuration options in GNUnet.

The easiest way to configure your peer is to use the ``gnunet-setup``
tool. ``gnunet-setup`` is part of the ``gnunet-gtk`` package. You might
have to install it separately.

Many of the specific sections from this chapter actually are linked from
within ``gnunet-setup`` to help you while using the setup tool.

While you can also configure your peer by editing the configuration file
by hand, this is not recommended for anyone except for developers as it
requires a more in-depth understanding of the configuration files and
internal dependencies of GNUnet.

.. _Configuration-of-the-HOSTLIST-proxy-settings:

Configuration of the HOSTLIST proxy settings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The hostlist client can be configured to use a proxy to connect to the
hostlist server. This functionality can be configured in the
configuration file directly or using the ``gnunet-setup`` tool.

The hostlist client supports the following proxy types at the moment:

-  HTTP and HTTP 1.0 only proxy

-  SOCKS 4/4a/5/5 with hostname

In addition authentication at the proxy with username and password can
be configured.

To configure proxy support for the hostlist client in the
``gnunet-setup`` tool, select the \"hostlist\" tab and select the
appropriate proxy type. The hostname or IP address (including port if
required) has to be entered in the \"Proxy hostname\" textbox. If
required, enter username and password in the \"Proxy username\" and
\"Proxy password\" boxes. Be aware that this information will be stored
in the configuration in plain text (TODO: Add explanation and generalize
the part in Chapter 3.6 about the encrypted home).

.. _Configuration-of-the-HTTP-and-HTTPS-transport-plugins:

Configuration of the HTTP and HTTPS transport plugins
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The client parts of the http and https transport plugins can be
configured to use a proxy to connect to the hostlist server. This
functionality can be configured in the configuration file directly or
using the gnunet-setup tool.

Both the HTTP and HTTPS clients support the following proxy types at the
moment:

-  HTTP 1.1 proxy

-  SOCKS 4/4a/5/5 with hostname

In addition authentication at the proxy with username and password can
be configured.

To configure proxy support for the clients in the gnunet-setup tool,
select the \"transport\" tab and activate the respective plugin. Now you
can select the appropriate proxy type. The hostname or IP address
(including port if required) has to be entered in the \"Proxy hostname\"
textbox. If required, enter username and password in the \"Proxy
username\" and \"Proxy password\" boxes. Be aware that these information
will be stored in the configuration in plain text.
