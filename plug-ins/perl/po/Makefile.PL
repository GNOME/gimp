use ExtUtils::MakeMaker;

do '../config.pl';

print "Portable message objects...";
if ($MSGFMT =~ /./) {
   for (<*.po>) {
      my ($po) = /(.*)\.po$/;
      print " $po";
      $install .= "	@-\$(MKPATH) \$(datadir)/locale/$po/LC_MESSAGES\n".
                  "	-\$(MSGFMT) -o \$(datadir)/locale/$po/LC_MESSAGES/gimp-perl.mo $po.po\n".
                  "	@-\$(CHMOD) 644 \$(datadir)/locale/$po/LC_MESSAGES/gimp-perl.mo\n";
      $uninstall .= "	\$(RM_F) \$(datadir)/locale/$po/LC_MESSAGES/gimp-perl.mo\n";
      $updatepo .= "	msgmerge -w 83 $po.po gimp-perl.pot >$po.po~ && mv $po.po~ $po.po\n";
   }
} else {
   print " skipped";
}
print "\n";

WriteMakefile(
    NAME	=> 'i18n translation tables',
    macro	=> \%cfg,
    realclean	=> { FILES => "gimp-perl.pot" },
    clean	=> { FILES => "*.mo" },
);

sub MY::postamble {
   <<XXX
prefix = $prefix
datadir  = $datadir

all :: update-pot

update-po: update-pot
$updatepo

update-pot: ./update.sh
	./update.sh

install :: install-po

uninstall :: uninstall-po

install-po ::
$install

uninstall-po ::
$uninstall

XXX
}
