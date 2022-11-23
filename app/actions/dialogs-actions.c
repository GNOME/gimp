/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmatoolbox.h"

#include "display/ligmaimagewindow.h"

#include "actions.h"
#include "dialogs-actions.h"
#include "dialogs-commands.h"

#include "ligma-intl.h"


const LigmaStringActionEntry dialogs_dockable_actions[] =
{
  { "dialogs-toolbox", NULL,
    NC_("windows-action", "Tool_box"), "<primary>B",
    NULL /* set in dialogs_actions_update() */,
    "ligma-toolbox",
    LIGMA_HELP_TOOLBOX },

  { "dialogs-tool-options", LIGMA_ICON_DIALOG_TOOL_OPTIONS,
    NC_("dialogs-action", "Tool _Options"), NULL,
    NC_("dialogs-action", "Open the tool options dialog"),
    "ligma-tool-options",
    LIGMA_HELP_TOOL_OPTIONS_DIALOG },

  { "dialogs-device-status", LIGMA_ICON_DIALOG_DEVICE_STATUS,
    NC_("dialogs-action", "_Device Status"), NULL,
    NC_("dialogs-action", "Open the device status dialog"),
    "ligma-device-status",
    LIGMA_HELP_DEVICE_STATUS_DIALOG },

  { "dialogs-symmetry", LIGMA_ICON_SYMMETRY,
    NC_("dialogs-action", "_Symmetry Painting"), NULL,
    NC_("dialogs-action", "Open the symmetry dialog"),
    "ligma-symmetry-editor",
    LIGMA_HELP_SYMMETRY_DIALOG },

  { "dialogs-layers", LIGMA_ICON_DIALOG_LAYERS,
    NC_("dialogs-action", "_Layers"), "<primary>L",
    NC_("dialogs-action", "Open the layers dialog"),
    "ligma-layer-list",
    LIGMA_HELP_LAYER_DIALOG },

  { "dialogs-channels", LIGMA_ICON_DIALOG_CHANNELS,
    NC_("dialogs-action", "_Channels"), NULL,
    NC_("dialogs-action", "Open the channels dialog"),
    "ligma-channel-list",
    LIGMA_HELP_CHANNEL_DIALOG },

  { "dialogs-vectors", LIGMA_ICON_DIALOG_PATHS,
    NC_("dialogs-action", "_Paths"), NULL,
    NC_("dialogs-action", "Open the paths dialog"),
    "ligma-vectors-list",
    LIGMA_HELP_PATH_DIALOG },

  { "dialogs-indexed-palette", LIGMA_ICON_COLORMAP,
    NC_("dialogs-action", "Color_map"), NULL,
    NC_("dialogs-action", "Open the colormap dialog"),
    "ligma-indexed-palette",
    LIGMA_HELP_INDEXED_PALETTE_DIALOG },

  { "dialogs-histogram", LIGMA_ICON_HISTOGRAM,
    NC_("dialogs-action", "Histogra_m"), NULL,
    NC_("dialogs-action", "Open the histogram dialog"),
    "ligma-histogram-editor",
    LIGMA_HELP_HISTOGRAM_DIALOG },

  { "dialogs-selection-editor", LIGMA_ICON_SELECTION,
    NC_("dialogs-action", "_Selection Editor"), NULL,
    NC_("dialogs-action", "Open the selection editor"),
    "ligma-selection-editor",
    LIGMA_HELP_SELECTION_DIALOG },

  { "dialogs-navigation", LIGMA_ICON_DIALOG_NAVIGATION,
    NC_("dialogs-action", "Na_vigation"), NULL,
    NC_("dialogs-action", "Open the display navigation dialog"),
    "ligma-navigation-view",
    LIGMA_HELP_NAVIGATION_DIALOG },

  { "dialogs-undo-history", LIGMA_ICON_DIALOG_UNDO_HISTORY,
    NC_("dialogs-action", "Undo _History"), NULL,
    NC_("dialogs-action", "Open the undo history dialog"),
    "ligma-undo-history",
    LIGMA_HELP_UNDO_DIALOG },

  { "dialogs-cursor", LIGMA_ICON_CURSOR,
    NC_("dialogs-action", "_Pointer"), NULL,
    NC_("dialogs-action", "Open the pointer information dialog"),
    "ligma-cursor-view",
    LIGMA_HELP_POINTER_INFO_DIALOG },

  { "dialogs-sample-points", LIGMA_ICON_SAMPLE_POINT,
    NC_("dialogs-action", "_Sample Points"), NULL,
    NC_("dialogs-action", "Open the sample points dialog"),
    "ligma-sample-point-editor",
    LIGMA_HELP_SAMPLE_POINT_DIALOG },

  { "dialogs-colors", LIGMA_ICON_COLORS_DEFAULT,
    NC_("dialogs-action", "Colo_rs"), NULL,
    NC_("dialogs-action", "Open the FG/BG color dialog"),
    "ligma-color-editor",
    LIGMA_HELP_COLOR_DIALOG },

  { "dialogs-brushes", LIGMA_ICON_BRUSH,
    NC_("dialogs-action", "_Brushes"), "<primary><shift>B",
    NC_("dialogs-action", "Open the brushes dialog"),
    "ligma-brush-grid|ligma-brush-list",
    LIGMA_HELP_BRUSH_DIALOG },

  { "dialogs-brush-editor", LIGMA_ICON_BRUSH,
    NC_("dialogs-action", "Brush Editor"), NULL,
    NC_("dialogs-action", "Open the brush editor"),
    "ligma-brush-editor",
    LIGMA_HELP_BRUSH_EDIT },

  { "dialogs-dynamics", LIGMA_ICON_DYNAMICS,
    NC_("dialogs-action", "Paint D_ynamics"), NULL,
    NC_("dialogs-action", "Open paint dynamics dialog"),
    "ligma-dynamics-list|ligma-dynamics-grid",
    LIGMA_HELP_DYNAMICS_DIALOG },

  { "dialogs-dynamics-editor", LIGMA_ICON_DYNAMICS,
    NC_("dialogs-action", "Paint Dynamics Editor"), NULL,
    NC_("dialogs-action", "Open the paint dynamics editor"),
    "ligma-dynamics-editor",
    LIGMA_HELP_DYNAMICS_EDITOR_DIALOG },

  { "dialogs-mypaint-brushes", LIGMA_ICON_MYPAINT_BRUSH,
    NC_("dialogs-action", "_MyPaint Brushes"), NULL,
    NC_("dialogs-action", "Open the mypaint brushes dialog"),
    "ligma-mypaint-brush-grid|ligma-mapyint-brush-list",
    LIGMA_HELP_MYPAINT_BRUSH_DIALOG },

  { "dialogs-patterns", LIGMA_ICON_PATTERN,
    NC_("dialogs-action", "P_atterns"), "<primary><shift>P",
    NC_("dialogs-action", "Open the patterns dialog"),
    "ligma-pattern-grid|ligma-pattern-list",
    LIGMA_HELP_PATTERN_DIALOG },

  { "dialogs-gradients", LIGMA_ICON_GRADIENT,
    NC_("dialogs-action", "_Gradients"), "<primary>G",
    NC_("dialogs-action", "Open the gradients dialog"),
    "ligma-gradient-list|ligma-gradient-grid",
    LIGMA_HELP_GRADIENT_DIALOG },

  { "dialogs-gradient-editor", LIGMA_ICON_GRADIENT,
    NC_("dialogs-action", "Gradient Editor"), NULL,
    NC_("dialogs-action", "Open the gradient editor"),
    "ligma-gradient-editor",
    LIGMA_HELP_GRADIENT_EDIT },

  { "dialogs-palettes", LIGMA_ICON_PALETTE,
    NC_("dialogs-action", "Pal_ettes"), NULL,
    NC_("dialogs-action", "Open the palettes dialog"),
    "ligma-palette-list|ligma-palette-grid",
    LIGMA_HELP_PALETTE_DIALOG },

  { "dialogs-palette-editor", LIGMA_ICON_PALETTE,
    NC_("dialogs-action", "Palette _Editor"), NULL,
    NC_("dialogs-action", "Open the palette editor"),
    "ligma-palette-editor",
    LIGMA_HELP_PALETTE_EDIT },

  { "dialogs-tool-presets", LIGMA_ICON_TOOL_PRESET,
    NC_("dialogs-action", "Tool Pre_sets"), NULL,
    NC_("dialogs-action", "Open tool presets dialog"),
    "ligma-tool-preset-list|ligma-tool-preset-grid",
    LIGMA_HELP_TOOL_PRESET_DIALOG },

  { "dialogs-fonts", LIGMA_ICON_FONT,
    NC_("dialogs-action", "_Fonts"), NULL,
    NC_("dialogs-action", "Open the fonts dialog"),
    "ligma-font-list|ligma-font-grid",
    LIGMA_HELP_FONT_DIALOG },

  { "dialogs-buffers", LIGMA_ICON_BUFFER,
    NC_("dialogs-action", "B_uffers"), "",
    NC_("dialogs-action", "Open the named buffers dialog"),
    "ligma-buffer-list|ligma-buffer-grid",
    LIGMA_HELP_BUFFER_DIALOG },

  { "dialogs-images", LIGMA_ICON_DIALOG_IMAGES,
    NC_("dialogs-action", "_Images"), NULL,
    NC_("dialogs-action", "Open the images dialog"),
    "ligma-image-list|ligma-image-grid",
    LIGMA_HELP_IMAGE_DIALOG },

  { "dialogs-document-history", LIGMA_ICON_DOCUMENT_OPEN_RECENT,
    NC_("dialogs-action", "Document Histor_y"), "",
    NC_("dialogs-action", "Open the document history dialog"),
    "ligma-document-list|ligma-document-grid",
    LIGMA_HELP_DOCUMENT_DIALOG },

  { "dialogs-templates", LIGMA_ICON_TEMPLATE,
    NC_("dialogs-action", "_Templates"), "",
    NC_("dialogs-action", "Open the image templates dialog"),
    "ligma-template-list|ligma-template-grid",
    LIGMA_HELP_TEMPLATE_DIALOG },

  { "dialogs-error-console", LIGMA_ICON_DIALOG_WARNING,
    NC_("dialogs-action", "Error Co_nsole"), NULL,
    NC_("dialogs-action", "Open the error console"),
    "ligma-error-console",
    LIGMA_HELP_ERRORS_DIALOG },

  { "dialogs-dashboard", LIGMA_ICON_DIALOG_DASHBOARD,
    NC_("dialogs-action", "_Dashboard"), NULL,
    NC_("dialogs-action", "Open the dashboard"),
    "ligma-dashboard",
    LIGMA_HELP_ERRORS_DIALOG }
};

