# Meson build file

# http://bugzilla.gnome.org/enter_bug.cgi?product=GIMP
project('gimp', ['c', 'cpp'], version: '2.9.9',
  meson_version: '>=0.44.0',
  default_options: [
    'sysconfdir=/etc',
    'localstatedir=/var',
    'sharedstatedir=/var/lib'
  ],
)

conf = configuration_data()

################################################################################
######
#     # #####   ####       # ######  ####  #####      # #    # ######  ####
#     # #    # #    #      # #      #    #   #        # ##   # #      #    #
######  #    # #    #      # #####  #        #        # # #  # #####  #    #
#       #####  #    #      # #      #        #        # #  # # #      #    #
#       #   #  #    # #    # #      #    #   #        # #   ## #      #    #
#       #    #  ####   ####  ######  ####    #        # #    # #       ####



prettyname = 'GIMP'
full_name  = 'GNU Image Manipulation Program'

# Making releases on the stable branch:
#   micro_version += 1
#   interface_age += 1

version  = meson.project_version().split('.')
major_version = version[0].to_int()
minor_version = version[1].to_int()
micro_version = version[2].to_int()
interface_age = 0

api_version   = '@0@.0'  .format(major_version)
app_version   = '@0@.@1@'.format(major_version, minor_version)
user_version  = '@0@.@1@'.format(major_version, minor_version)
gimp_version  = '@0@.@1@.@2@'.format(major_version, minor_version, micro_version)

gimp_command  = 'gimp-' + app_version
gimp_api_name = 'gimp-' + api_version
package_string  = prettyname + ' ' + gimp_version

gettext_package = 'gimp20'
conf.set_quoted('GETTEXT_PACKAGE', gettext_package)


# TODO cleanup all this versions mess
binary_age = 100 * minor_version + micro_version

lt_current = 100 * minor_version + micro_version - interface_age
lt_revision = interface_age
lt_age = binary_age - interface_age
lt_version_info = '@0@:@1@:@2@'.format(lt_current, lt_revision, lt_age)
lt_current_minus_age = lt_current - lt_age

so_version = '@0@.@1@.@2@'.format(lt_revision, lt_current, lt_current_minus_age)


stable = (minor_version % 2 == 0)
conf.set10('GIMP_UNSTABLE', not stable)

versionconfig = configuration_data()
versionconfig.set('GIMP_FULL_NAME',     full_name)
versionconfig.set('GIMP_MAJOR_VERSION', major_version)
versionconfig.set('GIMP_MINOR_VERSION', minor_version)
versionconfig.set('GIMP_MICRO_VERSION', micro_version)
versionconfig.set('GIMP_VERSION',       gimp_version)
versionconfig.set('GIMP_API_VERSION',   api_version)


################################################################################
# Get configuration and Meson modules

pkgconfig = import('pkgconfig')
i18n      = import('i18n')
gnome     = import('gnome')
pythonmod = import('python3')

cc        = meson.get_compiler('c')
cxx       = meson.get_compiler('cpp')
prefix    = get_option('prefix')
buildtype = get_option('buildtype')


################################################################################
# Host system detection

 #####
#     # #   #  ####  ##### ###### #    #
#        # #  #        #   #      ##  ##
 #####    #    ####    #   #####  # ## #
      #   #        #   #   #      #    #
#     #   #   #    #   #   #      #    #
 #####    #    ####    #   ###### #    #


cpu = host_machine.cpu().to_lower()

arch_x86_64 = (cpu == 'x86_64')
arch_x86    = (cpu.startswith('i') and cpu.endswith('86')) or arch_x86_64
arch_ppc64  = (cpu == 'ppc64' or cpu == 'powerpc64')
arch_ppc    = (cpu == 'ppc'   or cpu == 'powerpc') or arch_ppc64

if not (arch_x86 or arch_ppc)
  error('Unknown host architecture')
endif

conf.set10('ARCH_X86',    arch_x86)
conf.set10('ARCH_X86_64', arch_x86_64)
conf.set10('ARCH_PPC',    arch_ppc)
conf.set10('ARCH_PPC64',  arch_ppc64)


host_os = host_machine.system().to_lower()
message('Host os: ' + host_os)

platform_win32 = (
  host_os.contains('mingw') or
  host_os.contains('cygwin') or
  host_os.contains('windows')
)

platform_osx = (
  host_os.contains('machten') or
  host_os.contains('rhapsody') or
  host_os.contains('darwin')
)

conf.set('PLATFORM_OSX', platform_osx)

if platform_win32
  windows = import('windows')
  # AC_CHECK_PROG(ms_librarian, lib.exe, yes, no)
  # AM_CONDITIONAL(MS_LIB_AVAILABLE, test "x$ms_librarian" = xyes)
  # add_project_link_arguments(
  #   '-Wl,--large-address-aware',
  #   language: [ 'c', 'cpp' ]
  # )
endif
# dirs_separator = platform_win32 ? '\\' : '/'
# path_separator = platform_win32 ? ';'  : ':'


if cc.version() == '7.2.0'
  warning('''
    GCC 7.2.0 has a serious bug affecting GEGL/GIMP. We advise
    against using this version of the compiler (previous and
    further versions are fine).
    See https://bugzilla.gnome.org/show_bug.cgi?id=787222
  ''')
endif



################################################################################
# Compiler CPU extensions for optimizations

 #####  ######  #     #    #######
#     # #     # #     #    #     # #####  ##### # #    #  ####
#       #     # #     #    #     # #    #   #   # ##  ## #
#       ######  #     #    #     # #    #   #   # # ## #  ####
#       #       #     #    #     # #####    #   # #    #      #
#     # #       #     #    #     # #        #   # #    # #    #
 #####  #        #####     ####### #        #   # #    #  ####


