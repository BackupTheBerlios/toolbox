SUBDIRS = Ipc
INCLUDES = -I. -I$(top_srcdir)/include @SSL_INCLUDE@
AM_CFLAGS   = @COMMON_CFLAGS@

noinst_LTLIBRARIES = libComposites.la

libComposites_la_SOURCES  = \
	Composites.c \
	Iterators.c  \
	WPool.c      \
	Socket.c     \
	Server.c     \
	Sock_ssl.c   \
	Sock_ACL.c   \
	SockIO.c     
libComposites_la_LIBADD   = Ipc/libIpc.la
libComposites_la_LDFLAGS  = 




