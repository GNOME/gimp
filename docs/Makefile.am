scriptdata =

EXTRA_DIST = \
	gimp.txt 		pdb_self_doc.el \
	pdb_dump 		xcf.doc \
	cheat_sheet.txt		keybindings.txt	\
	texinfo.tex 		script-fu.tex \
	architecture.eps 	logo.eps \
	net-fu.eps 		timeline.eps \
	gimp-white-paper.tex

info_TEXINFOS = pdb.texi

pdb.texi: $(top_srcdir)/docs/pdb_self_doc.el pdb_dump
	@EMACS@ --batch -l $(top_srcdir)/docs/pdb_self_doc.el -f make-docs-noargs

pdb_dump: $(top_builddir)/app/*.c
	$(top_builddir)/app/gimp --no-interface --batch '(gimp-procedural-db-dump "pdb_dump")' '(gimp-quit 0)'

script-fu.ps: script-fu.dvi
	dvips -f script-fu.dvi > script-fu.ps

script-fu.dvi:
	latex script-fu.tex

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files macros/*; do \
	  echo $$p; \
	done
