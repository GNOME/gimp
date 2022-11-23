## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
xobjective_cxx = "-xobjective-c++"
xnone = "-xnone"
endif

AM_CPPFLAGS = \
	-DLIGMADIR=\""$(ligmadir)"\"			\
	-DLIGMA_VERSION=\"$(LIGMA_VERSION)\"		\
	-DLIGMA_APP_VERSION=\"$(LIGMA_APP_VERSION)\"	\
	-DLIGMA_USER_VERSION=\"$(LIGMA_USER_VERSION)\"	\
	-DG_LOG_DOMAIN=\"Ligma-Core\"			\
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_builddir)/app				\
	-I$(top_srcdir)/app				\
	$(APPSTREAM_GLIB_CFLAGS)			\
	$(CAIRO_CFLAGS)					\
	$(GEGL_CFLAGS)					\
	$(GDK_PIXBUF_CFLAGS)				\
	$(LIBMYPAINT_CFLAGS)				\
	$(MYPAINT_BRUSHES_CFLAGS)			\
	$(GEXIV2_CFLAGS)				\
	$(LIBUNWIND_CFLAGS)				\
	-I$(includedir)

AM_CFLAGS = \
	$(xobjective_c)

AM_CXXFLAGS = \
	$(xobjective_cxx)

AM_LDFLAGS = \
	$(xnone)

noinst_LIBRARIES = libappcore.a

