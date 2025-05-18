if platform_windows or platform_osx
  gimp_debug_tool_dir = get_option('bindir')
else
  gimp_debug_tool_dir = get_option('libexecdir')
endif


gimp_debug_tool = executable('gimp-debug-tool' + exec_ver,
  'gimp-debug-tool.c',
  include_directories: rootInclude,
  dependencies: [
    fontconfig,
    gio,
    gegl,
    gtk3,
  ],
  link_with: [
    libapp,
    libappwidgets,
    libgimpbase,
  ],
  install: true,
  install_dir: gimp_debug_tool_dir
)

if enable_default_bin
  if not platform_windows
    install_symlink(fs.name(gimp_debug_tool.full_path()).replace(exec_ver, ''),
                    pointing_to: fs.name(gimp_debug_tool.full_path()),
                    install_dir: gimp_debug_tool_dir)
    install_symlink(fs.name(gimp_debug_tool.full_path()).replace(exec_ver, '-@0@'.format(api_version_major)),
                    pointing_to: fs.name(gimp_debug_tool.full_path()),
                    install_dir: gimp_debug_tool_dir)
  else
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2]))',
                             gimp_debug_tool.full_path(), fs.name(gimp_debug_tool.name()).replace(exec_ver, '.exe'))
    meson.add_install_script(python, '-c', 'from shutil import copy2; from pathlib import Path; from sys import argv; import os; copy2(argv[1], str(Path(os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")) / "bin" / argv[2].replace("@0@", argv[3])))',
                             gimp_debug_tool.full_path(), fs.name(gimp_debug_tool.name()).replace(exec_ver, '-@0@.exe'), api_version_major.to_string())
  endif
endif
