/*  gimprc-blurbs.h  --  descriptions for gimprc properties  */

#pragma once

/*  Not all strings defined here are used in the user interface
 *  (the preferences dialog mainly) and only those that are should
 *  be marked for translation.
 */

#define ACTIVATE_ON_FOCUS_BLURB \
_("When enabled, an image will become the active image when its image " \
  "window receives the focus. This is useful for window managers using " \
  "\"click to focus\".")

#define BRUSH_PATH_BLURB \
"Sets the brush search path."

#define BRUSH_PATH_WRITABLE_BLURB ""

#define DYNAMICS_PATH_BLURB \
_("Sets the dynamics search path.")

#define DYNAMICS_PATH_WRITABLE_BLURB ""

#define TOOL_PRESET_PATH_BLURB \
_("Sets the dynamics search path.")

#define TOOL_PRESET_PATH_WRITABLE_BLURB ""

#define CANVAS_PADDING_COLOR_BLURB \
_("Sets the canvas padding color used if the padding mode is set to " \
  "custom color.")

#define CANVAS_PADDING_IN_SHOW_ALL_BLURB \
_("Specifies whether to keep the canvas padding when \"View -> Show All\" " \
  "is enabled.")

#define CANVAS_PADDING_MODE_BLURB \
_("Specifies how the area around the image should be drawn.")

#define CHECK_UPDATES_BLURB \
_("Check for availability of GIMP updates through background internet queries.")

#define CHECK_UPDATE_TIMESTAMP_BLURB \
_("Timestamp of the last update check.")

#define COLOR_MANAGEMENT_BLURB \
"Defines the color management behavior."

#define COLOR_PROFILE_POLICY_BLURB \
_("What to do when opening a file with an embedded ICC color profile.")

#define COLOR_PROFILE_PATH_BLURB \
_("Sets the default folder path for all color profile file dialogs.")

#define CURSOR_MODE_BLURB \
_("Sets the type of mouse pointers to use.")

#define CURSOR_HANDEDNESS_BLURB \
_("Sets the handedness for cursor positioning.")

#define CURSOR_UPDATING_BLURB \
_("Context-dependent mouse pointers are helpful.  They are enabled by " \
  "default.  However, they require overhead that you may want to do without.")

#define DEFAULT_BRUSH_BLURB \
"Specify a default brush.  The brush is searched for in the " \
"specified brush path."

#define DEFAULT_DYNAMICS_BLURB \
"Specify a default dynamics.  The dynamics is searched for in the " \
"specified dynamics path."

#define DEFAULT_TOOL_PRESET_BLURB \
"Specify a default tool preset.  The tool preset is searched for in the " \
"specified tool prests path."

#define CUSTOM_TITLE_BAR_BLURB \
_("Merge menu and title bar (client-side decoration)")

#define DEFAULT_SHOW_ALL_BLURB \
_("Show full image content by default.")

#define DEFAULT_DOT_FOR_DOT_BLURB \
_("When enabled, this will ensure that each pixel of an image gets " \
  "mapped to a pixel on the screen.")

#define DEFAULT_FONT_BLURB \
"Specify a default font."

#define DEFAULT_GRADIENT_BLURB \
"Specify a default gradient."

#define DEFAULT_GRID_BLURB \
"Specify a default image grid."

#define DEFAULT_IMAGE_BLURB \
"Sets the default image in the \"File/New\" dialog."

#define DEFAULT_MYPAINT_BRUSH_BLURB \
"Specify a default MyPaint brush."

#define DEFAULT_PATTERN_BLURB \
"Specify a default pattern."

#define DEFAULT_PALETTE_BLURB \
"Specify a default palette."

#define DEFAULT_SNAP_DISTANCE_BLURB \
_("This is the distance in pixels where Guide and Grid snapping " \
  "activates.")

#define SNAP_TO_GUIDES_BLURB \
_("Snap to guides by default in new image windows.")

#define SNAP_TO_GRID_BLURB \
_("Snap to the grid by default in new image windows.")

