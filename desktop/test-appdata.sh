#!/bin/sh

cd $GIMP_TESTING_BUILDDIR
if [ $GIMP_RELEASE -eq 1 ]; then
  appstreamcli validate org.gimp.GIMP.appdata.xml
  exit $?
else
  APPDATA=`mktemp org.gimp.GIMP.appdata.XXX.xml`
  sed "s/date=\"TODO\"/date=\"`date --iso-8601`\"/" org.gimp.GIMP.appdata.xml > $APPDATA
  appstreamcli validate $APPDATA
  success=$?
  rm $APPDATA
  exit $success
fi
