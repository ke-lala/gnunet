---
v: 3

title: "The GNU Taler Protocol"
docname: draft-guetschow-taler-protocol
category: info

ipr: trust200902
workgroup: independent
stream: independent
keyword:
  - taler
  - cryptography
  - ecash
  - payments

#venue:
#  repo: https://git.gnunet.org/lsd0009.git/
#  latest: https://lsd.gnunet.org/lsd0009/

author:
 -
    name: Mikolai Gütschow
    org: TUD Dresden University of Technology
    abbrev: TU Dresden
    street: Helmholtzstr. 10
    city: Dresden
    code: D-01069
    country: Germany
    email: mikolai.guetschow@tu-dresden.de

normative:
  RFC20:
  RFC2104:
  RFC5869:
  RFC6234:
  RFC7748:
  RFC8032:
  RFC8785:
  HKDF: DOI.10.1007/978-3-642-14623-7_34
  SHS: DOI.10.6028/NIST.FIPS.180-4

informative:


--- abstract

\[ TBW \]

--- middle

# Introduction

\[ TBW \]

Beware that this document is still work-in-progress and may contain errors.
Use at your own risk!

# Notation

- `"abc"` denotes the literal string `abc` encoded as ASCII [RFC20], without trailing '\0' character.
- `a | b` denotes the concatenation of a with b
- `len(a)` denotes the length in bytes of the byte string a
- `padZero(y, a)` denotes the byte string a, zero-padded to the length of y bytes
- `bits(x)`/`bytes(x)` denotes the minimal number of bits/bytes necessary to represent the multiple precision integer x
- `uint(y, x)` denotes the `y` least significant bits of the integer `x`, zero-padded and encoded in network byte order (big endian)
- `uintY(x)` where `Y` is a positive integer number is equivalent to `uint(Y, x)`
- `random(y)` denotes a randomly generated sequence of y bits
- `a * b (mod N)` / `a ** b (mod N)` denotes the multiplication / exponentiation of multiple precision integers a and b, modulo N
- `for`, `if`, variable assignment `=`, and conditional operators are to be interpreted like their Python/Julia equivalents
- `data.key` denotes the property `key` on the object `data`
- `0..n` denotes the exclusive range of integer numbers from `0` to `n`, i.e., `0, 1, 2, ..., n-1`
- `⟨dataᵢ⟩` within a context of `i = 0..n` denotes `n` objects `dataᵢ`, represented in memory as a continuous array
- `⟨dataᵢ.key⟩` within a context of `i = 0..n` denotes an array of the `n` properties `key` of all `n` objects `dataᵢ`

# Cryptographic Primitives

// todo: maybe change this description to something more similar to protocol functions (Julia-inspired syntax)

## Cryptographic Hash Functions

### SHA-256 {#sha256}

~~~
SHA-256(msg) -> hash

Input:
    msg     input message of length L < 2^61 octets

Output:
    hash    message digest of fixed length HashLen = 32 octets
~~~

`hash` is the output of SHA-256 as per Sections 4.1, 5.1, 6.1, and 6.2 of [RFC6234].

### SHA-512 {#sha512}

~~~
SHA-512(msg) -> hash

Input:
    msg     input message of length L < 2^125 octets

Output:
    hash    message digest of fixed length HashLen = 64 octets
~~~

`hash` is the output of SHA-512 as per Sections 4.2, 5.2, 6.3, and 6.4 of [RFC6234].

### SHA-512-256 (truncated SHA-512) {#sha512-trunc}

~~~
SHA-512-256(msg) -> hash

Input:
    msg     input message of length L < 2^125 octets

Output:
    hash    message digest of fixed length HashLen = 32 octets
~~~

The output `hash` corresponds to the first 32 octets of the output of SHA-512 defined in {{sha512}}:

~~~
temp = SHA-512(msg)
hash = temp[0:31]
~~~

Note that this operation differs from SHA-512/256 as defined in [SHS] in the initial hash value.


## Message Authentication Codes

### HMAC {#hmac}

~~~
HMAC-Hash(key, text) -> out

Option:
    Hash    cryptographic hash function with output length HashLen

Input:
    key     secret key of length at least HashLen
    text    input data of arbitary length

Output:
    out     output of length HashLen
~~~

`out` is calculated as defined in [RFC2104].


## Key Derivation Functions

### HKDF {#hkdf}

The Hashed Key Derivation Function (HKDF) used in Taler is an instantiation of [RFC5869]
with two different hash functions for the Extract and Expand step as suggested in [HKDF]:
`HKDF-Extract` uses `HMAC-SHA512`, while `HKDF-Expand` uses `HMAC-SHA256` (cf. {{hmac}}).

~~~
HKDF(salt, IKM, info, L) -> OKM

Inputs:
    salt    optional salt value (a non-secret random value);
              if not provided, it is set to a string of 64 zeros.
    IKM     input keying material
    info    optional context and application specific information
              (can be a zero-length string)
    L       length of output keying material in octets
              (<= 255*32 = 8160)

Output:
    OKM      output keying material (of L octets)
~~~

The output OKM is calculated as follows:

~~~
PRK = HKDF-Extract(salt, IKM) with Hash = SHA-512 (HashLen = 64)
OKM = HKDF-Expand(PRK, info, L) with Hash = SHA-256 (HashLen = 32)
~~~

### HKDF-Mod

Based on the HKDF defined in {{hkdf}}, this function returns an OKM that is smaller than a given multiple precision integer N.

~~~
HKDF-Mod(N, salt, IKM, info) -> OKM

Inputs:
    N        multiple precision integer
    salt     optional salt value (a non-secret random value);
              if not provided, it is set to a string of 64 zeros.
    IKM      input keying material
    info     optional context and application specific information
              (can be a zero-length string)

Output:
    OKM      output keying material (smaller than N)
~~~

The final output `OKM` is determined deterministically based on a counter initialized at zero.

~~~
counter = 0
do until OKM < N:
    x = HKDF(salt, IKM, info | uint16(counter), bytes(N))
    OKM = uint(bits(N), x)
    counter += 1
~~~

## Non-Blind Signatures

### Ed25519 {#ed25519}

Taler uses EdDSA instantiated with curve25519 as Ed25519,
as defined in Section 5.1 of [RFC8032].
In particular, Taler does _not_ make use of Ed25519ph or Ed25519ctx
as defined in that document.

#### Key generation

~~~
Ed25519-GetPub(priv) -> pub

Input:
    priv    private Ed25519 key

Output:
    pub     public Ed25519 key
~~~

`pub` is calculated as described in Section 5.1.5 of [RFC8032].

~~~
Ed25519-Keygen() -> (priv, pub)

Output:
    priv    private Ed25519 key
    pub     public Ed25519 key
~~~

`priv` and `pub` are calculated as described in Section 5.1.5 of [RFC8032],
which is equivalent to the following:

