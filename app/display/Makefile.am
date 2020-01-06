## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Display\"	\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(xnone)

noinst_LIBRARIES = libappdisplay.a

libappdisplay_a_sources = \
	display-enums.h				\
	display-types.h				\
	gimpcanvas.c				\
	gimpcanvas.h				\
	gimpcanvas-style.c			\
	gimpcanvas-style.h			\
	gimpcanvasarc.c				\
	gimpcanvasarc.h				\
	gimpcanvasboundary.c			\
	gimpcanvasboundary.h			\
	gimpcanvasbufferpreview.c		\
	gimpcanvasbufferpreview.h		\
	gimpcanvascanvasboundary.c		\
	gimpcanvascanvasboundary.h		\
	gimpcanvascorner.c			\
	gimpcanvascorner.h			\
	gimpcanvascursor.c			\
	gimpcanvascursor.h			\
	gimpcanvasgrid.c			\
	gimpcanvasgrid.h			\
	gimpcanvasgroup.c			\
	gimpcanvasgroup.h			\
	gimpcanvasguide.c			\
	gimpcanvasguide.h			\
	gimpcanvashandle.c			\
	gimpcanvashandle.h			\
	gimpcanvasitem.c			\
	gimpcanvasitem.h			\
	gimpcanvasitem-utils.c			\
	gimpcanvasitem-utils.h			\
	gimpcanvaslayerboundary.c		\
	gimpcanvaslayerboundary.h		\
	gimpcanvasline.c			\
	gimpcanvasline.h			\
	gimpcanvaspassepartout.c		\
	gimpcanvaspassepartout.h		\
	gimpcanvaspath.c			\
	gimpcanvaspath.h			\
	gimpcanvaspen.c				\
	gimpcanvaspen.h				\
	gimpcanvaspolygon.c			\
	gimpcanvaspolygon.h			\
	gimpcanvasprogress.c			\
	gimpcanvasprogress.h			\
	gimpcanvasproxygroup.c			\
	gimpcanvasproxygroup.h			\
	gimpcanvasrectangle.c			\
	gimpcanvasrectangle.h			\
	gimpcanvasrectangleguides.c		\
	gimpcanvasrectangleguides.h		\
	gimpcanvassamplepoint.c			\
	gimpcanvassamplepoint.h			\
	gimpcanvastextcursor.c			\
	gimpcanvastextcursor.h			\
	gimpcanvastransformguides.c		\
	gimpcanvastransformguides.h		\
	gimpcanvastransformpreview.c		\
	gimpcanvastransformpreview.h		\
	gimpcursorview.c			\
	gimpcursorview.h			\
	gimpdisplay.c				\
	gimpdisplay.h				\
	gimpdisplay-foreach.c			\
	gimpdisplay-foreach.h			\
	gimpdisplay-handlers.c			\
	gimpdisplay-handlers.h			\
	gimpdisplayshell.c			\
	gimpdisplayshell.h			\
	gimpdisplayshell-actions.c		\
	gimpdisplayshell-actions.h		\
	gimpdisplayshell-appearance.c		\
	gimpdisplayshell-appearance.h		\
	gimpdisplayshell-autoscroll.c		\
	gimpdisplayshell-autoscroll.h		\
	gimpdisplayshell-callbacks.c		\
	gimpdisplayshell-callbacks.h		\
	gimpdisplayshell-close.c		\
	gimpdisplayshell-close.h		\
	gimpdisplayshell-cursor.c		\
	gimpdisplayshell-cursor.h		\
	gimpdisplayshell-dnd.c			\
	gimpdisplayshell-dnd.h			\
	gimpdisplayshell-draw.c			\
	gimpdisplayshell-draw.h			\
	gimpdisplayshell-expose.c		\
	gimpdisplayshell-expose.h		\
	gimpdisplayshell-grab.c			\
	gimpdisplayshell-grab.h			\
	gimpdisplayshell-handlers.c		\
	gimpdisplayshell-handlers.h		\
	gimpdisplayshell-filter.c		\
	gimpdisplayshell-filter.h		\
	gimpdisplayshell-filter-dialog.c	\
	gimpdisplayshell-filter-dialog.h	\
	gimpdisplayshell-layer-select.c		\
	gimpdisplayshell-layer-select.h		\
	gimpdisplayshell-icon.c			\
	gimpdisplayshell-icon.h			\
	gimpdisplayshell-items.c		\
	gimpdisplayshell-items.h		\
	gimpdisplayshell-profile.c		\
	gimpdisplayshell-profile.h		\
	gimpdisplayshell-progress.c		\
	gimpdisplayshell-progress.h		\
	gimpdisplayshell-render.c		\
	gimpdisplayshell-render.h		\
	gimpdisplayshell-rotate.c		\
	gimpdisplayshell-rotate.h		\
	gimpdisplayshell-rotate-dialog.c	\
	gimpdisplayshell-rotate-dialog.h	\
	gimpdisplayshell-rulers.c		\
	gimpdisplayshell-rulers.h		\
	gimpdisplayshell-scale.c		\
	gimpdisplayshell-scale.h		\
	gimpdisplayshell-scale-dialog.c		\
	gimpdisplayshell-scale-dialog.h		\
	gimpdisplayshell-scroll.c		\
	gimpdisplayshell-scroll.h		\
	gimpdisplayshell-scrollbars.c		\
	gimpdisplayshell-scrollbars.h		\
	gimpdisplayshell-selection.c		\
	gimpdisplayshell-selection.h		\
	gimpdisplayshell-title.c		\
	gimpdisplayshell-title.h		\
	gimpdisplayshell-tool-events.c		\
	gimpdisplayshell-tool-events.h		\
	gimpdisplayshell-transform.c		\
	gimpdisplayshell-transform.h		\
	gimpdisplayshell-utils.c		\
	gimpdisplayshell-utils.h		\
	gimpdisplayxfer.c			\
	gimpdisplayxfer.h			\
	gimpimagewindow.c			\
	gimpimagewindow.h			\
	gimpmotionbuffer.c			\
	gimpmotionbuffer.h			\
	gimpmultiwindowstrategy.c		\
	gimpmultiwindowstrategy.h		\
	gimpnavigationeditor.c			\
	gimpnavigationeditor.h			\
	gimpscalecombobox.c			\
	gimpscalecombobox.h			\
	gimpsinglewindowstrategy.c		\
	gimpsinglewindowstrategy.h		\
	gimpstatusbar.c				\
	gimpstatusbar.h				\
	gimptooldialog.c			\
	gimptooldialog.h			\
	gimptoolgui.c				\
	gimptoolgui.h				\
	gimptoolcompass.c			\
	gimptoolcompass.h			\
	gimptoolgyroscope.c			\
	gimptoolgyroscope.h			\
	gimptoolhandlegrid.c			\
	gimptoolhandlegrid.h			\
	gimptoolline.c				\
	gimptoolline.h				\
	gimptoolpath.c				\
	gimptoolpath.h				\
	gimptoolpolygon.c			\
	gimptoolpolygon.h			\
	gimptoolrectangle.c			\
	gimptoolrectangle.h			\
	gimptoolrotategrid.c			\
	gimptoolrotategrid.h			\
	gimptoolsheargrid.c			\
	gimptoolsheargrid.h			\
	gimptooltransform3dgrid.c		\
	gimptooltransform3dgrid.h		\
	gimptooltransformgrid.c			\
	gimptooltransformgrid.h			\
	gimptoolwidget.c			\
	gimptoolwidget.h			\
	gimptoolwidgetgroup.c			\
	gimptoolwidgetgroup.h

libappdisplay_a_built_sources = display-enums.c

libappdisplay_a_SOURCES = \
	$(libappdisplay_a_built_sources)	\
	$(libappdisplay_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-dec
CLEANFILES = $(gen_sources)

xgen-dec: $(srcdir)/display-enums.h $(GIMP_MKENUMS) Makefile.am
	$(AM_V_GEN) $(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gio/gio.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"display-enums.h\"\n#include\"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/display-enums.c: xgen-dec
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