libappcore_a_sources = \
	core-enums.h				\
	core-types.h				\
	ligma.c					\
	ligma.h					\
	ligma-atomic.c				\
	ligma-atomic.h				\
	ligma-batch.c				\
	ligma-batch.h				\
	ligma-cairo.c				\
	ligma-cairo.h				\
	ligma-contexts.c				\
	ligma-contexts.h				\
	ligma-data-factories.c			\
	ligma-data-factories.h			\
	ligma-edit.c				\
	ligma-edit.h				\
	ligma-filter-history.c			\
	ligma-filter-history.h			\
	ligma-gradients.c			\
	ligma-gradients.h			\
	ligma-gui.c				\
	ligma-gui.h				\
	ligma-internal-data.c			\
	ligma-internal-data.h			\
	ligma-memsize.c				\
	ligma-memsize.h				\
	ligma-modules.c				\
	ligma-modules.h				\
	ligma-palettes.c				\
	ligma-palettes.h				\
	ligma-parallel.cc			\
	ligma-parallel.h				\
	ligma-parasites.c			\
	ligma-parasites.h			\
	ligma-spawn.c				\
	ligma-spawn.h				\
	ligma-tags.c				\
	ligma-tags.h				\
	ligma-templates.c			\
	ligma-templates.h			\
	ligma-transform-resize.c			\
	ligma-transform-resize.h			\
	ligma-transform-3d-utils.c		\
	ligma-transform-3d-utils.h		\
	ligma-transform-utils.c			\
	ligma-transform-utils.h			\
	ligma-units.c				\
	ligma-units.h				\
	ligma-user-install.c			\
	ligma-user-install.h			\
	ligma-utils.c				\
	ligma-utils.h				\
	ligmaasync.c				\
	ligmaasync.h				\
	ligmaasyncset.c				\
	ligmaasyncset.h				\
	ligmaauxitem.c				\
	ligmaauxitem.h				\
	ligmaauxitemundo.c			\
	ligmaauxitemundo.h			\
	ligmabacktrace.h				\
	ligmabacktrace-backend.h			\
	ligmabacktrace-linux.c			\
	ligmabacktrace-none.c			\
	ligmabacktrace-windows.c			\
	ligmabezierdesc.h			\
	ligmabezierdesc.c			\
	ligmaboundary.c				\
	ligmaboundary.h				\
	ligmabrush.c				\
	ligmabrush.h				\
	ligmabrush-boundary.c			\
	ligmabrush-boundary.h			\
	ligmabrush-header.h			\
	ligmabrush-load.c			\
	ligmabrush-load.h			\
	ligmabrush-mipmap.cc			\
	ligmabrush-mipmap.h			\
	ligmabrush-private.h			\
	ligmabrush-save.c			\
	ligmabrush-save.h			\
	ligmabrush-transform.cc			\
	ligmabrush-transform.h			\
	ligmabrushcache.c			\
	ligmabrushcache.h			\
	ligmabrushclipboard.c			\
	ligmabrushclipboard.h			\
	ligmabrushgenerated.c			\
	ligmabrushgenerated.h			\
	ligmabrushgenerated-load.c		\
	ligmabrushgenerated-load.h		\
	ligmabrushgenerated-save.c		\
	ligmabrushgenerated-save.h		\
	ligmabrushpipe.c				\
	ligmabrushpipe.h				\
	ligmabrushpipe-load.c			\
	ligmabrushpipe-load.h			\
	ligmabrushpipe-save.c			\
	ligmabrushpipe-save.h			\
	ligmabuffer.c				\
	ligmabuffer.h				\
	ligmacancelable.c			\
	ligmacancelable.h			\
	ligmachannel.c				\
	ligmachannel.h				\
	ligmachannel-combine.c			\
	ligmachannel-combine.h			\
	ligmachannel-select.c			\
	ligmachannel-select.h			\
	ligmachannelpropundo.c			\
	ligmachannelpropundo.h			\
	ligmachannelundo.c			\
	ligmachannelundo.h			\
	ligmachunkiterator.c			\
	ligmachunkiterator.h			\
	ligmacontainer.c				\
	ligmacontainer.h				\
	ligmacontainer-filter.c			\
	ligmacontainer-filter.h			\
	ligmacontext.c				\
	ligmacontext.h				\
	ligmacoords.c				\
	ligmacoords.h				\
	ligmacoords-interpolate.c		\
	ligmacoords-interpolate.h		\
	ligmacurve.c				\
	ligmacurve.h				\
	ligmacurve-load.c			\
	ligmacurve-load.h			\
	ligmacurve-map.c				\
	ligmacurve-map.h				\
	ligmacurve-save.c			\
	ligmacurve-save.h			\
	ligmadashpattern.c			\
	ligmadashpattern.h			\
	ligmadata.c				\
	ligmadata.h				\
	ligmadatafactory.c			\
	ligmadatafactory.h			\
	ligmadataloaderfactory.c			\
	ligmadataloaderfactory.h			\
	ligmadisplay.c				\
	ligmadisplay.h				\
	ligmadocumentlist.c			\
	ligmadocumentlist.h			\
	ligmadrawable.c				\
	ligmadrawable.h				\
	ligmadrawable-bucket-fill.c		\
	ligmadrawable-bucket-fill.h		\
	ligmadrawable-combine.c			\
	ligmadrawable-combine.h			\
	ligmadrawable-edit.c			\
	ligmadrawable-edit.h			\
	ligmadrawable-equalize.c			\
	ligmadrawable-equalize.h			\
	ligmadrawable-fill.c			\
	ligmadrawable-fill.h			\
	ligmadrawable-filters.c			\
	ligmadrawable-filters.h			\
	ligmadrawable-floating-selection.c	\
	ligmadrawable-floating-selection.h	\
	ligmadrawable-foreground-extract.c	\
	ligmadrawable-foreground-extract.h	\
	ligmadrawable-gradient.c			\
	ligmadrawable-gradient.h			\
	ligmadrawable-histogram.c		\
	ligmadrawable-histogram.h		\
	ligmadrawable-levels.c			\
	ligmadrawable-levels.h			\
	ligmadrawable-offset.c			\
	ligmadrawable-offset.h			\
	ligmadrawable-operation.c		\
	ligmadrawable-operation.h		\
	ligmadrawable-preview.c			\
	ligmadrawable-preview.h			\
	ligmadrawable-private.h			\
	ligmadrawable-shadow.c			\
	ligmadrawable-shadow.h			\
	ligmadrawable-stroke.c			\
	ligmadrawable-stroke.h			\
	ligmadrawable-transform.c		\
	ligmadrawable-transform.h		\
	ligmadrawablefilter.c			\
	ligmadrawablefilter.h			\
	ligmadrawablemodundo.c			\
	ligmadrawablemodundo.h			\
	ligmadrawablepropundo.c			\
	ligmadrawablepropundo.h			\
	ligmadrawablestack.c			\
	ligmadrawablestack.h			\
	ligmadrawableundo.c			\
	ligmadrawableundo.h			\
	ligmadynamics.c				\
	ligmadynamics.h				\
	ligmadynamics-load.c			\
	ligmadynamics-load.h			\
	ligmadynamics-save.c			\
	ligmadynamics-save.h			\
	ligmadynamicsoutput.c			\
	ligmadynamicsoutput.h			\
	ligmaerror.c				\
	ligmaerror.h				\
	ligmaextension.c				\
	ligmaextension.h				\
	ligmaextension-error.c			\
	ligmaextension-error.h			\
	ligmaextensionmanager.c			\
	ligmaextensionmanager.h			\
	ligmafilloptions.c			\
	ligmafilloptions.h			\
	ligmafilter.c				\
	ligmafilter.h				\
	ligmafilteredcontainer.c			\
	ligmafilteredcontainer.h			\
	ligmafilterstack.c			\
	ligmafilterstack.h			\
	ligmafloatingselectionundo.c		\
	ligmafloatingselectionundo.h		\
	ligmagradient.c				\
	ligmagradient.h				\
	ligmagradient-load.c			\
	ligmagradient-load.h			\
	ligmagradient-save.c			\
	ligmagradient-save.h			\
	ligmagrid.c				\
	ligmagrid.h				\
	ligmagrouplayer.c			\
	ligmagrouplayer.h			\
	ligmagrouplayerundo.c			\
	ligmagrouplayerundo.h			\
	ligmaguide.c				\
	ligmaguide.h				\
	ligmaguideundo.c				\
	ligmaguideundo.h				\
	ligmahistogram.c				\
	ligmahistogram.h				\
	ligmaidtable.c				\
	ligmaidtable.h				\
	ligmaimage.c				\
	ligmaimage.h				\
	ligmaimage-arrange.c			\
	ligmaimage-arrange.h			\
	ligmaimage-color-profile.c		\
	ligmaimage-color-profile.h		\
	ligmaimage-colormap.c			\
	ligmaimage-colormap.h			\
	ligmaimage-convert-indexed.c		\
	ligmaimage-convert-indexed.h		\
	ligmaimage-convert-fsdither.h		\
	ligmaimage-convert-data.h		\
	ligmaimage-convert-precision.c		\
	ligmaimage-convert-precision.h		\
	ligmaimage-convert-type.c		\
	ligmaimage-convert-type.h		\
	ligmaimage-crop.c			\
	ligmaimage-crop.h			\
	ligmaimage-duplicate.c			\
	ligmaimage-duplicate.h			\
	ligmaimage-flip.c			\
	ligmaimage-flip.h			\
	ligmaimage-grid.h                	\
	ligmaimage-grid.c                	\
	ligmaimage-guides.c			\
	ligmaimage-guides.h			\
	ligmaimage-item-list.c			\
	ligmaimage-item-list.h			\
	ligmaimage-merge.c			\
	ligmaimage-merge.h			\
	ligmaimage-metadata.c			\
	ligmaimage-metadata.h			\
	ligmaimage-new.c				\
	ligmaimage-new.h				\
	ligmaimage-pick-color.c			\
	ligmaimage-pick-color.h			\
	ligmaimage-pick-item.c			\
	ligmaimage-pick-item.h			\
	ligmaimage-preview.c			\
	ligmaimage-preview.h			\
	ligmaimage-private.h			\
	ligmaimage-quick-mask.c			\
	ligmaimage-quick-mask.h			\
	ligmaimage-resize.c			\
	ligmaimage-resize.h			\
	ligmaimage-rotate.c			\
	ligmaimage-rotate.h			\
	ligmaimage-sample-points.c		\
	ligmaimage-sample-points.h		\
	ligmaimage-scale.c			\
	ligmaimage-scale.h			\
	ligmaimage-snap.c			\
	ligmaimage-snap.h			\
	ligmaimage-symmetry.c			\
	ligmaimage-symmetry.h			\
	ligmaimage-transform.c			\
	ligmaimage-transform.h			\
	ligmaimage-undo.c			\
	ligmaimage-undo.h			\
	ligmaimage-undo-push.c			\
	ligmaimage-undo-push.h			\
	ligmaimageproxy.c			\
	ligmaimageproxy.h			\
	ligmaimageundo.c				\
	ligmaimageundo.h				\
	ligmaimagefile.c				\
	ligmaimagefile.h				\
	ligmaitem.c				\
	ligmaitem.h				\
	ligmaitem-exclusive.c			\
	ligmaitem-exclusive.h			\
	ligmaitem-preview.c			\
	ligmaitem-preview.h			\
	ligmaitemlist.c				\
	ligmaitemlist.h				\
	ligmaitempropundo.c			\
	ligmaitempropundo.h			\
	ligmaitemstack.c				\
	ligmaitemstack.h				\
	ligmaitemtree.c				\
	ligmaitemtree.h				\
	ligmaitemundo.c				\
	ligmaitemundo.h				\
	ligmalayer.c				\
	ligmalayer.h				\
	ligmalayer-floating-selection.c		\
	ligmalayer-floating-selection.h		\
	ligmalayer-new.c				\
	ligmalayer-new.h				\
	ligmalayermask.c				\
	ligmalayermask.h				\
	ligmalayermaskpropundo.c			\
	ligmalayermaskpropundo.h			\
	ligmalayermaskundo.c			\
	ligmalayermaskundo.h			\
	ligmalayerpropundo.c			\
	ligmalayerpropundo.h			\
	ligmalayerstack.c			\
	ligmalayerstack.h			\
	ligmalayerundo.c				\
	ligmalayerundo.h				\
	ligmalineart.c				\
	ligmalineart.h				\
	ligmalist.c				\
	ligmalist.h				\
	ligmamaskundo.c				\
	ligmamaskundo.h				\
	ligmamybrush.c				\
	ligmamybrush.h				\
	ligmamybrush-load.c			\
	ligmamybrush-load.h			\
	ligmamybrush-private.h			\
	ligmaobject.c				\
	ligmaobject.h				\
	ligmaobjectqueue.c			\
	ligmaobjectqueue.h			\
	ligmapaintinfo.c				\
	ligmapaintinfo.h				\
	ligmapattern.c				\
	ligmapattern.h				\
	ligmapattern-header.h			\
	ligmapattern-load.c			\
	ligmapattern-load.h			\
	ligmapattern-save.c			\
	ligmapattern-save.h			\
	ligmapatternclipboard.c			\
	ligmapatternclipboard.h			\
	ligmapalette.c				\
	ligmapalette.h				\
	ligmapalette-import.c			\
	ligmapalette-import.h			\
	ligmapalette-load.c			\
	ligmapalette-load.h			\
	ligmapalette-save.c			\
	ligmapalette-save.h			\
	ligmapalettemru.c			\
	ligmapalettemru.h			\
	ligmaparamspecs.c			\
	ligmaparamspecs.h			\
	ligmaparamspecs-desc.c			\
	ligmaparamspecs-desc.h			\
	ligmaparasitelist.c			\
	ligmaparasitelist.h			\
	ligmapdbprogress.c			\
	ligmapdbprogress.h			\
	ligmapickable.c				\
	ligmapickable.h				\
	ligmapickable-auto-shrink.c		\
	ligmapickable-auto-shrink.h		\
	ligmapickable-contiguous-region.cc	\
	ligmapickable-contiguous-region.h	\
	ligmaprogress.c				\
	ligmaprogress.h				\
	ligmaprojectable.c			\
	ligmaprojectable.h			\
	ligmaprojection.c			\
	ligmaprojection.h			\
	ligmasamplepoint.c			\
	ligmasamplepoint.h			\
	ligmasamplepointundo.c			\
	ligmasamplepointundo.h			\
	ligmascanconvert.c			\
	ligmascanconvert.h			\
	ligmaselection.c				\
	ligmaselection.h				\
	ligmasettings.c				\
	ligmasettings.h				\
	ligmastrokeoptions.c			\
	ligmastrokeoptions.h			\
	ligmasubprogress.c			\
	ligmasubprogress.h			\
	ligmasymmetry.c				\
	ligmasymmetry.h				\
	ligmasymmetry-mandala.c			\
	ligmasymmetry-mandala.h			\
	ligmasymmetry-mirror.c			\
	ligmasymmetry-mirror.h			\
	ligmasymmetry-tiling.c			\
	ligmasymmetry-tiling.h			\
	ligmatag.c				\
	ligmatag.h				\
	ligmatagcache.c				\
	ligmatagcache.h				\
	ligmatagged.c				\
	ligmatagged.h				\
	ligmataggedcontainer.c			\
	ligmataggedcontainer.h			\
	ligmatempbuf.c				\
	ligmatempbuf.h				\
	ligmatemplate.c				\
	ligmatemplate.h				\
	ligmatilehandlerprojectable.c		\
	ligmatilehandlerprojectable.h		\
	ligmatoolgroup.c				\
	ligmatoolgroup.h				\
	ligmatoolinfo.c				\
	ligmatoolinfo.h				\
	ligmatoolitem.c				\
	ligmatoolitem.h				\
	ligmatooloptions.c			\
	ligmatooloptions.h			\
	ligmatoolpreset.c			\
	ligmatoolpreset.h			\
	ligmatoolpreset-load.c			\
	ligmatoolpreset-load.h			\
	ligmatoolpreset-save.c			\
	ligmatoolpreset-save.h			\
	ligmatreehandler.c			\
	ligmatreehandler.h			\
	ligmatreeproxy.c				\
	ligmatreeproxy.h				\
	ligmatriviallycancelablewaitable.c	\
	ligmatriviallycancelablewaitable.h	\
	ligmauncancelablewaitable.c		\
	ligmauncancelablewaitable.h		\
	ligmaunit.c				\
	ligmaunit.h				\
	ligmaundo.c				\
	ligmaundo.h				\
	ligmaundostack.c				\
	ligmaundostack.h				\
	ligmaviewable.c				\
	ligmaviewable.h				\
	ligmawaitable.c				\
	ligmawaitable.h

