package Gimp::Fu;

use Carp;
use Gimp ();
use Gimp::Data;
use File::Basename;

require Exporter;

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

Attention: at the moment it's neccessary to always import the C<Gimp::Fu>
module after the C<Gimp> module.

=cut

sub PF_INT8	() { Gimp::PARAM_INT8	};
sub PF_INT16	() { Gimp::PARAM_INT16	};
sub PF_INT32	() { Gimp::PARAM_INT32	};
sub PF_FLOAT	() { Gimp::PARAM_FLOAT	};
sub PF_STRING	() { Gimp::PARAM_STRING	};
sub PF_COLOR	() { Gimp::PARAM_COLOR	};
sub PF_COLOUR	() { Gimp::PARAM_COLOR	};
sub PF_IMAGE	() { Gimp::PARAM_IMAGE	};
sub PF_LAYER	() { Gimp::PARAM_LAYER	};
sub PF_CHANNEL	() { Gimp::PARAM_CHANNEL};
sub PF_DRAWABLE	() { Gimp::PARAM_DRAWABLE};

sub PF_TOGGLE	() { Gimp::PARAM_END+1	};
sub PF_SLIDER	() { Gimp::PARAM_END+2	};
sub PF_FONT	() { Gimp::PARAM_END+3	};
sub PF_SPINNER	() { Gimp::PARAM_END+4	};
sub PF_ADJUSTMENT(){ Gimp::PARAM_END+5	}; # compatibility fix for script-fu _ONLY_
sub PF_BRUSH	() { Gimp::PARAM_END+6	};
sub PF_PATTERN	() { Gimp::PARAM_END+7	};
sub PF_GRADIENT	() { Gimp::PARAM_END+8	};
sub PF_RADIO	() { Gimp::PARAM_END+9	};
sub PF_CUSTOM	() { Gimp::PARAM_END+10	};
sub PF_FILE	() { Gimp::PARAM_END+11	};
sub PF_TEXT	() { Gimp::PARAM_END+12	};

sub PF_BOOL	() { PF_TOGGLE		};
sub PF_INT	() { PF_INT32		};
sub PF_VALUE	() { PF_STRING		};

sub Gimp::RUN_FULLINTERACTIVE (){ Gimp::RUN_INTERACTIVE+100 };	# you don't want to know

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
         &PF_RADIO	=> 'string',
         &PF_CUSTOM	=> 'string',
         &PF_FILE	=> 'string',
         &PF_TEXT	=> 'string',
         &PF_IMAGE	=> 'NYI',
         &PF_LAYER	=> 'NYI',
         &PF_CHANNEL	=> 'NYI',
         &PF_DRAWABLE	=> 'NYI',
);

@_params=qw(PF_INT8 PF_INT16 PF_INT32 PF_FLOAT PF_VALUE PF_STRING PF_COLOR
            PF_COLOUR PF_TOGGLE PF_IMAGE PF_DRAWABLE PF_FONT PF_LAYER
            PF_CHANNEL PF_BOOL PF_SLIDER PF_INT PF_SPINNER PF_ADJUSTMENT
            PF_BRUSH PF_PATTERN PF_GRADIENT PF_RADIO PF_CUSTOM PF_FILE
            PF_TEXT);

#@EXPORT_OK = qw(interact $run_mode save_image);

sub import {
   local $^W=0;
   my $up = caller;
   shift;
   @_ = (qw(register main),@_params) unless @_;
   for (@_) {
      if ($_ eq ":params") {
         push (@_, @_params);
      } else {
         *{"${up}::$_"} = \&$_;
      }
   }
}

# expand all the pod directives in string (currently they are only removed)
sub expand_podsections() {
   my $pod;
   for (@scripts) {
      $_->[2] ||= "=pod(NAME)";
      $_->[3] ||= "=pod(HELP)";
      $_->[4] ||= "=pod(AUTHOR)";
      $_->[5] ||= "=pod(AUTHOR)";
      $_->[6] ||= "=pod(DATE)";

      for (@{$_}[2,3,4,5,6]) {
         s/=pod\(([^)]*)\)/
            require Gimp::Pod;
            $pod ||= new Gimp::Pod;
            $pod->section($1) || $pod->format;
         /eg;
      }
   }
}

