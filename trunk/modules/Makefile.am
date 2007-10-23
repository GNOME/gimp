## Process this file with automake to produce Makefile.in

libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpmodule = $(top_builddir)/libgimpmodule/libgimpmodule-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

libdir = $(gimpplugindir)/modules

INCLUDES = \
        -I$(top_srcdir)	\
        $(GTK_CFLAGS)	\
        -I$(includedir)

EXTRA_DIST = \
	makefile.msc

if HAVE_LCMS
cdisplay_lcms_module = libcdisplay_lcms.la
cdisplay_proof_module = libcdisplay_proof.la
endif

if PLATFORM_WIN32
else
controller_midi_module = libcontroller_midi.la
endif

if HAVE_LINUX_INPUT
controller_linux_input_module = libcontroller_linux_input.la
endif

if HAVE_DX_DINPUT
controller_dx_dinput_module = libcontroller_dx_dinput.la
endif

lib_LTLIBRARIES = \
	libcolorsel_cmyk.la		\
	libcolorsel_triangle.la		\
	libcolorsel_water.la		\
	libcdisplay_colorblind.la	\
	libcdisplay_gamma.la		\
	libcdisplay_highcontrast.la	\
	$(cdisplay_lcms_module)		\
	$(cdisplay_proof_module)	\
	$(controller_midi_module)	\
	$(controller_linux_input_module)\
	$(controller_dx_dinput_module)


modules_libadd = $(libgimpmodule) $(libgimpwidgets) $(GTK_LIBS)

colorsel_libadd = $(libgimpcolor) $(modules_libadd)
cdisplay_libadd = $(libgimpbase) $(libgimpconfig) $(libgimpcolor) $(modules_libadd)
controller_libadd = $(modules_libadd)

if HAVE_LCMS
libcolorsel_cmyk_la_SOURCES = colorsel_cmyk_lcms.c
libcolorsel_cmyk_la_LIBADD = $(libgimpconfig) $(colorsel_libadd) $(LIBLCMS)
else
libcolorsel_cmyk_la_SOURCES = colorsel_cmyk.c
libcolorsel_cmyk_la_LIBADD = $(colorsel_libadd)
endif

libcolorsel_cmyk_la_LDFLAGS = -avoid-version -module $(no_undefined)

libcolorsel_triangle_la_SOURCES = colorsel_triangle.c
libcolorsel_triangle_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolorsel_triangle_la_LIBADD = $(colorsel_libadd)

libcolorsel_water_la_SOURCES = colorsel_water.c
libcolorsel_water_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolorsel_water_la_LIBADD = $(colorsel_libadd)

libcdisplay_colorblind_la_SOURCES = cdisplay_colorblind.c
libcdisplay_colorblind_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_colorblind_la_LIBADD = $(cdisplay_libadd)

libcdisplay_gamma_la_SOURCES = cdisplay_gamma.c
libcdisplay_gamma_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_gamma_la_LIBADD = $(cdisplay_libadd)

libcdisplay_highcontrast_la_SOURCES = cdisplay_highcontrast.c
libcdisplay_highcontrast_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_highcontrast_la_LIBADD = $(cdisplay_libadd)

libcdisplay_lcms_la_SOURCES = cdisplay_lcms.c
libcdisplay_lcms_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_lcms_la_LIBADD = $(cdisplay_libadd) $(LIBLCMS)

if PLATFORM_WIN32
libcdisplay_lcms_la_LIBADD += -lgdi32
endif

libcdisplay_proof_la_SOURCES = cdisplay_proof.c
libcdisplay_proof_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcdisplay_proof_la_LIBADD = $(cdisplay_libadd) $(LIBLCMS)

libcontroller_linux_input_la_SOURCES = \
	gimpinputdevicestore.c	\
	gimpinputdevicestore.h	\
	controller_linux_input.c
libcontroller_linux_input_la_CFLAGS = $(DBUS_GLIB_CFLAGS) $(HAL_CFLAGS)
libcontroller_linux_input_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcontroller_linux_input_la_LIBADD = \
	$(controller_libadd) $(DBUS_GLIB_LIBS) $(HAL_LIBS)

libcontroller_dx_dinput_la_SOURCES = \
	gimpinputdevicestore.c	\
	gimpinputdevicestore.h	\
	controller_dx_dinput.c
# Use -Wl to avoid libtool lossage
libcontroller_dx_dinput_la_LDFLAGS = -avoid-version -module $(no_undefined) -Wl,-ldinput8 -Wl,-ldxguid
libcontroller_dx_dinput_la_LIBADD = \
	$(controller_libadd) -lrpcrt4

libcontroller_midi_la_SOURCES = controller_midi.c
libcontroller_midi_la_CFLAGS = $(ALSA_CFLAGS)
libcontroller_midi_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcontroller_midi_la_LIBADD = $(controller_libadd) $(ALSA_LIBS)
