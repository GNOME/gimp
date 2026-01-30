#!/usr/bin/env python3
import os
import platform
import re
import shutil
import subprocess
import stat
import sys
from pathlib import Path
from glob import glob

# This script is used to create a GIMP .app bundle on macOS. A bundle
# is used as source of files for making the .dmg installer
if not os.getenv("MESON_BUILD_ROOT"):
  # Let's prevent contributors from creating broken bundles
  print("\033[31m(ERROR)\033[0m: Script called standalone. Please build GIMP targeting DMG installer creation.")
  sys.exit(1)
# Get variables from MESON_BUILD_ROOT/config.h that can be used on this script
with open("config.h") as file:
  for line in file:
    match = re.match(r'^#\s*define\s+(\S+)(?:\s+(.*))?$', line)
    if match:
      key, value = match.groups()
      if value is None or not value.strip():
        value = "1" #needed when there is no explicit value
      else:
        value = value.strip().strip('"').strip("'")
      os.environ[key] = value
if not os.getenv("ENABLE_RELOCATABLE_RESOURCES"):
  print("\n\033[31m(ERROR)\033[0m: No relocatable GIMP build found. You can build GIMP with '-Drelocatable-bundle=yes' to make a build suitable for .app creation.")
  sys.exit(1)


# Bundle deps and GIMP files
GIMP_SOURCE = Path(os.getenv("MESON_SOURCE_ROOT"))

## System prefix: it is OPT_PREFIX (see 1_build-deps-macports)
OPT_PREFIX = Path(os.getenv("OPT_PREFIX"))
## GIMP prefix: as set at meson configure time
GIMP_PREFIX = Path(os.getenv("MESON_INSTALL_DESTDIR_PREFIX"))

## Bundle dir: we make a "perfect" bundle separated from GIMP_PREFIX
#NOTE: The bundling script need to set $OPT_PREFIX to our dist scripts
#fallback code be able to identify what arch they are distributing
GIMP_DISTRIB = Path(GIMP_SOURCE) / f"gimp-{platform.machine()}.app" / "Contents"

