SUBDIRS=Expat
INCLUDES = -I$(top_srcdir)/include -I.
AM_CFLAGS   = @COMMON_CFLAGS@ 

noinst_LTLIBRARIES = libXmlLite.la

libXmlLite_la_SOURCES  = XmlDoc.c XmlElt.c XmlRpc.c XmlRpc_impl.c XmlRpc_util.c
libXmlLite_la_LIBADD   = Expat/libexpat.la