project('gimp',
  'c', 'cpp',
  version: '2.99.1',
  meson_version: '>=0.50.0',
  default_options: [
    'cpp_std=gnu++14',
  ],
)

project_url = 'https://gitlab.gnome.org/GNOME/gimp'
project_url_issues = project_url + '/issues/new'

conf = configuration_data()

################################################################################
# Project info

prettyname = 'GIMP'
full_name  = 'GNU Image Manipulation Program'

# General version
gimp_version = meson.project_version()

package_string= prettyname + ' ' + gimp_version

gimp_app_version_arr = gimp_version.split('.')
gimp_app_version_major = gimp_app_version_arr[0].to_int()
gimp_app_version_minor = gimp_app_version_arr[1].to_int()
gimp_app_version_micro = gimp_app_version_arr[2].to_int()

# Override for Release-candidates
gimp_app_version = '@0@.@1@'.format(
  gimp_app_version_major,
  gimp_app_version_minor
)


# API & pkg-config version
api_version_major = gimp_app_version_major
api_version_minor = gimp_app_version_minor

if api_version_minor == 99
  api_version_major += 1
  api_version_minor = 0
endif

gimp_api_version = '@0@.@1@'.format(api_version_major, api_version_minor)
gimp_api_name = 'gimp-' + gimp_api_version



gimp_command  = 'gimp-' + gimp_app_version

gettext_package= 'gimp@0@@1@'.format(api_version_major, api_version_minor)
conf.set_quoted('GETTEXT_PACKAGE', gettext_package)

conf.set_quoted('GIMP_VERSION', gimp_version)

stable = (gimp_app_version_minor % 2 == 0)
conf.set10('GIMP_UNSTABLE', not stable)

versionconfig = configuration_data()
versionconfig.set('GIMP_FULL_NAME',     full_name)
versionconfig.set('GIMP_MAJOR_VERSION', gimp_app_version_major)
versionconfig.set('GIMP_MINOR_VERSION', gimp_app_version_minor)
versionconfig.set('GIMP_MICRO_VERSION', gimp_app_version_micro)
versionconfig.set('GIMP_VERSION',       gimp_version)
versionconfig.set('GIMP_API_VERSION',   gimp_api_version)


# Libtool versionning
gimp_interface_age = 0
lt_current = 0
lt_revision = gimp_app_version_micro
lt_age = 100*api_version_minor + 0
so_version = '@0@.@1@.@2@'.format(lt_current, lt_age, lt_revision)


################################################################################
# Get configuration and Meson modules

pkgconfig = import('pkgconfig')
i18n      = import('i18n')
gnome     = import('gnome')
pythonmod = import('python')

cc        = meson.get_compiler('c')
cxx       = meson.get_compiler('cpp')
prefix    = get_option('prefix')
buildtype = get_option('buildtype')

compiler_args = []
linker_args = []

################################################################################
# Host system detection

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

platform_linux = (
  host_os.contains('linux')
)

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
  # compiler_args += '-Wl,--large-address-aware'
endif


if cc.get_id() == 'gcc' and cc.version() == '7.2.0'
  warning('''
    GCC 7.2.0 has a serious bug affecting GEGL/GIMP. We advise
    against using this version of the compiler (previous and
    further versions are fine).
    See https://bugzilla.gnome.org/show_bug.cgi?id=787222
  ''')
endif



################################################################################
# Compiler CPU extensions for optimizations

if (get_option('buildtype') == 'release' or
    get_option('buildtype') == 'debugoptimized')

  # Check for compiler CPU extensions
  existing_cpu_exts = [
    '-mfpmath=sse',
    '-mmmx',
    '-msse',
    '-msse2',
    '-msse4.1',
  ]
  supported_cpu_exts = cc.get_supported_arguments(existing_cpu_exts)

  compiler_args += supported_cpu_exts

  conf.set  ('USE_MMX',                   '-mmmx'    in supported_cpu_exts)
  conf.set  ('USE_SSE',                   '-msse'    in supported_cpu_exts)
  conf.set10('COMPILE_SSE2_INTRINISICS',  '-msse2'   in supported_cpu_exts)
  conf.set10('COMPILE_SSE4_1_INTRINISICS','-msse4.1' in supported_cpu_exts)


  have_altivec        = false
  have_altivec_sysctl = false
  if arch_ppc
    altivec_args = cc.get_supported_arguments([
      '-faltivec',
      '-maltivec',
      '-mabi=altivec',
    ])

    if altivec_args != []
      compiler_args += altivec_args
      linker_args   += altivec_args

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
# CFlags


if get_option('profiling') and cc.get_id() == 'gcc'
  compiler_args += '-pg'
  linker_args   += '-pg'
endif

if get_option('ansi')
  compiler_args += [ '-ansi', '-pedantic']
endif

warning_cflags_common = [
  '-fdiagnostics-show-option',
  '-fno-common',

  '-Wformat-security',
  '-Winit-self',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wpointer-arith',
  '-Wreturn-type',
  '-Wtype-limits',

  # You can uncomment this to debug with less warning outputs
  '-Wno-deprecated-declarations',

  '-Werror=implicit-function-declaration',
]
warning_cflags_c = [
  '-Wdeclaration-after-statement',
  '-Wold-style-definition',
  '-Wno-strict-prototypes',
]
warning_cflags_cpp = [
]

