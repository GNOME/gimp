#!/usr/bin/env python3
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from glob import glob

# This script is used to create a GIMP bundle dir on Windows. A bundle
# is used as source of files for making both .exe installer and .msix package
if not os.getenv("MESON_BUILD_ROOT"):
  # Let's prevent contributors from creating broken bundles
  print("\033[31m(ERROR)\033[0m: Script called standalone. Please build GIMP targeting installer or msix creation.")
  sys.exit(1)


# Bundle deps and GIMP files
GIMP_SOURCE = Path(os.getenv("MESON_SOURCE_ROOT")).as_posix()

## System prefix: it is MSYSTEM_PREFIX
with open("meson-logs/meson-log.txt") as f:
  meson_log = f.read()
match = re.search(r"Main binary: (.*)\\bin\\python\.exe", meson_log)
MSYSTEM_PREFIX = match.group(1).replace("\\", "/")
## GIMP prefix: as set at meson configure time
GIMP_PREFIX = Path(os.getenv("MESON_INSTALL_DESTDIR_PREFIX")).as_posix()

## Bundle dir: we make a "perfect" bundle separated from GIMP_PREFIX
#NOTE: The bundling script need to set $MSYSTEM_PREFIX to our dist scripts
#fallback code be able to identify what arch they are distributing
GIMP_DISTRIB = Path(GIMP_SOURCE) / f"gimp-{Path(MSYSTEM_PREFIX).name}"

def bundle(src_root, pattern):
  ## Search for targets in search path
  src_root = Path(src_root)
  paths_to_bundle = list(src_root.glob(pattern))
  if not paths_to_bundle:
    print(f"\033[31m(ERROR)\033[0m: not found {src_root}/{pattern}")
    sys.exit(1)
  for src_path in paths_to_bundle:
    ## Copy found targets to bundle path
    dest_path = GIMP_DISTRIB / src_path.relative_to(src_root)
    dest_path.parent.mkdir(parents=True, exist_ok=True)
    print(f"Bundling {src_path} to {dest_path.parent}")
    if src_path.is_dir():
      shutil.copytree(src_path, dest_path, dirs_exist_ok=True)
    else:
      shutil.copy2(src_path, dest_path)
      ## Process .typelib dependencies
      if str(src_path).endswith(".typelib"):
        def process_typelib(path, typelib_list=None):
          cmd = ['g-ir-inspect', '--print-typelibs', os.path.basename(path).split('-')[0]]
          result = subprocess.run(cmd, capture_output=True, text=True)
          for line in result.stdout.splitlines():
            typelib = line.replace("typelib: ", "").strip()
            if typelib_list is None:
              typelib_list = set()
            if typelib not in typelib_list:
              typelib_list.add(typelib)
              typelib_path = Path(f"{MSYSTEM_PREFIX}/lib/girepository-1.0/{typelib}.typelib")
              if typelib_path.exists():
                shutil.copy2(typelib_path, dest_path.parent)
              process_typelib(typelib)
        process_typelib(src_path)

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
(GIMP_DISTRIB / ".gitignore").write_text("*\n")
### Add a wrapper at tree root, less messy than having to look for the
### binary inside bin/, in the middle of all the DLLs.
with open("config.h") as file:
  for line in file:
    match = re.match(r'^#define\s+(\S+)\s+(.*)', line)
    if match:
      key, value = match.groups()
      os.environ[key] = value.strip().strip('"').strip("'")
(GIMP_DISTRIB / "gimp.cmd").write_text(f"powershell bin\\gimp-{os.getenv('GIMP_MUTEX_VERSION')}.exe\n")


## BUNDLE BASE (BARE MINIMUM TO RUN GTK APPS).
### Needed to not pollute output. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/8877
bundle(MSYSTEM_PREFIX, "bin/gdbus.exe")
### Needed for hyperlink support etc... See: https://gitlab.gnome.org/GNOME/gimp/-/issues/12288
####...when running from `gimp*.exe --verboses`
bundle(MSYSTEM_PREFIX, "bin/gspawn*-console.exe")
if not os.getenv("GIMP_UNSTABLE") and os.getenv("GIMP_RELEASE"):
  ####...when running from `gimp*.exe`
  bundle(MSYSTEM_PREFIX, "bin/gspawn*-helper.exe")
