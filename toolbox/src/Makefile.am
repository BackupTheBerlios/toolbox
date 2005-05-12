SUBDIRS     = Objects Scalars Containers Regex Composites Utils XmlLite Memory
INCLUDES    = -I. -I$(top_srcdir)/include
AM_CFLAGS   = @COMMON_CFLAGS@


LIBS_DEPEND = Objects/libObjects.la \
	Scalars/libScalars.la       \
	Containers/libContainers.la \
	Regex/libRegex.la           \
	XmlLite/libXmlLite.la       \
	Composites/libComposites.la \
	Memory/libMemory.la         \
	Utils/libUtils.la


lib_LTLIBRARIES    = libtbx.la 
libtbx_la_SOURCES  = revision.c housekeeper.c 
libtbx_la_LIBADD   = $(LIBS_DEPEND)
libtbx_la_LDFLAGS  = @ADD_LDFLAGS@ -version-info 5:0:0


revision.c: housekeeper.c $(LIBS_DEPEND)
	$(top_srcdir)/cranker.sh


test:
	cd tests;make check

