package Gimp::UI;

use Gimp ('__');
use Gimp::Fu;
use Gtk;

$VERSION = 1.15;

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
      my $item = new Gtk::MenuItem __"(none)";
      $menu->append($item);
      $$var=undef;
   }
   $menu->show_all;
   $menu;
}

package Gimp::UI::PreviewSelect;

use Gtk;
use Gimp '__';
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
   
   my $button = new Gtk::Button __"OK";
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
   
   $button = new Gtk::Button __"Cancel";
   signal_connect $button "clicked", sub {hide $w};
   $w->action_area->pack_start($button,1,1,0);
   can_default $button 1;
   show $button;
   
   $self->signal_connect("clicked",sub {show $w});
}

package Gimp::UI::PatternSelect;

use Gtk;
use Gimp '__';
use base 'Gimp::UI::PreviewSelect';

sub get_title { __"Pattern Selection Dialog" }
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
use Gimp '__';
use base 'Gimp::UI::PreviewSelect';

sub get_title { __"Brush Selection Dialog" }
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
use Gimp '__';

sub get_title { __"Gradient Selection Dialog" }
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
use Gimp '__';
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

    my $cs_window=new Gtk::ColorSelectionDialog(__"Color");
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

package Gimp::UI;

sub logo {
   &logo_xpm;
}

sub logo_xpm {
   my $window=shift;
   new Gtk::Pixmap(Gtk::Gdk::Pixmap->create_from_xpm_d($window->window,undef,
      #%XPM:logo%
      '79 33 25 1', '  c None', '. c #020204', '+ c #848484', '@ c #444444',
      '# c #C3C3C4', '$ c #252524', '% c #A5A5A4', '& c #646464', '* c #E4E4E4',
      '= c #171718', '- c #989898', '; c #585858', '> c #D7D7D7', ', c #383838',
      '\' c #B8B8B8', ') c #787878', '! c #F7F7F8', '~ c #0B0B0C', '{ c #8C8C8C',
      '] c #4C4C4C', '^ c #CCCCCC', '/ c #2C2C2C', '( c #ABABAC', '_ c #6C6C6C',
      ': c #EBEBEC',
      '                                                                               ',
      '                  ]&@;%                                                        ',
      '     ;]_        ]];{_,&(              ^{__{^    #);^                           ',
      '  ]);;+;)      ,//,@;@@)_           #_......_^  (..;                           ',
      ' ;-\'\'@];@      /$=$/@_@;&          #]........]\' ^..{                           ',
      ' @@_+%-,,]    ,/$///_^)&@;         -...{^>+./(  \'*^!  {{  ##(  ##\'   {{  ##(   ',
      '    ;))@/;  //]);/$]_(\');]        %,..+   ^*!   #/,{ #,/%&..@*&..,^ >,,(;..,^  ',
      '   /,)];]] ,/],+%;_%-#!#()_       \'...> >)_)_))\'\'.._ (..=~...=.~..; ^..=....=> ',
      '   ,]]&;;] /@;->>+-+{(\'\'-+]       #...# #.....=\'\'..) \'..]*\'..$>>../-^..$##,..- ',
      '   @_{@/, @$@_^*>(_;_&;{);\']      \'~..> ^,,/../-\'.._ (..{ ^..; \'=./-^..%  #..& ',
      '   ,&);,& ,])-^:>#%#%+;)>->]       ;..)   >(..; \'..) \'..- #.._ -=./-^..(  ^..& ',
      '   ,&&%]-&/]]_::^\'#--(#!:#:]&      ^...)^#-~..# \'.._ (..% #.._ %=./-^..,>*;..+ ',
      '   ,/&%;{%;//_#^#+%+{%#!:-#%]]      -........{  \'..) \'..% #.._ %=./-^..~....~* ',
      '   ;$@%+)#)@$/-\')%-+-)+^#@;)@,       #@..../\'   #~~) \'~~% #~=_ -/~,-^..)/..=\'  ',
      '    ,@+(\'#);,={)]%^);@;&@=]] ,        %#\'#^(     (%(  (%   %%(  (%% ^..{>###   ',
      '     ,@)^#;,/={)_\'-;///$$=;@ ,,                                     ^..{       ',
      '     ],&)_=$==/])\'+),],,/$)@ @,                %(\'((\'((\'            ^..{       ',
      '       @@]/=====@-)-]$$, ]_/ ,                 %=~~=~==&            >%%^       ',
      '          =$@/@,@]/]$=/  ])$ &       {{{{      %=====~=_      \'-{%             ',
      '          ,$// /$/@ /$,  $,,       %;@,,,;{>   (\'\'\'\'\'\'\'\'      #~.$-            ',
      '          //=/ $,/; $,,   @@       ($......,>                 #~.${            ',
      '          /$,  /,,,  @@   ,,       %$..],...{                 ^~.$-            ',
      '           ],  ]@]   )&    ,       ($..>({..;  #\'+)\'^  ^#\'*>(-!~.${            ',
      '           @,  --    (;    @       %$..^({..] *,..../* ^.._,.$!~.$-            ',
      '           _,  @\'   ;\'     )       %$..@@...)!@.$#(=.; ^..~.~,!~.${            ',
      '           ]/   ])  -      ]       ($......=>^..;--@.~^>...(^#:~.$-            ',
      '           ;     ;-__      ;       ($../,])> %........#>..@(  #~.${            ',
      '           _      )*       ]       %$..>{    \'..->^*>>\'>..;   #~.$-            ',
      '           )      &&+      _       %$..\'     >=.]>>)&^ ^..;   #~.${            ',
      '          ;-     @;];]    &-       ($..\'      \'~.....+ ^..;   #~.$-            ',
      '          \')    ]_& @     __       %{))#       >_@,;\'  >)+(   #+){             ',
      '         &%               @;                                                   ',
      '        ,{_                                                                    '
      #%XPM%
   ))
}