### Needed for file dialogs (only .compiled file is needed on MS Windows)
bundle(MSYSTEM_PREFIX, "share/glib-*/schemas/gschemas.compiled")
### Needed to open remote files
bundle(MSYSTEM_PREFIX, "lib/gio")
### Needed to not crash UI. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/6165
bundle(MSYSTEM_PREFIX, "share/icons/Adwaita")
### Needed by GTK to use icon themes. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/5080
bundle(GIMP_PREFIX, "share/icons/hicolor")
### Needed to loading icons in GUI
bundle(MSYSTEM_PREFIX, "lib/gdk-pixbuf-*/*/loaders.cache")
bundle(MSYSTEM_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-png.dll")
bundle(MSYSTEM_PREFIX, "lib/gdk-pixbuf-*/*/loaders/pixbufloader_svg.dll")


## CORE FEATURES.
bundle(GIMP_PREFIX, "bin/libgimp*.dll")
bundle(GIMP_PREFIX, "lib/gimp")
bundle(GIMP_PREFIX, "lib/babl-*")
bundle(GIMP_PREFIX, "lib/gegl-*")
bundle(GIMP_PREFIX, "share/gimp")
lang_array = [Path(f).stem for f in glob(str(Path(GIMP_SOURCE)/"po/*.po"))]
for lang in lang_array:
  bundle(GIMP_PREFIX, f"share/locale/{lang}/LC_MESSAGES/*.mo")
  # Needed for eventually used widgets, GTK inspector etc
  if glob(f"{MSYSTEM_PREFIX}/share/locale/{lang}/LC_MESSAGES/gtk*.mo"):
    bundle(MSYSTEM_PREFIX, f"share/locale/{lang}/LC_MESSAGES/gtk*.mo")
  # For language list in text tool options
  if glob(f"{MSYSTEM_PREFIX}/share/locale/{lang}/LC_MESSAGES/iso_639_3.mo"):
    bundle(MSYSTEM_PREFIX, f"share/locale/{lang}/LC_MESSAGES/iso_639_3.mo")
bundle(GIMP_PREFIX, "etc/gimp")


## OTHER FEATURES AND PLUG-INS.
### mypaint brushes
bundle(MSYSTEM_PREFIX, "share/mypaint-data/2.0")
### Needed for 'th' word breaking in Text tool etc
bundle(MSYSTEM_PREFIX, "share/libthai")
### Needed for full CJK and Cyrillic support in file-pdf
bundle(MSYSTEM_PREFIX, "share/poppler")
### Needed for file-wmf work
bundle(MSYSTEM_PREFIX, "share/libwmf")
### Needed for 'Show image graph'.
### Note: we want the same test as around the global variable
### show_debug_menu in app/main.c
if (os.getenv("GIMP_UNSTABLE") or not os.getenv("GIMP_RELEASE")) and "32" not in MSYSTEM_PREFIX:
  #### See: https://gitlab.gnome.org/GNOME/gimp/-/issues/6045
  bundle(MSYSTEM_PREFIX, "bin/dot.exe")
  #### See: https://gitlab.gnome.org/GNOME/gimp/-/issues/12119
  bundle(MSYSTEM_PREFIX, "bin/libgvplugin_dot*.dll")
  bundle(MSYSTEM_PREFIX, "bin/libgvplugin_pango*.dll")
  bundle(MSYSTEM_PREFIX, "bin/config6")
### Binaries for GObject Introspection support. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/13170
bundle(GIMP_PREFIX, "lib/girepository-*/*.typelib")
bundle(MSYSTEM_PREFIX, "bin/libgirepository-*.dll")
#### Python support
#####python.exe is needed for plug-ins output in `gimp-console*.exe`
bundle(MSYSTEM_PREFIX, "bin/python.exe")
if not os.getenv("GIMP_UNSTABLE") and os.getenv("GIMP_RELEASE"):
  #####pythonw.exe is needed to run plug-ins silently in `gimp*.exe`
  bundle(MSYSTEM_PREFIX, "bin/pythonw.exe")
bundle(MSYSTEM_PREFIX, "lib/python*")
clean(GIMP_DISTRIB, "lib/python*/*.pyc")
#####Needed for internet connection on python. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/14722
bundle(MSYSTEM_PREFIX, "etc/ssl/cert.pem")
#### FIXME: luajit crashes at startup: See: https://gitlab.gnome.org/GNOME/gimp/-/issues/11597
#bundle(MSYSTEM_PREFIX, "bin/luajit.exe")
#bundle(MSYSTEM_PREFIX, "lib/lua")
#bundle(MSYSTEM_PREFIX, "share/lua")
### Support for legacy Win clipboard images: https://gitlab.gnome.org/GNOME/gimp/-/issues/4802
bundle(MSYSTEM_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-bmp.dll")
### Support for non .PAT patterns: https://gitlab.gnome.org/GNOME/gimp/-/issues/12351
bundle(MSYSTEM_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-jpeg.dll")
bundle(MSYSTEM_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-gif.dll")
bundle(MSYSTEM_PREFIX, "lib/gdk-pixbuf-*/*/loaders/libpixbufloader-tiff.dll")
### Needed for fontconfig
bundle(MSYSTEM_PREFIX, "etc/fonts")
#### Avoid other programs breaking the cache. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/1366
fonts_conf = GIMP_DISTRIB / "etc/fonts/fonts.conf"
text = fonts_conf.read_text()
new_text = text.replace(
  "LOCAL_APPDATA_FONTCONFIG_CACHE",
  f"~/AppData/Local/GIMP/{os.getenv('GIMP_APP_VERSION')}/fontconfig/cache"
)
fonts_conf.write_text(new_text)


## MAIN EXECUTABLES AND DEPENDENCIES
### Minimal (and some additional) executables for the 'bin' folder
bundle(GIMP_PREFIX, "bin/gimp*.exe")
### Bundled just to promote GEGL. See: https://gitlab.gnome.org/GNOME/gimp/-/issues/10580
bundle(GIMP_PREFIX, "bin/gegl.exe")
### We save the list of already copied DLLs to keep a state between 2_bundle-gimp-uni_dep runs.
done_dll = Path(f"{os.getenv('MESON_BUILD_ROOT')}/done-dll.list")
done_dll.unlink(missing_ok=True)
### Deps (DLLs) of the binaries in 'bin' and 'lib' dirs
for dir in ["bin", "lib"]:
  search_dir = GIMP_DISTRIB / dir
  print(f"Searching for dependencies of {search_dir} in {GIMP_PREFIX} and {MSYSTEM_PREFIX}")
  for ext in ("*.dll", "*.exe"):
    for dep in search_dir.rglob(ext):
      subprocess.run([
        sys.executable, f"{GIMP_SOURCE}/build/windows/2_bundle-gimp-uni_dep.py",
        str(dep), f"{GIMP_PREFIX}/", f"{MSYSTEM_PREFIX}/",
        str(GIMP_DISTRIB), "--output-dll-list", done_dll.as_posix()
      ], check=True)


## .PDB/CODEVIEW DEBUG SYMBOLS (from babl, gegl and GIMP binaries)
if "32" not in MSYSTEM_PREFIX:
  bundle(GIMP_PREFIX, "bin/*.pdb")
### Remove .pdb without corresponding binaries (depends on what was choosen to be bundled above)
files = os.listdir(GIMP_DISTRIB / "bin")
binaries = {os.path.splitext(file)[0] for file in files if file.endswith(('.exe', '.dll'))}
for file in files:
  if file.endswith('.pdb'):
    if os.path.splitext(file)[0] not in binaries:
      print(f"(INFO): removing orphan {file}")
      os.remove(os.path.join(GIMP_DISTRIB / "bin", file))
### Remove ancient COFF symbol table (not used for debugging) from MSYS2 binaries
for bin_path in GIMP_DISTRIB.rglob("*"):
  if bin_path.suffix.lower() in (".dll", ".exe", ".pyd"):
    with open(bin_path, "rb") as f:
      if b"RSDS" not in f.read():
        print(f"(INFO): stripping COFF symbols from {bin_path.name}")
        try:
          subprocess.run(["strip", str(bin_path)], check=True)
        except:
          continue


## DEVELOPMENT FILES (to build GEGL filters and GIMP plug-ins).
clean(GIMP_DISTRIB, "lib/*.a")
bundle(GIMP_PREFIX, "include/gimp-*")
bundle(GIMP_PREFIX, "include/babl-*")
bundle(GIMP_PREFIX, "include/gegl-*")
bundle(GIMP_PREFIX, "lib/libgimp*.a")
bundle(GIMP_PREFIX, "lib/libbabl*.a")
bundle(GIMP_PREFIX, "lib/libgegl*.a")
bundle(GIMP_PREFIX, "lib/pkgconfig/gimp*")
bundle(GIMP_PREFIX, "lib/pkgconfig/babl*")
bundle(GIMP_PREFIX, "lib/pkgconfig/gegl*")
