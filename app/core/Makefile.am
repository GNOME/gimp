## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @STRIP_BEGIN@ \
	-DG_LOG_DOMAIN=\"Gimp-Core\"	\
	@GIMP_THREAD_FLAGS@ 		\
	@GIMP_MP_FLAGS@			\
@STRIP_END@

INCLUDES = @STRIP_BEGIN@ \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/app		\
	$(GTK_CFLAGS)			\
	-I$(includedir)			\
@STRIP_END@

noinst_LIBRARIES = libappcore.a

libappcore_a_sources = @STRIP_BEGIN@ \
	core-types.h			\
	gimp.c				\
	gimp.h				\
	gimpbrush.c			\
	gimpbrush.h			\
	gimpbrush-header.h		\
	gimpbrushgenerated.c		\
	gimpbrushgenerated.h		\
	gimpbrushpipe.c			\
	gimpbrushpipe.h			\
	gimpbuffer.c			\
	gimpbuffer.h			\
	gimpchannel.c			\
	gimpchannel.h			\
	gimpcontainer.c			\
	gimpcontainer.h			\
	gimpcontext.c			\
	gimpcontext.h			\
	gimpcoreconfig.c		\
	gimpcoreconfig.h		\
	gimpdata.c			\
	gimpdata.h			\
	gimpdatafactory.c		\
	gimpdatafactory.h		\
	gimpdatafiles.c			\
	gimpdatafiles.h			\
	gimpdatalist.c			\
	gimpdatalist.h			\
	gimpdocuments.c			\
	gimpdocuments.h			\
	gimpdrawable.c			\
	gimpdrawable.h			\
	gimpdrawable-blend.c		\
	gimpdrawable-blend.h		\
	gimpdrawable-bucket-fill.c	\
	gimpdrawable-bucket-fill.h	\
	gimpdrawable-desaturate.c	\
	gimpdrawable-desaturate.h	\
	gimpdrawable-equalize.c		\
	gimpdrawable-equalize.h		\
	gimpdrawable-histogram.c	\
	gimpdrawable-histogram.h	\
	gimpdrawable-invert.c		\
	gimpdrawable-invert.h		\
	gimpdrawable-offset.c		\
	gimpdrawable-offset.h		\
	gimpdrawable-preview.c		\
	gimpdrawable-preview.h		\
	gimpdrawable-transform.c	\
	gimpdrawable-transform.h	\
	gimpdrawable-transform-utils.c	\
	gimpdrawable-transform-utils.h	\
	gimpedit.c			\
	gimpedit.h			\
	gimpgradient.c			\
	gimpgradient.h			\
	gimpimage.c			\
	gimpimage.h			\
	gimpimage-colorhash.c		\
	gimpimage-colorhash.h		\
	gimpimage-contiguous-region.c	\
	gimpimage-contiguous-region.h	\
	gimpimage-convert.c		\
	gimpimage-convert.h		\
	gimpimage-convert-fsdither.h	\
	gimpimage-crop.c		\
	gimpimage-crop.h		\
	gimpimage-duplicate.c		\
	gimpimage-duplicate.h		\
	gimpimage-guides.c		\
	gimpimage-guides.h		\
	gimpimage-mask.c		\
	gimpimage-mask.h		\
	gimpimage-mask-select.c		\
	gimpimage-mask-select.h		\
	gimpimage-merge.c		\
	gimpimage-merge.h		\
	gimpimage-new.c			\
	gimpimage-new.h			\
	gimpimage-projection.c		\
	gimpimage-projection.h		\
	gimpimage-resize.c		\
	gimpimage-resize.h		\
	gimpimage-scale.c		\
	gimpimage-scale.h		\
	gimpimage-undo.c		\
	gimpimage-undo.h		\
	gimpimagefile.c			\
	gimpimagefile.h			\
	gimplayer.c			\
	gimplayer.h			\
	gimplayermask.c			\
	gimplayermask.h			\
	gimplist.c			\
	gimplist.h			\
	gimpmoduleinfo.c		\
	gimpmoduleinfo.h		\
	gimpmodules.c			\
	gimpmodules.h			\
	gimpobject.c			\
	gimpobject.h			\
	gimppattern.c			\
	gimppattern.h			\
	gimppattern-header.h		\
	gimppalette.c			\
	gimppalette.h			\
	gimppalette-import.c		\
	gimppalette-import.h		\
	gimpparasite.c			\
	gimpparasite.h			\
	gimpparasitelist.c		\
	gimpparasitelist.h		\
	gimppreviewcache.c		\
	gimppreviewcache.h		\
	gimpscanconvert.c		\
	gimpscanconvert.h		\
	gimptoolinfo.c			\
	gimptoolinfo.h			\
	gimpunit.c			\
	gimpunit.h			\
	gimpundo.c			\
	gimpundo.h			\
	gimpundostack.c			\
	gimpundostack.h			\
	gimpviewable.c			\
	gimpviewable.h			\
@STRIP_END@

libappcore_a_built_sources = @STRIP_BEGIN@ \
	stamp-gimpmarshal.h		\
	gimpmarshal.c			\
	gimpmarshal.h			\
@STRIP_END@

libappcore_a_extra_sources = @STRIP_BEGIN@ \
	gimpmarshal.list		\
@STRIP_END@

libappcore_a_SOURCES = $(libappcore_a_built_sources) $(libappcore_a_sources)

EXTRA_DIST = \
	makefile.msc			\
	$(libappcore_a_extra_sources)

#
# rules to generate built sources
#
# setup autogeneration dependancies
gen_sources = xgen-gmh xgen-gmc
CLEANFILES += $(gen_sources)

$(srcdir)/stamp-gimpmarshal.h: $(srcdir)/gimpmarshal.list gimpmarshal.h
	glib-genmarshal --prefix=gimp_marshal $(srcdir)/gimpmarshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh $(srcdir)/gimpmarshal.h || cp xgen-gmh $(srcdir)/gimpmarshal.h) \
	&& rm -f xgen-gmh xgen-gmh~ \
	&& echo timestamp > $@
$(srcdir)/gimpmarshal.c: $(srcdir)/stamp-gimpmarshal.h
	echo "#include <glib.h>" > xgen-gmc \
	&& echo "#include <glib-object.h>" >> xgen-gmc \
	&& echo "#include \"gimpmarshal.h\"" >> xgen-gmc \
	&& glib-genmarshal --prefix=gimp_marshal $(srcdir)/gimpmarshal.list --body >> xgen-gmc \
	&& cp xgen-gmc $(srcdir)/gimpmarshal.c \
	&& rm -f xgen-gmc xgen-gmc~