sub _new_adjustment {
   my @adj = eval { @{$_[1]} };

   $adj[2]||=($adj[1]-$adj[0])*0.01;
   $adj[3]||=($adj[1]-$adj[0])*0.01;
   $adj[4]||=0;
   
   new Gtk::Adjustment $_[0],@adj;
}

# find a suitable value for the "digits" value
sub _find_digits {
   my $adj = shift;
   my $digits = log($adj->step_increment || 1)/log(0.1);
   $digits>0 ? int $digits+0.9 : 0;
}

sub help_window(\$$$) {
   my($helpwin,$blurb,$help)=@_;
   unless ($$helpwin) {
      $$helpwin = new Gtk::Dialog;
      $$helpwin->set_title(__("Help for ").$Gimp::function);
      my($font,$b);

      $b = new Gtk::Text;
      $b->set_editable (0);
      $b->set_word_wrap (1);

      $font = load Gtk::Gdk::Font __"9x15bold";
      $font = fontset_load Gtk::Gdk::Font __"-*-courier-medium-r-normal--*-120-*-*-*-*-*" unless $font;
      $font = $b->style->font unless $font;
      my $cs = new Gtk::ScrolledWindow undef,undef;
      $cs->set_policy(-automatic,-automatic);
      $cs->add($b);
      $$helpwin->vbox->add($cs);
      $b->insert($font,$b->style->fg(-normal),undef,__"BLURB:\n\n$blurb\n\nHELP:\n\n$help");
      $b->set_usize($font->string_width('M')*80,($font->ascent+$font->descent)*26);

      my $button = new Gtk::Button __"OK";
      signal_connect $button "clicked",sub { hide $$helpwin };
      $$helpwin->action_area->add($button);
      
      $$helpwin->signal_connect("destroy",sub { undef $$helpwin });

      Gtk->idle_add(sub {
         require Gimp::Pod;
         my $pod = new Gimp::Pod;
         my $text = $pod->format;
         if ($text) {
            $b->insert($font,$b->style->fg(-normal),undef,__"\n\nEMBEDDED POD DOCUMENTATION:\n\n");
            $b->insert($font,$b->style->fg(-normal),undef,$text);
         }
      });
   }

   $$helpwin->show_all();
}

