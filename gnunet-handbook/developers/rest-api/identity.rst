Identity
========

Definition
~~~~~~~~~~

Variables in single quotes ``'...'`` can or must be changed according to your specific case.

``public_key`` is the public key of an identity.

``name`` is the name of an identity. 

``newname`` is the new name of an identity for the rename request.

``subsystem`` is a subsystem, e.g. namestore.

Identity
--------

An identity consists of a ``public key`` and a ``name``. An identity can be assigned to a ``subsystem``. Each subsystem can only have one default identity.


Error Response
--------------

An error response is sent in the JSON format: ``{"error":"*error_description*"}``

Following numbers are added for references inside the documentation only.

Error descriptions are::
    
    Nr. Error Description           - Explanation
    1)  Unknown Error               - Error is not specified
    2)  No identity found           - Identity was not found with given name, public key or no identity was found at all
    3)  Missing identity public key - Identity public key length is zero
    4)  Missing identity name       - Identity name length is zero
    5)  Missing subsystem name      - Subsystem name length is zero
    6)  No data                     - No JSON data given
    7)  Data invalid                - Wrong JSON data given
    8)  Rename failed               - Rename request failed due to wrong name, etc.
    9)  Setting subsystem failed    - Setting the subsystem for an identity failed (usually this error does not occur)

Error ``1)`` is always possible and is not listed in following requests.

ATTENTION: Any error message from the Identity API (not REST API) can occur and can be returned in the error response. These responses are not listed here.

Response Code
-------------

A response of a message has a HTTP response code. Usually, this code is 200 OK for a successful response. The code changes in some cases::

    a) 200 OK         - Normal response (but may contain an error message)
    b) 201 Created    - Success after POST request
    c) 204 No Content - Success PUT or DELETE request
    d) 404 Not Found  - Identity is not found with identifier
    e) 409 Conflict   - PUT or POST request not possible due to existing duplicate
    
``d) 404 Not Found`` is always used when the error message is either ``2)``, ``3)`` or ``4)``.

Requests
~~~~~~~~

GET Requests
------------

+--------------------+---------------------------------------------------------------+
|**Title**           |Returns all identities with name and public key                |
+--------------------+---------------------------------------------------------------+
|**URL**             |:literal:`/identity/all`                                       |
+--------------------+---------------------------------------------------------------+
|**Method**          |**GET**                                                        |
+--------------------+---------------------------------------------------------------+
|**URL Params**      |none                                                           |
+--------------------+---------------------------------------------------------------+
|**Data Params**     |none                                                           |
+--------------------+---------------------------------------------------------------+
|**Success Response**|[{"pubkey":"*public_key*", "name":"*name*"},...]               |
+--------------------+---------------------------------------------------------------+
|**Error Response**  | {"error":"*error_desc*"} :sup:`2`                             |
+--------------------+---------------------------------------------------------------+
|**Attention**       | The response in this request is an array!                     |
+--------------------+---------------------------------------------------------------+

|

+--------------------+----------------------------------------------------------------+
|**Title**           |Returns only a specific identity                                |
+--------------------+----------------------------------------------------------------+
|**URL**             | ``/identity/pubkey/'public_key'`` or ``/identity/name/'name'`` |
+--------------------+----------------------------------------------------------------+
|**Method**          |**GET**                                                         |
+--------------------+----------------------------------------------------------------+
|**URL Params**      |none                                                            |
+--------------------+----------------------------------------------------------------+
|**Data Params**     |none                                                            |
+--------------------+----------------------------------------------------------------+
|**Success Response**|{"pubkey":"*public_key*", "name":"*name*"}                      |
+--------------------+----------------------------------------------------------------+
|**Error Response**  | {"error":"*error_desc*"} :sup:`2; 3 or 4`                      |
+--------------------+----------------------------------------------------------------+

|

+--------------------+---------------------------------------------------------------+
|**Title**           |Returns default identity for specific subsystem                |
+--------------------+---------------------------------------------------------------+
|**URL**             |:literal:`/identity/subsystem/'subsystem'`                     |
+--------------------+---------------------------------------------------------------+
|**Method**          |**GET**                                                        |
+--------------------+---------------------------------------------------------------+
|**URL Params**      |none                                                           |
+--------------------+---------------------------------------------------------------+
|**Data Params**     |none                                                           |
+--------------------+---------------------------------------------------------------+
|**Success Response**|{"pubkey":"*public_key*", "name":"*name*"}                     |
+--------------------+---------------------------------------------------------------+
|**Error Response**  | {"error":"*error_desc*"} :sup:`2; 5`                          |
+--------------------+---------------------------------------------------------------+

