## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libapptools.a

libapptools_a_SOURCES = \
	tools.c				\
	tools.h				\
	tools-types.h			\
	gimpairbrushtool.c		\
	gimpairbrushtool.h		\
	gimpbezierselecttool.c  	\
	gimpbezierselecttool.h  	\
	gimpblendtool.c			\
	gimpblendtool.h			\
	gimpbrightnesscontrasttool.c	\
	gimpbrightnesscontrasttool.h	\
	gimpbucketfilltool.c		\
	gimpbucketfilltool.h		\
	gimpbycolorselecttool.c		\
	gimpbycolorselecttool.h		\
	gimpclonetool.c			\
	gimpclonetool.h			\
	gimpcolorbalancetool.c		\
	gimpcolorbalancetool.h		\
	gimpcolorpickertool.c		\
	gimpcolorpickertool.h		\
	gimpconvolvetool.c		\
	gimpconvolvetool.h		\
	gimpcroptool.c			\
	gimpcroptool.h			\
	gimpcurvestool.c		\
	gimpcurvestool.h		\
	gimpdodgeburntool.c		\
	gimpdodgeburntool.h		\
	gimpdrawtool.c			\
	gimpdrawtool.h			\
	gimpeditselectiontool.c		\
	gimpeditselectiontool.h		\
	gimpellipseselecttool.c		\
	gimpellipseselecttool.h		\
	gimperasertool.c		\
	gimperasertool.h		\
	gimpfliptool.c			\
	gimpfliptool.h			\
	gimpfreeselecttool.c		\
	gimpfreeselecttool.h		\
	gimpfuzzyselecttool.c		\
	gimpfuzzyselecttool.h		\
	gimphistogramtool.c		\
	gimphistogramtool.h		\
	gimphuesaturationtool.c		\
	gimphuesaturationtool.h		\
	gimpimagemaptool.c		\
	gimpimagemaptool.h		\
	gimpinktool.c			\
	gimpinktool.h			\
	gimpinktool-blob.c		\
	gimpinktool-blob.h		\
	gimpiscissorstool.c		\
	gimpiscissorstool.h		\
	gimplevelstool.c		\
	gimplevelstool.h		\
	gimpmagnifytool.c		\
	gimpmagnifytool.h		\
	gimpmeasuretool.c		\
	gimpmeasuretool.h		\
	gimpmovetool.c			\
	gimpmovetool.h			\
	gimppaintbrushtool.c		\
	gimppaintbrushtool.h		\
	gimppainttool.c			\
	gimppainttool.h			\
	gimppainttool_kernels.h		\
	gimppathtool.c			\
	gimppathtool.h			\
	gimppenciltool.c		\
	gimppenciltool.h		\
	gimpperspectivetool.c		\
	gimpperspectivetool.h		\
	gimpposterizetool.c		\
	gimpposterizetool.h		\
	gimprectselecttool.c		\
	gimprectselecttool.h		\
	gimprotatetool.c		\
	gimprotatetool.h		\
	gimpscaletool.c			\
	gimpscaletool.h			\
	gimpselectiontool.c		\
	gimpselectiontool.h		\
	gimpsheartool.c			\
	gimpsheartool.h			\
	gimpsmudgetool.c		\
	gimpsmudgetool.h		\
	gimptexttool.c			\
	gimptexttool.h			\
	gimpthresholdtool.c		\
	gimpthresholdtool.h		\
	gimptool.c			\
	gimptool.h			\
	gimptransformtool.c		\
	gimptransformtool.h		\
	\
	transform_options.c     	\
	transform_options.h     	\
	paint_options.c			\
	paint_options.h			\
	selection_options.c		\
	selection_options.h		\
	tool_options.c			\
	tool_options.h			\
	\
	icons.h				\
	tool_manager.c			\
	tool_manager.h			\
	path_tool.h			\
	path_tool.c			\
	path_toolP.h

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