~~~
priv = random(256)
pub = Ed25519-GetPub(priv)
~~~

#### Signing

~~~
Ed25519-Sign(priv, msg) -> sig

Inputs:
    priv    Ed25519 private key
    msg     message to be signed

Output:
    sig     signature on the message by the given private key
~~~

`sig` is calculated as described in Section 5.1.6 of [RFC8032].

#### Verifying

~~~
Ed25519-Verify(pub, msg, sig) -> out

Inputs:
    pub     Ed25519 public key
    msg     signed message
    sig     signature on msg

Output:
    out     true, if sig is a valid signature for msg
~~~

`out` is the outcome of the last check of Section 5.1.7 of [RFC8032].

## Key Agreement

### X25519

Taler uses Elliptic Curve Diffie-Hellman (ECDH) on curve25519 as defined in Section 6.1 of [RFC7748],
but reuses Ed25519 keypairs for one side of the agreement instead of random bytes.
Depending on whether the private or public part is from Ed25519, two different functions are used.

{::comment}
see https://libsodium.gitbook.io/doc/advanced/scalar_multiplication
see https://libsodium.gitbook.io/doc/advanced/ed25519-curve25519
{:/}

~~~
ECDH-Ed25519-Priv(priv, pub) -> shared

Input:
    priv    private Ed25519 key
    pub     public X25519 key

Output:
    shared  shared secret based on the given keys
~~~

`shared` is calculated as follows, using the function X25519 defined in Section 5 of [RFC7748]:

