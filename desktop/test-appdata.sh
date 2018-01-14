#!/bin/sh

appstream-util validate-strict ${GIMP_TESTING_ABS_TOP_SRCDIR}/desktop/org.gimp.GIMP.appdata.xml.in.in
appstream-util validate-strict ${GIMP_TESTING_ABS_TOP_SRCDIR}/desktop/gimp-data-extras.metainfo.xml.in
