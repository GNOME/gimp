%define name     gimp
%define ver      1.1.14
%define subver   1.1
%define rel      1
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
Requires:	perl >= 5.004
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
# I can't get this to work..
#perl '-V:archname'
#export archname

%define archname i386-linux

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files 
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

%defattr (0444, bin, bin, 0555) 
%{prefix}/share/gimp/scripts/3d-outline.scm
%{prefix}/share/gimp/scripts/3dTruchet.scm
%{prefix}/share/gimp/scripts/add-bevel.scm
%{prefix}/share/gimp/scripts/addborder.scm
%{prefix}/share/gimp/scripts/alien-glow-arrow.scm
%{prefix}/share/gimp/scripts/alien-glow-bar.scm
%{prefix}/share/gimp/scripts/alien-glow-bullet.scm
%{prefix}/share/gimp/scripts/alien-glow-button.scm
%{prefix}/share/gimp/scripts/alien-glow-logo.scm
%{prefix}/share/gimp/scripts/asc2img.scm
%{prefix}/share/gimp/scripts/basic1-logo.scm
%{prefix}/share/gimp/scripts/basic2-logo.scm
%{prefix}/share/gimp/scripts/beavis.jpg
%{prefix}/share/gimp/scripts/beveled-button.scm
%{prefix}/share/gimp/scripts/beveled-pattern-arrow.scm
%{prefix}/share/gimp/scripts/beveled-pattern-bullet.scm
%{prefix}/share/gimp/scripts/beveled-pattern-button.scm
%{prefix}/share/gimp/scripts/beveled-pattern-heading.scm
%{prefix}/share/gimp/scripts/beveled-pattern-hrule.scm
%{prefix}/share/gimp/scripts/blend-anim.scm
%{prefix}/share/gimp/scripts/blended-logo.scm
%{prefix}/share/gimp/scripts/bovinated-logo.scm
%{prefix}/share/gimp/scripts/camo.scm
%{prefix}/share/gimp/scripts/carve-it.scm
%{prefix}/share/gimp/scripts/carved-logo.scm
%{prefix}/share/gimp/scripts/chalk.scm
%{prefix}/share/gimp/scripts/chip-away.scm
%{prefix}/share/gimp/scripts/chrome-it.scm
%{prefix}/share/gimp/scripts/chrome-logo.scm
%{prefix}/share/gimp/scripts/circuit.scm
%{prefix}/share/gimp/scripts/clothify.scm
%{prefix}/share/gimp/scripts/coffee.scm
%{prefix}/share/gimp/scripts/color-cycling.scm
%{prefix}/share/gimp/scripts/comic-logo.scm
%{prefix}/share/gimp/scripts/coolmetal-logo.scm
%{prefix}/share/gimp/scripts/copy-visible.scm
%{prefix}/share/gimp/scripts/crystal-logo.scm
%{prefix}/share/gimp/scripts/distress_selection.scm
%{prefix}/share/gimp/scripts/drop-shadow.scm
%{prefix}/share/gimp/scripts/erase-rows.scm
%{prefix}/share/gimp/scripts/fade-outline.scm
%{prefix}/share/gimp/scripts/flatland.scm
%{prefix}/share/gimp/scripts/font-map.scm
%{prefix}/share/gimp/scripts/frosty-logo.scm
%{prefix}/share/gimp/scripts/fuzzyborder.scm
%{prefix}/share/gimp/scripts/gimp-headers.scm
%{prefix}/share/gimp/scripts/gimp-labels.scm
%{prefix}/share/gimp/scripts/glossy.scm
%{prefix}/share/gimp/scripts/glowing-logo.scm
%{prefix}/share/gimp/scripts/gradient-bevel-logo.scm
%{prefix}/share/gimp/scripts/gradient-example.scm
%{prefix}/share/gimp/scripts/grid-system.scm
%{prefix}/share/gimp/scripts/hsv-graph.scm
%{prefix}/share/gimp/scripts/i26-gunya2.scm
%{prefix}/share/gimp/scripts/image-structure.scm
%{prefix}/share/gimp/scripts/land.scm
%{prefix}/share/gimp/scripts/lava.scm
%{prefix}/share/gimp/scripts/line-nova.scm
%{prefix}/share/gimp/scripts/old_photo.scm
%{prefix}/share/gimp/scripts/mkbrush.scm
%{prefix}/share/gimp/scripts/neon-logo.scm
%{prefix}/share/gimp/scripts/news-text.scm
%{prefix}/share/gimp/scripts/perspective-shadow.scm
%{prefix}/share/gimp/scripts/predator.scm
%{prefix}/share/gimp/scripts/pupi-button.scm
%{prefix}/share/gimp/scripts/rendermap.scm
%{prefix}/share/gimp/scripts/ripply-anim.scm
%{prefix}/share/gimp/scripts/round-corners.scm
%{prefix}/share/gimp/scripts/select_to_brush.scm
%{prefix}/share/gimp/scripts/select_to_image.scm
%{prefix}/share/gimp/scripts/selection-round.scm
%{prefix}/share/gimp/scripts/slide.scm
%{prefix}/share/gimp/scripts/sota-chrome-logo.scm
%{prefix}/share/gimp/scripts/speed-text.scm
%{prefix}/share/gimp/scripts/sphere.scm
%{prefix}/share/gimp/scripts/spinning_globe.scm
%{prefix}/share/gimp/scripts/starburst-logo.scm
%{prefix}/share/gimp/scripts/starscape-logo.scm
%{prefix}/share/gimp/scripts/swirltile.scm
%{prefix}/share/gimp/scripts/swirly-pattern.scm
%{prefix}/share/gimp/scripts/t-o-p-logo.scm
%{prefix}/share/gimp/scripts/text-circle.scm
%{prefix}/share/gimp/scripts/texture.jpg
%{prefix}/share/gimp/scripts/texture1.jpg
%{prefix}/share/gimp/scripts/texture2.jpg
%{prefix}/share/gimp/scripts/texture3.jpg
%{prefix}/share/gimp/scripts/textured-logo.scm
%{prefix}/share/gimp/scripts/title-header.scm
%{prefix}/share/gimp/scripts/tileblur.scm
%{prefix}/share/gimp/scripts/trochoid.scm
%{prefix}/share/gimp/scripts/truchet.scm
%{prefix}/share/gimp/scripts/unsharp-mask.scm
%{prefix}/share/gimp/scripts/waves-anim.scm
%{prefix}/share/gimp/scripts/weave.scm
%{prefix}/share/gimp/scripts/xach-effect.scm
%{prefix}/share/gimp/scripts/test-sphere.scm
%{prefix}/share/gimp/scripts/sel-to-anim-img.scm
%{prefix}/share/gimp/scripts/web-browser.scm

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

