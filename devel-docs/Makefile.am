## Process this file with automake to produce Makefile.in

if ENABLE_GTK_DOC_APP
app = app
endif

SUBDIRS = \
	tools		\
	libgimpbase	\
	libgimpcolor	\
	libgimpconfig	\
	libgimpmath	\
	libgimpmodule	\
	libgimpthumb	\
	libgimpwidgets	\
	libgimp		\
	$(app)

EXTRA_DIST = \
	ChangeLog			\
	README				\
	README.gtkdoc			\
	bugzilla-milestones.txt		\
	contexts.txt			\
	debug-plug-ins.txt		\
	exif-handling.txt		\
	gbr.txt				\
	ggr.txt  			\
	gih.txt  			\
	gpb.txt  			\
	includes.txt			\
	parasites.txt			\
	pat.txt				\
	gtkbuilder-porting-guide.txt	\
	release-howto.txt		\
	structure.xml			\
	submitting-patches.txt		\
	tagging.txt			\
	ui-framework.txt		\
	undo.txt			\
	vbr.txt				\
	xcf.txt


# require gtk-doc when making dist
#
if ENABLE_GTK_DOC
dist-check-gtk-doc:
if ENABLE_GTK_DOC_APP
	@echo "*** gtk-doc-app must be disabled in order to make dist"
	@false
endif
else
dist-check-gtk-doc:
	@echo "*** gtk-doc must be enabled in order to make dist"
	@false
endif

dist-hook: dist-check-gtk-doc
