INCLUDES = -I$(top_srcdir)/include -I.
AM_CFLAGS   = @COMMON_CFLAGS@ -DXML_DTD -DXML_NS

XML_TOK_SRC     = xmltok.c xmlrole.c 
XML_PARSE_SRC   = xmlparse.c 

noinst_LTLIBRARIES = libexpat.la


libexpat_la_SOURCES  = $(XML_TOK_SRC) $(XML_PARSE_SRC)


