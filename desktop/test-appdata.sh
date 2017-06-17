#!/bin/sh

appstream-util validate-strict ${GIMP_TESTING_ABS_TOP_SRCDIR}/desktop/gimp.appdata.xml.in
appstream-util validate-strict ${GIMP_TESTING_ABS_TOP_SRCDIR}/desktop/gimp-data-extras.metainfo.xml.in