#define SNAP_TO_CANVAS_BLURB \
_("Snap to the canvas edges by default in new image windows.")

#define SNAP_TO_PATH_BLURB \
_("Snap to the active path by default in new image windows.")

#define SNAP_TO_BBOX_BLURB \
_("Snap to the layers bounding boxes by default in new image windows.")

#define SNAP_TO_EQUIDISTANCE_BLURB \
_("Snap to the equidistance between three bounding boxes by default in new image windows.")

#define DEFAULT_THRESHOLD_BLURB \
_("Tools such as fuzzy-select and bucket fill find regions based on a " \
  "seed-fill algorithm.  The seed fill starts at the initially selected " \
  "pixel and progresses in all directions until the difference of pixel " \
  "intensity from the original is greater than a specified threshold. " \
  "This value represents the default threshold.")

#define DEFAULT_VIEW_BLURB \
"Sets the default settings for the image view."

#define DEFAULT_FULLSCREEN_VIEW_BLURB \
"Sets the default settings used when an image is viewed in fullscreen mode."

#define DOCK_WINDOW_HINT_BLURB \
_("The window type hint that is set on dock windows and the toolbox " \
  "window. This may affect the way your window manager decorates and " \
  "handles these windows.")

#define ENVIRON_PATH_BLURB \
"Sets the environ search path."

#define FRACTALEXPLORER_PATH_BLURB \
"Where to search for fractals used by the Fractal Explorer plug-in."

#define GFIG_PATH_BLURB \
"Where to search for Gfig figures used by the Gfig plug-in."

#define GFLARE_PATH_BLURB \
"Where to search for gflares used by the GFlare plug-in."

#define GIMPRESSIONIST_PATH_BLURB \
"Where to search for data used by the Gimpressionist plug-in."

#define GLOBAL_BRUSH_BLURB \
_("When enabled, the selected brush will be used for all tools.")

#define GLOBAL_DYNAMICS_BLURB \
_("When enabled, the selected dynamics will be used for all tools.")

#define GLOBAL_FONT_BLURB \
"When enabled, the selected font will be used for all tools."

#define GLOBAL_GRADIENT_BLURB \
_("When enabled, the selected gradient will be used for all tools.")

#define GLOBAL_PATTERN_BLURB \
_("When enabled, the selected pattern will be used for all tools.")

#define GLOBAL_PALETTE_BLURB \
"When enabled, the selected palette will be used for all tools."

#define GLOBAL_EXPAND_BLURB \
_("When enabled, the selected auto expand layer settings will be used for all tools.")

#define GRADIENT_PATH_BLURB \
"Sets the gradient search path."

#define GRADIENT_PATH_WRITABLE_BLURB ""

#define FONT_PATH_BLURB \
"Where to look for fonts in addition to the system-wide installed fonts."

#define HELP_BROWSER_BLURB \
_("Sets the browser used by the help system.")

#define HELP_LOCALES_BLURB \
"Specifies the language preferences used by the help system. This is a " \
"colon-separated list of language identifiers with decreasing priority. " \
"If empty, the language is taken from the user's locale setting."

#define FILTER_TOOL_MAX_RECENT_BLURB \
_("How many recent settings to keep around in filter tools.")

#define FILTER_TOOL_USE_LAST_SETTINGS_BLURB \
_("Default to the last used settings in filter tools.")

#define FILTER_TOOL_SHOW_COLOR_OPTIONS_BLURB \
_("Show advanced color options in filter tools.")

#define IMAGE_STATUS_FORMAT_BLURB \
_("Sets the text to appear in image window status bars.")

#define IMAGE_TITLE_FORMAT_BLURB \
_("Sets the text to appear in image window titles.")

#define IMPORT_PROMOTE_FLOAT_BLURB \
_("Promote imported images to floating point precision. Does not apply " \
  "to indexed images.")

#define IMPORT_PROMOTE_DITHER_BLURB \
_("When promoting imported images to floating point precision, also add " \
  "minimal noise in order to distribute color values a bit.")

#define IMPORT_ADD_ALPHA_BLURB \
_("Add an alpha channel to all layers of imported images.")