~~~
priv' = SHA-512-256(priv)
// todo: missing bit clamping from https://github.com/jedisct1/libsodium/blob/master/src/libsodium/crypto_sign/ed25519/ref10/keypair.c#L71
shared' = X25519(priv', pub)
shared = SHA-512(shared')
~~~

{::comment}
see GNUNET_CRYPTO_eddsa_ecdh
{:/}

~~~
ECDH-Ed25519-Pub(priv, pub) -> shared

Input:
    priv    private X25519 key
    pub     public Ed25519 key

Output:
    shared  shared secret based on the given keys
~~~

`shared` is calculated as follows, using the function X25519 defined in Section 5 of [RFC7748],
and `Convert-Point-Ed25519-Curve25519(p)` which implements the birational map of Section 4.1 of [RFC7748]:

~~~
pub' = Convert-Point-Ed25519-Curve25519(pub)
shared' = X25519(priv, pub')
shared = SHA-512(shared')
~~~

{::comment}
see GNUNET_CRYPTO_eddsa_ecdh
{:/}

~~~
ECDH-GetPub(priv) -> pub

Input:
    priv    private X25519 key

Output:
    pub     public X25519 key
~~~

`pub` is calculated according to Section 6.1 of [RFC7748]:

~~~
pub = X25519(priv, 9)
~~~

{::comment}
see GNUNET_CRYPTO_ecdhe_key_get_public
{:/}

## Blind Signatures

### RSA-FDH {#rsa-fdh}

#### Supporting Functions

~~~
RSA-FDH(msg, pubkey) -> fdh

Inputs:
    msg     message
    pubkey  RSA public key consisting of modulus N and public exponent e

Output:
    fdh     full-domain hash of msg over pubkey.N
~~~

`fdh` is calculated based on HKDF-Mod from {{hkdf-mod}} as follows:

~~~
info = "RSA-FDA FTpsW!"
salt = uint16(bytes(pubkey.N)) | uint16(bytes(pubkey.e))
     | pubkey.N | pubkey.e
fdh = HKDF-Mod(pubkey.N, salt, msg, info)
~~~

The resulting `fdh` can be used to test against a malicious RSA pubkey
by verifying that the greatest common denominator (gcd) of `fdh` and `pubkey.N` is 1.

~~~
RSA-FDH-Derive(bks, pubkey) -> out

Inputs:
    bks     blinding key secret of length L = 32 octets
    pubkey  RSA public key consisting of modulus N and public exponent e

Output:
    out     full-domain hash of bks over pubkey.N
~~~

`out` is calculated based on HKDF-Mod from {{hkdf-mod}} as follows:

~~~
info = "Blinding KDF"
salt = "Blinding KDF extractor HMAC key"
fdh = HKDF-Mod(pubkey.N, salt, bks, info)
~~~

#### Blinding

~~~
RSA-FDH-Blind(msg, bks, pubkey) -> out

Inputs:
    msg     message
    bks     blinding key secret of length L = 32 octets
    pubkey  RSA public key consisting of modulus N and public exponent e

Output:
    out     message blinded for pubkey
~~~

`out` is calculated based on RSA-FDH from {{rsa-fdh}} as follows:

~~~
data = RSA-FDH(msg, pubkey)
r = RSA-FDH-Derive(bks, pubkey)
r_e = r ** pubkey.e (mod pubkey.N)
out = r_e * data (mod pubkey.N)
~~~

#### Signing

~~~
RSA-FDH-Sign(data, privkey) -> sig

Inputs:
    data    data to be signed, an integer smaller than privkey.N
    privkey RSA private key consisting of modulus N and private exponent d

Output:
    sig     signature on data by privkey
~~~

`sig` is calculated as follows:

~~~
sig = data ** privkey.d (mod privkey.N)
~~~

#### Unblinding

~~~
RSA-FDH-Unblind(sig, bks, pubkey) -> out

Inputs:
    sig     blind signature
    bks     blinding key secret of length L = 32 octets
    pubkey  RSA public key consisting of modulus N and public exponent e

Output:
    out     unblinded signature
~~~

`out` is calculated as follows:

~~~
r = RSA-FDH-Derive(bks, pubkey)
r_inv = inverse of r (mod pubkey.N)
out = sig * r_inv (mod pubkey.N)
~~~

#### Verifying

~~~
RSA-FDH-Verify(msg, sig, pubkey) -> out

Inputs:
    msg     message
    sig     signature of pubkey over msg
    pubkey  RSA public key consisting of modulus N and public exponent e

Output:
    out     true, if sig is a valid signature
~~~

`out` is calculated based on RSA-FDH from {{rsa-fdh}} as follows:

~~~
data = RSA-FDH(msg, pubkey)
exp = sig ** pubkey.e (mod pubkey.N)
out = (data == exp)
~~~

### Clause-Schnorr {#cbs}

# Datatypes and Notation

## Amounts {#amounts}

Amounts are represented in Taler as positive fixed-point values
consisting of `value` as the non-negative integer part of the base currency,
the `fraction` given in units of one hundred millionth (1e-8) of the base currency,
and `currency` as the 3-11 ASCII characters identifying the currency.

Whenever used in the protocol, the binary representation of an `amount` is
`uint64(amount.value) | uint32(amount.fraction) | padZero(12, amount.currency)`.

## Timestamps

Absolute timestamps are represented as `uint64(x)` where `x` corresponds to
the microseconds since `1970-01-01 00:00 CEST` (the UNIX epoch).
The special value `0xFFFFFFFFFFFFFFFF` represents "never".
<!--
// todo: check if needed and correct
Relative timestamps are represented as `uint64(x)` where `x` is given in microseconds.
The special value `0xFFFFFFFFFFFFFFFF` represents "forever".
-->

## Signatures

All messages to be signed in Taler start with a header containing their total size
(including the header) and a fixed signing context (purpose) as registered by GANA in the
[GNUnet Signature Purposes](https://gana.gnunet.org/gnunet-signatures/gnunet_signatures.html)
registry. Taler-specific purposes start at 1000.

~~~
Gen-Msg(purpose, msg) -> out

Inputs:
    purpose signature purpose as registered at GANA
    msg     message content (excl. header) to be signed

Output:
    out     complete message (incl. header) to be signed
~~~

`out` is formed as follows:

~~~
out = uint32(len(msg) + 8) | uint32(purpose) | msg
~~~

## Helper Functions

There are a certain number of single-argument functions which are often needed,
and therefore omit the parentheses of the typical function syntax:

- `Knows data` specifies `data` that is known a priori at the start of the protocol operation
- `Determine data` specifies `data` that is determined according to the business logic and current state of the protocol entity
- `Check cond` verifies that the boolean condition or variable `cond` is true,
  or aborts the protocol operation otherwise
- `Persist data` persists the given `data` to the local database
- `data = Lookup by key` retrieves previously persisted `data` by the given `key`
- `Sum ⟨dataᵢ⟩` is valid for numerical objects `dataᵢ` including amounts (cf. {{amounts}}),
  and denotes the numerical sum of these objects

Some more functions that are commonly used throughout {{protocol}}:

~~~
Hash-Denom(denom) =
  SHA-512(uint32(0) | uint32(1) | denom.pub)

Hash-Planchet(planchet, denom) =
  SHA-512( SHA-512( denom.pub ) | uint32(0x1) | planchet )

Hash-Contract(contract) =
  SHA-512( canonicalJSON(contract) | 0x0)

Check-Subtract(value, subtrahend) =
  Check value >= subtrahend
  Persist value -= subtrahend
~~~

`canonicalJSON(data)` canonicalizes `data` represented as JSON
according to the JSON Canonicalization Scheme (JCS) defined in [RFC8785].
Note that `data` as input to `canonicalJSON` is restricted as follows for Taler:

- For JSON Object member names, only strings matching the regular expression `^[0-9A-Z_a-z]+$` or the literal names `$forgettable` or `$forgotten` are allowed.
This makes the sorting of object members easier, as [RFC8785] requires sorting by UTF-16 code points.
- Floating point numbers are forbidden. Numbers must be integers in the range `-(2**53 - 1)` to `(2**52) - 1`.

# The Taler Crypto Protocol {#protocol}

The Taler payment protocol is a token-based _e-cash_ system
which ensures anonymity for payers (much like physical cash),
while guaranteing income transparency on the payees' side (much like most digital payment systems).
Contrary to what the name might suggest,
Taler neither is a separate currency (as cryptocurrencies do)
nor is it tied to a specific currency.
Instead, the payment system operator offering the Taler payment protocol
can freely choose the assets backing the payment system.

The basic system consists of three types of entities:

1. The Taler _exchange_ is run by the payment system operator.
It is the central, trusted entity which hands out e-cash and holds the corresponding value.
2. A Taler _wallet_ manages e-cash in self-custody for end users.
3. A Taler _merchant_ can redeem e-cash at the exchange
after the wallet authorized a deposit permission during a payment.

E-cash in Taler is represented as digital tokens called _coins_.
They are public-private keypairs where ownership of the coin
is equivalent to the knowledge of the private key `coin.priv`.
Every coin has an initial value corresponding to a denomination (`denom`) offered by the exchange.
The validity of coins is signaled by the presence of
a valid denomination signature `coin.sig` on the (hash of the) public key `coin.pub`.
To ensure payer anonymity, the exchange generates `coin.sig` without learning the actual (hash of) `coin.pub`
using a _blind_ signature scheme.

Wallets obtain coins from the exchange during _withdrawal_ (cf. {{withdraw}})
and use them during _payment_ at merchants, who in turn _deposit_ them at the exchange (cf. {{payment}}).
Residual value on partly spent coins can be _refreshed_ by the wallet subsequently in order to obtain unlinkable change (cf. {{refresh}}).
Taler also supports receiving e-cash in a wallet without acting as a merchant using _wallet-to-wallet payments_ (W2W, cf. {{w2w}}),
which are always handled via the exchange.

Honest operation of the exchange can be optionally supervised by an independant third-party Taler _auditor_.
This supervision is not part of the basic Taler protocol and thus not part of this document.

~~~
                 - exchange -
                /            \
   Withdrawal  /              \  Deposit
     Refresh  /  W2W           \
             /                  \
          wallet ----------- merchant
                   Payment
~~~

// todo: capitalize wallet, exchange, merchant everywhere?

In the default configuration, Taler uses RSA-FDH (cf. {{rsa-fdh}}) for (blind) denomination signatures
and Ed25519 (cf. {{ed25519}}) signatures everywhere else.
Clause-Schnorr Signatures (cf. {{cbs}}) provide an alternative blind signature scheme operating on Elliptic Curves.
As their usage is still experimental, they are not described as part of this document.

Taler has optional support for age-restricted coins, enabling privacy-preserving age restriction.
As an optional feature, it is not part of the basic Taler protocol and thus left out of the description in this document.

## Obtaining E-Cash

### Withdrawal {#withdraw}

The wallet generates `n > 0` coins `⟨coinᵢ⟩` and requests `n` signatures `⟨blind_sigᵢ⟩` from the exchange,
attributing value to the coins according to `n` chosen denominations `⟨denomᵢ⟩`.
The total value and withdrawal fee (defined by the exchange per denomination)
must be smaller or equal to the amount stored in the single reserve used for withdrawal.

// todo: document TALER_MAX_COINS = 64 per operation (due to CS-encoding)

// todo: extend with extra roundtrip for CBS

~~~
            wallet                                  exchange
Knows ⟨denomᵢ⟩                          Knows ⟨denomᵢ.priv⟩
               |                                        |
+-----------------------------+                         |
| (W1) reserve key generation |                         |
+-----------------------------+                         |
               |                                        |
               |----------- (bank transfer) ----------->|
               | (subject: reserve.pub, amount: value)  |
               |                                        |
               |                      +------------------------------+
               |                      | Persist (reserve.pub, value) |
               |                      +------------------------------+
               |                                        |
+-----------------------------------+                   |
| (W2) coin generation and blinding |                   |
+-----------------------------------+                   |
               |                                        |
               |-------------- /withdraw -------------->|
               |    (reserve.pub, planchets, sig)       |
               |                                        |
               |                      +--------------------------------+
               |                      | (E1) coin issuance and signing |
               |                      +--------------------------------+
               |                                        |
               |<---------- (⟨blind_sigᵢ⟩) -------------|
               |                                        |
+----------------------+                                |
| (W3) coin unblinding |                                |
+----------------------+                                |
               |                                        |
~~~

where (for RSA, without age-restriction)

~~~ pseudocode
(W1) reserve key generation (wallet)

reserve = Ed25519-Keygen()
Persist (reserve, value)
~~~

The wallet derives coins and blinding secrets using a HKDF from a single seed per withdrawal operation,
together with an integer index.
This is strictly speaking an implementation detail since the seed is never revealed to any other party,
and might be chosen to be implemented differently.

~~~ pseudocode
(W2) coin generation and blinding (wallet)

batch_seed = random(256)
Persist batch_seed
for i in 0..n:
  coin_seedᵢ = HKDF(uint32(i), batch_seed, "taler-withdrawal-coin-derivation", 64)
  blind_secretᵢ = coin_seedᵢ[32:]
  coinᵢ.priv = coin_seedᵢ[:32]
  coinᵢ.pub = Ed25519-GetPub(coinᵢ.priv)
  h_denomᵢ = Hash-Denom(denomᵢ)
  planchetᵢ = RSA-FDH-Blind(SHA-512(coinᵢ.pub), blind_secretᵢ, denomᵢ.pub)
  h_planchetᵢ = Hash-Planchet(planchetᵢ, denomᵢ)
planchets = (⟨h_denomᵢ⟩, ⟨planchetᵢ⟩)
msg = Gen-Msg(WALLET_RESERVE_WITHDRAW,
    ( Sum ⟨denomᵢ.value⟩ | Sum ⟨denomᵢ.fee_withdraw⟩
    | SHA-512( ⟨h_planchetᵢ⟩ ) | uint256(0x0) | uint32(0x0) | uint32(0x0) ))
sig = Ed25519-Sign(reserve.priv, msg)

// todo: exchange.git uses different derivation than wallet-core.git (above):
⟨coin_seedᵢ⟩ = HKDF(uint32(n), batch_seed, "taler-withdraw-secrets", 32*n)
for i in 0..n:
  blind_secretᵢ = HKDF("bks", coin_seedᵢ, "", 32)
  coinᵢ.priv = HKDF("coin", coin_seedᵢ, "", 32)
~~~

~~~ pseudocode
(E1) coin issuance and signing (exchange)

(⟨h_denomᵢ⟩, ⟨planchetᵢ⟩) = planchets
for i in 0..n:
  denomᵢ = Lookup by h_denomᵢ
  Check denomᵢ known and not withdraw-expired
  h_planchetᵢ = Hash-Planchet(planchetᵢ, denomᵢ)
msg = Gen-Msg(WALLET_RESERVE_WITHDRAW,
    ( Sum ⟨denomᵢ.value⟩ | Sum ⟨denomᵢ.fee_withdraw⟩
    | SHA-512( ⟨h_planchetᵢ⟩ ) | uint256(0x0) | uint32(0x0) | uint32(0x0) ))
Check Ed25519-Verify(reserve.pub, msg, sig)
Check reserve KYC status ok or not needed
total = Sum ⟨denomᵢ.value⟩ + Sum ⟨denomᵢ.fee_withdraw⟩
Check-Subtract(reserve.balance, total)
for i in 0..n:
  blind_sigᵢ = RSA-FDH-Sign(planchetᵢ, denomᵢ.priv)
Persist withdrawal // todo: what exactly? should be checked first for replay?
~~~

~~~ pseudocode
(W3) coin unblinding (wallet)

for i in 0..n:
  coinᵢ.sig = RSA-FDH-Unblind(blind_sigᵢ, blind_secretᵢ, denomᵢ.pub)
  Check RSA-FDH-Verify(SHA-512(coinᵢ.pub), coinᵢ.sig, denomᵢ.pub)
  coinᵢ.value = denomᵢ.value
  coinᵢ.h_denom = h_denomᵢ
  coinᵢ.blind_secret = blind_secretᵢ  // todo: why save blind_secret, if batch_seed already persisted?
Persist ⟨coinᵢ⟩
~~~

### Recoup {#withdraw-recoup}

// todo

## Payment with E-Cash

### Payment and Deposit {#payment}

The wallet obtains `contract` information for an `order` from the merchant
after claiming it with a `nonce`.
Payment of the order is prepared by signing (partial) deposit authorizations `⟨depositᵢ⟩` with coins `⟨coinᵢ⟩` of certain denominations `⟨denomᵢ⟩`,
where the sum of all contributions (`contributionᵢ + denomᵢ.fee_deposit <= denomᵢ.value`)
must match the `contract.price` plus potential deposit fees `⟨denomᵢ.fee_deposit⟩`.
The payment is complete as soon as the merchant successfully redeems the deposit authorizations at the exchange.

Deposit could also be used directly by a wallet with its own payto and a minimal contract.

// todo: should we integrate payment templates here?

~~~
      wallet                        merchant                       exchange
Knows ⟨coinᵢ⟩                  Knows merchant.priv         Knows exchange.priv
        |                      Knows exchange, payto       Knows ⟨denomᵢ⟩
        |                              |                              |
        |                 +-----------------------+                   |
        |                 | (M1) order generation |                   |
        |                 +-----------------------+                   |
        |                              |                              |
        |<--- (QR-Code / NFC / URI) ---|                              |
        |      (order.{id,token?})     |                              |
        |                              |                              |
+-----------------------+              |                              |
| (W1) nonce generation |              |                              |
+-----------------------+              |                              |
        |                              |                              |
        |-- /orders/{order.id}/claim ->|                              |
        |  (nonce.pub, order.token?)   |                              |
        |                              |                              |
        |                 +--------------------------+                |
        |                 | (M2) contract generation |                |
        |                 +--------------------------+                |
        |                              |                              |
        |<-- (contract, merchant.pub, -|                              |
        |            sig)              |                              |
        |                              |                              |
+--------------------------+           |                              |
| (W2) payment preparation |           |                              |
+--------------------------+           |                              |
        |                              |                              |
        |--- /orders/{order.id}/pay -->|                              |
        |         (⟨depositᵢ⟩)         |                              |
        |                              |                              |
        |                 +--------------------------+                |
        |                 | (M3) deposit preparation |                |
        |                 +--------------------------+                |
        |                              |                              |
        |                              |-------- /batch-deposit ----->|
        |                              | (info, h_contract, ⟨depositᵢ⟩|
        |                              |        merchant.pub, sig)    |
        |                              |                              |
        |                              |                  +--------------------+
        |                              |                  | (E1) deposit check |
        |                              |                  +--------------------+
        |                              |                              |
        |                              |<------ (time_deposit, -------|
        |                              |      exchange.pub, sig)      |
        |                              |                              |
        |                 +---------------------------+               |
        |                 | (M4) deposit verification |               |
        |                 +---------------------------+               |
        |                              |                              |
        |<----------- (sig) -----------|                              |
        |                              |                              |
+---------------------------+          |                              |
| (W3) payment verification |          |                              |
+---------------------------+          |                              |
        |                              |                              |
~~~

where (without age restriction, policy and wallet data hash)

~~~ pseudocode
(M1) order generation (merchant)

wire_salt = random(128)
Determine price, and ASCII strings id, info, token?
Persist order = (id, price, info, token?, wire_salt)
~~~

~~~ pseudocode
(W1) nonce generation (wallet)

nonce = Ed25519-Keygen()
Persist nonce.priv
~~~

Note that the private key of `nonce` is currently not used anywhere in the protocol.
However, it could be used in the future to prove ownership of an order transaction,
enabling use-cases such as "unclaiming" or transferring an order to another person,
or proving the payment without resorting to the individual coins.

~~~ pseudocode
(M2) contract generation (merchant)

Check order.token? == token?
h_wire = HKDF(wire_salt, payto, "merchant-wire-signature", 64)
timestamp = now()
Determine refund_deadline, wire_deadline from timestamp
Determine max_fees from price
contract = (order.{id,price,info,token?}, exchange, h_wire, timestamp, refund_deadline, wire_deadline, max_fees)
contract.nonce = nonce.pub
Persist contract
h_contract = Hash-Contract(contract)
msg = Gen-Msg(MERCHANT_CONTRACT, h_contract)
sig = Ed25519-Sign(merchant.priv, msg)
~~~

~~~ pseudocode
(W2) payment preparation (wallet)

h_contract = Hash-Contract(contract)
msg = Gen-Msg(MERCHANT_CONTRACT, h_contract)
Check Ed25519-Verify(merchant.pub, msg, sig)
Check contract.nonce == nonce
// TODO: double-check extra hash check?
for i in 0..n:
  Determine coinᵢ, denomᵢ, contribution_netᵢ for contract.{exchange,price,max_fees}
  contribution_grossᵢ = contribution_netᵢ + denomᵢ.fee_deposit
  Check-Subtract(coinᵢ.value, contribution_grossᵢ)
  msgᵢ = Gen-Msg(WALLET_COIN_DEPOSIT,
      ( h_contract | uint256(0x0)
      | uint512(0x0) | contract.h_wire | coinᵢ.h_denom
      | timestamp | contract.refund_deadline
      | contribution_grossᵢ | denomᵢ.fee_deposit
      | merchant.pub | uint512(0x0) ))
  sigᵢ = Ed25519-Sign(coinᵢ.priv, msgᵢ)
  depositᵢ = (coinᵢ.{pub,sig,h_denom}, contribution_grossᵢ, sigᵢ)
Persist (contract, ⟨sigᵢ⟩, ⟨depositᵢ⟩)
~~~

~~~ pseudocode
(M3) deposit preparation (merchant)

for i in 0..n:
  denomᵢ = Lookup by depositᵢ.coin.h_denom
  contribution_netᵢ = depositᵢ.contribution_gross - denomᵢ.fee_deposit
Check Sum ⟨contribution_netᵢ⟩ >= contract.price - contract.max_fees
info.time = contract.{timestamp, wire_deadline, refund_deadline}
info.wire = (payto, wire_salt)
h_contract = Hash-Contract(contract)
msg = Gen-Msg(MERCHANT_CONTRACT, h_contract)
sig = Ed25519-Sign(merchant.priv, msg)
~~~

TODO: what about wire_fees, those should be checked for as well, or do we just assume merchant will pay those?
see src/backend/taler-merchant-httpd_post-orders-ORDER_ID-pay.c:2760

~~~ pseudocode
(E1) deposit check (exchange)

h_wire = HKDF(info.wire.wire_salt, info.wire.payto, "merchant-wire-signature", 64)
for i in 0..n:
  coinᵢ = depositᵢ.coin
  denomᵢ = Lookup by coinᵢ.h_denom
  Check denomᵢ known and not deposit-expired
  msgᵢ = Gen-Msg(WALLET_COIN_DEPOSIT,
      ( h_contract | uint256(0x0)
      | uint512(0x0) | h_wire | coinᵢ.h_denom
      | info.time.timestamp | info.time.refund_deadline
      | depositᵢ.contribution_gross | denomᵢ.fee_deposit
      | merchant.pub | uint512(0x0) ))
  Check Ed25519-Verify(coinᵢ.pub, msgᵢ, depositᵢ.sig)
  Check RSA-FDH-Verify(SHA-512(coinᵢ.pub), coinᵢ.sig, denomᵢ.pub)
  Check-Subtract(coinᵢ.value, depositᵢ.contribution_gross)
Persist deposit-record
schedule bank transfer to payto
time_deposit = now()
msg = Gen-Msg(EXCHANGE_CONFIRM_DEPOSIT,
    ( h_contract | h_wire | uint512(0x0)
    | time_deposit | info.time.wire_deadline
    | info.time.refund_deadline
    | Sum ⟨depositᵢ.contribution_gross⟩
    | SHA-512( ⟨depositᵢ.sig⟩ ) | merchant.pub ))
sig = Ed25519-Sign(exchange.priv, msg)
~~~

~~~ pseudocode
(M2) deposit verification (merchant)

h_wire = HKDF(wire_salt, payto, "merchant-wire-signature", 64)
msg = Gen-Msg(EXCHANGE_CONFIRM_DEPOSIT,
    ( h_contract | h_wire | uint512(0x0)
    | time_deposit | contract.wire_deadline
    | contract.refund_deadline
    | Sum ⟨depositᵢ.contribution⟩
    | SHA-512( ⟨depositᵢ.sig⟩ ) | merchant.pub ))
Check Ed25519-Verify(exchange.pub, msg, sig)
msg = Gen-Msg(MERCHANT_PAYMENT_OK, h_contract)
sig = Ed25519-Sign(merchant.priv, msg)
~~~~

~~~ pseudocode
(W3) payment verification (wallet)

msg = Gen-Msg(MERCHANT_PAYMENT_OK, h_contract)
Check Ed25519-Verify(merchant.pub, msg, sig)
~~~

### Refund {#refund}

A wallet can request a refund for an order from the merchant after it has been completed successfully
(cf. {{payment}}) and before the merchant has been paid out by the exchange (i.e., before `contract.wire_deadline`).
The merchant needs to approve the refund via its business logic,
and is free to decide the total amount of the refund
as well as which coins' deposit operations are (potentially partly) invalidated.
After the exchange has accepted the refund request,
the coins obtain their (partial) value back.
The wallet should proceed to refresh (cf. {{refresh}}) the coins before spending them again
to obtain unlinkability.

In case the wallet itself has used deposit to its own payto,
it can act as the merchant in the protocol below.

~~~
      wallet                        merchant                       exchange
Knows order.id                 Knows merchant.priv         Knows deposit_record
Knows contract                         |                         for coinᵢ.pub
        |                              |                              |
+---------------------+                |                              |
| (W1) refund request |                |                              |
+---------------------+                |                              |
        |                              |                              |
        |- /orders/{order.id}/refund ->|                              |
        |          (h_contract)        |                              |
        |                              |                              |
        |                 +------------------------+                  |
        |                 | (M1) refund processing |                  |
        |                 +------------------------+                  |
        |                              |                              |
        |                              |- /coins/{coinᵢ.pub}/refund ->|
        |                              |   (valueᵢ, h_contract, id,   |
        |                              |      merchant.pub, sigᵢ)     |
        |                              |                              |
        |                              |                  +-------------------+
        |                              |                  | (E1) refund check |
        |                              |                  +-------------------+
        |                              |                              |
        |                              |<--- (exchange.pub, sigᵢ) ----|
        |                              |                              |
        |                 +--------------------------+                |
        |                 | (M2) refund confirmation |                |
        |                 +--------------------------+                |
        |                              |                              |
        |<-----(value, ⟨refundᵢ⟩,------|                              |
        |        merchant.pub)         |                              | // todo: why merchant.pub if no sig transmitted?
        |                              |                              |
+-----------------------+              |                              |
| (W2) refund reception |              |                              |
+-----------------------+              |                              |
        |                              |                              |
~~~

where (for RSA, without age-restriction)

{::comment}

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(W1) refund request (wallet)

h_contract = Hash-Contract(contract)
~~~

{::comment}

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(M1) refund processing (merchant)

Check h_contract known and refund possible
time = now()
⟨coinᵢ⟩ = Lookup by h_contract
id = uint32(random(32))
for i in 0..n:
  denomᵢ = Lookup by coinᵢ.h_denom
  valueᵢ = refund amount // todo: split wisely
  msgᵢ = Gen-Msg(MERCHANT_REFUND,
       ( h_contract | coinᵢ.pub | id | valueᵢ | denomᵢ.fee_refund ))
  sigᵢ = Ed25519-Sign(merchant.priv, msgᵢ)
~~~

{::comment}

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(E1) refund check and confirmation (exchange)

deposit_record = Lookup by h_contract // todo: needs to be persisted before with order.id and used coins!
Check refund possible (prior to wire transfer deadline)
for i in 0..n:
  Check coinᵢ.pub part of deposit_record
  denomᵢ = Lookup by coinᵢ.pub
  msgᵢ = Gen-Msg(MERCHANT_REFUND,
      ( h_contract | coinᵢ.pub | id | valueᵢ | denomᵢ.fee_refund ))
  Check Ed25519-Verify(merchant.pub, msgᵢ, sigᵢ)
  Check valueᵢ >= denomᵢ.fee_refund
  remove/update scheduled wire transfer
  Persist coinᵢ.value += valueᵢ - denomᵢ.fee_refund
  msgᵢ = Gen-Msg(MERCHANT_REFUND_OK, SHA-512(order.id))
  sigᵢ = Ed25519-Sign(exchange.priv, msgᵢ)
~~~

{::comment}

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(M2) refund confirmation (merchant)

for i in 0..n:
  msgᵢ = Gen-Msg(MERCHANT_REFUND_OK, SHA-512(order.id))
  Check Ed25519-Verify(exchange.pub, msgᵢ, sigᵢ)
  update business logic
  refundᵢ = (valueᵢ, sigᵢ, id, coinᵢ.pub, time)
value = sum ⟨valueᵢ⟩
~~~

{::comment}

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(W2) refund reception (wallet)

for i in 0..n:
  (valueᵢ, sigᵢ, id, coinᵢ.pub, time) = refundᵢ
update persistent transaction information
refresh ⟨coinᵢ⟩
~~~

## Obtaining unlinkable change

### Refresh {#refresh}

The wallet obtains `n` new coins `⟨coinᵢ⟩` of denominations `⟨denomᵢ⟩`
in exchange for one old `coin` of denomination `denom` from the exchange.
There are three reasons why a wallet needs to do this:

1. Obtaining unlinkable change after using only a part of the coin's value during a payment (cf. {{payment}}),
i.e., where `coin.value < denom.value`
2. Obtaining unlinkable change after a successful refund (cf. {{refund}})
3. Renewing a coin before it deposit-expires

The sum of the refresh fee of `denom` and the new denominations' values and withdrawal fees (defined by the exchange)
must be smaller or equal to the residual value of the old coin (`coin.value`).

The private key of each new coin candidate `⟨coinₖᵢ.priv⟩` is transitively derived from the old coin's private key `coin.priv`
via a 512-bit secret `⟨sharedₖᵢ⟩` according to `Refresh-Derive`.
The secret is regeneratable with the knowledge of `coin.priv` via the link protocol (cf. {{link}}).
The derivation ensures that ownership of coins (knowledge of the private key) is correctly transferred,
and thereby that value transfer among untrusted parties can only happen via payment and deposit, not via refresh.

~~~
Refresh-Derive(shared, i, denom) =
  planchet_seed = HKDF(uint32(i), shared, "taler-coin-derivation", 32)
  blind_secret = HKDF("bks", planchet_seed, "", 32)
  coin.priv = HKDF("coin", planchet_seed, "", 32)
  coin.pub = Ed25519-GetPub(coin.priv)
  planchet = RSA-FDH-Blind(SHA-512(coin.pub), blind_secret, denom.pub)
  h_planchet = Hash-Planchet(planchet, denom)
  return (coin, blind_secret, planchet, h_planchet)
~~~

Taler uses a cut-and-choose protocol with the fixed parameter `κ=3` to enforce correct derivation
of `⟨sharedₖᵢ⟩` from a single seed per batch of planchets `⟨batch_seedₖ⟩`
(in (κ-1)/κ of the cases, making income concealment for tax evasion purposes unpractical).

Refreshing consists of two parts:

1. Melting of the old coin and commiting to κ batches of blinded planchet candidates
2. Revelation of κ-1 secrets `⟨revealed_seedₖ⟩` to prove the proper construction of the (revealed) batches of blinded planchet candidates.

~~~
            wallet                                  exchange
Knows ⟨denomᵢ⟩                          Knows ⟨denomᵢ.priv⟩
Knows coin                                              |
               |                                        |
+-------------------+                                   |
| (W1) coin melting |                                   |
+-------------------+                                   |
               |                                        |
               |---------------- /melt ---------------->|
               |     (coin.{pub,sig,h_denom}, value,    |
               |      refresh_seed, planchets, sig)     |
               |                                        |
               |                      +---------------------------------------+
               |                      | (E1) gamma selection and coin signing |
               |                      +---------------------------------------+
               |                                        |
               |<------ (ɣ, exchange.pub, sig) ---------|
               |                                        |
+------------------------+                              |
| (W2) secret revelation |                              |
+------------------------+                              |
               |                                        |
               |------------ /reveal-melt ------------->|
               |     (commitment, ⟨revealed_seedₖ⟩)     |
               |                                        |
               |                      +----------------------------+
               |                      | (E2) commitment validation |
               |                      +----------------------------+
               |                                        |
               |<---------- (⟨blind_sigᵢ⟩) -------------|
               |                                        |
+----------------------+                                |
| (W3) coin unblinding |                                |
+----------------------+                                |
               |                                        |
~~~

where (for RSA, without age-restriction)

{::comment}
// see TALER_EXCHANGE_get_melt_data
⟨batch_seedₖ⟩ // see TALER_refresh_expand_seed_to_kappa_batch_seeds
⟨transferₖᵢ.priv⟩ // see TALER_refresh_expand_batch_seed_to_transfer_data
h_planchetₖᵢ // see TALER_coin_ev_hash
h_planchetsₖ // see TALER_wallet_blinded_planchet_details_hash
commitment // see TALER_refresh_get_commitment

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(W1) coin melting (wallet)

refresh_seed = random(512)
⟨batch_seedₖ⟩ = HKDF("refresh-batch-seeds", refresh_seed, coin.priv, k*64)
for k in 0..κ:
  ⟨transferₖᵢ.priv⟩ = HKDF("refresh-transfer-private-keys", batch_seedₖ, "", n*32)
  for i in 0..n:
    transferₖᵢ.pub = ECDH-GetPub(transferₖᵢ.priv)
    sharedₖᵢ = ECDH-Ed25519-Pub(transferₖᵢ.priv, coin.pub)
    (coinₖᵢ, blind_secretₖᵢ, planchetₖᵢ, h_planchetₖᵢ) = Refresh-Derive(sharedₖᵢ, denomᵢ)
  h_planchetsₖ = SHA-512( ⟨h_planchetₖᵢ⟩ )
value = coin.denom.fee_refresh + Sum ⟨denomᵢ.value⟩ + Sum ⟨denomᵢ.fee_withdraw⟩
commitment = SHA-512( refresh_seed | uint256(0x0) | coin.pub
                    | value | ⟨h_planchetsₖ⟩ )
for i in 0..n:
  h_denomᵢ = Hash-Denom(denomᵢ)
planchets = (⟨h_denomᵢ⟩, ⟨planchetₖᵢ⟩, ⟨transferₖᵢ.pub⟩))
msg = Gen-Msg(WALLET_COIN_MELT,
    ( commitment | coin.h_denom | uint256(0x0)
    | value | denom.fee_refresh ))
sig = Ed25519-Sign(coin.priv, msg)
Persist (coin.denom.pub, ...) // todo: double-check
~~~

{::comment}

see TEH_handler_melt

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(E1) gamma selection and coin signing (exchange)

denom = Lookup by coin.h_denom
Check denom known and not deposit-expired
Check RSA-FDH-Verify(SHA-512(coin.pub), coin.sig, denom.pub)
Check coin.pub known and dirty
(⟨h_denomᵢ⟩, ⟨planchetₖᵢ⟩, ⟨transferₖᵢ.pub⟩)) = planchets
for i in 0..n:
  denomᵢ = Lookup by h_denomᵢ
  Check denomᵢ known and not withdraw-expired
