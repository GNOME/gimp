#ifndef __GIMP_CONFIG_BLURBS_H__
#define __GIMP_CONFIG_BLURBS_H__


#define TEMP_PATH_BLURB \
"Set the temporary storage directory. Files will appear here " \
"during the course of running the gimp.  Most files will disappear " \
"when the gimp exits, but some files are likely to remain, " \
"such as working palette files, so it is best if this directory " \
"not be one that is shared by other users or is cleared on machine " \
"reboot such as /tmp."

#define SWAP_PATH_BLURB \
"Set the swap file location. The gimp uses a tile based memory " \
"allocation scheme. The swap file is used to quickly and easily " \
"swap tiles out to disk and back in. Be aware that the swap file " \
"can easily get very large if the gimp is used with large images. " \
"Also, things can get horribly slow if the swap file is created on " \
"a directory that is mounted over NFS.  For these reasons, it may " \
"be desirable to put your swap file in \"/tmp\"."

#define BRUSH_PATH_BLURB \
"Set the brush search path."

#define PATTERN_PATH_BLURB \
"Set the pattern search path."

#define PLUG_IN_PATH_BLURB \
"Set the plug-in search path."

#define PALETTE_PATH_BLURB \
"Set the palette search path."

#define GRADIENT_PATH_BLURB \
"Set the gradient search path."

#define MODULE_PATH_BLURB \
"Set the module search path."

#define DEFAULT_BRUSH_BLURB \
"Specify a default brush.  The brush is searched for in the " \
"specified brush path."

#define DEFAULT_GRADIENT_BLURB \
"Specify a default gradient.  The gradient is searched for in the " \
"specified gradient path."

#define DEFAULT_PATTERN_BLURB \
"Specify a default pattern. The pattern is searched for in the " \
"specified pattern path."

#define DEFAULT_PALETTE_BLURB \
"Specify a default palette.  The palette is searched for in the " \
"specified palette path."

#define GAMMA_CORRECTION_BLURB \
"Set the gamma correction value for the display. 1.0 corresponds to no " \
"gamma correction.  For most displays, gamma correction should be set " \
"to between 2.0 and 2.6. One important thing to keep in mind: Many images "\
"that you might get from outside sources will in all likelihood already "\
"be gamma-corrected.  In these cases, the image will look washed-out if "\
"the gimp has gamma-correction turned on.  If you are going to work with "\
"images of this sort, turn gamma correction off by setting the value to 1.0."

#define INSTALL_COLORMAP_BLURB \
"Install a private colormap; might be useful on pseudocolor visuals."

#define TILE_CACHE_SIZE_BLURB \
"The tile cache is used to make sure the gimp doesn't thrash " \
"tiles between memory and disk. Setting this value higher will " \
"cause the gimp to use less swap space, but will also cause " \
"the gimp to use more memory. Conversely, a smaller cache size " \
"causes the gimp to use more swap space and less memory."

#define MARCHING_ANTS_SPEED_BLURB \
"Speed of marching ants in the selection outline.  This value is in " \
"milliseconds (less time indicates faster marching)."

#define LAST_OPENED_SIZE_BLURB \
"How many recently opened image filenames to keep on the File menu."

#define UNDO_LEVELS_BLURB \
"Set the number of operations kept on the undo stack."

#define TRANSPARENCY_TYPE_BLURB \
"Set the manner in which transparency is displayed in images."

#define TRANSPARENCY_SIZE_BLURB \
"Sets the size of the checkerboard used to display transparency."

#define PERFECT_MOUSE_BLURB \
"If set to true, the X server is queried for the mouse's current " \
"position on each motion event, rather than relying on the position " \
"hint.  This means painting with large brushes should be more accurate, " \
"but it may be slower.  Perversely, on some X servers turning on this " \
"option results in faster painting."

#define COLORMAP_CYCLING_BLURB \
"Specify that marching ants for selected regions will be drawn with " \
"colormap cycling as oposed to redrawing with different stipple masks. " \
"This color cycling option works only with 8-bit displays."

#define DEFAULT_THRESHOLD_BLURB \
"Tools such as fuzzy-select and bucket fill find regions based on a " \
"seed-fill algorithm.  The seed fill starts at the intially selected " \
"pixel and progresses in all directions until the difference of pixel " \
"intensity from the original is greater than a specified threshold. " \
"This value represents the default threshold."

#define STINGY_MEMORY_USE_BLURB \
"There is always a tradeoff between memory usage and speed.  In most " \
"cases, the GIMP opts for speed over memory.  However, if memory is a " \
"big issue, set stingy-memory-use."

#define RESIZE_WINDOWS_ON_ZOOM_BLURB \
"When zooming into and out of images, this option enables the automatic " \
"resizing of windows. " \

#define RESIZE_WINDOWS_ON_RESIZE_BLURB \
"When the physical image size changes, this option enables the automatic " \
"resizing of windows."

#define CURSOR_UPDATING_BLURB \
"Context-dependent cursors are cool.  They are enabled by default. " \
"However, they require overhead that you may want to do without."

#define PREVIEW_SIZE_BLURB \
"Set the default preview size."

#define SHOW_MENUBAR_BLURB \
"Set the menubar visibility. This can also be toggled with the "\
"View->Toggle Menubar command."

#define SHOW_RULERS_BLURB \
"Set the ruler visibility. This can also be toggled with the "\
"View->Toggle Rulers command."