#define IMPORT_RAW_PLUG_IN_BLURB \
_("Which plug-in to use for importing raw digital camera files.")

#define EXPORT_FILE_TYPE_BLURB \
_("Export file type used by default.")

#define EXPORT_COLOR_PROFILE_BLURB \
_("Export the image's color profile by default.")

#define EXPORT_COMMENT_BLURB \
_("Export the image's comment by default.")

#define EXPORT_THUMBNAIL_BLURB \
_("Export the image's thumbnail by default")

/* Translators: tooltip for configuration option (checkbox).
 * It determines how file export plug-ins handle Exif by default.
 */
#define EXPORT_METADATA_EXIF_BLURB \
_("Export Exif metadata by default.")

/* Translators: tooltip for configuration option (checkbox).
 * It determines how file export plug-ins handle XMP by default.
 */
#define EXPORT_METADATA_XMP_BLURB \
_("Export XMP metadata by default.")

/* Translators: tooltip for configuration option (checkbox).
 * It determines how file export plug-ins handle IPTC by default.
 */
#define EXPORT_METADATA_IPTC_BLURB \
_("Export IPTC metadata by default.")

/* Translators: tooltip for configuration option (checkbox).
 * It determines what metadata is updated when exporting.
 */
#define EXPORT_UPDATE_METADATA_BLURB \
_("When enabled, add and update metadata automatically. When disabled, " \
  "only the minimum necessary metadata changes are made, without changing " \
  "modification date, synchronizing tags, or updating the software and " \
  "change history metadata.")

#define GENERATE_BACKTRACE_BLURB \
_("Try generating debug data for bug reporting when appropriate.")

#define WIN32_POINTER_INPUT_API_BLURB \
_("Sets the preferred pen and touch input API.")

#define INITIAL_ZOOM_TO_FIT_BLURB \
_("When enabled, this will ensure that the full image is visible after a " \
  "file is opened, otherwise it will be displayed with a scale of 1:1.")

#define DRAG_ZOOM_MODE_BLURB \
_("Whether to zoom based on distance moved or time spent moving, when " \
  "zooming via dragging the mouse.")

#define DRAG_ZOOM_SPEED_BLURB \
_("Adjusts the rate at which dragging the mouse will zoom the canvas, " \
  "in percentage.")

#define INTERPOLATION_TYPE_BLURB \
_("Sets the level of interpolation used for scaling and other " \
  "transformations.")

#define INTERPRETER_PATH_BLURB \
"Sets the interpreter search path."

#define LANGUAGE_BLURB \
_("Specifies the language to use for the user interface.")

#define LAST_KNOWN_RELEASE_BLURB \
_("The last known release version of GIMP as queried from official website.")

#define CONFIG_VERSION_BLURB \
_("The version of GIMP config files.")

#define LAST_OPENED_SIZE_BLURB \
_("How many recently opened image filenames to keep on the File menu.")

#define LAST_RELEASE_TIMESTAMP_BLURB \
_("The timestamp for the last known release date.")

#define LAST_REVISION_BLURB \
_("The last revision number for the release.")

#define MARCHING_ANTS_SPEED_BLURB \
_("Speed of marching ants in the selection outline.  This value is in " \
  "milliseconds (less time indicates faster marching).")

#define MAX_NEW_IMAGE_SIZE_BLURB  \
_("GIMP will warn the user if an attempt is made to create an image that " \
  "would take more memory than the size specified here.")

#define METADATA_ROTATION_POLICY_BLURB \
_("How to handle \"Orientation\" metadata when opening a file.")

#define MODULE_PATH_BLURB \
"Sets the module search path."

#define MONITOR_RES_FROM_GDK_BLURB \
"When enabled, GIMP will use the monitor resolution from the windowing system."

#define MONITOR_XRESOLUTION_BLURB \
_("Sets the monitor's horizontal resolution, in dots per inch.  If set to " \
  "0, forces the X server to be queried for both horizontal and vertical " \
  "resolution information.")