if (get_option('buildtype') == 'release' or
    get_option('buildtype') == 'debugoptimized')

  # Check for compiler CPU extensions
  cpuext_args = [
    '-mfpmath=sse',
    '-mmmx',
    '-msse',
    '-msse2',
    '-msse4.1',
  ]
  add_project_arguments(cc .get_supported_arguments(cpuext_args), language: 'c')
  add_project_arguments(cxx.get_supported_arguments(cpuext_args), language: 'cpp')

  conf.set10('USE_MMX',                   cc.has_argument('-mmmx'))
  conf.set10('USE_SSE',                   cc.has_argument('-msse'))
  conf.set10('COMPILE_SSE2_INTRINISICS',  cc.has_argument('-msse2'))
  conf.set10('COMPILE_SSE4_1_INTRINISICS',cc.has_argument('-msse4.1'))


  have_altivec        = false
  have_altivec_sysctl = false
  if arch_ppc
    altivec_args = cc.get_supported_arguments([
      '-faltivec',
      '-maltivec',
      '-mabi=altivec',
    ])

    if altivec_args != []
      add_project_arguments     (altivec_args, language: [ 'c', 'cpp'])
      add_project_link_arguments(altivec_args, language: [ 'c', 'cpp'])

      if host_os.contains('darwin')
        have_altivec = true
        have_altivec_sysctl = true
      elif cc.compiles('''
        int main() { asm ("vand %v0, %v0, %v0"); return 0; }
        ''')
        have_altivec = true
      endif
    endif
  endif
  conf.set('HAVE_ALTIVEC_SYSCTL', have_altivec_sysctl)
  conf.set('USE_ALTIVEC',         have_altivec)

endif

################################################################################
# Mandatory Dependencies

######
#     # ###### #####  ###### #    # #####  ###### #    #  ####  # ######  ####
#     # #      #    # #      ##   # #    # #      ##   # #    # # #      #
#     # #####  #    # #####  # #  # #    # #####  # #  # #      # #####   ####
#     # #      #####  #      #  # # #    # #      #  # # #      # #           #
#     # #      #      #      #   ## #    # #      #   ## #    # # #      #    #
######  ###### #      ###### #    # #####  ###### #    #  ####  # ######  ####



math              = cc.find_library('m')

atk               = dependency('atk',                version: '>=2.2.0')

babl_minver       = '>=0.1.42'
babl              = dependency('babl',               version: babl_minver)

cairo_minver      = '>=1.12.2'
cairo             = dependency('cairo',              version: cairo_minver)

dbus_glib         = dependency('dbus-glib-1')

fontconfig_minver = platform_win32 ? '>=2.11.1'         : '>=2.2.0'
# fontconfig_name   = platform_win32 ? 'fontconfig_win32' : 'fontconfig'
fontconfig_name   = 'fontconfig'
fontconfig        = dependency(fontconfig_name,      version: fontconfig_minver)

freetype2         = dependency('freetype2',          version: '>=2.1.7')

gdk_pixbuf_minver = '>=2.30.8'
gdk_pixbuf        = dependency('gdk-pixbuf-2.0',     version: gdk_pixbuf_minver)

gegl_minver       = '>=0.3.29'
gegl              = dependency('gegl-0.3',           version: gegl_minver)

gexiv2            = dependency('gexiv2',             version: '>=0.10.6')

gio               = dependency('gio-2.0')
gio_specific_name = platform_win32 ? 'gio-windows-2.0' : 'gio-unix-2.0'
gio_specific      = dependency(gio_specific_name)

glib              = dependency('glib-2.0',           version: '>=2.54.2')
gobject           = dependency('gobject-2.0',        version: '>=2.54.2')
gmodule           = dependency('gmodule-no-export-2.0')

gtk2_minver       = '>=2.24.32'
gtk2              = dependency('gtk+-2.0',           version: gtk2_minver)

harfbuzz          = dependency('harfbuzz',           version: '>=0.9.19')
lcms              = dependency('lcms2',              version: '>=2.8')

libmypaint_name   = platform_win32 ? 'libmypaint' : 'libmypaint-1.3'
libmypaint        = dependency(libmypaint_name,      version: '>=1.3.0')
libmypaint_brushes= dependency('mypaint-brushes-1.0',required: false)
# Mypaint-brushes not available on MingW
# TODO should be removed when MingW adds them
mypaint_brushes = libmypaint_brushes.found() ? [
  libmypaint_brushes,
] : []
mypaint_args    = libmypaint_brushes.found() ? [] : [
  '-DMYPAINT_BRUSHES_DIR="/usr/share/mypaint-data/1.0/brushes"',
]

pangocairo        = dependency('pangocairo',         version: '>=1.29.4')
pangoft2          = dependency('pangoft2',           version: '>=1.29.4')
rsvg              = dependency('librsvg-2.0',        version: '>=2.40.6')



################################################################################
# Check for GLib Networking

glib_networking_works_run = cc.run(
  '''#include <gio/gio.h>
  int main() {
    return !g_tls_backend_supports_tls (g_tls_backend_get_default ());
  }''',
  dependencies: gio,
)
glib_networking_works =(glib_networking_works_run.compiled()
                    and glib_networking_works_run.returncode() == 0)

if not glib_networking_works
  if is_cross_build()
    warning('''
      Test for glib-networking cannot be performed while cross-compiling.
      Make sure glib-networking is installed, otherwise GIMP will not be able
      to display the remote help pages through the help browser, nor will it
      be able to open remote HTTPS (or other protocol using SSL/TLS) files.
      HTTPS is becoming the expected standard and should not be considered
      optional anymore.
    ''')
  else
    error('Test for glib-networking failed. This is required.')
  endif
endif

################################################################################
# Check if Pango is built with a recent fontconfig

pango_check_run = cc.run(
  '''#include <fontconfig/fontconfig.h>
  int main() {
    FcObjectSet *os; os = FcObjectSetBuild (FC_FAMILY, FC_WIDTH);
  }''',
  dependencies: fontconfig,
)
pango_check =(pango_check_run.compiled()
          and pango_check_run.returncode() == 0)

if not pango_check
  warning('\n  *** '.join([
  'You have a fontconfig >= fontconfig_required_version installed on your',
  'system, but your Pango library is using an older version. This old version',
  'is probably in /usr/X11R6. Look at the above output, and note that the',
  'result for FONTCONFIG_CFLAGS is not in the result for PANGOCAIRO_CFLAGS,',
  'and that there is likely an extra -I line, other than the ones for GLIB,',
  'Freetype, and Pango itself. That\'s where your old fontconfig files are.',
  'Rebuild pango, and make sure that it uses the newer fontconfig.',
  'The easiest way be sure of this is to simply get rid of the old',
  'fontconfig. When you rebuild pango, make sure the result for',
  'FONTCONFIG_CFLAGS is the same as the result here.',
  ]))
