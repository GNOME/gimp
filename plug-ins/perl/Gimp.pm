package Gimp;

use strict 'vars';
use Carp;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK $AUTOLOAD %EXPORT_TAGS @EXPORT_FAIL
            @_consts @_procs $interface_pkg $interface_type @_param @_al_consts
            @PREFIXES $_PROT_VERSION
            @gimp_gui_functions $function $basename $spawn_opts
            $in_quit $in_run $in_net $in_init $in_query $no_SIG
            $help $verbose $host);
use subs qw(init end lock unlock canonicalize_color);

require DynaLoader;

@ISA=qw(DynaLoader);
$VERSION = 1.094;

@_param = qw(
	PARAM_BOUNDARY	PARAM_CHANNEL	PARAM_COLOR	PARAM_DISPLAY	PARAM_DRAWABLE
	PARAM_END	PARAM_FLOAT	PARAM_IMAGE	PARAM_INT32	PARAM_FLOATARRAY
	PARAM_INT16	PARAM_PARASITE	PARAM_STRING	PARAM_PATH	PARAM_INT16ARRAY
	PARAM_INT8	PARAM_INT8ARRAY	PARAM_LAYER	PARAM_REGION	PARAM_STRINGARRAY
	PARAM_SELECTION	PARAM_STATUS	PARAM_INT32ARRAY
);

# constants that, in some earlier version, were autoloaded
@_consts = (@_param,
#ENUM_NAME#
'PRESSURE',            'SOFT',                'HARD',                'INDEXEDA_IMAGE',      'RGBA_IMAGE',          'GRAYA_IMAGE',         'RGB_IMAGE',           
'INDEXED_IMAGE',       'GRAY_IMAGE',          'TRANS_IMAGE_FILL',    'WHITE_IMAGE_FILL',    'BG_IMAGE_FILL',       'FG_IMAGE_FILL',       'NO_IMAGE_FILL',       
'VERTICAL_FLIP',       'HORIZONTAL_FLIP',     'LOOP_SAWTOOTH',       'ONCE_BACKWARDS',      'ONCE_FORWARD',        'ONCE_END_COLOR',      'LOOP_TRIANGLE',       
'CYAN_HUES',           'RED_HUES',            'GREEN_HUES',          'YELLOW_HUES',         'ALL_HUES',            'MAGENTA_HUES',        'BLUE_HUES',           
'MONO_PALETTE',        'WEB_PALETTE',         'MAKE_PALETTE',        'REUSE_PALETTE',       'CUSTOM_PALETTE',      'POINTS',              'PIXELS',              
'SATURATION_MODE',     'ADDITION_MODE',       'NORMAL_MODE',         'HUE_MODE',            'SUBTRACT_MODE',       'DIVIDE_MODE',         'SCREEN_MODE',         
'BEHIND_MODE',         'MULTIPLY_MODE',       'DISSOLVE_MODE',       'DIFFERENCE_MODE',     'DARKEN_ONLY_MODE',    'VALUE_MODE',          'LIGHTEN_ONLY_MODE',   
'REPLACE_MODE',        'COLOR_MODE',          'OVERLAY_MODE',        'ERASE_MODE',          'CUSTOM',              'FG_TRANS',            'FG_BG_HSV',           
'FG_BG_RGB',           'CONTINUOUS',          'INCREMENTAL',         'APPLY',               'DISCARD',             'BLUE_CHANNEL',        'GREEN_CHANNEL',       
'GRAY_CHANNEL',        'AUXILLARY_CHANNEL',   'INDEXED_CHANNEL',     'RED_CHANNEL',         'HORIZONTAL_SHEAR',    'VERTICAL_SHEAR',      'BLUR_CONVOLVE',       
'CUSTOM_CONVOLVE',     'SHARPEN_CONVOLVE',    'SELECTION_SUB',       'SELECTION_INTERSECT', 'SELECTION_ADD',       'SELECTION_REPLACE',   'SHADOWS',             
'MIDTONES',            'HIGHLIGHTS',          'IMAGE_CLONE',         'PATTERN_CLONE',       'INDEXED',             'RGB',                 'GRAY',                
'ADD_BLACK_MASK',      'ADD_WHITE_MASK',      'ADD_ALPHA_MASK',      'BILINEAR',            'SPIRAL_CLOCKWISE',    'SQUARE',              'RADIAL',              
'CONICAL_SYMMETRIC',   'SHAPEBURST_DIMPLED',  'CONICAL_ASYMMETRIC',  'LINEAR',              'SPIRAL_ANTICLOCKWISE','SHAPEBURST_ANGULAR',  'SHAPEBURST_SPHERICAL',
'ALPHA_LUT',           'GREEN_LUT',           'BLUE_LUT',            'VALUE_LUT',           'RED_LUT',             'HORIZONTAL_GUIDE',    'VERTICAL_GUIDE',      
'OFFSET_BACKGROUND',   'OFFSET_TRANSPARENT',  'MESSAGE_BOX',         'ERROR_CONSOLE',       'CONSOLE',             'RUN_INTERACTIVE',     'RUN_WITH_LAST_VALS',  
'RUN_NONINTERACTIVE',  'EXPAND_AS_NECESSARY', 'CLIP_TO_BOTTOM_LAYER','CLIP_TO_IMAGE',       'FLATTEN_IMAGE',       'REPEAT_NONE',         'REPEAT_SAWTOOTH',     
'REPEAT_TRIANGULAR',   'BG_BUCKET_FILL',      'FG_BUCKET_FILL',      'PATTERN_BUCKET_FILL', 
#ENUM_NAME#
	'STATUS_CALLING_ERROR',		'STATUS_EXECUTION_ERROR',	'STATUS_PASS_THROUGH',
        'STATUS_SUCCESS',		'PARASITE_PERSISTENT',		'PARASITE_ATTACH_PARENT',
        'PARASITE_PARENT_PERSISTENT',	'PARASITE_ATTACH_GRANDPARENT',	'PARASITE_GRANDPARENT_PERSISTENT',
        'PARASITE_UNDOABLE',		'PARASITE_PARENT_UNDOABLE',	'PARASITE_GRANDPARENT_UNDOABLE',
	'TRACE_NONE',	'TRACE_CALL',	'TRACE_TYPE',	'TRACE_NAME',	'TRACE_DESC',	'TRACE_ALL',
	'COMPRESSION_NONE',		'COMPRESSION_LZW',		'COMPRESSION_PACKBITS',
        'WRAP',				'SMEAR',			'BLACK',
);

@_procs = ('main','xlfd_size');

bootstrap Gimp $VERSION;

