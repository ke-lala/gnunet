all: txt html

draft-guetschow-taler-protocol.xml: draft-guetschow-taler-protocol.md $(wildcard test-vectors/*)
	kdrfc -c draft-guetschow-taler-protocol.md
	mv draft-guetschow-taler-protocol.v2v3.xml  draft-guetschow-taler-protocol.xml

html: draft-guetschow-taler-protocol.xml
	xml2rfc --html draft-guetschow-taler-protocol.xml

txt: draft-guetschow-taler-protocol.xml
	xml2rfc draft-guetschow-taler-protocol.xml