#define MONITOR_YRESOLUTION_BLURB \
_("Sets the monitor's vertical resolution, in dots per inch.  If set to " \
  "0, forces the X server to be queried for both horizontal and vertical " \
  "resolution information.")

#define EDIT_NON_VISIBLE_BLURB \
_("When enabled, non-visible layers can be edited as normal.")

#define MOVE_TOOL_CHANGES_ACTIVE_BLURB \
_("If enabled, the move tool sets the edited layer or path as active.  " \
  "This used to be the default behavior in older versions.")

#define MYPAINT_BRUSH_PATH_BLURB \
"Sets the brush search path."

#define MYPAINT_BRUSH_PATH_WRITABLE_BLURB ""

#define NAVIGATION_PREVIEW_SIZE_BLURB \
_("Sets the size of the navigation preview available in the lower right " \
  "corner of the image window.")

#define NUM_PROCESSORS_BLURB \
_("Sets how many threads GIMP should use for operations that support it.")

#define PALETTE_PATH_BLURB \
"Sets the palette search path."

#define PALETTE_PATH_WRITABLE_BLURB ""

#define PATTERN_PATH_BLURB \
"Sets the pattern search path."

#define PATTERN_PATH_WRITABLE_BLURB ""

#define FILTER_HISTORY_SIZE_BLURB \
"How many recently used filters and plug-ins to keep on the Filters menu."

#define PLUG_IN_PATH_BLURB \
"Sets the plug-in search path."

#define PLUGINRC_PATH_BLURB \
"Sets the pluginrc search path."

#define LAYER_PREVIEWS_BLURB \
_("Sets whether GIMP should create previews of layers and channels. " \
  "Previews in the layers and channels dialog are nice to have but they " \
  "can slow things down when working with large images.")

#define GROUP_LAYER_PREVIEWS_BLURB \
_("Sets whether GIMP should create previews of layer groups. " \
  "Layer group previews are more expensive than ordinary layer previews.")

#define LAYER_PREVIEW_SIZE_BLURB \
_("Sets the preview size used for layers and channel previews in newly " \
  "created dialogs.")

#define QUICK_MASK_COLOR_BLURB \
_("Sets the default quick mask color.")

#define RESIZE_WINDOWS_ON_RESIZE_BLURB \
_("When enabled, the image window will automatically resize itself " \
  "whenever the physical image size changes. This setting only takes " \
  "effect in multi-window mode.")

#define RESIZE_WINDOWS_ON_ZOOM_BLURB \
_("When enabled, the image window will automatically resize itself " \
  "when zooming into and out of images. This setting only takes " \
  "effect in multi-window mode.")

#define RESTORE_SESSION_BLURB \
_("Let GIMP try to restore your last saved session on each startup.")

#define RESTORE_MONITOR_BLURB \
_("When enabled, GIMP will try to restore windows on the monitor they " \
  "were open before.  When disabled, windows will appear on the currently " \
  "used monitor.")

#define SAVE_DEVICE_STATUS_BLURB \
_("Remember the current tool, pattern, color, and brush across GIMP " \
  "sessions.")

#define DEVICES_SHARE_TOOL_BLURB \
_("When enabled, the same tool and tool options will be used for all " \
  "input devices. No tool switching will occur when the input device " \
  "changes.")

#define SAVE_DOCUMENT_HISTORY_BLURB \
_("Keep a permanent record of all opened and saved files in the Recent " \
  "Documents list.")

#define SAVE_SESSION_INFO_BLURB \
_("Save the positions and sizes of the main dialogs when GIMP exits.")

#define SAVE_TOOL_OPTIONS_BLURB \
_("Save the tool options when GIMP exits.")

#define SCRIPT_FU_PATH_BLURB \
"This path will be searched for scripts when the Script-Fu plug-in is run."

#define SHOW_BRUSH_OUTLINE_BLURB \
_("When enabled, all paint tools will show a preview of the current " \
  "brush's outline.")

#define SNAP_BRUSH_OUTLINE_BLURB \
_("When enabled, the brush outline will snap to individual dabs while " \
  "painting.")

