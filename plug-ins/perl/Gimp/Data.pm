package Gimp::Data;

use strict;
use Carp;
use Tie::Hash;
use Gimp qw();
use Exporter ();
use vars qw(@EXPORT @ISA);

@ISA = qw(Tie::StdHash Exporter);
@EXPORT = qw();

sub FETCH {
    Gimp->gimp_get_data ($_[1]);
}
     
sub STORE {
    Gimp->gimp_set_data ($_[1], $_[2]);
}
     
tie (%Gimp::Data, 'Gimp::Data');

1;
__END__

=head1 NAME

Gimp::Data - Set and get state data.

=head1 SYNOPSIS

  use Gimp::Data;
  
  $Gimp::Data{'value1'} = "Hello";
  print $Gimp::Data{'value1'},", World!!\n";

=head1 DESCRIPTION

With this module, you can access plugin-specific (or global) data in Gimp,
i.e. you can store and retrieve values that are stored in the main Gimp
application.

An example would be to save parameter values in Gimp, so that on subsequent
invocations of your plug-in, the user does not have to set all parameter
values again (L<Gimp::Fu> does this already).

=head1 %Gimp::Data

You can store and retrieve anything you like in this hash. It's contents
will automatically be stored in Gimp, and can be accessed in later invocations
of your plug-in. Be aware that other plug-ins store data in the same "hash", so
better prefix your key with something unique, like your plug-in's name.

=head1 LIMITATIONS

You cannot store references, and you cannot iterate through
the keys (with C<keys>, C<values> or C<each>).

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
