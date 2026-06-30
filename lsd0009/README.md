# The GNU Taler Protocol

This is the working area for the [Living Standards Document](https://www.gnunet.org/en/livingstandards.html) 0009,
"The GNU Taler Protocol".

* [Source repository](https://git.taler.net/lsd0009.git/)
* [Rendered version](https://lsd.gnunet.org/lsd0009/)

## Tooling

Text, HTML and the intermediate XML version of the draft are built from the [markdown file](./draft-guetschow-taler-protocol.md) using `make`.

```sh
$ make
```

This requires that you have the necessary software installed:
- GNU make
- [kramdown-rfc](https://github.com/cabo/kramdown-rfc)
- [xml2rfc](https://github.com/ietf-tools/xml2rfc) version >= 3.32.0

To alleviate the CI from the `kramdown-rfc` dependency, the [generated xml file](./draft-guetschow-taler-protocol.xml)
needs to be checked in to git before pushing.
