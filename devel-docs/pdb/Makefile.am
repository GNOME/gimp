EXTRA_DIST = \
	pdb.texi	\
	pdb_self_doc.el \
	texinfo.tex

pdb_dump.texi: $(srcdir)/pdb_self_doc.el $(srcdir)/pdb_dump
	cd $(srcdir) && \
	@EMACS@ --batch -l pdb_self_doc.el -f make-docs-noargs

## use `cp' followed by `rm' since `mv' may not be able to move across mount pts
$(srcdir)/pdb_dump: $(top_srcdir)/app/*_cmds.c $(top_builddir)/app/gimp
	$(top_builddir)/app/gimp --no-interface --batch '(gimp-procedural-db-dump "pdb_dump.tmp")' '(gimp-quit 0)' 
	cp pdb_dump.tmp $(srcdir)/pdb_dump
	-rm -f pdb_dump.tmp

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files macros/*; do \
	  echo $$p; \
	done

# devel-docs/Makefile.am ends here