POST Request
------------

+--------------------+---------------------------------------------------------------+
|**Title**           |Creates an identity                                            |
+--------------------+---------------------------------------------------------------+
|**URL**             |:literal:`/identity`                                           |
+--------------------+---------------------------------------------------------------+
|**Method**          |**POST**                                                       |
+--------------------+---------------------------------------------------------------+
|**URL Params**      |none                                                           |
+--------------------+---------------------------------------------------------------+
|**Data Params**     | {"name":'*name*'}                                             |
+--------------------+---------------------------------------------------------------+
|**Success Response**| Response Code: :literal:` b) 201 Created`                     |
+--------------------+---------------------------------------------------------------+
|**Error Response**  | | {"error":"*error_desc*"} :sup:`6; 7`                        |
|                    | | *or*                                                        |
|                    | | Response Code: ``e) 409 Conflict`` if name in use           |
+--------------------+---------------------------------------------------------------+

PUT Request
-----------

+--------------------+----------------------------------------------------------------+
|**Title**           |Changes name of identity                                        |
+--------------------+----------------------------------------------------------------+
|**URL**             | ``/identity/pubkey/'public_key'`` or ``/identity/name/'name'`` |
+--------------------+----------------------------------------------------------------+
|**Method**          |**PUT**                                                         |
+--------------------+----------------------------------------------------------------+
|**URL Params**      |none                                                            |
+--------------------+----------------------------------------------------------------+
|**Data Params**     | {"newname":'*newname*'}                                        |
+--------------------+----------------------------------------------------------------+
|**Success Response**| Response Code: :literal:`c) 204 No Content`                    |
+--------------------+----------------------------------------------------------------+
|**Error Response**  | | {"error":"*error_desc*"} :sup:`2; 3 or 4; 6; 7; 8`           |
|                    | | *or*                                                         |
|                    | | Response Code: :literal:`e) 409 Conflict` if newname in use  |
+--------------------+----------------------------------------------------------------+

|

+--------------------+----------------------------------------------------------------+
|**Title**           |Sets identity as default for a subsystem                        |
+--------------------+----------------------------------------------------------------+
|**URL**             | ``/identity/subsystem/'name'``                                 |
+--------------------+----------------------------------------------------------------+
|**Method**          |**PUT**                                                         |
+--------------------+----------------------------------------------------------------+
|**URL Params**      |none                                                            |
+--------------------+----------------------------------------------------------------+
|**Data Params**     | {"subsystem":'*subsystem*'}                                    |
+--------------------+----------------------------------------------------------------+
|**Success Response**| Response Code: :literal:`c) 204 No Content`                    |
+--------------------+----------------------------------------------------------------+
|**Error Response**  | {"error":"*error_desc*"} :sup:`2; 4; 6; 7; 9`                  |
+--------------------+----------------------------------------------------------------+

DELETE Request
--------------

+--------------------+----------------------------------------------------------------+
|**Title**           |Deletes specific identity                                       |
+--------------------+----------------------------------------------------------------+
|**URL**             | ``/identity/pubkey/'public_key'`` or ``/identity/name/'name'`` |
+--------------------+----------------------------------------------------------------+
|**Method**          |**DELETE**                                                      |
+--------------------+----------------------------------------------------------------+
|**URL Params**      |none                                                            |
+--------------------+----------------------------------------------------------------+
|**Data Params**     |none                                                            |
+--------------------+----------------------------------------------------------------+
|**Success Response**| Response Code: :literal:`c) 204 No Content`                    |
+--------------------+----------------------------------------------------------------+
|**Error Response**  | {"error":"*error_desc*"} :sup:`2; 3 or 4`                      |
+--------------------+----------------------------------------------------------------+

OPTIONS Request
---------------

+--------------------+----------------------------------------------------------------+
|**Title**           |Gets request options                                            |
+--------------------+----------------------------------------------------------------+
|**URL**             |:literal:`/identity`                                            |
+--------------------+----------------------------------------------------------------+
|**Method**          |**OPTIONS**                                                     |
+--------------------+----------------------------------------------------------------+
|**URL Params**      |none                                                            |
+--------------------+----------------------------------------------------------------+
|**Data Params**     |none                                                            |
+--------------------+----------------------------------------------------------------+
|**Success Response**|                                                                |
+--------------------+----------------------------------------------------------------+
|**Error Response**  |none                                                            |
+--------------------+----------------------------------------------------------------+
