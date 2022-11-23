## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Ligma-Display\"	\
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app		\
	-I$(top_srcdir)/app		\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

noinst_LIBRARIES = libappdisplay.a

libappdisplay_a_sources = \
	display-enums.h				\
	display-types.h				\
	ligmacanvas.c				\
	ligmacanvas.h				\
	ligmacanvas-style.c			\
	ligmacanvas-style.h			\
	ligmacanvasarc.c				\
	ligmacanvasarc.h				\
	ligmacanvasboundary.c			\
	ligmacanvasboundary.h			\
	ligmacanvasbufferpreview.c		\
	ligmacanvasbufferpreview.h		\
	ligmacanvascanvasboundary.c		\
	ligmacanvascanvasboundary.h		\
	ligmacanvascorner.c			\
	ligmacanvascorner.h			\
	ligmacanvascursor.c			\
	ligmacanvascursor.h			\
	ligmacanvasgrid.c			\
	ligmacanvasgrid.h			\
	ligmacanvasgroup.c			\
	ligmacanvasgroup.h			\
	ligmacanvasguide.c			\
	ligmacanvasguide.h			\
	ligmacanvashandle.c			\
	ligmacanvashandle.h			\
	ligmacanvasitem.c			\
	ligmacanvasitem.h			\
	ligmacanvasitem-utils.c			\
	ligmacanvasitem-utils.h			\
	ligmacanvaslayerboundary.c		\
	ligmacanvaslayerboundary.h		\
	ligmacanvaslimit.c			\
	ligmacanvaslimit.h			\
	ligmacanvasline.c			\
	ligmacanvasline.h			\
	ligmacanvaspassepartout.c		\
	ligmacanvaspassepartout.h		\
	ligmacanvaspath.c			\
	ligmacanvaspath.h			\
	ligmacanvaspen.c				\
	ligmacanvaspen.h				\
	ligmacanvaspolygon.c			\
	ligmacanvaspolygon.h			\
	ligmacanvasprogress.c			\
	ligmacanvasprogress.h			\
	ligmacanvasproxygroup.c			\
	ligmacanvasproxygroup.h			\
	ligmacanvasrectangle.c			\
	ligmacanvasrectangle.h			\
	ligmacanvasrectangleguides.c		\
	ligmacanvasrectangleguides.h		\
	ligmacanvassamplepoint.c			\
	ligmacanvassamplepoint.h			\
	ligmacanvastextcursor.c			\
	ligmacanvastextcursor.h			\
	ligmacanvastransformguides.c		\
	ligmacanvastransformguides.h		\
	ligmacanvastransformpreview.c		\
	ligmacanvastransformpreview.h		\
	ligmacursorview.c			\
	ligmacursorview.h			\
	ligmadisplay.c				\
	ligmadisplay.h				\
	ligmadisplay-foreach.c			\
	ligmadisplay-foreach.h			\
	ligmadisplay-handlers.c			\
	ligmadisplay-handlers.h			\
	ligmadisplayshell.c			\
	ligmadisplayshell.h			\
	ligmadisplayshell-actions.c		\
	ligmadisplayshell-actions.h		\
	ligmadisplayshell-appearance.c		\
	ligmadisplayshell-appearance.h		\
	ligmadisplayshell-autoscroll.c		\
	ligmadisplayshell-autoscroll.h		\
	ligmadisplayshell-callbacks.c		\
	ligmadisplayshell-callbacks.h		\
	ligmadisplayshell-close.c		\
	ligmadisplayshell-close.h		\
	ligmadisplayshell-cursor.c		\
	ligmadisplayshell-cursor.h		\
	ligmadisplayshell-dnd.c			\
	ligmadisplayshell-dnd.h			\
	ligmadisplayshell-draw.c			\
	ligmadisplayshell-draw.h			\
	ligmadisplayshell-expose.c		\
	ligmadisplayshell-expose.h		\
	ligmadisplayshell-grab.c			\
	ligmadisplayshell-grab.h			\
	ligmadisplayshell-handlers.c		\
	ligmadisplayshell-handlers.h		\
	ligmadisplayshell-filter.c		\
	ligmadisplayshell-filter.h		\
	ligmadisplayshell-filter-dialog.c	\
	ligmadisplayshell-filter-dialog.h	\
	ligmadisplayshell-layer-select.c		\
	ligmadisplayshell-layer-select.h		\
	ligmadisplayshell-items.c		\
	ligmadisplayshell-items.h		\
	ligmadisplayshell-profile.c		\
	ligmadisplayshell-profile.h		\
	ligmadisplayshell-progress.c		\
	ligmadisplayshell-progress.h		\
	ligmadisplayshell-render.c		\
	ligmadisplayshell-render.h		\
	ligmadisplayshell-rotate.c		\
	ligmadisplayshell-rotate.h		\
	ligmadisplayshell-rotate-dialog.c	\
	ligmadisplayshell-rotate-dialog.h	\
	ligmadisplayshell-rulers.c		\
	ligmadisplayshell-rulers.h		\
	ligmadisplayshell-scale.c		\
	ligmadisplayshell-scale.h		\
	ligmadisplayshell-scale-dialog.c		\
	ligmadisplayshell-scale-dialog.h		\
	ligmadisplayshell-scroll.c		\
	ligmadisplayshell-scroll.h		\
	ligmadisplayshell-scrollbars.c		\
	ligmadisplayshell-scrollbars.h		\
	ligmadisplayshell-selection.c		\
	ligmadisplayshell-selection.h		\
	ligmadisplayshell-title.c		\
	ligmadisplayshell-title.h		\
	ligmadisplayshell-tool-events.c		\
	ligmadisplayshell-tool-events.h		\
	ligmadisplayshell-transform.c		\
	ligmadisplayshell-transform.h		\
	ligmadisplayshell-utils.c		\
	ligmadisplayshell-utils.h		\
	ligmamodifiersmanager.c			\
	ligmamodifiersmanager.h			\
	ligmaimagewindow.c			\
	ligmaimagewindow.h			\
	ligmamotionbuffer.c			\
	ligmamotionbuffer.h			\
	ligmamultiwindowstrategy.c		\
	ligmamultiwindowstrategy.h		\
	ligmanavigationeditor.c			\
	ligmanavigationeditor.h			\
	ligmascalecombobox.c			\
	ligmascalecombobox.h			\
	ligmasinglewindowstrategy.c		\
	ligmasinglewindowstrategy.h		\
	ligmastatusbar.c				\
	ligmastatusbar.h				\
	ligmatooldialog.c			\
	ligmatooldialog.h			\
	ligmatoolgui.c				\
	ligmatoolgui.h				\
	ligmatoolcompass.c			\
	ligmatoolcompass.h			\
	ligmatoolfocus.h				\
	ligmatoolfocus.c				\
	ligmatoolgyroscope.c			\
	ligmatoolgyroscope.h			\
	ligmatoolhandlegrid.c			\
	ligmatoolhandlegrid.h			\
	ligmatoolline.c				\
	ligmatoolline.h				\
	ligmatoolpath.c				\
	ligmatoolpath.h				\
	ligmatoolpolygon.c			\
	ligmatoolpolygon.h			\
	ligmatoolrectangle.c			\
	ligmatoolrectangle.h			\
	ligmatoolrotategrid.c			\
	ligmatoolrotategrid.h			\
	ligmatoolsheargrid.c			\
	ligmatoolsheargrid.h			\
	ligmatooltransform3dgrid.c		\
	ligmatooltransform3dgrid.h		\
	ligmatooltransformgrid.c			\
	ligmatooltransformgrid.h			\
	ligmatoolwidget.c			\
	ligmatoolwidget.h			\
	ligmatoolwidgetgroup.c			\
	ligmatoolwidgetgroup.h

libappdisplay_a_built_sources = display-enums.c

libappdisplay_a_SOURCES = \
	$(libappdisplay_a_built_sources)	\
	$(libappdisplay_a_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = stamp-display-enums.h xgen-dec
CLEANFILES = $(gen_sources)

xgen-dec: $(srcdir)/display-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-display-enums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"display-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-display-enums.h: xgen-dec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/display-enums.c: xgen-dec stamp-display-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi
