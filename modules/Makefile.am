## Process this file with automake to produce Makefile.in

libligmabase = $(top_builddir)/libligmabase/libligmabase-$(LIGMA_API_VERSION).la
libligmacolor = $(top_builddir)/libligmacolor/libligmacolor-$(LIGMA_API_VERSION).la
libligmaconfig = $(top_builddir)/libligmaconfig/libligmaconfig-$(LIGMA_API_VERSION).la
libligmamodule = $(top_builddir)/libligmamodule/libligmamodule-$(LIGMA_API_VERSION).la
libligmawidgets = $(top_builddir)/libligmawidgets/libligmawidgets-$(LIGMA_API_VERSION).la

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

libdir = $(ligmaplugindir)/modules

AM_CPPFLAGS = \
        -I$(top_srcdir)	\
	$(GEGL_CFLAGS)	\
        $(GTK_CFLAGS)	\
        -I$(includedir)

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
	libdisplay-filter-clip-warning.la	\
	libdisplay-filter-color-blind.la	\
	libdisplay-filter-aces-rrt.la		\
	libdisplay-filter-gamma.la		\
	libdisplay-filter-high-contrast.la	\
	$(controller_midi_module)		\
	$(controller_linux_input_module)	\
	$(controller_dx_dinput_module)


modules_libadd = $(libligmamodule) $(libligmawidgets) $(GTK_LIBS)

color_selector_libadd = $(libligmaconfig) $(libligmacolor) $(modules_libadd) $(BABL_LIBS)
display_filter_libadd = $(libligmabase) $(libligmaconfig) $(libligmacolor) $(modules_libadd) $(GEGL_LIBS)
controller_libadd = $(modules_libadd)

libcolor_selector_cmyk_la_SOURCES = color-selector-cmyk.c
libcolor_selector_cmyk_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolor_selector_cmyk_la_LIBADD = $(libligmaconfig) $(color_selector_libadd)

libcolor_selector_water_la_SOURCES = color-selector-water.c
libcolor_selector_water_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolor_selector_water_la_LIBADD = $(color_selector_libadd)

libcolor_selector_wheel_la_SOURCES = color-selector-wheel.c ligmacolorwheel.c ligmacolorwheel.h
libcolor_selector_wheel_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcolor_selector_wheel_la_LIBADD = $(color_selector_libadd)

libdisplay_filter_clip_warning_la_SOURCES = display-filter-clip-warning.c
libdisplay_filter_clip_warning_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_clip_warning_la_LIBADD = $(display_filter_libadd)

libdisplay_filter_color_blind_la_SOURCES = display-filter-color-blind.c
libdisplay_filter_color_blind_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_color_blind_la_LIBADD = $(display_filter_libadd)


libdisplay_filter_aces_rrt_la_SOURCES = display-filter-aces-rrt.c
libdisplay_filter_aces_rrt_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_aces_rrt_la_LIBADD = $(display_filter_libadd)

libdisplay_filter_gamma_la_SOURCES = display-filter-gamma.c
libdisplay_filter_gamma_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_gamma_la_LIBADD = $(display_filter_libadd)

libdisplay_filter_high_contrast_la_SOURCES = display-filter-high-contrast.c
libdisplay_filter_high_contrast_la_LDFLAGS = -avoid-version -module $(no_undefined)
libdisplay_filter_high_contrast_la_LIBADD = $(display_filter_libadd)

libcontroller_linux_input_la_SOURCES = \
	ligmainputdevicestore-gudev.c	\
	ligmainputdevicestore.h		\
	controller-linux-input.c
libcontroller_linux_input_la_CFLAGS = $(GUDEV_CFLAGS)
libcontroller_linux_input_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcontroller_linux_input_la_LIBADD = \
	$(controller_libadd) $(GUDEV_LIBS)

libcontroller_dx_dinput_la_SOURCES = \
	ligmainputdevicestore-dx.c	\
	ligmainputdevicestore.h		\
	controller-dx-dinput.c
# Use -Wl to avoid libtool lossage
libcontroller_dx_dinput_la_LDFLAGS = -avoid-version -module $(no_undefined) -Wl,-ldinput8 -Wl,-ldxguid
libcontroller_dx_dinput_la_LIBADD = \
	$(controller_libadd) -lrpcrt4

libcontroller_midi_la_SOURCES = controller-midi.c
libcontroller_midi_la_CFLAGS = $(ALSA_CFLAGS)
libcontroller_midi_la_LDFLAGS = -avoid-version -module $(no_undefined)
libcontroller_midi_la_LIBADD = $(controller_libadd) $(ALSA_LIBS)