%{prefix}/share/gimp/brushes/10x10square.gbr
%{prefix}/share/gimp/brushes/10x10squareBlur.gbr
%{prefix}/share/gimp/brushes/11circle.gbr
%{prefix}/share/gimp/brushes/11fcircle.gbr
%{prefix}/share/gimp/brushes/13circle.gbr
%{prefix}/share/gimp/brushes/13fcircle.gbr
%{prefix}/share/gimp/brushes/15circle.gbr
%{prefix}/share/gimp/brushes/15fcircle.gbr
%{prefix}/share/gimp/brushes/17circle.gbr
%{prefix}/share/gimp/brushes/17fcircle.gbr
%{prefix}/share/gimp/brushes/19circle.gbr
%{prefix}/share/gimp/brushes/19fcircle.gbr
%{prefix}/share/gimp/brushes/1circle.gbr
%{prefix}/share/gimp/brushes/20x20square.gbr
%{prefix}/share/gimp/brushes/20x20squareBlur.gbr
%{prefix}/share/gimp/brushes/3circle.gbr
%{prefix}/share/gimp/brushes/3fcircle.gbr
%{prefix}/share/gimp/brushes/5circle.gbr
%{prefix}/share/gimp/brushes/5fcircle.gbr
%{prefix}/share/gimp/brushes/5x5square.gbr
%{prefix}/share/gimp/brushes/5x5squareBlur.gbr
%{prefix}/share/gimp/brushes/7circle.gbr
%{prefix}/share/gimp/brushes/7fcircle.gbr
%{prefix}/share/gimp/brushes/9circle.gbr
%{prefix}/share/gimp/brushes/9fcircle.gbr
%{prefix}/share/gimp/brushes/DStar11.gbr
%{prefix}/share/gimp/brushes/DStar17.gbr
%{prefix}/share/gimp/brushes/DStar25.gbr
%{prefix}/share/gimp/brushes/callig1.gbr
%{prefix}/share/gimp/brushes/callig2.gbr
%{prefix}/share/gimp/brushes/callig3.gbr
%{prefix}/share/gimp/brushes/callig4.gbr
%{prefix}/share/gimp/brushes/confetti.gbr
%{prefix}/share/gimp/brushes/dunes.gbr
%{prefix}/share/gimp/brushes/galaxy.gbr
%{prefix}/share/gimp/brushes/galaxy_big.gbr
%{prefix}/share/gimp/brushes/galaxy_small.gbr
%{prefix}/share/gimp/brushes/pepper.gpb
%{prefix}/share/gimp/brushes/pixel.gbr
%{prefix}/share/gimp/brushes/thegimp.gbr
%{prefix}/share/gimp/brushes/vine.gih
%{prefix}/share/gimp/brushes/xcf.gbr

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