def bundle(src_root, pattern, option="None", override=None):
  ## Search for targets in search path
  src_root = Path(src_root)
  paths_to_bundle = list(src_root.glob(pattern))
  if not paths_to_bundle:
    print(f"\033[31m(ERROR)\033[0m: not found {src_root}/{pattern}")
    sys.exit(1)
  for src_path in paths_to_bundle:
    ## Copy found targets to bundle path
    symlink_cleanup = True
    if "--dest" in option:
      dest_path = GIMP_DISTRIB / Path(override) / src_path.name
    elif "--rename" in option:
      dest_path = GIMP_DISTRIB / Path(override)
    elif "bin/" in pattern:
      symlink_cleanup = False
      dest_path = GIMP_DISTRIB / "MacOS" / Path(src_path.relative_to(src_root)).name
    elif "lib/" in pattern:
      dest_path = GIMP_DISTRIB / "Frameworks" / src_path.relative_to(src_root / "lib")
    elif "share/" in pattern:
      dest_path = GIMP_DISTRIB / "Resources" / src_path.relative_to(src_root / "share")
    elif "etc/" in pattern:
      dest_path = GIMP_DISTRIB / "SharedSupport" / src_path.relative_to(src_root / "etc")
    dest_path.parent.mkdir(parents=True, exist_ok=True)
    print(f"Bundling {src_path} to {dest_path.parent}")
    if src_path.is_dir():
      try:
        shutil.copytree(src_path, dest_path, dirs_exist_ok=True)
      except shutil.Error as e:
        print(f"\033[33m(WARNING)\033[0m: {dest_path} seems to already exist and have permission problems")
    else:
      if not str(src_path).endswith(".typelib"):
        try:
          shutil.copy2(src_path, dest_path, follow_symlinks=symlink_cleanup)
        except shutil.Error as e:
          print(f"\033[33m(WARNING)\033[0m: {dest_path} seems to already exist and have permission problems")
        if "MacOS/" in str(dest_path) and not dest_path.is_symlink():
          os.chmod(dest_path, 0o755)
      else:
        ## Process .typelib dependencies (as relocatable)
        tmp_gir_dir = GIMP_DISTRIB / "tmp"
        tmp_gir_dir.mkdir(parents=True, exist_ok=True)
        def set_typelib_rpath(typelib, prefix):
          typelib_path = Path(f"{prefix}/lib/girepository-1.0/{typelib}.typelib")
          target_path = dest_path.parent / typelib_path.name
          if typelib_path.exists() and not target_path.exists():
            if typelib_path != src_path:
              print(f"Bundling {typelib_path} to {dest_path.parent}")
            tmp_gir_path = Path(f"{tmp_gir_dir}/{typelib}.gir")
            shutil.copy2(Path(f"{prefix}/share/gir-1.0/{typelib}.gir"), tmp_gir_path)
            text = tmp_gir_path.read_text()
            text = re.sub(r'shared-library="([^"]+)"', lambda m: 'shared-library="' + ",".join("@rpath/" + os.path.basename(p) for p in m.group(1).split(",")) + '"', text)
            tmp_gir_path.write_text(text)
            subprocess.run(["g-ir-compiler", f"--includedir={tmp_gir_dir}", str(tmp_gir_path), "-o", target_path], check=True)
        def process_typelib(path, typelib_list=None):
          set_typelib_rpath(Path(path).stem, GIMP_PREFIX)
          if typelib_list is None:
            typelib_list = set()
          cmd = ['g-ir-inspect', '--print-typelibs', os.path.basename(path).split('-')[0]]
          result = subprocess.run(cmd, capture_output=True, text=True)
          for line in result.stdout.splitlines():
            typelib = line.replace("typelib: ", "").strip()
            if typelib and typelib not in typelib_list:
              typelib_list.add(typelib)
              for prefix in [GIMP_PREFIX, OPT_PREFIX]:
                typelib_path = Path(f"{prefix}/lib/girepository-1.0/{typelib}.typelib")
                target_path = dest_path.parent / typelib_path.name
                if typelib_path.exists() and not target_path.exists():
                  set_typelib_rpath(typelib, prefix)
                process_typelib(typelib, typelib_list)
        process_typelib(src_path)
        shutil.rmtree(tmp_gir_dir)

def clean(base_path, pattern):
  base_path = Path(base_path)
  first_found = False
  for parent_path in base_path.glob(os.path.dirname(pattern)):
    for path in parent_path.rglob(os.path.basename(pattern)):
      if path.exists():
        if not first_found:
           print(f"Cleaning {base_path}/{pattern}")
           first_found = True
        if path.is_dir():
          shutil.rmtree(path)
        else:
          path.unlink()


## PREPARE BUNDLE
GIMP_DISTRIB.mkdir(parents=True, exist_ok=True)
### Prevent Git going crazy
(GIMP_DISTRIB / ".." / ".gitignore").write_text("*\n")
### Info.plist (it will be configured by 3_dist-gimp-apple script)
shutil.copy2(Path(f"{GIMP_SOURCE}/build/macos/Info.plist"), GIMP_DISTRIB)
### FIXME: Icon (generate Assets.car for Liquid Glass: https://gitlab.gnome.org/Infrastructure/Infrastructure/-/issues/2159)
(GIMP_DISTRIB / "Resources").mkdir(parents=True, exist_ok=True)
#subprocess.run(["xcrun","actool",str(Path(f"{os.getenv('MESON_BUILD_ROOT')}/gimp-data/images/logo/AppIcon.icon")),"--output-format","human-readable-text","--compile",f"{GIMP_DISTRIB / 'Resources'}","--include-all-app-icons","--enable-on-demand-resources","NO","--enable-icon-stack-fallback-generation","NO","--development-region","en","--target-device","mac","--platform","macosx","--minimum-deployment-target",os.getenv('MACOSX_DEPLOYMENT_TARGET')], check=True)
shutil.copy2(Path(f"{os.getenv('MESON_BUILD_ROOT')}/gimp-data/images/logo/gimp.icns"), GIMP_DISTRIB / "Resources/AppIcon.icns")
shutil.copy2(Path(f"{os.getenv('MESON_BUILD_ROOT')}/build/macos/fileicon-xcf.icns"), GIMP_DISTRIB / "Resources/fileicon-xcf.icns")
shutil.copy2(Path(f"{os.getenv('MESON_BUILD_ROOT')}/build/macos/fileicon.icns"), GIMP_DISTRIB / "Resources/fileicon.icns")


