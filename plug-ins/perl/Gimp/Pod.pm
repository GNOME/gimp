package Gimp::Pod;

$VERSION=1.14;

sub myqx(&) {
   local $/;
   local *MYQX;
   if (0==open MYQX,"-|") {
      &{$_[0]};
      close STDOUT;
      Gimp::_exit;
   }
   <MYQX>;
}

sub find_converters {
   my $path = eval 'use Config; $Config{installscript}';

   if ($] < 5.00558) {
      $converter{text} = sub { my $pod=shift; require Pod::Text; myqx { Pod::Text::pod2text (-60000,       $pod) } };
      $converter{texta}= sub { my $pod=shift; require Pod::Text; myqx { Pod::Text::pod2text (-60000, '-a', $pod) } };
   } else {
      $converter{text} = sub { qx($path/pod2text $_[0]) } if -x "$path/pod2text" ;
      $converter{texta}= sub { qx($path/pod2text $_[0]) } if -x "$path/pod2text" ;
   }
   $converter{html} = sub { my $pod=shift; require Pod::Html; myqx { Pod::Html::pod2html ($pod) } };
   $converter{man}  = sub { qx($path/pod2man   $_[0]) } if -x "$path/pod2man" ;
   $converter{latex}= sub { qx($path/pod2latex $_[0]) } if -x "$path/pod2latex" ;
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

sub _cache {
   my $self = shift;
   my $fmt = shift;
   if (!$self->{doc}{$fmt} && $converter{$fmt}) {
      local $^W = 0;
      my $doc = $converter{$fmt}->($self->{path});
      undef $doc if $?>>8;
      undef $doc if $doc=~/^[ \t\r\n]*$/;
      $self->{doc}{$fmt}=\$doc;
   }
   $self->{doc}{$fmt};
}

sub format {
   my $self = shift;
   my $fmt = shift || 'text';
   ${$self->_cache($fmt)};
}

sub sections {
   my $self = shift;
   my $doc = $self->_cache('text');
   $$doc =~ /^\S.*$/mg;
}

sub section {
   my $self = shift;
   my $doc = $self->_cache('text');
   if (defined $$doc) {
      ($doc) = $$doc =~ /^$_[0]$(.*?)(?:^[A-Z]|$)/sm;
      if ($doc) {
         $doc =~ y/\r//d;
         $doc =~ s/^\s*\n//;
         $doc =~ s/[ \t\r\n]+$/\n/;
         $doc =~ s/^    //mg;
      }
      $doc;
   } else {
      ();
   }
}

sub author {
   my $self = shift;
   $self->section('AUTHOR');
}

sub blurb {
   my $self = shift;
   $self->section('BLURB') || $self->section('NAME');
}

sub description {
   my $self = shift;
   $self->section('DESCRIPTION');
}

sub copyright {
   my $self = shift;
   $self->section('COPYRIGHT') || $self->section('AUTHOR');
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
  $synopsis = $pod->section ('SYNOPSIS');
  $author = $pod->author;
  @sections = $pod->sections;

=head1 DESCRIPTION

Gimp::Pod can be used to find and parse embedded pod documentation in
gimp-perl scripts.  At the moment only the formatted text can be fetched,
future versions might have more interesting features.

=head1 METHODS

=over 4

=item new

return a new Gimp::Pod object representing the current script or undef, if
an error occured.

=item format([$format])

Returns the embedded pod documentation in the given format, or undef if no
documentation can be found.  Format can be one of 'text', 'html', 'man' or
'latex'. If none is specified, 'text' is assumed.

=item section($header)

Tries to retrieve the section with the header C<$header>. There is no
trailing newline on the returned string, which may be undef in case the
section can't be found.

=item author

=item blurb

=item description

=item copyright

Tries to retrieve fields suitable for calls to the register function.

=item sections

Returns a list of paragraphs found in the pod.

=back

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), Gimp(1),

=cut
