package Gimp;

use strict 'vars';
use Carp;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK $AUTOLOAD %EXPORT_TAGS @EXPORT_FAIL
            @_consts @_procs $interface_pkg $interface_type @_param @_al_consts
            @PREFIXES $_PROT_VERSION
            @gimp_gui_functions
            $help $verbose $host);

use base qw(DynaLoader);

require DynaLoader;

$VERSION = 1.061;

@_param = qw(
	PARAM_BOUNDARY	PARAM_CHANNEL	PARAM_COLOR	PARAM_DISPLAY	PARAM_DRAWABLE
	PARAM_END	PARAM_FLOAT	PARAM_IMAGE	PARAM_INT32	PARAM_FLOATARRAY
	PARAM_INT16	PARAM_PARASITE	PARAM_STRING	PARAM_PATH	PARAM_INT16ARRAY
	PARAM_INT8	PARAM_INT8ARRAY	PARAM_LAYER	PARAM_REGION	PARAM_STRINGARRAY
	PARAM_SELECTION	PARAM_STATUS	PARAM_INT32ARRAY
);

# constants that, in some earlier version, were autoloaded
@_consts = (@_param,qw(
	ADDITION_MODE	ALPHA_MASK	APPLY		BEHIND_MODE	BG_BUCKET_FILL
	BG_IMAGE_FILL	BILINEAR	BLACK_MASK	BLUE_CHANNEL	BLUR
	CLIP_TO_BOTTOM_LAYER		CLIP_TO_IMAGE	COLOR_MODE	CONICAL_ASYMMETRIC
	CONICAL_SYMMETRIC	CUSTOM	DARKEN_ONLY_MODE		DIFFERENCE_MODE
	DISCARD		DISSOLVE_MODE	EXPAND_AS_NECESSARY		FG_BG_HSV
	FG_BG_RGB	FG_BUCKET_FILL	FG_TRANS	GRAY		GRAYA_IMAGE
	GRAY_CHANNEL	GRAY_IMAGE	GREEN_CHANNEL	HUE_MODE	IMAGE_CLONE
	INDEXED		INDEXEDA_IMAGE	INDEXED_CHANNEL	INDEXED_IMAGE	LIGHTEN_ONLY_MODE
	LINEAR		MULTIPLY_MODE	NORMAL_MODE	NO_IMAGE_FILL	OVERLAY_MODE
	PATTERN_CLONE	PIXELS		POINTS		PATTERN_BUCKET_FILL
	PROC_EXTENSION	PROC_PLUG_IN	PROC_TEMPORARY	RADIAL		RED_CHANNEL
	REPEAT_NONE	REPEAT_SAWTOOTH	FG_IMAGE_FILL	WHITE_MASK	REPEAT_TRIANGULAR
	RGBA_IMAGE	RGB_IMAGE	RUN_INTERACTIVE	RUN_NONINTERACTIVE
	SATURATION_MODE	SCREEN_MODE	SELECTION_ADD	RGB		RUN_WITH_LAST_VALS
	SELECTION_INTERSECT		SELECTION_REPLACE		SELECTION_SUB
	SHAPEBURST_ANGULAR		SHAPEBURST_DIMPLED		SHAPEBURST_SPHERICAL
	SHARPEN		SQUARE		STATUS_CALLING_ERROR		STATUS_EXECUTION_ERROR
	STATUS_PASS_THROUGH		STATUS_SUCCESS	SUBTRACT_MODE	TRANS_IMAGE_FILL
	VALUE_MODE	DIVIDE_MODE	PARASITE_PERSISTANT		WHITE_IMAGE_FILL
        SPIRAL_CLOCKWISE		SPIRAL_ANTICLOCKWISE
	
	TRACE_NONE	TRACE_CALL	TRACE_TYPE	TRACE_NAME	TRACE_DESC
	TRACE_ALL
	
	MESSAGE_BOX	CONSOLE
	
	ALL_HUES	RED_HUES	YELLOW_HUES	GREEN_HUES	CYAN_HUES
	BLUE_HUES	MAGENTA_HUES
	
	HORIZONTAL	VERTICAL
));

@_procs = qw(
	main
);

bootstrap Gimp $VERSION;

# defs missing from libgimp

sub BEHIND_MODE		(){ 2 };

sub FG_BG_RGB		(){ 0 };
sub FG_BG_HSV		(){ 1 };
sub FG_TRANS		(){ 2 };
sub CUSTOM		(){ 3 };

sub LINEAR		(){ 0 };
sub BILINEAR		(){ 1 };
sub RADIAL		(){ 2 };
sub SQUARE		(){ 3 };
sub CONICAL_SYMMETRIC	(){ 4 };
sub CONICAL_ASYMMETRIC	(){ 5 };
sub SHAPEBURST_ANGULAR	(){ 6 };
sub SHAPEBURST_SPHERICAL(){ 7 };
sub SHAPEBURST_DIMPLED	(){ 8 };
sub SPIRAL_CLOCKWISE	(){ 9 };
sub SPIRAL_ANTICLOCKWISE(){10 };