## BUNDLE BASE (BARE MINIMUM TO RUN GTK APPS).
### FIXME: Needed for 'Send to Email' support (should be on sendmail.c source)
bundle(GIMP_SOURCE, "build/macos/patches/xdg-email", "--dest", "MacOS")
### Needed for file dialogs (only .compiled file is needed on macOS)
bundle(OPT_PREFIX, "share/glib-*/schemas/gschemas.compiled")
### Mostly bogus since we do use macOS API directly from gimp to open remote files
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "lib/libproxy/libpxbackend*.dylib", "--dest", "Frameworks")
bundle(OPT_PREFIX, "lib/gio")
### Needed to not crash UI. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/6165
bundle(OPT_PREFIX, "share/icons/Adwaita")
### Needed by GTK to use icon themes. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/5080
bundle(GIMP_PREFIX, "share/icons/hicolor")
### Needed to loading icons in GUI
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader*svg*")
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders.cache")
loaders_cache = glob(f"{GIMP_DISTRIB}/Frameworks/gdk-pixbuf-*/*/loaders.cache")
text = Path(loaders_cache[0]).read_text()
new_text = text.replace(f"{OPT_PREFIX}/lib/", "")
Path(loaders_cache[0]).write_text(new_text)
### Needed for printing support
bundle(OPT_PREFIX, "lib/gtk-3.0/*.*.*/printbackends/*.so")
### Needed for macOS emoji keyboard support (with im-quartz.so)
bundle(OPT_PREFIX, "lib/gtk-3.0/*.*.*/immodules/*.so")
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "etc/gtk-3.0/gtk.immodules", "--rename", "Frameworks/gtk-3.0/3.0.0/immodules.cache")
else: #os.path.exists(OPT_PREFIX / "bin/brew"):
  bundle(OPT_PREFIX, "lib/gtk-3.0/*.*.*/immodules.cache")
im_cache = glob(f"{GIMP_DISTRIB}/Frameworks/gtk-3.0/*.*.*/immodules.cache")
text = Path(im_cache[0]).read_text()
new_text = re.sub(r'/.*?(?=gtk-3\.0/)', '', text)
Path(im_cache[0]).write_text(new_text)
### Needed for MacOS-style keyboard shortcuts
bundle(OPT_PREFIX, "share/themes/Mac")


## CORE FEATURES.
bundle(GIMP_PREFIX, "lib/libbabl*-*.*.*.dylib")
bundle(GIMP_PREFIX, "lib/babl-*")
bundle(GIMP_PREFIX, "lib/libgegl*-*.*.*.dylib")
bundle(GIMP_PREFIX, "lib/gegl-*")
bundle(GIMP_PREFIX, "lib/libgimp*-*.*.*.dylib")
bundle(GIMP_PREFIX, "lib/gimp")
bundle(GIMP_PREFIX, "share/gimp")
lang_array = [Path(f).stem for f in glob(str(Path(GIMP_SOURCE)/"po/*.po"))]
for lang in lang_array:
  bundle(GIMP_PREFIX, f"share/locale/{lang}/LC_MESSAGES/*.mo")
  # Needed for eventually used widgets, GTK inspector etc
  if glob(f"{OPT_PREFIX}/share/locale/{lang}/LC_MESSAGES/gtk*.mo"):
    bundle(OPT_PREFIX, f"share/locale/{lang}/LC_MESSAGES/gtk*.mo")
  # FIXME: For language list in text tool options (not working)
  if glob(f"{OPT_PREFIX}/share/locale/{lang}/LC_MESSAGES/iso_639_3.mo"):
    bundle(OPT_PREFIX, f"share/locale/{lang}/LC_MESSAGES/iso_639_3.mo")