%{prefix}/share/gimp/help/C/dialogs/layers/add_mask.html
%{prefix}/share/gimp/help/C/dialogs/layers/apply_mask.html
%{prefix}/share/gimp/help/C/dialogs/layers/edit_layer_attributes.html
%{prefix}/share/gimp/help/C/dialogs/layers/index.html
%{prefix}/share/gimp/help/C/dialogs/layers/layers.html
%{prefix}/share/gimp/help/C/dialogs/layers/merge_visible_layers.html
%{prefix}/share/gimp/help/C/dialogs/layers/new_layer.html
%{prefix}/share/gimp/help/C/dialogs/layers/resize_layer.html
%{prefix}/share/gimp/help/C/dialogs/layers/scale_layer.html
%{prefix}/share/gimp/help/C/dialogs/channels/channels.html
%{prefix}/share/gimp/help/C/dialogs/channels/edit_channel_attributes.html
%{prefix}/share/gimp/help/C/dialogs/channels/index.html
%{prefix}/share/gimp/help/C/dialogs/channels/new_channel.html
%{prefix}/share/gimp/help/C/dialogs/paths/export_path.html
%{prefix}/share/gimp/help/C/dialogs/paths/import_path.html
%{prefix}/share/gimp/help/C/dialogs/paths/index.html
%{prefix}/share/gimp/help/C/dialogs/paths/paths.html
%{prefix}/share/gimp/help/C/dialogs/paths/rename_path.html
%{prefix}/share/gimp/help/C/dialogs/palette_editor/import_palette.html
%{prefix}/share/gimp/help/C/dialogs/palette_editor/index.html
%{prefix}/share/gimp/help/C/dialogs/palette_editor/merge_palette.html
%{prefix}/share/gimp/help/C/dialogs/palette_editor/new_palette.html
%{prefix}/share/gimp/help/C/dialogs/palette_editor/palette_editor.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/copy_gradient.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/delete_gradient.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/gradient_editor.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/index.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/new_gradient.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/rename_gradient.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/replicate_segment.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/save_as_pov_ray.html
%{prefix}/share/gimp/help/C/dialogs/gradient_editor/split_segments_uniformly.html
%{prefix}/share/gimp/help/C/dialogs/display_filters/display_filters.html
%{prefix}/share/gimp/help/C/dialogs/display_filters/gamma.html
%{prefix}/share/gimp/help/C/dialogs/display_filters/index.html
%{prefix}/share/gimp/help/C/dialogs/color_selectors/built_in.html
%{prefix}/share/gimp/help/C/dialogs/color_selectors/gtk.html
%{prefix}/share/gimp/help/C/dialogs/color_selectors/index.html
%{prefix}/share/gimp/help/C/dialogs/color_selectors/triangle.html
%{prefix}/share/gimp/help/C/dialogs/color_selectors/watercolor.html
%{prefix}/share/gimp/help/C/dialogs/preferences/directories.html
%{prefix}/share/gimp/help/C/dialogs/preferences/display.html
%{prefix}/share/gimp/help/C/dialogs/preferences/environment.html
%{prefix}/share/gimp/help/C/dialogs/preferences/index.html
%{prefix}/share/gimp/help/C/dialogs/preferences/interface.html
%{prefix}/share/gimp/help/C/dialogs/preferences/monitor.html
%{prefix}/share/gimp/help/C/dialogs/preferences/new_file.html
%{prefix}/share/gimp/help/C/dialogs/preferences/preferences.html
%{prefix}/share/gimp/help/C/dialogs/preferences/session.html
%{prefix}/share/gimp/help/C/dialogs/about.html
%{prefix}/share/gimp/help/C/dialogs/border_selection.html
%{prefix}/share/gimp/help/C/dialogs/brush_editor.html
%{prefix}/share/gimp/help/C/dialogs/brush_selection.html
%{prefix}/share/gimp/help/C/dialogs/convert_to_indexed.html
%{prefix}/share/gimp/help/C/dialogs/copy_named.html
%{prefix}/share/gimp/help/C/dialogs/cut_named.html
%{prefix}/share/gimp/help/C/dialogs/device_status.html
%{prefix}/share/gimp/help/C/dialogs/document_index.html
%{prefix}/share/gimp/help/C/dialogs/edit_qmask_attributes.html
%{prefix}/share/gimp/help/C/dialogs/error_console.html
%{prefix}/share/gimp/help/C/dialogs/feather_selection.html
%{prefix}/share/gimp/help/C/dialogs/file_new.html
%{prefix}/share/gimp/help/C/dialogs/file_open.html
%{prefix}/share/gimp/help/C/dialogs/file_save.html
%{prefix}/share/gimp/help/C/dialogs/gradient_selection.html
%{prefix}/share/gimp/help/C/dialogs/grow_selection.html
%{prefix}/share/gimp/help/C/dialogs/help.html
%{prefix}/share/gimp/help/C/dialogs/index.html
%{prefix}/share/gimp/help/C/dialogs/indexed_palette.html
%{prefix}/share/gimp/help/C/dialogs/info_window.html
%{prefix}/share/gimp/help/C/dialogs/input_devices.html
%{prefix}/share/gimp/help/C/dialogs/layers_and_channels.html
%{prefix}/share/gimp/help/C/dialogs/module_browser.html
%{prefix}/share/gimp/help/C/dialogs/navigation_window.html
%{prefix}/share/gimp/help/C/dialogs/offset.html
%{prefix}/share/gimp/help/C/dialogs/palette_selection.html
%{prefix}/share/gimp/help/C/dialogs/paste_named.html
%{prefix}/share/gimp/help/C/dialogs/pattern_selection.html
%{prefix}/share/gimp/help/C/dialogs/really_close.html
%{prefix}/share/gimp/help/C/dialogs/really_quit.html
%{prefix}/share/gimp/help/C/dialogs/resize_image.html
%{prefix}/share/gimp/help/C/dialogs/scale_image.html
%{prefix}/share/gimp/help/C/dialogs/shrink_selection.html
%{prefix}/share/gimp/help/C/dialogs/tip_of_the_day.html
%{prefix}/share/gimp/help/C/dialogs/tool_options.html
%{prefix}/share/gimp/help/C/dialogs/undo_history.html
%{prefix}/share/gimp/help/C/tools/airbrush.html
%{prefix}/share/gimp/help/C/tools/bezier_select.html
%{prefix}/share/gimp/help/C/tools/blend.html
%{prefix}/share/gimp/help/C/tools/brightness_contrast.html
%{prefix}/share/gimp/help/C/tools/bucket_fill.html
%{prefix}/share/gimp/help/C/tools/by_color_select.html
%{prefix}/share/gimp/help/C/tools/clone.html
%{prefix}/share/gimp/help/C/tools/color_balance.html
%{prefix}/share/gimp/help/C/tools/color_picker.html
%{prefix}/share/gimp/help/C/tools/convolve.html
%{prefix}/share/gimp/help/C/tools/crop.html
%{prefix}/share/gimp/help/C/tools/curves.html
%{prefix}/share/gimp/help/C/tools/dodgeburn.html
%{prefix}/share/gimp/help/C/tools/ellipse_select.html
%{prefix}/share/gimp/help/C/tools/eraser.html
%{prefix}/share/gimp/help/C/tools/flip.html
%{prefix}/share/gimp/help/C/tools/free_select.html
%{prefix}/share/gimp/help/C/tools/fuzzy_select.html
%{prefix}/share/gimp/help/C/tools/histogram.html
%{prefix}/share/gimp/help/C/tools/hue_saturation.html
%{prefix}/share/gimp/help/C/tools/index.html
%{prefix}/share/gimp/help/C/tools/ink.html
%{prefix}/share/gimp/help/C/tools/intelligent_scissors.html
%{prefix}/share/gimp/help/C/tools/levels.html
%{prefix}/share/gimp/help/C/tools/magnify.html
%{prefix}/share/gimp/help/C/tools/measure.html
%{prefix}/share/gimp/help/C/tools/move.html
%{prefix}/share/gimp/help/C/tools/paintbrush.html
%{prefix}/share/gimp/help/C/tools/path.html
%{prefix}/share/gimp/help/C/tools/pencil.html
%{prefix}/share/gimp/help/C/tools/posterize.html
%{prefix}/share/gimp/help/C/tools/rect_select.html
%{prefix}/share/gimp/help/C/tools/smudge.html
%{prefix}/share/gimp/help/C/tools/text.html
%{prefix}/share/gimp/help/C/tools/threshold.html
%{prefix}/share/gimp/help/C/tools/transform.html
%{prefix}/share/gimp/help/C/tools/transform_perspective.html
%{prefix}/share/gimp/help/C/tools/transform_rotate.html
%{prefix}/share/gimp/help/C/tools/transform_scale.html
%{prefix}/share/gimp/help/C/tools/transform_shear.html
%{prefix}/share/gimp/help/C/tools/xinput_airbrush.html
%{prefix}/share/gimp/help/C/layers/stack/index.html
%{prefix}/share/gimp/help/C/layers/stack/stack.html
%{prefix}/share/gimp/help/C/layers/add_alpha_channel.html
%{prefix}/share/gimp/help/C/layers/alpha_to_selection.html
%{prefix}/share/gimp/help/C/layers/anchor_layer.html
%{prefix}/share/gimp/help/C/layers/delete_layer.html
%{prefix}/share/gimp/help/C/layers/duplicate_layer.html
%{prefix}/share/gimp/help/C/layers/flatten_image.html
%{prefix}/share/gimp/help/C/layers/index.html
%{prefix}/share/gimp/help/C/layers/mask_to_selection.html
%{prefix}/share/gimp/help/C/layers/merge_down.html
%{prefix}/share/gimp/help/C/channels/channel_to_selection.html
%{prefix}/share/gimp/help/C/channels/channels.html
%{prefix}/share/gimp/help/C/channels/delete_channel.html
%{prefix}/share/gimp/help/C/channels/duplicate_channel.html
%{prefix}/share/gimp/help/C/channels/index.html
%{prefix}/share/gimp/help/C/channels/lower_channel.html
%{prefix}/share/gimp/help/C/channels/raise_channel.html
%{prefix}/share/gimp/help/C/paths/copy_path.html
%{prefix}/share/gimp/help/C/paths/delete_path.html
%{prefix}/share/gimp/help/C/paths/duplicate_path.html
%{prefix}/share/gimp/help/C/paths/index.html
%{prefix}/share/gimp/help/C/paths/new_path.html
%{prefix}/share/gimp/help/C/paths/paste_path.html
%{prefix}/share/gimp/help/C/paths/path_to_selection.html
%{prefix}/share/gimp/help/C/paths/stroke_path.html
%{prefix}/share/gimp/help/C/toolbox/index.html
%{prefix}/share/gimp/help/C/toolbox/toolbox.html
%{prefix}/share/gimp/help/C/image/edit/clear.html
%{prefix}/share/gimp/help/C/image/edit/copy.html
%{prefix}/share/gimp/help/C/image/edit/cut.html
%{prefix}/share/gimp/help/C/image/edit/fill.html
%{prefix}/share/gimp/help/C/image/edit/index.html
%{prefix}/share/gimp/help/C/image/edit/paste.html
%{prefix}/share/gimp/help/C/image/edit/paste_as_new.html
%{prefix}/share/gimp/help/C/image/edit/paste_into.html
%{prefix}/share/gimp/help/C/image/edit/redo.html
%{prefix}/share/gimp/help/C/image/edit/stroke.html
%{prefix}/share/gimp/help/C/image/edit/undo.html
%{prefix}/share/gimp/help/C/image/select/all.html
%{prefix}/share/gimp/help/C/image/select/float.html
%{prefix}/share/gimp/help/C/image/select/index.html
%{prefix}/share/gimp/help/C/image/select/invert.html
%{prefix}/share/gimp/help/C/image/select/none.html
%{prefix}/share/gimp/help/C/image/select/save_to_channel.html
%{prefix}/share/gimp/help/C/image/select/sharpen.html
%{prefix}/share/gimp/help/C/image/view/dot_for_dot.html
%{prefix}/share/gimp/help/C/image/view/index.html
%{prefix}/share/gimp/help/C/image/view/new_view.html
%{prefix}/share/gimp/help/C/image/view/shrink_wrap.html
%{prefix}/share/gimp/help/C/image/view/snap_to_guides.html
%{prefix}/share/gimp/help/C/image/view/toggle_guides.html
%{prefix}/share/gimp/help/C/image/view/toggle_rulers.html
%{prefix}/share/gimp/help/C/image/view/toggle_selection.html
%{prefix}/share/gimp/help/C/image/view/toggle_statusbar.html
%{prefix}/share/gimp/help/C/image/view/zoom.html
%{prefix}/share/gimp/help/C/image/image/transforms/index.html
%{prefix}/share/gimp/help/C/image/image/colors/desaturate.html
%{prefix}/share/gimp/help/C/image/image/colors/equalize.html
%{prefix}/share/gimp/help/C/image/image/colors/index.html
%{prefix}/share/gimp/help/C/image/image/colors/invert.html
%{prefix}/share/gimp/help/C/image/image/convert_to_grayscale.html
%{prefix}/share/gimp/help/C/image/image/convert_to_rgb.html
%{prefix}/share/gimp/help/C/image/image/duplicate.html
%{prefix}/share/gimp/help/C/image/image/index.html
%{prefix}/share/gimp/help/C/image/image_window.html
%{prefix}/share/gimp/help/C/image/index.html
%{prefix}/share/gimp/help/C/open/index.html
%{prefix}/share/gimp/help/C/open/open_by_extension.html
%{prefix}/share/gimp/help/C/save/index.html
%{prefix}/share/gimp/help/C/save/save_by_extension.html
%{prefix}/share/gimp/help/C/filters/alienmap.html
%{prefix}/share/gimp/help/C/filters/alienmap2.html
%{prefix}/share/gimp/help/C/filters/align_layers.html
%{prefix}/share/gimp/help/C/filters/animationplay.html
%{prefix}/share/gimp/help/C/filters/animoptimize.html
%{prefix}/share/gimp/help/C/filters/apply_lens.html
%{prefix}/share/gimp/help/C/filters/autocrop.html
%{prefix}/share/gimp/help/C/filters/autostretch_hsv.html
%{prefix}/share/gimp/help/C/filters/blinds.html
%{prefix}/share/gimp/help/C/filters/blur.html
%{prefix}/share/gimp/help/C/filters/bmp.html
%{prefix}/share/gimp/help/C/filters/borderaverage.html
%{prefix}/share/gimp/help/C/filters/bumpmap.html
%{prefix}/share/gimp/help/C/filters/bz2.html
%{prefix}/share/gimp/help/C/filters/c_astretch.html
%{prefix}/share/gimp/help/C/filters/cel.html
%{prefix}/share/gimp/help/C/filters/checkerboard.html
%{prefix}/share/gimp/help/C/filters/cml_explorer.html
%{prefix}/share/gimp/help/C/filters/color_enhance.html
%{prefix}/share/gimp/help/C/filters/colorify.html
%{prefix}/share/gimp/help/C/filters/compose.html
%{prefix}/share/gimp/help/C/filters/convmatrix.html
%{prefix}/share/gimp/help/C/filters/csource.html
%{prefix}/share/gimp/help/C/filters/cubism.html
%{prefix}/share/gimp/help/C/filters/curve_bend.html
%{prefix}/share/gimp/help/C/filters/dbbrowser.html
%{prefix}/share/gimp/help/C/filters/decompose.html
%{prefix}/share/gimp/help/C/filters/deinterlace.html
%{prefix}/share/gimp/help/C/filters/depthmerge.html
%{prefix}/share/gimp/help/C/filters/despeckle.html
%{prefix}/share/gimp/help/C/filters/destripe.html
%{prefix}/share/gimp/help/C/filters/diffraction.html
%{prefix}/share/gimp/help/C/filters/displace.html
%{prefix}/share/gimp/help/C/filters/edge.html
%{prefix}/share/gimp/help/C/filters/emboss.html
%{prefix}/share/gimp/help/C/filters/engrave.html
%{prefix}/share/gimp/help/C/filters/exchange.html
%{prefix}/share/gimp/help/C/filters/faxg3.html
%{prefix}/share/gimp/help/C/filters/film.html
%{prefix}/share/gimp/help/C/filters/fits.html
%{prefix}/share/gimp/help/C/filters/flame.html
%{prefix}/share/gimp/help/C/filters/flarefx.html
%{prefix}/share/gimp/help/C/filters/fp.html
%{prefix}/share/gimp/help/C/filters/gap_filter.html
%{prefix}/share/gimp/help/C/filters/fractalexplorer.html
%{prefix}/share/gimp/help/C/filters/fractaltrace.html
%{prefix}/share/gimp/help/C/filters/gap_plugins.html
%{prefix}/share/gimp/help/C/filters/gauss_iir.html
%{prefix}/share/gimp/help/C/filters/gauss_rle.html
%{prefix}/share/gimp/help/C/filters/gbr.html
%{prefix}/share/gimp/help/C/filters/gdyntext.html
%{prefix}/share/gimp/help/C/filters/gee.html
%{prefix}/share/gimp/help/C/filters/gfig.html
%{prefix}/share/gimp/help/C/filters/gflare.html
%{prefix}/share/gimp/help/C/filters/gfli.html
%{prefix}/share/gimp/help/C/filters/gicon.html
%{prefix}/share/gimp/help/C/filters/gif.html
%{prefix}/share/gimp/help/C/filters/gifload.html
%{prefix}/share/gimp/help/C/filters/gimpressionist.html
%{prefix}/share/gimp/help/C/filters/glasstile.html
%{prefix}/share/gimp/help/C/filters/gpb.html
%{prefix}/share/gimp/help/C/filters/gqbist.html
%{prefix}/share/gimp/help/C/filters/gradmap.html
%{prefix}/share/gimp/help/C/filters/grid.html
%{prefix}/share/gimp/help/C/filters/gtm.html
%{prefix}/share/gimp/help/C/filters/guillotine.html
%{prefix}/share/gimp/help/C/filters/gz.html
%{prefix}/share/gimp/help/C/filters/header.html
%{prefix}/share/gimp/help/C/filters/hot.html
%{prefix}/share/gimp/help/C/filters/hrz.html
%{prefix}/share/gimp/help/C/filters/ifscompose.html
%{prefix}/share/gimp/help/C/filters/illusion.html
%{prefix}/share/gimp/help/C/filters/imagemap.html
%{prefix}/share/gimp/help/C/filters/index.html
%{prefix}/share/gimp/help/C/filters/iwarp.html
%{prefix}/share/gimp/help/C/filters/jigsaw.html
%{prefix}/share/gimp/help/C/filters/jpeg.html
%{prefix}/share/gimp/help/C/filters/laplace.html
%{prefix}/share/gimp/help/C/filters/lic.html
%{prefix}/share/gimp/help/C/filters/lighting.html
%{prefix}/share/gimp/help/C/filters/mail.html
%{prefix}/share/gimp/help/C/filters/mapcolor.html
%{prefix}/share/gimp/help/C/filters/mapobject.html
%{prefix}/share/gimp/help/C/filters/max_rgb.html
%{prefix}/share/gimp/help/C/filters/maze.html
%{prefix}/share/gimp/help/C/filters/mblur.html
%{prefix}/share/gimp/help/C/filters/mosaic.html
%{prefix}/share/gimp/help/C/filters/newsprint.html
%{prefix}/share/gimp/help/C/filters/nlfilt.html
%{prefix}/share/gimp/help/C/filters/noisify.html
%{prefix}/share/gimp/help/C/filters/normalize.html
%{prefix}/share/gimp/help/C/filters/nova.html
%{prefix}/share/gimp/help/C/filters/oilify.html
%{prefix}/share/gimp/help/C/filters/pagecurl.html
%{prefix}/share/gimp/help/C/filters/papertile.html
%{prefix}/share/gimp/help/C/filters/pat.html
%{prefix}/share/gimp/help/C/filters/pcx.html
%{prefix}/share/gimp/help/C/filters/pix.html
%{prefix}/share/gimp/help/C/filters/pixelize.html
%{prefix}/share/gimp/help/C/filters/plasma.html
%{prefix}/share/gimp/help/C/filters/plugindetails.html
%{prefix}/share/gimp/help/C/filters/png.html
%{prefix}/share/gimp/help/C/filters/pnm.html
%{prefix}/share/gimp/help/C/filters/polar.html
%{prefix}/share/gimp/help/C/filters/print.html
%{prefix}/share/gimp/help/C/filters/ps.html
%{prefix}/share/gimp/help/C/filters/psd.html
%{prefix}/share/gimp/help/C/filters/psp.html
%{prefix}/share/gimp/help/C/filters/randomize.html
%{prefix}/share/gimp/help/C/filters/rcm.html
%{prefix}/share/gimp/help/C/filters/repeat_last.html
%{prefix}/share/gimp/help/C/filters/reshow_last.html
%{prefix}/share/gimp/help/C/filters/ripple.html
%{prefix}/share/gimp/help/C/filters/rotate.html
%{prefix}/share/gimp/help/C/filters/rotators.html
%{prefix}/share/gimp/help/C/filters/sample_colorize.html
%{prefix}/share/gimp/help/C/filters/scatter_hsv.html
%{prefix}/share/gimp/help/C/filters/screenshot.html
%{prefix}/share/gimp/help/C/filters/script-fu.html
%{prefix}/share/gimp/help/C/filters/sel2path.html
%{prefix}/share/gimp/help/C/filters/sel_gauss.html
%{prefix}/share/gimp/help/C/filters/semiflatten.html
%{prefix}/share/gimp/help/C/filters/sgi.html
%{prefix}/share/gimp/help/C/filters/sharpen.html
%{prefix}/share/gimp/help/C/filters/shift.html
%{prefix}/share/gimp/help/C/filters/sinus.html
%{prefix}/share/gimp/help/C/filters/smooth_palette.html
%{prefix}/share/gimp/help/C/filters/snoise.html
%{prefix}/share/gimp/help/C/filters/sobel.html
%{prefix}/share/gimp/help/C/filters/sparkle.html
%{prefix}/share/gimp/help/C/filters/spheredesigner.html
%{prefix}/share/gimp/help/C/filters/spread.html
%{prefix}/share/gimp/help/C/filters/struc.html
%{prefix}/share/gimp/help/C/filters/sunras.html
%{prefix}/share/gimp/help/C/filters/tga.html
%{prefix}/share/gimp/help/C/filters/threshold_alpha.html
%{prefix}/share/gimp/help/C/filters/tiff.html
%{prefix}/share/gimp/help/C/filters/tile.html
%{prefix}/share/gimp/help/C/filters/tileit.html
%{prefix}/share/gimp/help/C/filters/tiler.html
%{prefix}/share/gimp/help/C/filters/unsharp.html
%{prefix}/share/gimp/help/C/filters/url.html
%{prefix}/share/gimp/help/C/filters/video.html
%{prefix}/share/gimp/help/C/filters/vinvert.html
%{prefix}/share/gimp/help/C/filters/vpropagate.html
%{prefix}/share/gimp/help/C/filters/warp.html
%{prefix}/share/gimp/help/C/filters/waves.html
%{prefix}/share/gimp/help/C/filters/webbrowser.html
%{prefix}/share/gimp/help/C/filters/whirlpinch.html
%{prefix}/share/gimp/help/C/filters/wind.html
%{prefix}/share/gimp/help/C/filters/wmf.html
%{prefix}/share/gimp/help/C/filters/xbm.html
%{prefix}/share/gimp/help/C/filters/xjt.html
%{prefix}/share/gimp/help/C/filters/xpm.html
%{prefix}/share/gimp/help/C/filters/xwd.html
%{prefix}/share/gimp/help/C/filters/zealouscrop.html
%{prefix}/share/gimp/help/C/file/close.html
%{prefix}/share/gimp/help/C/file/index.html
%{prefix}/share/gimp/help/C/file/last_opened.html
%{prefix}/share/gimp/help/C/file/quit.html
%{prefix}/share/gimp/help/C/file/revert.html
%{prefix}/share/gimp/help/C/contents.html
%{prefix}/share/gimp/help/C/index.html
%{prefix}/share/gimp/help/C/welcome.html
%{prefix}/share/gimp/help/C/modes.html
%{prefix}/share/gimp/help/images/eek.png
%{prefix}/share/gimp/help/images/wilber.png
%{prefix}/share/gimp/gimprc
%{prefix}/share/gimp/gimprc_user
%{prefix}/share/gimp/gtkrc
%{prefix}/share/gimp/unitrc
%{prefix}/share/gimp/gimp_logo.ppm
%{prefix}/share/gimp/gimp_splash.ppm
%{prefix}/share/gimp/gimp1_1_splash.ppm
%{prefix}/share/gimp/ps-menurc
%{prefix}/share/gimp/gtkrc.forest2


