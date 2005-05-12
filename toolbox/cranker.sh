
version=`ls version* |sed -e s/.*version-//`
if ! test -e revision;  then  
		echo "0" >revision
		rev=0
else 
		rev=`expr $(cat revision) + 1`
fi


RELEASE="$version"
VERSION=$RELEASE."$rev"

NAME=`echo $version |sed -e s/-.*//`
MAJOR=`echo $version|sed -e s/.*-//`
MAJOR=`echo $MAJOR|sed -e s/[.].*//`
MINOR=`echo $version|sed -e s/.*-//`
MINOR=`echo $version|sed -e s/.*[.]//`

TAG="BUILD-"$NAME"-"$MAJOR"-"$MINOR"-"$rev

h=$NAME"_revision.h"
c="revision.c"

DATE=`date`;

echo "#ifndef _"$NAME"_REVISION_" > $h
echo "#define _"$NAME"_REVISION_" >>$h
echo "#define "$NAME"_MAJOR \"$MAJOR\""  >>$h
echo "#define "$NAME"_MINOR \"$MINOR\""  >>$h
echo "#define "$NAME"_REVISION \"$rev\""  >>$h
echo "#define "$NAME"_VERSION_RELEASE \"$RELEASE\""  >>$h
echo "#define "$NAME"_BUILD_REVISION \"$TAG\"" >>$h
echo "#define "$NAME"_BUILD_DATE \"$DATE\"" >>$h
echo "char * "$NAME"_getVersion();" >>$h
echo "char * "$NAME"_getBuild();" >>$h
echo "char * "$NAME"_getBuildDate();" >>$h
echo "#endif" >>$h


echo "#include \""$NAME"_revision.h\"" >$c
echo "char * _"$NAME"_VERSION    = "$NAME"_VERSION_RELEASE;" >>$c
echo "char * _"$NAME"_BUILD      = "$NAME"_VERSION_RELEASE\" r\""$NAME"_REVISION;" >>$c
echo "char * _"$NAME"_BUILD_DATE = "$NAME"_BUILD_DATE;" >>$c

echo "char *"$NAME"_getVersion()   { return _"$NAME"_VERSION; }">>$c
echo "char *"$NAME"_getBuild()     { return _"$NAME"_BUILD; }">>$c
echo "char *"$NAME"_getBuildDate() { return _"$NAME"_BUILD_DATE; }">>$c

echo $rev >revision

echo cranked $RELEASE to rev $rev