#define SHOW_STATUSBAR_BLURB \
"Controlling statusbar visibility. This can also be toggled with "\
"the View->Toggle Statusbar command."

#define INTERPOLATION_TYPE_BLURB \
"Set the level of interpolation used for scaling and other transformations."

#define CONFIRM_ON_CLOSE_BLURB \
"Ask for confirmation before closing an image without saving."

#define SAVE_SESSION_INFO_BLURB \
"Remember the positions and sizes of the main dialogs and asks your " \
"window-manager to place them there again the next time you use the " \
"GIMP."

#define SAVE_DEVICE_STATUS_BLURB \
"Remember the current tool, pattern, color, and brush across GIMP " \
"sessions."

#define ALWAYS_RESTORE_SESSION_BLURB \
"Let GIMP try to restore your last saved session."

#define SHOW_TIPS_BLURB \
"Set to display a handy GIMP tip on startup."

#define SHOW_TOOL_TIPS_BLURB \
"Set to display tooltips."

#define DEFAULT_IMAGE_WIDTH_BLURB \
"Set the default image width in the File/New dialog."

#define DEFAULT_IMAGE_HEIGHT_BLURB \
"Set the default image height in the File/New dialog."

#define DEFAULT_IMAGE_TYPE_BLURB \
"Set the default image type in the File/New dialog."

#define DEFAULT_UNIT_BLURB \
"Set the default unit for new images and for the File/New dialog. " \
"This units will be used for coordinate display when not in dot-for-dot " \
"mode."

#define DEFAULT_XRESOLUTION_BLURB \
"Set the default horizontal resolution for new images and for the " \
"File/New dialog. This value is always in dpi (dots per inch)."

#define DEFAULT_YRESOLUTION_BLURB \
"Set the default vertical resolution for new images and for the " \
"File/New dialog. This value is always in dpi (dots per inch)."

#define DEFAULT_RESOLUTION_UNIT_BLURB \
"Set the units for the display of the default resolution in the " \
"File/New dialog."

#define MONITOR_XRESOLUTION_BLURB \
"Set the monitor's horizontal resolution, in dots per inch.  If set to " \
"0, forces the X server to be queried for both horizontal and vertical " \
"resolution information."

#define MONITOR_YRESOLUTION_BLURB \
"Set the monitor's vertical resolution, in dots per inch.  If set to " \
"0, forces the X server to be queried for both horizontal and vertical " \
"resolution information."

#define NUM_PROCESSORS_BLURB \
"On multiprocessor machines, if GIMP has been compiled with --enable-mp " \
"this sets how many processors GIMP should use simultaneously."

#define IMAGE_TITLE_FORMAT_BLURB \
"Set the text to appear in image window titles.  Certain % character " \
"sequences are recognised and expanded as follows:\n" \
"\n" \
"%%  literal percent sign\n" \
"%f  bare filename, or \"Untitled\"\n" \
"%F  full path to file, or \"Untitled\"\n" \
"%p  PDB image id\n" \
"%i  view instance number\n" \
"%t  image type (RGB, grayscale, indexed)\n" \
"%z  zoom factor as a percentage\n" \
"%s  source scale factor\n" \
"%d  destination scale factor\n" \
"%Dx expands to x if the image is dirty, the empty string otherwise\n" \
"%Cx expands to x if the image is clean, the empty string otherwise\n" \
"%m  memory used by the image\n" \
"%l  the number of layers\n" \
"%L  the name of the active layer/channel\n" \
"%w  image width in pixels\n" \
"%W  image width in real-world units\n" \
"%h  image height in pixels\n" \
"%H  image height in real-world units\n" \
"%u  unit symbol\n" \
"%U  unit abbreviation\n\n"


#define IMAGE_STATUS_FORMAT_BLURB \
"Set the text to appear in image window status bars. See image-title-format " \
"for the list of possible % sequences."

#define TOOL_PLUG_IN_PATH_BLURB        NULL
#define ENVIRON_PATH_BLURB             NULL
#define PLUGINRC_PATH_BLURB            NULL
#define MONITOR_RES_FROM_GDK_BLURB     NULL
#define THEME_PATH_BLURB               NULL
#define DEFAULT_COMMENT_BLURB          NULL
#define DEFAULT_DOT_FOR_DOT_BLURB      NULL
#define MODULE_LOAD_INHIBIT_BLURB      NULL
#define INFO_WINDOW_PER_DISPLAY_BLURB  NULL
#define TRUST_DIRTY_FLAG_BLURB         NULL
#define RESTORE_SESSION_BLURB          NULL
#define TEAROFF_MENUS_BLURB            NULL
#define THEME_BLURB                    NULL
#define USE_HELP_BLURB                 NULL
#define THUMBNAIL_SIZE_BLURB           NULL
#define MIN_COLORS_BLURB               NULL
#define CURSOR_MODE_BLURB              NULL
#define NAVIGATION_PREVIEW_SIZE_BLURB  NULL
#define CANVAS_PADDING_MODE_BLURB      NULL
#define CANVAS_PADDING_COLOR_BLURB     NULL
#define HELP_BROWSER_BLURB             NULL
#define MAX_NEW_IMAGE_SIZE_BLURB       NULL


#endif  /* __GIMP_CONFIG_BLURBS_H__ */