%defattr (444, bin, bin, 555)
%lang(cs)      /usr/share/locale/cs/LC_MESSAGES/gimp.mo
%lang(da)      /usr/share/locale/da/LC_MESSAGES/gimp.mo
%lang(de)      /usr/share/locale/de/LC_MESSAGES/gimp.mo
%lang(fi)      /usr/share/locale/fi/LC_MESSAGES/gimp.mo
%lang(fr)      /usr/share/locale/fr/LC_MESSAGES/gimp.mo
%lang(hu)      /usr/share/locale/hu/LC_MESSAGES/gimp.mo
%lang(it)      /usr/share/locale/it/LC_MESSAGES/gimp.mo
%lang(ja)      /usr/share/locale/ja/LC_MESSAGES/gimp.mo
%lang(ko)      /usr/share/locale/ko/LC_MESSAGES/gimp.mo
%lang(nl)      /usr/share/locale/nl/LC_MESSAGES/gimp.mo
%lang(no)      /usr/share/locale/no/LC_MESSAGES/gimp.mo
%lang(pl)      /usr/share/locale/pl/LC_MESSAGES/gimp.mo
%lang(ru)      /usr/share/locale/ru/LC_MESSAGES/gimp.mo
%lang(sk)      /usr/share/locale/sk/LC_MESSAGES/gimp.mo
%lang(sv)      /usr/share/locale/sv/LC_MESSAGES/gimp.mo