#ENUM_DEFS#
sub PRESSURE            (){ 2} sub SOFT                (){ 1} sub HARD                (){ 0} sub INDEXEDA_IMAGE      (){ 5} sub RGBA_IMAGE          (){ 1}
sub GRAYA_IMAGE         (){ 3} sub RGB_IMAGE           (){ 0} sub INDEXED_IMAGE       (){ 4} sub GRAY_IMAGE          (){ 2} sub TRANS_IMAGE_FILL    (){ 3}
sub WHITE_IMAGE_FILL    (){ 2} sub BG_IMAGE_FILL       (){ 1} sub FG_IMAGE_FILL       (){ 0} sub NO_IMAGE_FILL       (){ 4} sub VERTICAL_FLIP       (){ 1}
sub HORIZONTAL_FLIP     (){ 0} sub LOOP_SAWTOOTH       (){ 2} sub ONCE_BACKWARDS      (){ 1} sub ONCE_FORWARD        (){ 0} sub ONCE_END_COLOR      (){ 4}
sub LOOP_TRIANGLE       (){ 3} sub CYAN_HUES           (){ 4} sub RED_HUES            (){ 1} sub GREEN_HUES          (){ 3} sub YELLOW_HUES         (){ 2}
sub ALL_HUES            (){ 0} sub MAGENTA_HUES        (){ 6} sub BLUE_HUES           (){ 5} sub MONO_PALETTE        (){ 3} sub WEB_PALETTE         (){ 2}
sub MAKE_PALETTE        (){ 0} sub REUSE_PALETTE       (){ 1} sub CUSTOM_PALETTE      (){ 4} sub POINTS              (){ 1} sub PIXELS              (){ 0}
sub SATURATION_MODE     (){12} sub ADDITION_MODE       (){ 7} sub NORMAL_MODE         (){ 0} sub HUE_MODE            (){11} sub SUBTRACT_MODE       (){ 8}
sub DIVIDE_MODE         (){15} sub SCREEN_MODE         (){ 4} sub BEHIND_MODE         (){ 2} sub MULTIPLY_MODE       (){ 3} sub DISSOLVE_MODE       (){ 1}
sub DIFFERENCE_MODE     (){ 6} sub DARKEN_ONLY_MODE    (){ 9} sub VALUE_MODE          (){14} sub LIGHTEN_ONLY_MODE   (){10} sub REPLACE_MODE        (){17}
sub COLOR_MODE          (){13} sub OVERLAY_MODE        (){ 5} sub ERASE_MODE          (){16} sub CUSTOM              (){ 3} sub FG_TRANS            (){ 2}
sub FG_BG_HSV           (){ 1} sub FG_BG_RGB           (){ 0} sub CONTINUOUS          (){ 0} sub INCREMENTAL         (){ 1} sub APPLY               (){ 0}
sub DISCARD             (){ 1} sub BLUE_CHANNEL        (){ 2} sub GREEN_CHANNEL       (){ 1} sub GRAY_CHANNEL        (){ 3} sub AUXILLARY_CHANNEL   (){ 5}
sub INDEXED_CHANNEL     (){ 4} sub RED_CHANNEL         (){ 0} sub HORIZONTAL_SHEAR    (){ 0} sub VERTICAL_SHEAR      (){ 1} sub BLUR_CONVOLVE       (){ 0}
sub CUSTOM_CONVOLVE     (){ 2} sub SHARPEN_CONVOLVE    (){ 1} sub SELECTION_SUB       (){ 1} sub SELECTION_INTERSECT (){ 3} sub SELECTION_ADD       (){ 0}
sub SELECTION_REPLACE   (){ 2} sub SHADOWS             (){ 0} sub MIDTONES            (){ 1} sub HIGHLIGHTS          (){ 2} sub IMAGE_CLONE         (){ 0}
sub PATTERN_CLONE       (){ 1} sub INDEXED             (){ 2} sub RGB                 (){ 0} sub GRAY                (){ 1} sub ADD_BLACK_MASK      (){ 1}
sub ADD_WHITE_MASK      (){ 0} sub ADD_ALPHA_MASK      (){ 2} sub BILINEAR            (){ 1} sub SPIRAL_CLOCKWISE    (){ 9} sub SQUARE              (){ 3}
sub RADIAL              (){ 2} sub CONICAL_SYMMETRIC   (){ 4} sub SHAPEBURST_DIMPLED  (){ 8} sub CONICAL_ASYMMETRIC  (){ 5} sub LINEAR              (){ 0}
sub SPIRAL_ANTICLOCKWISE(){10} sub SHAPEBURST_ANGULAR  (){ 6} sub SHAPEBURST_SPHERICAL(){ 7} sub ALPHA_LUT           (){ 4} sub GREEN_LUT           (){ 2}
sub BLUE_LUT            (){ 3} sub VALUE_LUT           (){ 0} sub RED_LUT             (){ 1} sub HORIZONTAL_GUIDE    (){ 1} sub VERTICAL_GUIDE      (){ 2}
sub OFFSET_BACKGROUND   (){ 0} sub OFFSET_TRANSPARENT  (){ 1} sub MESSAGE_BOX         (){ 0} sub ERROR_CONSOLE       (){ 2} sub CONSOLE             (){ 1}
sub RUN_INTERACTIVE     (){bless \(my $x=0),'Gimp::run_mode'} sub RUN_WITH_LAST_VALS  (){bless \(my $x=2),'Gimp::run_mode'} sub RUN_NONINTERACTIVE  (){bless \(my $x=1),'Gimp::run_mode'} sub EXPAND_AS_NECESSARY (){ 0} sub CLIP_TO_BOTTOM_LAYER(){ 2}
sub CLIP_TO_IMAGE       (){ 1} sub FLATTEN_IMAGE       (){ 3} sub REPEAT_NONE         (){ 0} sub REPEAT_SAWTOOTH     (){ 1} sub REPEAT_TRIANGULAR   (){ 2}
sub BG_BUCKET_FILL      (){ 1} sub FG_BUCKET_FILL      (){ 0} sub PATTERN_BUCKET_FILL (){ 2}
#ENUM_DEFS#

sub WRAP		(){ 0 }
sub SMEAR		(){ 1 }
sub BLACK		(){ 2 }

# file_tiff_*
sub COMPRESSION_NONE		(){ 0 }
sub COMPRESSION_LZW		(){ 1 }
sub COMPRESSION_PACKBITS	(){ 2 }

# internal constants shared with Perl-Server

sub _PS_FLAG_QUIET	{ 0000000001 };	# do not output messages
sub _PS_FLAG_BATCH	{ 0000000002 }; # started via Gimp::Net, extra = filehandle

$_PROT_VERSION = "3";			# protocol version

# we really abuse the import facility..
sub import($;@) {
   my $pkg = shift;
   my $up = caller;
   my @export;

   # make a quick but dirty guess ;)
   
   @_=(@_procs,':consts',':auto') unless @_;
   
   for(@_) {
      if ($_ eq ":auto") {
         push(@export,@_consts,@_procs);
         *{"$up\::AUTOLOAD"} = sub {
            croak "Cannot call '$AUTOLOAD' at this time" unless initialized();
            my ($class,$name) = $AUTOLOAD =~ /^(.*)::(.*?)$/;
            *{$AUTOLOAD} = sub { Gimp->$name(@_) };
            goto &$AUTOLOAD;
         };
      } elsif ($_ eq ":consts") {
         push(@export,@_consts);
      } elsif ($_ eq ":param") {
         push(@export,@_param);
      } elsif (/^interface=(\S+)$/) {
         croak "interface=... tag is no longer supported\n";
      } elsif ($_=~/spawn_options=(\S+)/) {
         $spawn_opts = $1;
      } elsif ($_ ne "") {
         push(@export,$_);
      } elsif ($_ eq "") {
         #nop #d#FIXME, Perl-Server requires this!
      } else {
         croak "$_ is not a valid import tag for package $pkg";
      }
   }
   
   for(@export) {
      *{"$up\::$_"} = \&$_;
   }
}

sub xlfd_size($) {
  local $^W=0;
  my ($px,$pt)=(split(/-/,$_[0]))[7,8];
  $px>0 ? ($px    ,&Gimp::PIXELS)
        : ($pt*0.1,&Gimp::POINTS);
}

sub init_gtk {
   require Gtk;

   Gtk->init;
   Gtk::Rc->parse (Gimp->gtkrc);
   Gtk::Gdk->set_use_xshm (Gimp->use_xshm);
   Gtk::Preview->set_gamma (Gimp->gamma);
   Gtk::Preview->set_install_cmap (Gimp->install_cmap);
   Gtk::Preview->set_color_cube (Gimp->color_cube);
   Gtk::Widget->set_default_visual (Gtk::Preview->get_visual);
   Gtk::Widget->set_default_colormap (Gtk::Preview->get_cmap);
}

# internal utility function for Gimp::Fu and others
sub wrap_text {
   my $x=$_[0];
   $x=~s/\G(.{1,$_[1]})(\s+|$)/$1\n/gm;
   $x=~s/[ \t\r\n]+$//g;
   $x;
}

my %rgb_db;
my $rgb_db_path;

sub set_rgb_db($) {
   $rgb_db_path=$_[0];
   undef %rgb_db;
}

sub canonicalize_colour {
   if (@_ == 3) {
      [@_];
   } elsif (ref $_[0]) {
      $_[0];
   } elsif ($_[0] =~ /^#([0-9a-fA-F]{2,2})([0-9a-fA-F]{2,2})([0-9a-fA-F]{2,2})$/) {
      [map {eval "0x$_"} ($1,$2,$3)];
   } else {
      unless (defined %rgb_db) {
         if ($rgb_db_path) {
            open RGB_TEXT,"<$rgb_db_path" or croak "unable to open $rgb_db_path";
         } else {
            *RGB_TEXT=*DATA;
         }
         while(<RGB_TEXT>) {
            next unless /^\s*(\d+)\s+(\d+)\s+(\d+)\s+(.+?)\s*$/;
            $rgb_db{lc($4)}=[$1,$2,$3];
         }
         close RGB_TEXT if defined $rgb_db_path;
      }
      if ($rgb_db{lc($_[0])}) {
         return $rgb_db{lc($_[0])};
      } else {
         croak "Unable to grok '".join(",",@_),"' as colour specifier";
      }
   }
}

*canonicalize_color = \&canonicalize_colour;