compiler_args += cc.get_supported_arguments(warning_cflags_common)
add_project_arguments(cc .get_supported_arguments(warning_cflags_c),   language: 'c')
add_project_arguments(cxx.get_supported_arguments(warning_cflags_cpp), language: 'cpp')


# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc.
if platform_win32 and cc.get_id() == 'gcc'
  msvc_compat_args = cc.first_supported_argument([
    '-fnative-struct',
    '-mms-bitfields',
  ])
  if msvc_compat_args == []
    error('''
      GCC does not support '-fnative-struct' nor '-mms-bitfields'.
      Build will be incompatible with GTK+ DLLs.
    ''')
  endif
  compiler_args += msvc_compat_args
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
# Dependencies
no_dep = dependency('', required: false)

################################################################################
# Mandatory Dependencies

relocatable_bundle = get_option('relocatable-bundle')
conf.set('ENABLE_RELOCATABLE_RESOURCES', relocatable_bundle)


math              = cc.find_library('m')
dl                = platform_win32 ? no_dep : cc.find_library('dl')
rpc               = platform_win32 ? cc.find_library('rpcrt4') : no_dep
dbghelp           = platform_win32 ? cc.find_library('dbghelp') : no_dep

atk               = dependency('atk',                version: '>=2.4.0')

babl              = dependency('babl',               version: '>=0.1.72')

cairo             = dependency('cairo',              version: '>=1.12.2')

dbus_glib         = dependency('dbus-glib-1')

# fontconfig_name   = platform_win32 ? 'fontconfig_win32' : 'fontconfig'
fontconfig_name   = 'fontconfig'
fontconfig        = dependency(fontconfig_name,      version: '>=2.12.4')

freetype2         = dependency('freetype2',          version: '>=2.1.7')

gdk_pixbuf        = dependency('gdk-pixbuf-2.0',     version: '>=2.30.8')

gegl              = dependency('gegl-0.4',           version: '>=0.4.17')

gexiv2            = dependency('gexiv2',             version: '>=0.10.6')

gio               = dependency('gio-2.0')
gio_specific_name = platform_win32 ? 'gio-windows-2.0' : 'gio-unix-2.0'
gio_specific      = dependency(gio_specific_name)

glib_version = platform_win32 ? '>=2.56.2' : '>=2.54.2'
glib              = dependency('glib-2.0',           version: glib_version)
conf.set('G_DISABLE_DEPRECATED',    glib.version().version_compare('>=2.57'))

gobject           = dependency('gobject-2.0',        version: '>=2.54.2')
gmodule           = dependency('gmodule-no-export-2.0')

gtk3              = dependency('gtk+-3.0',           version: '>=3.16.10')

harfbuzz          = dependency('harfbuzz',           version: '>=0.9.19')
lcms              = dependency('lcms2',              version: '>=2.8')

libmypaint        = dependency('libmypaint',         version: '>=1.3.0')

libmypaint_brushes= relocatable_bundle ? no_dep : \
  dependency('mypaint-brushes-1.0', version: '>=1.3.0', required: false)

libmypaint_brushes_dir = libmypaint_brushes.get_pkgconfig_variable(
  'brushesdir',
  default: '${gimp_installation_dir}'/'share'/'mypaint-data'/'1.0'/'brushes',
)

conf.set_quoted('MYPAINT_BRUSHES_DIR', libmypaint_brushes_dir)

pangocairo        = dependency('pangocairo',         version: '>=1.29.4')
pangoft2          = dependency('pangoft2',           version: '>=1.29.4')
rsvg              = dependency('librsvg-2.0',        version: '>=2.40.6')

conf.set('PANGO_DISABLE_DEPRECATED',pangocairo.version().version_compare('<1.43'))


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
  if meson.is_cross_build()
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
# Check for extension support

appstream_glib = dependency('appstream-glib',     version: '>=0.7.7')

libarchive = dependency('libarchive')


################################################################################
# Check for debug console (Win32)

if platform_win32
  conf.set('ENABLE_WIN32_DEBUG_CONSOLE', get_option('win32-debug-console'))
endif

################################################################################
# Check for 32-bit DLLs (Win32 64-bit)

if platform_win32 and arch_x86_64
  conf.set_quoted('WIN32_32BIT_DLL_FOLDER', get_option('win32-32bits-dll-folder'))
endif

################################################################################
# Check for detailed backtraces support

## Check for libbacktrace

libbacktrace = ( get_option('libbacktrace')
  ? cc.find_library('backtrace', has_headers: 'backtrace.h', required: false)
  : no_dep
)
conf.set('HAVE_LIBBACKTRACE', libbacktrace.found())

## Check for libunwind
libunwind = ( get_option('libunwind')
  ? dependency('libunwind', version: '>=1.1.0', required: false)
  : no_dep
)
conf.set('HAVE_LIBUNWIND', libunwind.found())