bundle(GIMP_PREFIX, "etc/gimp")


## OTHER FEATURES AND PLUG-INS.
### Support for non .PAT patterns: https://gitlab.gnome.org/GNOME/gimp/-/issues/12351
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-bmp*")
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-gif*")
bundle(OPT_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-tiff*")
### FIXME: mypaint brushes (needs patching https://github.com/Homebrew/homebrew-core/pull/262039)
bundle(OPT_PREFIX, "share/mypaint-data/2.0")
### Needed for fontconfig
bundle(OPT_PREFIX, "etc/fonts")
#### Avoid writing in the system and avoid other programs breaking the cache
fonts_conf = GIMP_DISTRIB / "SharedSupport/fonts/fonts.conf"
text = fonts_conf.read_text()
new_text = text.replace(
  f"{os.getenv('OPT_PREFIX')}/var",
  f"~/Library/Application Support/GIMP/{os.getenv('GIMP_APP_VERSION')}"
)
fonts_conf.write_text(new_text)
### Needed for 'th' word breaking in Text tool etc
bundle(OPT_PREFIX, "share/libthai")
### Needed for full CJK and Cyrillic support in file-pdf
bundle(OPT_PREFIX, "share/poppler")
#### Needed for signature support in file-pdf lib
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "lib/nss/libssl3.dylib", "--dest", "Frameworks")
  bundle(OPT_PREFIX, "lib/nss/libsmime3.dylib", "--dest", "Frameworks")
  bundle(OPT_PREFIX, "lib/nss/libnssutil3.dylib", "--dest", "Frameworks")
  bundle(OPT_PREFIX, "lib/nss/libnss3.dylib", "--dest", "Frameworks")
  bundle(OPT_PREFIX, "lib/nspr/*.dylib", "--dest", "Frameworks")
### Needed for file-ps work
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "share/ghostscript/1*/iccprofiles/*.icc", "--dest", "Resources/ghostscript/iccprofiles")
  bundle(OPT_PREFIX, "share/ghostscript/1*/Resource/Init/*", "--dest", "Resources/ghostscript/Resource/Init")
  bundle(OPT_PREFIX, "share/ghostscript/1*/Resource/Font/*", "--dest", "Resources/ghostscript/Resource/Font")
else: #os.path.exists(OPT_PREFIX / "bin/brew"):
  bundle(OPT_PREFIX, "share/ghostscript/iccprofiles/*.icc")
  bundle(OPT_PREFIX, "share/ghostscript/Resource/Init")
  bundle(OPT_PREFIX, "share/ghostscript/Resource/Font")
### Needed for file-wmf work
if os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, "share/fonts/libwmf/*", "--dest", "Resources/libwmf/fonts")
else: #os.path.exists(OPT_PREFIX / "bin/brew"):
  bundle(OPT_PREFIX, "Cellar/libwmf/*/share/libwmf/fonts/*", "--dest", "Resources/libwmf/fonts")
