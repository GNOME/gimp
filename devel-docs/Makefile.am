## Process this file with automake to produce Makefile.in

SUBDIRS = \
	libgimpbase	\
	libgimpcolor	\
	libgimpmath	\
	libgimpmodule	\
	libgimpthumb	\
	libgimpwidgets	\
	libgimp

EXTRA_DIST = \
	ChangeLog		\
	README			\
	README.gtkdoc		\
	app			\
	debug-plug-ins.txt	\
	gbr.txt			\
	ggr.txt  		\
	gih.txt  		\
	gpb.txt  		\
	includes.txt		\
	parasites.txt		\
	pat.txt			\
	structure.xml		\
	undo.txt		\
	xcf.txt


# require gtk-doc when making dist
#
if ENABLE_GTK_DOC
dist-check-gtk-doc:
else
dist-check-gtk-doc:
	@echo "*** gtk-doc must be enabled in order to make dist"
	@false
endif

dist-hook: dist-check-gtk-doc