%defattr (0555, bin, bin)
%{prefix}/share/gimp/user_install

%{prefix}/lib/perl5/site_perl/%{archname}/auto/Gimp/Lib/Lib.so
%{prefix}/lib/perl5/site_perl/%{archname}/auto/Gimp/Net/Net.so
%{prefix}/lib/perl5/site_perl/%{archname}/auto/Gimp/UI/UI.so
%{prefix}/lib/perl5/site_perl/%{archname}/auto/Gimp/Gimp.so

#%{prefix}/lib/perl5/%{archname}/5.00405/perllocal.pod

%{prefix}/lib/perl5/man/man3/Gimp::Module.3
%{prefix}/lib/perl5/man/man3/Gimp::Lib.3
%{prefix}/lib/perl5/man/man3/Gimp::Pixel.3
%{prefix}/lib/perl5/man/man3/Gimp::Data.3
%{prefix}/lib/perl5/man/man3/Gimp::Fu.3
%{prefix}/lib/perl5/man/man3/Gimp::Feature.3
%{prefix}/lib/perl5/man/man3/Gimp::Util.3
%{prefix}/lib/perl5/man/man3/Gimp::PDL.3
%{prefix}/lib/perl5/man/man3/Gimp::Pod.3
%{prefix}/lib/perl5/man/man3/Gimp::OO.3
%{prefix}/lib/perl5/man/man3/Gimp::Compat.3
%{prefix}/lib/perl5/man/man3/Gimp::Net.3
%{prefix}/lib/perl5/man/man3/Gimp::UI.3
%{prefix}/lib/perl5/man/man3/Gimp::basewidget.3
%{prefix}/lib/perl5/man/man3/UI::UI.3
%{prefix}/lib/perl5/man/man3/UI::basewidget.3
%{prefix}/lib/perl5/man/man3/Gimp.3
%{prefix}/lib/perl5/man/man3/Net::Net.3

