## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.msc


if BUILD_HELP_BROWSER
help_browser = help-browser
endif

if BUILD_JPEG
file_jpeg = file-jpeg
file_psd = file-psd
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
file_ico = file-ico
endif

if OS_WIN32
twain = twain
win_snap = win-snap
if HAVE_LIBCURL
file_uri = file-uri
endif
else
file_uri = file-uri
endif

if BUILD_XJT
file_xjt = file-xjt
endif

if BUILD_METADATA
metadata = metadata
endif


SUBDIRS = \
	$(script_fu)		\
	$(pygimp)		\
	color-rotate		\
	file-bmp		\
	file-faxg3		\
	file-fits		\
	file-fli		\
	$(file_ico)		\
	$(file_jpeg)		\
	$(file_psd)		\
	file-sgi		\
	$(file_uri)		\
	$(file_xjt)		\
	flame			\
	fractal-explorer	\
	gfig			\
	ui			\
	gimpressionist		\
	gradient-flare		\
	help			\
	$(help_browser)		\
	ifs-compose		\
	imagemap		\
	lighting		\
	map-object		\
	maze			\
	$(metadata)		\
	pagecurl		\
	$(print)		\
	selection-to-path	\
	$(twain)		\
	$(win_snap)		\
	common