#define SHOW_HELP_BUTTON_BLURB \
_("When enabled, dialogs will show a help button that gives access to " \
  "the related help page.  Without this button, the help page can still " \
  "be reached by pressing F1.")

#define SHOW_PAINT_TOOL_CURSOR_BLURB \
_("When enabled, the pointer will be shown over the image while " \
  "using a paint tool. "                                          \
  "If both the brush outline and pointer are disabled, the "      \
  "position will be indicated as unobtrusively as possibly.")

#define SHOW_MENUBAR_BLURB \
_("When enabled, the menubar is visible by default. This can also be " \
  "toggled with the \"View->Show Menubar\" command.")

#define SHOW_RULERS_BLURB \
_("When enabled, the rulers are visible by default. This can also be " \
  "toggled with the \"View->Show Rulers\" command.")

#define SHOW_SCROLLBARS_BLURB \
_("When enabled, the scrollbars are visible by default. This can also be " \
  "toggled with the \"View->Show Scrollbars\" command.")

#define SHOW_STATUSBAR_BLURB \
_("When enabled, the statusbar is visible by default. This can also be " \
  "toggled with the \"View->Show Statusbar\" command.")

#define SHOW_SELECTION_BLURB \
_("When enabled, the selection is visible by default. This can also be " \
  "toggled with the \"View->Show Selection\" command.")

#define SHOW_LAYER_BOUNDARY_BLURB \
_("When enabled, the layer boundary is visible by default. This can also " \
  "be toggled with the \"View->Show Layer Boundary\" command.")

#define SHOW_CANVAS_BOUNDARY_BLURB \
_("When enabled, the canvas boundary is visible by default. This can also " \
  "be toggled with the \"View->Show Canvas Boundary\" command.")

#define SHOW_GUIDES_BLURB \
_("When enabled, the guides are visible by default. This can also be " \
  "toggled with the \"View->Show Guides\" command.")

#define SHOW_GRID_BLURB \
_("When enabled, the grid is visible by default. This can also be toggled " \
  "with the \"View->Show Grid\" command.")

#define SHOW_SAMPLE_POINTS_BLURB \
_("When enabled, the sample points are visible by default. This can also be " \
  "toggled with the \"View->Show Sample Points\" command.")

#define SHOW_TOOLTIPS_BLURB \
_("Show a tooltip when the pointer hovers over an item.")

#define SINGLE_WINDOW_MODE_BLURB \
_("Use GIMP in a single-window mode.")

#define HIDE_DOCKS_BLURB \
_("Hide docks and other windows, leaving only image windows.")

#define SHOW_TABS_BLURB \
_("Show the image tabs bar in single window mode.")

#define PLAYGROUND_NPD_TOOL_BLURB \
_("Enable the N-Point Deformation tool.")

#define PLAYGROUND_HANDLE_TRANSFORM_TOOL_BLURB \
_("Enable the Handle Transform tool.")

#define PLAYGROUND_SYMMETRY_BLURB \
_("Enable symmetry on painting.")

#define PLAYGROUND_MYBRUSH_TOOL_BLURB \
_("Enable the MyPaint Brush tool.")

#define PLAYGROUND_SEAMLESS_CLONE_TOOL_BLURB \
_("Enable the Seamless Clone tool.")

#define PLAYGROUND_PAINT_SELECT_TOOL_BLURB \
_("Enable the Paint Select tool.")

#define PLAYGROUND_USE_LIST_BOX_BLURB \
_("Use the new GtkListBox widget for simple lists.")

#define SPACE_BAR_ACTION_BLURB \
_("What to do when the space bar is pressed in the image window.")

#define SWAP_COMPRESSION_BLURB \
_("The compression method used for tile data stored in the swap file.")

#define SWAP_PATH_BLURB \
_("Sets the swap file location. GIMP uses a tile based memory allocation " \
  "scheme. The swap file is used to quickly and easily swap tiles out to " \
  "disk and back in. Be aware that the swap file can easily get very large " \
  "if GIMP is used with large images. " \
  "Also, things can get horribly slow if the swap file is created on " \
  "a folder that is mounted over NFS.  For these reasons, it may be " \
  "desirable to put your swap file in \"/tmp\".")