## Check for Dr. Mingw
drmingw = no_dep
if platform_win32
  exchndl = cc.find_library('exchndl')
  exchndl_fn = cc.has_function('ExcHndlSetLogFileNameA', dependencies: exchndl)
  if exchndl.found() and exchndl_fn
    drmingw = declare_dependency(dependencies: exchndl)
  endif
endif
conf.set('HAVE_EXCHNDL', drmingw.found())


detailed_backtraces = (
  libbacktrace.found() or
  libunwind.found() or
  drmingw.found()
)

################################################################################
# Check for x11 support

x11_target = gtk3.get_pkgconfig_variable('targets').contains('x11')

x11   = x11_target ? dependency('x11')    : no_dep
xmu   = x11_target ? dependency('xmu')    : no_dep
xext  = x11_target ? dependency('xext')   : no_dep
xfixes= x11_target ? dependency('xfixes') : no_dep
x11_deps = [ x11, xmu, xext, xfixes ]
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
conf.set('HAVE_X11_EXTENSIONS_SHAPE_H', x11_target and cc.has_header('X11/extensions/shape.h'))
conf.set('HAVE_X11_XMU_WINUTIL_H',      x11_target and cc.has_header('X11/Xmu/WinUtil.h'))


# Features requiring x11

have_doc_shooter= x11_target

if get_option('print').disabled()
  have_print = false
elif get_option('print').enabled() and not x11_target
  error('Printing requires x libs')
else
  have_print = x11_target
endif

if get_option('screenshot').disabled()
  have_screenshot = false
elif get_option('screenshot').enabled() and not x11_target
  error('Printing requires x libs')
else
  have_screenshot = x11_target
endif

################################################################################
# Plugins (optional dependencies)

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



libtiff = dependency('libtiff-4', required: get_option('tiff'))
if libtiff.found()
  MIMEtypes += 'image/tiff'
endif


libjpeg = dependency('libjpeg', required: get_option('jpeg'))
conf.set('HAVE_LIBJPEG', libjpeg.found())
if libjpeg.found()
  MIMEtypes += 'image/jpeg'
endif


zlib = dependency('zlib')
MIMEtypes += 'image/x-psp'

bz2 = cc.find_library('bz2')

liblzma = dependency('liblzma', version: '>=5.0.0')


ghostscript = cc.find_library('gs', required: get_option('ghostscript'))
if ghostscript.found()
  MIMEtypes += 'application/postscript'
else
  ghostscript = disabler()
endif


libpng = dependency('libpng', version: '>=1.6.25', required: get_option('png'))
if libpng.found()
  MIMEtypes += [ 'image/png', 'image/x-icon']
endif


libmng = cc.find_library('mng', required: get_option('mng'))

libaa = cc.find_library('aa', required: get_option('aa'))

libxpm = dependency('xpm', required: get_option('xpm'))
if libxpm.found()
  MIMEtypes += 'image/x-xpixmap'
endif


openexr = dependency('OpenEXR', version: '>=1.6.1', required: get_option('openexr'))
if openexr.found()
  MIMEtypes += 'image/x-exr'
endif

webp_libs = [
  dependency('libwebp',     version: '>=0.6.0', required: get_option('webp')),
  dependency('libwebpmux',  version: '>=0.6.0', required: get_option('webp')),
  dependency('libwebpdemux',version: '>=0.6.0', required: get_option('webp')),
]
webp_found = true
foreach lib : webp_libs
  webp_found = webp_found and  lib.found()
endforeach

if webp_found
  MIMEtypes += 'image/x-webp'
endif


libheif = dependency('libheif', version: '>=1.3.2', required: get_option('heif'))
conf.set('HAVE_LIBHEIF_4_1_0', libheif.version().version_compare('>=1.4.0'))

if libheif.found()
  MIMEtypes += [
    'image/heif',
    'image/heic',
  ]
endif

webkit = dependency('webkit2gtk-4.0', version: '>=2.20.3', required: get_option('webkit'))
conf.set('HAVE_WEBKIT', webkit.found())


if get_option('poppler')
  poppler = [
    dependency('poppler-glib', version: '>=0.69.0'),
    dependency('poppler-data', version: '>=0.4.9'),
  ]
else
  poppler = no_dep
endif


cairopdf = dependency('cairo-pdf', version: '>=1.12.2', required: get_option('cairo-pdf'))

# PDF import support is a granted feature.
MIMEtypes += 'application/pdf'

wmf = dependency('libwmf', version: '>=0.2.8', required: get_option('wmf'))
if wmf.found()
  MIMEtypes += 'image/x-wmf'
endif


openjpeg = dependency('libopenjp2', version: '>=2.1.0', required: get_option('jpeg2000'))
if openjpeg.found()
  MIMEtypes += [ 'image/jp2', 'image/jpeg2000', 'image/jpx', ]
endif


xmc = dependency('xcursor', required: get_option('xcursor'))
if xmc.found()
  MIMEtypes += 'image/x-xcursor'
endif


alsa = dependency('alsa', version: '>=1.0.0', required: get_option('alsa'))
conf.set('HAVE_ALSA', alsa.found())


