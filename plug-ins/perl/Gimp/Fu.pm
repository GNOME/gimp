package Gimp::Fu;

use Gimp ('croak', '__');
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

E<Attention:> at the moment it's neccessary to always import the C<Gimp::Fu>
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
         &PF_IMAGE	=> 'path',
         &PF_LAYER	=> 'index',
         &PF_CHANNEL	=> 'index',
         &PF_DRAWABLE	=> 'index',
);

@_params=qw(PF_INT8 PF_INT16 PF_INT32 PF_FLOAT PF_VALUE PF_STRING PF_COLOR
            PF_COLOUR PF_TOGGLE PF_IMAGE PF_DRAWABLE PF_FONT PF_LAYER
            PF_CHANNEL PF_BOOL PF_SLIDER PF_INT PF_SPINNER PF_ADJUSTMENT
            PF_BRUSH PF_PATTERN PF_GRADIENT PF_RADIO PF_CUSTOM PF_FILE
            PF_TEXT);

#@EXPORT_OK = qw($run_mode save_image);

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

sub carp {
   require Carp;
   goto &Carp::carp;
}

# Some Standard Arguments
my @image_params = ([&Gimp::PARAM_IMAGE		, "image",	"The image to work on"],
                    [&Gimp::PARAM_DRAWABLE	, "drawable",	"The drawable to work on"]);

my @load_params  = ([&Gimp::PARAM_STRING	, "filename",	"The name of the file"],
                    [&Gimp::PARAM_STRING	, "raw_filename","The name of the file"]);

my @save_params  = (@image_params, @load_params);

my @load_retvals = ([&Gimp::PARAM_IMAGE		, "image",	"Output image"]);

my $image_retval = [&Gimp::PARAM_IMAGE		, "image",	"The resulting image"];

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

sub interact {
   eval { require Gtk };

   if ($@) {
      my @res = map {
         die __"the gtk perl module is required to run\nthis plug-in in interactive mode\n" unless defined $_->[3];
         $_->[3];
      } @types;
      Gimp::logger(message => __"the gtk perl module is required to open a dialog\nwindow, running with default values",
                   fatal => 1, function => $function);
      return (1,@res);
   }

   require Gimp::UI;
   goto &Gimp::UI::interact;
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
      $fun =~ s/^(?:perl_fu|plug_in)_//;
      return $this if lc($exe) eq lc($fun);
      push(@names,$fun);
   }
   die __"function '$exe' not found in this script (must be one of ".join(", ",@names).")\n";
}

my $latest_image;

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
      die __"$s: not an integer\n" unless $s==int($s);
      $s*1;
   } elsif($type==PF_FLOAT) {
      $s*1;
   } elsif($type==PF_COLOUR) {
      $s=Gimp::canonicalize_colour($s);
   } elsif($type==PF_TOGGLE) {
      $s?1:0;
   #} elsif($type==PF_IMAGE) {
   } else {
      die __"conversion to type $pf_type2string{$type} is not yet implemented\n";
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

Gimp::on_net {
   no strict 'refs';
   my $this = this_script;
   my(%map,@args);
   my($interact)=1;

   my($perl_sub,$function,$blurb,$help,$author,$copyright,$date,
      $menupath,$imagetypes,$params,$results,$features,$code,$type)=@$this;

   for(@$features) {
      return unless fu_feature_present($_, $function);
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
	   print __"no additional information available, use --help\n";
	   exit 0;
	 } else {
           my $arg=shift @ARGV;
	   my $idx=$map{$1};
	   die __"$_: illegal switch, try $0 --help\n" unless defined($idx);
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
       die __"parameter '$entry->[1]' is not optional\n" unless defined $args[$i] || $interact>0;
   }

   # Go for it
   $perl_sub->(($interact>0 ? &Gimp::RUN_FULLINTERACTIVE : &Gimp::RUN_NONINTERACTIVE),
               @args);
};

Gimp::on_query {
   expand_podsections;
   script:
   for(@scripts) {
      my($perl_sub,$function,$blurb,$help,$author,$copyright,$date,
         $menupath,$imagetypes,$params,$results,$features,$code,$type)=@$_;

      for (@$results) {
         next if ref $_;
         if ($_ == &Gimp::PARAM_IMAGE) {
            $_ = $image_retval;
         }
      }

      for(@$features) {
         next script unless fu_feature_present($_,$function);
      }

      # guess the datatype. yeah!
      sub datatype(@) {
         for(@_) {
            return Gimp::PARAM_STRING unless /^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/; # perlfaq4
            return Gimp::PARAM_FLOAT  unless /^[+-]?\d+$/; # again
         }
         return Gimp::PARAM_INT32;
      }
      sub odd_values(@) {
         my %x = @_; values %x;
      }

      for(@$params) {
         $_->[0]=Gimp::PARAM_INT32	if $_->[0] == PF_TOGGLE;
         $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_FONT;
         $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_BRUSH;
         $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_PATTERN;
         $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_GRADIENT;
         $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_CUSTOM;
         $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_FILE;
         $_->[0]=Gimp::PARAM_STRING	if $_->[0] == PF_TEXT;
         $_->[0]=datatype(odd_values(@{$_->[4]})) if $_->[0] == PF_RADIO;
         $_->[0]=datatype($_->[3],@{$_->[4]}) if $_->[0] == PF_SLIDER;
         $_->[0]=datatype($_->[3],@{$_->[4]}) if $_->[0] == PF_SPINNER;
         $_->[0]=datatype($_->[3],@{$_->[4]}) if $_->[0] == PF_ADJUSTMENT;
      }
      Gimp->gimp_install_procedure($function,$blurb,$help,$author,$copyright,$date,
                                   $menupath,$imagetypes,$type,
                                   [[Gimp::PARAM_INT32,"run_mode","Interactive, [non-interactive]"],
                                    @$params],
                                   $results);

      Gimp::logger(message => 'OK', function => $function, fatal => 0);
   }
};

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
"file_", "plug_in_" or "extension_", it will be prepended. If you
don't want this, prefix your function name with a single "+". The idea
here is that every Gimp::Fu plug-in will be found under the common
C<perl_fu_>-prefix.

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
values. Of course, default values and the enhanced Gimp::Fu parameter
types don't make much sense here. (Even if they did, it's not implemented
anyway..). This argument is optional.

