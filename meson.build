project('gimp',
  'c', 'cpp',
  version: '2.99.1',
  meson_version: '>=0.50.0',
  default_options: [
    'cpp_std=gnu++14',
    'buildtype=debugoptimized',
  ],
)

project_url = 'https://gitlab.gnome.org/GNOME/gimp'
project_url_issues = project_url + '/issues/new'

conf = configuration_data()

warnings = []

# git-version.h is already present and not generated if dist tarball
is_git_repository = run_command('python3', '-c',
  'import sys,os; sys.exit(0 if os.path.exists(".git") else 1)'
).returncode() == 0

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


# Libtool versioning
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

host_cpu_family = host_machine.cpu_family()
message('Host machine cpu family: ' + host_cpu_family)

host_cpu_family = host_machine.cpu_family()
if   host_cpu_family == 'x86'
  have_x86 = true
  conf.set10('ARCH_X86',    true)
elif host_cpu_family == 'x86_64'
  have_x86 = true
  conf.set10('ARCH_X86',    true)
  conf.set10('ARCH_X86_64', true)
elif host_cpu_family == 'ppc'
  have_ppc = true
  conf.set10('ARCH_PPC',    true)
elif host_cpu_family == 'ppc64'
  have_ppc = true
  conf.set10('ARCH_PPC',    true)
  conf.set10('ARCH_PPC64',  true)
endif


host_os = host_machine.system().to_lower()
message('Host os: ' + host_os)

platform_linux = (
  host_os.contains('linux')
)

platform_windows = (
  host_os.contains('mingw') or
  host_os.contains('cygwin') or
  host_os.contains('windows')
)

platform_osx = (
  host_os.contains('machten') or
  host_os.contains('rhapsody') or
  host_os.contains('darwin')
)

if platform_osx
  conf.set('PLATFORM_OSX', 1)
endif

if platform_windows
  windows = import('windows')
  # AC_CHECK_PROG(ms_librarian, lib.exe, yes, no)
  # AM_CONDITIONAL(MS_LIB_AVAILABLE, test "x$ms_librarian" = xyes)
  # compiler_args += '-Wl,--large-address-aware'
endif

# on OSX ObjC and C sources are mixed so adding objc to the linkflags

osx_ldflags = []
if platform_osx
  add_languages('objc')
  osx_ldflags += ['-Wl,-framework,Foundation', '-Wl,-framework,AppKit', '-ObjC']
  add_project_link_arguments(osx_ldflags, language : ['objc', 'c'])
endif

if cc.get_id() == 'gcc' and cc.version() == '7.2.0'
  gcc_warning = '''
    GCC 7.2.0 has a serious bug affecting GEGL/GIMP. We advise
    against using this version of the compiler (previous and
    further versions are fine).
    See https://bugzilla.gnome.org/show_bug.cgi?id=787222
  '''
  warning(gcc_warning)
  warnings += gcc_warning
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
  if host_cpu_family == 'ppc'
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

  '-Wformat',
  '-Wformat-security',
  '-Winit-self',
  '-Wlogical-op',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wpointer-arith',
  '-Wreturn-type',
  '-Wtype-limits',
]
warning_cflags_c = [
  '-Wabsolute-value',
  '-Wdeclaration-after-statement',
  '-Wenum-conversion',
  '-Wliteral-conversion',
  '-Wno-strict-prototypes',
  '-Wold-style-definition',
  '-Wparentheses-equality',
  '-W#pragma-messages',
  '-Wsometimes-uninitialized',
  '-Wtautological-unsigned-enum-zero-compare',
  '-Wunneeded-internal-declaration',
  '-Wunused-function',
  '-Wunused-value',

  '-Werror=implicit-function-declaration',
]
warning_cflags_cpp = [
]

compiler_args += cc.get_supported_arguments(warning_cflags_common)
add_project_arguments(cc .get_supported_arguments(warning_cflags_c),   language: 'c')
add_project_arguments(cxx.get_supported_arguments(warning_cflags_cpp), language: 'cpp')


# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc.
if platform_windows and cc.get_id() == 'gcc'
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

if get_option('relocatable-bundle') == 'yes'
  relocatable_bundle = true
elif get_option('relocatable-bundle') == 'no'
  relocatable_bundle = false