value' = coin.denom.fee_refresh + Sum ⟨denomᵢ.value⟩ + Sum ⟨denomᵢ.fee_withdraw⟩
Check value' == value
Check-Subtract(coin.value, value)
for k in 0..κ:
  for i in 0..n:
    h_planchetₖᵢ = Hash-Planchet(planchetₖᵢ, denomᵢ)
  h_planchetsₖ = SHA-512( ⟨h_planchetₖᵢ⟩ )
commitment = SHA-512( refresh_seed | uint256(0x0) | coin.pub
                    | value | ⟨h_planchetsₖ⟩ )
msg = Gen-Msg(WALLET_COIN_MELT,
    ( commitment | coin.h_denom | uint256(0x0)
    | value | denom.fee_refresh ))
Check Ed25519-Verify(coin.pub, msg, sig)
refresh_record = Lookup by commitment
(ɣ, _, _, done, _) = refresh_record
if refresh_record not found:
  ɣ = 0..κ at random
  for i in 0..n:
    blind_sigᵢ = RSA-FDH-Sign(planchetᵧᵢ, denomᵧᵢ.priv)
  link_info = (refresh_seed, ⟨transferₖᵢ.pub⟩, ⟨h_denomᵢ⟩, coin_sig)
  Persist refresh_record = (commitment, ɣ, ⟨blind_sigᵢ⟩, h_planchetsᵧ, false, link_info)
