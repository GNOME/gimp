## Process this file with automake to produce Makefile.in

if HAVE_WEBKIT
help_browser = help-browser
endif

if HAVE_OPENEXR
file_exr = file-exr
endif

if BUILD_PRINT
print = print
endif

if BUILD_PYTHON
pygimp = pygimp
endif

if OS_WIN32
twain = twain
endif

if HAVE_WEBP
file_webp = file-webp
endif

SUBDIRS = \
	script-fu		\
##	$(pygimp)		\
	file-bmp		\
	$(file_darktable)	\
	$(file_exr)		\
	file-faxg3		\
	file-fits		\
	file-fli		\
	file-ico		\
	file-jpeg		\
	file-psd		\
	file-raw		\
	file-sgi		\
	file-tiff		\
	$(file_webp)		\
	flame			\
	fractal-explorer	\
	gfig			\
##	gimpressionist		\
	gradient-flare		\
	help			\
	$(help_browser)		\
	ifs-compose		\
	imagemap		\
	lighting		\
	map-object		\
	metadata		\
	pagecurl		\
	$(print)		\
	screenshot		\
	selection-to-path	\
	$(twain)		\
	ui			\
	common