gint n_dialogs_dockable_actions = G_N_ELEMENTS (dialogs_dockable_actions);

static const LigmaStringActionEntry dialogs_toplevel_actions[] =
{
  { "dialogs-preferences", LIGMA_ICON_PREFERENCES_SYSTEM,
    NC_("dialogs-action", "_Preferences"), NULL,
    NC_("dialogs-action", "Open the preferences dialog"),
    "ligma-preferences-dialog",
    LIGMA_HELP_PREFS_DIALOG },

  { "dialogs-input-devices", LIGMA_ICON_INPUT_DEVICE,
    NC_("dialogs-action", "_Input Devices"), NULL,
    NC_("dialogs-action", "Open the input devices editor"),
    "ligma-input-devices-dialog",
    LIGMA_HELP_INPUT_DEVICES },

  { "dialogs-keyboard-shortcuts", LIGMA_ICON_CHAR_PICKER,
    NC_("dialogs-action", "_Keyboard Shortcuts"), NULL,
    NC_("dialogs-action", "Open the keyboard shortcuts editor"),
    "ligma-keyboard-shortcuts-dialog",
    LIGMA_HELP_KEYBOARD_SHORTCUTS },

  { "dialogs-module-dialog", LIGMA_ICON_SYSTEM_RUN,
    NC_("dialogs-action", "_Modules"), NULL,
    NC_("dialogs-action", "Open the module manager dialog"),
    "ligma-module-dialog",
    LIGMA_HELP_MODULE_DIALOG },

  { "dialogs-tips", LIGMA_ICON_DIALOG_INFORMATION,
    NC_("dialogs-action", "_Tip of the Day"), NULL,
    NC_("dialogs-action", "Show some helpful tips on using LIGMA"),
    "ligma-tips-dialog",
    LIGMA_HELP_TIPS_DIALOG },

  { "dialogs-welcome", LIGMA_ICON_DIALOG_INFORMATION,
    NC_("dialogs-action", "Welcome Dialog"), NULL,
    NC_("dialogs-action", "Show information on running LIGMA release"),
    "ligma-welcome-dialog",
    LIGMA_HELP_WELCOME_DIALOG },

  { "dialogs-about", LIGMA_ICON_HELP_ABOUT,
#if defined(G_OS_WIN32)
    NC_("dialogs-action", "About LIGMA"),
#elif defined(PLATFORM_OSX)
    NC_("dialogs-action", "About"),
#else /* UNIX: use GNOME HIG */
    NC_("dialogs-action", "_About"),
#endif
    NULL,
    NC_("dialogs-action", "About LIGMA"),
    "ligma-about-dialog",
      LIGMA_HELP_ABOUT_DIALOG },

  { "dialogs-action-search", LIGMA_ICON_TOOL_ZOOM,
    NC_("dialogs-action", "_Search and Run a Command"), "slash",
    NC_("dialogs-action", "Search commands by keyword, and run them"),
    "ligma-action-search-dialog",
    LIGMA_HELP_ACTION_SEARCH_DIALOG },

  { "dialogs-extensions", LIGMA_ICON_PLUGIN,
    NC_("dialogs-action", "Manage _Extensions"), NULL,
    NC_("dialogs-action", "Manage Extensions: search, install, uninstall, update."),
    "ligma-extensions-dialog",
    LIGMA_HELP_EXTENSIONS_DIALOG }
};