#define CAN_CHANGE_ACCELS_BLURB \
_("When enabled, you can change keyboard shortcuts for menu items " \
  "by hitting a key combination while the menu item is highlighted.")

#define SAVE_ACCELS_BLURB \
_("Save changed keyboard shortcuts when GIMP exits.")

#define RESTORE_ACCELS_BLURB \
_("Restore saved keyboard shortcuts on each GIMP startup.")

#define TEMP_PATH_BLURB \
_("Sets the folder for temporary storage. Files will appear here " \
  "during the course of running GIMP.  Most files will disappear " \
  "when GIMP exits, but some files are likely to remain, so it " \
  "is best if this folder not be one that is shared by other users.")

#define THEME_BLURB \
_("The name of the theme to use.")

#define THEME_SCHEME_BLURB \
_("Chooses the color scheme variant of the theme.")

#define THEME_PATH_BLURB \
"Sets the theme search path."

#define ICON_THEME_BLURB \
_("The name of the icon theme to use.")

#define OVERRIDE_THEME_ICON_SIZE_BLURB \
_("Override theme-set icon sizes.")

#define VIEWABLES_FOLLOW_THEME_BLURB \
_("Use theme colors for preview")

#define ICON_SIZE_BLURB \
_("The size of the icons to use.")

#define PREFER_SYMBOLIC_ICONS_BLURB \
_("When enabled, symbolic icons will be preferred if available.")

#define ICON_THEME_PATH_BLURB \
"Sets the icon theme search path."

#define FONT_SIZE_BLURB \
_("Tweak font size of the graphical interface.")

#define IMAGE_CONVERT_PROFILE_INTENT_BLURB \
_("Sets the default rendering intent for the 'Convert to Color Profile' dialog.")

#define IMAGE_CONVERT_PROFILE_BPC_BLURB \
_("Sets the default 'Black Point Compensation' state for the " \
  "'Convert to Color Profile' dialog.")

#define IMAGE_CONVERT_PRECISION_LAYER_DITHER_METHOD_BLURB \
_("Sets the default layer dithering method for the 'Convert Precision' dialog.")

#define IMAGE_CONVERT_PRECISION_TEXT_LAYER_DITHER_METHOD_BLURB \
_("Sets the default text layer dithering method for the 'Convert Precision' dialog.")

#define IMAGE_CONVERT_PRECISION_CHANNEL_DITHER_METHOD_BLURB \
_("Sets the default channel dithering method for the 'Convert Precision' dialog.")

#define IMAGE_CONVERT_INDEXED_PALETTE_TYPE_BLURB \
_("Sets the default palette type for the 'Convert to Indexed' dialog.")

#define IMAGE_CONVERT_INDEXED_MAX_COLORS_BLURB \
_("Sets the default maximum number of colors for the 'Convert to Indexed' dialog.")

#define IMAGE_CONVERT_INDEXED_REMOVE_DUPLICATES_BLURB \
_("Sets the default 'Remove duplicate colors' state for the 'Convert to Indexed' dialog.")

#define IMAGE_CONVERT_INDEXED_DITHER_TYPE_BLURB \
_("Sets the default dithering type for the 'Convert to Indexed' dialog.")

#define IMAGE_CONVERT_INDEXED_DITHER_ALPHA_BLURB \
_("Sets the default 'Dither alpha' state for the 'Convert to Indexed' dialog.")

#define IMAGE_CONVERT_INDEXED_DITHER_TEXT_LAYERS_BLURB \
_("Sets the default 'Dither text layers' state for the 'Convert to Indexed' dialog.")

#define IMAGE_RESIZE_FILL_TYPE_BLURB \
_("Sets the default fill type for the 'Canvas Size' dialog.")

#define IMAGE_RESIZE_LAYER_SET_BLURB \
_("Sets the default set of layers to resize for the 'Canvas Size' dialog.")

