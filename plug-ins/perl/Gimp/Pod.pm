package Gimp::Pod;

use Carp;
use Config;

$VERSION=$Gimp::VERSION;

sub find_converters {
   my $path = $Config{installscript};

   $converter{text}="$path/pod2text" if -x "$path/pod2text";
   $converter{html}="$path/pod2html" if -x "$path/pod2html";
   $converter{man} ="$path/pod2man"  if -x "$path/pod2man" ;
}

sub find {
   -f $0 ? $0 : ();
}

sub new {
   my $pkg = shift;
   my $self={};
   return () unless defined($self->{path}=find);
   bless $self, $pkg;
}

sub cache_doc {
   my $self = shift;
   my $fmt = shift;
   if (!$self->{doc}{$fmt} && $converter{$fmt}) {
      my $doc = qx($converter{$fmt} $self->{path});
      undef $doc if $?>>8;
      $self->{doc}{$fmt}=$doc;
   }
   $self->{doc}{$fmt};
}

sub format {
   my $self = shift;
   my $fmt = shift || 'text';
   $self->cache_doc($fmt);
}

find_converters;

1;
__END__

=head1 NAME

Gimp::Pod - Evaluate pod documentation embedded in scripts.

=head1 SYNOPSIS

  use Gimp::Pod;

  $pod = new Gimp::Pod;
  $text = $pod->format ();
  $html = $pod->format ('html');

=head1 DESCRIPTION

Gimp::Pod can be used to find and parse embedded pod documentation in
gimp-perl scripts.  At the moment only the formatted text can be fetched,
future versions might have more interesting features.

=head1 METHODS

=over 4

=item new

return a new pod object representing the current script or undef, if an
error occured.

=item format [format]

=back

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), Gimp(1),

=cut