endif

################################################################################
# Optional Dependencies

libsocket         = cc.find_library('socket', required: false)
conf.set('HAVE_LIBSOCKET', libsocket.found())


################################################################################
# Check for Dr. Mingw

drmingw = dependency('', required: false)
if platform_win32
  exchndl = cc.find_library('exchndl')
  exchndl_fn = cc.has_function('ExcHndlSetLogFileNameA', dependencies: exchndl)
  if exchndl.found() and exchndl_fn
    drmingw = declare_dependency(dependencies: exchndl)
  endif
endif
conf.set('HAVE_EXCHNDL', drmingw.found())


################################################################################
# Check for x11 support

x11_target = gtk2.get_pkgconfig_variable('target') == 'x11'

x11   = x11_target ? dependency('x11')    : dependency('', required: false)
xmu   = x11_target ? dependency('xmu')    : dependency('', required: false)
xt    = x11_target ? dependency('xt')     : dependency('', required: false)
xext  = x11_target ? dependency('xext')   : dependency('', required: false)
xfixes= x11_target ? dependency('xfixes') : dependency('', required: false)
conf.set('HAVE_XFIXES', xfixes.found())

if x11_target
  foreach header : [ 'X11/Xmu/WinUtil.h', 'X11/extensions/shape.h', ]
    if not cc.has_header(header, dependencies: [ xext, xmu ])
      error('x11 install does not provide required header ' + header)
    endif
  endforeach

  foreach function : [ 'XmuClientWindow', 'XShapeGetRectangles', ]
    if not cc.has_function(function, dependencies: [ xext, xmu ])
      error('x11 install does not provide required function ' + function)
    endif
  endforeach
endif
conf.set('HAVE_X11_EXTENSIONS_SHAPE_H', cc.has_header('X11/extensions/shape.h'))
conf.set('HAVE_X11_XMU_WINUTIL_H',      cc.has_header('X11/Xmu/WinUtil.h'))


# Features requiring x11

have_doc_shooter= x11_target
have_print      = get_option('with-print')
have_screenshot = get_option('with-screenshot')
if have_print and not x11_target
  error('Printing requires x libs')
endif
if have_screenshot and not x11_target
  error('Screenshot requires x libs')
endif


################################################################################
# The list of MIME types that are supported by plug-ins

MIMEtypes = [
  'image/bmp',
  'image/g3fax',
  'image/gif',
  'image/svg+xml',
  'image/x-compressed-xcf',
  'image/x-fits',
  'image/x-gimp-gbr',
  'image/x-gimp-gih',
  'image/x-gimp-pat',
  'image/x-pcx',
  'image/x-portable-anymap',
  'image/x-portable-bitmap',
  'image/x-portable-graymap',
  'image/x-portable-pixmap',
  'image/x-psd',
  'image/x-sgi',
  'image/x-tga',
  'image/x-xbitmap',
  'image/x-xcf',
  'image/x-xwindowdump',
]



libtiff = dependency('libtiff-4', required: false)
if libtiff.found()
  MIMEtypes += 'image/tiff'
endif


libjpeg = dependency('libjpeg', required: false)
if libjpeg.found()
  MIMEtypes += 'image/jpeg'
endif
conf.set('HAVE_LIBJPEG', libjpeg.found())


zlib = dependency('zlib', required: false)
if zlib.found()
  MIMEtypes += 'image/x-psp'
endif


bz2 = cc.find_library('bz2', required: false)


liblzma = dependency('liblzma', version: '>=5.0.0', required: false)


if get_option('with-ghostscript')
  ghostscript = cc.find_library('gs')
  MIMEtypes += 'application/postscript'
else
  ghostscript = dependency('', required: false)
endif


libpng = dependency('libpng', version: '>=1.6.25', required: false)
if libpng.found()
  MIMEtypes += [ 'image/png', 'image/x-icon']
endif


if get_option('with-mng')
  libmng = cc.find_library('mng')
else
  libmng = dependency('', required: false)
endif


if get_option('with-aa')
  libaa = cc.find_library('aa')
else
  libaa = dependency('', required: false)
endif


if get_option('with-xpm')
  libxpm = dependency('xpm')
  MIMEtypes += 'image/x-xpixmap'
else
  libxpm = dependency('', required: false)
endif


if get_option('with-openexr')
  openexr = dependency('OpenEXR', version: '>=1.6.1')
  MIMEtypes += 'image/x-exr'
else
  openexr = dependency('', required: false)
endif


if get_option('with-webp')
  webp_deps = [
    dependency('libwebp',     version: '>=0.6.0'),
    dependency('libwebpmux',  version: '>=0.6.0'),
    dependency('libwebpdemux',version: '>=0.6.0'),
  ]
  webp = declare_dependency(
    dependencies: webp_deps,
    version: webp_deps[0].version(),
  )
  MIMEtypes += 'image/x-webp'
else
  webp = dependency('', required: false)
endif


if get_option('with-webkit')
  webkit = dependency('webkit-1.0', version: '>=1.6.1')
else
  webkit = dependency('', required: false)
endif
conf.set('HAVE_WEBKIT', webkit.found())


if get_option('with-poppler')
  # All modern distributions have at least this version and CJK/Cyrillic
  # support should not be considered as "second-zone citizen".
  poppler = [
    dependency('poppler-glib', version: '>=0.44.0'),
    dependency('poppler-data', version: '>=0.4.7'),
  ]
else
  poppler = dependency('', required: false)
endif


if get_option('with-cairo-pdf')
  cairopdf = dependency('cairo-pdf', version: '>=1.12.2')
else
  cairopdf = dependency('', required: false)
endif


# PDF import support is a granted feature.
MIMEtypes += 'application/pdf'


if get_option('with-wmf')
  wmf = dependency('libwmf', version: '>=0.2.8')
  MIMEtypes += [ 'image/x-wmf', ]
else
  wmf = dependency('', required: false)
endif


if get_option('with-jasper')
  jasper = cc.find_library('jasper')
  MIMEtypes += [ 'image/jp2', 'image/jpeg2000', 'image/jpx', ]
else
  jasper =
endif