%{prefix}/lib/perl5/site_perl/Gimp/Data.pm
%{prefix}/lib/perl5/site_perl/Gimp/Fu.pm
%{prefix}/lib/perl5/site_perl/Gimp/Feature.pm
%{prefix}/lib/perl5/site_perl/Gimp/Util.pm
%{prefix}/lib/perl5/site_perl/Gimp/UI.pm
%{prefix}/lib/perl5/site_perl/Gimp/basewidget.pm
%{prefix}/lib/perl5/site_perl/Gimp/PDL.pm
%{prefix}/lib/perl5/site_perl/Gimp/Net.pm
%{prefix}/lib/perl5/site_perl/Gimp/Module.pm
%{prefix}/lib/perl5/site_perl/Gimp/Config.pm
%{prefix}/lib/perl5/site_perl/Gimp/Lib.pm
%{prefix}/lib/perl5/site_perl/Gimp/Pixel.pod
%{prefix}/lib/perl5/site_perl/Gimp/Pod.pm
%{prefix}/lib/perl5/site_perl/Gimp/Compat.pm
%{prefix}/lib/perl5/site_perl/Gimp/OO.pod
%{prefix}/lib/perl5/site_perl/Gimp.pm

%{prefix}/lib/libgimp-%{subver}.so.14.0.0
%{prefix}/lib/libgimp-%{subver}.so.14
%{prefix}/lib/libgimpui-%{subver}.so.14.0.0
%{prefix}/lib/libgimpui-%{subver}.so.14
%{prefix}/lib/libgck-%{subver}.so.14.0.0
%{prefix}/lib/libgck-%{subver}.so.14

%{prefix}/bin/gimp
%{prefix}/bin/scm2scm
%{prefix}/bin/scm2perl
%{prefix}/bin/embedxpm
%{prefix}/bin/gimpdoc
%{prefix}/bin/xcftopnm

