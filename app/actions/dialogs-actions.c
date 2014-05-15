/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimptoolbox.h"

#include "display/gimpimagewindow.h"

#include "actions.h"
#include "dialogs-actions.h"
#include "dialogs-commands.h"

#include "gimp-intl.h"

static gboolean
dialogs_actions_toolbox_exists (Gimp *gimp);

const GimpStringActionEntry dialogs_dockable_actions[] =
  {
    { "dialogs-toolbox", NULL, NC_("windows-action", "Tool_box"), "<primary>B",
    NULL /* set in dialogs_actions_update() */, "gimp-toolbox",
    GIMP_HELP_TOOLBOX },

    { "dialogs-tool-options", GIMP_STOCK_TOOL_OPTIONS, NC_("dialogs-action",
                                                           "Tool _Options"),
        NULL, NC_("dialogs-action", "Open the tool options dialog"),
        "gimp-tool-options",
        GIMP_HELP_TOOL_OPTIONS_DIALOG },

    { "dialogs-device-status", GIMP_STOCK_DEVICE_STATUS, NC_("dialogs-action",
                                                             "_Device Status"),
        NULL, NC_("dialogs-action", "Open the device status dialog"),
        "gimp-device-status",
        GIMP_HELP_DEVICE_STATUS_DIALOG },

    { "dialogs-layers", GIMP_STOCK_LAYERS, NC_("dialogs-action", "_Layers"),
        "<primary>L", NC_("dialogs-action", "Open the layers dialog"),
        "gimp-layer-list",
        GIMP_HELP_LAYER_DIALOG },

    { "dialogs-channels", GIMP_STOCK_CHANNELS, NC_("dialogs-action",
                                                   "_Channels"), NULL, NC_(
        "dialogs-action", "Open the channels dialog"), "gimp-channel-list",
    GIMP_HELP_CHANNEL_DIALOG },

    { "dialogs-vectors", GIMP_STOCK_PATHS, NC_("dialogs-action", "_Paths"),
        NULL, NC_("dialogs-action", "Open the paths dialog"),
        "gimp-vectors-list",
        GIMP_HELP_PATH_DIALOG },

    { "dialogs-indexed-palette", GIMP_STOCK_COLORMAP, NC_("dialogs-action",
                                                          "Color_map"), NULL,
        NC_("dialogs-action", "Open the colormap dialog"),
        "gimp-indexed-palette",
        GIMP_HELP_INDEXED_PALETTE_DIALOG },

    { "dialogs-histogram", GIMP_STOCK_HISTOGRAM, NC_("dialogs-action",
                                                     "Histogra_m"), NULL, NC_(
        "dialogs-action", "Open the histogram dialog"), "gimp-histogram-editor",
    GIMP_HELP_HISTOGRAM_DIALOG },

    { "dialogs-selection-editor", GIMP_STOCK_TOOL_RECT_SELECT, NC_(
        "dialogs-action", "_Selection Editor"), NULL, NC_(
        "dialogs-action", "Open the selection editor"), "gimp-selection-editor",
    GIMP_HELP_SELECTION_DIALOG },

    { "dialogs-navigation", GIMP_STOCK_NAVIGATION, NC_("dialogs-action",
                                                       "Na_vigation"), NULL,
        NC_("dialogs-action", "Open the display navigation dialog"),
        "gimp-navigation-view",
        GIMP_HELP_NAVIGATION_DIALOG },

    { "dialogs-undo-history", GIMP_STOCK_UNDO_HISTORY, NC_("dialogs-action",
                                                           "Undo _History"),
        NULL, NC_("dialogs-action", "Open the undo history dialog"),
        "gimp-undo-history",
        GIMP_HELP_UNDO_DIALOG },

    { "dialogs-cursor", GIMP_STOCK_CURSOR, NC_("dialogs-action", "Pointer"),
        NULL, NC_("dialogs-action", "Open the pointer information dialog"),
        "gimp-cursor-view",
        GIMP_HELP_POINTER_INFO_DIALOG },

    { "dialogs-sample-points", GIMP_STOCK_SAMPLE_POINT, NC_("dialogs-action",
                                                            "_Sample Points"),
        NULL, NC_("dialogs-action", "Open the sample points dialog"),
        "gimp-sample-point-editor",
        GIMP_HELP_SAMPLE_POINT_DIALOG },

    { "dialogs-colors", GIMP_STOCK_DEFAULT_COLORS, NC_("dialogs-action",
                                                       "Colo_rs"), NULL, NC_(
        "dialogs-action", "Open the FG/BG color dialog"), "gimp-color-editor",
    GIMP_HELP_COLOR_DIALOG },

    { "dialogs-brushes", GIMP_STOCK_BRUSH, NC_("dialogs-action", "_Brushes"),
        "<primary><shift>B", NC_("dialogs-action", "Open the brushes dialog"),
        "gimp-brush-grid|gimp-brush-list",
        GIMP_HELP_BRUSH_DIALOG },

    { "dialogs-brush-editor", GIMP_STOCK_BRUSH, NC_("dialogs-action",
                                                    "Brush Editor"), NULL, NC_(
        "dialogs-action", "Open the brush editor"), "gimp-brush-editor",
    GIMP_HELP_BRUSH_EDIT },

    { "dialogs-dynamics", GIMP_STOCK_DYNAMICS, NC_("dialogs-action",
                                                   "Paint Dynamics"), NULL, NC_(
        "dialogs-action", "Open paint dynamics dialog"), "gimp-dynamics-list",
    GIMP_HELP_DYNAMICS_DIALOG },

    { "dialogs-dynamics-editor", GIMP_STOCK_DYNAMICS, NC_(
        "dialogs-action", "Paint Dynamics Editor"), NULL, NC_(
        "dialogs-action", "Open the paint dynamics editor"),
        "gimp-dynamics-editor",
        GIMP_HELP_DYNAMICS_EDITOR_DIALOG },

    { "dialogs-patterns", GIMP_STOCK_PATTERN, NC_("dialogs-action",
                                                  "P_atterns"),
        "<primary><shift>P", NC_("dialogs-action", "Open the patterns dialog"),
        "gimp-pattern-grid|gimp-pattern-list",
        GIMP_HELP_PATTERN_DIALOG },

    { "dialogs-gradients", GIMP_STOCK_GRADIENT, NC_("dialogs-action",
                                                    "_Gradients"), "<primary>G",
        NC_("dialogs-action", "Open the gradients dialog"),
        "gimp-gradient-list|gimp-gradient-grid",
        GIMP_HELP_GRADIENT_DIALOG },

    { "dialogs-gradient-editor", GIMP_STOCK_GRADIENT, NC_("dialogs-action",
                                                          "Gradient Editor"),
        NULL, NC_("dialogs-action", "Open the gradient editor"),
        "gimp-gradient-editor",
        GIMP_HELP_GRADIENT_EDIT },

    { "dialogs-palettes", GIMP_STOCK_PALETTE, NC_("dialogs-action",
                                                  "Pal_ettes"), NULL, NC_(
        "dialogs-action", "Open the palettes dialog"),
        "gimp-palette-list|gimp-palette-grid",
        GIMP_HELP_PALETTE_DIALOG },

    { "dialogs-palette-editor", GIMP_STOCK_PALETTE, NC_("dialogs-action",
                                                        "Palette Editor"), NULL,
        NC_("dialogs-action", "Open the palette editor"), "gimp-palette-editor",
        GIMP_HELP_PALETTE_EDIT },

    { "dialogs-tool-presets", GIMP_STOCK_TOOL_PRESET, NC_("dialogs-action",
                                                          "Tool presets"), NULL,
        NC_("dialogs-action", "Open tool presets dialog"),
        "gimp-tool-preset-list",
        GIMP_HELP_TOOL_PRESET_DIALOG },

    { "dialogs-fonts", GIMP_STOCK_FONT, NC_("dialogs-action", "_Fonts"), NULL,
        NC_("dialogs-action", "Open the fonts dialog"),
        "gimp-font-list|gimp-font-grid",
        GIMP_HELP_FONT_DIALOG },

    { "dialogs-buffers", GIMP_STOCK_BUFFER, NC_("dialogs-action", "B_uffers"),
        "", NC_("dialogs-action", "Open the named buffers dialog"),
        "gimp-buffer-list|gimp-buffer-grid",
        GIMP_HELP_BUFFER_DIALOG },

    { "dialogs-images", GIMP_STOCK_IMAGES, NC_("dialogs-action", "_Images"),
        NULL, NC_("dialogs-action", "Open the images dialog"),
        "gimp-image-list|gimp-image-grid",
        GIMP_HELP_IMAGE_DIALOG },

    { "dialogs-document-history", "document-open-recent", NC_(
        "dialogs-action", "Document Histor_y"), "", NC_(
        "dialogs-action", "Open the document history dialog"),
        "gimp-document-list|gimp-document-grid",
        GIMP_HELP_DOCUMENT_DIALOG },

    { "dialogs-templates", GIMP_STOCK_TEMPLATE, NC_("dialogs-action",
                                                    "_Templates"), "", NC_(
        "dialogs-action", "Open the image templates dialog"),
        "gimp-template-list|gimp-template-grid",
        GIMP_HELP_TEMPLATE_DIALOG },

    { "dialogs-error-console", GIMP_STOCK_WARNING, NC_("dialogs-action",
                                                       "Error Co_nsole"), NULL,
        NC_("dialogs-action", "Open the error console"), "gimp-error-console",
        GIMP_HELP_ERRORS_DIALOG } };

