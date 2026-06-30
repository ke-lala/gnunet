.. _reclaimID-Identity-Provider:

re:claimID
----------

The re:claimID Identity Provider (IdP) is a decentralized IdP service.
It allows its users to manage and authorize third parties to access
their identity attributes such as email or shipping addresses.

It basically mimics the concepts of centralized IdPs, such as those
offered by Google or Facebook. Like other IdPs, reclaimID features an
(optional) OpenID Connect 1.0-compliant protocol layer that can be used
for websites to integrate reclaimID as an Identity Provider with little
effort.

.. _Managing-Attributes:

Managing Attributes
~~~~~~~~~~~~~~~~~~~

Before adding attributes to an identity, you must first create an ego:

::

   $ gnunet-identity --create="user"

Henceforth, you can manage a new user profile of the user "user".

To add an email address to your user profile, simply use the
``gnunet-reclaim`` command line tool::

::

   $ gnunet-reclaim -e "user" -a "email" -V "username@example.gnunet"

All of your attributes can be listed using the ``gnunet-reclaim``
command line tool as well:

::

   $ gnunet-reclaim -e "user" -D

Currently, and by default, attribute values are interpreted as plain
text. In the future there might be more value types such as X.509
certificate credentials.

.. _Managing-Credentials:

Managing Credentials
~~~~~~~~~~~~~~~~~~~~

Attribute values may reference a claim in a third party attested
credential. Such a credential can have a variety of formats such as
JSON-Web-Tokens or X.509 certificates. Currently, reclaimID only
supports JSON-Web-Token credentials.

To add a credential to your user profile, invoke the ``gnunet-reclaim``
command line tool as follows:

::

   $ gnunet-reclaim -e "user"\
                    --credential-name="email"\
                    --credential-type="JWT"\
                    --value="ey..."

All of your credentials can be listed using the ``gnunet-reclaim``
command line tool as well:

::

   $ gnunet-reclaim -e "user" --credentials

In order to add an attribe backed by a credential, specify the attribute
value as the claim name in the credential to reference along with the
credential ID:

::

   $ gnunet-reclaim -e "user"\
                    --add="email"\
                    --value="verified_email"\
                    --credential-id="<CREDENTIAL_ID>"

.. _Sharing-Attributes-with-Third-Parties:

Sharing Attributes with Third Parties
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want to allow a third party such as a website or friend to access
to your attributes (or a subset thereof) execute:

::

   $ TICKET=$(gnunet-reclaim -e "user"\
                             -r "$RP_KEY"\
                             -i "attribute1,attribute2,...")

The command will return a \"ticket\" string. You must give $TICKET to
the requesting third party.

$RP_KEY is the public key of the third party and
\"attribute1,attribute2,\...\" is a comma-separated list of attribute
names, such as \"email,name,\...\", that you want to share.

The third party may retrieve the key in string format for use in the
above call using \"gnunet-identity\":

.. code-block:: shell

   $ RP_KEY=$(gnunet-identity -d | grep "relyingparty" | awk '{print $3}')

The third party can then retrieve your shared identity attributes using:

.. code-block:: shell

   $ gnunet-reclaim -e "relyingparty" -C "ticket"

Where \"relyingparty\" is the name for the identity behind $RP_KEY that
the requesting party is using. This will retrieve and list the shared
identity attributes. The above command will also work if the user is
currently offline since the attributes are retrieved from GNS. Further,
$TICKET can be re-used later to retrieve up-to-date attributes in case
\"friend\" has changed the value(s). For instance, because his email
address changed.

To list all given authorizations (tickets) you can execute:

.. code-block:: shell

   $ gnunet-reclaim -e "user" -T

.. _Revoking-Authorizations-of-Third-Parties:

Revoking Authorizations of Third Parties
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want to revoke the access of a third party to your attributes you
can execute:

::

   $ gnunet-reclaim -e "user" -R $TICKET

This will prevent the third party from accessing the attribute in the
future. Please note that if the third party has previously accessed the
attribute, there is not way in which the system could have prevented the
thiry party from storing the data. As such, only access to updated data
in the future can be revoked. This behaviour is \_exactly the same\_ as
with other IdPs.

.. _OpenID-Connect:

