.. _namestore_rest_api:

Namestore
=========

The NAMESTORE REST API.

Data model
^^^^^^^^^^

The following data model definitions are expected to be used in a JSON representation
in the respective requests.
Responses are provided in the JSON format accordingly.

.. _RecordSet:
.. ts:def:: RecordSet

    interface RecordSet {

      // The name of the record set
      record_name: string;

      // The record set data array.
      data: RecordData[];

    }

.. _RecordData:
.. ts:def:: RecordData

    interface RecordData {

      // The string representation of the record data value, e.g. "1.2.3.4" for an A record
      value: string;

      // The string representation of the record type, e.g. "A" for an IPv4 address
      record_type: string;

      // The relative expiration time, in microseconds. Set if is_relative_expiration: true
      relative_expiration: integer;

      // The absolute expiration time, in microseconds. Set if is_relative_expiration: false
      absolute_expiration: integer;

      // Whether or not this is a private record
      is_private: boolean;

      // Whether or not the expiration time is relative (else absolute)
      is_relative_expiration: boolean;

      // Whether or not this is a supplemental record
      is_supplemental: boolean;

      // Whether or not this is a shadow record
      is_shadow: boolean

      // Whether or not this is a maintenance record
      is_maintenance: boolean
    }

.. _NamestoreError:
.. ts:def:: GnunetError

  interface GnunetError {

    // The error description
    error: string;

    // The error code
    error_code: integer

  }


**NOTE:** All endpoints can return with an HTTP status (5xx).
In this case the response body contains a `NamestoreError`_.


Requests
^^^^^^^^

.. http:get:: /namestore/$ZNAME

  This endpoint returns all namestore entries for one zone identified by ``$ZNAME``.

  **Request**

  :query record_type: *Optional*. The string representation of a DNS or GNS record type. If given, only record sets including record with this type will be returned.
  :query omit_private: *Optional*. If set, private records are omitted from the results.
  :query include_maintenance: *Optional*. If set, maintenance records are included in the results.

  **Response**

  :http:statuscode:`200 Ok`:
    The body is a `RecordSet`_ array.
  :http:statuscode:`404 Not found`:
    The zone was not found.

.. http:get:: /namestore/$ZNAME/$LABEL

  This endpoint returns the record set under label ``$LABEL`` of the zone
  identified by ``$ZNAME``.

  **Request**

  :query record_type: *Optional*. The string representation of a DNS or GNS record type. If given, only record sets including record with this type will be returned.
  :query omit_private: *Optional*. If set, private records are omitted from the results.
  :query include_maintenance: *Optional*. If set, maintenance records are included in the results.

  **Response**

  :http:statuscode:`200 Ok`:
    The body is a `RecordSet`_.
  :http:statuscode:`404 Not found`:
    The zone or label was not found.


.. http:post:: /namestore/$ZNAME

  Create or append a `RecordSet`_ to a the zone identified by ``$ZNAME``.

  **Request**
    The request body is a single `RecordSet`_.

  **Response**

  :http:statuscode:`204 No Content`:
    The zone was successfully added.
  :http:statuscode:`404 Not found`:
    The zone was not found.

.. http:put:: /namestore/$ZNAME

  Create or replace a `RecordSet`_ in the zone identified by ``$ZNAME``.

  **Request**
    The request body is a single `RecordSet`_.

  **Response**

  :http:statuscode:`204 No Content`:
    The zone was successfully updated.
  :http:statuscode:`404 Not found`:
    The zone was not found.



.. http:delete:: /namestore/$ZNAME/$LABEL

  Delete all records under name ``$LABEL`` in the zone identified by ``$ZNAME``.

  **Response**

  :http:statuscode:`204 No Content`:
    The records were successfully deleted.
  :http:statuscode:`404 Not found`:
    The zone or label was not found.

.. http:post:: /namestore/import/$ZNAME

  Bulk import of record sets into the zone identified by ``$ZNAME``.
  This API adds the provided array of `RecordSet`_ to the zone.
  This operation does **NOT** replace existing records under the label(s).
  If you want a clean import, reset your database before using this API.
  This API is making sure that the records are added within one database
  transaction, calling ``GNUNET_NAMESTORE_transaction_begin``,
  ``GNUNET_NAMESTORE_records_store2`` (successively, if necessary) and finally
  ``GNUNET_NAMESTORE_transaction_commit``.

  **Request**
    The request body is a `RecordSet`_ array.

  **Response**

  :http:statuscode:`204 No Content`:
    The record sets were successfully added.
  :http:statuscode:`404 Not found`:
    The zone $ZNAME was not found.