sub interact($$$$@) {
   local $^W=0;
   my($function)=shift;
   my($blurb)=shift;
   my($help)=shift;
   my(@types)=@{shift()};
   my(@getvals,@setvals,@lastvals,@defaults);
   my($button,$box,$bot,$g);
   my($helpwin);
   my $res=0;

   Gimp::init_gtk;

   my $gimp_10 = Gimp->major_version==1 && Gimp->minor_version==0;
   
   for(;;) {
     my $t = new Gtk::Tooltips;
     my $w = new Gtk::Dialog;
     my $accel = new Gtk::AccelGroup;

     $accel->attach($w);

     set_title $w $Gimp::function;
     
     my $h = new Gtk::HBox 0,2;
     $h->add(new Gtk::Label Gimp::wrap_text($blurb,40));
     $w->vbox->pack_start($h,1,1,0);
     realize $w;
     my $l = logo($w);
     $h->add($l);
     
     $g = new Gtk::Table scalar@types,2,0;
     $g->border_width(4);
     $w->vbox->pack_start($g,1,1,0);

     for(@types) {
        my($label,$a);
        my($type,$name,$desc,$default,$extra)=@$_;
        my($value)=shift;
        
        local *new_PF_STRING = sub {
           my $e = new Gtk::Entry;
           set_usize $e 0,25;
           push(@setvals,sub{set_text $e defined $_[0] ? $_[0] : ""});
           #select_region $e 0,1;
           push(@getvals,sub{get_text $e});
           $a=$e;
        };

        if($type == PF_ADJUSTMENT) { # support for scm2perl
           my(@x)=@$default;
           $default=shift @x;
           $type = pop(@x) ? PF_SPINNER : PF_SLIDER;
           $extra=[@x];
        }
        
        $value=$default unless defined $value;
        $label="$name: ";
        
        if($type == PF_INT8		# perl just maps
        || $type == PF_INT16		# all this crap
        || $type == PF_INT32		# into the scalar
        || $type == PF_FLOAT		# domain.
        || $type == PF_STRING) {	# I love it
           &new_PF_STRING;
           
        } elsif($type == PF_FONT) {
           my $fs=new Gtk::FontSelectionDialog __"Font Selection Dialog ($desc)";
           my $def = __"-*-helvetica-medium-r-normal-*-34-*-*-*-p-*-iso8859-1";
           my $val;
           
           my $l=new Gtk::Label "!error!";
           my $setval = sub {
              $val=$_[0];
              unless (defined $val && $fs->set_font_name ($val)) {
                 warn __"Illegal default font description for $function: $val\n" if defined $val;
                 $val=$def;
                 $fs->set_font_name ($val);
              }
              
              my($n,$t)=Gimp::xlfd_size($val);
              $l->set((split(/-/,$val))[2]."\@$n".($t ? "p" : ""));
           };
           
           $fs->ok_button->signal_connect("clicked",sub {$setval->($fs->get_font_name); $fs->hide});
           $fs->cancel_button->signal_connect("clicked",sub {$fs->hide});
           
           push(@setvals,$setval);
           push(@getvals,sub { $val });
           
           $a=new Gtk::Button;
           $a->add($l);
           $a->signal_connect("clicked", sub { show $fs });
           
        } elsif($type == PF_SPINNER) {
           my $adj = _new_adjustment ($value,$extra);
           $a=new Gtk::SpinButton $adj,1,0;
           $a->set_digits (_find_digits $adj);
           $a->set_usize (120,0);
           push(@setvals,sub{$adj->set_value($_[0])});
           push(@getvals,sub{$adj->get_value});
           
        } elsif($type == PF_SLIDER) {
           my $adj = _new_adjustment ($value,$extra);
           $a=new Gtk::HScale $adj;
           $a->set_digits (_find_digits $adj);
           $a->set_usize (120,0);
           push(@setvals,sub{$adj->set_value($_[0])});
           push(@getvals,sub{$adj->get_value});
           
        } elsif($type == PF_COLOR) {
           $a=new Gtk::HBox (0,5);
           my $b=new Gimp::UI::ColorSelectButton -width => 90, -height => 18;
           $a->pack_start ($b,1,1,0);
           $default = [216, 152, 32] unless defined $default;
           push(@setvals,sub{$b->set('color', "@{defined $_[0] ? Gimp::canonicalize_color $_[0] : [216,152,32]}")});
           push(@getvals,sub{[split ' ',$b->get('color')]});
           set_tip $t $b,$desc;
           
           my $c = new Gtk::Button __"FG";
           signal_connect $c "clicked", sub {
             $b->set('color', "@{Gimp::Palette->get_foreground}");
           };
           set_tip $t $c,__"get current foreground colour from the gimp";
           $a->pack_start ($c,1,1,0);
           
           my $d = new Gtk::Button __"BG";
           signal_connect $d "clicked", sub {
             $b->set('color', "@{Gimp::Palette->get_background}");
           };
           set_tip $t $d,__"get current background colour from the gimp";
           $a->pack_start ($d,1,1,0);
           
        } elsif($type == PF_TOGGLE) {
           $a=new Gtk::CheckButton $desc;
           push(@setvals,sub{set_state $a ($_[0] ? 1 : 0)});
           push(@getvals,sub{state $a eq "active"});
           
        } elsif($type == PF_RADIO) {
           my $b = new Gtk::HBox 0,5;
           my($r,$prev);
           my $prev_sub = sub { $r = $_[0] };
           while (@$extra) {
              my $label = shift @$extra;
              my $value = shift @$extra;
              my $radio = new Gtk::RadioButton $label;
              $radio->set_group ($prev) if $prev;
              $b->pack_start ($radio,1,0,5);
              $radio->signal_connect(clicked => sub { $r = $value });
              my $prev_sub_my = $prev_sub;
              $prev_sub = sub { $radio->set_active ($_[0] eq $value); &$prev_sub_my };
              $prev = $radio;
           }
           $a = new Gtk::Frame;
           $a->add($b);
           push(@setvals,$prev_sub);
           push(@getvals,sub{$r});
           
        } elsif($type == PF_IMAGE) {
           my $res;
           $a=new Gtk::HBox (0,5);
           my $b=new Gtk::OptionMenu;
           $b->set_menu(new Gimp::UI::ImageMenu(sub {1},-1,\$res));
           $a->pack_start ($b,1,1,0);
           push(@setvals,sub{});
           push(@getvals,sub{$res});
           set_tip $t $b,$desc;
           
#           my $c = new Gtk::Button "Load";
#           signal_connect $c "clicked", sub {$res = 2; main_quit Gtk};
##           $g->attach($c,1,2,$res,$res+1,{},{},4,2);
#           $a->pack_start ($c,1,1,0);
#           set_tip $t $c,"Load an image into the Gimp";
           
        } elsif($type == PF_LAYER) {
           my $res;
           $a=new Gtk::OptionMenu;
           $a->set_menu(new Gimp::UI::LayerMenu(sub {1},-1,\$res));
           push(@setvals,sub{});
           push(@getvals,sub{$res});
           
        } elsif($type == PF_CHANNEL) {
           my $res;
           $a=new Gtk::OptionMenu;
           $a->set_menu(new Gimp::UI::ChannelMenu(sub {1},-1,\$res));
           push(@setvals,sub{});
           push(@getvals,sub{$res});
           
        } elsif($type == PF_DRAWABLE) {
           my $res=13;
           $a=new Gtk::OptionMenu;
           $a->set_menu(new Gimp::UI::DrawableMenu(sub {1},-1,\$res));
           push(@setvals,sub{});
           push(@getvals,sub{$res});
           
        } elsif($type == PF_PATTERN) {
           if ($gimp_10) {
              &new_PF_STRING;
           } else {
              $a=new Gimp::UI::PatternSelect -active => defined $value ? $value : (Gimp->patterns_get_pattern)[0];
              push(@setvals,sub{$a->set('active',$_[0])});
              push(@getvals,sub{$a->get('active')});
           }
           
        } elsif($type == PF_BRUSH) {
           if ($gimp_10) {
              &new_PF_STRING;
           } else {
              $a=new Gimp::UI::BrushSelect -active =>  defined $value ? $value : (Gimp->brushes_get_brush)[0];
              push(@setvals,sub{$a->set('active',$_[0])});
              push(@getvals,sub{$a->get('active')});
           }
           
        } elsif($type == PF_GRADIENT) {
           if ($gimp_10) {
              &new_PF_STRING;
           } else {
              $a=new Gimp::UI::GradientSelect -active => defined $value ? $value : (Gimp->gimp_gradients_get_active)[0];
              push(@setvals,sub{$a->set('active',$_[0])});
              push(@getvals,sub{$a->get('active')});
           }
           
        } elsif($type == PF_CUSTOM) {
           my (@widget)=&$extra;
           $a=$widget[0];
           push(@setvals,$widget[1]);
           push(@getvals,$widget[2]);
           
        } elsif($type == PF_FILE) {
           &new_PF_STRING;
           my $s = $a;
           $a = new Gtk::HBox 0,5;
           $a->add ($s);
           my $b = new Gtk::Button __"Browse";
           $a->add ($b);
           my $f = new Gtk::FileSelection $desc;
           $b->signal_connect (clicked => sub { $f->set_filename ($s->get_text); $f->show_all });
           $f->ok_button    ->signal_connect (clicked => sub { $f->hide; $s->set_text ($f->get_filename) });
           $f->cancel_button->signal_connect (clicked => sub { $f->hide });
           
        } elsif($type == PF_TEXT) {
           $a = new Gtk::Frame;
           my $h = new Gtk::VBox 0,5;
           $a->add($h);
           my $e = new Gtk::Text;
           my %e;
           %e = $$extra if ref $extra eq "HASH";

           my $sv = sub { 
              my $t = shift,
              $e->delete_text(0,-1);
              $e->insert_text($t,0);
           };
           my $gv = sub {
              $e->get_chars(0,-1);
           };

           $h->add ($e);
           $e->set_editable (1);

           my $buttons = new Gtk::HBox 1,5;
           $h->add($buttons);

           my $load = new Gtk::Button __"Load"; $buttons->add($load);
           my $save = new Gtk::Button __"Save"; $buttons->add($save);
           my $edit = new Gtk::Button __"Edit"; $buttons->add($edit);

           $edit->signal_connect(clicked => sub {
              my $editor = $ENV{EDITOR} || "vi";
              my $tmp = Gimp->temp_name("txt");
              open TMP,">$tmp" or die __"FATAL: unable to create $tmp: $!\n"; print TMP &$gv; close TMP;
              $w->hide;
              main_iteration Gtk;
              system ('xterm','-T',"$editor: $name",'-e',$editor,$tmp);
              $w->show;
              if (open TMP,"<$tmp") {
                 local $/; &$sv(scalar<TMP>); close TMP;
              } else {
                 Gimp->message(__"unable to read temporary file $tmp: $!");
              }
           });

           my $filename = ($e{prefix} || eval { Gimp->directory } || ".") . "/";
           
           my $f = new Gtk::FileSelection __"Fileselector for $name";
           $f->set_filename($filename);
           $f->cancel_button->signal_connect (clicked => sub { $f->hide });
           my $lf =sub {
              $f->hide;
              my $fn = $f->get_filename;
              if(open TMP,"<$fn") {
                 local $/; &$sv(scalar<TMP>);
                 close TMP;
              } else {
                 Gimp->message(__"unable to read '$fn': $!");
              }
           };
           my $sf =sub {
              $f->hide;
              my $fn = $f->get_filename;
              if(open TMP,">$fn") {
                 print TMP &$gv;
                 close TMP;
              } else {
                 Gimp->message(__"unable to create '$fn': $!");
              }
           };
           my $lshandle;
           $load->signal_connect (clicked => sub {
              $f->set_title(__"Load $name");
              $f->ok_button->signal_disconnect($lshandle) if $lshandle;
              $lshandle=$f->ok_button->signal_connect (clicked => $lf);
              $f->show_all;
           });
           $save->signal_connect (clicked => sub {
              $f->set_title(__"Save $name");
              $f->ok_button->signal_disconnect($lshandle) if $lshandle;
              $lshandle=$f->ok_button->signal_connect (clicked => $sf);
              $f->show_all;
           });

           push @setvals,$sv;
           push @getvals,$gv;

        } else {
           $label=__"Unsupported argumenttype $type";
           push(@setvals,sub{});
           push(@getvals,sub{$value});
        }
        
        push(@lastvals,$value);
        push(@defaults,$default);
        $setvals[-1]->($value);
        
        $label=new Gtk::Label $label;
        $label->set_alignment(0,0.5);
        $g->attach($label,0,1,$res,$res+1,{},{},4,2);
        $a && do {
           set_tip $t $a,$desc;
           $g->attach($a,1,2,$res,$res+1,["expand","fill"],["expand","fill"],4,2);
        };
        $res++;
     }
     
     $button = new Gtk::Button __"Help";
     $g->attach($button,0,1,$res,$res+1,{},{},4,2);
     signal_connect $button "clicked", sub { help_window($helpwin,$blurb,$help) };
     
     my $v=new Gtk::HBox 0,5;
     $g->attach($v,1,2,$res,$res+1,{},{},4,2);
     
     $button = new Gtk::Button __"Defaults";
     signal_connect $button "clicked", sub {
       for my $i (0..$#defaults) {
         $setvals[$i]->($defaults[$i]);
       }
     };
     set_tip $t $button,__"Reset all values to their default";
     $v->add($button);
     
     $button = new Gtk::Button __"Previous";
     signal_connect $button "clicked", sub {
       for my $i (0..$#lastvals) {
         $setvals[$i]->($lastvals[$i]);
       }
     };
     $v->add($button);
     set_tip $t $button,__"Restore values to the previous ones";
     
     signal_connect $w "destroy", sub {main_quit Gtk};

     $button = new Gtk::Button __"OK";
     signal_connect $button "clicked", sub {$res = 1; hide $w; main_quit Gtk};
     $w->action_area->pack_start($button,1,1,0);
     can_default $button 1;
     grab_default $button;
     add $accel 0xFF0D, [], [], $button, "clicked";
     
     $button = new Gtk::Button __"Cancel";
     signal_connect $button "clicked", sub {hide $w; main_quit Gtk};
     $w->action_area->pack_start($button,1,1,0);
     can_default $button 1;
     add $accel 0xFF1B, [], [], $button, "clicked";
     
     $res=0;
     
     show_all $w;
     main Gtk;
     #$w->destroy; # buggy in gtk-1.1 (?)
     
     return undef if $res == 0;
     @_ = map {&$_} @getvals;
     return (1,@_) if $res == 1;
#     Gimp->file_load(&Gimp::RUN_INTERACTIVE,"","");
   }
}

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>. The ColorSelectButton code (now
rebundled into the Gtk module) is written by Dov Grobgeld
<dov@imagic.weizmann.ac.il>, with modifications by Kenneth Albanowski
<kjahds@kjahds.com>.

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