libappcore_a_built_sources = \
	ligma-core-pixbufs.c	\
	ligma-core-pixbufs.h	\
	core-enums.c		\
	ligmamarshal.c		\
	ligmamarshal.h

libappcore_a_extra_sources = \
	ligmamarshal.list

libappcore_a_SOURCES = $(libappcore_a_built_sources) $(libappcore_a_sources)

BUILT_SOURCES = \
	$(libappcore_a_built_sources)

EXTRA_DIST = \
	$(libappcore_a_extra_sources)

#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = \
	ligma-core-pixbufs.c	\
	ligma-core-pixbufs.h	\
	stamp-core-enums.h	\
	xgen-gmh		\
	xgen-gmc		\
	xgen-cec

CLEANFILES = $(gen_sources)

ligmamarshal.h: $(srcdir)/ligmamarshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=ligma_marshal $(srcdir)/ligmamarshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh $(@F) || cp xgen-gmh $(@F)) \
	&& rm -f xgen-gmh xgen-gmh~

ligmamarshal.c: ligmamarshal.h
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=ligma_marshal $(srcdir)/ligmamarshal.list --prototypes --body >> xgen-gmc \
	&& cp xgen-gmc $(@F) \
	&& rm -f xgen-gmc xgen-gmc~

xgen-cec: $(srcdir)/core-enums.h $(LIGMA_MKENUMS) Makefile.am
	$(AM_V_GEN) $(LIGMA_MKENUMS) \
		--fhead "#include \"stamp-core-enums.h\"\n#include \"config.h\"\n#include <gio/gio.h>\n#include \"libligmabase/ligmabase.h\"\n#include \"core-enums.h\"\n#include \"ligma-intl.h\"" \
		--fprod "\n/* enumerations from \"@basename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Ligma@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ },@if ('@valueabbrev@' ne 'NULL')@\n    /* Translators: this is an abbreviated version of @valueudesc@.\n       Keep it short. */\n    { @VALUENAME@, @valueabbrev@, NULL },@endif@" \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      ligma_type_set_translation_context (type, \"@enumnick@\");\n      ligma_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$< > $@

stamp-core-enums.h: xgen-cec
	echo "/* Generated on `date`. */" > $@

# copy the generated enum file back to the source directory only if it's
# changed; otherwise, only update its timestamp, so that the recipe isn't
# executed again on the next build, however, allow this to (harmlessly) fail,
# to support building from a read-only source tree.
$(srcdir)/core-enums.c: xgen-cec stamp-core-enums.h
	$(AM_V_GEN) if ! cmp -s $< $@; then \
		cp $< $@; \
	else \
		touch $@ 2> /dev/null \
		|| true; \
	fi

ligma-core-pixbufs.h: $(top_builddir)/icons/Color/ligma-core-pixbufs.gresource.xml
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(top_builddir)/icons/Color --generate-header \
	  --target=$@ $(top_builddir)/icons/Color/ligma-core-pixbufs.gresource.xml

ligma-core-pixbufs.c: ligma-core-pixbufs.h
	$(AM_V_GEN) $(HOST_GLIB_COMPILE_RESOURCES) \
	  --sourcedir=$(top_builddir)/icons/Color --generate-source \
	  --target=$@ $(top_builddir)/icons/Color/ligma-core-pixbufs.gresource.xml