else # == 'platform-default'
  # By default, assume building for Windows or macOS everything to be on
  # the same prefix and can be relocated.
  # On other platforms, build-time paths are meaningful.
  if platform_windows or platform_osx
      relocatable_bundle = true
  else
      relocatable_bundle = false
  endif
endif
conf.set('ENABLE_RELOCATABLE_RESOURCES', relocatable_bundle)


math              = cc.find_library('m')
dl                = platform_windows ? no_dep : cc.find_library('dl')
rpc               = platform_windows ? cc.find_library('rpcrt4') : no_dep
dbghelp           = platform_windows ? cc.find_library('dbghelp') : no_dep
winsock           = platform_windows ? cc.find_library('ws2_32') : no_dep
atk_minver        = '2.4.0'
atk               = dependency('atk',                version: '>='+atk_minver)
babl_minver       = '0.1.76'
babl              = dependency('babl',               version: '>='+babl_minver)
cairo_minver      = '1.12.2'
cairo             = dependency('cairo',              version: '>='+cairo_minver)

# fontconfig_name   = platform_windows ? 'fontconfig_win32' : 'fontconfig'
fontconfig_name   = 'fontconfig'
fontconfig_minver = '2.12.4'
fontconfig        = dependency(fontconfig_name,      version: '>='+fontconfig_minver)
freetype2_minver  = '2.1.7'
freetype2         = dependency('freetype2',          version: '>='+freetype2_minver)
gdk_pixbuf_minver = '2.30.8'
gdk_pixbuf        = dependency('gdk-pixbuf-2.0',     version: '>='+gdk_pixbuf_minver)
gegl_minver       = '0.4.23'
gegl              = dependency('gegl-0.4',           version: '>='+gegl_minver)
gexiv2_minver     = '0.10.6'
gexiv2            = dependency('gexiv2',             version: '>='+gexiv2_minver)

gio               = dependency('gio-2.0')
gio_specific_name = platform_windows ? 'gio-windows-2.0' : 'gio-unix-2.0'
gio_specific      = dependency(gio_specific_name)

glib_minver       = '2.56.2'
glib              = dependency('glib-2.0',           version: '>='+glib_minver)

conf.set('G_DISABLE_DEPRECATED', glib.version().version_compare('>=2.57'))

gobject           = dependency('gobject-2.0',        version: '>='+glib_minver)
gmodule           = dependency('gmodule-no-export-2.0')

gtk3_minver       = '3.16.10'
gtk3              = dependency('gtk+-3.0',           version: '>='+gtk3_minver)
harfbuzz_minver   = '0.9.19'
harfbuzz          = dependency('harfbuzz',           version: '>='+harfbuzz_minver)
json_glib         = dependency('json-glib-1.0',      version: '>=1.2.6')
lcms_minver       = '2.8'
lcms              = dependency('lcms2',              version: '>='+lcms_minver)
libmypaint_minver = '1.3.0'
libmypaint        = dependency('libmypaint',         version: '>='+libmypaint_minver)
mypaint_brushes   = dependency('mypaint-brushes-1.0',version: '>='+libmypaint_minver)
if not libmypaint.version().version_compare('>=1.4.0')
  libmypaint_warning='''

        libmypaint lower than version 1.4.0 is known to crash when
        parsing MyPaint 2 brushes. Please update.
    '''
    warning(libmypaint_warning)
    warnings += libmypaint_warning
endif


if relocatable_bundle
  mypaint_brushes_dir = '${gimp_installation_dir}'\
                        /'share'/'mypaint-data'/'1.0'/'brushes'
else
  mypaint_brushes_dir = mypaint_brushes.get_pkgconfig_variable('brushesdir')
endif

conf.set_quoted('MYPAINT_BRUSHES_DIR', mypaint_brushes_dir)

pangocairo_minver = '1.42.0'
pangocairo        = dependency('pangocairo',         version: '>='+pangocairo_minver)
pangoft2          = dependency('pangoft2',           version: '>=1.29.4')
rsvg_minver       = '2.40.6'
rsvg              = dependency('librsvg-2.0',        version: '>='+rsvg_minver)

conf.set('PANGO_DISABLE_DEPRECATED',pangocairo.version().version_compare('<1.43'))


################################################################################
# Check for GLib Networking

