## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_SCRIPTS = consolio pdb_help stained_glass sbutton

SUFFIXES = .tcl

CLEANFILES = ${pluginlib_SCRIPTS}

.tcl:
	sed s,@THEGIMPTCL@,${libexecdir}/gimptcl, $< > $@
	chmod +x $@