sub REPEAT_NONE		(){ 0 };
sub REPEAT_SAWTOOTH	(){ 1 };
sub REPEAT_TRIANGULAR	(){ 2 };

sub FG_BUCKET_FILL	(){ 0 };
sub BG_BUCKET_FILL	(){ 1 };
sub PATTERN_BUCKET_FILL	(){ 2 };

sub RED_CHANNEL		(){ 0 };
sub GREEN_CHANNEL	(){ 1 };
sub BLUE_CHANNEL	(){ 2 };
sub GRAY_CHANNEL	(){ 3 };
sub INDEXED_CHANNEL	(){ 4 };

sub WHITE_MASK		(){ 0 };
sub BLACK_MASK		(){ 1 };
sub ALPHA_MASK		(){ 2 };

sub APPLY		(){ 0 };
sub DISCARD		(){ 1 };

sub EXPAND_AS_NECESSARY	(){ 0 };
sub CLIP_TO_IMAGE	(){ 1 };
sub CLIP_TO_BOTTOM_LAYER(){ 2 };

sub SELECTION_ADD	(){ 0 };
sub SELECTION_SUB	(){ 1 };
sub SELECTION_REPLACE	(){ 2 };
sub SELECTION_INTERSECT	(){ 3 };

sub PIXELS		(){ 0 };
sub POINTS		(){ 1 };

sub IMAGE_CLONE		(){ 0 };
sub PATTERN_CLONE	(){ 1 };

sub BLUR		(){ 0 };
sub SHARPEN		(){ 1 };

sub ALL_HUES		(){ 0 };
sub RED_HUES		(){ 1 };
sub YELLOW_HUES		(){ 2 };
sub GREEN_HUES		(){ 3 };
sub CYAN_HUES		(){ 4 };
sub BLUE_HUES		(){ 5 };
sub MAGENTA_HUES	(){ 6 };

sub MESSAGE_BOX		(){ 0 };
sub CONSOLE		(){ 1 };

sub SHADOWS		(){ 0 };
sub MIDTONES		(){ 1 };
sub HIGHLIGHTS		(){ 2 };

sub HORIZONTAL		(){ 0 };
sub VERTICAL		(){ 1 };

# internal constants shared with Perl-Server

sub _PS_FLAG_QUIET	{ 0000000001 };	# do not output messages
sub _PS_FLAG_BATCH	{ 0000000002 }; # started via Gimp::Net, extra = filehandle

$_PROT_VERSION	= "2";			# protocol version

# we really abuse the import facility..
sub import($;@) {
   my $pkg = shift;
   my $up = caller();
   my @export;

   # make a quick but dirty guess ;)
   
   @_=qw(main xlfd_size :auto) unless @_;
   
   for(@_) {
      if ($_ eq ":auto") {
         push(@export,@_consts,@_procs);
         *{"${up}::AUTOLOAD"} = sub {
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
      } elsif ($_ ne "") {
         push(@export,$_);
      } elsif ($_ eq "") {
         #nop #d#FIXME, Perl-Server requires this!
      } else {
         croak "$_ is not a valid import tag for package $pkg";
      }
   }
   
   for(@export) {
      *{"${up}::$_"} = \&$_;
   }
}

sub xlfd_size($) {
  local $^W=0;
  my ($px,$pt)=(split(/-/,$_[0]))[7,8];
  $px>0 ? ($px    ,&Gimp::PIXELS)
        : ($pt*0.1,&Gimp::POINTS);
}

# internal utility function for Gimp::Fu and others
sub wrap_text {
   my $x=$_[0];
   $x=~s/(\G.{1,$_[1]})(\s+|$)/$1\n/g;
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
   } elsif ($_[0] =~ /^#([0-9a-f]{2,2})([0-9a-f]{2,2})([0-9a-f]{2,2})$/) {
      [map {eval "0x$_"} ($1,$2,$3)];
   } else {
      unless (defined %rgb_db) {
         if ($rgb_db_path) {
            open RGB,"<$rgb_db_path" or croak "unable to open $rgb_db_path";
         } else {
            *RGB=*DATA;
         }
         while(<RGB>) {
            next unless /^\s*(\d+)\s+(\d+)\s+(\d+)\s+(.+?)\s*$/;
            $rgb_db{lc($4)}=[$1,$2,$3];
         }
         close RGB if defined $rgb_db_path;
      }
      if ($rgb_db{lc($_[0])}) {
         return $rgb_db{lc($_[0])};
      } else {
         croak "Unable to grok '".join(",",@_),"' as colour specifier";
      }
   }
}

*canonicalize_color = \&canonicalize_colour;

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

sub _initialized_callback {
   if (@log) {
      Gimp->_gimp_append_data ('gimp-perl-log', map join("\1",@$_)."\0",@log);
      @log=();
   }
}