msg = Gen-Msg(EXCHANGE_CONFIRM_MELT,
    ( commitment | uint32(ɣ) ))
sig = Ed25519-Sign(exchange.priv, msg)
~~~

{::comment}

// see src/lib/exchange_api_post-melt.c: handle_melt_finished
// see src/lib/exchange_api_post-reveal-melt.c: perform_protocol

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(W2) secret revelation (wallet)

Check exchange.pub known
msg = Gen-Msg(EXCHANGE_CONFIRM_MELT,
    ( commitment | uint32(ɣ) ))
Check Ed25519-Verify(exchange.pub, msg, sig)
Persist refresh-challenge // what exactly?
for k in 0..κ and k != ɣ:
  revealed_seedₖ = batch_seedₖ
~~~

{::comment}

// see TEH_handler_reveal_melt

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(E2) commitment validation (exchange)

refresh_record = Lookup by commitment
(ɣ, ⟨blind_sigᵢ⟩, h_planchetsᵧ, done, _) = refresh_record
Check not done // todo: sure?
for k in 0..κ and k != ɣ:
  ⟨transferₖᵢ.priv⟩ = HKDF("refresh-transfer-private-keys", batch_seedₖ, "", n*32)
  for i in 0..n:
    transferₖᵢ.pub = ECDH-GetPub(transferₖᵢ.priv)
    sharedₖᵢ = ECDH-Ed25519-Pub(transferₖᵢ.priv, coin.pub)
    (_, _, _, h_planchetₖᵢ) = Refresh-Derive(sharedₖᵢ, denomᵢ)
  h_planchetsₖ = SHA-512( ⟨h_planchetₖᵢ⟩ )
