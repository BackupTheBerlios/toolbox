SUBDIRS    = src include

doc: doxy.conf
	rm -rf doc/html &&	doxygen doxy.conf

dist-hook:
	cp doxy.conf $(distdir)
	mkdir $(distdir)/config
	cp -r $(srcdir)/config/* $(distdir)/config
	mkdir $(distdir)/include
	cp -r $(srcdir)/include/* $(distdir)/include
	mkdir $(distdir)/doc
	cp -r $(srcdir)/doc/* $(distdir)/doc
	mkdir $(distdir)/src/tests
	cp -r $(srcdir)/src/tests/* $(distdir)/src/tests
	cp -r $(srcdir)/modules/tb_www/tests  $(distdir)/modules/tb_www
	cp -lur $(srcdir)/src/XmlLite/Expat/* $(distdir)/src/XmlLite/Expat/



