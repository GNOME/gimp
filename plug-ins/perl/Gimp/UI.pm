package Gimp::UI;

use Carp;
use Gimp;
use Gtk;

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

package Gimp::UI::PatternSelect;

use Gtk;

@ISA='Gtk::Button';
register_type Gimp::UI::PatternSelect;

sub GTK_CLASS_INIT {
   my $class = shift;
   
   add_arg_type $class "active","GtkString",3;
}

sub GTK_OBJECT_SET_ARG {
   my($self,$arg,$id,$value) = @_;
   if ($arg eq "active") {
      my $count;
      
      $self->{set_dialog_preview}->($value);
      $value=$self->{dialog_pattern};
      $self->{label}->set($value);
      $self->{list}->foreach(sub {
         if ($_[0]->children->get eq $value) {
            $self->{list}->select_item($count);
         };
         $count++;
      });
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
   $w->set_title("Pattern Selection Dialog");
   $w->set_usize(400,300);
   
   (my $h=new Gtk::HBox 0,0)->show;
   $w->vbox->pack_start($h,1,1,0);
   
   my $cp=new Gtk::Preview "color";
   my $gp=new Gtk::Preview "grayscale";
   
   $self->{set_dialog_preview} = sub {
      my ($name,$w,$h,$bpp,$mask)=Gimp->patterns_get_pattern_data ($_[0]);
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
      
      $self->{dialog_pattern}=$name;
   };
   
   (my $s=new Gtk::ScrolledWindow undef,undef)->show;
   $s->set_policy(-automatic, -automatic);
   $s->set_usize(200,300);
   $h->pack_start($s,1,1,0);
   $h->pack_start($cp,1,1,0);
   $h->pack_start($gp,1,1,0);
   
   my $l=new Gtk::List;
   $l->set_selection_mode(-single);
   $l->set_selection_mode(-browse);
   $self->{list}=$l;
   
   for(Gimp->patterns_list) {
      $l->add(new Gtk::ListItem $_);
   }
   
   $l->show_all;
   $l->signal_connect("selection_changed",sub {
      $l->selection and
         $self->{set_dialog_preview}->($l->selection->children->get);
   });
   $s->add ($l);
   
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

sub new {
   new Gtk::Widget @_;
}

package Gimp::UI::BrushSelect;

use Gtk;

@ISA='Gtk::Button';
register_type Gimp::UI::BrushSelect;

sub GTK_CLASS_INIT {
   my $class = shift;
   
   add_arg_type $class "active","GtkString",3;
}

sub GTK_OBJECT_SET_ARG {
   my($self,$arg,$id,$value) = @_;
   if ($arg eq "active") {
      $arg = Gimp->brushes_get_brush unless exists $self->{brushes}->{$arg};
      $self->{label}->set($arg);
      $self->{list}->select_item($self->{brushes}->{$arg});
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
   $w->set_title("Brush Selection Dialog");
   $w->set_usize(400,300);
   
   (my $h=new Gtk::HBox 0,0)->show;
   
   (my $s=new Gtk::ScrolledWindow undef,undef)->show;
   $s->set_policy(-automatic, -automatic);
   $s->set_usize(200,300);
   $w->vbox->pack_start($s,1,1,0);
   
   my $l=new Gtk::List;
   $l->set_selection_mode(-single);
   $l->set_selection_mode(-browse);
   $self->{list}=$l;
   
   my $count=0;
   
   for(Gimp->brushes_list) {
      $l->add(new Gtk::ListItem $_);
      $self->{brushes}->{$_}=$count++;
   }
   
   my $dialog_selection;
   
   $l->show_all;
   $l->signal_connect("selection_changed",sub {
      $dialog_selection = $l->selection->children->get
         if $l->selection;
   });
   $s->add ($l);
   
   my $button = new Gtk::Button "OK";
   signal_connect $button "clicked", sub {
      hide $w;
      $label->set($dialog_selection)
         if $l->selection;
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

sub new {
   new Gtk::Widget @_;
}

package Gimp::UI::GradientSelect;

use Gtk;

@ISA='Gtk::Button';
register_type Gimp::UI::GradientSelect;

sub GTK_CLASS_INIT {
   my $class = shift;
   
   add_arg_type $class "active","GtkString",3;
}

sub GTK_OBJECT_SET_ARG {
   my($self,$arg,$id,$value) = @_;
   if ($arg eq "active") {
      $arg = Gimp->gradients_get_active unless exists $self->{gradients}->{$arg};
      $self->{label}->set($arg);
      $self->{list}->select_item($self->{gradients}->{$arg});
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
   $w->set_title("Gradient Selection Dialog");
   $w->set_usize(400,300);
   
   (my $h=new Gtk::HBox 0,0)->show;
   
   (my $s=new Gtk::ScrolledWindow undef,undef)->show;
   $s->set_policy(-automatic, -automatic);
   $s->set_usize(200,300);
   $w->vbox->pack_start($s,1,1,0);
   
   my $l=new Gtk::List;
   $l->set_selection_mode(-single);
   $l->set_selection_mode(-browse);
   $self->{list}=$l;
   
   my $count=0;
   
   for(Gimp->gradients_get_list) {
      $l->add(new Gtk::ListItem $_);
      $self->{gradients}->{$_}=$count++;
   }
   
   my $dialog_selection;
   
   $l->show_all;
   $l->signal_connect("selection_changed",sub {
      $dialog_selection = $l->selection->children->get
         if $l->selection;
   });
   $s->add ($l);
   
   my $button = new Gtk::Button "OK";
   signal_connect $button "clicked", sub {
      hide $w;
      $label->set($dialog_selection)
         if $l->selection;
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

sub new {
   new Gtk::Widget @_;
}

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