### Needed for 'Show image graph'.
#### See: https://gitlab.gnome.org/GNOME/gimp/-/issues/6045
bundle(OPT_PREFIX, "bin/dot", "--dest", "MacOS")
#### See: https://gitlab.gnome.org/GNOME/gimp/-/issues/12119
bundle(OPT_PREFIX, "lib/graphviz/libgvplugin_dot*.dylib")
bundle(OPT_PREFIX, "lib/graphviz/libgvplugin_pango*.dylib")
bundle(OPT_PREFIX, "lib/graphviz/config*")
### Binaries for GObject Introspection support. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/13170
bundle(GIMP_PREFIX, "lib/girepository-*/*.typelib")
bundle(OPT_PREFIX, "lib/libgirepository-*.dylib")
#### Python support
bundle(OPT_PREFIX, f"bin/python{os.getenv('PYTHON_VERSION')}", "--rename", "MacOS/python3")
if os.path.exists(OPT_PREFIX / "bin/brew") or (os.path.exists(OPT_PREFIX / "bin/port") and os.getenv('GITLAB_CI')):
  bundle(OPT_PREFIX, f"Frameworks/Python.framework/Versions/{os.getenv('PYTHON_VERSION')}", "--dest", "Frameworks/Python.framework/Versions")
elif os.path.exists(OPT_PREFIX / "bin/port"):
  bundle(OPT_PREFIX, f"Library/Frameworks/Python.framework/Versions/{os.getenv('PYTHON_VERSION')}", "--dest", "Frameworks/Python.framework/Versions")
bundle(OPT_PREFIX, f"lib/python{os.getenv('PYTHON_VERSION')}/site-packages/*", "--dest", f"Frameworks/Python.framework/Versions/{os.getenv('PYTHON_VERSION')}/lib/python{os.getenv('PYTHON_VERSION')}/site-packages")
clean(GIMP_DISTRIB, "Frameworks/Python.framework/*.pyc")
#####Needed for internet connection on python. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/14722
pythonpath = Path(f"{GIMP_DISTRIB}/Frameworks/Python.framework/Versions/{os.getenv('PYTHON_VERSION')}/lib/python{os.getenv('PYTHON_VERSION')}")
for d in (pythonpath, pythonpath / "site-packages"):
  sitecustomize = d / "sitecustomize.py"
  code = """\nimport os\nimport certifi\n\n# Only set if not already configured by user\nif not os.environ.get('SSL_CERT_FILE'):\n    os.environ['SSL_CERT_FILE'] = certifi.where()\n"""
  if sitecustomize.exists() and code.strip() not in sitecustomize.read_text():
    sitecustomize.write_text(sitecustomize.read_text() + code)
  elif not sitecustomize.exists():
    sitecustomize.write_text(code)
#####Needed since we use [[NSBundle mainBundle] bundlePath] on libgimpbase/gimpenv.c
real_path = Path(f"{GIMP_DISTRIB}/Resources/icons")
link_path = Path(f"{GIMP_DISTRIB}/Frameworks/Python.framework/Versions/{os.getenv('PYTHON_VERSION')}/Resources/Python.app/Contents/Resources/icons")
link_path.symlink_to(os.path.relpath(real_path, link_path.parent))
#### lua is buggy, and hard to bundle due to LUA_*PATH etc (see AppImage script)
#if os.path.exists(OPT_PREFIX / "bin/port"):
  #bundle(OPT_PREFIX, "bin/luajit", "--dest", "MacOS")
  #bundle(OPT_PREFIX, "lib/lua")
  #bundle(OPT_PREFIX, "share/lua")


## MAIN EXECUTABLES AND DEPENDENCIES
### Minimal (and some additional) executables for the 'MacOS' folder
bundle(GIMP_PREFIX, "bin/gimp*")
if os.path.exists(OPT_PREFIX / "bin/brew"):
  bundle(OPT_PREFIX, "Cellar/libarchive/*/lib/libarchive.*.dylib", "--dest", "Frameworks")
