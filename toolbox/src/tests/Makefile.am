INCLUDES = -I. -I$(top_srcdir)/include 
AM_LDFLAGS  = -L$(top_srcdir)/src/.libs -L/usr/local/lib
AM_CFLAGS   = @COMMON_CFLAGS@

bin_PROGRAMS  = \
	string_test \
	dict_test   \
	hash_test   \
	vector_test \
	socket_test \
	socket_acl_test \
	lock_test   \
	xml_test    \
	ssl_srv     \
	ssl_cli     \
	ssl_cli2     \
	srv_test    \
	wpool_test  \
	marshall

TESTS           = $(bin_PROGRAMS)
 
marshall_SOURCES       = marshall.c
marshall_LDADD         = ../libtbx.la
marshall_LDFLAGS       = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

string_test_SOURCES    = string_test.c
string_test_LDADD      = ../libtbx.la 
string_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

vector_test_SOURCES    = vector_test.c
vector_test_LDADD      = ../libtbx.la
vector_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

hash_test_SOURCES      = hash_test.c
hash_test_LDADD        = ../libtbx.la    
hash_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

dict_test_SOURCES    = dict_test.c
dict_test_LDADD      = ../libtbx.la 
dict_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

xml_test_SOURCES       = xml_test.c
xml_test_LDADD         = ../libtbx.la    
xml_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

socket_test_SOURCES    = socket_test.c
socket_test_LDADD      = ../libtbx.la    
socket_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

socket_acl_test_SOURCES    = socket_acl_test.c
socket_acl_test_LDADD      = ../libtbx.la    
socket_acl_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

lock_test_SOURCES    = lock_test.c
lock_test_LDADD      = ../libtbx.la
lock_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

ssl_srv_SOURCES    = ssl_srv.c
ssl_srv_LDADD      = ../libtbx.la -lssl -lcrypto
ssl_srv_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

ssl_cli_SOURCES    = ssl_cli.c
ssl_cli_LDADD      = ../libtbx.la -lssl -lcrypto
ssl_cli_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

srv_test_SOURCES    = srv_test.c
srv_test_LDADD      = ../libtbx.la
srv_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

wpool_test_SOURCES    = wpool_test.c
wpool_test_LDADD      = ../libtbx.la
wpool_test_LDFLAGS    = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

 
