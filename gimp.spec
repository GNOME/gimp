%define name     gimp
%define ver      1.1.15
%define subver   1.1
%define microver 15
%define rel      2
%define prefix	 /usr

Summary: The GNU Image Manipulation Program
Name: 		%name
Version: 	%ver
Release: 	%rel
Copyright: 	GPL, LGPL
Group: 		Applications/Graphics
URL: 		http://www.gimp.org/
BuildRoot: 	/var/tmp/%{name}-%{version}-root
Docdir:		%{prefix}/doc
Prefix:		%{prefix}
Obsoletes: 	gimp-data-min
Obsoletes:	gimp-libgimp
Requires: 	gtk+ >= 1.2.0
Source: 	ftp://ftp.gimp.org/pub/gimp/unstable/v%{PACKAGE_VERSION}/%{name}-%{PACKAGE_VERSION}.tar.gz

%description
The GIMP is an image manipulation program suitable for photo retouching,
image composition and image authoring.  Many people find it extremely useful
in creating logos and other graphics for web pages.  The GIMP has many of the
tools and filters you would expect to find in similar commercial offerings,
and some interesting extras as well.

The GIMP provides a large image manipulation toolbox, including channel
operations and layers, effects, sub-pixel imaging and anti-aliasing,
and conversions, all with multi-level undo.

This version of The GIMP includes a scripting facility, but many of the
included scripts rely on fonts that we cannot distribute.  The GIMP ftp
site has a package of fonts that you can install by yourself, which
includes all the fonts needed to run the included scripts.  Some of the
fonts have unusual licensing requirements; all the licenses are documented
in the package.  Get ftp://ftp.gimp.org/pub/gimp/fonts/freefonts-0.10.tar.gz
and ftp://ftp.gimp.org/pub/gimp/fonts/sharefonts-0.10.tar.gz if you are so
inclined.  Alternatively, choose fonts which exist on your system before
running the scripts.

%package devel
Summary: GIMP plugin and extension development kit
Group: 		Applications/Graphics
Requires: 	gtk+-devel
%description devel
Static libraries and header files for writing GIMP plugins and extensions.

%package perl
Summary: GIMP perl extensions and plugins.
Group:		Applications/Graphics
Requires:	gimp
Requires:	perl

%description perl
The gimp-perl package contains all the perl extensions and perl plugins. 

%prep
%setup -q

%build
%ifarch alpha
MYARCH_FLAGS="--host=alpha-redhat-linux"
%endif

if [ ! -f configure ]; then
  CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --quiet $MYARCH_FLAGS --prefix=/usr
else
  CFLAGS="$RPM_OPT_FLAGS" ./configure --quiet $MYARCH_FLAGS --prefix=/usr
fi

if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi


%install
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/info $RPM_BUILD_ROOT/usr/include \
	$RPM_BUILD_ROOT/usr/lib $RPM_BUILD_ROOT/usr/bin
make prefix=$RPM_BUILD_ROOT/usr PREFIX=$RPM_BUILD_ROOT/usr install

# Strip the executables
strip $RPM_BUILD_ROOT/usr/bin/gimp
# Only strip execuable files and leave scripts alone.
strip `file $RPM_BUILD_ROOT/usr/lib/gimp/%{subver}/plug-ins/* | grep ELF | cut -d':' -f 1`

# Compress down the online documentation.
if [ -d $RPM_BUILD_ROOT/usr/man ]; then
  find $RPM_BUILD_ROOT/usr/man -type f -exec gzip -9nf {} \;
fi