value = coin.denom.fee_refresh + Sum ⟨denomᵢ.value⟩ + Sum ⟨denomᵢ.fee_withdraw⟩
commitment' = SHA-512( refresh_seed | uint256(0x0) | coin.pub
                     | value | ⟨h_planchetsₖ⟩ )
Check commitment == commitment'
Persist refresh_record = (_, _, _, true, _)
~~~

{::comment}

// see src/lib/exchange_api_post-reveal-melt.c: reveal_melt_ok

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(W3) coin unblinding (wallet)

for i in 0..n:
  coinᵧᵢ.sig = RSA-FDH-Unblind(blind_sigᵧᵢ, blind_secretᵧᵢ, denomᵢ.pub)
  Check RSA-FDH-Verify(SHA-512(coinᵧᵢ.pub), coinᵧᵢ.sig, denomᵢ.pub)
  coinᵧᵢ.h_denom = h_denomᵢ
  Persist ⟨coinᵧᵢ⟩
~~~

### Link {#link}

Coins ⟨coinᵧᵢ⟩ obtained via the refresh protocol (cf. {{refresh}}) can be regenerated
with the knowledge of the old coin's private key `coin.priv` using the link protocol,
integrated in the coin history endpoint.

~~~
            wallet                                  exchange
Knows coin                              Knows refresh_record for coin.pub
               |                                        |
