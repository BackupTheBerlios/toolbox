INCLUDES = -I. -I$(top_srcdir)/include 
AM_CFLAGS   = @COMMON_CFLAGS@

bin_PROGRAMS  = \
	rpcbdg           \
	string_test \
	dict_test   \
	hash_test   \
	vector_test \
	socket_test \
	lock_test   \
	xml_test    \
	ssl_srv     \
	ssl_cli     \
	srv_test    \
	xmlrpc_client    \
	xmlrpc_server    \
	rpc_server       \
	rpc_client       \
	date_test    \
	marshall

TESTS           = $(bin_PROGRAMS)
 
rpc_server_SOURCES       = rpc_server.c
rpc_server_LDADD         = ../libtbx.la
rpc_server_LDFLAGS       = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

rpc_client_SOURCES       = rpc_client.c
rpc_client_LDADD         = ../libtbx.la
rpc_client_LDFLAGS       = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

rpcbdg_SOURCES       = rpcbridge.c
rpcbdg_LDADD         = ../libtbx.la
rpcbdg_LDFLAGS       = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

xmlrpc_client_SOURCES       = xmlrpc_client.c
xmlrpc_client_LDADD         = ../libtbx.la
xmlrpc_client_LDFLAGS       = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

xmlrpc_server_SOURCES       = xmlrpc_server.c
xmlrpc_server_LDADD         = ../libtbx.la
xmlrpc_server_LDFLAGS       = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

date_test_SOURCES                = date_test.c
date_test_LDADD                  = ../libtbx.la
date_test_LDFLAGS                = @ADD_LDFLAGS@ @ADD_BOOTSTRAP@ @ADD_LIBS@

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


 