if get_option('with-xcursor')
  xmc = dependency('xcursor')
  MIMEtypes += [ 'image/x-xcursor', ]
else
  xmc = dependency('', required: false)
endif


if get_option('with-alsa')
  alsa = dependency('alsa', version: '>=1.0.0')
else
  alsa = dependency('', required: false)
endif
conf.set('HAVE_ALSA', alsa.found())


have_linuxinput = get_option('with-linux-input')
if have_linuxinput and not cc.has_header('linux/input.h')
  error('linux/input.h header not found.')
endif

# DirectX DirectInput
directx_sdk_path = get_option('with-directx-sdk')
if directx_sdk_path != '' and platform_win32
  if directx_sdk_path.contains(' ')
    warning('''
    The DirectX SDK should be accessible through a path without spaces.
    Use MSYS mounts.
    Ignoring your with-directx-sdk switch.
    ''')
  elif directx_sdk_path.contains('\\')
    warning('''
    Use plain (forward) slashes only for the DirectX SDK path.
    Ignoring your with-directx-sdk switch.
    ''')
  else
    directx = declare_dependency(
      link_with: cc.find_library('dxguid',
                    dirs:  join_paths(directx_sdk_path, 'Lib', 'x86')),
      include_directories: join_paths(directx_sdk_path, 'Include'),
    )
  endif
else
  directx = dependency('', required: false)
endif
conf.set('HAVE_DX_DINPUT', directx.found())


if get_option('with-gudev')
  gudev = dependency('gudev-1.0', version: '>=167')
else
  gudev = dependency('', required: false)
endif
conf.set('HAVE_LIBGUDEV', gudev.found())


################################################################################
# Email sending
email_message = false

sendmail_choice = get_option('with-sendmail')
if not [ '', 'false', 'no', ].contains(sendmail_choice)
  sendmail_path = (
    [ 'true', 'yes' ].contains(sendmail_choice) ? 'sendmail' : sendmail_choice
  )
  sendmail = find_program(sendmail_path, required: false)
  if sendmail.found()
    sendmail_path = sendmail.path()
  else
    warning('''
      Sendmail specified but not found. It should be installed at runtime!
    ''')
  endif

  email_message = '@0@ (@1@)'.format(true, sendmail_path)
  conf.set_quoted('SENDMAIL', sendmail_path)
else
  xdg_email_path = 'xdg-email'
  xdg_email = find_program(xdg_email_path, required: false)
  if xdg_email.found()
    xdg_email_path = xdg_email.path()
  else
    warning('''
      Sendmail specified but not found. It should be installed at runtime!
    ''')
  endif

  email_message = '@0@ (@1@)'.format(true, xdg_email_path)
endif


################################################################################
# ISO codes

isocodes = dependency('iso-codes')
isocodes_prefix = isocodes.get_pkgconfig_variable('prefix')
isocodes_location = join_paths(
  isocodes_prefix, get_option('datadir'), 'xml', 'iso-codes'
)
isocodes_localedir= join_paths(
  isocodes_prefix, get_option('datadir'), 'locale'
)
conf.set('HAVE_ISO_CODES', isocodes.found())


################################################################################
# Program tools

#######
   #     ####   ####  #       ####
   #    #    # #    # #      #
   #    #    # #    # #       ####
   #    #    # #    # #           #
   #    #    # #    # #      #    #
   #     ####   ####  ######  ####



perl = find_program('perl5', 'perl', 'perl5.005', 'perl5.004', 'perl')


# python
have_python = get_option('with-python')
if have_python
  python2 = find_program('python2')

  libpython2= dependency('python2',       version: '>=2.5.0')
  pygobject = dependency('pygobject-2.0', version: '>=2.10.4')
  pygtk2    = dependency('pygtk-2.0',     version: '>=2.10.4')
  pycairo   = dependency('pycairo',       version: '>=1.0.2')

  pygobjectcodegen = find_program('pygobject-codegen-2.0')
  pygtk_defsdir = pygtk2.get_pkgconfig_variable('defsdir')
endif


# Script-fu
have_scriptfu = get_option('with-script-fu')


# Check for GTK Mac Integration
if platform_osx and (gtk2.get_pkgconfig_variable('target') == 'xquartz')
  gtk2_macos = dependency('gtk-mac-integration-gtk2', version: '>=2.0.0',
    required: false
  )
  if not gtk2_macos.found()
    gtk2_macos = dependency('gtk-mac-integration', version: '>=2.0.0')
  endif
else
  gtk2_macos = dependency('', required: false)
endif

# Check for XML tools
xmllint             = find_program('xmllint', required: false)
xsltproc            = find_program('xsltproc',required: false)
intltool_merge      = find_program('intltool-merge')
desktop_validate    = find_program('desktop-file-validate')
gdk_pixbuf_csource  = find_program('gdk-pixbuf-csource')

if get_option('with-appdata-test')
  appstream_util = find_program('appstream-util')
else
  appstream_util = dependency('', required: false)
endif


# Check for vector icons
have_vector_icons = get_option('with-vec-icons')
if have_vector_icons
  # shared-mime-info is needed to correctly detect SVG files
  # (except on Windows, apparently).
  if platform_win32
    warning('''
      You enabled vector icons on Win32. Make sure to run:
      $ gdk-pixbuf-query-loaders.exe --update-cache
      on the target machine (this command generates loaders.cache)
      so that GdkPixbuf knows where to find the SVG loader.
    ''')
  else
    shared_mime_info = dependency('shared-mime-info')
  endif
endif

if get_option('with-xvfb-run')
  xvfb_run = find_program('xvfb-run')
else
  xvfb_run = dependency('', required: false)
endif
conf.set('HAVE_XVFB_RUN', xvfb_run.found())


if get_option('with-gtk-doc')
  gtkdoc_scan = find_program('gtkdoc-scan', required : true)
else
  gtkdoc_scan = dependency('', required: false)
endif

enable_default_bin = get_option('enable-default-bin')
enable_console_bin = get_option('enable-console-bin')


# Possibly change default gimpdir from $XDG_CONFIG_HOME/GIMP/gimp_user_version
gimpdir = get_option('gimpdir')
if gimpdir == ''
  # Default value
  gimpdir = meson.project_name().to_upper()
endif

