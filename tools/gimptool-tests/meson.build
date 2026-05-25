#These source files are used by the AppImage, Windows and macOS bundling scripts
#(at build/ subdirs) after making GIMP SDK

#https://developer.gimp.org/resource/writing-a-plug-in/tutorial-c-basic/
gimptool_libgimp_test = configure_file(
    input: 'c-hello-world.c',
    output: 'c-hello-world.c',
    copy: true
  )

#https://developer.gimp.org/resource/writing-a-plug-in/tutorial-c-gui/
gimptool_libgimpui_test = configure_file(
    input: 'c-hello-world-ui.c',
    output: 'c-hello-world-ui.c',
    copy: true
  )

#https://developer.gimp.org/resource/writing-a-filter/meta-operation/
gimptool_libgegl_test = configure_file(
    input: 'gimp-tutorial-meta-op.c',
    output: 'gimp-tutorial-meta-op.c',
    copy: true
  )
