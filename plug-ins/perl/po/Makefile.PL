use ExtUtils::MakeMaker;

do '../config.pl';

print "Portable message objects...";
if ($MSGFMT =~ /./ && $MSGFMT ne "no") {
   $alltarget = "update-gmo";
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
   $alltarget = "";
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
	-if cmp -s \$*.po~ \$*.po; then rm -f \$*.po~ ; else mv \$*.po~ \$*.po; fi

all :: $alltarget

update-gmo: $mofiles

update-po: update-pot
$updatepo

gimp-perl.pot: update-pot

update-pot: force
	\$(PERL) ../pxgettext `find .. -name '*.pm' -o -name '*.xs' -o -path '../examples/*'` ../Perl-Server > gimp-perl.pot~
	if cmp -s gimp-perl.pot~ gimp-perl.pot; then rm -f gimp-perl.pot~ ; else mv gimp-perl.pot~ gimp-perl.pot; fi

force:

install :: install-po

uninstall :: uninstall-po

install-po ::
$install

uninstall-po ::
$uninstall

clean ::
	test -f Makefile || mv -f Makefile.old Makefile
XXX
}