project_subdir = join_paths(meson.project_name(), api_version)
gimpdatadir    = join_paths(get_option('datadir'),    project_subdir)
gimpplugindir  = join_paths(get_option('libdir'),     project_subdir)
gimpsysconfdir = join_paths(get_option('sysconfdir'), project_subdir)
gimpmanpagedir = gimpdir
localedir      = join_paths(get_option('datadir'), 'locale')



# Disable deprecated APIs
add_project_arguments([
  '-DGIMP_DISABLE_DEPRECATED',
  '-DBABL_DISABLE_DEPRECATED',
  '-DGSEAL_ENABLE'
  ],
  language: 'c',
)

if glib.version().version_compare('>=2.43')
  add_project_arguments([
    '-DG_DISABLE_DEPRECATED',
    ],
    language: 'c',
  )
endif
if gtk2.version().version_compare('>=2.26')
  add_project_arguments([
    '-DGDK_DISABLE_DEPRECATED',
    '-DGTK_DISABLE_DEPRECATED',
    ],
    language: 'c',
  )
endif
if glib.version().version_compare('>=1.32')
  add_project_arguments([
    '-DPANGO_DISABLE_DEPRECATED',
    ],
    language: 'c',
  )
endif


# Require multihead safe API
add_project_arguments([
  '-DGDK_MULTIHEAD_SAFE',
  '-DGTK_MULTIHEAD_SAFE',
  ],
  language: 'c',
)


# Check for internal tools
defcheck            = find_program(join_paths('tools','defcheck.py'))
extract_vector_icon = find_program(join_paths('tools','extract-vector-icon.sh'))
generate_changelog  = find_program(join_paths('tools','generate_changelog.sh'))
generate_news       = find_program(join_paths('tools','generate-news'))
gimppath2svg        = find_program(join_paths('tools','gimppath2svg.py'))
module_dependencies = find_program(join_paths('tools','module-dependencies.py'))

gimp_mkenums        = find_program(join_paths('tools', 'gimp-mkenums'))
gimp_mkenums_custom_target_commonargs = [
  '--fprod','/* enumerations from "@basename@" */\n',
  '--vhead','GType\n'+
            '@enum_name@_get_type (void)\n'+
            '{\n'+
            '  static const G@Type@Value values[] =\n'+
            '  {',
  '--vprod','    { @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
  '--vtail','    { 0, NULL, NULL }\n'+
            '  };\n',
  '--dhead','  static const Gimp@Type@Desc descs[] =\n'+
            '  {',
  '--dprod','    {\n'+
            '      @VALUENAME@, @valuedesc@, @valuehelp@\n'+
            '    },\n'+
            '    @if (\'@valueabbrev@\' ne \'NULL\')@\n'+
            '    /* Translators:\n'+
            '       this is an abbreviated version of @valueudesc@.\n'+
            '       Keep it short. */\n'+
            '    { @VALUENAME@, @valueabbrev@, NULL },\n'+
            '    @endif@',
  '--dtail','    { 0, NULL, NULL }\n'+
            '  };\n'+
            '\n'+
            '  static GType type = 0;\n'+
            '\n'+
            '  if (G_UNLIKELY (! type))\n'+
            '    {\n'+
            '      type = g_@type@_register_static ("@EnumName@", values);\n'+
            '      gimp_type_set_translation_context (type, "@enumnick@");\n'+
            '      gimp_@type@_set_value_descriptions (type, descs);\n'+
            '    }\n'+
            '\n'+
            '  return type;\n'+
            '}\n',
  '@INPUT@',
]


conf.set('ENABLE_NLS',   true)
conf.set('HAVE_GETTEXT', true)

# localedir = join_paths(get_option('prefix'), get_option('localedir'))




################################################################################
# CFlags

 #####  #######
#     # #       #        ##    ####   ####
#       #       #       #  #  #    # #
#       #####   #      #    # #       ####
#       #       #      ###### #  ###      #
#     # #       #      #    # #    # #    #
 #####  #       ###### #    #  ####   ####



if get_option('profiling') and cc.get_id() == 'gcc'
  add_project_arguments     ('-pg', language: [ 'c', 'cpp', ])
  add_project_link_arguments('-pg', language: [ 'c', 'cpp', ])
endif

if get_option('ansi')
  add_project_arguments('-ansi', '-pedantic', language: [ 'c', 'cpp', ])
endif

warning_cflags = [
  '-fdiagnostics-show-option',
  '-fno-common',

  '-Wdeclaration-after-statement',
  '-Wformat-security',
  '-Winit-self',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wold-style-definition',
  '-Wpointer-arith',
  '-Wreturn-type',
  '-Wtype-limits',

  # You can uncomment this to debug with less warning outputs
  # '-Wno-deprecated-declarations',

  '-Werror=implicit-function-declaration',
  '-Werror=strict-prototypes',
]
add_project_arguments(cc .get_supported_arguments(warning_cflags), language: 'c')
add_project_arguments(cxx.get_supported_arguments(warning_cflags), language: 'cpp')

# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc.
if platform_win32 and cc.get_id() == 'gcc'
  msvc_compat_args = cc.first_supported_argument([
    '-fnative-struct',
    '-mms-bitfielads',
  ])
  if msvc_compat_args == []
    warning('''
      GCC does not support '-fnative-struct' nor '-mms-bitfields'.
      Build will be incompatible with GTK+ DLLs.
    ''')
  endif
  add_project_arguments(msvc_compat_args, language: [ 'c', 'cpp' ])
endif


conf.set('HAVE__NL_MEASUREMENT_MEASUREMENT',
  cc.compiles('''
    #include<langinfo.h>
    int main() {
      char c = *((unsigned char *)  nl_langinfo(_NL_MEASUREMENT_MEASUREMENT));
    }
  ''')
)



################################################################################
# Miscelaneous configuration

#     #
##   ## #  ####   ####
# # # # # #      #    #
#  #  # #  ####  #
#     # #      # #
#     # # #    # #    #
#     # #  ####   ####



# Enable support for multiprocessing
conf.set10('ENABLE_MP', get_option('enable-multiproc'))