# the old value of the trace flag
my $old_trace;

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
      $$helpwin->set_title("Help for ".$Gimp::function);
      my($font,$b);

      $b = new Gtk::Text;
      $b->set_editable (0);
      $b->set_word_wrap (1);

      $font = load Gtk::Gdk::Font "9x15bold";
      $font = fontset_load Gtk::Gdk::Font "-*-courier-medium-r-normal--*-120-*-*-*-*-*" unless $font;
      $font = $b->style->font unless $font;
      $$helpwin->vbox->add($b);
      $b->realize; # for gtk-1.0
      $b->insert($font,$b->style->fg(-normal),undef,"BLURB:\n\n$blurb\n\nHELP:\n\n$help");
      $b->set_usize($font->string_width('M')*80,($font->ascent+$font->descent)*26);

      my $button = new Gtk::Button "OK";
      signal_connect $button "clicked",sub { hide $$helpwin };
      $$helpwin->action_area->add($button);
      
      $$helpwin->signal_connect("destroy",sub { undef $$helpwin });

      Gtk->idle_add(sub {
         require Gimp::Pod;
         my $pod = new Gimp::Pod;
         my $text = $pod->format;
         if ($text) {
            $b->insert($font,$b->style->fg(-normal),undef,"\n\nEMBEDDED POD DOCUMENTATION:\n\n");
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

   # only pull these in if _really_ required
   # gets us some speed we really need
   eval { require Gtk };
   
   if ($@) {
      my @res = map {
         die "the gtk perl module is required to run\nthis plug-in in interactive mode\n" unless defined $_->[3];
         $_->[3];
      } @types;
      Gimp::logger(message => "the gtk perl module is required to open a dialog\nwindow, running with default values",
                   fatal => 1, function => $function);
      return (1,@res);
   }

   Gimp::init_gtk;

   require Gimp::UI; import Gimp::UI;

   my $gimp_10 = Gimp->major_version==1 && Gimp->minor_version==0;
   
   for(;;) {
     my $t = new Gtk::Tooltips;
     my $w = new Gtk::Dialog;

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
           my $fs=new Gtk::FontSelectionDialog "Font Selection Dialog ($desc)";
           my $def = "-*-helvetica-medium-r-normal-*-24-*-*-*-p-*-iso8859-1";
           my $val;
           
           my $l=new Gtk::Label "!error!";
           my $setval = sub {
              $val=$_[0];
              unless (defined $val && $fs->set_font_name ($val)) {
                 warn "illegal default font description for $function: $val\n" if defined $val;
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
           
           my $c = new Gtk::Button "FG";
           signal_connect $c "clicked", sub {
             $b->set('color', "@{Gimp::Palette->get_foreground}");
           };
           set_tip $t $c,"get current foreground colour from the gimp";
           $a->pack_start ($c,1,1,0);
           
           my $d = new Gtk::Button "BG";
           signal_connect $d "clicked", sub {
             $b->set('color', "@{Gimp::Palette->get_background}");
           };
           set_tip $t $d,"get current background colour from the gimp";
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
              $prev_sub = sub { $radio->set_active ($_[0] == $value); &$prev_sub_my };
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
              $a=new Gimp::UI::PatternSelect -active => $default;
              push(@setvals,sub{$a->set('active',$default)});
              push(@getvals,sub{$a->get('active')});
           }
           
        } elsif($type == PF_BRUSH) {
           if ($gimp_10) {
              &new_PF_STRING;
           } else {
              $a=new Gimp::UI::BrushSelect -active => $default;
              push(@setvals,sub{$a->set('active',$default)});
              push(@getvals,sub{$a->get('active')});
           }
           
        } elsif($type == PF_GRADIENT) {
           if ($gimp_10) {
              &new_PF_STRING;
           } else {
              $a=new Gimp::UI::GradientSelect -active => $default;
              push(@setvals,sub{$a->set('active',$default)});
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
           my $b = new Gtk::Button "Browse";
           $a->add ($b);
           my $f = new Gtk::FileSelection $desc;
           $b->signal_connect (clicked => sub { $f->set_filename ($s->get_text); $f->show_all });
           $f->ok_button    ->signal_connect (clicked => sub { $f->hide; $s->set_text ($f->get_filename) });
           $f->cancel_button->signal_connect (clicked => sub { $f->hide });
           
        } elsif($type == PF_TEXT) {
           $a = new Gtk::HBox 0,5;
           my $e = new Gtk::Text;
           $a->add ($e);
           $e->set_editable (1);

           push @setvals, sub { 
              $e->delete_text(0,-1);
              $e->insert_text($_[0],0);
           };
           push @getvals, sub {
              $e->get_chars(0,-1);
           };

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
        $g->attach($label,0,1,$res,$res+1,{},{},4,2);
        $a && do {
           set_tip $t $a,$desc;
           $g->attach($a,1,2,$res,$res+1,["expand","fill"],["expand","fill"],4,2);
        };
        $res++;
     }
     
     $button = new Gtk::Button "Help";
     $g->attach($button,0,1,$res,$res+1,{},{},4,2);
     signal_connect $button "clicked", sub { help_window($helpwin,$blurb,$help) };
     
     my $v=new Gtk::HBox 0,5;
     $g->attach($v,1,2,$res,$res+1,{},{},4,2);
     
     $button = new Gtk::Button "Defaults";
     signal_connect $button "clicked", sub {
       for my $i (0..$#defaults) {
         $setvals[$i]->($defaults[$i]);
       }
     };
     set_tip $t $button,"Reset all values to their default";
     $v->add($button);
     
     $button = new Gtk::Button "Previous";
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
     
     $button = new Gtk::Button "Cancel";
     signal_connect $button "clicked", sub {hide $w; main_quit Gtk};
     $w->action_area->pack_start($button,1,1,0);
     can_default $button 1;
     
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

sub fu_feature_present($$) {
   my ($feature,$function)=@_;
   require Gimp::Feature;
   if (Gimp::Feature::present($feature)) {
      1;
   } else {
      Gimp::Feature::missing(Gimp::Feature::describe($feature),$function);
      0;
   }
}

sub this_script {
   return $scripts[0] unless $#scripts;
   # well, not-so-easy-day today
   require File::Basename;
   my $exe = File::Basename::basename($0);
   my @names;
   for my $this (@scripts) {
      my $fun = (split /\//,$this->[1])[-1];
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
      || $type==PF_CUSTOM
      || $type==PF_FILE
      || $type==PF_TEXT
      || $type==PF_RADIO	# for now! #d#
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
   my $params = $this->[9];
   
   for(@{$this->[11]}) {
      return unless fu_feature_present($_,$this->[1]);
   }

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
	   $args[$idx]=string2pf($arg,$params->[$idx]);
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
   $this->[0]->($interact>0 ? $this->[7]=~/^<Image>/ ? (&Gimp::RUN_FULLINTERACTIVE,undef,undef,@args)
                                                     : (&Gimp::RUN_INTERACTIVE,@args)
                            : (&Gimp::RUN_NONINTERACTIVE,@args));
}

# the <Image> arguments
@image_params = ([&Gimp::PARAM_IMAGE	, "image",	"The image to work on"],
                 [&Gimp::PARAM_DRAWABLE	, "drawable",	"The drawable to work on"]);

sub query {
   my($type);
   expand_podsections;
   script:
   for(@scripts) {
      my($perl_sub,$function,$blurb,$help,$author,$copyright,$date,
         $menupath,$imagetypes,$params,$results,$features,$code)=@$_;

      for(@$features) {
         next script unless fu_feature_present($_,$function);
      }

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
                                      $_->[0]=Gimp::PARAM_INT32		if $_->[0] == PF_TOGGLE;
                                      $_->[0]=Gimp::PARAM_INT32		if $_->[0] == PF_SLIDER;
                                      $_->[0]=Gimp::PARAM_INT32		if $_->[0] == PF_SPINNER;
                                      $_->[0]=Gimp::PARAM_INT32		if $_->[0] == PF_ADJUSTMENT;
                                      $_->[0]=Gimp::PARAM_INT32		if $_->[0] == PF_RADIO;
                                      $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_FONT;
                                      $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_BRUSH;
                                      $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_PATTERN;
                                      $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_GRADIENT;
                                      $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_CUSTOM;
                                      $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_FILE;
                                      $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_TEXT;
                                      $_;
                                   } @$params],
                                   $results);

      Gimp::logger(message => 'OK', function => $function, fatal => 0);
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
       # etc...
     ],
     [
       # like above, but for return values (optional)
     ],
     ['feature1', 'feature2'...], # optionally check for features
     sub { code };

=over 2

=item function name

The pdb name of the function, i.e. the name under which is will be
registered in the Gimp database. If it doesn't start with "perl_fu_",
"plug_in_" or "extension_", it will be prepended. If you don't want this,
prefix your function name with a single "+". The idea here is that every
Gimp::Fu plug-in will be found under the common C<perl_fu_>-prefix.

=item blurb

A small description of this script/plug-in. Defaults to "=pod(NAME)" (see
the section on EMBEDDED POD DOCUMENTATION for an explanation of this
string).

=item help

A help text describing this script. Should be longer and more verbose than
C<blurb>. Default is "=pod(HELP)".

=item author

The name (and also the e-mail address if possible!) of the
script-author. Default is "=pod(AUTHOR)".

=item copyright

The copyright designation for this script. Important! Safe your intellectual
rights! The default is "=pod(AUTHOR)".

=item date

The "last modified" time of this script. There is no strict syntax here, but
I recommend ISO format (yyyymmdd or yyyy-mm-dd). Default value is "=pod(DATE)".

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
did, it's not implemented anyway..). This argument is optional.

=item the features requirements

See L<Gimp::Features> for a description of which features can be checked
for. This argument is optional (but remember to specify an empty return
value array, C<[]>, if you want to specify it).

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

A gimp image.

=item PF_DRAWABLE

A gimp drawable (image, channel or layer).

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

=item PF_RADIO

In addition to a default value, an extra argument describing the various
options I<must> be provided. That extra argument must be a reference
to an array filled with C<Option-Name => Option-Value> pairs. Gimp::Fu
will then generate a horizontal frame with radio buttons, one for each
alternative. For example:

 [PF_RADIO, "direction", "the direction to move to", 5, [Left => 5,  Right => 7]]]

draws two buttons, when the first (the default, "Left") is activated, 5
will be returned. If the second is activated, 7 is returned.

=item PF_FONT

Lets the user select a font and returns a X Logical Font Descriptor (XLFD).
The default argument, if specified, must be a full XLFD specification, or a
warning will be printed. Please note that the gimp text functions using
these fontnames (gimp_text_..._fontname) ignore the size. You can extract
the size and dimension by using the C<xlfd_size> function.

In older Gimp-Versions a user-supplied string is returned.

=item PF_BRUSH, PF_PATTERN, PF_GRADIENT

Lets the user select a brush/pattern/gradient whose name is returned as a
string. The default brush/pattern/gradient-name can be preset.

=item PF_CUSTOM

PF_CUSTOM is for those of you requiring some non-standard-widget. You have
to supply a code reference returning three values as the extra argument:

 (widget, settor, gettor)

C<widget> is Gtk widget that should be used.

C<settor> is a function that takes a single argument, the new value for
the widget (the widget should be updated accordingly).

C<gettor> is a function that should return the current value of the widget.

While the values can be of any type (as long as it fits into a scalar),
you should be prepared to get a string when the script is started from the
commandline or via the PDB.

=item PF_FILE

This represents a file system object. It usually is a file, but can be
anything (directory, link). It might not even exist at all.

=item PF_TEXT

Similar to PF_STRING, but the entry widget is much larger and has Load and
Save buttons.

=back

=head2 EMBEDDED POD DOCUMENTATION

The register functions expects strings (actually scalars) for
documentation, and nobody wants to embed long parts of documentation into
a string, cluttering the whole script.

Therefore, Gimp::Fu utilizes the Gimp::Pod module to display the full text
of the pod sections that are embedded in your scripts (see L<perlpod> for
an explanation of the POD documentation format) when the user hits the
"Help" button in the dialog box.

Since version 1.094, you can embed specific sections or the full pod
text into any of the blurb, help, author, copyright and date arguments
to the register functions. Gimp::Fu will look into all these strings
for sequences of the form "=pod(section-name)". If found, they will
be replaced by the text of the corresponding section from the pod
documentation. If the named section is not found (or is empty, as in
"=pod()"), the full pod documentation is embedded.

Most of the mentioned arguments have default values (see THE REGISTER
FUNCTION) that are used when the arguments are either undefined or empty
strings, making the register call itself much shorter and, IMHO, more
readable.

=cut

sub register($$$$$$$$$;@) {
   no strict 'refs';
   my($function,$blurb,$help,$author,$copyright,$date,
      $menupath,$imagetypes,$params)=splice(@_,0,9);
   my($results,$features,$code);
   
   $results  = (ref $_[0] eq "ARRAY") ? shift : [];
   $features = (ref $_[0] eq "ARRAY") ? shift : [];
   $code = shift;

   @_==0 or die "register called with too many or wrong arguments\n";
   
   for my $p (@$params,@$results) {
      int($p->[0]) eq $p->[0] or croak "$function: argument/return value '$p->[1]' has illegal type '$p->[0]'";
      $p->[1]=~/^[0-9a-z_]+$/ or carp "$function: argument name '$p->[1]' contains illegal characters, only 0-9, a-z and _ allowed";
   }

   $function=~/^[0-9a-z_]+(-ALT)?$/ or carp "$function: function name contains unusual characters, good style is to use only 0-9, a-z and _";
   
   $function="perl_fu_".$function unless $function=~/^(?:perl_fu|extension|plug_in)/ || $function=~s/^\+//;
   
   Gimp::logger message => "function name contains dashes instead of underscores",
                function => $function, fatal => 0
      if $function =~ y/-//;

   my $perl_sub = sub {
      $run_mode=shift;	# global!
      my(@pre,@defaults,@lastvals,$input_image);

      if ($menupath=~/^<Image>\//) {
         @_ >= 2 or die "<Image> plug-in called without both image and drawable arguments!\n";
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

      if ($run_mode == &Gimp::RUN_INTERACTIVE
          || $run_mode == &Gimp::RUN_WITH_LAST_VALS) {
         my $fudata = $Gimp::Data{"$function/_fu_data"};
         my $VAR1; # Data::Dumper is braindamaged
         local $^W=0; # perl -w is braindamaged

         if ($run_mode == &Gimp::RUN_WITH_LAST_VALS && $fudata ne "") {
            @_ = @{eval $fudata};
         } else {
            if (@_) {
               my $res;
               local $^W=0; # perl -w is braindamaged
               # gimp is braindamaged, is doesn't deliver useful values!!
               ($res,@_)=interact($function,$blurb,$help,$params,@{eval $fudata});
               return unless $res;
            }
         }
      } elsif ($run_mode == &Gimp::RUN_FULLINTERACTIVE) {
         my($res);
         ($res,@_)=interact($function,$blurb,$help,[@image_params,@$params],[@pre,@_]);
         undef @pre;
         return unless $res;
      } elsif ($run_mode == &Gimp::RUN_NONINTERACTIVE) {
      } else {
         die "run_mode must be INTERACTIVE, NONINTERACTIVE or WITH_LAST_VALS\n";
      }
      $input_image = $_[0]   if ref $_[0]   eq "Gimp::Image";
      $input_image = $pre[0] if ref $pre[0] eq "Gimp::Image";
      
      eval { require Data::Dumper };
      $Gimp::Data{"$function/_fu_data"}=Data::Dumper::Dumper([@_]) unless $@;
      
      print $function,"(",join(",",(@pre,@_)),")\n" if $Gimp::verbose;
      
      Gimp::set_trace ($old_trace);
      my @imgs = &$code(@pre,@_);
      $old_trace = Gimp::set_trace (0);
      
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
                  $img->display_new unless $input_image && $$img == $$input_image;
               }
            } elsif (!@$results) {
               warn "WARNING: $function returned something that is not an image: \"$img\"\n";
            }
	 }
      }
      Gimp->displays_flush;
      
      Gimp::set_trace ($old_trace);
      wantarray ? @imgs : $imgs[0];
   };

   Gimp::register_callback($function,$perl_sub);
   push(@scripts,[$perl_sub,$function,$blurb,$help,$author,$copyright,$date,
                  $menupath,$imagetypes,$params,$results,$features,$code]);
}

=cut

=head2 MISC. FUNCTIONS

=over

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
      $quality=$1*0.01,		next if s/^-[qQ](\d+)//;
      $compress=$1,		next if s/^-[cC](\d+)//;
      croak "$_: unknown/illegal file-save option";
   }
   $flatten=(()=$img->get_layers)>1 unless defined $flatten;
   
   $img->flatten if $flatten;
   
   # always save the active layer
   my $layer = $img->get_active_layer;
   
   if ($type eq "JPG" or $type eq "JPEG") {
      eval { Gimp->file_jpeg_save(&Gimp::RUN_NONINTERACTIVE,$img,$layer,$path,$path,$quality,$smooth,1) };
      Gimp->file_jpeg_save(&Gimp::RUN_NONINTERACTIVE,$img,$layer,$path,$path,$quality,$smooth,1,1,"") if $@;
   } elsif ($type eq "GIF") {
      $img->convert_indexed (1,256) unless $layer->indexed;
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
   for(@{$this->[9]}) {
      my $type=$pf_type2string{$_->[0]};
      my $key=mangle_key($_->[1]);
      printf "           -%-25s %s%s\n","$key $type",$_->[2],defined $_->[3] ? " [$_->[3]]" : "";
   }
}

sub main {
   $old_trace = Gimp::set_trace (0);#d#
   if ($Gimp::help) {
      my $this=this_script;
      print <<EOF;
       interface-arguments are
           -o | --output <filespec>   write image to disk, don't display
           -i | --interact            let the user edit the values first
       script-arguments are
EOF
      print_switches ($this);
   } else {
      Gimp::main;
   }
};

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

1;
__END__

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
