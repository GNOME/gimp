package Gimp::UI;

use Gimp ();
use Gtk;

$VERSION = $Gimp::VERSION;

=head1 NAME

Gimp::UI - "simulation of libgimpui", and more!

=head1 SYNOPSIS

  use Gimp::UI;

=head1 DESCRIPTION

Due to the braindamaged (read: "unusable") libgimpui API, I had to
reimplement all of it in perl.

=over 4

 $option_menu = new Gimp::UI::ImageMenu
 $option_menu = new Gimp::UI::LayerMenu
 $option_menu = new Gimp::UI::ChannelMenu
 $option_menu = new Gimp::UI::DrawableMenu (constraint_func, active_element, \var);
 
 $button = new Gimp::UI::PatternSelect;
 $button = new Gimp::UI::BrushSelect;
 $button = new Gimp::UI::GradientSelect;

 $button = new Gimp::UI::ColorSelectButton;

=back

=cut

@Gimp::UI::ImageMenu::ISA   =qw(Gimp::UI);
@Gimp::UI::LayerMenu::ISA   =qw(Gimp::UI);
@Gimp::UI::ChannelMenu::ISA =qw(Gimp::UI);
@Gimp::UI::DrawableMenu::ISA=qw(Gimp::UI);

sub image_name {
   my $name = $_[0]->get_filename;
   $name.="-".${$_[0]} if $name eq "Untitled";
   $name;
}

sub Gimp::UI::ImageMenu::_items {
  map [[$_],$_,image_name($_)],
      Gimp->list_images ();
}
sub Gimp::UI::LayerMenu::_items {
  map { my $i = $_; map [[$i,$_],$_,image_name($i)."/".$_->get_name],$i->get_layers }
      Gimp->list_images ();
}

sub Gimp::UI::ChannelMenu::_items {
  map { my $i = $_; map [[$i,$_],$_,image_name($i)."/".$_->get_name],$i->get_channels }
      Gimp->list_images ();
}

sub Gimp::UI::DrawableMenu::_items {
  map { my $i = $_; map [[$i,$_],$_,image_name($i)."/".$_->get_name],($i->get_layers, $i->get_channels) }
      Gimp->list_images ();
}

sub new($$$$) {
   my($class,$constraint,$active,$var)=@_;
   my(@items)=$class->_items;
   my $menu = new Gtk::Menu;
   for(@items) {
      my($constraints,$result,$name)=@$_;
      next unless $constraint->(@{$constraints});
      my $item = new Gtk::MenuItem $name;
      $item->signal_connect(activate => sub { $$var=$result });
      $menu->append($item);
   }
   if (@items) {
      $$var=$items[0]->[1];
   } else {
      my $item = new Gtk::MenuItem "(none)";
      $menu->append($item);
      $$var=undef;
   }
   $menu->show_all;
   $menu;
}

package Gimp::UI::PreviewSelect;

use Gtk;
use base 'Gtk::Button';

