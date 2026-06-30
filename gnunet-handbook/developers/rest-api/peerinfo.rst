Peerinfo
========

Definition
~~~~~~~~~~

Variables in single quotes ``'...'`` can or must be changed according to your specific case.

``friend`` is to enable the optional friend information. It is either ``yes`` or can be left away.

Peer
----

A peer consists of an ``identifier`` and one or more ``addresses`` with ``expiration dates``.

Peerinfo Response
-----------------

The response of the peerinfo API is a JSON Array:

.. code-block:: text

   [
      {
        "peer":'identifier', 
        "array": [
                   {
                     "address":'peer_address',
                     "expires":'address_expiration'
                   }, 
                   ...
                 ]
      },
      ...
    ]
    
``ìdentifier`` is a 52-character, alphanumeric identifier of the peer.

``peer_address`` is one URI as string.

``address_expiration`` is the date, when the address expires, e.g. "Wed Aug 1 10:00:00 2018".

    
Error Response
--------------

An error response is sent in the JSON format: ``{"error":"*error_description*"}``

Following numbers are added for references inside the documentation only.

Error descriptions are::
    
    Nr. Error Description           - Explanation
    1)  Unknown Error               - Error is not specified
    2)  No peers found              - Peers were not found, this is combined with the HTTP Error Code 404 Not Found

Error ``1)`` is always possible and is not listed in following requests.

ATTENTION: Any error message from the Peerinfo API (not REST API) can occur and can be returned in the error response. These responses are not listed here.

Requests
~~~~~~~~

GET Request
------------

+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Title**           | Returns all peers and resolves their addresses                                                                            |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**URL**             | :literal:`/peerinfo`                                                                                                      |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Method**          | **GET**                                                                                                                   |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**URL Params**      | ``?friend='friend'`` optional                                                                                             |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Data Params**     | none                                                                                                                      |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Success Response**| Peerinfo Response *or* Response Code: ``500 Internal Server Error``                                                       |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Error Response**  | {"error":"*error_desc*"} :sup:`2`                                                                                         |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+

OPTIONS Request
---------------

+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Title**           |Gets request options                                                                                                       |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**URL**             |:literal:`/peerinfo`                                                                                                       |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Method**          |**OPTIONS**                                                                                                                |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**URL Params**      |none                                                                                                                       |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Data Params**     |none                                                                                                                       |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Success Response**|                                                                                                                           |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
|**Error Response**  |none                                                                                                                       |
+--------------------+---------------------------------------------------------------------------------------------------------------------------+