($basename = $0) =~ s/^.*[\\\/]//;

$spawn_opts = "";

# extra check for Gimp::Feature::import
$in_query=0 unless defined $in_query;	# perl -w is SOOO braindamaged
$in_quit=$in_run=$in_net=$in_init=0;	# perl -w is braindamaged

$verbose=0;

$interface_type = "net";
if (@ARGV) {
   if ($ARGV[0] eq "-gimp") {
      $interface_type = "lib";
      # ignore other parameters completely
   } else {
      while(@ARGV) {
         $_=shift(@ARGV);
         if (/^-h$|^--?help$|^-\?$/) {
            $help=1;
            print <<EOF;
Usage: $0 [gimp-args..] [interface-args..] [script-args..]
       gimp-arguments are
           -gimp <anything>           used internally only
           -h | -help | --help | -?   print some help
           -v | --verbose             be more verbose in what you do
           --host|--tcp HOST[:PORT]   connect to HOST (optionally using PORT)
                                      (for more info, see Gimp::Net(3))
EOF
         } elsif (/^-v$|^--verbose$/) {
            $verbose++;
         } elsif (/^--host$|^--tcp$/) {
            $host=shift(@ARGV);
         } else {
            unshift(@ARGV,$_);
            last;
         }
      }
   }
}

my @log;
my $caller;

sub format_msg {
   $_=shift;
   "$_->[0]: $_->[2] ".($_->[1] ? "($_->[1])":"");
}

sub _initialized_callback {
   # load the compatibility module on older versions
   if ($interface_pkg eq "Gimp::Lib") {
      # this must match @max_gimp_version in Gimp::Compat
      my @compat_gimp_version = (1,1);
      if ((Gimp->major_version < $compat_gimp_version[0])
          || (Gimp->major_version == $compat_gimp_version[0]
              && Gimp->minor_version < $compat_gimp_version[1])) {
         require Gimp::Compat;
         $compat_gimp_version[0] == $Gimp::Compat::max_gimp_version[0]
            && $compat_gimp_version[1] == $Gimp::Compat::max_gimp_version[1]
               or die "FATAL: Gimp::Compat version mismatch\n";
      }
   }
   if (@log) {
      unless ($in_net || $in_query || $in_quit || $in_init) {
         for(@log) {
            Gimp->message(format_msg($_)) if $_->[3];
         }
      }
      Gimp->_gimp_append_data ('gimp-perl-log', map join("\1",@$_)."\0",@log);
      @log=();
   }
}

# message
# function
# fatal
sub logger {
   my %args = @_;
   $args{message}  = "unknown message"    unless defined $args{message};
   $args{function} = $function            unless defined $args{function};
   $args{function} = ""                   unless defined $args{function};
   $args{fatal}    = 1                    unless defined $args{fatal};
   push(@log,[$basename,@args{'function','message','fatal'}]);
   print STDERR format_msg($log[-1]),"\n" if !($in_query || $in_init || $in_quit) || $verbose;
   _initialized_callback if initialized();
}

sub die_msg {
   logger(message => substr($_[0],0,-1), fatal => 1, function => 'ERROR');
}

# this needs to be improved
sub quiet_die {
   die "IGNORE THIS MESSAGE\n";
}

unless ($no_SIG) {
   $SIG{__DIE__} = sub {
      unless ($^S || !defined $^S || $in_quit) {
         die_msg $_[0];
         initialized() ? &quiet_die : exit quiet_main();
      } else {
        die $_[0];
      }
   };

   $SIG{__WARN__} = sub {
      unless ($in_quit) {
        warn $_[0];
      } else {
        logger(message => substr($_[0],0,-1), fatal => 0, function => 'WARNING');
      }
   };
}

my %callback;

sub call_callback {
   my $req = shift;
   my $cb = shift;
   return () if $caller eq "Gimp";
   if ($callback{$cb}) {
      &{$callback{$cb}};
   } elsif (UNIVERSAL::can($caller,$cb)) {
      &{"$caller\::$cb"};
   } else {
      die_msg "required callback '$cb' not found\n" if $req;
   }
}

sub callback {
   my $type = shift;
   if ($type eq "-run") {
      local $function = shift;
      local $in_run = 1;
      _initialized_callback;
      call_callback 1,$function,@_;
   } elsif ($type eq "-net") {
      local $in_net = 1;
      _initialized_callback;
      call_callback 1,"net";
   } elsif ($type eq "-query") {
      local $in_query = 1;
      _initialized_callback;
      call_callback 1,"query";
   } elsif ($type eq "-quit") {
      local $in_quit = 1;
      call_callback 0,"quit";
   }
}

sub register_callback($$) {
   $callback{$_[0]}=$_[1];
}

sub main {
   $caller=caller;
   #d# #D# # BIG BUG LURKING SOMEWHERE
   # just calling exit() will be too much for bigexitbug.pl
   xs_exit(&{"$interface_pkg\::gimp_main"});
}

# same as main, but callbacks are ignored
sub quiet_main {
   main;
}

##############################################################################

if ($interface_type=~/^lib$/i) {
   $interface_pkg="Gimp::Lib";
} elsif ($interface_type=~/^net$/i) {
   $interface_pkg="Gimp::Net";
} else {
   croak "interface '$interface_type' unsupported.";
}

eval "require $interface_pkg" or croak "$@";
$interface_pkg->import;

# create some common aliases
for(qw(_gimp_procedure_available gimp_call_procedure set_trace initialized)) {
   *$_ = \&{"$interface_pkg\::$_"};
}

*init  = \&{"$interface_pkg\::gimp_init"};
*end   = \&{"$interface_pkg\::gimp_end" };
*lock  = \&{"$interface_pkg\::lock" };
*unlock= \&{"$interface_pkg\::unlock" };

my %ignore_function = ();

@PREFIXES=("gimp_", "");

@gimp_gui_functions = qw(
   gimp_progress_init
   gimp_progress_update
   gimp_displays_flush
   gimp_display_new
   gimp_display_delete
);

sub ignore_functions(@) {
   @ignore_function{@_}++;
}

sub _croak($) {
  $_[0] =~ s/ at .*? line \d+.*$//s;
  croak($_[0]);
}

sub build_thunk($) {
   my $sub = $_[0];
   sub {
      shift unless ref $_[0];
      unshift @_,$sub;
      #goto &gimp_call_procedure; # does not always work, PERLBUG! #FIXME
      my @r=eval { gimp_call_procedure (@_) };
      _croak $@ if $@;
      wantarray ? @r : $r[0];
   };
}

sub AUTOLOAD {
   my ($class,$name) = $AUTOLOAD =~ /^(.*)::(.*?)$/;
   for(@{"$class\::PREFIXES"}) {
      my $sub = $_.$name;
      if (exists $ignore_function{$sub}) {
        *{$AUTOLOAD} = sub { () };
        goto &$AUTOLOAD;
      } elsif (UNIVERSAL::can(Gimp::Util,$sub)) {
         my $ref = \&{"Gimp::Util::$sub"};
         *{$AUTOLOAD} = sub {
            shift unless ref $_[0];
            #goto &$ref; # does not always work, PERLBUG! #FIXME
            my @r = eval { &$ref };
            _croak $@ if $@;
            wantarray ? @r : $r[0];
         };
         goto &$AUTOLOAD;
      } elsif (UNIVERSAL::can($interface_pkg,$sub)) {
         my $ref = \&{"$interface_pkg\::$sub"};
         *{$AUTOLOAD} = sub {
            shift unless ref $_[0];
            #goto &$ref;	# does not always work, PERLBUG! #FIXME
            my @r = eval { &$ref };
            _croak $@ if $@;
            wantarray ? @r : $r[0];
         };
         goto &$AUTOLOAD;
      } elsif (_gimp_procedure_available ($sub)) {
         *{$AUTOLOAD} = build_thunk ($sub);
         goto &$AUTOLOAD;
      }
   }
   # for performance reasons: supply a DESTROY method
   if($name eq "DESTROY") {
      *{$AUTOLOAD} = sub {};
      return;
   }
   croak "function/macro \"$name\" not found in $class";
}

sub _pseudoclass {
  my ($class, @prefixes)= @_;
  unshift(@prefixes,"");
  *{"Gimp::$class\::AUTOLOAD"} = \&AUTOLOAD;
  push(@{"$class\::ISA"}		, "Gimp::$class");
  push(@{"Gimp::$class\::PREFIXES"}	, @prefixes); @prefixes=@{"Gimp::$class\::PREFIXES"};
  push(@{"$class\::PREFIXES"}		, @prefixes); @prefixes=@{"$class\::PREFIXES"};
}