if get_option('linux-input').disabled()
  have_linuxinput = false
elif get_option('linux-input').enabled() and not cc.has_header('linux/input.h')
  error('linux/input.h header not found.')
else
  have_linuxinput = x11_target
endif

# DirectX DirectInput
directx = no_dep
directx_sdk_path = get_option('directx-sdk-dir')
if directx_sdk_path != '' and platform_win32
  if directx_sdk_path.contains(' ') or directx_sdk_path.contains('\\')
    error('\n'.join([
      'The DirectX SDK path should be given :',
      '* without spaces (use MSys mounts)',
      '* with plain (forward) slashes only,'
    ]))
  endif

  directx = declare_dependency(
    link_with: cc.find_library('dxguid',
                  dirs:  directx_sdk_path / 'Lib' / 'x86'),
    include_directories: directx_sdk_path / 'Include',
  )
endif
conf.set('HAVE_DX_DINPUT', directx.found())

gudev = dependency('gudev-1.0', version: '>=167', required: get_option('gudev'))
conf.set('HAVE_LIBGUDEV', gudev.found())


################################################################################
# Email sending
email_message = false

sendmail_choice = get_option('with-sendmail')
if not [ '', 'false', 'no', ].contains(sendmail_choice)
  if [ 'true', 'yes' ].contains(sendmail_choice)
    sendmail_path = 'sendmail'
  else
    sendmail_path = sendmail_choice
  endif

  sendmail = find_program(sendmail_path, required: false)
  if sendmail.found()
    sendmail_path = sendmail.path()
  else
    warning(
      'Sendmail specified but not found. It should be installed at runtime!'
    )
  endif

  email_message = '@0@ (@1@)'.format(true, sendmail_path)
  conf.set_quoted('SENDMAIL', sendmail_path)
else
  xdg_email_path = 'xdg-email'
  xdg_email = find_program(xdg_email_path, required: false)
  if xdg_email.found()
    xdg_email_path = xdg_email.path()
  else
    warning(
      'Xdg-email not found, but required at runtime for email sending.'
    )
  endif

  email_message = '@0@ (@1@)'.format(true, xdg_email_path)
endif


################################################################################
# ISO codes

isocodes = dependency('iso-codes')
isocodes_prefix = isocodes.get_pkgconfig_variable('prefix')
isocodes_location = isocodes_prefix / get_option('datadir') / 'xml' / 'iso-codes'
isocodes_localedir= isocodes_prefix / get_option('datadir') / 'locale'
conf.set('HAVE_ISO_CODES', isocodes.found())


################################################################################
# Program tools

perl = find_program('perl5', 'perl', 'perl5.005', 'perl5.004', 'perl')

## Python

# By default, we want packagers to install Python plug-ins to get the
# optimum experience. -Dpython=true will check for a Python 3
# interpreter and PyGObject, and warns without.
# It is up to the packager to ensure they are available at run time.
# This can be useful in particular when cross-compiling since anyway
# the interpreter is not useful at build time.

python3_required_version = '>=3.6.0'

if get_option('python') == 'never'
  warning('''
    You disabled the installation of core Python plug-ins. This is discouraged
    as it won't provide the full GIMP experience.
    Note that you may install the Python plug-ins even if you have no Python
    interpreter at build-time by passing the configure option -Dpython=always.
    Just make sure that a Python @0@ interpreter is available at run-time.
  '''.format(python3_required_version))

  have_python = false

else
  always_install = get_option('python') == 'always'

  python = pythonmod.find_installation('python3', required: false)

  python_found = (
    python.found() and
    python.language_version().version_compare(python3_required_version)
  )
  if python_found
    pygobject_found = run_command(
      python, '-c',
      '\n'.join([
        '''import sys, gi''',
        '''version = '@0@' '''.format('3.0'),
        '''sys.exit(gi.check_version(version))''',
      ]),
    ).returncode() == 0
    python_found = python_found and pygobject_found
  endif

  if (not python_found) and (not always_install)
    error('''
    Python @0@ or PyGObject was not found.
    Note that you may install the Python plug-ins even if you have no Python
    interpreter at build-time by passing the configure option -Dpython=always.
    Just make sure that a Python @0@ interpreter is available at run-time.
    '''.format(python3_required_version))
  endif

  if (not python_found) and (always_install)
    warning('''
    Python @0@ or PyGObject was not found.
    Python plug-ins will be installed anyway but you should make sure that
    a compatible Python interpreter is available at installation, otherwise
    installed plug-ins won't be usable.
    '''.format(python3_required_version))
  endif

  have_python = python_found or always_install
endif

## Javascript

if get_option('javascript') == 'never'
  warning('''
    You disabled the installation of core Javascript plug-ins. This is
    discouraged as it won't provide the full GIMP experience.
    Note that you may install the Javascript plug-ins even if you have no GJS
    interpreter at build-time by passing the configure option -Djavascript=always.
    Just make sure that the GJS interpreter is available at run-time.
  ''')
  have_javascript = false

