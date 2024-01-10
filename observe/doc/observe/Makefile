all: doc

pdf/observe.pdf: ./observe.xml
	mkdir -p pdf
	dblatex -o pdf/observe.pdf observe.xml

odf/observe.odf: ./observe.xml
	mkdir -p odf
	docbook2odf observe.xml

html/observe.html: ./observe.xml
	mkdir -p html
	test -L html/images || ln -s ../images html
	xsltproc -o html/observe.html /usr/share/xml/docbook/stylesheet/docbook-xsl/xhtml/docbook.xsl observe.xml

doc: pdf/observe.pdf html/observe.html odf/observe.odf