glib_networking_works_run=false
if meson.is_cross_build() and not meson.has_exe_wrapper()
    # Cross-compilation without run capability: we won't be able to
    # check networking support.
    glib_networking_works = true
    glib_warning = '''
      Test for glib-networking cannot be performed while cross-compiling,
      unless you set an `exe_wrapper` binary in your toolchain file.
      Make sure glib-networking is installed, otherwise GIMP will not be able
      to display the remote help pages through the help browser, nor will it
      be able to open remote HTTPS (or other protocol using SSL/TLS) files.
      HTTPS is becoming the expected standard and should not be considered
      optional anymore.
    '''
    warning(glib_warning)
    warnings += glib_warning
else # not meson.is_cross_build() or meson.has_exe_wrapper()
    glib_networking_works_run = cc.run(
      '''#include <gio/gio.h>
      int main() {
        return !g_tls_backend_supports_tls (g_tls_backend_get_default ());
      }''',
      dependencies: gio,
    )
    glib_networking_works = (glib_networking_works_run.compiled() and
                             glib_networking_works_run.returncode() == 0)
    if not glib_networking_works and meson.is_cross_build()
        # Since cross-platform test runs may be unreliable, let's be
        # flexible and pass the test with a warning.
        glib_networking_works = true
        glib_warning = '''
      The cross-platform test for glib-networking failed, using the
      `exe_wrapper` set in your toolchain file.
      Make sure glib-networking is installed, otherwise GIMP will not be able
      to display the remote help pages through the help browser, nor will it
      be able to open remote HTTPS (or other protocol using SSL/TLS) files.
      HTTPS is becoming the expected standard and should not be considered
      optional anymore.
        '''
        warning(glib_warning)
        warnings += glib_warning
    endif
endif

if not glib_networking_works
  error('Test for glib-networking failed. This is required.')
endif

################################################################################
# Check if Pango is built with a recent fontconfig

pango_check = cc.links(
  '''#include <fontconfig/fontconfig.h>
  int main() {
    FcObjectSet *os; os = FcObjectSetBuild (FC_FAMILY, FC_WIDTH);
  }''',
  dependencies: fontconfig,
)

if not pango_check
  pango_warning = '\n  *** '.join([
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
  ])
  warning(pango_warning)
  warnings += pango_warning
endif

################################################################################
# Optional Dependencies

libsocket         = cc.find_library('socket', required: false)
conf.set('HAVE_LIBSOCKET', libsocket.found())

################################################################################
# Check for extension support

appstream_glib_minver = '0.7.7'
appstream_glib = dependency('appstream-glib', version: '>='+appstream_glib_minver)

libarchive = dependency('libarchive')


################################################################################
# Check for debug console (Win32)

if platform_windows
  conf.set('ENABLE_WIN32_DEBUG_CONSOLE', get_option('win32-debug-console'))
endif

################################################################################
# Check for 32-bit DLLs (Win32 64-bit)

if platform_windows and host_cpu_family == 'x86_64'
  conf.set_quoted('WIN32_32BIT_DLL_FOLDER', get_option('win32-32bits-dll-folder'))
endif

################################################################################
# Check for detailed backtraces support

## Check for libbacktrace

if get_option('libbacktrace')
  libbacktrace = cc.find_library('backtrace', required: false)

  if libbacktrace.found()
    libbacktrace_links = cc.links('''
        #include <stddef.h>
        #include <backtrace.h>
        #include <backtrace-supported.h>

        #if ! BACKTRACE_SUPPORTED
        #   error ! BACKTRACE_SUPPORTED
        #endif

        int main() {
          (void) backtrace_create_state (NULL, 0, NULL, NULL);
          return 0;
        }
      ''',
      dependencies: libbacktrace,
    )

    if not libbacktrace_links
      warning(
        'libbacktrace was found, but the test compilation failed.\n' +
        'You can find more info in meson-logs/meson-logs.txt.'
      )
      libbacktrace = no_dep
    endif

  endif
else
  libbacktrace = no_dep
endif
conf.set('HAVE_LIBBACKTRACE', libbacktrace.found())

## Check for libunwind
libunwind = ( get_option('libunwind')
  ? dependency('libunwind', version: '>=1.1.0', required: false)
  : no_dep
)
conf.set('HAVE_LIBUNWIND', libunwind.found())


## Check for Dr. Mingw
drmingw = no_dep
if platform_windows
  exchndl = cc.find_library('exchndl', required: false)
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
conf.set('HAVE_X11_EXTENSIONS_SHAPE_H',
  x11_target and cc.has_header('X11/extensions/shape.h'))