# message
# function
# fatal
sub logger {
   my %args = @_;
   my $file=$0;
   $file=~s/^.*[\\\/]//;
   $args{message}  = "unknown message"    unless defined $args{message};
   $args{function} = ""                   unless defined $args{function};
   $args{fatal}    = 1                    unless defined $args{fatal};
   print STDERR "$file: $args{message} (for function $args{function})\n" if $verbose || $interface_type eq 'net';
   push(@log,[$file,@args{'function','message','fatal'}]);
   _initialized_callback if initialized();
}

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
   *$_ = \&{"${interface_pkg}::$_"};
}

*main               = \&{"${interface_pkg}::gimp_main"};
*init               = \&{"${interface_pkg}::gimp_init"};
*end                = \&{"${interface_pkg}::gimp_end" };

*lock  = \&{"${interface_pkg}::lock" };
*unlock= \&{"${interface_pkg}::unlock" };

@PREFIXES=("gimp_", "");

my %ignore_function = ();

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
  Carp::croak $_[0];
}

sub AUTOLOAD {
   my ($class,$name) = $AUTOLOAD =~ /^(.*)::(.*?)$/;
   for(@{"${class}::PREFIXES"}) {
      my $sub = $_.$name;
      if (exists $ignore_function{$sub}) {
        *{$AUTOLOAD} = sub { () };
        goto &$AUTOLOAD;
      } elsif (UNIVERSAL::can(Gimp::Util,$sub)) {
         my $ref = \&{"Gimp::Util::$sub"};
         *{$AUTOLOAD} = sub {
            shift unless ref $_[0];
#               goto &$ref # does not always work, PERLBUG! #FIXME
            my @r = eval { &$ref };
            _croak $@ if $@;
            wantarray ? @r : $r[0];
         };
         goto &$AUTOLOAD;
      } elsif (UNIVERSAL::can($interface_pkg,$sub)) {
         my $ref = \&{"${interface_pkg}::$sub"};
         *{$AUTOLOAD} = sub {
            shift unless ref $_[0];
#               goto &$ref;	# does not always work, PERLBUG! #FIXME
            my @r = eval { &$ref };
            _croak $@ if $@;
            wantarray ? @r : $r[0];
         };
         goto &$AUTOLOAD;
      } elsif (_gimp_procedure_available ($sub)) {
         *{$AUTOLOAD} = sub {
            shift unless ref $_[0];
#               goto gimp_call_procedure # does not always work, PERLBUG! #FIXME
            my @r=eval { gimp_call_procedure ($sub,@_) };
            _croak $@ if $@;
            wantarray ? @r : $r[0];
         };
         goto &$AUTOLOAD;
      } elsif (defined(*{"${interface_pkg}::$sub"}{CODE})) {
         die "safety net $interface_pkg :: $sub (REPORT THIS!!)";#d#
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
  *{"Gimp::${class}::AUTOLOAD"} = \&AUTOLOAD;
  push(@{"${class}::ISA"}		, "Gimp::${class}");
  push(@{"Gimp::${class}::PREFIXES"}	, @prefixes); @prefixes=@{"Gimp::${class}::PREFIXES"};
  push(@{"${class}::PREFIXES"}		, @prefixes); @prefixes=@{"${class}::PREFIXES"};
}

_pseudoclass qw(Layer		gimp_layer_ gimp_drawable_ gimp_floating_sel_ gimp_image_ gimp_ plug_in_);
_pseudoclass qw(Image		gimp_image_ gimp_drawable_ gimp_ plug_in_);
_pseudoclass qw(Drawable	gimp_drawable_ gimp_layer_ gimp_image_ gimp_ plug_in_);
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
_pseudoclass qw(GDrawable	gimp_drawable_);
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

sub data {
   my $self = shift;
   $self->set_data(@_) if @_;
   defined(wantarray) ? $self->get_data : undef;
}

package Gimp::GDrawable;

sub pixel_rgn($$$$$$) {
   Gimp::gimp_pixel_rgn_init(@_);
}

package Gimp::PixelRgn;

push(@PixelRgn::ISA, "Gimp::PixelRgn");

sub new($$$$$$$$) {
   shift;
   init Gimp::PixelRgn(@_);
}

package Gimp::Parasite;

sub is_type($$)		{ $_[0]->[0] eq $_[1] }
sub is_persistant($)	{ $_[0]->[1] & PARASITE_PERSISTANT }
sub is_error($)		{ $_[0]->is_type("error") }
sub error($)		{ ["error", 0, ""] }
sub copy($)		{ [@{$_[0]}] }

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

If you don't specify any import tags, Gimp assumes :consts
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

=back

The default (unless '' is specified) is C<main xlfd_size :auto>.

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
main eventloop.

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
having different calling conventions/semantics than I would expect (I cannot
speak for you), or just plain interesting functions.

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

=item Gimp::init([connection-argument]), Gimp::end()

These is an alternative and experimental interface that replaces the call to
Gimp::main and the net callback. At the moment it only works for the Net
interface (L<Gimp::Net>), and not as a native plug-in. Here's an example:

 use Gimp;
 
 Gimp::init;
 <do something with the gimp>
 Gimp::end;

The optional argument to init has the same format as the GIMP_HOST variable
described in L<Gimp::Net>.

=item Gimp::lock(), Gimp::unlock()

These functions can be used to gain exclusive access to the Gimp. After
calling lock, all accesses by other clients will be blocked and executed
after the call to unlock. Calls to lock and unlock can be nested.

Currently, these functions only lock the current Perl-Server instance
against exclusive access, they are nops when used via the Gimp::Lib
interface.

=item gimp_install_procedure(name, blurb, help, author, copyright, date, menu_path, image_types, type, [params], [return_vals])

Mostly same as gimp_install_procedure. The parameters and return values for
the functions are specified as an array ref containing either integers or
array-refs with three elements, [PARAM_TYPE, \"NAME\", \"DESCRIPTION\"].

=item gimp_progress_init(message)

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

=item set_rgb_db filespec

Use the given rgb database instead of the default one. The format is the
same as the one used by the X11 Consortiums rgb database (you might have a
copy in /usr/lib/X11/rgb.txt). You can view the default database with
C<perldoc -m Gimp>, at the end of the file.

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

=item set_trace (tracemask)

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

=item set_trace(\$tracevar)

write trace into $tracevar instead of printing it to STDERR. $tracevar only
contains the last command traces, i.e. it's cleared on every PDB invocation
invocation.

=item set_trace(*FILEHANDLE)

write trace to FILEHANDLE instead of STDERR.

=item initialized ()

this function returns true whenever it is safe to clal gimp functions. This is
usually only the case after gimp_main or gimp_init have been called.

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
L<Gimp::Lib>, L<scm2perl> and L<scm2scm>.

=cut

__DATA__
255 250 250		snow
248 248 255		ghost white
248 248 255		GhostWhite
245 245 245		white smoke
245 245 245		WhiteSmoke
220 220 220		gainsboro
255 250 240		floral white
255 250 240		FloralWhite
253 245 230		old lace
253 245 230		OldLace
250 240 230		linen
250 235 215		antique white
250 235 215		AntiqueWhite
255 239 213		papaya whip
255 239 213		PapayaWhip
255 235 205		blanched almond
255 235 205		BlanchedAlmond
255 228 196		bisque
255 218 185		peach puff
255 218 185		PeachPuff
255 222 173		navajo white
255 222 173		NavajoWhite
255 228 181		moccasin
255 248 220		cornsilk
255 255 240		ivory
255 250 205		lemon chiffon
255 250 205		LemonChiffon
255 245 238		seashell
240 255 240		honeydew
245 255 250		mint cream
245 255 250		MintCream
240 255 255		azure
240 248 255		alice blue
240 248 255		AliceBlue
230 230 250		lavender
255 240 245		lavender blush
255 240 245		LavenderBlush
255 228 225		misty rose
255 228 225		MistyRose
255 255 255		white
  0   0   0		black
 47  79  79		dark slate gray
 47  79  79		DarkSlateGray
 47  79  79		dark slate grey
 47  79  79		DarkSlateGrey
105 105 105		dim gray
105 105 105		DimGray
105 105 105		dim grey
105 105 105		DimGrey
112 128 144		slate gray
112 128 144		SlateGray
112 128 144		slate grey
112 128 144		SlateGrey
119 136 153		light slate gray
119 136 153		LightSlateGray
119 136 153		light slate grey
119 136 153		LightSlateGrey
190 190 190		gray
190 190 190		grey
211 211 211		light grey
211 211 211		LightGrey
211 211 211		light gray
211 211 211		LightGray
 25  25 112		midnight blue
 25  25 112		MidnightBlue
  0   0 128		navy
  0   0 128		navy blue
  0   0 128		NavyBlue
100 149 237		cornflower blue
100 149 237		CornflowerBlue
 72  61 139		dark slate blue
 72  61 139		DarkSlateBlue
106  90 205		slate blue
106  90 205		SlateBlue
123 104 238		medium slate blue
123 104 238		MediumSlateBlue
132 112 255		light slate blue
132 112 255		LightSlateBlue
  0   0 205		medium blue
  0   0 205		MediumBlue
 65 105 225		royal blue
 65 105 225		RoyalBlue
  0   0 255		blue
 30 144 255		dodger blue
 30 144 255		DodgerBlue
  0 191 255		deep sky blue
  0 191 255		DeepSkyBlue
135 206 235		sky blue
135 206 235		SkyBlue
135 206 250		light sky blue
135 206 250		LightSkyBlue
 70 130 180		steel blue
 70 130 180		SteelBlue
176 196 222		light steel blue
176 196 222		LightSteelBlue
173 216 230		light blue
173 216 230		LightBlue
176 224 230		powder blue
176 224 230		PowderBlue
175 238 238		pale turquoise
175 238 238		PaleTurquoise
  0 206 209		dark turquoise
  0 206 209		DarkTurquoise
 72 209 204		medium turquoise
 72 209 204		MediumTurquoise
 64 224 208		turquoise
  0 255 255		cyan
224 255 255		light cyan
224 255 255		LightCyan
 95 158 160		cadet blue
 95 158 160		CadetBlue
102 205 170		medium aquamarine
102 205 170		MediumAquamarine
127 255 212		aquamarine
  0 100   0		dark green
  0 100   0		DarkGreen
 85 107  47		dark olive green
 85 107  47		DarkOliveGreen
143 188 143		dark sea green
143 188 143		DarkSeaGreen
 46 139  87		sea green
 46 139  87		SeaGreen
 60 179 113		medium sea green
 60 179 113		MediumSeaGreen
 32 178 170		light sea green
 32 178 170		LightSeaGreen
152 251 152		pale green
152 251 152		PaleGreen
  0 255 127		spring green
  0 255 127		SpringGreen
124 252   0		lawn green
124 252   0		LawnGreen
  0 255   0		green
127 255   0		chartreuse
  0 250 154		medium spring green
  0 250 154		MediumSpringGreen
173 255  47		green yellow
173 255  47		GreenYellow
 50 205  50		lime green
 50 205  50		LimeGreen
154 205  50		yellow green
154 205  50		YellowGreen
 34 139  34		forest green
 34 139  34		ForestGreen
107 142  35		olive drab
107 142  35		OliveDrab
189 183 107		dark khaki
189 183 107		DarkKhaki
240 230 140		khaki
238 232 170		pale goldenrod
238 232 170		PaleGoldenrod
250 250 210		light goldenrod yellow
250 250 210		LightGoldenrodYellow
255 255 224		light yellow
255 255 224		LightYellow
255 255   0		yellow
255 215   0 		gold
238 221 130		light goldenrod
238 221 130		LightGoldenrod
218 165  32		goldenrod
184 134  11		dark goldenrod
184 134  11		DarkGoldenrod
188 143 143		rosy brown
188 143 143		RosyBrown
205  92  92		indian red
205  92  92		IndianRed
139  69  19		saddle brown
139  69  19		SaddleBrown
160  82  45		sienna
205 133  63		peru
222 184 135		burlywood
245 245 220		beige
245 222 179		wheat
244 164  96		sandy brown
244 164  96		SandyBrown
210 180 140		tan
210 105  30		chocolate
178  34  34		firebrick
165  42  42		brown
233 150 122		dark salmon
233 150 122		DarkSalmon
250 128 114		salmon
255 160 122		light salmon
255 160 122		LightSalmon
255 165   0		orange
255 140   0		dark orange
255 140   0		DarkOrange
255 127  80		coral
240 128 128		light coral
240 128 128		LightCoral
255  99  71		tomato
255  69   0		orange red
255  69   0		OrangeRed
255   0   0		red
255 105 180		hot pink
255 105 180		HotPink
255  20 147		deep pink
255  20 147		DeepPink
255 192 203		pink
255 182 193		light pink
255 182 193		LightPink
219 112 147		pale violet red
219 112 147		PaleVioletRed
176  48  96		maroon
199  21 133		medium violet red
199  21 133		MediumVioletRed
208  32 144		violet red
208  32 144		VioletRed
255   0 255		magenta
238 130 238		violet
221 160 221		plum
218 112 214		orchid
186  85 211		medium orchid
186  85 211		MediumOrchid
153  50 204		dark orchid
153  50 204		DarkOrchid
148   0 211		dark violet
148   0 211		DarkViolet
138  43 226		blue violet
138  43 226		BlueViolet
160  32 240		purple
147 112 219		medium purple
147 112 219		MediumPurple
216 191 216		thistle
255 250 250		snow1
238 233 233		snow2
205 201 201		snow3
139 137 137		snow4
255 245 238		seashell1
238 229 222		seashell2
205 197 191		seashell3
139 134 130		seashell4
255 239 219		AntiqueWhite1
238 223 204		AntiqueWhite2
205 192 176		AntiqueWhite3
139 131 120		AntiqueWhite4
255 228 196		bisque1
238 213 183		bisque2
205 183 158		bisque3
139 125 107		bisque4
255 218 185		PeachPuff1
238 203 173		PeachPuff2
205 175 149		PeachPuff3
139 119 101		PeachPuff4
255 222 173		NavajoWhite1
238 207 161		NavajoWhite2
205 179 139		NavajoWhite3
139 121	 94		NavajoWhite4
255 250 205		LemonChiffon1
238 233 191		LemonChiffon2
205 201 165		LemonChiffon3
139 137 112		LemonChiffon4
255 248 220		cornsilk1
238 232 205		cornsilk2
205 200 177		cornsilk3
139 136 120		cornsilk4
255 255 240		ivory1
238 238 224		ivory2
205 205 193		ivory3
139 139 131		ivory4
240 255 240		honeydew1
224 238 224		honeydew2
193 205 193		honeydew3
131 139 131		honeydew4
255 240 245		LavenderBlush1
238 224 229		LavenderBlush2
205 193 197		LavenderBlush3
139 131 134		LavenderBlush4
255 228 225		MistyRose1
238 213 210		MistyRose2
205 183 181		MistyRose3
139 125 123		MistyRose4
240 255 255		azure1
224 238 238		azure2
193 205 205		azure3
131 139 139		azure4
131 111 255		SlateBlue1
122 103 238		SlateBlue2
105  89 205		SlateBlue3
 71  60 139		SlateBlue4
 72 118 255		RoyalBlue1
 67 110 238		RoyalBlue2
 58  95 205		RoyalBlue3
 39  64 139		RoyalBlue4
  0   0 255		blue1
  0   0 238		blue2
  0   0 205		blue3
  0   0 139		blue4
 30 144 255		DodgerBlue1
 28 134 238		DodgerBlue2
 24 116 205		DodgerBlue3
 16  78 139		DodgerBlue4
 99 184 255		SteelBlue1
 92 172 238		SteelBlue2
 79 148 205		SteelBlue3
 54 100 139		SteelBlue4
  0 191 255		DeepSkyBlue1
  0 178 238		DeepSkyBlue2
  0 154 205		DeepSkyBlue3
  0 104 139		DeepSkyBlue4
135 206 255		SkyBlue1
126 192 238		SkyBlue2
108 166 205		SkyBlue3
 74 112 139		SkyBlue4
176 226 255		LightSkyBlue1
164 211 238		LightSkyBlue2
141 182 205		LightSkyBlue3
 96 123 139		LightSkyBlue4
198 226 255		SlateGray1
185 211 238		SlateGray2
159 182 205		SlateGray3
108 123 139		SlateGray4
202 225 255		LightSteelBlue1
188 210 238		LightSteelBlue2
162 181 205		LightSteelBlue3
110 123 139		LightSteelBlue4
191 239 255		LightBlue1
178 223 238		LightBlue2
154 192 205		LightBlue3
104 131 139		LightBlue4
224 255 255		LightCyan1
209 238 238		LightCyan2
180 205 205		LightCyan3
122 139 139		LightCyan4
187 255 255		PaleTurquoise1
174 238 238		PaleTurquoise2
150 205 205		PaleTurquoise3
102 139 139		PaleTurquoise4
152 245 255		CadetBlue1
142 229 238		CadetBlue2
122 197 205		CadetBlue3
 83 134 139		CadetBlue4
  0 245 255		turquoise1
  0 229 238		turquoise2
  0 197 205		turquoise3
  0 134 139		turquoise4
  0 255 255		cyan1
  0 238 238		cyan2
  0 205 205		cyan3
  0 139 139		cyan4
151 255 255		DarkSlateGray1
141 238 238		DarkSlateGray2
121 205 205		DarkSlateGray3
 82 139 139		DarkSlateGray4
127 255 212		aquamarine1
118 238 198		aquamarine2
102 205 170		aquamarine3
 69 139 116		aquamarine4
193 255 193		DarkSeaGreen1
180 238 180		DarkSeaGreen2
155 205 155		DarkSeaGreen3
105 139 105		DarkSeaGreen4
 84 255 159		SeaGreen1
 78 238 148		SeaGreen2
 67 205 128		SeaGreen3
 46 139	 87		SeaGreen4
154 255 154		PaleGreen1
144 238 144		PaleGreen2
124 205 124		PaleGreen3
 84 139	 84		PaleGreen4
  0 255 127		SpringGreen1
  0 238 118		SpringGreen2
  0 205 102		SpringGreen3
  0 139	 69		SpringGreen4
  0 255	  0		green1
  0 238	  0		green2
  0 205	  0		green3
  0 139	  0		green4
127 255	  0		chartreuse1
118 238	  0		chartreuse2
102 205	  0		chartreuse3
 69 139	  0		chartreuse4
192 255	 62		OliveDrab1
179 238	 58		OliveDrab2
154 205	 50		OliveDrab3
105 139	 34		OliveDrab4
202 255 112		DarkOliveGreen1
188 238 104		DarkOliveGreen2
162 205	 90		DarkOliveGreen3
110 139	 61		DarkOliveGreen4
255 246 143		khaki1
238 230 133		khaki2
205 198 115		khaki3
139 134	 78		khaki4
255 236 139		LightGoldenrod1
238 220 130		LightGoldenrod2
205 190 112		LightGoldenrod3
139 129	 76		LightGoldenrod4
255 255 224		LightYellow1
238 238 209		LightYellow2
205 205 180		LightYellow3
139 139 122		LightYellow4
255 255	  0		yellow1
238 238	  0		yellow2
205 205	  0		yellow3
139 139	  0		yellow4
255 215	  0		gold1
238 201	  0		gold2
205 173	  0		gold3
139 117	  0		gold4
255 193	 37		goldenrod1
238 180	 34		goldenrod2
205 155	 29		goldenrod3
139 105	 20		goldenrod4
255 185	 15		DarkGoldenrod1
238 173	 14		DarkGoldenrod2
205 149	 12		DarkGoldenrod3
139 101	  8		DarkGoldenrod4
255 193 193		RosyBrown1
238 180 180		RosyBrown2
205 155 155		RosyBrown3
139 105 105		RosyBrown4
255 106 106		IndianRed1
238  99	 99		IndianRed2
205  85	 85		IndianRed3
139  58	 58		IndianRed4
255 130	 71		sienna1
238 121	 66		sienna2
205 104	 57		sienna3
139  71	 38		sienna4
255 211 155		burlywood1
238 197 145		burlywood2
205 170 125		burlywood3
139 115	 85		burlywood4
255 231 186		wheat1
238 216 174		wheat2
205 186 150		wheat3
139 126 102		wheat4
255 165	 79		tan1
238 154	 73		tan2
205 133	 63		tan3
139  90	 43		tan4
255 127	 36		chocolate1
238 118	 33		chocolate2
205 102	 29		chocolate3
139  69	 19		chocolate4
255  48	 48		firebrick1
238  44	 44		firebrick2
205  38	 38		firebrick3
139  26	 26		firebrick4
255  64	 64		brown1
238  59	 59		brown2
205  51	 51		brown3
139  35	 35		brown4
255 140 105		salmon1
238 130	 98		salmon2
205 112	 84		salmon3
139  76	 57		salmon4
255 160 122		LightSalmon1
238 149 114		LightSalmon2
205 129	 98		LightSalmon3
139  87	 66		LightSalmon4
255 165	  0		orange1
238 154	  0		orange2
205 133	  0		orange3
139  90	  0		orange4
255 127	  0		DarkOrange1
238 118	  0		DarkOrange2
205 102	  0		DarkOrange3
139  69	  0		DarkOrange4
255 114	 86		coral1
238 106	 80		coral2
205  91	 69		coral3
139  62	 47		coral4
255  99	 71		tomato1
238  92	 66		tomato2
205  79	 57		tomato3
139  54	 38		tomato4
255  69	  0		OrangeRed1
238  64	  0		OrangeRed2
205  55	  0		OrangeRed3
139  37	  0		OrangeRed4
255   0	  0		red1
238   0	  0		red2
205   0	  0		red3
139   0	  0		red4
255  20 147		DeepPink1
238  18 137		DeepPink2
205  16 118		DeepPink3
139  10	 80		DeepPink4
255 110 180		HotPink1
238 106 167		HotPink2
205  96 144		HotPink3
139  58  98		HotPink4
255 181 197		pink1
238 169 184		pink2
205 145 158		pink3
139  99 108		pink4
255 174 185		LightPink1
238 162 173		LightPink2
205 140 149		LightPink3
139  95 101		LightPink4
255 130 171		PaleVioletRed1
238 121 159		PaleVioletRed2
205 104 137		PaleVioletRed3
139  71	 93		PaleVioletRed4
255  52 179		maroon1
238  48 167		maroon2
205  41 144		maroon3
139  28	 98		maroon4
255  62 150		VioletRed1
238  58 140		VioletRed2
205  50 120		VioletRed3
139  34	 82		VioletRed4
255   0 255		magenta1
238   0 238		magenta2
205   0 205		magenta3
139   0 139		magenta4
255 131 250		orchid1
238 122 233		orchid2
205 105 201		orchid3
139  71 137		orchid4
255 187 255		plum1
238 174 238		plum2
205 150 205		plum3
139 102 139		plum4
224 102 255		MediumOrchid1
209  95 238		MediumOrchid2
180  82 205		MediumOrchid3
122  55 139		MediumOrchid4
191  62 255		DarkOrchid1
178  58 238		DarkOrchid2
154  50 205		DarkOrchid3
104  34 139		DarkOrchid4
155  48 255		purple1
145  44 238		purple2
125  38 205		purple3
 85  26 139		purple4
171 130 255		MediumPurple1
159 121 238		MediumPurple2
137 104 205		MediumPurple3
 93  71 139		MediumPurple4
255 225 255		thistle1
238 210 238		thistle2
205 181 205		thistle3
139 123 139		thistle4
  0   0   0		gray0
  0   0   0		grey0
  3   3   3		gray1
  3   3   3		grey1
  5   5   5		gray2
  5   5   5		grey2
  8   8   8		gray3
  8   8   8		grey3
 10  10  10 		gray4
 10  10  10 		grey4
 13  13  13 		gray5
 13  13  13 		grey5
 15  15  15 		gray6
 15  15  15 		grey6
 18  18  18 		gray7
 18  18  18 		grey7
 20  20  20 		gray8
 20  20  20 		grey8
 23  23  23 		gray9
 23  23  23 		grey9
 26  26  26 		gray10
 26  26  26 		grey10
 28  28  28 		gray11
 28  28  28 		grey11
 31  31  31 		gray12
 31  31  31 		grey12
 33  33  33 		gray13
 33  33  33 		grey13
 36  36  36 		gray14
 36  36  36 		grey14
 38  38  38 		gray15
 38  38  38 		grey15
 41  41  41 		gray16
 41  41  41 		grey16
 43  43  43 		gray17
 43  43  43 		grey17
 46  46  46 		gray18
 46  46  46 		grey18
 48  48  48 		gray19
 48  48  48 		grey19
 51  51  51 		gray20
 51  51  51 		grey20
 54  54  54 		gray21
 54  54  54 		grey21
 56  56  56 		gray22
 56  56  56 		grey22
 59  59  59 		gray23
 59  59  59 		grey23
 61  61  61 		gray24
 61  61  61 		grey24
 64  64  64 		gray25
 64  64  64 		grey25
 66  66  66 		gray26
 66  66  66 		grey26
 69  69  69 		gray27
 69  69  69 		grey27
 71  71  71 		gray28
 71  71  71 		grey28
 74  74  74 		gray29
 74  74  74 		grey29
 77  77  77 		gray30
 77  77  77 		grey30
 79  79  79 		gray31
 79  79  79 		grey31
 82  82  82 		gray32
 82  82  82 		grey32
 84  84  84 		gray33
 84  84  84 		grey33
 87  87  87 		gray34
 87  87  87 		grey34
 89  89  89 		gray35
 89  89  89 		grey35
 92  92  92 		gray36
 92  92  92 		grey36
 94  94  94 		gray37
 94  94  94 		grey37
 97  97  97 		gray38
 97  97  97 		grey38
 99  99  99 		gray39
 99  99  99 		grey39
102 102 102 		gray40
102 102 102 		grey40
105 105 105 		gray41
105 105 105 		grey41
107 107 107 		gray42
107 107 107 		grey42
110 110 110 		gray43
110 110 110 		grey43
112 112 112 		gray44
112 112 112 		grey44
115 115 115 		gray45
115 115 115 		grey45
117 117 117 		gray46
117 117 117 		grey46
120 120 120 		gray47
120 120 120 		grey47
122 122 122 		gray48
122 122 122 		grey48
125 125 125 		gray49
125 125 125 		grey49
127 127 127 		gray50
127 127 127 		grey50
130 130 130 		gray51
130 130 130 		grey51
133 133 133 		gray52
133 133 133 		grey52
135 135 135 		gray53
135 135 135 		grey53
138 138 138 		gray54
138 138 138 		grey54
140 140 140 		gray55
140 140 140 		grey55
143 143 143 		gray56
143 143 143 		grey56
145 145 145 		gray57
145 145 145 		grey57
148 148 148 		gray58
148 148 148 		grey58
150 150 150 		gray59
150 150 150 		grey59
153 153 153 		gray60
153 153 153 		grey60
156 156 156 		gray61
156 156 156 		grey61
158 158 158 		gray62
158 158 158 		grey62
161 161 161 		gray63
161 161 161 		grey63
163 163 163 		gray64
163 163 163 		grey64
166 166 166 		gray65
166 166 166 		grey65
168 168 168 		gray66
168 168 168 		grey66
171 171 171 		gray67
171 171 171 		grey67
173 173 173 		gray68
173 173 173 		grey68
176 176 176 		gray69
176 176 176 		grey69
179 179 179 		gray70
179 179 179 		grey70
181 181 181 		gray71
181 181 181 		grey71
184 184 184 		gray72
184 184 184 		grey72
186 186 186 		gray73
186 186 186 		grey73
189 189 189 		gray74
189 189 189 		grey74
191 191 191 		gray75
191 191 191 		grey75
194 194 194 		gray76
194 194 194 		grey76
196 196 196 		gray77
196 196 196 		grey77
199 199 199 		gray78
199 199 199 		grey78
201 201 201 		gray79
201 201 201 		grey79
204 204 204 		gray80
204 204 204 		grey80
207 207 207 		gray81
207 207 207 		grey81
209 209 209 		gray82
209 209 209 		grey82
212 212 212 		gray83
212 212 212 		grey83
214 214 214 		gray84
214 214 214 		grey84
217 217 217 		gray85
217 217 217 		grey85
219 219 219 		gray86
219 219 219 		grey86
222 222 222 		gray87
222 222 222 		grey87
224 224 224 		gray88
224 224 224 		grey88
227 227 227 		gray89
227 227 227 		grey89
229 229 229 		gray90
229 229 229 		grey90
232 232 232 		gray91
232 232 232 		grey91
235 235 235 		gray92
235 235 235 		grey92
237 237 237 		gray93
237 237 237 		grey93
240 240 240 		gray94
240 240 240 		grey94
242 242 242 		gray95
242 242 242 		grey95
245 245 245 		gray96
245 245 245 		grey96
247 247 247 		gray97
247 247 247 		grey97
250 250 250 		gray98
250 250 250 		grey98
252 252 252 		gray99
252 252 252 		grey99
255 255 255 		gray100
255 255 255 		grey100
169 169 169		dark grey
169 169 169		DarkGrey
169 169 169		dark gray
169 169 169		DarkGray
0     0 139		dark blue
0     0 139		DarkBlue
0   139 139		dark cyan
0   139 139		DarkCyan
139   0 139		dark magenta
139   0 139		DarkMagenta
139   0   0		dark red
139   0   0		DarkRed
144 238 144		light green
144 238 144		LightGreen
