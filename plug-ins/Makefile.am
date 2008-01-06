## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.msc


if BUILD_HELPBROWSER
helpbrowser = helpbrowser
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
winicon = winicon
endif

if OS_WIN32
twain = twain
winsnap = winsnap
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
	FractalExplorer		\
	Lighting		\
	MapObject		\
	bmp			\
	faxg3			\
	fits			\
	flame			\
	gfig			\
	gflare			\
	gfli			\
	gimpressionist		\
	help			\
	$(helpbrowser)		\
	ifscompose		\
	imagemap		\
	$(jpeg)                 \
	maze			\
	metadata		\
	pagecurl		\
	$(print)		\
	$(pygimp)		\
	psd			\
	rcm			\
	sgi			\
	sel2path		\
	$(twain)		\
	$(uri)			\
	$(winicon)		\
	$(winsnap)		\
	$(xjt)			\
	common