conf.set('HAVE_X11_XMU_WINUTIL_H',
  x11_target and cc.has_header('X11/Xmu/WinUtil.h'))


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

liblzma_minver = '5.0.0'
liblzma = dependency('liblzma', version: '>='+liblzma_minver)


ghostscript = cc.find_library('gs', required: get_option('ghostscript'))
if ghostscript.found()
  MIMEtypes += 'application/postscript'
else
  ghostscript = disabler()
endif

libpng_minver = '1.6.25'
libpng = dependency('libpng', version: '>='+libpng_minver,
  required: get_option('png')
)
if libpng.found()
  MIMEtypes += [ 'image/png', 'image/x-icon']
endif


libmng = cc.find_library('mng', required: get_option('mng'))

libaa = cc.find_library('aa', required: get_option('aa'))

libxpm = dependency('xpm', required: get_option('xpm'))
if libxpm.found()
  MIMEtypes += 'image/x-xpixmap'
endif

openexr_minver = '1.6.1'
openexr = dependency('OpenEXR', version: '>='+openexr_minver,
  required: get_option('openexr')
)
if openexr.found()
  MIMEtypes += 'image/x-exr'
endif

webp_minver = '0.6.0'
webp_libs = [
  dependency('libwebp',     version: '>='+webp_minver, required: get_option('webp')),
  dependency('libwebpmux',  version: '>='+webp_minver, required: get_option('webp')),
  dependency('libwebpdemux',version: '>='+webp_minver, required: get_option('webp')),
]
webp_found = true
foreach lib : webp_libs
  webp_found = webp_found and  lib.found()
endforeach

if webp_found
  MIMEtypes += 'image/x-webp'
endif

libheif_minver = '1.3.2'
libheif = dependency('libheif', version: '>='+libheif_minver,
  required: get_option('heif')
)
conf.set('HAVE_LIBHEIF_1_4_0', libheif.version().version_compare('>=1.4.0'))
conf.set('HAVE_LIBHEIF_1_6_0', libheif.version().version_compare('>=1.6.0'))

if libheif.found()
  MIMEtypes += [
    'image/heif',
    'image/heic',
  ]
endif

libheif_warning=''
if libheif.version().version_compare('==1.5.0') or libheif.version().version_compare('==1.5.1')
  libheif_warning='''

        libheif version 1.5.0 and 1.5.1 are known to crash when
        exporting (bug #4185). Please update.
    '''
    warning(libheif_warning)
    warnings += libheif_warning
endif

have_vala = add_languages('vala', required: get_option('vala-plugins'))
if have_vala
  babl = declare_dependency(
    dependencies: [
      babl,
      meson.get_compiler('vala').find_library('babl-0.1'),
    ]
  )

  # TODO: remove this once we release 3.0
  valac = meson.get_compiler('vala')
  if valac.version().version_compare('>= 0.31.1')
    add_project_arguments('--disable-since-check', language: 'vala')
  endif
endif

webkit_minver = '2.20.3'
webkit = dependency('webkit2gtk-4.0', version: '>=' + webkit_minver,
  required: get_option('webkit')
)
conf.set('HAVE_WEBKIT', webkit.found())

poppler_minver = '0.69.0'
poppler_data_minver = '0.4.9'
if get_option('poppler')
  poppler = [
    dependency('poppler-glib', version: '>='+poppler_minver),
    dependency('poppler-data', version: '>='+poppler_data_minver),
  ]
else
  poppler = no_dep
endif

cairopdf_minver = '1.12.2'
cairopdf = dependency('cairo-pdf', version: '>='+cairopdf_minver,
  required: get_option('cairo-pdf')
)

# PDF import support is a granted feature.
MIMEtypes += 'application/pdf'

wmf_minver = '0.2.8'
wmf = dependency('libwmf', version: '>='+wmf_minver,
  required: get_option('wmf')
)
if wmf.found()
  MIMEtypes += 'image/x-wmf'
endif

openjpeg_minver = '2.1.0'
openjpeg = dependency('libopenjp2', version: '>='+openjpeg_minver,
  required: get_option('jpeg2000')
)
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
if directx_sdk_path != '' and platform_windows
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
    mail_warning = 'Sendmail specified but not found. It should be installed at runtime!'
    warning(mail_warning)
    warnings += mail_warning
  endif

  email_message = '@0@ (@1@)'.format(true, sendmail_path)
  conf.set_quoted('SENDMAIL', sendmail_path)
