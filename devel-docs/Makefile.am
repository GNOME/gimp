SUBDIRS = libgimp

EXTRA_DIST = \
	README			\
	README.gtkdoc		\
	pdb/pdb_self_doc.el 	\
	pdb/pdb_dump		\
	pdb/pdb_dump.texi	\
	pdb/texinfo.tex 	\
	gih.txt  		\
	gpb.txt  		\
	parasites.txt		\
	undo.txt		\
	xcf.txt

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files macros/*; do \
	  echo $$p; \
	done

# devel-docs/Makefile.am ends here