else
  always_install = get_option('javascript') == 'always'

  gjs = find_program('gjs', required: false)

  if (not gjs.found()) and (not always_install)
    error('''
    GJS was not found.
    Note that you may install the Javascript plug-ins even if you have no GJS
    interpreter at build-time by passing the configure option -Djavascript=always.
    Just make sure that the GJS interpreter is available at run-time.
    ''')
  endif

  if (not gjs.found()) and (always_install)
    warning('''
    GJS was not found.
    JavaScript plug-ins will be installed anyway but you should make sure that
    the JavaScript interpreter GJS is available at installation, otherwise
    installed plug-ins won't be usable.
    ''')
  endif

  have_javascript = gjs.found() or always_install
endif

## Lua

if get_option('lua') == 'never'
  warning('''
    You disabled the installation of core Lua plug-ins. This is discouraged
    as it won't provide the full GIMP experience.
    Note that you may install the Lua plug-ins even if you have no Lua LGI
    interpreter at build-time by passing the configure option -Dlua=always.
    Just make sure that the Lua LGI interpreter is available at run-time.
  ''')
  have_lua = false

else
  always_install = get_option('lua') == 'always'

  lua = find_program('luajit', required: false)

  if (not lua.found()) and (not always_install)
    error('''
    Luajit was not found.
    Note that you may install the Lua plug-ins even if you have no Lua LGI
    interpreter at build-time by passing the configure option -Dlua=always.
    Just make sure that the Lua LGI interpreter is available at run-time.
    ''')
  endif

  if (not lua.found()) and always_install
    warning('''
    Luajit was not found.
    Lua plug-ins will be installed anyway but you should make sure that
    luajit and LGI are available at installation, otherwise
    installed plug-ins won't be usable.
    ''')
  endif

  have_lua = lua.found() or always_install
endif


# Check for GTK Mac Integration
if platform_osx and (gtk3.get_pkgconfig_variable('targets').contains('xquartz'))
  gtk3_macos = dependency('gtk-mac-integration', version: '>=2.0.0')
else
  gtk3_macos = no_dep
endif

# Check for XML tools
xmllint             = find_program('xmllint', required: false)
xsltproc            = find_program('xsltproc')
intltool_merge      = find_program('intltool-merge')
desktop_validate    = find_program('desktop-file-validate', required: false)

appstream_util = find_program('appstream-util', required: get_option('appdata-test'))

# Check for doc generation tools
have_gtk_doc = get_option('gtk-doc')

# Check for vector icons
have_vector_icons = get_option('vec-icons')
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

xvfb_run = find_program('xvfb-run', required: get_option('xvfb-run'))
conf.set('HAVE_XVFB_RUN', xvfb_run.found())

# Set bug report URL

# Allowing third-party packagers to set their own bugtracker URL, in
# order to filter first packaging bugs from core bugs.
bug_report_url = get_option('bug-report-url')

if bug_report_url == ''
  message('''
    NOTE: if you plan on packaging GIMP for distribution, it is recommended
    to override the bug report URL with option:
      -Dbug-report-url=https://example.com/
    so that you can filter packaging bugs from core bugs before reporting upstream.
  ''')

  bug_report_url = project_url_issues
endif
conf.set_quoted('PACKAGE_BUGREPORT', project_url_issues)
conf.set_quoted('BUG_REPORT_URL', bug_report_url)

# Default ICC directory #

# This is necessary because some Unix systems may have a different
# standard path for color profiles. And in particular, sandbox builds
# might mount the host system at a different root. This is for
# instance the case of flatpak which mount the host root at /run/host/.

# if not (platform_osx or platform_win32)

icc_directory = get_option('icc-directory')
if icc_directory == ''
  icc_directory = '/usr/share/color/icc'
endif
conf.set_quoted('COLOR_PROFILE_DIRECTORY', icc_directory)

# endif

enable_default_bin = get_option('enable-default-bin')
enable_console_bin = get_option('enable-console-bin')


# Possibly change default gimpdir from $XDG_CONFIG_HOME/GIMP/gimp_user_version
gimpdir = get_option('gimpdir')
if gimpdir == ''
  # Default value
  gimpdir = meson.project_name().to_upper()
endif

project_subdir = meson.project_name() / gimp_app_version
gimpdatadir    = get_option('datadir')    / project_subdir
gimpplugindir  = get_option('libdir')     / project_subdir
gimpsysconfdir = get_option('sysconfdir') / project_subdir
gimpmanpagedir = gimpdir
localedir      = get_option('datadir') / 'locale'



# Check for internal tools
defcheck            = find_program('tools'/'defcheck.py')
extract_vector_icon = find_program('tools'/'extract-vector-icon.sh')
generate_changelog  = find_program('tools'/'generate_changelog.sh')
generate_news       = find_program('tools'/'generate-news')
gimppath2svg        = find_program('tools'/'gimppath2svg.py')
module_dependencies = find_program('tools'/'module-dependencies.py')

gimp_mkenums        = find_program('tools'/'gimp-mkenums')
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