# Check for available functions
foreach fn : [
    'alloca',
    'backtrace',
    'bind_textdomain_codeset',
    'dcgettext',
    'difftime',
    'finite',
    'finite',
    'fsync',
    'getaddrinfo',
    'getnameinfo',
    'gettext',
    'isfinite',
    'mmap',
    'rint',
    'vprintf',
  ]
  macro_name = 'HAVE_' + fn.to_upper().underscorify()
  conf.set(macro_name, cc.has_function(fn) ? 1 : false)
endforeach


# Check for available headers
foreach header : [
    'alloca.h',
    'dlfcn.h',
    'execinfo.h',
    'ieeefp.h',
    'inttypes.h',
    'locale.h',
    'math.h',
    'memory.h',
    # 'sys/mman.h',
    # 'sys/ipc.h',
    # 'sys/shm.h',
    'stdint.h',
    'stdlib.h',
    'string.h',
    'strings.h',
    'sys/param.h',
    'sys/select.h',
    'sys/stat.h',
    'sys/time.h',
    'sys/times.h',
    'sys/types.h',
    'sys/wait.h',
    'unistd.h',
  ]
  macro_name = 'HAVE_' + header.to_upper().underscorify()
  conf.set(macro_name, cc.has_header(header) ? 1 : false)
endforeach

conf.set('HAVE_MMAN_H', cc.has_header('sys/mman.h'))
conf.set('HAVE_IPC_H',  cc.has_header('sys/ipc.h'))
conf.set('HAVE_SHM_H',  cc.has_header('sys/shm.h'))


################################################################################
# Check for shared memory handling

shmem_choice = get_option('shmem-type')
if shmem_choice == 'auto'
  shmem_choice = 'sysv'

  # MacOS X has broken SysV shm
  if platform_osx
    shmem_choice = 'posix'
  endif
  if platform_win32
    shmem_choice = 'win32'
  endif
endif

if shmem_choice == 'sysv'
  check_ip_rmid_deferred_release = cc.run('''
  #include <sys/types.h>
  #include <sys/ipc.h>
  #include <sys/shm.h>
  int main() {
    int id = shmget(IPC_PRIVATE, 4, IPC_CREAT | 0600);
    if (id == -1)
      exit(2);

    char *shmaddr = shmat(id, 0, 0);
    shmctl(id, IPC_RMID, 0);
    if ((char*) shmat(id, 0, 0) == (char*) -1) {
      shmdt(shmaddr);
      exit(1);
    }
    shmdt(shmaddr);
    shmdt(shmaddr);
    exit(0);
  }
  ''').returncode() == 0
  conf.set('IPC_RMID_DEFERRED_RELEASE', check_ip_rmid_deferred_release)
  conf.set('USE_SYSV_SHM', true)
elif shmem_choice == 'posix'
  conf.set('USE_POSIX_SHM', true)
endif


conf.set('NO_FD_SET',
  not platform_win32
  and not cc.compiles('''
    #include <sys/types.h>
    int main() { fd_set readMask, writeMask; return 0; }
  ''')
)



# GCC attributes
conf.set('HAVE_FUNC_ATTRIBUTE_DESTRUCTOR',
  cc.compiles('''__attribute__ ((destructor)) void destructor_fn(void) { }''')
)




################################################################################
# Set/regroup common CFlags for subdirs

######
#     # ###### ###### # #    # ######  ####
#     # #      #      # ##   # #      #
#     # #####  #####  # # #  # #####   ####
#     # #      #      # #  # # #           #
#     # #      #      # #   ## #      #    #
######  ###### #      # #    # ######  ####

# Global arguments
# Should be moved into config.h someday

config_defines = [
  # Compiler
  '-DCC='                   +'"'+ cc.get_id()           +'"',
  '-DCC_VERSION='           +'"'+ cc.version()          +'"',

  # Names
  '-DGIMP_PACKAGE='         +'"'+ meson.project_name()  +'"',
  '-DPACKAGE_NAME='         +'"'+ meson.project_name()  +'"',
  '-DPACKAGE_STRING='       +'"'+ package_string        +'"',
  '-DGIMP_COMMAND='         +'"'+ gimp_command          +'"',

  # Versions
  '-DGIMP_APP_VERSION='     +'"'+ app_version           +'"',
  '-DGIMP_DATA_VERSION='    +'"'+ api_version           +'"',
  '-DGIMP_PLUGIN_VERSION='  +'"'+ api_version           +'"',
  '-DGIMP_TOOL_VERSION='    +'"'+ api_version           +'"',

  '-DGIMP_SYSCONF_VERSION=' +'"'+ api_version           +'"',
  '-DGIMP_USER_VERSION='    +'"'+ user_version          +'"',
  '-DGIMP_APP_VERSION_STRING="' + app_version           +'"',

  # Directories
  '-DPREFIX='         +'"'+ prefix                +'"',
  '-DEXEC_PREFIX='    +'"'+ prefix                +'"',
  '-DGIMPDIR='        +'"'+ gimpdir               +'"',
  '-DSYSCONFDIR='     +'"'+ gimpsysconfdir        +'"',
  '-DDATADIR='        +'"'+ join_paths(prefix, gimpdatadir)    +'"',
  '-DGIMPDATADIR='    +'"'+ join_paths(prefix, gimpdatadir)    +'"',
  '-DGIMPPLUGINDIR='  +'"'+ join_paths(prefix, gimpplugindir)  +'"',
  '-DPLUGINDIR='      +'"'+ join_paths(prefix, gimpplugindir)  +'"',
  '-DLOCALEDIR='      +'"'+ join_paths(prefix, localedir)      +'"',

  '-DLOCALSTATEDIR='  +'"'+ get_option('localstatedir')        +'"',
  '-DSHAREDSTATEDIR=' +'"'+ get_option('sharedstatedir')       +'"', # /usr/com?
  '-DSYSCONFDIR='     +'"'+ get_option('sysconfdir')           +'"',
  '-DBINDIR='         +'"'+ join_paths(prefix, get_option('bindir'))       +'"',
  '-DDATAROOTDIR='    +'"'+ join_paths(prefix, get_option('datadir'))      +'"',
  '-DINFODIR='        +'"'+ join_paths(prefix, get_option('infodir'))      +'"',
  '-DLIBDIR='         +'"'+ join_paths(prefix, get_option('libdir'))       +'"',
  '-DLIBEXECDIR='     +'"'+ join_paths(prefix, get_option('libexecdir'))   +'"',
  '-DMANDIR='         +'"'+ join_paths(prefix, get_option('mandir'))       +'"',
  '-DSBINDIR='        +'"'+ join_paths(prefix, get_option('sbindir'))      +'"',
  '-DSYSDATADIR='     +'"'+ join_paths(prefix, get_option('datadir'))      +'"',

  # Third-party/Misc
  '-DISO_CODES_LOCATION='   +'"'+ isocodes_location  +'"',
  '-DISO_CODES_LOCALEDIR='  +'"'+ isocodes_localedir +'"',

  '-DHAVE_CONFIG_H',
]

