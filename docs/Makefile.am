EXTRA_DIST = \
	gimp.txt 		pdb_self_doc.el \
	pdb_dump 		xcf.doc \
	cheat_sheet.txt		keybindings.txt	\
	texinfo.tex 		script-fu.tex \
	architecture.eps 	logo.eps \
	net-fu.eps 		timeline.eps \
	pdb_dump.texi

BUILT_SOURCES = pdb_dump.texi

info_TEXINFOS = pdb.texi
pdb_TEXINFOS = pdb_dump.texi
man_MANS = gimp.1

pdb_dump.texi: $(srcdir)/pdb_self_doc.el $(srcdir)/pdb_dump
	cd $(srcdir) && \
	@EMACS@ --batch -l pdb_self_doc.el -f make-docs-noargs

## use `cp' followed by `rm' since `mv' may not be able to move across mount pts
$(srcdir)/pdb_dump: $(top_srcdir)/app/*.c
	$(top_builddir)/app/gimp --no-interface --batch '(gimp-procedural-db-dump "pdb_dump.tmp")' '(gimp-quit 0)' 
	cp pdb_dump.tmp $(srcdir)/pdb_dump
	-rm -f pdb_dump.tmp

script-fu.ps: script-fu.dvi
	dvips -f script-fu.dvi > script-fu.ps

script-fu.dvi:
	latex script-fu.tex

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files macros/*; do \
	  echo $$p; \
	done