gint n_dialogs_dockable_actions = G_N_ELEMENTS(dialogs_dockable_actions);

static const GimpStringActionEntry dialogs_toplevel_actions[] =
  {
    { "dialogs-preferences", GTK_STOCK_PREFERENCES, NC_("dialogs-action",
                                                        "_Preferences"), NULL,
        NC_("dialogs-action", "Open the preferences dialog"),
        "gimp-preferences-dialog",
        GIMP_HELP_PREFS_DIALOG },

    { "dialogs-input-devices", GIMP_STOCK_INPUT_DEVICE, NC_("dialogs-action",
                                                            "_Input Devices"),
        NULL, NC_("dialogs-action", "Open the input devices editor"),
        "gimp-input-devices-dialog",
        GIMP_HELP_INPUT_DEVICES },

    { "dialogs-keyboard-shortcuts", GIMP_STOCK_CHAR_PICKER, NC_(
        "dialogs-action", "_Keyboard Shortcuts"), NULL, NC_(
        "dialogs-action", "Open the keyboard shortcuts editor"),
        "gimp-keyboard-shortcuts-dialog",
        GIMP_HELP_KEYBOARD_SHORTCUTS },

    { "dialogs-module-dialog", GTK_STOCK_EXECUTE, NC_("dialogs-action",
                                                      "_Modules"), NULL, NC_(
        "dialogs-action", "Open the module manager dialog"),
        "gimp-module-dialog",
        GIMP_HELP_MODULE_DIALOG },

    { "dialogs-tips", GIMP_STOCK_INFO, NC_("dialogs-action", "_Tip of the Day"),
        NULL, NC_("dialogs-action", "Show some helpful tips on using GIMP"),
        "gimp-tips-dialog",
        GIMP_HELP_TIPS_DIALOG },

    { "dialogs-about", GTK_STOCK_ABOUT,
#if defined (G_OS_WIN32)
        NC_("dialogs-action", "About GIMP"), NULL,
#elif defined (PLATFORM_OSX)
        NC_("dialogs-action", "About"), NULL,
#else /* UNIX: use GNOME HIG */
        NC_("dialogs-action", "_About"), NULL,
#endif
        NC_("dialogs-action", "About GIMP"), "gimp-about-dialog",
        GIMP_HELP_ABOUT_DIALOG } };

