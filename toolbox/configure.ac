AC_INIT
AC_CONFIG_SRCDIR([src/Objects/Objects.c])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE(toolbox, 5.0)
AM_CONFIG_HEADER(config.h)
AM_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_CHECK_LIB(pthread, pthread_create)

MEMDEBUG=false
MEM_DEBUG=""
DEBUG_MODE=""
COMMON_CFLAGS="-O2 -Wall -g -D_THREAD_SAFE -D_REENTRANT"
MY_OS=`uname |tr [a-z] [A-Z]`
ADD_LIBS=	
ADD_LDFLAGS=
ADD_BOOTSTRAP=

AC_ARG_ENABLE(debug,
     [  --enable-debug	debug build],
     [case "${enableval}" in
       yes) debug=true  
				COMMON_CFLAGS="$COMMON_CFLAGS -g -DDEBUG" 	
			 ;;
       no)  debug=false 
				COMMON_CFLAGS="$COMMON_CFLAGS -DRELEASE" 	
			 ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug); ;;
     esac],[debug=false])
  	 AM_CONDITIONAL(DEBUG, test x$debug = xtrue) 

AC_ARG_ENABLE(memdebug,
		[  --enable-memdebug	build-in mem logger (require --enable-debug)],
    	[case "${enableval}" in
				yes) memdebug=true ; 
					printf "turning mem checking on\n";
					COMMON_CFLAGS="$COMMON_CFLAGS -DTB_MEM_DEBUG -D__BUILD"; 
			 		;;
       	no)  memdebug=false; printf "turning mem checking off\n";;
      	*) AC_MSG_ERROR(bad value ${enableval} for --enable-memdebug); printf "(off)\n";;
     	esac],[memdebug=false])
  	AM_CONDITIONAL(MEM_DEBUG, test x$memdebug = xtrue) 



printf "enabling ssl... "
AC_ARG_ENABLE(ssl,
     [  --enable-ssl		build SSL support builtin],
     [case "${enableval}" in
       yes) ssl=true
				ADD_LDFLAGS="$ADD_LDFLAGS -lssl -lcrypto"
				COMMON_CFLAGS="$COMMON_CFLAGS -DWITH_SSL"; 	
				;;
       no)  ssl=false ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-ssl) ;;
     esac],[ssl=false])
     AM_CONDITIONAL(SSL, test x$ssl = xtrue) 
printf "${ssl}\n"




AC_ARG_WITH(ssl-include, 
	[  --with-ssl-include=DIR         where to find ssl includes],
	[ SSL_INCLUDE="-I${withval}" ]
	) 
AC_ARG_WITH(ssl-lib, 
	[  --with-ssl-lib=DIR         where to find ssl libs],
	[ SSL_LIB="-L${withval}" ]
	) 


case $MY_OS in
	AIX) 
		ADD_LIBS=" -lgcc -lc_r"
		ADD_LDFLAGS=" -nostartfiles"
		ADD_BOOTSTRAP=" /usr/lib/crt0_r.o"
		COMMON_CFLAGS="$COMMON_CFLAGS -D_LONG_LONG -D_ALL_SOURCE"
	;;
	LINUX) 
		COMMON_CFLAGS="$COMMON_CFLAGS -D_GNU_SOURCE -rdynamic"
	;;
	SUNOS) 
	;;
	*) ;;
esac

printf "COMMON_CFLAGS=%s\n", $COMMON_CFLAGS
printf "MYSQL_INCLUDE=%s\n", $MYSQL_INCLUDE


AC_SUBST(MY_OS)
AC_SUBST(MEM_DEBUG)
AC_SUBST(ADD_LIBS)
AC_SUBST(ADD_LDFLAGS)
AC_SUBST(ADD_BOOTSTRAP)
AC_SUBST(SSL_INCLUDE)
AC_SUBST(SSL_LIB)

AC_SUBST(COMMON_CFLAGS)
AC_CONFIG_FILES([\
	include/Makefile               \
	include/tb_global.h            \
	src/version.h                  \
	src/Makefile                   \
	src/Objects/Makefile           \
	src/Scalars/Makefile           \
	src/Containers/Makefile        \
	src/Regex/Makefile             \
	src/Composites/Makefile        \
	src/Memory/Makefile            \
	src/XmlLite/Makefile           \
	src/XmlLite/Expat/Makefile     \
	src/Utils/Makefile             \
	src/tests/Makefile             \
	Makefile])
AC_OUTPUT	






