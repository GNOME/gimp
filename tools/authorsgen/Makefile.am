EXTRA_DIST = \
	contributors

authorsgen_scripts = \
	authorsgen.pl

$(srcdir)/authorsgen.pl: stamp-authors
	@:
stamp-authors: contributors Makefile.am
	rootme=`pwd`; \
	cd $(srcdir) && $(PERL) authorsgen.pl \
	&& echo timestamp > $$rootme/stamp-authors

all-local: stamp-authors