# localedir = get_option('prefix') / get_option('localedir')


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
    { 'm': 'HAVE_ALLOCA',                   'v': 'alloca', },
    { 'm': 'HAVE_BACKTRACE',                'v': 'backtrace', },
    { 'm': 'HAVE_DCGETTEXT',                'v': 'dcgettext', },
    { 'm': 'HAVE_DIFFTIME',                 'v': 'difftime', },
    { 'm': 'HAVE_FINITE',                   'v': 'finite', },
    { 'm': 'HAVE_FINITE',                   'v': 'finite', },
    { 'm': 'HAVE_FSYNC',                    'v': 'fsync', },
    { 'm': 'HAVE_GETADDRINFO',              'v': 'getaddrinfo', },
    { 'm': 'HAVE_GETNAMEINFO',              'v': 'getnameinfo', },
    { 'm': 'HAVE_GETTEXT',                  'v': 'gettext', },
    { 'm': 'HAVE_ISFINITE',                 'v': 'isfinite', },
    { 'm': 'HAVE_MMAP',                     'v': 'mmap', },
    { 'm': 'HAVE_RINT',                     'v': 'rint', },
    { 'm': 'HAVE_THR_SELF',                 'v': 'thr_self', },
    { 'm': 'HAVE_VFORK',                    'v': 'vfork', },
  ]
  conf.set(fn['m'],
    cc.has_function(fn['v']) ? 1 : false
  )
endforeach

conf.set('HAVE_BIND_TEXTDOMAIN_CODESET',
  cc.has_header_symbol('libintl.h', 'bind_textdomain_codeset') ? 1 : false
)
conf.set('HAVE_VPRINTF',
  cc.has_header_symbol('libintl.h', 'vprintf') ? 1 : false
)


# Check for available headers
foreach header : [
    { 'm': 'HAVE_ALLOCA_H',       'v': 'alloca.h' },
    { 'm': 'HAVE_DLFCN_H',        'v': 'dlfcn.h' },
    { 'm': 'HAVE_EXECINFO_H',     'v': 'execinfo.h' },
    { 'm': 'HAVE_FCNTL_H',        'v': 'fcntl.h' },
    { 'm': 'HAVE_IEEEFP_H',       'v': 'ieeefp.h' },
    { 'm': 'HAVE_INTTYPES_H',     'v': 'inttypes.h' },
    { 'm': 'HAVE_LOCALE_H',       'v': 'locale.h' },
    { 'm': 'HAVE_MATH_H',         'v': 'math.h' },
    { 'm': 'HAVE_MEMORY_H',       'v': 'memory.h' },
    { 'm': 'HAVE_STDINT_H',       'v': 'stdint.h' },
    { 'm': 'HAVE_STDLIB_H',       'v': 'stdlib.h' },
    { 'm': 'HAVE_STRING_H',       'v': 'string.h' },
    { 'm': 'HAVE_STRINGS_H',      'v': 'strings.h' },
    { 'm': 'HAVE_SYS_PARAM_H',    'v': 'sys/param.h' },
    { 'm': 'HAVE_SYS_SELECT_H',   'v': 'sys/select.h' },
    { 'm': 'HAVE_SYS_STAT_H',     'v': 'sys/stat.h' },
    { 'm': 'HAVE_SYS_THR_H',      'v': 'sys/thr.h' },
    { 'm': 'HAVE_SYS_TIME_H',     'v': 'sys/time.h' },
    { 'm': 'HAVE_SYS_TIMES_H',    'v': 'sys/times.h' },
    { 'm': 'HAVE_SYS_TYPES_H',    'v': 'sys/types.h' },
    { 'm': 'HAVE_SYS_WAIT_H',     'v': 'sys/wait.h' },
    { 'm': 'HAVE_UNISTD_H',       'v': 'unistd.h' },
    { 'm': 'HAVE_MMAN_H',         'v': 'sys/mman.h' },
    { 'm': 'HAVE_IPC_H',          'v': 'sys/ipc.h' },
    { 'm': 'HAVE_SHM_H',          'v': 'sys/shm.h' },
  ]
  conf.set(header['m'], cc.has_header(header['v']) ? 1 : false)
endforeach



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

# Compiler
conf.set_quoted('CC',             cc.get_id())
conf.set_quoted('CC_VERSION',     cc.version())

# Names
conf.set_quoted('GIMP_PACKAGE',   meson.project_name())
conf.set_quoted('PACKAGE_NAME',   meson.project_name())
conf.set_quoted('PACKAGE_STRING', package_string)
conf.set_quoted('GIMP_COMMAND',   gimp_command)

# Versions
conf.set_quoted('GIMP_APP_VERSION_STRING',gimp_app_version)
conf.set_quoted('GIMP_APP_VERSION',       gimp_app_version)
conf.set_quoted('GIMP_USER_VERSION',      gimp_app_version)

conf.set_quoted('GIMP_DATA_VERSION',      gimp_app_version)
conf.set_quoted('GIMP_PLUGIN_VERSION',    gimp_app_version)
conf.set_quoted('GIMP_SYSCONF_VERSION',   gimp_app_version)
conf.set_quoted('GIMP_TOOL_VERSION',      gimp_app_version)
conf.set_quoted('GIMP_PKGCONFIG_VERSION', gimp_api_version)