# this is an utter HACK for the braindamanged gtk (NOT Gtk!)
sub register_types {
   unless ($once) {
      $once=1;
      Gtk::Button->register_subtype(Gimp::UI::PreviewSelect);
      Gtk::Button->register_subtype(Gimp::UI::PatternSelect);
      Gtk::Button->register_subtype(Gimp::UI::BrushSelect);
      Gtk::Button->register_subtype(Gimp::UI::GradientSelect);
      Gtk::Button->register_subtype(Gimp::UI::ColorSelectButton);
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
   
   for(sort $self->get_list) {
      $l->add(new Gtk::ListItem $_);
   }
   
   $l->show_all;
   $l->signal_connect("selection_changed",sub {
      $l->selection and
         $self->set_preview($l->selection->children->get);
   });
   $s->add_with_viewport ($l);
   
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
   can_default $button 1;
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
   $p->size ($w, $h);
   for(0..$h-1) {
      $p->draw_row (substr ($mask, $w*$bpp*$_), 0, $_, $w);
   }
   $p->draw(undef);
   show $p;
   
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
   for(0..$h-1) {
     $p->draw_row (substr ($mask, $w*$_) ^ $xor, 0, $_, $w);
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

package Gimp::UI::ColorSelectButton;

use strict;
use vars qw($VERSION @ISA);
use Gtk;

@ISA = qw(Gtk::Button);

# Class defaults data
my @class_def_color = (255,175,0);

sub GTK_CLASS_INIT {
	my($class) = shift;
	
	if (Gtk->major_version < 1 or (Gtk->major_version == 1 and Gtk->minor_version < 1)) {
		add_arg_type $class "color", "string", 3; #R/W
	} else {
		add_arg_type $class "color", "GtkString", 3; #R/W
	}
}

sub GTK_OBJECT_INIT {
    my (@color) = @class_def_color;
    
    my($color_button) = @_;
    
    $color_button->{_color} ||= [@color];

    my $preview = new Gtk::Preview -color;
    
    $color_button->{_preview} = $preview;
    
    # work around funny bug somewhere in gtk...
    $preview->size(300,50);
    $preview->show;
    $color_button->add($preview);
        
    signal_connect $color_button "size_allocate" => sub {
    	my($self,$allocation) = @_;
    	my($x,$y,$w,$h) = @$allocation;
    	$w -= 6;
    	$h -= 6;
    	$self->{_preview_width} = $w;
    	$self->{_preview_height} = $h;
        $self->{_preview}->size($self->{_preview_width},$self->{_preview_height});
    	$self->update_color;
    };
    
    signal_connect $color_button "clicked" => \&cb_color_button;
}

sub GTK_OBJECT_SET_ARG {
   my($self,$arg,$id, $value) = @_;
   $self->{_color} = [split(' ',$value)];
   $self->update_color;
}

sub GTK_OBJECT_GET_ARG {
   my($self,$arg,$id) = @_;
   return join(' ',@{$self->{_color}});
}

sub update_color($) {
    my($this) = shift;
    
    return unless defined $this->{_preview} and defined $this->{_preview_width};
    
    my($preview, $color) = ($this->{_preview}, $this->{_color});
    my($width, $height) = ($this->{_preview_width}, $this->{_preview_height});
    
    my($buf) = pack("C3", @$color) x $width;

    for(0..$height-1) {
       $preview->draw_row($buf, 0, $_, $width);
    }
    $preview->draw(undef);
}

sub color_selection_ok {
    my($widget, $dialog, $color_button) = @_;
	
    my(@color) = $dialog->colorsel->get_color;
    @{$color_button->{_color}} = map(int(255.99*$_),@color);

    $color_button->update_color();
    $dialog->destroy();
    delete $color_button->{_cs_window};
}

sub cb_color_button {
    my($color_button) = @_;
    
    if (defined $color_button->{_cs_window}) {
    	if (!$color_button->{_cs_window}->mapped) {
	    	$color_button->{_cs_window}->hide;
	    }
    	$color_button->{_cs_window}->show;
    	$color_button->{_cs_window}->window->raise;
    	return;
    }

    my $cs_window=new Gtk::ColorSelectionDialog("Color");
    $cs_window->colorsel->set_color(map($_*1/255,@{$color_button->{_color}}));
    $cs_window->show();
    $cs_window->ok_button->signal_connect("clicked",
					  \&color_selection_ok,
					  $cs_window,
					  $color_button);
    $cs_window->cancel_button->signal_connect("clicked",
					      sub { $cs_window->destroy; delete $color_button->{_cs_window} });
    $color_button->{_cs_window} = $cs_window;
}

sub new {
    my $pkg = shift;
   Gimp::UI::PreviewSelect::register_types;
    return new Gtk::Widget $pkg, @_;
}

1;

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>. The ColorSelectButton code is written by
Dov Grobgeld <dov@imagic.weizmann.ac.il>, with modifications by Kenneth
Albanowski <kjahds@kjahds.com>.

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