add_project_arguments(
  config_defines,
  language: [ 'c', 'cpp', ],
)

if platform_osx
  # libgimp_cflags += '-xobjective-c'
  # libgimp_lflags += ['-framework', 'Cocoa']
endif


rootInclude = include_directories('.')
appInclude  = include_directories('app')


################################################################################
# Generate files

  #####                                   #######
 #     #  ####  #    # ###### #  ####     #       # #      ######  ####
 #       #    # ##   # #      # #    #    #       # #      #      #
 #       #    # # #  # #####  # #         #####   # #      #####   ####
 #       #    # #  # # #      # #  ###    #       # #      #           #
 #     # #    # #   ## #      # #    #    #       # #      #      #    #
  #####   ####  #    # #      #  ####     #       # ###### ######  ####



gitversion_h1 = vcs_tag(
  input : 'app/git-version.h.in',
  output: 'git-version.h.in.1',
  command: [ 'git', 'describe', '--always', ],
  replace_string: '@GIMP_GIT_VERSION@',
  fallback: '',
)
gitversion_h2 = vcs_tag(
  input : gitversion_h1,
  output: 'git-version.h.in.2',
  command: [ 'git', 'rev-parse', '--short', 'HEAD', ],
  replace_string: '@GIMP_GIT_VERSION_ABBREV@',
  fallback: '',
)
gitversion_h = vcs_tag(
  input : gitversion_h2,
  output: 'git-version.h',
  command: [ 'git', 'log', '-n1', '--date=format:%Y', '--pretty=%cd', ],
  replace_string: '@GIMP_GIT_LAST_COMMIT_YEAR@',
  fallback: '',
)


install_conf = configuration_data()
install_conf.set('GIMP_APP_VERSION', app_version)
install_conf.set('GIMP_PKGCONFIG_VERSION', gimp_version)
install_conf.set('GIMP_VERSION', gimp_version)
install_conf.set('ATK_REQUIRED_VERSION',          atk         .version())
install_conf.set('BABL_REQUIRED_VERSION',         babl        .version())
install_conf.set('CAIRO_PDF_REQUIRED_VERSION',    cairopdf    .version())
install_conf.set('CAIRO_REQUIRED_VERSION',        cairo       .version())
install_conf.set('FONTCONFIG_REQUIRED_VERSION',   fontconfig  .version())
install_conf.set('FONTCONFIG_WIN32_RECOMMENDED_VERSION', fontconfig.version())
install_conf.set('FREETYPE2_REQUIRED_VERSION',    freetype2   .version())
install_conf.set('GDK_PIXBUF_REQUIRED_VERSION',   gdk_pixbuf  .version())
install_conf.set('GEGL_REQUIRED_VERSION',         gegl        .version())
install_conf.set('GLIB_REQUIRED_VERSION',         glib        .version())
install_conf.set('GTK_REQUIRED_VERSION',          gtk2        .version())
install_conf.set('HARFBUZZ_REQUIRED_VERSION',     harfbuzz    .version())
install_conf.set('INTLTOOL_REQUIRED_VERSION',     '0.40.1')
install_conf.set('LCMS_REQUIRED_VERSION',         lcms        .version())
install_conf.set('LIBLZMA_REQUIRED_VERSION',      liblzma     .version())
install_conf.set('LIBMYPAINT_REQUIRED_VERSION',   libmypaint  .version())
install_conf.set('LIBPNG_REQUIRED_VERSION',       libpng      .version())
install_conf.set('OPENEXR_REQUIRED_VERSION',      openexr     .version())
install_conf.set('PANGOCAIRO_REQUIRED_VERSION',   pangocairo  .version())
install_conf.set('POPPLER_DATA_REQUIRED_VERSION', poppler[1]  .version())
install_conf.set('POPPLER_REQUIRED_VERSION',      poppler[0]  .version())
install_conf.set('PYTHON2_REQUIRED_VERSION',      '2.5.0')
install_conf.set('RSVG_REQUIRED_VERSION',         rsvg        .version())
install_conf.set('WEBKIT_REQUIRED_VERSION',       webkit      .version())
install_conf.set('WEBP_REQUIRED_VERSION',         webp        .version())
install_conf.set('WMF_REQUIRED_VERSION',          wmf         .version())
install_conf.set('XGETTEXT_RECOMMENDED_VERSION', '0.19')

configure_file(
  input : 'INSTALL.in',
  output: 'INSTALL',
  configuration: install_conf
)


configure_file(
  output: 'config.h',
  configuration: conf
)


pkgconfig.generate(filebase: 'gimp-' + api_version,
  name: prettyname,
  description: 'GIMP Library',
  version: gimp_version,
  requires: [
    'gdk-pixbuf-2.0 ' + gdk_pixbuf_minver,
    'cairo '          + cairo_minver,
    'gegl-0.3 '       + gegl_minver,
  ],
  libraries: [
    '-L${libdir}',
    '-lgimp-'       + api_version,
    '-lgimpmath-'   + api_version,
    '-lgimpconfig-' + api_version,
    '-lgimpcolor-'  + api_version,
    '-lgimpbase-'   + api_version,
  ],
  subdirs: [
    gimp_api_name,
  ],
  variables: [
    'datarootdir='    +'${prefix}/'+ get_option('datadir'),
    'gimpdatadir='    +'${prefix}/'+ gimpdatadir,
    'gimplibdir='     +'${prefix}/'+ gimpplugindir,
    'gimpsysconfdir=' + gimpsysconfdir,
    'gimplocaledir='  +'${prefix}/'+ localedir,
  ],
)

