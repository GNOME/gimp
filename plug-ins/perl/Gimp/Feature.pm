package Gimp::Feature;

require Exporter;

@ISA=(Exporter);
@EXPORT = ();

my $gtk;

sub _check_gtk {
   unless (defined $gtk) {
      eval { require Gtk }; $gtk = $@ eq "" && $Gtk::VERSION>=0.5;
   }
   $gtk;

}

my %description = (
   'gtk'        => 'the gtk perl module',
   'gtk-1.1'    => 'gtk+ version 1.1 or higher',
   'gtk-1.2'    => 'gtk+ version 1.2 or higher',
   'gtk-1.3'    => 'gtk+ version 1.3 or higher',
   'gtk-1.4'    => 'gtk+ version 1.4 or higher',

   'gimp-1.1'   => 'gimp version 1.1 or higher',
   'gimp-1.2'   => 'gimp version 1.2 or higher',
   'gimp-1.3'   => 'gimp version 1.3 or higher',

   'perl-5.005' => 'perl version 5.005 or higher',
   'pdl'        => 'compiled-in PDL support',
   'gnome'      => 'the gnome perl module',
   'gtkxmhtml'  => 'the Gtk::XmHTML module',
   'dumper'     => 'the Data::Dumper module',
   'never'      => '(for testing, will never be present)',
   'unix'	=> 'a unix-like operating system',
);

sub import {
   my $pkg = shift;
   my $feature;

   local $Gimp::in_query=(@ARGV and $ARGV[0] eq "-gimp");
   while(defined (my $feature = shift)) {
      $feature=~s/^://;
      need($feature);
   }
}

sub describe {
   $description{$_[0]};
}

sub Gimp::Feature::list {
   keys %description;
}

sub present {
   local $_ = shift;

   if ($_ eq "gtk") {
      _check_gtk;
   } elsif ($_ eq "gtk-1.1") {
      _check_gtk and (Gtk->major_version==1 && Gtk->minor_version>=1) || Gtk->major_version>1;
   } elsif ($_ eq "gtk-1.2") {
      _check_gtk and (Gtk->major_version==1 && Gtk->minor_version>=2) || Gtk->major_version>1;
   } elsif ($_ eq "gtk-1.3") {
      _check_gtk and (Gtk->major_version==1 && Gtk->minor_version>=3) || Gtk->major_version>1;
   } elsif ($_ eq "gtk-1.4") {
      _check_gtk and (Gtk->major_version==1 && Gtk->minor_version>=4) || Gtk->major_version>1;

   } elsif ($_ eq "gimp-1.1") {
      (Gimp->major_version==1 && Gimp->minor_version>=1) || Gimp->major_version>1;
   } elsif ($_ eq "gimp-1.2") {
      (Gimp->major_version==1 && Gimp->minor_version>=2) || Gimp->major_version>1;
   } elsif ($_ eq "gimp-1.3") {
      (Gimp->major_version==1 && Gimp->minor_version>=3) || Gimp->major_version>1;

   } elsif ($_ eq "perl-5.005") {
      $] >= 5.005;
   } elsif ($_ eq "pdl") {
      require Gimp::Config; $Gimp::Config{DEFINE1} =~ /HAVE_PDL/;
   } elsif ($_ eq "gnome") {
      eval { require Gnome }; $@ eq "";
   } elsif ($_ eq "gtkxmhtml") {
      eval { require Gtk::XmHTML }; $@ eq "";
   } elsif ($_ eq "dumper") {
      eval { require Data::Dumper }; $@ eq "";
   } elsif ($_ eq "unix") {
      !{
         MacOS		=> 1,
         MSWin32	=> 1,
         dos		=> 1,
         MSDOS		=> 1,
         os2		=> 1,
         VMS		=> 1,
         RISCOS		=> 1,
         AmigaOS	=> 1,
         utwin		=> 1,
       }->{$^O};
   } elsif ($_ eq "never") {
      0;
   } else {
      require Gimp;
      Gimp::logger(message => "unimplemented requirement '$_' (failed)");
      0;
   }
}

sub _missing {
   my ($msg,$function)=@_;
   require Gimp;
   Gimp::logger(message => "$_[0] is required but not found", function => $function);
   Gimp::initialized() ? &Gimp::quiet_die() : exit Gimp::quiet_main();
}

sub missing {
   local $Gimp::in_query=1;
   &_missing;
}

sub need {
   my ($feature,$function)=@_;
   _missing($description{$feature},$function) unless present $feature;
}

1;
__END__

=head1 NAME

Gimp::Features - check for specific features to be present before registering the script.

=head1 SYNOPSIS

  use Gimp::Features;

or

  use Gimp::Features qw(feature1 feature2 ...);

=head1 DESCRIPTION

This module can be used to check for specific features to be present. This
can be used to deny running the script when neccessary features are not
present. While some features can be checked for at any time, the Gimp::Fu
module offers a nicer way to check for them.

=over 4

=item C<gtk>

checks for the presence of the gtk interface module.

=item C<gtk-1.1>, C<gtk-1.2>

checks for the presence of gtk-1.1 (1.2) or higher.

=item C<perl-5.005>

checks for perl version 5.005 or higher.

=item C<pdl>

checks for the presence of a suitable version of PDL (>=1.9906).

=item C<gnome>

checks for the presence of the Gnome-Perl module.

=item C<gtkxmhtl>

checks for the presence of the Gtk::XmHTML module.

=item C<unix>

checks wether the script runs on a unix-like operating system. At the
moment, this is every system except windows, macos, os2 and vms.

=back

The following features can only be checked B<after> C<Gimp->main> has been
called (usually found in the form C<exit main>). See L<Gimp::Fu> on how to
check for these.

=over 4

=item C<gimp-1.1>, C<gimp-1.2>

checks for the presense of gimp in at least version 1.1 (1.2).

=back

=head2 FUNCTIONS

=over 4

=item present(feature)

Checks for the presense of the single feature given as the
argument. Returns true if the feature is present, false otherwise.

=item need(feature,[function-name])

Require a specific feature. If the required feature is not present the
program will exit gracefully, logging an appropriate message. You can
optionally supply a function name to further specify the place where this
feature was missing.

This is the function used when importing symbols from the module.

=item missing(feature-description,[function-name])
   
Indicates that a generic feature (described by the first argument) is
missing. A function name can further be specified. This function will log
the given message and exit gracefully.

=item describe(feature)

Returns a string describing the given feature in more detail, or undef if
there is no description for this feature.

=item list()

Returns a list of features that can be checked for. This list might not be
complete.

=back

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), Gimp(1).

=cut
