## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libapptools.la

libapptools_la_SOURCES = \
##	gimpairbrushtool.c	\
##	gimpairbrushtool.h	\
	gimpblendtool.c		\
	gimpblendtool.h		\
	gimpbucketfilltool.c	\
	gimpbucketfilltool.h	\
##	gimpbycolorselecttool.c	\
##	gimpbycolorselecttool.h	\
##	gimpclonetool.c		\
##	gimpclonetool.h		\
	gimpcolorpickertool.c	\
	gimpcolorpickertool.h	\
##	gimpconvolvetool.c	\
##	gimpconvolvetool.h	\
	gimpcroptool.c		\
	gimpcroptool.h		\
##	gimpdodgeburntool.c	\
##	gimpdodgeburntool.h	\
	gimpdrawtool.c		\
	gimpdrawtool.h		\
	gimpeditselectiontool.c	\
	gimpeditselectiontool.h	\
	gimpellipseselecttool.c	\
	gimpellipseselecttool.h	\
##	gimperasertool.c	\
##	gimperasertool.h	\
##	gimpfliptool.c		\
##	gimpfliptool.h		\
	gimpfreeselecttool.c	\
	gimpfreeselecttool.h	\
	gimpfuzzyselecttool.c	\
	gimpfuzzyselecttool.h	\
	gimpinktool.c		\
	gimpinktool.h		\
	gimpinktool-blob.c	\
	gimpinktool-blob.h	\
##	gimpiscissorstool.c	\
##	gimpiscissorstool.h	\
	gimpmagnifytool.c	\
	gimpmagnifytool.h	\
	gimpmeasuretool.c	\
	gimpmeasuretool.h	\
	gimpmovetool.c		\
	gimpmovetool.h		\
	gimppaintbrushtool.c	\
	gimppaintbrushtool.h	\
	gimppainttool.c		\
	gimppainttool.h		\
	gimppainttool_kernels.h	\
##	gimppenciltool.c	\
##	gimppenciltool.h	\
##	gimpperspectivetool.c	\
##	gimpperspectivetool.h	\
	gimprectselecttool.c	\
	gimprectselecttool.h	\
##	gimprotatetool.c	\
##	gimprotatetool.h	\
##	gimpscaletool.c		\
##	gimpscaletool.h		\
	gimpselectiontool.c	\
	gimpselectiontool.h	\
##	gimpsheartool.c		\
##	gimpsheartool.h		\
##	gimpsmudgetool.c	\
##	gimpsmudgetool.h	\
	gimptexttool.c		\
	gimptexttool.h		\
	gimptool.c		\
	gimptool.h		\
	gimptoolinfo.c		\
	gimptoolinfo.h		\
	gimptransformtool.c	\
	gimptransformtool.h	\
	\
	paint_options.c		\
	paint_options.h		\
	selection_options.c	\
	selection_options.h	\
	tool_options.c		\
	tool_options.h		\
	tool_options_dialog.c	\
	tool_options_dialog.h	\
	\
	tool_manager.c		\
	tool_manager.h		\
	tools.c

##	bezier_select.c		\
##	bezier_select.h		\
##	bezier_selectP.h	\
##	brightness_contrast.c	\
##	brightness_contrast.h	\
##	color_balance.c		\
##	color_balance.h		\
##	curves.c		\
##	curves.h		\
##	histogram_tool.c	\
##	histogram_tool.h	\
##	hue_saturation.c	\
##	hue_saturation.h	\
##	levels.c		\
##	levels.h		\
##	posterize.c		\
##	posterize.h		\
##	threshold.c		\
##	threshold.h		\
##	transform_core.c	\
##	transform_core.h	\
##	transform_tool.c	\
##	transform_tool.h


AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Tools\"

INCLUDES = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
