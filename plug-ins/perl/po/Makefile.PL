use ExtUtils::MakeMaker;

do '../config.pl';

print "Portable message objects...";
if ($MSGFMT =~ /./) {
   for (<*.po>) {
      my ($po) = /(.*)\.po$/;
      print " $po";
      $install .= "	@-\$(MKPATH) \$(datadir)/locale/$po/LC_MESSAGES\n".
                  "	-\$(CP) $po.gmo \$(datadir)/locale/$po/LC_MESSAGES/gimp-perl.mo\n".
                  "	@-\$(CHMOD) 644 \$(datadir)/locale/$po/LC_MESSAGES/gimp-perl.mo\n";
      $uninstall .= "	\$(RM_F) \$(datadir)/locale/$po/LC_MESSAGES/gimp-perl.mo\n";
      $mofiles   .= " $po.gmo";
   }
} else {
   print " skipped";
}
print "\n";

WriteMakefile(
    NAME	=> 'i18n',
    macro	=> \%cfg,
    realclean	=> { FILES => "gimp-perl.pot" },
    clean	=> { FILES => "*.mo" },
);

sub MY::postamble {
   <<XXX
prefix = $prefix
datadir  = $datadir

%.gmo: %.po
	$MSGFMT -o \$*.gmo \$*.po

%.po: gimp-perl.pot
	-msgmerge -w 83 \$*.po gimp-perl.pot >\$*.po~
	-if cmp -s \$*.po~ \$*.po; then : ; else mv \$*.po~ \$*.po; fi

all :: update-gmo

update-gmo: $mofiles

update-po: update-pot
$updatepo

gimp-perl.pot: update-pot

update-pot: force
	\$(PERL) ../pxgettext `find .. -name '*.pm' -o -name '*.xs' -o -path '../examples/*'` ../Perl-Server > gimp-perl.pot~
	if cmp -s gimp-perl.pot~ gimp-perl.pot; then : ; else mv gimp-perl.pot~ gimp-perl.pot; fi

force:

install :: install-po

uninstall :: uninstall-po

install-po ::
$install

uninstall-po ::
$uninstall

XXX
}
