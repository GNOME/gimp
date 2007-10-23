/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "dialogs-actions.h"
#include "dialogs-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry dialogs_actions[] =
{
  { "dialogs-menu",          NULL, N_("_Dialogs")         },
  { "dialogs-new-dock-menu", NULL, N_("Create New Doc_k") },

  { "dialogs-new-dock-lcp", NULL,
    N_("_Layers, Channels & Paths"), NULL,
    N_("Open a Layers, Channels & Paths dock"),
    G_CALLBACK (dialogs_create_lc_cmd_callback),
    GIMP_HELP_DOCK },

  { "dialogs-new-dock-data", NULL,
    N_("_Brushes, Patterns & Gradients"), NULL,
    N_("Open a Brushes, Patterns & Gradients dock"),
    G_CALLBACK (dialogs_create_data_cmd_callback),
    GIMP_HELP_DOCK },

  { "dialogs-new-dock-stuff", NULL,
    N_("_Misc. Stuff"), NULL,
    N_("Open a dock containing miscellaneous dialogs"),
    G_CALLBACK (dialogs_create_stuff_cmd_callback),
    GIMP_HELP_DOCK },

  { "dialogs-toolbox", NULL,
    N_("Tool_box"), "<control>B",
    N_("Raise the toolbox"),
    G_CALLBACK (dialogs_show_toolbox_cmd_callback),
    GIMP_HELP_TOOLBOX }
};

const GimpStringActionEntry dialogs_dockable_actions[] =
{
  { "dialogs-tool-options", GIMP_STOCK_TOOL_OPTIONS,
    N_("Tool _Options"), NULL,
    N_("Open the tool options dialog"),
    "gimp-tool-options",
    GIMP_HELP_TOOL_OPTIONS_DIALOG },

  { "dialogs-device-status", GIMP_STOCK_DEVICE_STATUS,
    N_("_Device Status"), NULL,
    N_("Open the device status dialog"),
    "gimp-device-status",
    GIMP_HELP_DEVICE_STATUS_DIALOG },

  { "dialogs-layers", GIMP_STOCK_LAYERS,
    N_("_Layers"), "<control>L",
    N_("Open the layers dialog"),
    "gimp-layer-list",
    GIMP_HELP_LAYER_DIALOG },

  { "dialogs-channels", GIMP_STOCK_CHANNELS,
    N_("_Channels"), NULL,
    N_("Open the channels dialog"),
    "gimp-channel-list",
    GIMP_HELP_CHANNEL_DIALOG },

  { "dialogs-vectors", GIMP_STOCK_PATHS,
    N_("_Paths"), NULL,
    N_("Open the paths dialog"),
    "gimp-vectors-list",
    GIMP_HELP_PATH_DIALOG },

  { "dialogs-indexed-palette", GIMP_STOCK_COLORMAP,
    N_("Color_map"), NULL,
    N_("Open the colormap dialog"),
    "gimp-indexed-palette",
    GIMP_HELP_INDEXED_PALETTE_DIALOG },

  { "dialogs-histogram", GIMP_STOCK_HISTOGRAM,
    N_("Histogra_m"), NULL,
    N_("Open the histogram dialog"),
    "gimp-histogram-editor",
    GIMP_HELP_HISTOGRAM_DIALOG },

  { "dialogs-selection-editor", GIMP_STOCK_TOOL_RECT_SELECT,
    N_("_Selection Editor"), NULL,
    N_("Open the selection editor"),
    "gimp-selection-editor",
    GIMP_HELP_SELECTION_DIALOG },

  { "dialogs-navigation", GIMP_STOCK_NAVIGATION,
    N_("Na_vigation"), NULL,
    N_("Open the display navigation dialog"),
    "gimp-navigation-view",
    GIMP_HELP_NAVIGATION_DIALOG },

  { "dialogs-undo-history", GIMP_STOCK_UNDO_HISTORY,
    N_("Undo _History"), NULL,
    N_("Open the undo history dialog"),
    "gimp-undo-history",
    GIMP_HELP_UNDO_DIALOG },

  { "dialogs-cursor", GIMP_STOCK_CURSOR,
    N_("Pointer"), NULL,
    N_("Open the pointer information dialog"),
    "gimp-cursor-view",
    GIMP_HELP_POINTER_INFO_DIALOG },

  { "dialogs-sample-points", GIMP_STOCK_SAMPLE_POINT,
    N_("_Sample Points"), NULL,
    N_("Open the sample points dialog"),
    "gimp-sample-point-editor",
    GIMP_HELP_SAMPLE_POINT_DIALOG },

  { "dialogs-colors", GIMP_STOCK_DEFAULT_COLORS,
    N_("Colo_rs"), NULL,
    N_("Open the FG/BG color dialog"),
    "gimp-color-editor",
    GIMP_HELP_COLOR_DIALOG },

  { "dialogs-brushes", GIMP_STOCK_BRUSH,
    N_("_Brushes"), "<control><shift>B",
    N_("Open the brushes dialog"),
    "gimp-brush-grid|gimp-brush-list",
    GIMP_HELP_BRUSH_DIALOG },

  { "dialogs-patterns", GIMP_STOCK_PATTERN,
    N_("P_atterns"), "<control><shift>P",
    N_("Open the patterns dialog"),
    "gimp-pattern-grid|gimp-pattern-list",
    GIMP_HELP_PATTERN_DIALOG },

  { "dialogs-gradients", GIMP_STOCK_GRADIENT,
    N_("_Gradients"), "<control>G",
    N_("Open the gradients dialog"),
    "gimp-gradient-list|gimp-gradient-grid",
    GIMP_HELP_GRADIENT_DIALOG },

  { "dialogs-palettes", GIMP_STOCK_PALETTE,
    N_("Pal_ettes"), NULL,
    N_("Open the palettes dialog"),
    "gimp-palette-list|gimp-palette-grid",
    GIMP_HELP_PALETTE_DIALOG },

  { "dialogs-fonts", GIMP_STOCK_FONT,
    N_("_Fonts"), NULL,
    N_("Open the fonts dialog"),
    "gimp-font-list|gimp-font-grid",
    GIMP_HELP_FONT_DIALOG },

  { "dialogs-buffers", GIMP_STOCK_BUFFER,
    N_("B_uffers"), "",
    N_("Open the named buffers dialog"),
    "gimp-buffer-list|gimp-buffer-grid",
    GIMP_HELP_BUFFER_DIALOG },

  { "dialogs-images", GIMP_STOCK_IMAGES,
    N_("_Images"), NULL,
    N_("Open the images dialog"),
    "gimp-image-list|gimp-image-grid",
    GIMP_HELP_IMAGE_DIALOG },

  { "dialogs-document-history", GTK_STOCK_OPEN,
    N_("Document Histor_y"), "",
    N_("Open the document history dialog"),
    "gimp-document-list|gimp-document-grid",
    GIMP_HELP_DOCUMENT_DIALOG },

  { "dialogs-templates", GIMP_STOCK_TEMPLATE,
    N_("_Templates"), "",
    N_("Open the image templates dialog"),
    "gimp-template-list|gimp-template-grid",
    GIMP_HELP_TEMPLATE_DIALOG },

  { "dialogs-tools", GIMP_STOCK_TOOLS,
    N_("T_ools"), NULL,
    N_("Open the tools dialog"),
    "gimp-tool-list|gimp-tool-grid",
    GIMP_HELP_TOOLS_DIALOG },

  { "dialogs-error-console", GIMP_STOCK_WARNING,
    N_("Error Co_nsole"), NULL,
    N_("Open the error console"),
    "gimp-error-console",
    GIMP_HELP_ERRORS_DIALOG }
};

