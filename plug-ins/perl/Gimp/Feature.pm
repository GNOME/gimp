package Gimp::Feature;

require Exporter;

@ISA=(Exporter);
@EXPORT = ();

my($gtk,$gtk_10,$gtk_11);

sub _check_gtk {
   return if defined $gtk;

   eval { require Gtk }; $gtk = $@ eq "";

   if ($gtk) {
      $gtk_10 = (Gtk->major_version==1 && Gtk->minor_version==0);
      $gtk_11 = (Gtk->major_version==1 && Gtk->minor_version>=1) || Gtk->major_version>1;
      $gtk_12 = (Gtk->major_version==1 && Gtk->minor_version>=2) || Gtk->major_version>1;
      $gtk_13 = (Gtk->major_version==1 && Gtk->minor_version>=3) || Gtk->major_version>1;
   }
}

my %description = (
   'gtk'        => 'the gtk perl module',
   'gtk-1.1'    => 'gtk+ version 1.1 or higher',
   'gtk-1.2'    => 'gtk+ version 1.2 or higher',
   'gtk-1.3'    => 'gtk+ version 1.3 or higher',
   'gimp-1.1'   => 'gimp version 1.1 or higher',
   'gimp-1.2'   => 'gimp version 1.2 or higher',
   'perl-5.005' => 'perl version 5.005 or higher',
   'pdl'        => 'PDL (the Perl Data Language), version 1.9906 or higher',
   'gnome'      => 'the gnome perl module',
   'gtkxmhtml'  => 'the Gtk::XmHTML module',
   'dumper'     => 'the Data::Dumper module',
   'never'      => '(for testing, will never be present)',
);

sub import {
   my $pkg = shift;
   my $feature;

   while(@_) {
      $_=shift;
      s/^://;
      need($_);
   }
}

sub missing {
   my ($msg,$function)=@_;
   require Gimp;
   Gimp::logger(message => "$_[0] is required but not found", function => $function);
}

sub need {
   my ($feature,$function)=@_;
   unless (present($feature)) {
      missing($description{$feature},$function);
      Gimp::initialized() ? die "BE QUIET ABOUT THIS DIE\n" : exit Gimp::quiet_main();
   }
}

sub describe {
   $description{$_[0]};
}

sub Gimp::Feature::list {
   keys %description;
}

sub present {
   $_ = shift;

   if ($_ eq "gtk") {
      _check_gtk; $gtk;
   } elsif ($_ eq "gtk-1.1") {
      _check_gtk; $gtk_11;
   } elsif ($_ eq "gtk-1.2") {
      _check_gtk; $gtk_12;
   } elsif ($_ eq "gtk-1.3") {
      _check_gtk; $gtk_13;
   } elsif ($_ eq "gimp-1.1") {
      (Gimp->major_version==1 && Gimp->minor_version>=1) || Gimp->major_version>1;
   } elsif ($_ eq "gimp-1.2") {
      (Gimp->major_version==1 && Gimp->minor_version>=2) || Gimp->major_version>1;
   } elsif ($_ eq "perl-5.005") {
      $] >= 5.005;
   } elsif ($_ eq "pdl") {
      eval { require PDL }; $@ eq "" && $PDL::VERSION>=1.9906;
   } elsif ($_ eq "gnome") {
      eval { require Gnome }; $@ eq "";
   } elsif ($_ eq "gtkxmhtml") {
      eval { require Gtk::XmHTML }; $@ eq "";
   } elsif ($_ eq "dumper") {
      eval { require Data::Dumper }; $@ eq "";
   } elsif ($_ eq "never") {
      0;
   } else {
      require Gimp;
      Gimp::logger(message => "unimplemented requirement '$_' (failed)", fatal => 1);
      0;
   }
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

=item need(feature,[function-name])

=item describe(feature)

=item missing(feature-description,[function-name])

=item list()

=back

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), Gimp(1).

=cut
