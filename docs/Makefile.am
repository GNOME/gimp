## Process this file with automake to produce Makefile.in

EXTRA_DIST = \
	Wilber.svg			\
	Wilber.xcf.gz			\
	Wilber.xcf.gz.README		\
	Wilber_Construction_Kit.xcf.gz	\
	cheat_sheet.txt			\
	gimp.txt			\
	keybindings.txt 		\
	quick_reference.ps 		\
	quick_reference.tar.gz

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
