package Gimp::UI;

use Carp;
use Gimp;
use Gtk;

$gtk_10 = Gtk->major_version==1 && Gtk->minor_version==0;

$VERSION = $Gimp::VERSION;

=head1 NAME

Gimp::UI - "simulation of libgimpui"

=head1 SYNOPSIS

  use Gimp::UI;

=head1 DESCRIPTION

Due to the braindamaged (read: "unusable") libgimpui API, I had to
reimplement all of it in perl.

=over 4

 $option_menu = new Gimp::UI::ImageMenu;
 $option_menu = new Gimp::UI::LayerMenu;
 $option_menu = new Gimp::UI::ChannelMenu;
 $option_menu = new Gimp::UI::DrawableMenu;
 
 $button = new Gimp::UI::PatternSelect;
 $button = new Gimp::UI::BrushSelect;
 $button = new Gimp::UI::GradientSelect;

=back

=cut

@Gimp::UI::ImageMenu::ISA   =qw(Gimp::UI);
@Gimp::UI::LayerMenu::ISA   =qw(Gimp::UI);
@Gimp::UI::ChannelMenu::ISA =qw(Gimp::UI);
@Gimp::UI::DrawableMenu::ISA=qw(Gimp::UI);

sub Gimp::UI::ImageMenu::_items {
  map [[$_],$_,$_->get_filename],
      Gimp->list_images ();
}
sub Gimp::UI::LayerMenu::_items {
  map { my $i = $_; map [[$i,$_],$_,$i->get_filename."/".$_->get_name],$i->get_layers }
      Gimp->list_images ();
}

sub Gimp::UI::ChannelMenu::_items {
  map { my $i = $_; map [[$i,$_],$_,$i->get_filename."/".$_->get_name],$i->get_channels }
      Gimp->list_images ();
}

sub Gimp::UI::DrawableMenu::_items {
  map { my $i = $_; map [[$i,$_],$_,$i->get_filename."/".$_->get_name],($i->get_layers, $i->get_channels) }
      Gimp->list_images ();
}

sub new($$$$) {
   my($class,$constraint,$active)=@_;
   my(@items)=$class->_items;
   my $menu = new Gtk::Menu;
   for(@items) {
      my($constraints,$result,$name)=@$_;
      next unless $constraint->(@{$constraints});
      my $item = new Gtk::MenuItem $name;
      $item->show;
      $item->signal_connect(activate => sub { $_[3]=$result });
      $menu->append($item);
   }
   if (@items) {
      $_[3]=$items[0]->[1];
   } else {
      my $item = new Gtk::MenuItem "(none)";
      $item->show;
      $menu->append($item);
   }
   $menu;
}

package Gimp::UI::PreviewSelect;

use Gtk;
use base 'Gtk::Button';

# this is an utter HACK for the braindamanged gtk (NOT Gtk!)
sub register_types {
   unless ($once) {
      $once=1;
      register_type Gimp::UI::PreviewSelect;
      register_type Gimp::UI::PatternSelect;
      register_type Gimp::UI::BrushSelect;
      register_type Gimp::UI::GradientSelect;
   }
}

sub GTK_CLASS_INIT {
   my $class = shift;
   add_arg_type $class "active","GtkString",3;
}

sub GTK_OBJECT_SET_ARG {
   my($self,$arg,$id,$value) = @_;
   if ($arg eq "active") {
      my $count;
      
      if (!defined $self->{value} || $value ne $self->{value}) {
         $self->{value}=$value=$self->set_preview($value);
         $self->{label}->set($value);
         $self->{list}->foreach(sub {
            if ($_[0]->children->get eq $value) {
               $self->{list}->select_item($count);
            };
            $count++;
         });
      }
   }
}

sub GTK_OBJECT_GET_ARG {
   my($self,$arg,$id) = @_;
   $self->{label}->get;
}