_pseudoclass qw(Layer		gimp_layer_ gimp_drawable_ gimp_floating_sel_ gimp_image_ gimp_ plug_in_);
_pseudoclass qw(Image		gimp_image_ gimp_drawable_ gimp_ plug_in_);
_pseudoclass qw(Drawable	gimp_drawable_ gimp_layer_ gimp_channel_ gimp_image_ gimp_ plug_in_);
_pseudoclass qw(Selection 	gimp_selection_);
_pseudoclass qw(Channel		gimp_channel_ gimp_drawable_ gimp_selection_ gimp_image_ gimp_ plug_in_);
_pseudoclass qw(Display		gimp_display_ gimp_);
_pseudoclass qw(Plugin		plug_in_);
_pseudoclass qw(Gradients	gimp_gradients_);
_pseudoclass qw(Edit		gimp_edit_);
_pseudoclass qw(Progress	gimp_progress_);
_pseudoclass qw(Region		);
_pseudoclass qw(Parasite	parasite_ gimp_);

# "C"-Classes
_pseudoclass qw(GDrawable	gimp_gdrawable_ gimp_drawable_);
_pseudoclass qw(PixelRgn	gimp_pixel_rgn_);
_pseudoclass qw(Tile		gimp_tile_);

# Classes without GIMP-Object
_pseudoclass qw(Palette		gimp_palette_);
_pseudoclass qw(Brushes		gimp_brushes_);
_pseudoclass qw(Edit		gimp_edit_);
_pseudoclass qw(Gradients	gimp_gradients_);
_pseudoclass qw(Patterns	gimp_patterns_);

package Gimp::Tile;

unshift (@Tile::ISA, "Gimp::Tile");

package Gimp::PixelRgn;

push(@PixelRgn::ISA, "Gimp::PixelRgn");

sub new($$$$$$$$) {
   shift;
   init Gimp::PixelRgn(@_);
}

package Gimp::Parasite;

sub is_type($$)		{ $_[0]->[0] eq $_[1] }
sub is_persistant($)	{ $_[0]->[1] & &Gimp::PARASITE_PERSISTANT }
sub is_error($)		{ !defined $_[0]->[0] }
sub has_flag($$)	{ $_[0]->[1] & $_[1] }
sub copy($)		{ [@{$_[0]}] }
sub name($)		{ $_[0]->[0] }
sub flags($)		{ $_[0]->[1] }
sub data($)		{ $_[0]->[2] }
sub compare($$)		{ $_[0]->[0] eq $_[1]->[0] and
			  $_[0]->[1] eq $_[1]->[1] and 
			  $_[0]->[2] eq $_[1]->[2] }

package Gimp::run_mode;

# I guess I now use almost every perl feature available ;)

use overload fallback => 1,
             '0+'     => sub { ${$_[0]} };

package Gimp; # for __DATA__

1;

=cut

=head1 NAME

Gimp - Perl extension for writing Gimp Extensions/Plug-ins/Load & Save-Handlers

This is mostly a reference manual. For a quick intro, look at
L<Gimp::Fu>. For more information, including tutorials, look at the
Gimp-Perl pages at http://gimp.pages.de.

=head1 RATIONALE

Well, scheme (which is used by script-fu), is IMnsHO the crappiest language
ever (well, the crappiest language that one actually can use, so it's not
_that_ bad). Scheme has the worst of all languages, no data types, but still
using variables. Look at haskell (http://www.haskell.org) to see how
functional is done right.

Since I was unable to write a haskell interface (and perl is the traditional
scripting language), I wrote a Perl interface instead. Not too bad a
decision I believe...

=head1 SYNOPSIS

  use Gimp;
  
  Other modules of interest:
  
  use Gimp::Fu;		# easy scripting environment
  use Gimp::PDL;	# interface to the Perl Data Language
  
  these have their own manpage.

=head2 IMPORT TAGS

If you don't specify any import tags, Gimp assumes C<qw/:consts main xlfd_size/>
which is usually what you want.

=over 4

=item :auto

Import useful constants, like RGB, RUN_NONINTERACTIVE... as well as all
libgimp and pdb functions automagically into the caller's namespace. BEWARE!
This will overwrite your AUTOLOAD function, if you have one!

=item :param

Import PARAM_* constants (PARAM_INT32, PARAM_STRING etc.) only.

=item :consts

All constants from gimpenums.h (BG_IMAGE_FILL, RUN_NONINTERACTIVE, NORMAL_MODE, PARAM_INT32 etc.).

=item spawn_options=I<options>

Set default spawn options to I<options>, see L<Gimp::Net>.

=back

The default (unless '' is specified) is C<main xlfd_size :consts>.

=head1 GETTING STARTED

You should first read the Gimp::Fu manpage and then come back. This manpage is mainly
intended for reference purposes.

Also, Dov Grobgeld has written an excellent tutorial for Gimp-Perl. You can
find it at http://imagic.weizmann.ac.il/~dov/gimp/perl-tut.html

=head1 DESCRIPTION

I think you already know what this is about: writing Gimp
plug-ins/extensions/scripts/file-handlers/standalone-scripts, just about
everything you can imagine in perl. If you are missing functionality (look
into TODO first), please feel free contact the author...

Some hilights:

=over 2

=item *
Networked plug-ins and plug-ins using the libgimp interfaces (i.e. to be
started from within The Gimp) look almost the same (if you use the Gimp::Fu
interface, there will be no visible differences at all), you can easily
create hybrid (networked & libgimp) scripts as well.

=item *
Use either a plain pdb (scheme-like) interface or nice object-oriented
syntax, i.e. "gimp_image_new(600,300,RGB)" is the same as "new Image(600,300,RGB)"

=item *
Gimp::Fu will start The Gimp for you, if it cannot connect to an existing
gimp process.

=item *
You can optionally overwrite the pixel-data functions by versions using piddles
(see L<Gimp::PDL>)

=back

noteworthy limitations (subject to be changed):

=over 2

=item *
callback procedures do not poass return values to The Gimp.

=back

=head1 OUTLINE OF A GIMP PLUG-IN

All plug-ins (and extensions etc.) _must_ contain a call to C<Gimp::main>.
The return code should be immediately handed out to exit:

 exit main;		# Gimp::main is exported by default.

Before the call to C<Gimp::main>, I<no> other PDB function must be called.

In a Gimp::Fu-script, you should call C<Gimp::Fu::main> instead:

 exit main;		# Gimp::Fu::main is exported by default as well.

This is similar to Gtk, Tk or similar modules, where you have to call the
main eventloop. Attention: although you call C<exit> with the result of
C<main>, the main function might not actually return. This depends on both
the version of Gimp and the version of the Gimp-Perl module that is in
use.  Do not depend on C<main> to return at all, but still call C<exit>
immediately.

If you need to do cleanups before exiting you should use the C<quit>
callback (which is not yet available if you use Gimp::Fu).

=head1 CALLBACKS

If you use the plain Gimp module (as opposed to Gimp::Fu), your program
should only call one function: C<main>. Everything else is going to be
B<called> from The Gimp at a later stage. For this to work, you should
define certain call-backs in the same module you called C<Gimp::main>:

=over 4

=item init (), query (), quit ()

the standard libgimp callback functions. C<run>() is missing, because this
module will directly call the function you registered with
C<gimp_install_procedure>. Some only make sense for extensions, some
only for normal plug-ins.

=item <installed_procedure>()

The callback for a registered function (C<gimp_install_procedure> and
friends). The arguments from The Gimp are passed as normal arguments
(with the exception of arrays being passed without a preceding count).

The return values from <installed_procedure>() are checked against the
specification, with the exception that a single C<undef> is treated like no
arguments. you can return less, but not more results than specified.

If you C<die> within the callback, the error will be reported to The Gimp
(as soon as The Gimp implements such a functionality) as an execution error.

=item net ()

this is called when the plug-in is not started directly from within the
Gimp, but instead from the B<Net-Server> (the perl network server extension you
hopefully have installed and started ;)

=back

=head1 CALLING GIMP FUNCTIONS

There are two different flavours of gimp-functions. Functions from the
B<PDB> (the Procedural DataBase), and functions from B<libgimp> (the
C-language interface library).

You can get a listing and description of every PDB function by starting the
B<DB Browser> extension in the Gimp-B<Xtns> menu (but remember that B<DB
Browser> is buggy and displays "_" (underscores) as "-" (dashes), so you
can't see the difference between gimp_quit and gimp-quit. As a rule of
thumb, B<Script-Fu> registers scripts with dashes, and everything else uses
underscores).

B<libgimp> functions can't be traced (and won't be traceable in the
foreseeable future).

