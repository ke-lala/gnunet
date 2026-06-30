Configuration
=============

In order to start the REST service, execute:

::

   $ gnunet-arm -i rest

The REST service will listen by default on port 7776.
The service is run by each user so you may have to modify the port accordingly:

::

  $ gnunet-config -s rest -o HTTP_PORT -V 7788

Note that you may need to authenticate agains the API using HTTP basic authentication.
The REST service autogenerates a password upon first launch.
You can get your user-specific authentication secret by executing:

::

  $ SECRET=$(gnunet-config -f -s rest -o BASIC_AUTH_SECRET_FILE)

To access the REST API, you can use any HTTP client such as a browser or cURL:

::

  $ curl localhost:7776/identity -u<$USER>:<$SECRET>

You may disable the authentication if you want to by executing:

::

  $ gnunet-config -s rest -o BASIC_AUTH_ENABLED -V NO

However, disabling authentication is not recommended.