#define IMAGE_RESIZE_RESIZE_TEXT_LAYERS_BLURB \
_("Sets the default 'Resize text layers' state for the 'Canvas Size' dialog.")

# define ITEMS_SELECT_METHOD_BLURB \
_("Sets how items are searched and selected from a textual pattern.")

#define LAYER_NEW_NAME_BLURB \
_("Sets the default layer name for the 'New Layer' dialog.")

#define LAYER_NEW_MODE_BLURB \
_("Sets the default mode for the 'New Layer' dialog.")

#define LAYER_NEW_BLEND_SPACE_BLURB \
_("Sets the default blend space for the 'New Layer' dialog.")

#define LAYER_NEW_COMPOSITE_SPACE_BLURB \
_("Sets the default composite space for the 'New Layer' dialog.")

#define LAYER_NEW_COMPOSITE_MODE_BLURB \
_("Sets the default composite mode for the 'New Layer' dialog.")

#define LAYER_NEW_OPACITY_BLURB \
_("Sets the default opacity for the 'New Layer' dialog.")

#define LAYER_NEW_FILL_TYPE_BLURB \
_("Sets the default fill type for the 'New Layer' dialog.")

#define LAYER_RESIZE_FILL_TYPE_BLURB \
_("Sets the default fill type for the 'Layer Boundary Size' dialog.")

#define LAYER_ADD_MASK_TYPE_BLURB \
_("Sets the default mask for the 'Add Layer Mask' dialog.")

#define LAYER_ADD_MASK_INVERT_BLURB \
_("Sets the default 'invert mask' state for the 'Add Layer Mask' dialog.")

#define LAYER_ADD_MASK_EDIT_MASK \
_("Sets the default 'edit mask' state for the 'Add Layer Mask' dialog.")

#define LAYER_MERGE_TYPE_BLURB \
_("Sets the default merge type for the 'Merge Visible Layers' dialog.")

#define LAYER_MERGE_ACTIVE_GROUP_ONLY_BLURB \
_("Sets the default 'Active group only' for the 'Merge Visible Layers' dialog.")

#define LAYER_MERGE_DISCARD_INVISIBLE_BLURB \
_("Sets the default 'Discard invisible' for the 'Merge Visible Layers' dialog.")

#define CHANNEL_NEW_NAME_BLURB \
_("Sets the default channel name for the 'New Channel' dialog.")

#define CHANNEL_NEW_COLOR_BLURB \
_("Sets the default color and opacity for the 'New Channel' dialog.")

#define PATH_NEW_NAME_BLURB \
_("Sets the default path name for the 'New Path' dialog.")

#define PATH_EXPORT_PATH_BLURB \
_("Sets the default folder path for the 'Export Path' dialog.")

#define PATH_EXPORT_ACTIVE_ONLY_BLURB \
_("Sets the default 'Export the selected paths' state for the 'Export Path' dialog.")

#define PATH_IMPORT_PATH_BLURB \
_("Sets the default folder path for the 'Import Path' dialog.")

#define PATH_IMPORT_MERGE_BLURB \
_("Sets the default 'Merge imported paths' state for the 'Import Path' dialog.")

#define PATH_IMPORT_SCALE_BLURB \
_("Sets the default 'Scale imported paths to fit size' state for the 'Import Path' dialog.")

#define SELECTION_FEATHER_RADIUS_BLURB \
_("Sets the default feather radius for the 'Feather Selection' dialog.")

#define SELECTION_FEATHER_EDGE_LOCK_BLURB \
_("Sets the default 'Selected areas continue outside the image' setting " \
  "for the 'Feather Selection' dialog.")

#define SELECTION_GROW_RADIUS_BLURB \
_("Sets the default grow radius for the 'Grow Selection' dialog.")

#define SELECTION_SHRINK_RADIUS_BLURB \
_("Sets the default shrink radius for the 'Shrink Selection' dialog.")

#define SELECTION_SHRINK_EDGE_LOCK_BLURB \
_("Sets the default 'Selected areas continue outside the image' setting " \
  "for the 'Shrink Selection' dialog.")

