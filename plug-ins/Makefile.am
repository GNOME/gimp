## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	makefile.msc

if BUILD_PRINT
print = print
endif

if BUILD_PYTHON
pygimp = pygimp
endif

if BUILD_HELPBROWSER
helpbrowser = helpbrowser
endif

if BUILD_XJT
xjt = xjt
endif

if OS_WIN32
win32dirs = \
	twain	\
	winsnap
endif


SUBDIRS = \
	libgimpoldpreview	\
	dbbrowser		\
	script-fu		\
	FractalExplorer		\
	Lighting		\
	MapObject		\
	bmp			\
	faxg3			\
	fits			\
	flame			\
	fp			\
	gfig			\
	gflare			\
	gfli			\
	gimpressionist		\
	help			\
	$(helpbrowser)		\
	ifscompose		\
	imagemap		\
	maze			\
	pagecurl		\
	$(print)		\
	$(pygimp)		\
	rcm			\
	sgi			\
	sel2path		\
	winicon			\
	$(win32dirs)		\
	$(xjt)			\
	common