# Directories
conf.set_quoted('PREFIX',          prefix)
conf.set_quoted('EXEC_PREFIX',     prefix)
conf.set_quoted('GIMPDIR',         gimpdir)
conf.set_quoted('GIMPSYSCONFDIR',  prefix / gimpsysconfdir)
conf.set_quoted('GIMPDATADIR',     prefix / gimpdatadir)
conf.set_quoted('GIMPPLUGINDIR',   prefix / gimpplugindir)
conf.set_quoted('PLUGINDIR',       prefix / gimpplugindir)
conf.set_quoted('LOCALEDIR',       prefix / localedir)

conf.set_quoted('LOCALSTATEDIR',   prefix / get_option('localstatedir'))
# /usr/com?
conf.set_quoted('SHAREDSTATEDIR',  prefix / get_option('sharedstatedir'))
conf.set_quoted('SYSCONFDIR',      prefix / get_option('sysconfdir'))
conf.set_quoted('BINDIR',          prefix / get_option('bindir'))
conf.set_quoted('DATAROOTDIR',     prefix / get_option('datadir'))
conf.set_quoted('INFODIR',         prefix / get_option('infodir'))
conf.set_quoted('LIBDIR',          prefix / get_option('libdir'))
conf.set_quoted('LIBEXECDIR',      prefix / get_option('libexecdir'))
conf.set_quoted('MANDIR',          prefix / get_option('mandir'))
conf.set_quoted('SBINDIR',         prefix / get_option('sbindir'))
conf.set_quoted('SYSDATADIR',      prefix / get_option('datadir'))

# Third-party/Misc
conf.set_quoted('ISO_CODES_LOCATION',    isocodes_location)
conf.set_quoted('ISO_CODES_LOCALEDIR',   isocodes_localedir)


if platform_osx
  # libgimp_cflags += '-xobjective-c'
  # libgimp_lflags += ['-framework', 'Cocoa']
endif


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
install_conf.set('GIMP_APP_VERSION', gimp_app_version)
install_conf.set('GIMP_PKGCONFIG_VERSION', gimp_version)
install_conf.set('GIMP_VERSION', gimp_version)
install_conf.set('APPSTREAM_GLIB_REQUIRED_VERSION', appstream_glib.version())
install_conf.set('ATK_REQUIRED_VERSION',          atk         .version())
install_conf.set('BABL_REQUIRED_VERSION',         babl        .version())
install_conf.set('CAIRO_PDF_REQUIRED_VERSION',    cairopdf    .version())
install_conf.set('CAIRO_REQUIRED_VERSION',        cairo       .version())
install_conf.set('FONTCONFIG_REQUIRED_VERSION',   fontconfig  .version())
install_conf.set('FREETYPE2_REQUIRED_VERSION',    freetype2   .version())
install_conf.set('GDK_PIXBUF_REQUIRED_VERSION',   gdk_pixbuf  .version())
install_conf.set('GEGL_REQUIRED_VERSION',         gegl        .version())
install_conf.set('GEXIV2_REQUIRED_VERSION',       gexiv2.version())
install_conf.set('GLIB_REQUIRED_VERSION',         glib        .version())
install_conf.set('GTK_REQUIRED_VERSION',          gtk3        .version())
install_conf.set('HARFBUZZ_REQUIRED_VERSION',     harfbuzz    .version())
install_conf.set('INTLTOOL_REQUIRED_VERSION',     '0.40.1')
install_conf.set('LCMS_REQUIRED_VERSION',         lcms        .version())
install_conf.set('LIBHEIF_REQUIRED_VERSION',      libheif     .version())
install_conf.set('LIBLZMA_REQUIRED_VERSION',      liblzma     .version())
install_conf.set('LIBMYPAINT_REQUIRED_VERSION',   libmypaint  .version())
install_conf.set('LIBPNG_REQUIRED_VERSION',       libpng      .version())
install_conf.set('OPENEXR_REQUIRED_VERSION',      openexr     .version())
install_conf.set('OPENJPEG_REQUIRED_VERSION',     openjpeg.version())
install_conf.set('PANGOCAIRO_REQUIRED_VERSION',   pangocairo  .version())
install_conf.set('POPPLER_DATA_REQUIRED_VERSION', poppler[1]  .version())
install_conf.set('POPPLER_REQUIRED_VERSION',      poppler[0]  .version())
install_conf.set('PYTHON3_REQUIRED_VERSION',      '3.6.0')
install_conf.set('RSVG_REQUIRED_VERSION',         rsvg        .version())
install_conf.set('WEBKITGTK_REQUIRED_VERSION',    webkit      .version())
install_conf.set('WEBP_REQUIRED_VERSION',         webp_libs[0].version())
install_conf.set('WMF_REQUIRED_VERSION',          wmf         .version())
install_conf.set('XGETTEXT_REQUIRED_VERSION', '0.19')

configure_file(
  input : 'INSTALL.in',
  output: 'INSTALL',
  configuration: install_conf
)


configure_file(
  output: 'config.h',
  configuration: conf
)
compiler_args +='-DHAVE_CONFIG_H'


