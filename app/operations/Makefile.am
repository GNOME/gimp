## Process this file with automake to produce Makefile.in

SUBDIRS = \
	layer-modes		\
	layer-modes-legacy	\
	tests

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"Gimp-Operations\"	\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_builddir)/app			\
	-I$(top_srcdir)/app			\
	$(CAIRO_CFLAGS)				\
	$(GEGL_CFLAGS)				\
	$(GDK_PIXBUF_CFLAGS)			\
	-I$(includedir)

noinst_LIBRARIES = \
	libappoperations.a

libappoperations_a_SOURCES = \
	operations-types.h			\
	gimp-operations.c			\
	gimp-operations.h			\
	\
	gimpbrightnesscontrastconfig.c		\
	gimpbrightnesscontrastconfig.h		\
	gimpcageconfig.c			\
	gimpcageconfig.h			\
	gimpcolorbalanceconfig.c		\
	gimpcolorbalanceconfig.h		\
	gimpcolorizeconfig.c			\
	gimpcolorizeconfig.h			\
	gimpcurvesconfig.c			\
	gimpcurvesconfig.h			\
	gimphuesaturationconfig.c		\
	gimphuesaturationconfig.h		\
	gimplevelsconfig.c			\
	gimplevelsconfig.h			\
	\
	gimpoperationblend.c			\
	gimpoperationblend.h			\
	gimpoperationborder.c			\
	gimpoperationborder.h			\
	gimpoperationcagecoefcalc.c		\
	gimpoperationcagecoefcalc.h		\
	gimpoperationcagetransform.c		\
	gimpoperationcagetransform.h		\
	gimpoperationcomposecrop.c		\
	gimpoperationcomposecrop.h		\
	gimpoperationequalize.c			\
	gimpoperationequalize.h			\
	gimpoperationflood.c			\
	gimpoperationflood.h			\
	gimpoperationgrow.c			\
	gimpoperationgrow.h			\
	gimpoperationhistogramsink.c		\
	gimpoperationhistogramsink.h		\
	gimpoperationmaskcomponents.c		\
	gimpoperationmaskcomponents.h		\
	gimpoperationprofiletransform.c		\
	gimpoperationprofiletransform.h		\
	gimpoperationscalarmultiply.c		\
	gimpoperationscalarmultiply.h		\
	gimpoperationsemiflatten.c		\
	gimpoperationsemiflatten.h		\
	gimpoperationsetalpha.c			\
	gimpoperationsetalpha.h			\
	gimpoperationshapeburst.c		\
	gimpoperationshapeburst.h		\
	gimpoperationshrink.c			\
	gimpoperationshrink.h			\
	gimpoperationthresholdalpha.c		\
	gimpoperationthresholdalpha.h		\
	\
	gimpoperationpointfilter.c		\
	gimpoperationpointfilter.h		\
	gimpoperationbrightnesscontrast.c	\
	gimpoperationbrightnesscontrast.h	\
	gimpoperationcolorbalance.c		\
	gimpoperationcolorbalance.h		\
	gimpoperationcolorize.c			\
	gimpoperationcolorize.h			\
	gimpoperationcurves.c			\
	gimpoperationcurves.h			\
	gimpoperationdesaturate.c		\
	gimpoperationdesaturate.h		\
	gimpoperationhuesaturation.c		\
	gimpoperationhuesaturation.h		\
	gimpoperationlevels.c			\
	gimpoperationlevels.h			\
	gimpoperationposterize.c		\
	gimpoperationposterize.h		\
	gimpoperationthreshold.c		\
	gimpoperationthreshold.h		\
	\
	gimpoperationpointlayermode.c		\
	gimpoperationpointlayermode.h		\
	gimpoperationdissolvemode.c     	\
	gimpoperationdissolvemode.h     	\
	gimpoperationbehindmode.c       	\
	gimpoperationbehindmode.h       	\
	gimpoperationscreenmode.c       	\
	gimpoperationscreenmode.h       	\
	gimpoperationoverlaymode.c      	\
	gimpoperationoverlaymode.h      	\
	gimpoperationdifferencemode.c   	\
	gimpoperationdifferencemode.h   	\
	gimpoperationadditionmode.c     	\
	gimpoperationadditionmode.h     	\
	gimpoperationsubtractmode.c     	\
	gimpoperationsubtractmode.h     	\
	gimpoperationdarkenonlymode.c   	\
	gimpoperationdarkenonlymode.h   	\
	gimpoperationlightenonlymode.c  	\
	gimpoperationlightenonlymode.h  	\
	gimpoperationhuemode.c          	\
	gimpoperationhuemode.h          	\
	gimpoperationsaturationmode.c   	\
	gimpoperationsaturationmode.h   	\
	gimpoperationcolormode.c        	\
	gimpoperationcolormode.h        	\
	gimpoperationvaluemode.c        	\
	gimpoperationvaluemode.h        	\
	gimpoperationdividemode.c       	\
	gimpoperationdividemode.h       	\
	gimpoperationburnmode.c         	\
	gimpoperationburnmode.h         	\
	gimpoperationhardlightmode.c    	\
	gimpoperationhardlightmode.h    	\
	gimpoperationsoftlightmode.c    	\
	gimpoperationsoftlightmode.h    	\
	gimpoperationgrainextractmode.c 	\
	gimpoperationgrainextractmode.h 	\
	gimpoperationgrainmergemode.c   	\
	gimpoperationgrainmergemode.h   	\
	gimpoperationcolorerasemode.c   	\
	gimpoperationcolorerasemode.h   	\
	gimpoperationlchhuemode.c		\
	gimpoperationlchhuemode.h		\
	gimpoperationlchchromamode.c		\
	gimpoperationlchchromamode.h		\
	gimpoperationlchcolormode.c		\
	gimpoperationlchcolormode.h		\
	gimpoperationlchlightnessmode.c		\
	gimpoperationlchlightnessmode.h		\
	gimpoperationerasemode.c        	\
	gimpoperationerasemode.h        	\
	gimpoperationreplacemode.c      	\
	gimpoperationreplacemode.h      	\
	gimpoperationantierasemode.c    	\
	gimpoperationantierasemode.h		\
	\
	gimplayermodefunctions.c		\
	gimplayermodefunctions.h