else
  xdg_email_path = 'xdg-email'
  xdg_email = find_program(xdg_email_path, required: false)
  if xdg_email.found()
    xdg_email_path = xdg_email.path()
  else
    mail_warning = 'Xdg-email not found, but required at runtime for email sending.'
    warning(mail_warning)
    warnings += mail_warning
  endif

  email_message = '@0@ (@1@)'.format(true, xdg_email_path)
endif


################################################################################
# ISO codes

isocodes = dependency('iso-codes', required: false)
if isocodes.found()
  isocodes_prefix = isocodes.get_pkgconfig_variable('prefix')
  isocodes_location = isocodes_prefix / get_option('datadir') / 'xml' / 'iso-codes'
  isocodes_localedir= isocodes_prefix / get_option('datadir') / 'locale'
endif
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

python3_minver = '>=3.6.0'

if get_option('python') == 'never'
  python_warning = '''
    You disabled the installation of core Python plug-ins. This is discouraged
    as it won't provide the full GIMP experience.
    Note that you may install the Python plug-ins even if you have no Python
    interpreter at build-time by passing the configure option -Dpython=always.
    Just make sure that a Python >=@0@ interpreter is available at run-time.
  '''.format(python3_minver)
  warning(python_warning)
  warnings += python_warning

  have_python = false