pkgconfig.generate(filebase: 'gimpthumb-' + api_version,
  name: 'GIMP Thumb',
  description: 'GIMP Thumbnail Library',
  version: gimp_version,
  requires: [
    'gdk-pixbuf-2.0 ' + gdk_pixbuf_minver,
  ],
  libraries: [
    '-L${libdir}',
    '-lgimpthumb-'  + api_version,
  ],
  subdirs: [
    gimp_api_name,
  ],
)

pkgconfig.generate(filebase: 'gimpui-' + api_version,
  name: 'GIMP UI',
  description: 'GIMP User Interface Library',
  version: gimp_version,
  requires: [
    'gimp-' + api_version + ' >= ' + app_version,
    'gtk+-2.0 ' + gtk2_minver,
  ],
  libraries: [
    '-L${libdir}',
    '-lgimpui-'  + api_version,
    '-lgimpwidgets-' + api_version,
    '-lgimpmodule-'  + api_version,
  ],
  subdirs: [
    gimp_api_name,
  ],
)


################################################################################
# Miscelaneous targets


 #     #                    #######
 ##   ## #  ####   ####        #      ##   #####   ####  ###### #####  ####
 # # # # # #      #    #       #     #  #  #    # #    # #        #   #
 #  #  # #  ####  #            #    #    # #    # #      #####    #    ####
 #     # #      # #            #    ###### #####  #  ### #        #        #
 #     # # #    # #    #       #    #    # #   #  #    # #        #   #    #
 #     # #  ####   ####        #    #    # #    #  ####  ######   #    ####



if xsltproc.found()
  custom_target('AUTHORS',
    input : [ 'authors.xsl', 'authors.xml', ],
    output: 'AUTHORS',
    command: [
      xsltproc,
      '-o', '@OUTPUT@',
      '@INPUT@',
    ],
    build_by_default: false,
  )
  custom_target('authors.md',
    input : [ 'authors4gimp-web.xsl', 'authors.xml', ],
    output: 'authors.md',
    command: [
      xsltproc,
      '--stringparam', 'today', '`date --iso-8601=seconds`',
      '-o', '@OUTPUT@',
      '@INPUT@',
    ],
    build_by_default: false,
  )
endif

if xmllint.found()
  custom_target('validate-authors',
    input : [ 'authors.xml', ],
    output: [ 'validate-authors', ],
    command: [
      xmllint,
      '--noout',
      '--valid', '@INPUT@',
    ],
    build_by_default: false,
  )
endif

if python2.found()
  # TODO this python script is waiting for autotools directory/files structure
  custom_target('check-defs',
    input : [ ],
    output: [ 'check-defs', ],
    command: [
      python2,
      join_paths(meson.source_root(), 'tools','defcheck.py'),
      meson.source_root(),
    ],
    build_by_default: false,
  )
endif

custom_target('Changelog',
  input : [ ],
  output: [ 'Changelog', ],
  command: [
    generate_changelog,
    meson.source_root(),
    '@OUTPUT@'
  ],
  build_by_default: false,
)


################################################################################
# Subdirs

  #####
 #     # #    # #####  #####  # #####   ####
 #       #    # #    # #    # # #    # #
  #####  #    # #####  #    # # #    #  ####
       # #    # #    # #    # # #####       #
 #     # #    # #    # #    # # #   #  #    #
  #####   ####  #####  #####  # #    #  ####

if platform_win32
  subdir('build/windows')

  if get_option('windows-installer')
    subdir('po-windows-installer')
    subdir('build/windows/installer')
  endif
endif

# Tools
subdir('libgimpbase')
subdir('tools')

# Translations
subdir('po')
subdir('po-libgimp')
subdir('po-plug-ins')
subdir('po-python')
subdir('po-script-fu')
subdir('po-tags')
subdir('po-tips')
# Data / Desktop / xml files
subdir('cursors')
subdir('data')
subdir('desktop')
subdir('etc')
subdir('icons')
subdir('m4macros')
subdir('menus')
subdir('themes')

# Libraries (order here is important!)
subdir('libgimpcolor')
subdir('libgimpmath')
subdir('libgimpconfig')
subdir('libgimpmodule')
subdir('libgimpthumb')
subdir('libgimpwidgets')
subdir('libgimp')

# Executables, plugins
subdir('modules')
subdir('plug-ins')
subdir('app')
subdir('pdb')

# Docs
subdir('devel-docs')
subdir('docs')


################################################################################

message('''
Extra Binaries:
  gimp-console:        @0@'''.format(enable_console_bin) +'''

Optional Features:
  Language selection:  @0@'''.format(isocodes.found()) +'''
  Vector icons:        @0@'''.format(have_vector_icons) +'''
  Dr. Mingw (Win32):   @0@'''.format(drmingw.found()) +'''

Optional Plug-Ins:
  Ascii Art:           @0@'''.format(libaa.found()) +'''
  Ghostscript:         @0@'''.format(ghostscript.found()) +'''
  Help Browser:        @0@'''.format(webkit.found()) +'''
  JPEG 2000:           @0@'''.format(jasper.found()) +'''
  MNG:                 @0@'''.format(libmng.found()) +'''
  OpenEXR:             @0@'''.format(openexr.found()) +'''
  WebP:                @0@'''.format(webp.found()) +'''
  PDF (export):        @0@'''.format(cairopdf.found()) +'''
  Print:               @0@'''.format(have_print) +'''
  Python 2:            @0@'''.format(have_python) +'''
  Script-Fu:           @0@'''.format(have_scriptfu) +'''
  TWAIN (Win32):       @0@'''.format(platform_win32) +'''
  Webpage:             @0@'''.format(webkit.found()) +'''
  WMF:                 @0@'''.format(wmf.found()) +'''
  X11 Mouse Cursor:    @0@'''.format(xmc.found()) +'''
  XPM:                 @0@'''.format(libxpm.found()) +'''
  Email:               @0@'''.format(email_message) +'''

Optional Modules:
  ALSA (MIDI Input):   @0@'''.format(alsa.found()) +'''
  Linux Input:         @0@ (GUdev support: @0@)'''
                        .format(have_linuxinput, gudev.found()) +'''
  DirectInput (Win32): @0@'''.format(directx.found()) +'''

Tests:
  Use xvfb-run         @0@'''.format(xvfb_run.found()) +'''
  Test appdata         @0@'''.format(appstream_util.found()) +'''
'''
)