+----------------------+                                |
| (W1) history request |                                |
+----------------------+                                |
               |                                        |
               |------ /coins/{coin.pub}/history ------>|
               |                 (sig)                  |
               |                                        |
               |                      +----------------------------+
               |                      | (E1) refresh secret lookup |
               |                      +----------------------------+
               |                                        |
               |<------------- (melt_info) -------------|
               |                                        |
+-----------------------+                               |
| (W2) coin acquisition |                               |
+-----------------------+                               |
               |                                        |
~~~

where (for RSA, without age-restriction)


{::comment}

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(W1) history request (wallet)

msg = Gen-Msg(COIN_HISTORY_REQUEST, uint64(0x0))
sig = Ed25519-Sign(coin.priv, msg)
~~~

{::comment}

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(E1) refresh secret lookup (exchange)

refresh_record = Lookup by coin.pub
(ɣ, ⟨blind_sigᵢ⟩, _, done, link_info) = refresh_record
if done:
  melt_info = (ɣ, link_info, ⟨blind_sigᵢ⟩)
else:
  melt_info = (ɣ, link_info)
~~~

{::comment}

⟨ᵧₖᵢ⟩
{:/}

~~~ pseudocode
(W2) coin acquisition (wallet)

(ɣ, link_info, ⟨blind_sigᵢ⟩?) = melt_info
(refresh_seed, ⟨transferₖᵢ.pub⟩, ⟨h_denomᵢ⟩, coin_sig) = link_info