To call pdb functions (or equivalent libgimp functions), just treat them like
normal perl (this requires the use of the C<:auto> import tag, but see below
for another possibility!):

 gimp_palette_set_foreground([20,5,7]);
 gimp_palette_set_background("cornsilk");

If you don't use the C<:auto> import tag, you can call all Gimp functions
using OO-Syntax:

 Gimp->gimp_palette_set_foreground([20,5,7]);
 Gimp->palette_set_background("cornsilk");
 Palette->set_foreground('#1230f0');

As you can see, you can also drop part of the name prefixes with this
syntax, so its actually shorter to write.

"But how do I call functions containing dashes?". Well, get your favourite
perl book and learn perl! Anyway, newer perls understand a nice syntax (see
also the description for C<gimp_call_procedure>):

 "plug-in-the-egg"->(RUN_INTERACTIVE,$image,$drawable);

Very old perls may need:

 &{"plug-in-the-egg"}(RUN_INTERACTIVE,$image,$drawable);

(unfortunately. the plug-in in this example is actually called
"plug_in_the_egg" *sigh*)

=head1 SPECIAL FUNCTIONS

In this section, you can find descriptions of special functions, functions
having different calling conventions/semantics than I would expect (I
cannot speak for you), or just plain interesting functions. All of these
functions must either be imported explicitly or called using a namespace
override (C<Gimp::>), not as Methods (C<Gimp-E<gt>>).

=over 4

=item main(), Gimp::main()

Should be called immediately when perl is initialized. Arguments are not yet
supported. Initializations can later be done in the init function.

=item xlfd_size(fontname)

This auxillary functions parses the XLFD (usually obtained from a C<PF_FONT>
parameter) and returns its size and unit (e.g. C<(20,POINTS)>). This can
conviniently used in the gimp_text_..._fontname functions, which ignore the
size (no joke ;). Example:

 $drawable->text_fontname (50, 50, "The quick", 5, 1, xlfd_size $font, $font;

=item Gimp::init_gtk()

Initialize Gtk in a similar way the Gimp itself did it. This automatically
parses gimp's gtkrc and sets a variety of default settings (visual,
colormap, gamma, shared memory...).

=item Gimp::init([connection-argument]), Gimp::end()

These is an alternative and experimental interface that replaces the call to
Gimp::main and the net callback. At the moment it only works for the Net
interface (L<Gimp::Net>), and not as a native plug-in. Here's an example:

 use Gimp;
 
 Gimp::init;
 <do something with the gimp>

The optional argument to init has the same format as the GIMP_HOST variable
described in L<Gimp::Net>. Calling C<Gimp::end> is optional.

=item Gimp::lock(), Gimp::unlock()

These functions can be used to gain exclusive access to the Gimp. After
calling lock, all accesses by other clients will be blocked and executed
after the call to unlock. Calls to lock and unlock can be nested.

Currently, these functions only lock the current Perl-Server instance
against exclusive access, they are nops when used via the Gimp::Lib
interface.

=item Gimp::set_rgb_db(filespec)

Use the given rgb database instead of the default one. The format is
the same as the one used by the X11 Consortiums rgb database (you might
have a copy in /usr/lib/X11/rgb.txt). You can view the default database
with C<perldoc -m Gimp>, at the end of the file (the default database is
similar, but not identical to the X11 default rgb.txt)

=item Gimp::initialized()

this function returns true whenever it is safe to clal gimp functions. This is
usually only the case after gimp_main or gimp_init have been called.

=item Gimp::register_callback(gimp_function_name,perl_function)

Using this fucntion you can overwrite the standard Gimp behaviour of
calling a perl subroutine of the same name as the gimp function.

The first argument is the name of a registered gimp function that you want
to overwrite ('perl_fu_make_something'), and the second argument can be
either a name of the corresponding perl sub ('Elsewhere::make_something')
or a code reference (\&my_make).

=back

=head1 SPECIAL METHODS

This chapter descibes methods that behave differently than you might
expect, or methods uniquely implemented in perl (that is, not in the
PDB). All of these must be invoked using the method syntax (C<Gimp-E<gt>>
or C<$object-E<gt>>).

=over 4

=item gimp_install_procedure(name, blurb, help, author, copyright, date, menu_path, image_types, type, [params], [return_vals])

Mostly same as gimp_install_procedure. The parameters and return values for
the functions are specified as an array ref containing either integers or
array-refs with three elements, [PARAM_TYPE, \"NAME\", \"DESCRIPTION\"].

=item gimp_progress_init(message,[])

Initializes a progress bar. In networked modules this is a no-op.

=item gimp_progress_update(percentage)

Updates the progress bar. No-op in networked modules.

=item gimp_tile_*, gimp_pixel_rgn_*, gimp_drawable_get

With these functions you can access the raw pixel data of drawables. They
are documented in L<Gimp::Pixel>, to keep this manual page short.

=item gimp_call_procedure(procname, arguments...)

This function is actually used to implement the fancy stuff. Its your basic
interface to the PDB. Every function call is eventually done through his
function, i.e.:

 gimp_image_new(args...);

is replaced by

 gimp_call_procedure "gimp_image_new",args...;

at runtime.

=item gimp_list_images, gimp_image_get_layers, gimp_image_get_channels

These functions return what you would expect: an array of images, layers or
channels. The reason why this is documented is that the usual way to return
C<PARAM_INT32ARRAY>'s would be to return a B<reference> to an B<array of
integers>, rather than blessed objects.

=item server_eval(string)

This evaluates the given string in array context and returns the
results. It's similar to C<eval>, but with two important differences: the
evaluating always takes place on the server side/server machine (which
might be the same as the local one) and compilation/runtime errors are
reported as runtime errors (i.e. throwing an exception).

=back

=head1 OBJECT ORIENTED SYNTAX

In this manual, only the plain syntax (that lesser languages like C use) is
described. Actually, the recommended way to write gimp scripts is to use the
fancy OO-like syntax you are used to in perl (version 5 at least ;). As a
fact, OO-syntax saves soooo much typing as well. See L<Gimp::OO> for
details.

=head1 DEBUGGING AIDS

No, I can't tell you how to cure immune deficiencies (which might well be
uncurable, as AIDS virii might be able to survive in brain cells, among
other unreachable (for medication) parts of your body), but I I<can> tell
you how Gimp can help you debugging your scripts:

=over 4

=item Gimp::set_trace (tracemask)

Tracking down bugs in gimp scripts is difficult: no sensible error messages.
If anything goes wrong, you only get an execution failure. Switch on
tracing to see which parameters are used to call pdb functions.

This function is never exported, so you have to qualify it when calling.

tracemask is any number of the following flags or'ed together.

=over 8

=item TRACE_NONE

nothing is printed.

=item TRACE_CALL

all pdb calls (and only pdb calls!) are printed
with arguments and return values.

=item TRACE_TYPE

the parameter types are printed additionally.

=item TRACE_NAME

the parameter names are printed.

=item TRACE_DESC

the parameter descriptions.

=item TRACE_ALL

all of the above.

=back

C<set_trace> returns the old tracemask.

=item Gimp::set_trace(\$tracevar)

write trace into $tracevar instead of printing it to STDERR. $tracevar only
contains the last command traces, i.e. it's cleared on every PDB invocation
invocation.

=item Gimp::set_trace(*FILEHANDLE)

write trace to FILEHANDLE instead of STDERR.

=back

=head1 SUPPORTED GIMP DATA TYPES

Gimp supports different data types like colors, regions, strings. In
perl, these are represented as:

=over 4

=item INT32, INT16, INT8, FLOAT, STRING

normal perl scalars. Anything except STRING will be mapped
to a perl-double.

=item INT32ARRAY, INT16ARRAY, INT8ARRAY, FLOATARRAY, STRINGARRAY

array refs containing scalars of the same type, i.e. [1, 2, 3, 4]. Gimp
implicitly swallows or generates a preceeding integer argument because the
preceding argument usually (this is a de-facto standard) contains the number
of elements.

=item COLOR

on input, either an array ref with 3 elements (i.e. [233,40,40]), a X11-like
string ("#rrggbb") or a colour name ("papayawhip") (see set_rgb_db).

=item DISPLAY, IMAGE, LAYER, CHANNEL, DRAWABLE, SELECTION

