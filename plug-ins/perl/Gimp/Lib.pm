package Gimp::Lib;

use strict;
use vars qw($VERSION @ISA);
use base qw(DynaLoader);

require DynaLoader;

$VERSION = 1.15;

use subs qw(
	gimp_call_procedure		gimp_main	gimp_init
	_gimp_procedure_available	set_trace	gimp_end
        initialized
);

sub gimp_init {
   Gimp::croak Gimp::_("gimp_init not implemented for the Lib interface");
}

sub gimp_end {
   Gimp::croak Gimp::_("gimp_end not implemented for in the Lib interface");
}

sub lock {
   # unimplemented, ignored
}

sub unlock {
   # unimplemented, ignored
}

sub import {}

bootstrap Gimp::Lib $VERSION;

# functions to "autobless" where the autobless mechanism
# does not work.

sub gimp_image_list {
   map _autobless($_,&Gimp::PARAM_IMAGE),gimp_call_procedure "gimp_image_list";
}

sub gimp_image_get_layers {
   map _autobless($_,&Gimp::PARAM_LAYER),gimp_call_procedure "gimp_image_get_layers",@_;
}

sub gimp_image_get_channels {
   map _autobless($_,&Gimp::PARAM_CHANNEL),gimp_call_procedure "gimp_image_get_channels",@_;
}

# "server-side" perl code evaluation
sub server_eval {
   my @res = eval shift;
   die $@ if $@;
   @res;
}

# this is here to be atomic over the perl-server
sub _gimp_append_data($$) {
   gimp_set_data ($_[0], gimp_get_data ($_[0]) . $_[1]);
}

# convinience functions
sub gimp_drawable_pixel_rgn($$$$$$) {
   Gimp::gimp_pixel_rgn_init(@_);
}

sub gimp_progress_init {
   if (@_<2) {
      goto &_gimp_progress_init;
   } else {
      eval { gimp_call_procedure "gimp_progress_init",@_ };
      gimp_call_procedure "gimp_progress_init",shift if $@;
   }
}

1;
__END__

=head1 NAME

Gimp::Lib - Interface to libgimp (as opposed to Gimp::Net)

=head1 SYNOPSIS

  use Gimp; # internal use only

=head1 DESCRIPTION

This is the package that interfaces to The Gimp via the libgimp interface,
i.e. the normal interface to use with the Gimp. You don't normally use this
module directly, look at the documentation for the package "Gimp".

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