If you supply a parameter type (e.g. C<PF_IMAGE>) instead of a full
specification (C<[PF_IMAGE, ...]>), Gimp::Fu might supply some default
values. This is only implemented for C<PF_IMAGE> at the moment.

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
   my($results,$features,$code,$type);

   $results  = (ref $_[0] eq "ARRAY") ? shift : [];
   $features = (ref $_[0] eq "ARRAY") ? shift : [];
   $code = shift;

   for($menupath) {
      if (/^<Image>\//) {
         $type = &Gimp::PROC_PLUG_IN;
         unshift @$params, @image_params;
      } elsif (/^<Load>\//) {
         $type = &Gimp::PROC_PLUG_IN;
         unshift @$params, @load_params;
         unshift @$results, @load_retvals;
      } elsif (/^<Save>\//) {
         $type = &Gimp::PROC_PLUG_IN;
         unshift @$params, @save_params;
      } elsif (/^<Toolbox>\//) {
         $type = &Gimp::PROC_EXTENSION;
      } elsif (/^<None>/) {
         $type = &Gimp::PROC_EXTENSION;
      } else {
         die __"menupath _must_ start with <Image>, <Toolbox>, <Load>, <Save> or <None>!";
      }
   }
   #$menupath =~ s%^<Toolbox>/Xtns/%__("<Toolbox>/Xtns/")%e;
   #$menupath =~ s%^<Image>/Filters/%__("<Image>/Filters/")%e;
   undef $menupath if $menupath eq "<None>";#d#

   @_==0 or die __"register called with too many or wrong arguments\n";

   for my $p (@$params,@$results) {
      next unless ref $p;
      int($p->[0]) eq $p->[0] or croak __"$function: argument/return value '$p->[1]' has illegal type '$p->[0]'";
      $p->[1]=~/^[0-9a-z_]+$/ or carp __"$function: argument name '$p->[1]' contains illegal characters, only 0-9, a-z and _ allowed";
   }

   $function="perl_fu_".$function unless $function =~ /^(?:perl_fu_|extension_|plug_in_|file_)/ || $function =~ s/^\+//;

   $function=~/^[0-9a-z_]+(-ALT)?$/ or carp __"$function: function name contains unusual characters, good style is to use only 0-9, a-z and _";

   Gimp::logger message => __"function name contains dashes instead of underscores",
                function => $function, fatal => 0
      if $function =~ y/-//;

   my $perl_sub = sub {
      $run_mode = shift;	# global!
      my(@pre,@defaults,@lastvals,$input_image);

      if (@defaults) {
         for (0..$#{$params}) {
	    $params->[$_]->[3]=$defaults[$_];
	 }
      }

      # supplement default arguments
      for (0..$#{$params}) {
         $_[$_]=$params->[$_]->[3] unless defined($_[$_]);
      }

      for($menupath) {
         if (/^<Image>\//) {
            @_ >= 2 or die __"<Image> plug-in called without both image and drawable arguments!\n";
            @pre = (shift,shift);
         } elsif (/^<Toolbox>\// or !defined $menupath) {
            # valid ;)
         } elsif (/^<Load>\//) {
            @_ >= 2 or die __"<Load> plug-in called without the 3 standard arguments!\n";
            @pre = (shift,shift);
         } elsif (/^<Save>\//) {
            @_ >= 4 or die __"<Save> plug-in called without the 5 standard arguments!\n";
            @pre = (shift,shift,shift,shift);
         } elsif (defined $_) {
            die __"menupath _must_ start with <Image>, <Toolbox>, <Load>, <Save> or <None>!";
         }
      }
      if ($run_mode == &Gimp::RUN_INTERACTIVE
          || $run_mode == &Gimp::RUN_WITH_LAST_VALS) {
         my $fudata = $Gimp::Data{"$function/_fu_data"};

         if ($run_mode == &Gimp::RUN_WITH_LAST_VALS && $fudata) {
            @_ = @$fudata;
         } else {
            if (@_) {
               # prevent the standard arguments from showing up in interact
               my @hide = splice @$params, 0, scalar@pre;

               my $res;
               local $^W=0; # perl -w is braindamaged
               # gimp is braindamaged, is doesn't deliver useful values!!
               ($res,@_)=interact($function,$blurb,$help,$params,@{$fudata});
               return unless $res;

               unshift @$params, @hide;
            }
         }
      } elsif ($run_mode == &Gimp::RUN_FULLINTERACTIVE) {
         if (@_) {
            my($res);
            ($res,@_)=interact($function,$blurb,$help,$params,@pre,@_);
            undef @pre;
            return unless $res;
         }
      } elsif ($run_mode == &Gimp::RUN_NONINTERACTIVE) {
         # nop
      } else {
         die __"run_mode must be INTERACTIVE, NONINTERACTIVE or RUN_WITH_LAST_VALS\n";
      }
      $input_image = $_[0]   if ref $_[0]   eq "Gimp::Image";
      $input_image = $pre[0] if ref $pre[0] eq "Gimp::Image";

      $Gimp::Data{"$function/_fu_data"}=[@_];

      print $function,"(",join(",",(@pre,@_)),")\n" if $Gimp::verbose;

      Gimp::set_trace ($old_trace);
      my @imgs = &$code(@pre,@_);
      $old_trace = Gimp::set_trace (0);

      if ($menupath !~ /^<Load>\//) {
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
               } elsif (!@$retvals) {
                  warn __"WARNING: $function returned something that is not an image: \"$img\"\n";
               }
            }
         }
         Gimp->displays_flush;
      }

      Gimp::set_trace ($old_trace);
      wantarray ? @imgs : $imgs[0];
   };

   Gimp::register_callback($function,$perl_sub);
   push(@scripts,[$perl_sub,$function,$blurb,$help,$author,$copyright,$date,
                  $menupath,$imagetypes,$params,$results,$features,$code,$type]);
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

 options for GIF animations (use with -F)
 +L	save as looping animation
 -L	save as non-looping animation (default)
 -Dn	default frame delay (default is 0)
 -Pn	frame disposal method: 0=don't care, 1 = combine, 2 = replace

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
   my($interlace,$flatten,$quality,$type,$smooth,$compress,$loop,$dispose);

   $interlace=0;
   $quality=75;
   $smooth=0;
   $compress=7;
   $loop=0;
   $delay=0;
   $dispose=0;

   $_=$path=~s/^([^:]+):// ? $1 : "";
   $type=uc($1) if $path=~/\.([^.]+)$/;
   $type=uc($1) if s/^(GIF|JPG|JPEG|PNM|PNG)//i;
   while($_ ne "") {
      $interlace=$1 eq "+", 	next if s/^([-+])[iI]//;
      $flatten=$1 eq "+", 	next if s/^([-+])[fF]//;
      $smooth=$1 eq "+", 	next if s/^([-+])[sS]//;
      $quality=$1*0.01,		next if s/^-[qQ](\d+)//;
      $compress=$1,		next if s/^-[cC](\d+)//;
      $loop=$1 eq "+",		next if s/^([-+])[lL]//;
      $delay=$1,		next if s/^-[dD](\d+)//;
      $dispose=$1,		next if s/^-[pP](\d+)//;
      croak __"$_: unknown/illegal file-save option";
   }
   $flatten=(()=$img->get_layers)>1 unless defined $flatten;

   $img->flatten if $flatten;

   # always save the active layer
   my $layer = $img->get_active_layer;

   if ($type eq "JPG" or $type eq "JPEG") {
      eval { $layer->file_jpeg_save($path,$path,$quality,$smooth,1) };
      $layer->file_jpeg_save($path,$path,$quality,$smooth,1,$interlace,"",0,1,0,0) if $@;
   } elsif ($type eq "GIF") {
      unless ($layer->is_indexed) {
         eval { $img->convert_indexed(1,256) };
         $img->convert_indexed(2,&Gimp::MAKE_PALETTE,256,1,1,"") if $@;
      }
      $layer->file_gif_save($path,$path,$interlace,$loop,$delay,$dispose);
   } elsif ($type eq "PNG") {
      $layer->file_png_save($path,$path,$interlace,$compress);
   } elsif ($type eq "PNM") {
      $layer->file_pnm_save($path,$path,1);
   } else {
      $layer->gimp_file_save($path,$path);
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
   $old_trace = Gimp::set_trace (0);
   if ($Gimp::help) {
      my $this=this_script;
      print __"       interface-arguments are
           -o | --output <filespec>   write image to disk, don't display
           -i | --interact            let the user edit the values first
       script-arguments are
";
      print_switches ($this);
   } else {
      Gimp::main;
   }
}

1;
__END__

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), L<Gimp>.

=cut