#
# This perl madness will drive me batty
#
eval perl '-V:archname'
find $RPM_BUILD_ROOT/usr/lib/perl5 -type f -print | sed "s@^$RPM_BUILD_ROOT@@g" | grep -v perllocal.pod > gimp-perl
#
# Help files appear to be in flux.
#
echo "%defattr (0444, bin, man, 0555)" > gimp-help-files
find $RPM_BUILD_ROOT/usr/share/gimp/help -type f -print | sed "s@^$RPM_BUILD_ROOT@@g"  >>gimp-help-files
#
# Plugins and modules change often (grab the executeable ones)
#
echo "%defattr (0555, bin, bin)" > gimp-plugin-files
find $RPM_BUILD_ROOT/usr/lib/gimp/%{subver} -type f -exec file {} \; | grep -v perl | cut -d':' -f 1 | sed "s@^$RPM_BUILD_ROOT@@g"  >>gimp-plugin-files
#
# Now pull the perl ones out.
#
echo "%defattr (0555, bin, bin)" > gimp-perl-plugin-files
find $RPM_BUILD_ROOT/usr/lib/gimp/%{subver} -type f -exec file {} \; | grep perl | cut -d':' -f 1 | sed "s@^$RPM_BUILD_ROOT@@g" >>gimp-perl-plugin-files
#
# Auto detect the lang files.
#
if [ -f /usr/lib/rpm/find-lang.sh ] ; then
 /usr/lib/rpm/find-lang.sh $RPM_BUILD_ROOT %name
 sed "s:(644, root, root, 755):(444, bin, bin, 555):" %{name}.lang >tmp.lang && mv tmp.lang %{name}.lang
fi
#
# Scripts
#
echo "%defattr (0444, bin, bin, 0555)" >gimp-script-files
find $RPM_BUILD_ROOT/usr/share/gimp/scripts -type f -print | sed "s@^$RPM_BUILD_ROOT@@g" >> gimp-script-files
#
# Brushes
#
echo "%defattr (0444, bin, bin, 0555)" >gimp-brushes-files
find $RPM_BUILD_ROOT/usr/share/gimp/brushes -type f -print | sed "s@^$RPM_BUILD_ROOT@@g" | sort >>gimp-brushes-files

