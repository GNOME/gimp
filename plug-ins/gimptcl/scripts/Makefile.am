## Process this file with automake to produce Makefile.in

pluginlibdir = $(gimpplugindir)/plug-ins

pluginlib_SCRIPTS = consolio pdb_help stained_glass

SUFFIXES = .tcl

.tcl:
	sed s,@THEGIMPTCL@,${libexecdir}/gimptcl, $< > $@
	chmod +x $@