these will be mapped to corresponding objects (IMAGE => Gimp::Image). In trace
output you will see small integers (the image/layer/etc..-ID)

=item PARASITE

represented as an array ref [name, flags, data], where name and data should be perl strings
and flags is the numerical flag value.

=item REGION, BOUNDARY, PATH, STATUS

Not yet supported (and might never be).

=back

=head1 AUTHOR

Marc Lehmann <pcg@goof.com>

=head1 SEE ALSO

perl(1), gimp(1), L<Gimp::OO>, L<Gimp::Data>, L<Gimp::Pixel>, L<Gimp::PDL>, L<Gimp::Util>, L<Gimp::UI>, L<Gimp::Feature>, L<Gimp::Net>,
L<Gimp::Compat>, L<Gimp::Config>, L<Gimp::Lib>, L<Gimp::Module>, L<scm2perl> and L<scm2scm>.

=cut

__DATA__
240 248 255		AliceBlue
250 235 215		AntiqueWhite
255 239 219		AntiqueWhite1
238 223 204		AntiqueWhite2
205 192 176		AntiqueWhite3
139 131 120		AntiqueWhite4
255 235 205		BlanchedAlmond
138  43 226		BlueViolet
 95 153 159		CadetBlue
152 245 255		CadetBlue1
142 229 238		CadetBlue2
122 197 205		CadetBlue3
 83 134 139		CadetBlue4
 34  34 152		CornflowerBlue
0     0 139		DarkBlue
0   139 139		DarkCyan
184 134  11		DarkGoldenrod
255 185	 15		DarkGoldenrod1
238 173	 14		DarkGoldenrod2
205 149	 12		DarkGoldenrod3
139 101	  8		DarkGoldenrod4
169 169 169		DarkGray
  0  83   0		DarkGreen
169 169 169		DarkGrey
189 183 107		DarkKhaki
139   0 139		DarkMagenta
 85 107  47		DarkOliveGreen
202 255 112		DarkOliveGreen1
188 238 104		DarkOliveGreen2
162 205	 90		DarkOliveGreen3
110 139	 61		DarkOliveGreen4
255 127   0		DarkOrange
255 127	  0		DarkOrange1
238 118	  0		DarkOrange2
205 102	  0		DarkOrange3
139  69	  0		DarkOrange4
153  50 204		DarkOrchid
191  62 255		DarkOrchid1
178  58 238		DarkOrchid2
154  50 205		DarkOrchid3
104  34 139		DarkOrchid4
139   0   0		DarkRed
233 150 122		DarkSalmon
143 188 143		DarkSeaGreen
193 255 193		DarkSeaGreen1
180 238 180		DarkSeaGreen2
155 205 155		DarkSeaGreen3
105 139 105		DarkSeaGreen4
 72  61 139		DarkSlateBlue
 47  79  79		DarkSlateGray
151 255 255		DarkSlateGray1
141 238 238		DarkSlateGray2
121 205 205		DarkSlateGray3
 82 139 139		DarkSlateGray4
 47  79  79		DarkSlateGrey
  0 195 205		DarkTurquoise
148   0 211		DarkViolet
255  20 147		DeepPink
255  20 147		DeepPink1
238  18 137		DeepPink2
205  16 118		DeepPink3
139  10	 80		DeepPink4
  0 191 255		DeepSkyBlue
  0 191 255		DeepSkyBlue1
  0 178 238		DeepSkyBlue2
  0 154 205		DeepSkyBlue3
  0 104 139		DeepSkyBlue4
105 105 105		DimGray
105 105 105		DimGrey
 30 144 255		DodgerBlue
 30 144 255		DodgerBlue1
 28 134 238		DodgerBlue2
 24 116 205		DodgerBlue3
 16  78 139		DodgerBlue4
 58  95 205		FlatMediumBlue
 72 118 255		FlatMediumBlue1
 67 110 238		FlatMediumBlue2
 58  95 205		FlatMediumBlue3
 39  64 139		FlatMediumBlue4
143 188 143		FlatMediumGreen
193 255 193		FlatMediumGreen1
180 238 180		FlatMediumGreen2
155 205 155		FlatMediumGreen3
105 139 105		FlatMediumGreen4
255 250 240		FloralWhite
 34 139  34		ForestGreen
248 248 255		GhostWhite
173 255  47		GreenYellow
255 105 180		HotPink
255 110 180		HotPink1
238 106 167		HotPink2
205  96 144		HotPink3
139  58  98		HotPink4
139  58  58		IndianRed
255 106 106		IndianRed1
238  99	 99		IndianRed2
205  85	 85		IndianRed3
139  58	 58		IndianRed4
255 240 245		LavenderBlush
255 240 245		LavenderBlush1
238 224 229		LavenderBlush2
205 193 197		LavenderBlush3
139 131 134		LavenderBlush4
124 252   0		LawnGreen
255 250 205		LemonChiffon
255 250 205		LemonChiffon1
238 233 191		LemonChiffon2
205 201 165		LemonChiffon3
139 137 112		LemonChiffon4
173 216 230		LightBlue
191 239 255		LightBlue1
178 223 238		LightBlue2
154 192 205		LightBlue3
104 131 139		LightBlue4
240 128 128		LightCoral
224 255 255		LightCyan
224 255 255		LightCyan1
209 238 238		LightCyan2
180 205 205		LightCyan3
122 139 139		LightCyan4
238 221 130		LightGoldenrod
255 236 139		LightGoldenrod1
238 220 130		LightGoldenrod2
205 190 112		LightGoldenrod3
139 129	 76		LightGoldenrod4
250 250 210		LightGoldenrodYellow
211 211 211		LightGray
144 238 144		LightGreen
211 211 211		LightGrey
255 174 185		LightPink
255 174 185		LightPink1
238 162 173		LightPink2
205 140 149		LightPink3
139  95 101		LightPink4
255 160 122		LightSalmon
255 160 122		LightSalmon1
238 149 114		LightSalmon2
205 129	 98		LightSalmon3
139  87	 66		LightSalmon4
 32 178 170		LightSeaGreen
176 226 255		LightSkyBlue
176 226 255		LightSkyBlue1
164 211 238		LightSkyBlue2
141 182 205		LightSkyBlue3
 96 123 139		LightSkyBlue4
132 112 255		LightSlateBlue
119 136 153		LightSlateGray
119 136 153		LightSlateGrey
 176 196 222		LightSteelBlue
202 225 255		LightSteelBlue1
188 210 238		LightSteelBlue2
162 181 205		LightSteelBlue3
110 123 139		LightSteelBlue4
255 255 224		LightYellow
255 255 224		LightYellow1
238 238 209		LightYellow2
205 205 180		LightYellow3
139 139 122		LightYellow4
 50 205  50		LimeGreen
102 205 170		MediumAquamarine
  0   0 205		MediumBlue
107 142  35		MediumForestGreen
184 134  11		MediumGoldenrod
255 185	 15		MediumGoldenrod1
238 173	 14		MediumGoldenrod2
205 149	 12		MediumGoldenrod3
139 101	  8		MediumGoldenrod4
186  85 211		MediumOrchid
224 102 255		MediumOrchid1
209  95 238		MediumOrchid2
180  82 205		MediumOrchid3
122  55 139		MediumOrchid4
255 125 179		MediumPink
255 125 179		MediumPink1
238 116 167		MediumPink2
205 100 144		MediumPink3
139  68	 98		MediumPink4
147 112 219		MediumPurple
171 130 255		MediumPurple1
159 121 238		MediumPurple2
137 104 205		MediumPurple3
 93  71 139		MediumPurple4
 60 179 113		MediumSeaGreen
123 104 238		MediumSlateBlue
127 255   0		MediumSpringGreen
  0 227 238		MediumTurquoise
199  21 133		MediumVioletRed
 25  25 100		MidnightBlue
245 255 250		MintCream
255 228 225		MistyRose
255 228 225		MistyRose1
238 213 210		MistyRose2
205 183 181		MistyRose3
139 125 123		MistyRose4
255 222 173		NavajoWhite
255 222 173		NavajoWhite1
238 207 161		NavajoWhite2
205 179 139		NavajoWhite3
139 121	 94		NavajoWhite4
 34  34 139		NavyBlue
