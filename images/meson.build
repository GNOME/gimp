## Splash Image and Welcome Dialog ##

splash = custom_target('gimp-splash.png',
                       input : [ 'export-splash.py' ],
                       output: [ 'gimp-splash.png', ],
                       depends: [ gimp_exe_depends ],
                       command: [ gimp_exe, '-nidfs',
                                  files('gimp-splash.xcf.xz'),
                                  '--batch-interpreter', 'python-fu-eval',
                                  '-b', '-', '--quit'],
                       feed: true,
                       build_by_default: true,
                       env: gimp_run_env,
                       install_dir: gimpdatadir / 'images',
                       install: true)

## Windows Installer's Splash Image and Intro Banner ##

if get_option('windows-installer')
  installsplash = custom_target('installsplash.bmp',
                                input : [ 'splash2installer.py' ],
                                output: [ 'stamp-installsplash.bmp', ],
                                depends: [ gimp_exe_depends ],
                                command: [ gimp_exe, '-nidfs',
                                           files('gimp-splash.xcf.xz'),
                                           '--batch-interpreter', 'python-fu-eval',
                                           '-b', '-', '--quit'],
                                feed: true,
                                build_by_default: true,
                                env: gimp_run_env)
endif

subdir('logo')
