package Gimp::Fu;

use strict 'vars';
use Carp;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK @EXPORT_FAIL %EXPORT_TAGS
            @scripts @_params $run_mode %pf_type2string @image_params);
use Gimp qw(:param);
use Gimp::UI;
use Gimp::Data;
use File::Basename;
use Gtk;
use Gtk::ColorSelectButton;
use Data::Dumper;

require Exporter;
require DynaLoader;
require AutoLoader;

=cut

=head1 NAME

Gimp::Fu - "easy to use" framework for Gimp scripts

=head1 SYNOPSIS

  use Gimp;
  use Gimp::Fu;
  
  (this module uses Gtk, so make sure it's correctly installed)

=head1 DESCRIPTION

Currently, there are only three functions in this module. This
fully suffices to provide a professional interface and the
ability to run this script from within the Gimp and standalone
from the commandline.

Dov Grobgeld has written an excellent tutorial for Gimp-Perl. While not
finished, it's definitely worth a look! You can find it at
C<http://imagic.weizmann.ac.il/~dov/gimp/perl-tut.html>.

=head1 INTRODUCTION

In general, a Gimp::Fu script looks like this:

   #!/path/to/your/perl
   
   use Gimp;
   use Gimp::Fu;
   
   register <many arguments>, sub {
      your code;
   }
   
   exit main;

(This distribution comes with example scripts. One is
C<examples/example-fu.pl>, which is small Gimp::Fu-script you can take as
starting point for your experiments)

=cut

sub PF_INT8	() { PARAM_INT8		};
sub PF_INT16	() { PARAM_INT16	};
sub PF_INT32	() { PARAM_INT32	};
sub PF_FLOAT	() { PARAM_FLOAT	};
sub PF_STRING	() { PARAM_STRING	};
sub PF_COLOR	() { PARAM_COLOR	};
sub PF_COLOUR	() { PARAM_COLOR	};
sub PF_IMAGE	() { PARAM_IMAGE	};
sub PF_LAYER	() { PARAM_LAYER	};
sub PF_CHANNEL	() { PARAM_CHANNEL	};
sub PF_DRAWABLE	() { PARAM_DRAWABLE	};

sub PF_TOGGLE	() { &PARAM_END+1	};
sub PF_SLIDER	() { &PARAM_END+2	};
sub PF_FONT	() { &PARAM_END+3	};
sub PF_SPINNER	() { &PARAM_END+4	};
sub PF_ADJUSTMENT(){ &PARAM_END+5	}; # compatibility fix for script-fu
sub PF_BRUSH	() { &PARAM_END+6	};
sub PF_PATTERN	() { &PARAM_END+7	};
sub PF_GRADIENT	() { &PARAM_END+8	};

sub PF_BOOL	() { PF_TOGGLE		};
sub PF_INT	() { PF_INT32		};
sub PF_VALUE	() { PF_STRING		};

sub Gimp::RUN_FULLINTERACTIVE { &Gimp::RUN_INTERACTIVE+100 };	# you don't want to know

%pf_type2string = (
         &PF_INT8	=> 'small integer',
         &PF_INT16	=> 'medium integer',
         &PF_INT32	=> 'integer',
         &PF_FLOAT	=> 'value',
         &PF_STRING	=> 'string',
         &PF_BRUSH	=> 'string',
         &PF_GRADIENT	=> 'string',
         &PF_PATTERN	=> 'string',
         &PF_COLOR	=> 'colour',
         &PF_FONT	=> 'XLFD',
         &PF_TOGGLE	=> 'boolean',
         &PF_SLIDER	=> 'integer',
         &PF_SPINNER	=> 'integer',
         &PF_ADJUSTMENT	=> 'integer',
         &PF_IMAGE	=> 'NYI',
         &PF_LAYER	=> 'NYI',
         &PF_CHANNEL	=> 'NYI',
         &PF_DRAWABLE	=> 'NYI',
);

@_params=qw(PF_INT8 PF_INT16 PF_INT32 PF_FLOAT PF_VALUE
            PF_STRING PF_COLOR PF_COLOUR PF_TOGGLE PF_IMAGE
            PF_DRAWABLE PF_FONT PF_LAYER PF_CHANNEL PF_BOOL
            PF_SLIDER PF_INT PF_SPINNER PF_ADJUSTMENT
            PF_BRUSH PF_PATTERN PF_GRADIENT);

@ISA = qw(Exporter);
@EXPORT = (qw(register main gimp_main xlfd_size),@_params);
@EXPORT_OK = qw(interact $run_mode save_image);
%EXPORT_TAGS = (params => [@_params]);

sub import {
   undef *{caller()."::main"};
   undef *{caller()."::gimp_main"};
   goto &Exporter::import;
}

sub _default {
   my $d = shift;
   my @a = @_;
   if (ref $d) {
     @a[0..$#$d] = @{$d};
   } elsif (defined $d) {
     $a[0] = $d;
   }
   @a;
}

sub xlfd_size {
  local $^W=0;
  my ($px,$pt)=(split(/-/,$_[0]))[7,8];
  $px>0 ? ($px    ,&Gimp::PIXELS)
        : ($pt*0.1,&Gimp::POINTS);
}

sub wrap_text {
   my $x=$_[0];
   $x=~s/(\G.{$_[1]}\S*)\s+/$1\n/g;
   $x;
}

sub interact($$$@) {
   local $^W=0;
   my($blurb)=shift;
   my($help)=shift;
   my(@types)=@{shift()};
   my(@getvals,@setvals,@lastvals,@defaults);
   my($w,$t,$button,$box,$bot,$g);
   my $res=0;
   
   init Gtk;
   parse Gtk::Rc Gimp->gtkrc;
   
   for(;;) {
     $t = new Gtk::Tooltips;
     
     $w = new Gtk::Dialog;
     set_title $w "$0";
     
     ($g = new Gtk::Label wrap_text($blurb,40))->show;
     $w->vbox->pack_start($g,1,1,0);
     
     ($g = new Gtk::Table scalar@types,2,0)->show;
     $g->border_width(4);
     $w->vbox->pack_start($g,1,1,0);
     
     for(@types) {
        my($label,$a);
        my($type,$name,$desc,$default,$extra)=@$_;
        my($value)=shift;
        
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
           $a=new Gtk::Entry;
           set_usize $a 0,25;
           push(@setvals,sub{set_text $a defined $_[0] ? $_[0] : ""});
           #select_region $a 0,1;
           push(@getvals,sub{get_text $a});
           
        } elsif($type == PF_FONT) {
           my $fs=new Gtk::FontSelectionDialog "Font Selection Dialog ($desc)";
           my $def = "-*-courier-helvetica-o-normal--34-*-*-*-*-*-*-*";
           my $val;
           
           (my $l=new Gtk::Label "!error!")->show;
           my $setval = sub {
              $val=$_[0];
              unless (defined $val && $fs->set_font_name ($val)) {
                 warn "illegal default font description: $val" if defined $val;
                 $val=$def;
                 $fs->set_font_name ($val);
              }
              
              my($n,$t)=xlfd_size($val);
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
           my $adj = new Gtk::Adjustment $value,_default($extra,0,99,1,5,5);
           $a=new Gtk::SpinButton $adj,1,0;
           $a->set_usize (120,0);
           push(@setvals,sub{$adj->set_value($_[0])});
           push(@getvals,sub{$adj->get_value});
           
        } elsif($type == PF_SLIDER) {
           my $adj = new Gtk::Adjustment $value,_default($extra,0,99,1,1,5);
           $a=new Gtk::HScale $adj;
           $a->set_usize (120,0);
           push(@setvals,sub{$adj->set_value($_[0])});
           push(@getvals,sub{$adj->get_value});
           
        } elsif($type == PF_COLOR) {
           $a=new Gtk::HBox (0,5);
           (my $b=new Gtk::ColorSelectButton -width => 90, -height => 18)->show;
           $a->pack_start ($b,1,1,0);
           $value = [216, 152, 32] unless defined $value;
           push(@setvals,sub{$b->color(join " ",@{Gimp::canonicalize_color $_[0]})});
           push(@getvals,sub{[split ' ',$b->color]});
           set_tip $t $b,$desc;
           
           (my $c = new Gtk::Button "FG")->show;
           signal_connect $c "clicked", sub {
             $b->color(join " ",@{Gimp::Palette->get_foreground});
           };
           set_tip $t $c,"get current foreground colour from the gimp";
           $a->pack_start ($c,1,1,0);
           
           (my $d = new Gtk::Button "BG")->show;
           signal_connect $d "clicked", sub {
             $b->color(join " ",@{Gimp::Palette->get_background});
           };
           set_tip $t $d,"get current background colour from the gimp";
           $a->pack_start ($d,1,1,0);
           
        } elsif($type == PF_TOGGLE) {
           $a=new Gtk::CheckButton $desc;
           push(@setvals,sub{set_state $a ($_[0] ? 1 : 0)});
           push(@getvals,sub{state $a eq "active"});
           
        } elsif($type == PF_IMAGE) {
           my $res;
           $a=new Gtk::HBox (0,5);
           (my $b=new Gtk::OptionMenu)->show;
           $b->set_menu(new Gimp::UI::ImageMenu(sub {1},-1,$res));
           $a->pack_start ($b,1,1,0);
           push(@setvals,sub{});
           push(@getvals,sub{$res});
           set_tip $t $b,$desc;
           
#           (my $c = new Gtk::Button "Load")->show;
#           signal_connect $c "clicked", sub {$res = 2; main_quit Gtk};
#           $g->attach($c,1,2,$res,$res+1,{},{},4,2);
#           $a->pack_start ($c,1,1,0);
#           set_tip $t $c,"Load an image into the Gimp (NYI)";
           
        } elsif($type == PF_LAYER) {
           my $res;
           $a=new Gtk::OptionMenu;
           $a->set_menu(new Gimp::UI::LayerMenu(sub {1},-1,$res));
           push(@setvals,sub{});
           push(@getvals,sub{$res});
           
        } elsif($type == PF_CHANNEL) {
           my $res;
           $a=new Gtk::OptionMenu;
           $a->set_menu(new Gimp::UI::ChannelMenu(sub {1},-1,$res));
           push(@setvals,sub{});
           push(@getvals,sub{$res});
           
        } elsif($type == PF_DRAWABLE) {
           my $res;
           $a=new Gtk::OptionMenu;
           $a->set_menu(new Gimp::UI::DrawableMenu(sub {1},-1,$res));
           push(@setvals,sub{});
           push(@getvals,sub{$res});
           
        } elsif($type == PF_PATTERN) {
           $a=new Gimp::UI::PatternSelect -active => $default;
           push(@setvals,sub{$a->set('active',$default)});
           push(@getvals,sub{$a->get('active')});
           
        } elsif($type == PF_BRUSH) {
           $a=new Gimp::UI::BrushSelect -active => $default;
           push(@setvals,sub{$a->set('active',$default)});
           push(@getvals,sub{$a->get('active')});
           
        } elsif($type == PF_GRADIENT) {
           $a=new Gimp::UI::GradientSelect -active => $default;
           push(@setvals,sub{$a->set('active',$default)});
           push(@getvals,sub{$a->get('active')});
           
        } else {
           $label="Unsupported argumenttype $type";
           push(@setvals,sub{});
           push(@getvals,sub{$value});
        }
        
        push(@lastvals,$value);
        push(@defaults,$default);
        $setvals[-1]->($value);
        
        $label=new Gtk::Label $label;
        $label->set_alignment(0,0.5);
        show $label;
        $g->attach($label,0,1,$res,$res+1,{},{},4,2);
        $a && do {
           set_tip $t $a,$desc;
           show $a;
           $g->attach($a,1,2,$res,$res+1,["expand","fill"],["expand","fill"],4,2);
        };
        $res++;
     }
     
     (my $v=new Gtk::HBox 0,5)->show;
     $g->attach($v,1,2,$res,$res+1,{},{},4,2);
     
     ($button = new Gtk::Button "Defaults")->show;
     signal_connect $button "clicked", sub {
       for my $i (0..$#defaults) {
         $setvals[$i]->($defaults[$i]);
       }
     };
     set_tip $t $button,"Reset all values to their default";
     $v->add($button);
     
     ($button = new Gtk::Button "Previous")->show;
     signal_connect $button "clicked", sub {
       for my $i (0..$#lastvals) {
         $setvals[$i]->($lastvals[$i]);
       }
     };
     $v->add($button);
     set_tip $t $button,"Restore values to the previous ones";
     
     signal_connect $w "destroy", sub {main_quit Gtk};

     $button = new Gtk::Button "OK";
     signal_connect $button "clicked", sub {$res = 1; hide $w; main_quit Gtk};
     $w->action_area->pack_start($button,1,1,0);
     can_default $button 1;
     grab_default $button;
     show $button;
     
     $button = new Gtk::Button "Cancel";
     signal_connect $button "clicked", sub {hide $w; main_quit Gtk};
     $w->action_area->pack_start($button,1,1,0);
     show $button;
     
     $res=0;
     
     show $w;
     main Gtk;
     $w->destroy;
     
     return undef if $res == 0;
     @_ = map {&$_} @getvals;
     return (1,@_) if $res == 1;
     Gimp->file_load(&Gimp::RUN_NONINTERACTIVE,"","");
   }
}

sub this_script {
   return $scripts[0] unless $#scripts;
   # well, not-so-easy-day today
   my $exe = basename($0);
   my @names;
   for my $this (@scripts) {
      my $fun = (split /\//,$this->[0])[-1];
      return $this if lc($exe) eq lc($fun);
      push(@names,$fun);
   }
   die "function '$exe' not found in this script (must be one of ".join(", ",@names).")\n";
}

sub string2pf($$) {
   my($s,$type,$name,$desc)=($_[0],@{$_[1]});
   if($type==PF_STRING
      || $type==PF_FONT
      || $type==PF_PATTERN
      || $type==PF_BRUSH
      || $type==PF_GRADIENT) {
      $s;
   } elsif($type==PF_INT8
           || $type==PF_INT16
           || $type==PF_INT32
           || $type==PF_SLIDER
           || $type==PF_SPINNER
           || $type==PF_ADJUSTMENT) {
      die "$s: not an integer\n" unless $s==int($s);
      $s*1;
   } elsif($type==PF_FLOAT) {
      $s*1;
   } elsif($type==PF_COLOUR) {
      $s=Gimp::canonicalize_colour($s);
   } elsif($type==PF_TOGGLE) {
      $s?1:0;
   } else {
      die "conversion to type $pf_type2string{$type} is not yet implemented\n";
   }
}

# set options read from the command line
my $outputfile;

# mangle argument switches to contain only a-z0-9 and the underscore,
# for easier typing.
sub mangle_key {
   my $key = shift;
   $key=~y/A-Z /a-z_/;
   $key=~y/a-z0-9_//cd;
   $key;
}

sub net {
   no strict 'refs';
   my $this = this_script;
   my(%map,@args);
   my($interact)=1;
   my $params = $this->[8];
   
   # %map is a hash that associates (mangled) parameter names to parameter index
   @map{map mangle_key($_->[1]), @{$params}} = (0..$#{$params});
   
   # Parse the command line
   while(defined($_=shift @ARGV)) {
      if (/^-+(.*)$/) {
	 if($1 eq "i" or $1 eq "interact") {
	   $interact=1e6;
	 } elsif($1 eq "o" or $1 eq "output") {
	   $outputfile=shift @ARGV;
	 } elsif($1 eq "info") {
	   print "no additional information available, use --help\n";
	   exit 0;
	 } else {
           my $arg=shift @ARGV;
	   my $idx=$map{$1};
	   die "$_: illegal switch, try $0 --help\n" unless defined($idx);
	   $args[$idx]=string2pf($arg,$params->[@args]);
	   $interact--;
	 }
      } else {
         push(@args,string2pf($_,$params->[@args]));
	 $interact--;
      }
   }
   
   # Fill in default arguments
   foreach my $i (0..@$params-1) {
       next if defined $args[$i];
       my $entry = $params->[$i];
       $args[$i] = $entry->[3];             # Default value
       die "parameter '$entry->[1]' is not optional\n" unless defined $args[$i] || $interact>0;
   }
   
   # Go for it
   $this->[0]->($interact>0 ? $this->[6]=~/^<Image>/ ? (&Gimp::RUN_FULLINTERACTIVE,undef,undef,@args)
                                                     : (&Gimp::RUN_INTERACTIVE,@args)
                            : (&Gimp::RUN_NONINTERACTIVE,@args));
}

# the <Image> arguments
@image_params = ([&Gimp::PARAM_IMAGE	, "image",	"The image to work on"],
                 [&Gimp::PARAM_DRAWABLE	, "drawable",	"The drawable to work on"]);

sub query {
   my($type);
   for(@scripts) {
      my($function,$blurb,$help,$author,$copyright,$date,
         $menupath,$imagetypes,$params,$results,$code)=@$_;
      
      if ($menupath=~/^<Image>\//) {
         $type=&Gimp::PROC_PLUG_IN;
         unshift(@$params,@image_params);
      } elsif ($menupath=~/^<Toolbox>\//) {
         $type=&Gimp::PROC_EXTENSION;
      } elsif ($menupath=~/^<None>/) {
         $type=&Gimp::PROC_EXTENSION;
      } else {
         die "menupath _must_ start with <Image>, <Toolbox> or <None>!";
      }
      
      unshift(@$params,
              [&Gimp::PARAM_INT32,"run_mode","Interactive, [non-interactive]"]);
      Gimp->gimp_install_procedure($function,$blurb,$help,$author,$copyright,$date,
                                   $menupath,$imagetypes,$type,
                                   [map {
                                      $_->[0]=PARAM_INT32	if $_->[0] == PF_TOGGLE;
                                      $_->[0]=PARAM_INT32	if $_->[0] == PF_SLIDER;
                                      $_->[0]=PARAM_INT32	if $_->[0] == PF_SPINNER;
                                      $_->[0]=PARAM_INT32	if $_->[0] == PF_ADJUSTMENT;
                                      $_->[0]=PARAM_STRING	if $_->[0] == PF_FONT;
                                      $_->[0]=PARAM_STRING	if $_->[0] == PF_BRUSH;
                                      $_->[0]=PARAM_STRING	if $_->[0] == PF_PATTERN;
                                      $_->[0]=PARAM_STRING	if $_->[0] == PF_GRADIENT;
                                      $_;
                                   } @$params],
                                   $results);
   }
}

=cut

=head2 THE REGISTER FUNCTION

   register
     "function_name",
     "blurb", "help",
     "author", "copyright",
     "date",
     "menu path",
     "image types",
     [
       [PF_TYPE,name,desc,optional-default,optional-extra-args],
       [PF_TYPE,name,desc,optional-default,optional-extra-args],
       etc...
     ],
     [
       like above, but for return values
     ],
     sub { code };

=over 2

=item function name

The pdb name of the function, i.e. the name under which is will be
registered in the Gimp database. If it doesn't start with "perl_fu_" it will
be prepended. If you don't want this, prefix your fucntion name with a
single "+". The idea here is that every Gimp::Fu plug-in will be found under
the common C<perl_fu_>-prefix.

=item blurb

A small description of this script/plug-in.

=item help

A help text describing this script. Should be longer and more verbose than
C<blurb>.

=item copyright

The copyright designation for this script. Important! Safe your intellectual
rights!

=item date

The "last modified" time of this script. There is no strict syntax here, but
I recommend ISO format (yyyymmdd or yyyy-mm-dd).

=item menu path

The menu entry Gimp should create. It should start either with <Image>, if
you want an entry in the image menu (the one that opens when clicking into
an image), <Xtns>, for the Xtns menu or <None> for none.

=item image types

The types of images your script will accept. Examples are "RGB", "RGB*",
"GRAY, RGB" etc... Most scripts will want to use "*", meaning "any type".

=item the parameter array

An array ref containing parameter definitions. These are similar to the
parameter definitions used for C<gimp_install_procedure>, but include an
additional B<default> value used when the caller doesn't supply one, and
optional extra arguments describing some types like C<PF_SLIDER>.

Each array element has the form C<[type, name, description, default_value, extra_args]>.

<Image>-type plugins get two additional parameters, image (C<PF_IMAGE>) and
drawable (C<PF_DRAWABLE>). Do not specify these yourself. Also, the
C<run_mode> argument is never given to the script, but its value canm be
accessed in the package-global C<$run_mode>. The B<name> is used in the
dialog box as a hint, the B<description> will be used as a tooltip.

See the section PARAMETER TYPES for the supported types.

=item the return values

This is just like the parameter array, just that it describes the return
values. Of course, default values don't make much sense here. (Even if they
did, it's not implemented anyway..)

=item the code

This is either a anonymous sub declaration (C<sub { your code here; }>, or a
coderef, which is called when the script is run. Arguments (including the
image and drawable for <Image> plug-ins) are supplied automatically.

It is good practise to return an image, if the script creates one, or
C<undef>, since the return value is interpreted by Gimp::Fu (like displaying
the image or writing it to disk). If your script creates multiple pictures,
return an array.

=back

=head2 PARAMETER TYPES

=over 2

=item PF_INT8, PF_INT16, PF_INT32, PF_INT, PF_FLOAT, PF_STRING, PF_VALUE

Are all mapped to a string entry, since perl doesn't really distinguish
between all these datatypes. The reason they exist is to help other scripts
(possibly written in other languages! really!). It's nice to be able to
specify a float as 13.45 instead of "13.45" in C! C<PF_VALUE> is synonymous
to C<PF_STRING>, and <PF_INT> is synonymous to <PF_INT32>.

=item PF_COLOR, PF_COLOUR

Will accept a colour argument. In dialogs, a colour preview will be created
which will open a colour selection box when clicked.

=item PF_IMAGE

A gimp image. Not yet supported in dialogs :(

=item PF_DRAWABLE

A gimp drawable (image, channel or layer). Not yet supported in dialogs :(

=item PF_FONT

An experimental value used to denote fonts. At the moment, this is just
a C<PF_STRING>. It might be replaced by a font selection dialog in the future.

Please note that the Gimp has no value describing a font, so the format of
this string is undefined (and will usually contain only the family name of
the selected font, but in the future it will contain a XLFD).

=item PF_TOGGLE, PF_BOOL

A boolean value (anything perl would accept as true or false). The description
will be used for the toggle-button label!

=item PF_SLIDER

Uses a horizontal scale. To set the range and stepsize, append an array ref
(see Gtk::Adjustment for an explanation) C<[range_min, range_max, step_size,
page_increment, page_size]> as "extra argument" to the description array.
Default values will be substitued for missing entries, like in:

 [PF_SLIDER, "alpha value", "the alpha value", 100, [0, 255, 1] ]

=item PF_SPINNER

The same as PF_SLIDER, except that this one uses a spinbutton instead of a scale.

=item PF_FONT

Lets the user select a font and returns a X Logical Font Descriptor (XLFD).
The default argument, if specified, must be a full XLFD specification, or a
warning will be printed. Please note that the gimp text functions using
these fontnames (gimp_text_..._fontname) ignore the size. You can extract
the size and dimension by using the C<xlfd_size> function.

=item PF_BRUSH, PF_PATTERN, PF_GRADIENT

Lets the user select a brush/pattern/gradient whose name is returned as a
string. The default brush/pattern/gradient-name can be preset.

=back

=cut

sub register($$$$$$$$$;@) {
   no strict 'refs';
   my($function,$blurb,$help,$author,$copyright,$date,
      $menupath,$imagetypes,$params,$results,$code)=@_;
   
   $code or ($results,$code)=([],$results);
   
   $function="perl_fu_".$function unless $function=~/^perl_fu/ || $function=~s/^\+//;
   
   *$function = sub {
      $run_mode=shift;	# global!
      my(@pre,@defaults,@lastvals);
      if ($menupath=~/^<Image>\//) {
         @_ >= 2 or die "<Image> plug-in called without an image and drawable!\n";
         @pre = (shift,shift);
      } elsif ($menupath=~/^<Toolbox>\//) {
         # valid ;)
      } else {
         die "menupath _must_ start with <Image> or <Toolbox>!";
      }
      
      if (@defaults) {
         for (0..$#{$params}) {
	    $params->[$_]->[3]=$defaults[$_];
	 }
      }
      
      # supplement default arguments
      for (0..$#{$params}) {
         $_[$_]=$params->[$_]->[3] unless defined($_[$_]);
      }
      
      if ($run_mode == &Gimp::RUN_INTERACTIVE) {
         if (@_) {
            my $res;
            local $^W=0; # perl -w is braindamaged
            my $VAR1; # Data::Dumper is braindamaged
            # gimp is braindamaged, is doesn't deliver useful values!!
            ($res,@_)=interact($blurb,$help,$params,@{eval $Gimp::Data{"$function/_fu_data"}});
            return unless $res;
         }
      } elsif ($run_mode == &Gimp::RUN_FULLINTERACTIVE) {
         my($res);
         ($res,@_)=interact($blurb,$help,[@image_params,@{$params}],[@pre,@_]);
         undef @pre;
         return unless $res;
      } elsif ($run_mode == &Gimp::RUN_NONINTERACTIVE) {
      } elsif ($run_mode == &Gimp::RUN_WITH_LAST_VALS) {
         @_=@defaults;	# FIXME
      } else {
         die "run_mode must be INTERACTIVE, NONINTERACTIVE or WITH_LAST_VALS\n";
      }
      
      $Gimp::Data{"$function/_fu_data"}=Dumper([@_]);
      
      print $function,"(",join(",",(@pre,@_)),")\n" if $Gimp::verbose;
      
      my @imgs = &$code(@pre,@_);
      
      if (@imgs) {
         for my $i (0..$#imgs) {
            my $img = $imgs[$i];
            next unless defined $img;
            if (ref $img eq "Gimp::Image") {
               if ($outputfile) {
                  my $path = sprintf $outputfile,$i;
                  if ($#imgs and $path eq $outputfile) {
                     $path=~s/\.(?=[^.]*$)/$i./; # insert image number before last dot
                  }
                  print "saving image $path\n" if $Gimp::verbose;
                  save_image($img,$path);
                  $img->delete;
               } elsif ($run_mode != &Gimp::RUN_NONINTERACTIVE) {
                  $img->display_new;
               }
            } elsif (!@$results) {
               warn "WARNING: $function returned something that is not an image: \"$img\"\n";
            }
	 }
      }
      Gimp->displays_flush;
      
      wantarray ? @imgs : $imgs[0];
   };
   push(@scripts,[$function,$blurb,$help,$author,$copyright,$date,
                  $menupath,$imagetypes,$params,$results,$code]);
}

=cut

=head2 MISC. FUNCTIONS

=over

=item C<xlfd_size> fontname

This auxillary functions parses the XLFD (usually obtained from a C<PF_FONT>
parameter) and returns its size and unit (e.g. C<(20,POINTS)>). This can
conviniently used in the gimp_text_..._fontname functions, which ignore the
size (no joke ;). Example:

 $drawable->text_fontname (50, 50, "The quick", 5, 1, xlfd_size $font, $font;

=item C<save_image(img,options_and_path)>

This is the internal function used to save images. As it does more than just
gimp_file_save, I thought it would be handy in other circumstances as well.

The C<img> is the image you want to save (which might get changed during
the operation!), C<options_and_path> denotes the filename and optinal
options. If there are no options, C<save_image> tries to deduce the filetype
from the extension. The syntax for options is

 [IMAGETYPE[OPTIONS...]:]filespec

IMAGETYPE is one of GIF, JPG, JPEG, PNM or PNG, options include

 options valid for all images
 +F	flatten the image (default depends on the image)
 -F	do not flatten the image
 
 options for GIF and PNG images
 +I	do save as interlaced (GIF only)
 -I	do not save as interlaced (default)
 
 options for PNG images
 -Cn	use compression level n

 options for JPEG images
 -Qn	use quality "n" to save file (JPEG only)
 -S	do not smooth (default)
 +S	smooth before saving
 
some examples:

 test.jpg		save the image as a simple jpeg
 JPG:test.jpg		same
 JPG-Q70:test.jpg	the same but force a quality of 70
 GIF-I-F:test.jpg	save a gif image(!) named test.jpg
 			non-inerlaced and without flattening

=back

=cut

sub save_image($$) {
   my($img,$path)=@_;
   my($interlace,$flatten,$quality,$type,$smooth,$compress);
   
   $interlace=0;
   $quality=75;
   $smooth=0;
   $compress=7;
   
   $_=$path=~s/^([^:]+):// ? $1 : "";
   $type=uc($1) if $path=~/\.([^.]+)$/;
   $type=uc($1) if s/^(GIF|JPG|JPEG|PNM|PNG)//i;
   while($_ ne "") {
      $interlace=$1 eq "+", 	next if s/^([-+])[iI]//;
      $flatten=$1 eq "+", 	next if s/^([-+])[fF]//;
      $smooth=$1 eq "+", 	next if s/^([-+])[sS]//;
      $quality=$1,		next if s/^-[qQ](\d+)//;
      $compress=$1,		next if s/^-[cC](\d+)//;
      croak "$_: unknown/illegal file-save option";
   }
   $flatten=(()=$img->get_layers)>1 unless defined $flatten;
   
   $img->flatten if $flatten;
   
   # always save the active layer
   my $layer = $img->get_active_layer;
   
   if ($type eq "JPG" or $type eq "JPEG") {
      Gimp->file_jpeg_save(&Gimp::RUN_NONINTERACTIVE,$img,$layer,$path,$path,$quality,$smooth,1);
   } elsif ($type eq "GIF") {
      Gimp->file_gif_save(&Gimp::RUN_NONINTERACTIVE,$img,$layer,$path,$path,$interlace,0,0,0);
   } elsif ($type eq "PNG") {
      Gimp->file_png_save(&Gimp::RUN_NONINTERACTIVE,$img,$layer,$path,$path,$interlace,$compress);
   } elsif ($type eq "PNM") {
      Gimp->file_pnm_save(&Gimp::RUN_NONINTERACTIVE,$img,$layer,$path,$path,1);
   } else {
      Gimp->gimp_file_save(&Gimp::RUN_NONINTERACTIVE,$img,$layer,$path,$path);
   }
}

# provide some clues ;)
sub print_switches {
   my($this)=@_;
   for(@{$this->[8]}) {
      my $type=$pf_type2string{$_->[0]};
      my $key=mangle_key($_->[1]);
      printf "           -%-25s %s\n","$key $type",$_->[2];
   }
}

*main = *gimp_main = sub {
   if (!@scripts) {
      die "well, there are no scripts registered.. what do you expect?\n";
   } elsif ($Gimp::help) {
      my $this=this_script;
      print <<EOF;
       interface-arguments are
           -o | --output <filespec>   write image to disk, then delete (NYI)
           -i | --interact            let the user edit the values first
       script-arguments are
EOF
      print_switches ($this);
   } else {
      Gimp::main;
   }
};

1;
__END__

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