%{prefix}/lib/gimp/%{subver}/plug-ins/dbbrowser
%{prefix}/lib/gimp/%{subver}/plug-ins/script-fu
%{prefix}/lib/gimp/%{subver}/plug-ins/PDB
%{prefix}/lib/gimp/%{subver}/plug-ins/Perl-Server
%{prefix}/lib/gimp/%{subver}/plug-ins/animate_cells
%{prefix}/lib/gimp/%{subver}/plug-ins/avi
%{prefix}/lib/gimp/%{subver}/plug-ins/blowinout.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/border.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/bricks
%{prefix}/lib/gimp/%{subver}/plug-ins/burst
%{prefix}/lib/gimp/%{subver}/plug-ins/centerguide
%{prefix}/lib/gimp/%{subver}/plug-ins/colorhtml
%{prefix}/lib/gimp/%{subver}/plug-ins/dataurl
%{prefix}/lib/gimp/%{subver}/plug-ins/ditherize.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/feedback.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/fire
%{prefix}/lib/gimp/%{subver}/plug-ins/fit-text
%{prefix}/lib/gimp/%{subver}/plug-ins/font_table
%{prefix}/lib/gimp/%{subver}/plug-ins/frame_filter
%{prefix}/lib/gimp/%{subver}/plug-ins/frame_reshuffle
%{prefix}/lib/gimp/%{subver}/plug-ins/gap-vcr
%{prefix}/lib/gimp/%{subver}/plug-ins/gimpmagick
%{prefix}/lib/gimp/%{subver}/plug-ins/glowing_steel
%{prefix}/lib/gimp/%{subver}/plug-ins/goldenmean
%{prefix}/lib/gimp/%{subver}/plug-ins/gouge
%{prefix}/lib/gimp/%{subver}/plug-ins/guide_remove
%{prefix}/lib/gimp/%{subver}/plug-ins/guidegrid
%{prefix}/lib/gimp/%{subver}/plug-ins/guides_to_selection
%{prefix}/lib/gimp/%{subver}/plug-ins/image_tile
%{prefix}/lib/gimp/%{subver}/plug-ins/innerbevel
%{prefix}/lib/gimp/%{subver}/plug-ins/layerfuncs
%{prefix}/lib/gimp/%{subver}/plug-ins/logulator
%{prefix}/lib/gimp/%{subver}/plug-ins/map_to_gradient
%{prefix}/lib/gimp/%{subver}/plug-ins/miff
%{prefix}/lib/gimp/%{subver}/plug-ins/mirrorsplit
%{prefix}/lib/gimp/%{subver}/plug-ins/parasite-editor
%{prefix}/lib/gimp/%{subver}/plug-ins/perlcc
%{prefix}/lib/gimp/%{subver}/plug-ins/perlotine
%{prefix}/lib/gimp/%{subver}/plug-ins/pixelmap
%{prefix}/lib/gimp/%{subver}/plug-ins/povray
%{prefix}/lib/gimp/%{subver}/plug-ins/prep4gif.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/randomart1
%{prefix}/lib/gimp/%{subver}/plug-ins/randomblends
%{prefix}/lib/gimp/%{subver}/plug-ins/repdup
%{prefix}/lib/gimp/%{subver}/plug-ins/roundrectsel
%{prefix}/lib/gimp/%{subver}/plug-ins/scratches.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/stampify
%{prefix}/lib/gimp/%{subver}/plug-ins/stamps
%{prefix}/lib/gimp/%{subver}/plug-ins/terral_text
%{prefix}/lib/gimp/%{subver}/plug-ins/tex-to-float
%{prefix}/lib/gimp/%{subver}/plug-ins/triangle
%{prefix}/lib/gimp/%{subver}/plug-ins/view3d.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/yinyang
%{prefix}/lib/gimp/%{subver}/plug-ins/webify.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/windify.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/xachlego.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/xachshadow.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/xachvision.pl
%{prefix}/lib/gimp/%{subver}/plug-ins/AlienMap
%{prefix}/lib/gimp/%{subver}/plug-ins/AlienMap2
%{prefix}/lib/gimp/%{subver}/plug-ins/FractalExplorer
%{prefix}/lib/gimp/%{subver}/plug-ins/Lighting
%{prefix}/lib/gimp/%{subver}/plug-ins/MapObject
%{prefix}/lib/gimp/%{subver}/plug-ins/bmp
%{prefix}/lib/gimp/%{subver}/plug-ins/borderaverage
%{prefix}/lib/gimp/%{subver}/plug-ins/faxg3
%{prefix}/lib/gimp/%{subver}/plug-ins/fits
%{prefix}/lib/gimp/%{subver}/plug-ins/flame
%{prefix}/lib/gimp/%{subver}/plug-ins/fp
%{prefix}/lib/gimp/%{subver}/plug-ins/gap_plugins
%{prefix}/lib/gimp/%{subver}/plug-ins/gap_filter
%{prefix}/lib/gimp/%{subver}/plug-ins/gap_frontends
%{prefix}/lib/gimp/%{subver}/plug-ins/gdyntext
%{prefix}/lib/gimp/%{subver}/plug-ins/gfig
%{prefix}/lib/gimp/%{subver}/plug-ins/gflare
%{prefix}/lib/gimp/%{subver}/plug-ins/gfli
%{prefix}/lib/gimp/%{subver}/plug-ins/gimpressionist
%{prefix}/lib/gimp/%{subver}/plug-ins/helpbrowser
%{prefix}/lib/gimp/%{subver}/plug-ins/ifscompose
%{prefix}/lib/gimp/%{subver}/plug-ins/imagemap
%{prefix}/lib/gimp/%{subver}/plug-ins/maze
%{prefix}/lib/gimp/%{subver}/plug-ins/mosaic
%{prefix}/lib/gimp/%{subver}/plug-ins/pagecurl
%{prefix}/lib/gimp/%{subver}/plug-ins/print
%{prefix}/lib/gimp/%{subver}/plug-ins/rcm
%{prefix}/lib/gimp/%{subver}/plug-ins/sgi
%{prefix}/lib/gimp/%{subver}/plug-ins/sel2path
%{prefix}/lib/gimp/%{subver}/plug-ins/sinus
%{prefix}/lib/gimp/%{subver}/plug-ins/struc
%{prefix}/lib/gimp/%{subver}/plug-ins/unsharp
%{prefix}/lib/gimp/%{subver}/plug-ins/webbrowser
%{prefix}/lib/gimp/%{subver}/plug-ins/xjt
%{prefix}/lib/gimp/%{subver}/plug-ins/CEL
%{prefix}/lib/gimp/%{subver}/plug-ins/CML_explorer
%{prefix}/lib/gimp/%{subver}/plug-ins/align_layers
%{prefix}/lib/gimp/%{subver}/plug-ins/animationplay
%{prefix}/lib/gimp/%{subver}/plug-ins/animoptimize
%{prefix}/lib/gimp/%{subver}/plug-ins/apply_lens
%{prefix}/lib/gimp/%{subver}/plug-ins/autocrop
%{prefix}/lib/gimp/%{subver}/plug-ins/autostretch_hsv
%{prefix}/lib/gimp/%{subver}/plug-ins/blinds
%{prefix}/lib/gimp/%{subver}/plug-ins/blur
%{prefix}/lib/gimp/%{subver}/plug-ins/bumpmap
%{prefix}/lib/gimp/%{subver}/plug-ins/bz2
%{prefix}/lib/gimp/%{subver}/plug-ins/c_astretch
%{prefix}/lib/gimp/%{subver}/plug-ins/checkerboard
%{prefix}/lib/gimp/%{subver}/plug-ins/color_enhance
%{prefix}/lib/gimp/%{subver}/plug-ins/colorify
%{prefix}/lib/gimp/%{subver}/plug-ins/colortoalpha
%{prefix}/lib/gimp/%{subver}/plug-ins/compose
%{prefix}/lib/gimp/%{subver}/plug-ins/convmatrix
%{prefix}/lib/gimp/%{subver}/plug-ins/csource
%{prefix}/lib/gimp/%{subver}/plug-ins/cubism
%{prefix}/lib/gimp/%{subver}/plug-ins/curve_bend
%{prefix}/lib/gimp/%{subver}/plug-ins/decompose
%{prefix}/lib/gimp/%{subver}/plug-ins/deinterlace
%{prefix}/lib/gimp/%{subver}/plug-ins/depthmerge
%{prefix}/lib/gimp/%{subver}/plug-ins/despeckle
%{prefix}/lib/gimp/%{subver}/plug-ins/destripe
%{prefix}/lib/gimp/%{subver}/plug-ins/diffraction
%{prefix}/lib/gimp/%{subver}/plug-ins/displace
%{prefix}/lib/gimp/%{subver}/plug-ins/edge
%{prefix}/lib/gimp/%{subver}/plug-ins/emboss
%{prefix}/lib/gimp/%{subver}/plug-ins/engrave
%{prefix}/lib/gimp/%{subver}/plug-ins/exchange
%{prefix}/lib/gimp/%{subver}/plug-ins/film
%{prefix}/lib/gimp/%{subver}/plug-ins/flarefx
%{prefix}/lib/gimp/%{subver}/plug-ins/fractaltrace
%{prefix}/lib/gimp/%{subver}/plug-ins/gauss_iir
%{prefix}/lib/gimp/%{subver}/plug-ins/gauss_rle
%{prefix}/lib/gimp/%{subver}/plug-ins/gbr
%{prefix}/lib/gimp/%{subver}/plug-ins/gee
%{prefix}/lib/gimp/%{subver}/plug-ins/gicon
%{prefix}/lib/gimp/%{subver}/plug-ins/gif
%{prefix}/lib/gimp/%{subver}/plug-ins/gifload
%{prefix}/lib/gimp/%{subver}/plug-ins/glasstile
%{prefix}/lib/gimp/%{subver}/plug-ins/gpb
%{prefix}/lib/gimp/%{subver}/plug-ins/gqbist
%{prefix}/lib/gimp/%{subver}/plug-ins/gradmap
%{prefix}/lib/gimp/%{subver}/plug-ins/grid
%{prefix}/lib/gimp/%{subver}/plug-ins/gtm
%{prefix}/lib/gimp/%{subver}/plug-ins/guillotine
%{prefix}/lib/gimp/%{subver}/plug-ins/gz
%{prefix}/lib/gimp/%{subver}/plug-ins/header
%{prefix}/lib/gimp/%{subver}/plug-ins/hot
%{prefix}/lib/gimp/%{subver}/plug-ins/hrz
%{prefix}/lib/gimp/%{subver}/plug-ins/illusion
%{prefix}/lib/gimp/%{subver}/plug-ins/iwarp
%{prefix}/lib/gimp/%{subver}/plug-ins/jigsaw
%{prefix}/lib/gimp/%{subver}/plug-ins/jpeg
%{prefix}/lib/gimp/%{subver}/plug-ins/laplace
%{prefix}/lib/gimp/%{subver}/plug-ins/lic
%{prefix}/lib/gimp/%{subver}/plug-ins/mail
%{prefix}/lib/gimp/%{subver}/plug-ins/mapcolor
%{prefix}/lib/gimp/%{subver}/plug-ins/max_rgb
%{prefix}/lib/gimp/%{subver}/plug-ins/mblur
%{prefix}/lib/gimp/%{subver}/plug-ins/newsprint
%{prefix}/lib/gimp/%{subver}/plug-ins/nlfilt
%{prefix}/lib/gimp/%{subver}/plug-ins/noisify
%{prefix}/lib/gimp/%{subver}/plug-ins/normalize
%{prefix}/lib/gimp/%{subver}/plug-ins/nova
%{prefix}/lib/gimp/%{subver}/plug-ins/oilify
%{prefix}/lib/gimp/%{subver}/plug-ins/papertile
%{prefix}/lib/gimp/%{subver}/plug-ins/pat
%{prefix}/lib/gimp/%{subver}/plug-ins/pcx
%{prefix}/lib/gimp/%{subver}/plug-ins/pix
%{prefix}/lib/gimp/%{subver}/plug-ins/pixelize
%{prefix}/lib/gimp/%{subver}/plug-ins/plasma
%{prefix}/lib/gimp/%{subver}/plug-ins/plugindetails
%{prefix}/lib/gimp/%{subver}/plug-ins/png
%{prefix}/lib/gimp/%{subver}/plug-ins/pnm
%{prefix}/lib/gimp/%{subver}/plug-ins/polar
%{prefix}/lib/gimp/%{subver}/plug-ins/ps
%{prefix}/lib/gimp/%{subver}/plug-ins/psd
%{prefix}/lib/gimp/%{subver}/plug-ins/psp
%{prefix}/lib/gimp/%{subver}/plug-ins/randomize
%{prefix}/lib/gimp/%{subver}/plug-ins/ripple
%{prefix}/lib/gimp/%{subver}/plug-ins/rotate
%{prefix}/lib/gimp/%{subver}/plug-ins/sample_colorize
%{prefix}/lib/gimp/%{subver}/plug-ins/scatter_hsv
%{prefix}/lib/gimp/%{subver}/plug-ins/screenshot
%{prefix}/lib/gimp/%{subver}/plug-ins/sel_gauss
%{prefix}/lib/gimp/%{subver}/plug-ins/semiflatten
%{prefix}/lib/gimp/%{subver}/plug-ins/sharpen
%{prefix}/lib/gimp/%{subver}/plug-ins/shift
%{prefix}/lib/gimp/%{subver}/plug-ins/smooth_palette
%{prefix}/lib/gimp/%{subver}/plug-ins/snoise
%{prefix}/lib/gimp/%{subver}/plug-ins/sobel
%{prefix}/lib/gimp/%{subver}/plug-ins/sparkle
%{prefix}/lib/gimp/%{subver}/plug-ins/spheredesigner
%{prefix}/lib/gimp/%{subver}/plug-ins/spread
%{prefix}/lib/gimp/%{subver}/plug-ins/sunras
%{prefix}/lib/gimp/%{subver}/plug-ins/tga
%{prefix}/lib/gimp/%{subver}/plug-ins/threshold_alpha
%{prefix}/lib/gimp/%{subver}/plug-ins/tiff
%{prefix}/lib/gimp/%{subver}/plug-ins/tile
%{prefix}/lib/gimp/%{subver}/plug-ins/tileit
%{prefix}/lib/gimp/%{subver}/plug-ins/tiler
%{prefix}/lib/gimp/%{subver}/plug-ins/url
%{prefix}/lib/gimp/%{subver}/plug-ins/video
%{prefix}/lib/gimp/%{subver}/plug-ins/vinvert
%{prefix}/lib/gimp/%{subver}/plug-ins/vpropagate
%{prefix}/lib/gimp/%{subver}/plug-ins/warp
%{prefix}/lib/gimp/%{subver}/plug-ins/waves
%{prefix}/lib/gimp/%{subver}/plug-ins/whirlpinch
%{prefix}/lib/gimp/%{subver}/plug-ins/wind
%{prefix}/lib/gimp/%{subver}/plug-ins/wmf
%{prefix}/lib/gimp/%{subver}/plug-ins/xbm
%{prefix}/lib/gimp/%{subver}/plug-ins/xpm
%{prefix}/lib/gimp/%{subver}/plug-ins/xwd
%{prefix}/lib/gimp/%{subver}/plug-ins/zealouscrop

%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_gtk.so
%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_triangle.so
%{prefix}/lib/gimp/%{subver}/modules/libcolorsel_water.so
%{prefix}/lib/gimp/%{subver}/modules/libcdisplay_gamma.so

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
%{prefix}/lib/libgpc.a

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
%{prefix}/man/man3/gpc.3.gz

