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
display_filter_lcms_module = libdisplay-filter-lcms.la
display_filter_proof_module = libdisplay-filter-proof.la
endif

if PLATFORM_WIN32
else
controller_midi_module = libcontroller-midi.la
endif

if HAVE_LINUX_INPUT
controller_linux_input_module = libcontroller-linux-input.la
endif

if HAVE_DX_DINPUT
controller_dx_dinput_module = libcontroller-dx-dinput.la
endif

lib_LTLIBRARIES = \
	libcolor-selector-cmyk.la		\
	libcolor-selector-water.la		\
	libcolor-selector-wheel.la		\
	libdisplay-filter-color-blind.la	\
	libdisplay-filter-gamma.la		\
	libdisplay-filter-high-contrast.la	\
	$(display_filter_lcms_module)		\
	$(display_filter_proof_module)		\
	$(controller_midi_module)		\
	$(controller_linux_input_module)	\
	$(controller_dx_dinput_module)


modules_libadd = $(libgimpmodule) $(libgimpwidgets) $(GTK_LIBS)

color_selector_libadd = $(libgimpcolor) $(modules_libadd)
display_filter_libadd = $(libgimpbase) $(libgimpconfig) $(libgimpcolor) $(modules_libadd)
controller_libadd = $(modules_libadd)

if HAVE_LCMS
libcolor_selector_cmyk_la_SOURCES = color-selector-cmyk-lcms.c
libcolor_selector_cmyk_la_CFLAGS = $(LCMS_CFLAGS)
libcolor_selector_cmyk_la_LIBADD = $(libgimpconfig) $(color_selector_libadd) $(LCMS_LIBS)
else
libcolor_selector_cmyk_la_SOURCES = color-selector-cmyk.c
libcolor_selector_cmyk_la_LIBADD = $(color_selector_libadd)
endif

libcolor_selector_cmyk_la_LDFLAGS = -avoid-version -module $(no_undefined)

libcolor_selector_water_la_SOURCES = color-selector-water.c
libcolor_selector_water_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolor_selector_water_la_LIBADD = $(color_selector_libadd)

libcolor_selector_wheel_la_SOURCES = color-selector-wheel.c
libcolor_selector_wheel_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolor_selector_wheel_la_LIBADD = $(color_selector_libadd)

libdisplay_filter_color_blind_la_SOURCES = display-filter-color-blind.c
libdisplay_filter_color_blind_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_color_blind_la_LIBADD = $(display_filter_libadd)

libdisplay_filter_gamma_la_SOURCES = display-filter-gamma.c
libdisplay_filter_gamma_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_gamma_la_LIBADD = $(display_filter_libadd)

libdisplay_filter_high_contrast_la_SOURCES = display-filter-high-contrast.c
libdisplay_filter_high_contrast_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_high_contrast_la_LIBADD = $(display_filter_libadd)

libdisplay_filter_lcms_la_SOURCES = display-filter-lcms.c
libdisplay_filter_lcms_la_CFLAGS = $(LCMS_CFLAGS)
libdisplay_filter_lcms_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_lcms_la_LIBADD = $(display_filter_libadd) $(LCMS_LIBS)

if PLATFORM_WIN32
libdisplay_filter_lcms_la_LIBADD += -lgdi32
endif

libdisplay_filter_proof_la_SOURCES = display-filter-proof.c
libdisplay_filter_proof_la_CFLAGS = $(LCMS_CFLAGS)
libdisplay_filter_proof_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_proof_la_LIBADD = $(display_filter_libadd) $(LCMS_LIBS)

libcontroller_linux_input_la_SOURCES = \
	gimpinputdevicestore-gudev.c	\
	gimpinputdevicestore.h		\
	controller-linux-input.c
libcontroller_linux_input_la_CFLAGS = $(GUDEV_CFLAGS)
libcontroller_linux_input_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcontroller_linux_input_la_LIBADD = \
	$(controller_libadd) $(GUDEV_LIBS)

libcontroller_dx_dinput_la_SOURCES = \
	gimpinputdevicestore-dx.c	\
	gimpinputdevicestore.h		\
	controller-dx-dinput.c
# Use -Wl to avoid libtool lossage
libcontroller_dx_dinput_la_LDFLAGS = -avoid-version -module $(no_undefined) -Wl,-ldinput8 -Wl,-ldxguid
libcontroller_dx_dinput_la_LIBADD = \
	$(controller_libadd) -lrpcrt4

libcontroller_midi_la_SOURCES = controller-midi.c
libcontroller_midi_la_CFLAGS = $(ALSA_CFLAGS)
libcontroller_midi_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcontroller_midi_la_LIBADD = $(controller_libadd) $(ALSA_LIBS)
