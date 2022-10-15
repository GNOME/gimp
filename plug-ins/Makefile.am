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
python = python
endif

if OS_WIN32
twain = twain
endif

if HAVE_WEBP
file_webp = file-webp
endif

# lkk temporarily not making these plugins until next commit fixes them for Resource
# 	gfig
#	pagecurl

SUBDIRS = \
	script-fu		\
	file-bmp		\
	$(file_darktable)	\
	file-dds		\
	$(file_exr)		\
	file-faxg3		\
	file-fits		\
	file-fli		\
	file-icns		\
	file-ico		\
	file-jpeg		\
	file-psd		\
	file-raw		\
	file-sgi		\
	file-tiff		\
	$(file_webp)		\
	flame			\
	fractal-explorer	\
	gimpressionist		\
	gradient-flare		\
	help			\
	$(help_browser)		\
	ifs-compose		\
	imagemap		\
	lighting		\
	map-object		\
	metadata		\
	$(print)		\
	$(python)		\
	screenshot		\
	selection-to-path	\
	$(twain)		\
	ui			\
	common
