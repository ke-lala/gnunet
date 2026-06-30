Config
======

Definition
~~~~~~~~~~

Variables in single quotes ``'...'`` can or must be changed according to your specific case.

``config`` refers to the configuration file.

``'section'`` is a section of settings in the configuration file.

``'option'`` is an setting in the configuration file with a modifiable ``'value'``.

Configuration
-------------

The configuration file is divided in ``sections``, each consisting of various ``options`` with their corresponding ``values``.

Error Response
--------------

An error response is sent in the JSON format: ``{"error":"*error_description*"}``

Following numbers are added for references inside the documentation only.

Error descriptions are::
    
    Nr. Error Description                           - Explanation
    1)  Unknown Error                               - Error is not specified
    2)  Unable to parse JSON Object from "*URI*"    - Corrupt JSON data given

Error ``1)`` is always possible and is not listed in following requests.

ATTENTION: Any error message from the Configuration API (not REST API) can occur and can be returned in the error response. These responses are not listed here.

Response Code
-------------

A response of a message has a HTTP response code. Usually, this code is 200 OK for a successful response. The code changes in some cases::

    a) 200 OK           - Normal response (but may contain an error message)
    b) 201 Created      - Success after POST request
    c) 400 Bad Request  - Invalid request

Requests
~~~~~~~~

GET Requests
------------

+--------------------+---------------------------------------------------------------+
|**Title**           | Returns the config or the specified section of the config     |
+--------------------+---------------------------------------------------------------+
|**URL**             | :literal:`/config`                                            |
+--------------------+---------------------------------------------------------------+
|**Method**          | **GET**                                                       |
+--------------------+---------------------------------------------------------------+
|**URL Params**      | none                                                          |
+--------------------+---------------------------------------------------------------+
|**Data Params**     | none                                                          |
+--------------------+---------------------------------------------------------------+
|**Success Response**| {"section":{"option":"*value*",...},...}                      |
+--------------------+---------------------------------------------------------------+
|**Error Response**  | {"error":"*error_desc*"}                                      |
+--------------------+---------------------------------------------------------------+

|

+--------------------+---------------------------------------------------------------+
|**Title**           | Returns only a specific section                               |
+--------------------+---------------------------------------------------------------+
|**URL**             | ``/config/'section'``                                         |
+--------------------+---------------------------------------------------------------+
|**Method**          | **GET**                                                       |
+--------------------+---------------------------------------------------------------+
|**URL Params**      | none                                                          |
+--------------------+---------------------------------------------------------------+
|**Data Params**     | none                                                          |
+--------------------+---------------------------------------------------------------+
|**Success Response**| {"option":"*value*",...}                                      |
+--------------------+---------------------------------------------------------------+
|**Error Response**  | {"error":"*error_desc*"}                                      |
+--------------------+---------------------------------------------------------------+


POST Request
------------

+--------------------+---------------------------------------------------------------+
|**Title**           | Creates/modifies options in the config                        |
+--------------------+---------------------------------------------------------------+
|**URL**             | :literal:`/config`                                            |
+--------------------+---------------------------------------------------------------+
|**Method**          | **POST**                                                      |
+--------------------+---------------------------------------------------------------+
|**URL Params**      | none                                                          |
+--------------------+---------------------------------------------------------------+
|**Data Params**     | {"'section'": {"'option'": "'value'",...},...}                |
+--------------------+---------------------------------------------------------------+
|**Success Response**| Response Code: ``b) 200 OK``                                  |
+--------------------+---------------------------------------------------------------+
|**Error Response**  | | {"error":"*error_desc*"}                                    |
+--------------------+---------------------------------------------------------------+


OPTIONS Request
---------------

+--------------------+---------------------------------------------------------------+
|**Title**           | Gets request options                                          |
+--------------------+---------------------------------------------------------------+
|**URL**             | :literal:`/config`                                            |
+--------------------+---------------------------------------------------------------+
|**Method**          | **OPTIONS**                                                   |
+--------------------+---------------------------------------------------------------+
|**URL Params**      | none                                                          |
+--------------------+---------------------------------------------------------------+
|**Data Params**     | none                                                          |
+--------------------+---------------------------------------------------------------+
|**Success Response**|                                                               |
+--------------------+---------------------------------------------------------------+
|**Error Response**  | none                                                          |
+--------------------+---------------------------------------------------------------+
