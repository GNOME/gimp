## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.msc


if BUILD_HELPBROWSER
helpbrowser = helpbrowser
endif

if BUILD_PYTHON
pygimp = pygimp
endif

if HAVE_MAC_TWAIN
twain = twain
endif

if OS_WIN32
twain = twain
winsnap = winsnap
else
uri = uri
endif

if BUILD_XJT
xjt = xjt
endif


SUBDIRS = \
	script-fu		\
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
	jpeg                    \
	maze			\
	metadata		\
	pagecurl		\
	$(pygimp)		\
	rcm			\
	sgi			\
	sel2path		\
	$(twain)		\
	$(uri)			\
	winicon			\
	$(winsnap)		\
	$(xjt)			\
	common
