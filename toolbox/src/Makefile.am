SUBDIRS     = Objects Scalars Containers Regex Composites Utils XmlLite Memory
INCLUDES    = -I. -I$(top_srcdir)/include
AM_CFLAGS   = @COMMON_CFLAGS@



lib_LTLIBRARIES = libtbx.la 
libtbx_la_SOURCES  = build.h .tstamp   \
	version.c    
libtbx_la_LIBADD   =     \
	Objects/libObjects.la       \
	Scalars/libScalars.la       \
	Containers/libContainers.la \
	Regex/libRegex.la           \
	XmlLite/libXmlLite.la       \
	Composites/libComposites.la \
	Memory/libMemory.la         \
	Utils/libUtils.la
libtbx_la_LDFLAGS  = @ADD_LDFLAGS@ -version-info 5:0:0


test:
	cd tests;make check


version.c: build.h
	echo updated build version

build.h: .tstamp
	tstamp=`cat .tstamp`; echo "#define TB_BUILD \"$$tstamp\"" >build.h && rm .tstamp

.tstamp:
	echo "$$USER@`hostname`"-"`date +"%Y%m%d-%T"`" |sed -e "s/\n//g">.tstamp
