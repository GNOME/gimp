package Gimp::basewidget; # pragma

use Gimp;
use Gtk;

$VERSION = 1.18;

=head1 NAME

Gimp::basewidget - pragma to declare the superclass of a gtk widget

=head1 SYNOPSIS

  use Gimp::basewidget 'superclass';

e.g.

  use Gimp::basewidget Gtk::Button;

=head1 DESCRIPTION

This pragma can (but does not need to) be used to declare the current
package as a childclass of an existing Gtk widget class. The only "import
tag" must be the name of the existing superclass.

The module automatically registers a subtype, calls a special callback
at gtk initialization time and provides default implementations for some
common methods (the list might grow in the future to enhance settor/gettor
functionality).

The following methods are provided. All of them can be overriden in your
package.

=over 4

=item new

A simple generic new constructor is provided. It will simply call
C<Gtk::Object::new> with all the provided arguments.

=item GTK_INIT

This callback is called as early as possible E<after> gtk was initialized,
but not before. This can be used to register additional subtypes, argument
types etc. It is similar to GTK_CLASS_INIT.

=item GTK_CLASS_INIT

Unlike the standard Gtk-callback of the same name, this method can be
omitted in your package (while still being a valid Widget).

=item GTK_OBJECT_INIT

This callback can also be omitted, but this rarely makes sense ;)

=back

=cut

# a generic perl widget helper class, or something that
# was a pain to implement "right". :(

sub GTK_INIT {
   # dummy function, maybe even totally superfluous
}

sub GTK_CLASS_INIT {
   # dummy function
}

sub GTK_OBJECT_INIT {
   # dummy function, should be overriden
}

sub DESTROY {
   # dummy function, very necessary
}

*new = \&Gtk::Object::new;

sub import {
   my $self  = shift;
   my $super = shift;
   my $class = caller;
   push @{$class."::ISA"}, $self, $super;
   Gimp::gtk_init_hook {
      $class->GTK_INIT;
      Gtk::Object::register_subtype($super,$class);
   };
}

1;

=head1 BUGS

This was a pain to implement, you will not believe this when looking at
the code, though.

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>.

=head1 SEE ALSO

perl(1), L<Gimp>, L<Gimp::UI>, L<Gtk>.