gint n_dialogs_dockable_actions = G_N_ELEMENTS (dialogs_dockable_actions);

static const GimpStringActionEntry dialogs_toplevel_actions[] =
{
  { "dialogs-preferences", GTK_STOCK_PREFERENCES,
    N_("_Preferences"), NULL,
    N_("Open the preferences dialog"),
    "gimp-preferences-dialog",
    GIMP_HELP_PREFS_DIALOG },

  { "dialogs-keyboard-shortcuts", GIMP_STOCK_CHAR_PICKER,
    N_("_Keyboard Shortcuts"), NULL,
    N_("Open the keyboard shortcuts editor"),
    "gimp-keyboard-shortcuts-dialog",
    GIMP_HELP_KEYBOARD_SHORTCUTS },

  { "dialogs-module-dialog", GTK_STOCK_EXECUTE,
    N_("_Module Manager"), NULL,
    N_("Open the module manager dialog"),
    "gimp-module-dialog",
    GIMP_HELP_MODULE_DIALOG },

  { "dialogs-tips", GIMP_STOCK_INFO,
    N_("_Tip of the Day"), NULL,
    N_("Show the tip of the day"),
    "gimp-tips-dialog",
    GIMP_HELP_TIPS_DIALOG },

  { "dialogs-about", GTK_STOCK_ABOUT,
    N_("_About"), NULL,
    N_("About GIMP"),
    "gimp-about-dialog",
    GIMP_HELP_ABOUT_DIALOG }
};


void
dialogs_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group,
                                 dialogs_actions,
                                 G_N_ELEMENTS (dialogs_actions));

  gimp_action_group_add_string_actions (group,
                                        dialogs_dockable_actions,
                                        G_N_ELEMENTS (dialogs_dockable_actions),
                                        G_CALLBACK (dialogs_create_dockable_cmd_callback));

  gimp_action_group_add_string_actions (group,
                                        dialogs_toplevel_actions,
                                        G_N_ELEMENTS (dialogs_toplevel_actions),
                                        G_CALLBACK (dialogs_create_toplevel_cmd_callback));
}

void
dialogs_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
}