238 221 130		OldGoldenrod
255 236 139		OldGoldenrod1
238 220 130		OldGoldenrod2
205 190 112		OldGoldenrod3
139 129	 76		OldGoldenrod4
253 245 230		OldLace
238 238 175		OldMediumGoldenrod
255 255 187		OldMediumGoldenrod1
238 238 174		OldMediumGoldenrod2
205 205 150		OldMediumGoldenrod3
139 139 102		OldMediumGoldenrod4
107 142  35		OliveDrab
192 255	 62		OliveDrab1
179 238	 58		OliveDrab2
154 205	 50		OliveDrab3
105 139	 34		OliveDrab4
255  69   0		OrangeRed
255  69	  0		OrangeRed1
238  64	  0		OrangeRed2
205  55	  0		OrangeRed3
139  37	  0		OrangeRed4
238 232 170		PaleGoldenrod
152 251 152		PaleGreen
154 255 154		PaleGreen1
144 238 144		PaleGreen2
124 205 124		PaleGreen3
 84 139	 84		PaleGreen4
255 170 200		PalePink
175 238 238		PaleTurquoise
187 255 255		PaleTurquoise1
174 238 238		PaleTurquoise2
150 205 205		PaleTurquoise3
102 139 139		PaleTurquoise4
219 112 147		PaleVioletRed
255 130 171		PaleVioletRed1
238 121 159		PaleVioletRed2
205 104 137		PaleVioletRed3
139  71	 93		PaleVioletRed4
255 239 213		PapayaWhip
255 218 185		PeachPuff
255 218 185		PeachPuff1
238 203 173		PeachPuff2
205 175 149		PeachPuff3
139 119 101		PeachPuff4
176 224 230		PowderBlue
188 143 143		RosyBrown
255 193 193		RosyBrown1
238 180 180		RosyBrown2
205 155 155		RosyBrown3
139 105 105		RosyBrown4
 65 105 225		RoyalBlue
 72 118 255		RoyalBlue1
 67 110 238		RoyalBlue2
 58  95 205		RoyalBlue3
 39  64 139		RoyalBlue4
139  69  19		SaddleBrown
244 164  96		SandyBrown
 46 139  87		SeaGreen
 84 255 159		SeaGreen1
 78 238 148		SeaGreen2
 67 205 128		SeaGreen3
 46 139	 87		SeaGreen4
135 206 255		SkyBlue
135 206 255		SkyBlue1
126 192 238		SkyBlue2
108 166 205		SkyBlue3
 74 112 139		SkyBlue4
106  90 205		SlateBlue
131 111 255		SlateBlue1
122 103 238		SlateBlue2
105  89 205		SlateBlue3
 71  60 139		SlateBlue4
112 128 144		SlateGray
198 226 255		SlateGray1
185 211 238		SlateGray2
159 182 205		SlateGray3
108 123 139		SlateGray4
112 128 144		SlateGrey
  0 255 127		SpringGreen
  0 255 127		SpringGreen1
  0 238 118		SpringGreen2
  0 205 102		SpringGreen3
  0 139	 69		SpringGreen4
 70 130 180		SteelBlue
 99 184 255		SteelBlue1
 92 172 238		SteelBlue2
 79 148 205		SteelBlue3
 54 100 139		SteelBlue4
255  62 150		VioletRed
255  62 150		VioletRed1
238  58 140		VioletRed2
205  50 120		VioletRed3
139  34	 82		VioletRed4
245 245 245		WhiteSmoke
154 205  50		YellowGreen
240 248 255		alice blue
250 235 215		antique white
41 171 151		aquamarine
127 255 212		aquamarine1
118 238 198		aquamarine2
102 205 170		aquamarine3
 69 139 116		aquamarine4
240 255 255		azure
240 255 255		azure1
224 238 238		azure2
193 205 205		azure3
131 139 139		azure4
245 245 220		beige
255 228 196		bisque
255 228 196		bisque1
238 213 183		bisque2
205 183 158		bisque3
139 125 107		bisque4
0 0 0			black
255 235 205		blanched almond
0 0 255			blue
114 33 188		blue violet
  0   0 255		blue1
  0   0 238		blue2
  0   0 205		blue3
  0   0 139		blue4
103 67 0		brown
255  64	 64		brown1
238  59	 59		brown2
205  51	 51		brown3
139  35	 35		brown4
222 184 135		burlywood
255 211 155		burlywood1
238 197 145		burlywood2
205 170 125		burlywood3
139 115	 85		burlywood4
126 125 160		cadet blue
127 255   0		chartreuse
127 255	  0		chartreuse1
118 238	  0		chartreuse2
102 205	  0		chartreuse3
 69 139	  0		chartreuse4
210 105  30		chocolate
255 127	 36		chocolate1
238 118	 33		chocolate2
205 102	 29		chocolate3
139  69	 19		chocolate4
248 137 117		coral
255 114	 86		coral1
238 106	 80		coral2
205  91	 69		coral3
139  62	 47		coral4
100 149 237		cornflower blue
255 248 220		cornsilk
255 248 220		cornsilk1
238 232 205		cornsilk2
205 200 177		cornsilk3
139 136 120		cornsilk4
0 255 255		cyan
  0 255 255		cyan1
  0 238 238		cyan2
  0 205 205		cyan3
  0 139 139		cyan4
0     0 139		dark blue
0   139 139		dark cyan
184 134  11		dark goldenrod
169 169 169		dark gray
0 83 0			dark green
169 169 169		dark grey
189 183 107		dark khaki
139   0 139		dark magenta
79 79 47		dark olive green
255 127   0		dark orange
106 37 102		dark orchid
139   0   0		dark red
233 150 122		dark salmon
143 188 143		dark sea green
51 62 99		dark slate blue
60 64 74		dark slate gray
60 64 74		dark slate grey
29 111 117		dark turquoise
148   0 211		dark violet
255  20 147		deep pink
  0 191 255		deep sky blue
105 105 105		dim gray
105 105 105		dim grey
 30 144 255		dodger blue
136 18 13		firebrick
255  48	 48		firebrick1
238  44	 44		firebrick2
205  38	 38		firebrick3
139  26	 26		firebrick4
 58  95 205		flat medium blue
143 188 143		flat medium green
255 250 240		floral white
85 192 52		forest green
220 220 220		gainsboro
248 248 255		ghost white
254 197 68		gold
255 215	  0		gold1
238 201	  0		gold2
205 173	  0		gold3
139 117	  0		gold4
218 165 32		goldenrod
255 193	 37		goldenrod1
238 180	 34		goldenrod2
205 155	 29		goldenrod3
139 105	 20		goldenrod4
174 174 174		gray
0 0 0 			gray0
3 3 3 			gray1
26 26 26 		gray10
255 255 255 		gray100
28 28 28 		gray11
31 31 31 		gray12
33 33 33 		gray13
36 36 36 		gray14
38 38 38 		gray15
41 41 41 		gray16
43 43 43 		gray17
46 46 46 		gray18
48 48 48 		gray19
5 5 5 			gray2
51 51 51 		gray20
54 54 54 		gray21
56 56 56 		gray22
59 59 59 		gray23
61 61 61 		gray24
64 64 64 		gray25
66 66 66 		gray26
69 69 69 		gray27
71 71 71 		gray28
74 74 74 		gray29
8 8 8 			gray3
77 77 77 		gray30
79 79 79 		gray31
82 82 82 		gray32
84 84 84 		gray33
87 87 87 		gray34
89 89 89 		gray35
92 92 92 		gray36
94 94 94 		gray37
97 97 97 		gray38
99 99 99 		gray39
10 10 10 		gray4
102 102 102 		gray40
105 105 105 		gray41
107 107 107 		gray42
110 110 110 		gray43
112 112 112 		gray44
115 115 115 		gray45
117 117 117 		gray46
120 120 120 		gray47
122 122 122 		gray48
125 125 125 		gray49
13 13 13 		gray5
127 127 127 		gray50
130 130 130 		gray51
133 133 133 		gray52
135 135 135 		gray53
138 138 138 		gray54
140 140 140 		gray55
143 143 143 		gray56
145 145 145 		gray57
148 148 148 		gray58
150 150 150 		gray59
15 15 15 		gray6
153 153 153 		gray60
156 156 156 		gray61
158 158 158 		gray62
161 161 161 		gray63
163 163 163 		gray64
166 166 166 		gray65
168 168 168 		gray66
171 171 171 		gray67
173 173 173 		gray68
176 176 176 		gray69
18 18 18 		gray7
179 179 179 		gray70
181 181 181 		gray71
184 184 184 		gray72
186 186 186 		gray73
189 189 189 		gray74
191 191 191 		gray75
194 194 194 		gray76
196 196 196 		gray77
199 199 199 		gray78
201 201 201 		gray79
20 20 20 		gray8
204 204 204 		gray80
207 207 207 		gray81
209 209 209 		gray82
212 212 212 		gray83
214 214 214 		gray84
217 217 217 		gray85
219 219 219 		gray86
222 222 222 		gray87
224 224 224 		gray88
227 227 227 		gray89
23 23 23 		gray9
229 229 229 		gray90
232 232 232 		gray91
235 235 235 		gray92
237 237 237 		gray93
240 240 240 		gray94
242 242 242 		gray95
245 245 245 		gray96
247 247 247 		gray97
250 250 250 		gray98
252 252 252 		gray99
0 255 0			green
159 211 0		green yellow
  0 255	  0		green1
  0 238	  0		green2
  0 205	  0		green3
  0 139	  0		green4
