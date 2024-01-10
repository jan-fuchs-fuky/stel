all: doc

pdf/observe_irc.pdf: ./observe_irc.xml
	mkdir -p pdf
	dblatex -o pdf/observe_irc.pdf observe_irc.xml

odt/observe_irc.odt: ./observe_irc.xml
	mkdir -p odt
	docbook2odf observe_irc.xml
	mv observe_irc.odt ./odt

html/observe_irc.html: ./observe_irc.xml
	mkdir -p html
	test -L html/images || ln -s ../images html
	xsltproc -o html/observe_irc.html /usr/share/xml/docbook/stylesheet/docbook-xsl/xhtml/docbook.xsl observe_irc.xml

doc: pdf/observe_irc.pdf html/observe_irc.html odt/observe_irc.odt