static gboolean
dialogs_actions_toolbox_exists (Gimp *gimp)
{
  GimpDialogFactory *factory = gimp_dialog_factory_get_singleton ();
  gboolean toolbox_found = FALSE;
  GList *iter;

  /* First look in session managed windows */
  toolbox_found = gimp_dialog_factory_find_widget (
      factory, "gimp-toolbox-window") != NULL;

  /* Then in image windows */
  if (!toolbox_found)
    {
      GList *windows = gimp ? gimp_get_image_windows (gimp) : NULL;

      for (iter = windows; iter; iter = g_list_next(iter))
        {
          GimpImageWindow *window = GIMP_IMAGE_WINDOW(windows->data);

          if (gimp_image_window_has_toolbox (window))
            {
              toolbox_found = TRUE;
              break;
            }
        }

      g_list_free (windows);
    }

  return toolbox_found;
}

void
dialogs_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_string_actions (
      group, "dialogs-action", dialogs_dockable_actions,
      G_N_ELEMENTS(dialogs_dockable_actions),
      G_CALLBACK(dialogs_create_dockable_cmd_callback));

  gimp_action_group_add_string_actions (
      group, "dialogs-action", dialogs_toplevel_actions,
      G_N_ELEMENTS(dialogs_toplevel_actions),
      G_CALLBACK(dialogs_create_toplevel_cmd_callback));
}

void
dialogs_actions_update (GimpActionGroup *group, gpointer data)
{
  Gimp *gimp = action_data_get_gimp (data);
  const gchar *toolbox_label = NULL;
  const gchar *toolbox_tooltip = NULL;

  if (dialogs_actions_toolbox_exists (gimp))
    {
      toolbox_label = _("Toolbox");
      toolbox_tooltip = _("Raise the toolbox");
    }
  else
    {
      toolbox_label = _("New Toolbox");
      toolbox_tooltip = _("Create a new toolbox");
    }

  gimp_action_group_set_action_label (group, "dialogs-toolbox", toolbox_label);
  gimp_action_group_set_action_tooltip (group, "dialogs-toolbox",
                                        toolbox_tooltip);
}