#
# Build the master filelists generated from the above mess.
#
cat gimp-help-files gimp-plugin-files gimp.lang gimp-script-files gimp-brushes-files >gimp.files
cat gimp-perl gimp-perl-plugin-files >gimp-perl-files

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files -f gimp.files
%attr (0555, bin, man) %doc AUTHORS COPYING ChangeLog MAINTAINERS NEWS README TODO
%attr (0555, bin, man) %doc docs/*.txt docs/*.eps ABOUT-NLS README.i18n README.perl README.win32 TODO
%defattr (0555, bin, bin)
%dir /usr/share/gimp
%dir /usr/share/gimp/brushes
%dir /usr/share/gimp/fractalexplorer
%dir /usr/share/gimp/gfig
%dir /usr/share/gimp/gflare
%dir /usr/share/gimp/gimpressionist
%dir /usr/share/gimp/gradients
%dir /usr/share/gimp/help
%dir /usr/share/gimp/palettes
%dir /usr/share/gimp/patterns
%dir /usr/share/gimp/scripts
%dir /usr/share/gimp/tips
%dir /usr/share/locale
%dir /usr/lib/gimp/%{subver}
%dir /usr/lib/gimp/%{subver}/modules
%dir /usr/lib/gimp/%{subver}/plug-ins

%{prefix}/share/gimp/fractalexplorer/Asteroid_Field
%{prefix}/share/gimp/fractalexplorer/Bar_Code_Label
%{prefix}/share/gimp/fractalexplorer/Beauty_of_Nature
%{prefix}/share/gimp/fractalexplorer/Blue_Curtain
%{prefix}/share/gimp/fractalexplorer/Car_Track
%{prefix}/share/gimp/fractalexplorer/Energetic_Diamond
%{prefix}/share/gimp/fractalexplorer/Explosive
%{prefix}/share/gimp/fractalexplorer/Flower
%{prefix}/share/gimp/fractalexplorer/Fragments
%{prefix}/share/gimp/fractalexplorer/Hemp
%{prefix}/share/gimp/fractalexplorer/High_Voltage
%{prefix}/share/gimp/fractalexplorer/Hoops
%{prefix}/share/gimp/fractalexplorer/Ice_Crystal
%{prefix}/share/gimp/fractalexplorer/Leaves
%{prefix}/share/gimp/fractalexplorer/Lightning
%{prefix}/share/gimp/fractalexplorer/Mandelbrot
%{prefix}/share/gimp/fractalexplorer/Marble
%{prefix}/share/gimp/fractalexplorer/Marble2
%{prefix}/share/gimp/fractalexplorer/Medusa
%{prefix}/share/gimp/fractalexplorer/Nautilus
%{prefix}/share/gimp/fractalexplorer/Nebula
%{prefix}/share/gimp/fractalexplorer/Plant
%{prefix}/share/gimp/fractalexplorer/Rose
%{prefix}/share/gimp/fractalexplorer/Saturn
%{prefix}/share/gimp/fractalexplorer/Snow_Crystal
%{prefix}/share/gimp/fractalexplorer/Soma
%{prefix}/share/gimp/fractalexplorer/Spark
%{prefix}/share/gimp/fractalexplorer/Suns
%{prefix}/share/gimp/fractalexplorer/Tentacles
%{prefix}/share/gimp/fractalexplorer/The_Green_Place
%{prefix}/share/gimp/fractalexplorer/Wave
%{prefix}/share/gimp/fractalexplorer/Wood
%{prefix}/share/gimp/fractalexplorer/Zooming_Circle

%{prefix}/share/gimp/gfig/A_star
%{prefix}/share/gimp/gfig/curves
%{prefix}/share/gimp/gfig/polys
%{prefix}/share/gimp/gfig/ring
%{prefix}/share/gimp/gfig/ring+star
%{prefix}/share/gimp/gfig/simily
%{prefix}/share/gimp/gfig/spirals_and_stars
%{prefix}/share/gimp/gfig/sprial
%{prefix}/share/gimp/gfig/star2
%{prefix}/share/gimp/gfig/stars

%{prefix}/share/gimp/gflare/Bright_Star
%{prefix}/share/gimp/gflare/Classic
%{prefix}/share/gimp/gflare/Distant_Sun
%{prefix}/share/gimp/gflare/Default
%{prefix}/share/gimp/gflare/GFlare_101
%{prefix}/share/gimp/gflare/GFlare_102
%{prefix}/share/gimp/gflare/Hidden_Planet

%{prefix}/share/gimp/gimpressionist/Brushes/arrow01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/ball.ppm
%{prefix}/share/gimp/gimpressionist/Brushes/blob.ppm
%{prefix}/share/gimp/gimpressionist/Brushes/box.ppm
%{prefix}/share/gimp/gimpressionist/Brushes/chalk01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/cone.ppm
%{prefix}/share/gimp/gimpressionist/Brushes/crayon01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/crayon02.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/crayon03.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/crayon04.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/crayon05.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/crayon06.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/crayon07.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/crayon08.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/defaultbrush.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/dribble.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/fabric.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/fabric01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/fabric02.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/fabric03.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/flower01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/flower02.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/flower03.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/flower04.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/grad01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/grad02.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/grad03.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/heart.ppm
%{prefix}/share/gimp/gimpressionist/Brushes/leaf01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paintbrush.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paintbrush01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paintbrush02.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paintbrush03.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paintbrush04.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paper01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paper02.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paper03.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/paper04.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/pentagram.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/scribble.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/shape01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/shape02.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/shape03.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/shape04.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/sphere.ppm
%{prefix}/share/gimp/gimpressionist/Brushes/splat1.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/splat2.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/splat3.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/spunge01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/spunge02.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/spunge03.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/spunge04.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/spunge05.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/strange01.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/thegimp.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/torus.ppm
%{prefix}/share/gimp/gimpressionist/Brushes/wavy.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/weave.pgm
%{prefix}/share/gimp/gimpressionist/Brushes/worm.pgm
%{prefix}/share/gimp/gimpressionist/Paper/bricks.pgm
%{prefix}/share/gimp/gimpressionist/Paper/bricks2.pgm
%{prefix}/share/gimp/gimpressionist/Paper/burlap.pgm
%{prefix}/share/gimp/gimpressionist/Paper/canvas2.pgm
%{prefix}/share/gimp/gimpressionist/Paper/defaultpaper.pgm
%{prefix}/share/gimp/gimpressionist/Paper/marble.pgm
%{prefix}/share/gimp/gimpressionist/Paper/marble2.pgm
%{prefix}/share/gimp/gimpressionist/Paper/stone.pgm
%{prefix}/share/gimp/gimpressionist/Presets/Ballpark
%{prefix}/share/gimp/gimpressionist/Presets/Canvas
%{prefix}/share/gimp/gimpressionist/Presets/Crosshatch
%{prefix}/share/gimp/gimpressionist/Presets/Cubism
%{prefix}/share/gimp/gimpressionist/Presets/Dotify
%{prefix}/share/gimp/gimpressionist/Presets/Embroidery
%{prefix}/share/gimp/gimpressionist/Presets/Feathers
%{prefix}/share/gimp/gimpressionist/Presets/Felt-marker
%{prefix}/share/gimp/gimpressionist/Presets/Flowerbed
%{prefix}/share/gimp/gimpressionist/Presets/Furry
%{prefix}/share/gimp/gimpressionist/Presets/Line-art
%{prefix}/share/gimp/gimpressionist/Presets/Line-art-2
%{prefix}/share/gimp/gimpressionist/Presets/Maggot-invasion
%{prefix}/share/gimp/gimpressionist/Presets/MarbleMadness
%{prefix}/share/gimp/gimpressionist/Presets/Mossy
%{prefix}/share/gimp/gimpressionist/Presets/Painted_Rock
%{prefix}/share/gimp/gimpressionist/Presets/Parquette
%{prefix}/share/gimp/gimpressionist/Presets/Patchwork
%{prefix}/share/gimp/gimpressionist/Presets/Ringworks
%{prefix}/share/gimp/gimpressionist/Presets/Sample
%{prefix}/share/gimp/gimpressionist/Presets/Smash
%{prefix}/share/gimp/gimpressionist/Presets/Straws
%{prefix}/share/gimp/gimpressionist/Presets/Weave
%{prefix}/share/gimp/gimpressionist/Presets/Wormcan

%{prefix}/share/gimp/gradients/Abstract_1
%{prefix}/share/gimp/gradients/Abstract_2
%{prefix}/share/gimp/gradients/Abstract_3
%{prefix}/share/gimp/gradients/Aneurism
%{prefix}/share/gimp/gradients/Blinds
%{prefix}/share/gimp/gradients/Blue_Green
%{prefix}/share/gimp/gradients/Browns
%{prefix}/share/gimp/gradients/Brushed_Aluminium
%{prefix}/share/gimp/gradients/Burning_Paper
%{prefix}/share/gimp/gradients/Burning_Transparency
%{prefix}/share/gimp/gradients/CD
%{prefix}/share/gimp/gradients/CD_Half
%{prefix}/share/gimp/gradients/Caribbean_Blues
%{prefix}/share/gimp/gradients/Coffee
%{prefix}/share/gimp/gradients/Cold_Steel
%{prefix}/share/gimp/gradients/Cold_Steel_2
%{prefix}/share/gimp/gradients/Crown_molding
%{prefix}/share/gimp/gradients/Dark_1
%{prefix}/share/gimp/gradients/Deep_Sea
%{prefix}/share/gimp/gradients/Default
%{prefix}/share/gimp/gradients/Flare_Glow_Angular_1
%{prefix}/share/gimp/gradients/Flare_Glow_Radial_1
%{prefix}/share/gimp/gradients/Flare_Glow_Radial_2
%{prefix}/share/gimp/gradients/Flare_Glow_Radial_3
%{prefix}/share/gimp/gradients/Flare_Glow_Radial_4
%{prefix}/share/gimp/gradients/Flare_Radial_101
%{prefix}/share/gimp/gradients/Flare_Radial_102
%{prefix}/share/gimp/gradients/Flare_Radial_103
%{prefix}/share/gimp/gradients/Flare_Rays_Radial_1
%{prefix}/share/gimp/gradients/Flare_Rays_Radial_2
%{prefix}/share/gimp/gradients/Flare_Rays_Size_1
%{prefix}/share/gimp/gradients/Flare_Sizefac_101
%{prefix}/share/gimp/gradients/Four_bars
%{prefix}/share/gimp/gradients/French_flag
%{prefix}/share/gimp/gradients/French_flag_smooth
%{prefix}/share/gimp/gradients/Full_saturation_spectrum_CCW
%{prefix}/share/gimp/gradients/Full_saturation_spectrum_CW
%{prefix}/share/gimp/gradients/German_flag
%{prefix}/share/gimp/gradients/German_flag_smooth
%{prefix}/share/gimp/gradients/Golden
%{prefix}/share/gimp/gradients/Greens
%{prefix}/share/gimp/gradients/Horizon_1
%{prefix}/share/gimp/gradients/Horizon_2
%{prefix}/share/gimp/gradients/Incandescent
%{prefix}/share/gimp/gradients/Land_1
%{prefix}/share/gimp/gradients/Land_and_Sea
%{prefix}/share/gimp/gradients/Metallic_Something
%{prefix}/share/gimp/gradients/Mexican_flag
%{prefix}/share/gimp/gradients/Mexican_flag_smooth
%{prefix}/share/gimp/gradients/Nauseating_Headache
%{prefix}/share/gimp/gradients/Neon_Cyan
%{prefix}/share/gimp/gradients/Neon_Green
%{prefix}/share/gimp/gradients/Neon_Yellow
%{prefix}/share/gimp/gradients/Pastel_Rainbow
%{prefix}/share/gimp/gradients/Pastels
%{prefix}/share/gimp/gradients/Purples
%{prefix}/share/gimp/gradients/Radial_Eyeball_Blue
%{prefix}/share/gimp/gradients/Radial_Eyeball_Brown
%{prefix}/share/gimp/gradients/Radial_Eyeball_Green
%{prefix}/share/gimp/gradients/Radial_Glow_1
%{prefix}/share/gimp/gradients/Radial_Rainbow_Hoop
%{prefix}/share/gimp/gradients/Romanian_flag
%{prefix}/share/gimp/gradients/Romanian_flag_smooth
%{prefix}/share/gimp/gradients/Rounded_edge
%{prefix}/share/gimp/gradients/Shadows_1
%{prefix}/share/gimp/gradients/Shadows_2
%{prefix}/share/gimp/gradients/Shadows_3
%{prefix}/share/gimp/gradients/Skyline
%{prefix}/share/gimp/gradients/Skyline_polluted
%{prefix}/share/gimp/gradients/Square_Wood_Frame
%{prefix}/share/gimp/gradients/Sunrise
%{prefix}/share/gimp/gradients/Three_bars_sin
%{prefix}/share/gimp/gradients/Tropical_Colors
%{prefix}/share/gimp/gradients/Tube_Red
%{prefix}/share/gimp/gradients/Wood_1
%{prefix}/share/gimp/gradients/Wood_2
%{prefix}/share/gimp/gradients/Yellow_Contrast
%{prefix}/share/gimp/gradients/Yellow_Orange

%{prefix}/share/gimp/palettes/Bears
%{prefix}/share/gimp/palettes/Bgold
%{prefix}/share/gimp/palettes/Blues
%{prefix}/share/gimp/palettes/Borders
%{prefix}/share/gimp/palettes/Browns_And_Yellows
%{prefix}/share/gimp/palettes/Caramel
%{prefix}/share/gimp/palettes/Cascade
%{prefix}/share/gimp/palettes/China
%{prefix}/share/gimp/palettes/Coldfire
%{prefix}/share/gimp/palettes/Cool_Colors
%{prefix}/share/gimp/palettes/Cranes
%{prefix}/share/gimp/palettes/Dark_pastels
%{prefix}/share/gimp/palettes/Default
%{prefix}/share/gimp/palettes/Ega
%{prefix}/share/gimp/palettes/Firecode
%{prefix}/share/gimp/palettes/Gold
%{prefix}/share/gimp/palettes/GrayViolet
%{prefix}/share/gimp/palettes/Grayblue
%{prefix}/share/gimp/palettes/Grays
%{prefix}/share/gimp/palettes/Greens
%{prefix}/share/gimp/palettes/Hilite
%{prefix}/share/gimp/palettes/Kahki
%{prefix}/share/gimp/palettes/Lights
%{prefix}/share/gimp/palettes/Muted
%{prefix}/share/gimp/palettes/Named_Colors
%{prefix}/share/gimp/palettes/News3
%{prefix}/share/gimp/palettes/Op2
%{prefix}/share/gimp/palettes/Paintjet
%{prefix}/share/gimp/palettes/Pastels
%{prefix}/share/gimp/palettes/Plasma
%{prefix}/share/gimp/palettes/Reds
%{prefix}/share/gimp/palettes/Reds_And_Purples
%{prefix}/share/gimp/palettes/Royal
%{prefix}/share/gimp/palettes/Topographic
%{prefix}/share/gimp/palettes/Visibone
%{prefix}/share/gimp/palettes/Visibone2
%{prefix}/share/gimp/palettes/Volcano
%{prefix}/share/gimp/palettes/Warm_Colors
%{prefix}/share/gimp/palettes/Web

%{prefix}/share/gimp/patterns/3dgreen.pat
%{prefix}/share/gimp/patterns/Craters.pat
%{prefix}/share/gimp/patterns/Moonfoot.pat
%{prefix}/share/gimp/patterns/amethyst.pat
%{prefix}/share/gimp/patterns/bark.pat
%{prefix}/share/gimp/patterns/blue.pat
%{prefix}/share/gimp/patterns/bluegrid.pat
%{prefix}/share/gimp/patterns/bluesquares.pat
%{prefix}/share/gimp/patterns/blueweb.pat
%{prefix}/share/gimp/patterns/brick.pat
%{prefix}/share/gimp/patterns/burlap.pat
%{prefix}/share/gimp/patterns/burlwood.pat
%{prefix}/share/gimp/patterns/choc_swirl.pat
%{prefix}/share/gimp/patterns/corkboard.pat
%{prefix}/share/gimp/patterns/cracked.pat
%{prefix}/share/gimp/patterns/crinklepaper.pat
%{prefix}/share/gimp/patterns/electric.pat
%{prefix}/share/gimp/patterns/fibers.pat
%{prefix}/share/gimp/patterns/granite1.pat
%{prefix}/share/gimp/patterns/ground1.pat
%{prefix}/share/gimp/patterns/ice.pat
%{prefix}/share/gimp/patterns/java.pat
%{prefix}/share/gimp/patterns/leather.pat
%{prefix}/share/gimp/patterns/leaves.pat
%{prefix}/share/gimp/patterns/leopard.pat
%{prefix}/share/gimp/patterns/lightning.pat
%{prefix}/share/gimp/patterns/marble1.pat
%{prefix}/share/gimp/patterns/marble2.pat
%{prefix}/share/gimp/patterns/marble3.pat
%{prefix}/share/gimp/patterns/nops.pat
%{prefix}/share/gimp/patterns/paper.pat
%{prefix}/share/gimp/patterns/parque1.pat
%{prefix}/share/gimp/patterns/parque2.pat
%{prefix}/share/gimp/patterns/parque3.pat
%{prefix}/share/gimp/patterns/pastel.pat
%{prefix}/share/gimp/patterns/pine.pat
%{prefix}/share/gimp/patterns/pink_marble.pat
%{prefix}/share/gimp/patterns/pool.pat
%{prefix}/share/gimp/patterns/qube1.pat
%{prefix}/share/gimp/patterns/rain.pat
%{prefix}/share/gimp/patterns/recessed.pat
%{prefix}/share/gimp/patterns/redcube.pat
%{prefix}/share/gimp/patterns/rock.pat
%{prefix}/share/gimp/patterns/sky.pat
%{prefix}/share/gimp/patterns/slate.pat
%{prefix}/share/gimp/patterns/sm_squares.pat
%{prefix}/share/gimp/patterns/starfield.pat
%{prefix}/share/gimp/patterns/stone33.pat
%{prefix}/share/gimp/patterns/terra.pat
%{prefix}/share/gimp/patterns/walnut.pat
%{prefix}/share/gimp/patterns/warning.pat
%{prefix}/share/gimp/patterns/wood1.pat
%{prefix}/share/gimp/patterns/wood2.pat
%{prefix}/share/gimp/patterns/wood3.pat
%{prefix}/share/gimp/patterns/wood4.pat
%{prefix}/share/gimp/patterns/wood5.pat

%{prefix}/share/gimp/tips/gimp_tips.txt
%lang(de)	%{prefix}/share/gimp/tips/gimp_tips.de.txt
%lang(ja)	%{prefix}/share/gimp/tips/gimp_tips.ja.txt
%lang(it)	%{prefix}/share/gimp/tips/gimp_tips.it.txt
%lang(ko)	%{prefix}/share/gimp/tips/gimp_tips.ko.txt
%lang(fr)	%{prefix}/share/gimp/tips/gimp_conseils.fr.txt
%lang(ru)	%{prefix}/share/gimp/tips/gimp_tips.ru.txt
%lang(pl)	%{prefix}/share/gimp/tips/gimp_tips.pl.txt


%{prefix}/share/gimp/gimprc
%{prefix}/share/gimp/gimprc_user
%{prefix}/share/gimp/gtkrc
%{prefix}/share/gimp/unitrc
%{prefix}/share/gimp/gimp_logo.ppm
%{prefix}/share/gimp/gimp_splash.ppm
%{prefix}/share/gimp/gimp1_1_splash.ppm
%{prefix}/share/gimp/ps-menurc
%{prefix}/share/gimp/gtkrc.forest2


%defattr (0555, bin, bin)
%{prefix}/share/gimp/user_install

%{prefix}/lib/libgimp-%{subver}.so.%{microver}.0.0
%{prefix}/lib/libgimp-%{subver}.so.%{microver}
%{prefix}/lib/libgimpui-%{subver}.so.%{microver}.0.0
%{prefix}/lib/libgimpui-%{subver}.so.%{microver}
%{prefix}/lib/libgck-%{subver}.so.%{microver}.0.0
%{prefix}/lib/libgck-%{subver}.so.%{microver}

%{prefix}/bin/gimp
%{prefix}/bin/scm2scm
%{prefix}/bin/scm2perl
%{prefix}/bin/embedxpm
%{prefix}/bin/gimpdoc
%{prefix}/bin/xcftopnm

%defattr (0444, bin, man)
/usr/man/man1/gimp.1.gz
/usr/man/man5/gimprc.5.gz

%files devel
%defattr (0555, bin, bin) 
%dir %{prefix}/include/libgimp
%dir %{prefix}/include/gck

%{prefix}/bin/gimptool
%{prefix}/share/aclocal/gimp.m4

%{prefix}/lib/libgimp.so
%{prefix}/lib/libgimp.la
%{prefix}/lib/libgimp.a
%{prefix}/lib/libgimpui.so
%{prefix}/lib/libgimpui.la
%{prefix}/lib/libgimpui.a
%{prefix}/lib/libgck.so
%{prefix}/lib/libgck.la
%{prefix}/lib/libgck.a
%{prefix}/lib/libmegawidget.a

%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_gtk.la
%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_gtk.a
%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_triangle.la
%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_triangle.a
%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_water.la
%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_water.a
%{prefix}/lib/gimp/%{subver}/modules/libcdisplay_gamma.la
%{prefix}/lib/gimp/%{subver}/modules/libcdisplay_gamma.a

%defattr (0444, bin, bin, 0555) 
%{prefix}/include/libgimp/color_display.h
%{prefix}/include/libgimp/color_selector.h
%{prefix}/include/libgimp/gimp.h
%{prefix}/include/libgimp/gimpchainbutton.h
%{prefix}/include/libgimp/gimpcolorbutton.h
%{prefix}/include/libgimp/gimpcompat.h
%{prefix}/include/libgimp/gimpenums.h
%{prefix}/include/libgimp/gimpenv.h
%{prefix}/include/libgimp/gimpexport.h
%{prefix}/include/libgimp/gimpfeatures.h
%{prefix}/include/libgimp/gimpfileselection.h
%{prefix}/include/libgimp/gimplimits.h
%{prefix}/include/libgimp/gimpmath.h
%{prefix}/include/libgimp/gimpmatrix.h
%{prefix}/include/libgimp/gimpmenu.h
%{prefix}/include/libgimp/gimpmodule.h
%{prefix}/include/libgimp/gimppatheditor.h
%{prefix}/include/libgimp/gimpsizeentry.h
%{prefix}/include/libgimp/gimpui.h
%{prefix}/include/libgimp/gimpunit.h
%{prefix}/include/libgimp/gimpunitmenu.h
%{prefix}/include/libgimp/gimpintl.h
%{prefix}/include/libgimp/gserialize.h
%{prefix}/include/libgimp/parasite.h
%{prefix}/include/libgimp/parasiteF.h
%{prefix}/include/libgimp/parasiteP.h
%{prefix}/include/libgimp/parasiteio.h

%{prefix}/include/gck/gck.h
%{prefix}/include/gck/gckcolor.h
%{prefix}/include/gck/gckcommon.h
%{prefix}/include/gck/gckimage.h
%{prefix}/include/gck/gcklistbox.h
%{prefix}/include/gck/gckmath.h
%{prefix}/include/gck/gcktypes.h
%{prefix}/include/gck/gckui.h
%{prefix}/include/gck/gckvector.h

%defattr (0444, bin, man, 0555)
%{prefix}/man/man1/gimptool.1.gz

%files perl -f gimp-perl-files


%changelog
* Wed Jan 19 2000 Gregory McLean <gregm@comstar.net>
- Version 1.1.15

* Wed Dec 22 1999 Gregory McLean <gregm@comstar.net>
- Version 1.1.14
- Added some auto %files section generation scriptlets