else
  always_install = get_option('python') == 'always'

  python = pythonmod.find_installation('python3', required: false)

  python_found = (
    python.found() and
    python.language_version().version_compare('>='+python3_minver)
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
    Python >=@0@ or PyGObject was not found.
    Note that you may install the Python plug-ins even if you have no Python
    interpreter at build-time by passing the configure option -Dpython=always.
    Just make sure that a Python >=@0@ interpreter is available at run-time.
    '''.format(python3_minver))
  endif

  if (not python_found) and (always_install)
    python_warning = '''
    Python >=@0@ or PyGObject was not found.
    Python plug-ins will be installed anyway but you should make sure that
    a compatible Python interpreter is available at installation, otherwise
    installed plug-ins won't be usable.
    '''.format(python3_minver)
    warning(python_warning)
    warnings += python_warning
  endif

  have_python = python_found or always_install
endif

## Javascript

if get_option('javascript') == 'never'
  js_warning = '''
    You disabled the installation of core Javascript plug-ins. This is
    discouraged as it won't provide the full GIMP experience.
    Note that you may install the Javascript plug-ins even if you have no GJS
    interpreter at build-time by passing the configure option -Djavascript=always.
    Just make sure that the GJS interpreter is available at run-time.
  '''
  warning(js_warning)
  warnings += js_warning
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
    js_warning = '''
    GJS was not found.
    JavaScript plug-ins will be installed anyway but you should make sure that
    the JavaScript interpreter GJS is available at installation, otherwise
    installed plug-ins won't be usable.
    '''
    warning(js_warning)
    warnings += js_warning
  endif

  have_javascript = gjs.found() or always_install
endif

## Lua

if get_option('lua') == 'never'
  lua_warning = '''
    You disabled the installation of core Lua plug-ins. This is discouraged
    as it won't provide the full GIMP experience.
    Note that you may install the Lua plug-ins even if you have no Lua LGI
    interpreter at build-time by passing the configure option -Dlua=always.
    Just make sure that the Lua LGI interpreter is available at run-time.
  '''
  warning(lua_warning)
  warnings += lua_warning
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
    lua_warning = '''
    Luajit was not found.
    Lua plug-ins will be installed anyway but you should make sure that
    luajit and LGI are available at installation, otherwise
    installed plug-ins won't be usable.
    '''
    warning(lua_warning)
    warnings += lua_warning
  endif

  have_lua = lua.found() or always_install
endif


# Check for GTK Mac Integration
if platform_osx and (gtk3.get_pkgconfig_variable('targets').contains('quartz'))
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
  if platform_windows
    vec_warning = '''
      You enabled vector icons on Win32. Make sure to run:
      $ gdk-pixbuf-query-loaders.exe --update-cache
      on the target machine (this command generates loaders.cache)
      so that GdkPixbuf knows where to find the SVG loader.
    '''
    warning(vec_warning)
    warnings += vec_warning
  else
    shared_mime_info = dependency('shared-mime-info')
  endif
endif

# Running tests headless
xvfb_run = find_program('xvfb-run', required: get_option('headless-tests'))
dbus_run_session = find_program('dbus-run-session', required: get_option('headless-tests'))
if xvfb_run.found() and dbus_run_session.found()
  add_test_setup('headless',
    exe_wrapper: find_program('build' / 'meson' / 'run_test_env.sh'),
    is_default: true,
  )
endif

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

# Build identifiers #

conf.set_quoted('GIMP_BUILD_ID', get_option('build-id'))
conf.set_quoted('GIMP_BUILD_PLATFORM', host_os)
if platform_linux
  conf.set_quoted('GIMP_BUILD_PLATFORM_FAMILY', 'linux')
elif platform_windows
  conf.set_quoted('GIMP_BUILD_PLATFORM_FAMILY', 'windows')
elif platform_osx
  conf.set_quoted('GIMP_BUILD_PLATFORM_FAMILY', 'macos')
else
  conf.set_quoted('GIMP_BUILD_PLATFORM_FAMILY', 'other')
endif

conf.set('CHECK_UPDATE', get_option('check-update'))

# Default ICC directory #

# This is necessary because some Unix systems may have a different
# standard path for color profiles. And in particular, sandbox builds
# might mount the host system at a different root. This is for
# instance the case of flatpak which mount the host root at /run/host/.

# if not (platform_osx or platform_windows)

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
meson_install_subdir= find_program('tools'/'meson_install_subdir.py')

gimp_mkenums        = find_program('tools'/'gimp-mkenums')
gimp_mkenums_custom_target_commonargs = [
  '--fprod',
    '/* enumerations from "@basename@" */',
  '--vhead',
    'GType\n'+
    '@enum_name@_get_type (void)\n'+
    '{\n'+
    '  static const G@Type@Value values[] =\n'+
    '  {',
  '--vprod',
    '    { @VALUENAME@, "@VALUENAME@", "@valuenick@" },',
  '--vtail',
    '    { 0, NULL, NULL }\n'+
    '  };\n',
  '--dhead',
    '  static const Gimp@Type@Desc descs[] =\n'+
    '  {',
  '--dprod',
    '    { @VALUENAME@, @valuedesc@, @valuehelp@ },'+
    '@if (\'@valueabbrev@\' ne \'NULL\')@\n'+
    '    /* Translators: this is an abbreviated version of @valueudesc@.\n'+
    '       Keep it short. */\n'+
    '    { @VALUENAME@, @valueabbrev@, NULL },'+
    '@endif@',
]

libgimp_mkenums_custom_target_commonargs = [
    gimp_mkenums_custom_target_commonargs,
    '--dtail',
      '    { 0, NULL, NULL }\n'+
      '  };\n'+
      '\n'+
      '  static GType type = 0;\n'+
      '\n'+
      '  if (G_UNLIKELY (! type))\n'+
      '    {\n'+
      '      type = g_@type@_register_static ("@EnumName@", values);\n'+
      '      gimp_type_set_translation_domain (type, GETTEXT_PACKAGE "-libgimp");\n'+
      '      gimp_type_set_translation_context (type, "@enumnick@");\n'+
      '      gimp_@type@_set_value_descriptions (type, descs);\n'+
      '    }\n'+
      '\n'+
      '  return type;\n'+
      '}\n',
]

conf.set('ENABLE_NLS',   true)
conf.set('HAVE_GETTEXT', true)

# localedir = get_option('prefix') / get_option('localedir')


################################################################################
# Miscellaneous configuration

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
    { 'm': 'HAVE_DCGETTEXT',                'v': 'dcgettext', },
    { 'm': 'HAVE_DIFFTIME',                 'v': 'difftime', },
    { 'm': 'HAVE_FSYNC',                    'v': 'fsync', },
    { 'm': 'HAVE_GETADDRINFO',              'v': 'getaddrinfo', },
    { 'm': 'HAVE_GETNAMEINFO',              'v': 'getnameinfo', },
    { 'm': 'HAVE_GETTEXT',                  'v': 'gettext', },
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
  if platform_windows
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
  not platform_windows
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

cc_version=''
if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  cc_cmd = run_command(cc, '-v')
  # Note: the call might actually fail when using ccache.
  # See: https://github.com/mesonbuild/meson/issues/6174
  if cc_cmd.returncode() == 0
    cc_version = cc_cmd.stdout() + cc_cmd.stderr()
  endif
else
  # Various compilers have various options. Try most common ones. This
  # list of options comes from autotools checks.
  foreach arg : [ '--version', '-v', '-V', '-qversion' ]
    cc_cmd = run_command(cc, arg)
    if cc_cmd.returncode() == 0
      cc_version = cc_cmd.stdout()
    endif
  endforeach
endif
if cc_version == ''
  # We didn't manage to get a meaningful verbose version from the
  # compiler. Just save its name and version.
  cc_version = cc.get_id() + ' ' + cc.version()
else
  # See also: https://github.com/mesonbuild/meson/pull/6179
  cc_version = '\\n'.join(cc_version.split('\n'))
endif
conf.set_quoted('CC_VERSION',     cc_version.strip())

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
if isocodes.found()
  conf.set_quoted('ISO_CODES_LOCATION',    isocodes_location)
  conf.set_quoted('ISO_CODES_LOCALEDIR',   isocodes_localedir)
endif


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


has_version_h = run_command('python3', '-c',
  'import sys,os; sys.exit(0 if os.path.exists("git-version.h") else 1)'
).returncode() == 0
if is_git_repository or not has_version_h
  gitversion_h1 = vcs_tag(
    input : 'app/git-version.h.in',
    output: 'git-version.h.in.1',
    command: [ 'git', 'describe', '--always', ],
    replace_string: '@GIMP_GIT_VERSION@',
    fallback: 'unknown (unsupported)',
  )
  gitversion_h2 = vcs_tag(
    input : gitversion_h1,
    output: 'git-version.h.in.2',
    command: [ 'git', 'rev-parse', '--short', 'HEAD', ],
    replace_string: '@GIMP_GIT_VERSION_ABBREV@',
    fallback: 'unknown (unsupported)',
  )
  gitversion_h = vcs_tag(
    input : gitversion_h2,
    output: 'git-version.h',
    command: [ 'git', 'log', '-n1', '--date=format:%Y', '--pretty=%cd', ],
    replace_string: '@GIMP_GIT_LAST_COMMIT_YEAR@',
    fallback: 'unknown (unsupported)',
  )

  git = find_program('git', required: false)
  if not is_git_repository or not git.found()
  # We create git-version.h but know it will be useless because we are
  # not in a git repository. Output a warning.
  git_warning = '''

        UNSUPPORTED BUILD!

        This is not a distribution tarball (git-version.h missing) and
        we could not establish the corresponding commit (either this is
        not a git repository or git command is missing). Therefore
        we have no reference for debugging.
        Please either use release tarballs or build from the repository.
  '''
  warning(git_warning)
  warnings += git_warning
  endif
else
  gitversion_h = files('git-version.h')
endif

install_conf = configuration_data()
install_conf.set('GIMP_APP_VERSION', gimp_app_version)
install_conf.set('GIMP_PKGCONFIG_VERSION', gimp_version)
install_conf.set('GIMP_VERSION', gimp_version)
install_conf.set('APPSTREAM_GLIB_REQUIRED_VERSION', appstream_glib_minver)
install_conf.set('ATK_REQUIRED_VERSION',          atk_minver)
install_conf.set('BABL_REQUIRED_VERSION',         babl_minver)
install_conf.set('CAIRO_PDF_REQUIRED_VERSION',    cairopdf_minver)
install_conf.set('CAIRO_REQUIRED_VERSION',        cairo_minver)
install_conf.set('FONTCONFIG_REQUIRED_VERSION',   fontconfig_minver)
install_conf.set('FREETYPE2_REQUIRED_VERSION',    freetype2_minver)
install_conf.set('GDK_PIXBUF_REQUIRED_VERSION',   gdk_pixbuf_minver)
install_conf.set('GEGL_REQUIRED_VERSION',         gegl_minver)
install_conf.set('GEXIV2_REQUIRED_VERSION',       gexiv2_minver)
install_conf.set('GLIB_REQUIRED_VERSION',         glib_minver)
install_conf.set('GTK_REQUIRED_VERSION',          gtk3_minver)
install_conf.set('HARFBUZZ_REQUIRED_VERSION',     harfbuzz_minver)
install_conf.set('INTLTOOL_REQUIRED_VERSION',     '0.40.1')
install_conf.set('LCMS_REQUIRED_VERSION',         lcms_minver)
install_conf.set('LIBHEIF_REQUIRED_VERSION',      libheif_minver)
install_conf.set('LIBLZMA_REQUIRED_VERSION',      liblzma_minver)
install_conf.set('LIBMYPAINT_REQUIRED_VERSION',   libmypaint_minver)
install_conf.set('LIBPNG_REQUIRED_VERSION',       libpng_minver)
install_conf.set('OPENEXR_REQUIRED_VERSION',      openexr_minver)
install_conf.set('OPENJPEG_REQUIRED_VERSION',     openjpeg_minver)
install_conf.set('PANGOCAIRO_REQUIRED_VERSION',   pangocairo_minver)
install_conf.set('POPPLER_DATA_REQUIRED_VERSION', poppler_data_minver)
install_conf.set('POPPLER_REQUIRED_VERSION',      poppler_minver)
install_conf.set('PYTHON3_REQUIRED_VERSION',      python3_minver)
install_conf.set('RSVG_REQUIRED_VERSION',         rsvg_minver)
install_conf.set('WEBKITGTK_REQUIRED_VERSION',    webkit_minver)
install_conf.set('WEBP_REQUIRED_VERSION',         webp_minver)
install_conf.set('WMF_REQUIRED_VERSION',          wmf_minver)
install_conf.set('XGETTEXT_REQUIRED_VERSION',     '0.19')

INSTALL = configure_file(
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
# Miscellaneous targets


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

meson.add_dist_script('meson_dist_script.sh')


################################################################################
# Subdirs


rootInclude = include_directories('.')
appInclude  = include_directories('app')

if platform_windows
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
# Subdir installations

foreach dir : [
    { 'dir': 'libgimp',        'deps': libgimp},
    { 'dir': 'libgimpbase',    'deps': libgimpbase},
    { 'dir': 'libgimpcolor',   'deps': libgimpcolor},
    { 'dir': 'libgimpconfig',  'deps': libgimpconfig},
    { 'dir': 'libgimpmath',    'deps': libgimpmath},
    { 'dir': 'libgimpmodule',  'deps': libgimpmodule},
    { 'dir': 'libgimpthumb',   'deps': libgimpthumb},
    { 'dir': 'libgimpwidgets', 'deps': libgimpwidgets},
    { 'dir': 'icons' },
    { 'dir': 'plug-ins', },
  ]
  run_target('install-' + dir.get('dir'),
    command: [
      meson_install_subdir, '-v', dir.get('dir')
    ],
    depends: dir.get('deps', [])
  )
endforeach

################################################################################

final_message = [
'''Extra Binaries:''',
'''  gimp-console:        @0@'''.format(enable_console_bin),
'',
'''Optional Features:''',
'''  Check updates at startup:  @0@'''.format(get_option('check-update')),
'''  Language selection:        @0@'''.format(isocodes.found()),
'''  Vector icons:              @0@'''.format(have_vector_icons),
'''  Dr. Mingw (Win32):         @0@'''.format(drmingw.found()),
'''  Relocatable Bundle:        @0@'''.format(relocatable_bundle),
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
'''  Heif:                @0@ [profile support: @1@@2@]'''
                                .format(libheif.found(),
                                        libheif.version().version_compare('>=1.4.0'),
                                       libheif_warning != '' ? ' (see warning below)' : ''),
'''  PDF (export):        @0@'''.format(cairopdf.found()),
'''  Print:               @0@'''.format(have_print),
'''  Python 3 plug-ins:   @0@'''.format(have_python),
'''  Javascript plug-ins: @0@'''.format(have_javascript),
'''  Lua plug-ins:        @0@'''.format(have_lua),
'''  TWAIN (Win32):       @0@'''.format(platform_windows),
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

if warnings.length() > 0
  warning('Warnings occurred during configuration')
  foreach warning : warnings
    warning(warning)
  endforeach
endif

warning('''

        MESON BUILD IS STILL EXPERIMENTAL!

        we encourage usage of meson if you want to test and
        report bugs. Yet if you are a packager, DO NOT USE
        meson until we fix all known issues and bugs.
        ''')