gboolean
dialogs_actions_toolbox_exists (Ligma *ligma)
{
  LigmaDialogFactory *factory       = ligma_dialog_factory_get_singleton ();
  gboolean           toolbox_found = FALSE;
  GList             *iter;

  /* First look in session managed windows */
  toolbox_found =
    ligma_dialog_factory_find_widget (factory, "ligma-toolbox-window") != NULL;

  /* Then in image windows */
  if (! toolbox_found)
    {
      GList *windows = ligma ? ligma_get_image_windows (ligma) : NULL;

      for (iter = windows; iter; iter = g_list_next (iter))
        {
          LigmaImageWindow *window = LIGMA_IMAGE_WINDOW (windows->data);

          if (ligma_image_window_has_toolbox (window))
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
dialogs_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_string_actions (group, "dialogs-action",
                                        dialogs_dockable_actions,
                                        G_N_ELEMENTS (dialogs_dockable_actions),
                                        dialogs_create_dockable_cmd_callback);

  ligma_action_group_add_string_actions (group, "dialogs-action",
                                        dialogs_toplevel_actions,
                                        G_N_ELEMENTS (dialogs_toplevel_actions),
                                        dialogs_create_toplevel_cmd_callback);
}

void
dialogs_actions_update (LigmaActionGroup *group,
                        gpointer         data)
{
  Ligma        *ligma            = action_data_get_ligma (data);
  const gchar *toolbox_label   = NULL;
  const gchar *toolbox_tooltip = NULL;

  if (dialogs_actions_toolbox_exists (ligma))
    {
      toolbox_label   = _("Tool_box");
      toolbox_tooltip = _("Raise the toolbox");
    }
  else
    {
      toolbox_label   = _("New Tool_box");
      toolbox_tooltip = _("Create a new toolbox");
    }

  ligma_action_group_set_action_label (group, "dialogs-toolbox", toolbox_label);
  ligma_action_group_set_action_tooltip (group, "dialogs-toolbox", toolbox_tooltip);
}
