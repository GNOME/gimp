=head1 NAME

 Gimp::Module - run scripts embedded into the Gimp program.

=head1 SYNOPSIS

 not anything you would expect - and not documented, even!

=head1 DESCRIPTION

=head1 FUNCTIONS

=over 4

=cut

package Gimp::Module;

use base qw(DynaLoader);
require DynaLoader;

$VERSION=0.00;

bootstrap Gimp::Module;

#use Gtk;
#init Gtk;

sub _init {
#   my $w = new Gtk::Window;
#   my $l = new Gtk::Label "Test";
#   $w->add($l);
#
#   $w->show_all;
#   Gtk->main_iteration while Gtk::Gdk->events_pending;
#   Gtk::Gdk->flush;
#   $w->destroy;

   &GIMP_MODULE_OK;
}

sub _deinit {
}

1;

=pod

=back

=head1 SEE ALSO

L<Gimp>.

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>