OpenID Connect
~~~~~~~~~~~~~~

There is an
`https://openid.net/specs/openid-connect-core-1_0.html <OpenID Connect>`__
API for use with re:claimID. However, its use is quite complicated to
setup.

::

   https://api.reclaim/openid/authorize
   http://localhost:7776/openid/token
   http://localhost:7776/openid/userinfo
   http://localhost:7776/openid/login

The token endpoint is protected using HTTP basic authentication. You can
authenticate using any username and the password configured under:

::

   $ gnunet-config -s reclaim-rest-plugin -o OIDC_CLIENT_SECRET

The authorize endpoint is protected using a Cookie which can be obtained
through a request against the login endpoint. This functionality is
meant to be used in the context of the OpenID Connect authorization flow
to collect user consent interactively. Without a Cookie, the authorize
endpoint redirects to a URI configured under:

::

   $ gnunet-config -s reclaim-rest-plugin -o ADDRESS

The token endpoint is protected using OAuth2 and expects the grant which
is retrieved from the authorization endpoint according to the standard.

The userinfo endpoint is protected using OAuth2 and expects a bearer
access token which is retrieved from a token request.

In order to make use of OpenID Connect flows as a user, you need to
install the browser plugin:

-  `Firefox Add-on <https://addons.mozilla.org/addon/reclaimid/>`__

-  `Chrome Web
   Store <https://chrome.google.com/webstore/detail/reclaimid/jiogompmdejcnacmlnjhnaicgkefcfll>`__

In order to create and register an OpenID Connect client as a relying
party, you need to execute the following steps:

::

   $ gnunet-identity -C <client_name>
   $ gnunet-namestore -z <client_name> -a -n "@" -t RECLAIM_OIDC_REDIRECT -V <redirect_uri> -e 1d -p
   $ gnunet-namestore -z <client_name> -a -n "@" -t RECLAIM_OIDC_CLIENT -V "My OIDC Client" -e 1d -p

The \"client_id\" for use in OpenID Connect is the public key of the
client as displayed using:

.. code-block:: shell

   $ gnunet-identity -d grep "relyingparty" | awk '{print $3}'

The RECLAIM_OIDC_REDIRECT record contains your website redirect URI. You
may use any globally unique DNS or GNS URI. The RECLAIM_OIDC_CLIENT
record represents the client description which whill be displayed to
users in an authorization request.

Any website or relying party must use the authorization endpoint
https://api.reclaim/openid/authorize in its authorization redirects,
e.g.

.. code-block:: html

   <a href="https://api.reclaim/openid/authorize?client_id=<PKEY>\
                                                &scope=openid email\
                                                &redirect_uri=<redirect_uri>\
                                                &nonce=<random>">Login</a>

This will direct the user's browser onto his local reclaimID instance.
After giving consent, you will be provided with the OpenID Connect
authorization code according to the specifications at your provided
redirect URI.

The ID Tokens issues by the token endpoints are signed using HS512 with
the shared secret configured under:

::

   $ gnunet-config -s reclaim-rest-plugin -o JWT_SECRET

The authorization code flow optionally supports `Proof Key for Code
Exchange <https://tools.ietf.org/html/rfc7636>`__. If PKCE is used, the
client does not need to authenticate against the token endpoint.

.. _Providing-Third-Party-Attestation:

Providing Third Party Attestation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you are running an identity provider (IdP) service you may be able to
support providing credentials for re:claimID users. IdPs can issue JWT
credentials as long as they support OpenID Connect and `OpenID Connect
Discovery <https://openid.net/specs/openid-connect-discovery-1_0.html>`__.

In order to allow users to import attributes through the re:claimID user
interface, you need to register the following public OAuth2/OIDC client:

-  client_id: reclaimid

-  client_secret: none

-  redirect_uri: https://ui.reclaim (The URI of the re:claimID
   webextension)

-  grant_type: authorization_code with PKCE
   (`RFC7636 <https://tools.ietf.org/html/rfc7636>`__)

-  scopes: all you want to offer.

-  id_token: JWT

When your users add an attribute with name \"email\" which supports
webfinger discovery they will be prompted with the option to retrieve
the OpenID Connect ID Token through the user interface.