for i in 0..n:
  denomᵢ = Lookup by h_denomᵢ
for k in 0..κ:
  for i in 0..n:
    sharedₖᵢ = ECDH-Ed25519-Priv(coin.priv, transferₖᵢ.pub)
    (coinₖᵢ, blind_secretₖᵢ _, h_planchetₖᵢ) = Refresh-Derive(sharedₖᵢ, denomᵢ)
  h_planchetsₖ = SHA-512( ⟨h_planchetₖᵢ⟩ )
value = coin.denom.fee_refresh + Sum ⟨denomᵢ.value⟩ + Sum ⟨denomᵢ.fee_withdraw⟩
commitment = SHA-512( refresh_seed | uint256(0x0) | coin.pub
                    | value | ⟨h_planchetsₖ⟩ )
msg = Gen-Msg(WALLET_COIN_MELT,
    ( commitment | coin.h_denom | uint256(0x0)
    | value | denom.fee_refresh ))
Check Ed25519-Verify(coin.pub, msg, sig)

if ⟨blind_sigᵢ⟩ returned:
  for i in 0..n:
    coinᵧᵢ.sig = RSA-FDH-Unblind(blind_sigᵧᵢ, blind_secretᵧᵢ, denomᵢ.pub)
    Check RSA-FDH-Verify(SHA-512(coinᵧᵢ.pub), coinᵧᵢ.sig, denomᵢ.pub)
    coinᵧᵢ.h_denom = h_denomᵢ
  Persist ⟨coinᵧᵢ⟩
~~~

### Recoup {#refresh-recoup}

// todo

## Transfer of E-Cash {#w2w}

// todo: introductory text

Transactions in E-Cash between wallets.
Commonly referred to as peer-to-peer transactions.
In Taler, interaction with exchange, therefore called wallet-to-wallet transactions.

### Account Creation {#w2w-account}

### Push Payment {#w2w-push}

// todo

### Pull Payment {#w2w-pull}

// todo

# Security Considerations

\[ TBD \]

# IANA Considerations

None.

--- back

# Test Vectors

This appendix provides two sets of test vectors for testing Taler Protocol implementations.
They are generated by going through the protocol operations in the following order:

1. Withdraw two coins `coin₀` and `coin₁` from a single `reserve` (cf. {{withdraw}}).
2. Pay for one `order` with the full value of `coin₀` and a partial value of `coin₁` (cf. {{payment}}).
3. Obtain a partial refund on `coin₀` used to pay for the `order` (cf. {{refund}}).
4. Refresh the now-dirty `coin₁` to two new coins `coin₂` and `coin₃` (cf. {{refresh}}).
5. Regenerate `coin₂` and `coin₃` with the knowledge of `coin₁` (cf. {{link}}).
6. Create an `account` for w2w transfers (cf. {{w2w-account}}).
7. Send a payment to `account` with the full value of `coin₂`, obtaining `coin₄` (cf. {{w2w-push}}).
8. Request a payment to `account`, which is paid with the full value of `coin₄`, obtaining `coin₅` (cf. {{w2w-pull}}).
9. Recoup the value of `coin₅` obtained via withdrawal from `account` (cf. {{withdraw-recoup}}).
10. Recoup the value of `coin₃` obtained via refresh from `coin₁` (cf. {{refresh-recoup}}).

// todo: p2p sending full coins only works without fees, should we set fees to zero?

// todo: refund would be slightly more interesting with 2 coins being (partially) refunded,
should we change to full refund coin0 + partial refund coin1 (coin1 value after fee_deposit + fee_refund should then match denom2 + denom3)

The test vectors in this document have been generated by the GNU Taler reference implementation written in C.
All binary data is provided in hexadecimal notation.
Big numbers for RSA are represented in big-endian byte order (most significant byte first).

## Test Case 1

{::include ./test-vectors/test-case-1.md}

## Test Case 2

# Change log

# Acknowledgments
{:numbered="false"}

\[ TBD \]

This work was supported in part by the German Federal Ministry of
Education and Research (BMBF) within the project Concrete Contracts.