sub GTK_OBJECT_INIT {
   my $self=shift;
   (my $label = new Gtk::Label "")->show;
   $self->add($label);
   $self->{label}=$label;
   
   my $w = new Gtk::Dialog;
   $w->set_title($self->get_title);
   $w->set_usize(400,300);
   
   (my $h=new Gtk::HBox 0,0)->show;
   $w->vbox->pack_start($h,1,1,0);
   
   (my $preview = $self->new_preview)->show;
   
   (my $s=new Gtk::ScrolledWindow undef,undef)->show;
   $s->set_policy(-automatic, -automatic);
   $s->set_usize(200,300);
   $h->pack_start($s,1,1,0);
   $h->pack_start($preview,1,1,0);
   
   my $l=new Gtk::List;
   $l->set_selection_mode(-single);
   $l->set_selection_mode(-browse);
   $self->{list}=$l;
   
   for($self->get_list) {
      $l->add(new Gtk::ListItem $_);
   }
   
   $l->show_all;
   $l->signal_connect("selection_changed",sub {
      $l->selection and
         $self->set_preview($l->selection->children->get);
   });
   $gtk_10 ? $s->add ($l) : $s->add_with_viewport ($l);
   
   my $button = new Gtk::Button "OK";
   signal_connect $button "clicked", sub {
      hide $w;
      if($l->selection) {
         my $p = $l->selection->children->get;
         $label->set($p);
      }
   };
   $w->action_area->pack_start($button,1,1,0);
   can_default $button 1;
   grab_default $button;
   show $button;
   
   $button = new Gtk::Button "Cancel";
   signal_connect $button "clicked", sub {hide $w};
   $w->action_area->pack_start($button,1,1,0);
   show $button;
   
   $self->signal_connect("clicked",sub {show $w});
}

package Gimp::UI::PatternSelect;

use Gtk;
use base 'Gimp::UI::PreviewSelect';

sub get_title { "Pattern Selection Dialog" }
sub get_list { Gimp->patterns_list }

sub new_preview {
   my $self = shift;
   my $cp = $self->{"color_preview"}=new Gtk::Preview "color";
   my $gp = $self->{"gray_preview"} =new Gtk::Preview "grayscale";
   my $preview = new Gtk::HBox 0,0;
   $preview->add($cp);
   $preview->add($gp);
   $preview;
}

sub set_preview {
   my $self = shift;
   my $value = shift;
   
   my $cp = $self->{"color_preview"};
   my $gp = $self->{"gray_preview"};
   
   my ($name,$w,$h,$bpp,$mask)=Gimp->patterns_get_pattern_data ($value);
   unless (defined $name) {
      $name=Gimp->patterns_get_pattern;
      ($name,$w,$h,$bpp,$mask)=Gimp->patterns_get_pattern_data ($name);
   }
   hide $cp;
   hide $gp;
   my $p = $bpp == 1 ? $gp : $cp;
   show $p;
   $p->size ($w, $h);
   while(--$h) {
     $p->draw_row (substr ($mask, $w*$bpp*$h), 0, $h, $w);
   }
   $p->draw(undef);
   
   $name;
}

sub new {
   Gimp::UI::PreviewSelect::register_types;
   new Gtk::Widget @_;
}

package Gimp::UI::BrushSelect;

use Gtk;
use base 'Gimp::UI::PreviewSelect';

sub get_title { "Brush Selection Dialog" }
sub get_list { Gimp->brushes_list }

sub new_preview {
   my $self=shift;
   $self->{"preview"}=new Gtk::Preview "grayscale";
}

sub set_preview {
   my $self = shift;
   my $value = shift;
   
   my $p = $self->{"preview"};
   
   my ($name,$opacity,$spacing,$mode,$w,$h,$mask)=eval { Gimp->brushes_get_brush_data ($value) };
   if ($@) {
      $name=Gimp->brushes_get_brush;
      ($name,$opacity,$spacing,$mode,$w,$h,$mask)=Gimp->brushes_get_brush_data ($name);
   }
   $mask=pack("C*",@$mask);
   $xor="\xff" x $w;
   hide $p;
   my $l=length($mask);
   $p->size ($w, $h);
   while(--$h) {
     $p->draw_row (substr ($mask, $w*$h) ^ $xor, 0, $h, $w);
   }
   $p->draw(undef);
   show $p;
   
   $name;
}

sub new {
   Gimp::UI::PreviewSelect::register_types;
   new Gtk::Widget @_;
}

package Gimp::UI::GradientSelect;

use Gtk;
use base 'Gimp::UI::PreviewSelect';

sub get_title { "Gradient Selection Dialog" }
sub get_list { keys %gradients }

sub new_preview {
   my $self = shift;
   new Gtk::Frame;
}

sub set_preview {
   my $self = shift;
   my $value = shift;
   exists $gradients{$value} ? $value : Gimp->gradients_get_active;
}

sub new {
   Gimp::UI::PreviewSelect::register_types;
   unless (defined %gradients) {
      undef @gradients{Gimp->gradients_get_list};
   }
   new Gtk::Widget @_;
}

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