add_project_arguments(compiler_args, language: [ 'c', 'cpp' ])
add_project_link_arguments(linker_args, language: [ 'c', 'cpp' ])

################################################################################
# Miscelaneous targets


 #     #                    #######
 ##   ## #  ####   ####        #      ##   #####   ####  ###### #####  ####
 # # # # # #      #    #       #     #  #  #    # #    # #        #   #
 #  #  # #  ####  #            #    #    # #    # #      #####    #    ####
 #     # #      # #            #    ###### #####  #  ### #        #        #
 #     # # #    # #    #       #    #    # #   #  #    # #        #   #    #
 #     # #  ####   ####        #    #    # #    #  ####  ######   #    ####



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

if xmllint.found()
  run_target('validate-authors',
    command: [
      xmllint,
      '--noout',
      '--valid', 'authors.xml',
    ],
  )
endif

if have_python
  # TODO this python script is waiting for autotools directory/files structure
  custom_target('check-defs',
    input : [ ],
    output: [ 'check-defs', ],
    command: [
      find_program(meson.source_root() / 'tools' / 'defcheck.py'),
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


rootInclude = include_directories('.')
appInclude  = include_directories('app')

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
subdir('app-tools')

# Docs
subdir('docs')
if have_gtk_doc
  subdir('devel-docs')
endif



pkgconfig.generate(libgimp,
  filebase: 'gimp-' + gimp_api_version,
  name: prettyname,
  description: 'GIMP Library',
  version: gimp_version,
  requires: [
    gdk_pixbuf,
    cairo,
    gegl,
  ],
  libraries: [
    libgimpbase,
    libgimpcolor,
    libgimpconfig,
    libgimpmath,
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

pkgconfig.generate(libgimpthumb,
  filebase: 'gimpthumb-' + gimp_api_version,
  name: 'GIMP Thumb',
  description: 'GIMP Thumbnail Library',
  version: gimp_version,
  requires: [
    libgimp,
    gdk_pixbuf,
  ],
  subdirs: [
    gimp_api_name,
  ],
)

pkgconfig.generate(libgimpui,
  filebase: 'gimpui-' + gimp_api_version,
  name: 'GIMP UI',
  description: 'GIMP User Interface Library',
  version: gimp_version,
  requires: [
    libgimp,
    gtk3,
  ],
  libraries: [
    libgimpwidgets,
    libgimpmodule,
  ],
  subdirs: [
    gimp_api_name,
  ],
)



################################################################################

final_message = [
'''Extra Binaries:''',
'''  gimp-console:        @0@'''.format(enable_console_bin),
'',
'''Optional Features:''',
'''  Language selection:  @0@'''.format(isocodes.found()),
'''  Vector icons:        @0@'''.format(have_vector_icons),
'''  Dr. Mingw (Win32):   @0@'''.format(drmingw.found()),
'''  Relocatable Bundle:  @0@'''.format(relocatable_bundle),
'''  Default ICC directory:     @0@'''.format(icc_directory),
'''  32-bit DLL folder (Win32): @0@'''.format(get_option('win32-32bits-dll-folder')),
'''  Detailed backtraces:       @0@'''.format(detailed_backtraces),
'',
'''Optional Plug-Ins:''',
'''  Ascii Art:           @0@'''.format(libaa.found()),
'''  Ghostscript:         @0@'''.format(ghostscript.found()),
'''  Help Browser:        @0@'''.format(webkit.found()),
'''  JPEG 2000:           @0@'''.format(openjpeg.found()),
'''  MNG:                 @0@'''.format(libmng.found()),
'''  OpenEXR:             @0@'''.format(openexr.found()),
'''  WebP:                @0@'''.format(webp_found),
'''  Heif:                @0@'''.format(libheif.found()),
'''  PDF (export):        @0@'''.format(cairopdf.found()),
'''  Print:               @0@'''.format(have_print),
'''  Python 3 plug-ins:   @0@'''.format(have_python),
'''  Javascript plug-ins: @0@'''.format(have_javascript),
'''  Lua plug-ins:        @0@'''.format(have_lua),
'''  TWAIN (Win32):       @0@'''.format(platform_win32),
'''  Webpage:             @0@'''.format(webkit.found()),
'''  WMF:                 @0@'''.format(wmf.found()),
'''  X11 Mouse Cursor:    @0@'''.format(xmc.found()),
'''  XPM:                 @0@'''.format(libxpm.found()),
'''  Email:               @0@'''.format(email_message),
'',
'''Optional Modules:''',
'''  ALSA (MIDI Input):   @0@'''.format(alsa.found()),
'''  Linux Input:         @0@ (GUdev support: @1@)'''
                                .format(have_linuxinput, gudev.found()),
'''  DirectInput (Win32): @0@'''.format(directx.found()),
'',
'''Tests:''',
'''  Use xvfb-run         @0@'''.format(xvfb_run.found()),
'''  Test appdata         @0@'''.format(appstream_util.found()),
'',
'''Bug report URL: @0@'''.format(bug_report_url),
]

message('\n'.join(final_message))