### Bundled just to promote GEGL. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10580
bundle(GIMP_PREFIX, "bin/gegl")
### Deps (DYLIBs) of the binaries in 'MacOS' and 'Frameworks' dirs
### We save the list of already copied DLLs to keep a state between 2_bundle-gimp-uni_dep runs.
done_dylib = Path(f"{os.getenv('MESON_BUILD_ROOT')}/done-dylib.list")
done_dylib.unlink(missing_ok=True)
for dir in ["MacOS", "Frameworks"]:
  search_dir = GIMP_DISTRIB / dir
  print(f"Searching for dependencies of {search_dir} in {GIMP_PREFIX} and {OPT_PREFIX}")
  for dep in search_dir.rglob("*"):
    if "Mach-O" in subprocess.run(["file", str(dep)], capture_output=True, text=True).stdout and ".dSYM" not in str(dep):
      subprocess.run([
        sys.executable, f"{GIMP_SOURCE}/tools/lib_bundle.py",
        str(dep), f"{GIMP_PREFIX}/", f"{OPT_PREFIX}/",
        str(GIMP_DISTRIB), "--output-dll-list", done_dylib
      ], check=True)


## .DSYM/DWARF DEBUG SYMBOLS (from babl, gegl and GIMP binaries)
for dir in ["MacOS", "Frameworks"]:
  search_dir = GIMP_DISTRIB / dir
  for binary in search_dir.rglob("*"):
    if "Mach-O" in subprocess.run(["file", str(binary)], capture_output=True, text=True).stdout and ".dSYM" not in str(binary) and not binary.is_symlink():
      result = subprocess.run(["dsymutil", "--no-output", binary], capture_output=True, text=True)
      if not "no debug symbols" in result.stdout + result.stderr and not "unable to open object file" in result.stdout + result.stderr:
        print(f"(INFO): generating debug symbols file as {binary}.dSYM")
        try:
          subprocess.run(["dsymutil", binary, "-o", f"{binary}.dSYM"], check=True, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError as e:
          sys.stderr.write(f"Failed to generate debug symbols from {binary}: {e}\n")
      elif "unable to open object file" in result.stdout + result.stderr and not os.getenv("OPT_PREFIX") in result.stdout + result.stderr and "Python.framework" not in str(binary):
        print(f"\n\033[31m(ERROR)\033[0m: {binary} is orphaned from .o file for .dSYM generation. Please make sure its build dir is present")
        sys.exit(1)


## DEVELOPMENT FILES ON UNIX-STYLE, NO MACOS-STYLE/.FRAMEWORK
## (to build GEGL filters and GIMP plug-ins).
clean(GIMP_DISTRIB, "Frameworks/*.a")
bundle(GIMP_PREFIX, "include/gimp-*", "--dest", "include")
bundle(GIMP_PREFIX, "include/babl-*", "--dest", "include")
bundle(GIMP_PREFIX, "include/gegl-*", "--dest", "include")
bundle(GIMP_PREFIX, "lib/pkgconfig/gimp*")
bundle(GIMP_PREFIX, "lib/pkgconfig/babl*")
bundle(GIMP_PREFIX, "lib/pkgconfig/gegl*")
real_bin_path = Path(f"{GIMP_DISTRIB}/MacOS")
link_bin_path = Path(f"{GIMP_DISTRIB}/bin")
link_bin_path.symlink_to(os.path.relpath(real_bin_path, link_bin_path.parent))
real_lib_path = Path(f"{GIMP_DISTRIB}/Frameworks")
link_lib_path = Path(f"{GIMP_DISTRIB}/lib")
link_lib_path.symlink_to(os.path.relpath(real_lib_path, link_lib_path.parent))
real_share_path = Path(f"{GIMP_DISTRIB}/Resources")
link_share_path = Path(f"{GIMP_DISTRIB}/share")
link_share_path.symlink_to(os.path.relpath(real_share_path, link_share_path.parent))
real_etc_path = Path(f"{GIMP_DISTRIB}/SharedSupport")
link_etc_path = Path(f"{GIMP_DISTRIB}/etc")
link_etc_path.symlink_to(os.path.relpath(real_etc_path, link_etc_path.parent))