#define SELECTION_BORDER_RADIUS_BLURB \
_("Sets the default border radius for the 'Border Selection' dialog.")

#define SELECTION_BORDER_EDGE_LOCK_BLURB \
_("Sets the default 'Selected areas continue outside the image' setting " \
  "for the 'Border Selection' dialog.")

#define SELECTION_BORDER_STYLE_BLURB \
_("Sets the default border style for the 'Border Selection' dialog.")

#define FILL_OPTIONS_BLURB \
"The default fill options for the fill dialogs."

#define STROKE_OPTIONS_BLURB \
"The default stroke options for the stroke dialogs."

#define THUMBNAIL_SIZE_BLURB \
_("Sets the size of the thumbnail shown in the Open dialog.")

#define THUMBNAIL_FILESIZE_LIMIT_BLURB \
_("The thumbnail in the Open dialog will be automatically updated " \
  "if the file being previewed is smaller than the size set here.")

#define TILE_CACHE_SIZE_BLURB \
_("When the amount of pixel data exceeds this limit, GIMP will start to " \
  "swap tiles to disk.  This is a lot slower but it makes it possible to " \
  "work on images that wouldn't fit into memory otherwise.  If you have a " \
  "lot of RAM, you may want to set this to a higher value.")

#define TOOLBOX_COLOR_AREA_BLURB \
_("Show the current foreground and background colors in the toolbox.")

#define TOOLBOX_FOO_AREA_BLURB \
_("Show the currently selected brush, pattern and gradient in the toolbox.")

#define TOOLBOX_GROUPS_BLURB \
_("Use a single toolbox button for grouped tools.")

#define TOOLBOX_IMAGE_AREA_BLURB \
_("Show the currently active image in the toolbox.")

#define TOOLBOX_WILBER_BLURB \
_("Show the GIMP mascot at the top of the toolbox.")

#define TRANSPARENCY_CUSTOM_COLOR1_BLURB \
_("The first color to use in the transparency checkerboard, " \
  "when Transparency Type is set to Custom colors.")

#define TRANSPARENCY_CUSTOM_COLOR2_BLURB \
_("The second color to use in the transparency checkerboard, " \
  "when Transparency Type is set to Custom colors.")

#define TRANSPARENCY_TYPE_BLURB \
_("Sets the manner in which transparency is displayed in images.")

#define TRANSPARENCY_SIZE_BLURB \
_("Sets the size of the checkerboard used to display transparency.")

#define TRUST_DIRTY_FLAG_BLURB \
_("When enabled, GIMP will not save an image if it has not been changed " \
  "since it was opened.")

#define UNDO_LEVELS_BLURB \
_("Sets the minimal number of operations that can be undone. More undo " \
  "levels are kept available until the undo-size limit is reached.")

#define UNDO_SIZE_BLURB \
_("Sets an upper limit to the memory that is used per image to keep " \
  "operations on the undo stack. Regardless of this setting, at least " \
  "as many undo-levels as configured can be undone.")

#define UNDO_PREVIEW_SIZE_BLURB \
_("Sets the size of the previews in the Undo History.")

#define USE_HELP_BLURB \
_("When enabled, pressing F1 will open the help browser.")

#define USE_OPENCL_BLURB \
_("When enabled, uses OpenCL for some operations.")

#define USER_MANUAL_ONLINE_BLURB \
"When enabled, the online user manual will be used by the help system. " \
"Otherwise the locally installed copy is used."

#define USER_MANUAL_ONLINE_URI_BLURB \
"The location of the online user manual. This is used if " \
"'user-manual-online' is enabled."

#define ZOOM_QUALITY_BLURB \
_("There's a tradeoff between speed and quality of the zoomed-out display.")

#define DEFAULT_USE_EVENT_HISTORY_BLURB \
"Bugs in event history buffer are frequent so in case of cursor " \
"offset problems turning it off helps."

#define SEARCH_SHOW_UNAVAILABLE_BLURB \
_("When enabled, a search of actions will also return inactive actions.")

#define ACTION_HISTORY_SIZE_BLURB \
_("The maximum number of actions saved in history.")
