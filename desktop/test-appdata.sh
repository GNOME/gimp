#!/bin/sh

# TODO: use validate-strict when the last errors for a strict validation
# are fixed.
# appstream-util validate-relax ${GIMP_TESTING_ABS_TOP_BUILDDIR}/desktop/org.gimp.GIMP.appdata.xml && \
appstream-util validate-relax ${GIMP_TESTING_ABS_TOP_BUILDDIR}/desktop/gimp-data-extras.metainfo.xml
