# Makefile.am for gimp/m4macros

installed_m4 = gimp-3.0.m4

uninstalled_m4 = \
	alsa.m4				\
	ax_compare_version.m4		\
	ax_cxx_compile_stdcxx.m4	\
	ax_gcc_func_attribute.m4	\
	ax_prog_cc_for_build.m4		\
	ax_prog_perl_version.m4		\
	detectcflags.m4			\
	introspection.m4		\
	python3dev.m4		\
	vapigen.m4

EXTRA_DIST = $(installed_m4) $(uninstalled_m4)

m4datadir = $(datadir)/aclocal
m4data_DATA = $(installed_m4)
