EXTRA_DIST = \
	contributors

authorsgen_scripts = \
	authorsgen.pl

stamp-authors: authorsgen.pl contributors Makefile.am
	rootme=`pwd`; \
	cd $(srcdir) && $(PERL) authorsgen.pl \
	&& echo timestamp > $$rootme/stamp-authors

all-local: stamp-authors
