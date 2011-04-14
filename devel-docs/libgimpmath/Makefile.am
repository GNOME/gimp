## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE = libgimpmath

# The top-level SGML file.
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.sgml

# The directory containing the source code.
DOC_SOURCE_DIR = $(top_srcdir)/$(DOC_MODULE)

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS =

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS =

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS = --sgml-mode --output-format=xml --name-space=gimp

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS =

# Used for dependencies
HFILE_GLOB = $(DOC_SOURCE_DIR)/*.h
CFILE_GLOB = $(DOC_SOURCE_DIR)/*.c

# Header files to ignore when scanning
IGNORE_HFILES =

# Images to copy into HTML directory
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =

# Other files to distribute
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
GTKDOC_CFLAGS =
GTKDOC_LIBS =


include $(top_srcdir)/gtk-doc.make

# Other files to distribute
EXTRA_DIST += version.in