174 174 174		grey
0 0 0 			grey0
3 3 3 			grey1
26 26 26 		grey10
255 255 255 		grey100
28 28 28 		grey11
31 31 31 		grey12
33 33 33 		grey13
36 36 36 		grey14
38 38 38 		grey15
41 41 41 		grey16
43 43 43 		grey17
46 46 46 		grey18
48 48 48 		grey19
5 5 5 			grey2
51 51 51 		grey20
54 54 54 		grey21
56 56 56 		grey22
59 59 59 		grey23
61 61 61 		grey24
64 64 64 		grey25
66 66 66 		grey26
69 69 69 		grey27
71 71 71 		grey28
74 74 74 		grey29
8 8 8 			grey3
77 77 77 		grey30
79 79 79 		grey31
82 82 82 		grey32
84 84 84 		grey33
87 87 87 		grey34
89 89 89 		grey35
92 92 92 		grey36
94 94 94 		grey37
97 97 97 		grey38
99 99 99 		grey39
10 10 10 		grey4
102 102 102 		grey40
105 105 105 		grey41
107 107 107 		grey42
110 110 110 		grey43
112 112 112 		grey44
115 115 115 		grey45
117 117 117 		grey46
120 120 120 		grey47
122 122 122 		grey48
125 125 125 		grey49
13 13 13 		grey5
127 127 127 		grey50
130 130 130 		grey51
133 133 133 		grey52
135 135 135 		grey53
138 138 138 		grey54
140 140 140 		grey55
143 143 143 		grey56
145 145 145 		grey57
148 148 148 		grey58
150 150 150 		grey59
15 15 15 		grey6
153 153 153 		grey60
156 156 156 		grey61
158 158 158 		grey62
161 161 161 		grey63
163 163 163 		grey64
166 166 166 		grey65
168 168 168 		grey66
171 171 171 		grey67
173 173 173 		grey68
176 176 176 		grey69
18 18 18 		grey7
179 179 179 		grey70
181 181 181 		grey71
184 184 184 		grey72
186 186 186 		grey73
189 189 189 		grey74
191 191 191 		grey75
194 194 194 		grey76
196 196 196 		grey77
199 199 199 		grey78
201 201 201 		grey79
20 20 20 		grey8
204 204 204 		grey80
207 207 207 		grey81
209 209 209 		grey82
212 212 212 		grey83
214 214 214 		grey84
217 217 217 		grey85
219 219 219 		grey86
222 222 222 		grey87
224 224 224 		grey88
227 227 227 		grey89
23 23 23 		grey9
229 229 229 		grey90
232 232 232 		grey91
235 235 235 		grey92
237 237 237 		grey93
240 240 240 		grey94
242 242 242 		grey95
245 245 245 		grey96
247 247 247 		grey97
250 250 250 		grey98
252 252 252 		grey99
240 255 240		honeydew
240 255 240		honeydew1
224 238 224		honeydew2
193 205 193		honeydew3
131 139 131		honeydew4
255 105 180		hot pink
101 46 46		indian red
255 255 240		ivory
255 255 240		ivory1
238 238 224		ivory2
205 205 193		ivory3
139 139 131		ivory4
189 167 107		khaki
255 246 143		khaki1
238 230 133		khaki2
205 198 115		khaki3
139 134	 78		khaki4
230 230 250		lavender
255 240 245		lavender blush
124 252   0		lawn green
255 250 205		lemon chiffon
171 197 255		light blue
240 128 128		light coral
224 255 255		light cyan
238 221 130		light goldenrod
250 250 210		light goldenrod yellow
211 211 211		light gray
144 238 144		light green
211 211 211		light grey
255 174 185		light pink
255 160 122		light salmon
 32 178 170		light sea green
176 226 255		light sky blue
132 112 255		light slate blue
119 136 153		light slate gray
119 136 153		light slate grey
52 152 202		light steel blue
255 255 224		light yellow
46 155 28		lime green
250 240 230		linen
255 0 211		magenta
255   0 255		magenta1
238   0 238		magenta2
205   0 205		magenta3
139   0 139		magenta4
103 7 72		maroon
255  52 179		maroon1
238  48 167		maroon2
205  41 144		maroon3
139  28	 98		maroon4
21 135 118		medium aquamarine
61 98 208		medium blue
107 142 35		medium forest green
184 134 11		medium goldenrod
172 77 166		medium orchid
255 125 179		medium pink
147 112 219		medium purple
27 134 86		medium sea green
95 109 154		medium slate blue
60 141 35		medium spring green
62 172 181		medium turquoise
199 21 133		medium violet red
12 62 99		midnight blue
245 255 250		mint cream
255 228 225		misty rose
255 228 181		moccasin
255 222 173		navajo white
0 0 142			navy
0 0 142			navy blue
238 221 130		old goldenrod
253 245 230		old lace
238 238 175		old medium goldenrod
107 142  35		olive drab
255 138 0		orange
226 65 42		orange red
255 165	  0		orange1
238 154	  0		orange2
205 133	  0		orange3
139  90	  0		orange4
218 107 212		orchid
255 131 250		orchid1
238 122 233		orchid2
205 105 201		orchid3
139  71 137		orchid4
238 232 170		pale goldenrod
152 255 152		pale green
255 170 200		pale pink
175 238 238		pale turquoise
219 112 147		pale violet red
255 239 213		papaya whip
255 218 185		peach puff
205 133  63		peru
255 174 185		pink
255 181 197		pink1
238 169 184		pink2
205 145 158		pink3
139  99 108		pink4
76 46 87		plum
255 187 255		plum1
238 174 238		plum2
205 150 205		plum3
139 102 139		plum4
176 224 230		powder blue
138  43 226		purple
155  48 255		purple1
145  44 238		purple2
125  38 205		purple3
 85  26 139		purple4
255 0 0			red
255   0	  0		red1
238   0	  0		red2
205   0	  0		red3
139   0	  0		red4
188 143 143		rosy brown
 65 105 225		royal blue
139  69  19		saddle brown
248 109 104		salmon
255 140 105		salmon1
238 130	 98		salmon2
205 112	 84		salmon3
139  76	 57		salmon4
178 143 86		sandy brown
43 167 112		sea green
255 245 238		seashell
255 245 238		seashell1
238 229 222		seashell2
205 197 191		seashell3
139 134 130		seashell4
142 107 35		sienna
255 130	 71		sienna1
238 121	 66		sienna2
205 104	 57		sienna3
139  71	 38		sienna4
0 138 255		sky blue
117 134 190		slate blue
112 128 144		slate gray
112 128 144		slate grey
255 250 250		snow
255 250 250		snow1
238 233 233		snow2
205 201 201		snow3
139 137 137		snow4
0 255 159		spring green
55 121 153		steel blue
176 155 125		tan
255 165	 79		tan1
238 154	 73		tan2
205 133	 63		tan3
139  90	 43		tan4
146 62 112		thistle
255 225 255		thistle1
238 210 238		thistle2
205 181 205		thistle3
139 123 139		thistle4
255  99  71		tomato
255  99	 71		tomato1
238  92	 66		tomato2
205  79	 57		tomato3
139  54	 38		tomato4
72 209 204		turquoise
  0 245 255		turquoise1
  0 229 238		turquoise2
  0 197 205		turquoise3
  0 134 139		turquoise4
148 0 211		violet
255 0 148		violet red
229 199 117		wheat
255 231 186		wheat1
238 216 174		wheat2
205 186 150		wheat3
139 126 102		wheat4
255 255 255		white
245 245 245		white smoke
255 255 0		yellow
75 211 0 		yellow green
255 255	  0		yellow1
238 238	  0		yellow2
205 205	  0		yellow3
139 139	  0		yellow4
