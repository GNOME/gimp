## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.msc


if BUILD_HELP_BROWSER
help_browser = help-browser
endif

if BUILD_JPEG
jpeg = jpeg
endif

if BUILD_PRINT
print = print
endif

if BUILD_PYTHON
pygimp = pygimp
endif

if BUILD_SCRIPT_FU
script_fu = script-fu
endif

if HAVE_MAC_TWAIN
twain = twain
endif

if HAVE_PNG
win_icon = win-icon
endif

if OS_WIN32
twain = twain
win_snap = win-snap
if HAVE_LIBCURL
uri = uri
endif
else
uri = uri
endif

if BUILD_XJT
xjt = xjt
endif


SUBDIRS = \
	$(script_fu)		\
	$(pygimp)		\
	bmp			\
	color-rotate		\
	faxg3			\
	fits			\
	flame			\
	fli			\
	fractal-explorer	\
	gfig			\
	gimpressionist		\
	gradient-flare		\
	help			\
	$(help_browser)		\
	ifs-compose		\
	imagemap		\
	$(jpeg)                 \
	lighting		\
	map-object		\
	maze			\
	metadata		\
	pagecurl		\
	$(print)		\
	psd			\
	selection-to-path	\
	sgi			\
	$(twain)		\
	$(uri)			\
	$(win_icon)		\
	$(win_snap)		\
	$(xjt)			\
	common
