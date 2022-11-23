/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * dialogs.c
 * Copyright (C) 2010 Martin Nordholts <martinn@src.gnome.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "config/ligmaguiconfig.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmalist.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadockwindow.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmasessioninfo-aux.h"
#include "widgets/ligmasessionmanaged.h"
#include "widgets/ligmatoolbox.h"

#include "dialogs.h"
#include "dialogs-constructors.h"

#include "ligma-log.h"

#include "ligma-intl.h"


LigmaContainer *global_recent_docks = NULL;


#define FOREIGN(id, singleton, remember_size) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    NULL                   /* new_func         */, \
    dialogs_restore_dialog /* restore_func     */, \
    0                      /* view_size        */, \
    singleton              /* singleton        */, \
    TRUE                   /* session_managed  */, \
    remember_size          /* remember_size    */, \
    FALSE                  /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    FALSE                  /* dockable         */}

#define IMAGE_WINDOW(id, singleton, remember_size) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    NULL                   /* new_func         */, \
    dialogs_restore_window /* restore_func     */, \
    0                      /* view_size        */, \
    singleton              /* singleton        */, \
    TRUE                   /* session_managed  */, \
    remember_size          /* remember_size    */, \
    TRUE                   /* remember_if_open */, \
    FALSE                  /* hideable         */, \
    TRUE                   /* image_window     */, \
    FALSE                  /* dockable         */}

#define TOPLEVEL(id, new_func, singleton, session_managed, remember_size) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    new_func               /* new_func         */, \
    dialogs_restore_dialog /* restore_func     */, \
    0                      /* view_size        */, \
    singleton              /* singleton        */, \
    session_managed        /* session_managed  */, \
    remember_size          /* remember_size    */, \
    FALSE                  /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    FALSE                  /* dockable         */}

#define DOCKABLE(id, name, blurb, icon_name, help_id, new_func, view_size, singleton) \
  { id                     /* identifier       */, \
    name                   /* name             */, \
    blurb                  /* blurb            */, \
    icon_name              /* icon_name        */, \
    help_id                /* help_id          */, \
    new_func               /* new_func         */, \
    NULL                   /* restore_func     */, \
    view_size              /* view_size        */, \
    singleton              /* singleton        */, \
    FALSE                  /* session_managed  */, \
    FALSE                  /* remember_size    */, \
    TRUE                   /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    TRUE                   /* dockable         */}

#define DOCK(id, new_func) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    new_func               /* new_func         */, \
    dialogs_restore_dialog /* restore_func     */, \
    0                      /* view_size        */, \
    FALSE                  /* singleton        */, \
    FALSE                  /* session_managed  */, \
    FALSE                  /* remember_size    */, \
    FALSE                  /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    FALSE                  /* dockable         */}

#define DOCK_WINDOW(id, new_func) \
  { id                     /* identifier       */, \
    NULL                   /* name             */, \
    NULL                   /* blurb            */, \
    NULL                   /* icon_name        */, \
    NULL                   /* help_id          */, \
    new_func               /* new_func         */, \
    dialogs_restore_dialog /* restore_func     */, \
    0                      /* view_size        */, \
    FALSE                  /* singleton        */, \
    TRUE                   /* session_managed  */, \
    TRUE                   /* remember_size    */, \
    TRUE                   /* remember_if_open */, \
    TRUE                   /* hideable         */, \
    FALSE                  /* image_window     */, \
    FALSE                  /* dockable         */}

#define LISTGRID(id, new_func, name, blurb, icon_name, help_id, view_size) \
  { "ligma-"#id"-list"             /* identifier       */,  \
    name                          /* name             */,  \
    blurb                         /* blurb            */,  \
    icon_name                     /* icon_name        */,  \
    help_id                       /* help_id          */,  \
    dialogs_##new_func##_list_view_new /* new_func         */,  \
    NULL                          /* restore_func     */,  \
    view_size                     /* view_size        */,  \
    FALSE                         /* singleton        */,  \
    FALSE                         /* session_managed  */,  \
    FALSE                         /* remember_size    */,  \
    TRUE                          /* remember_if_open */,  \
    TRUE                          /* hideable         */,  \
    FALSE                         /* image_window     */,  \
    TRUE                          /* dockable         */}, \
  { "ligma-"#id"-grid"             /* identifier       */,  \
    name                          /* name             */,  \
    blurb                         /* blurb            */,  \
    icon_name                     /* icon_name        */,  \
    help_id                       /* help_id          */,  \
    dialogs_##new_func##_grid_view_new /* new_func         */,  \
    NULL                          /* restore_func     */,  \
    view_size                     /* view_size        */,  \
    FALSE                         /* singleton        */,  \
    FALSE                         /* session_managed  */,  \
    FALSE                         /* remember_size    */,  \
    TRUE                          /* remember_if_open */,  \
    TRUE                          /* hideable         */,  \
    FALSE                         /* image_window     */,  \
    TRUE                          /* dockable         */}

#define LIST(id, new_func, name, blurb, icon_name, help_id, view_size) \
  { "ligma-"#id"-list"                   /* identifier       */, \
    name                                /* name             */, \
    blurb                               /* blurb            */, \
    icon_name                            /* icon_name         */, \
    help_id                             /* help_id          */, \
    dialogs_##new_func##_list_view_new  /* new_func         */, \
    NULL                                /* restore_func     */, \
    view_size                           /* view_size        */, \
    FALSE                               /* singleton        */, \
    FALSE                               /* session_managed  */, \
    FALSE                               /* remember_size    */, \
    TRUE                                /* remember_if_open */, \
    TRUE                                /* hideable         */, \
    FALSE                               /* image_window     */, \
    TRUE                                /* dockable         */}


static GtkWidget * dialogs_restore_dialog (LigmaDialogFactory *factory,
                                           GdkMonitor        *monitor,
                                           LigmaSessionInfo   *info);
static GtkWidget * dialogs_restore_window (LigmaDialogFactory *factory,
                                           GdkMonitor        *monitor,
                                           LigmaSessionInfo   *info);


static const LigmaDialogFactoryEntry entries[] =
{
  /*  foreign toplevels without constructor  */
  FOREIGN ("ligma-brightness-contrast-tool-dialog", TRUE,  FALSE),
  FOREIGN ("ligma-color-balance-tool-dialog",       TRUE,  FALSE),
  FOREIGN ("ligma-color-picker-tool-dialog",        TRUE,  TRUE),
  FOREIGN ("ligma-colorize-tool-dialog",            TRUE,  FALSE),
  FOREIGN ("ligma-crop-tool-dialog",                TRUE,  FALSE),
  FOREIGN ("ligma-curves-tool-dialog",              TRUE,  TRUE),
  FOREIGN ("ligma-desaturate-tool-dialog",          TRUE,  FALSE),
  FOREIGN ("ligma-foreground-select-tool-dialog",   TRUE,  FALSE),
  FOREIGN ("ligma-gegl-tool-dialog",                TRUE,  FALSE),
  FOREIGN ("ligma-gradient-tool-dialog",            TRUE,  FALSE),
  FOREIGN ("ligma-hue-saturation-tool-dialog",      TRUE,  FALSE),
  FOREIGN ("ligma-levels-tool-dialog",              TRUE,  TRUE),
  FOREIGN ("ligma-measure-tool-dialog",             TRUE,  FALSE),
  FOREIGN ("ligma-offset-tool-dialog",              TRUE,  FALSE),
  FOREIGN ("ligma-operation-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("ligma-posterize-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("ligma-rotate-tool-dialog",              TRUE,  FALSE),
  FOREIGN ("ligma-scale-tool-dialog",               TRUE,  FALSE),
  FOREIGN ("ligma-shear-tool-dialog",               TRUE,  FALSE),
  FOREIGN ("ligma-text-tool-dialog",                TRUE,  TRUE),
  FOREIGN ("ligma-threshold-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("ligma-transform-3d-tool-dialog",        TRUE,  FALSE),
  FOREIGN ("ligma-perspective-tool-dialog",         TRUE,  FALSE),
  FOREIGN ("ligma-unified-transform-tool-dialog",   TRUE,  FALSE),
  FOREIGN ("ligma-handle-transform-tool-dialog",    TRUE,  FALSE),

  FOREIGN ("ligma-toolbox-color-dialog",            TRUE,  FALSE),
  FOREIGN ("ligma-gradient-editor-color-dialog",    TRUE,  FALSE),
  FOREIGN ("ligma-palette-editor-color-dialog",     TRUE,  FALSE),
  FOREIGN ("ligma-colormap-editor-color-dialog",    TRUE,  FALSE),
  FOREIGN ("ligma-colormap-selection-color-dialog", TRUE,  FALSE),

  FOREIGN ("ligma-controller-editor-dialog",        FALSE, TRUE),
  FOREIGN ("ligma-controller-action-dialog",        FALSE, TRUE),

  /*  ordinary toplevels  */
  TOPLEVEL ("ligma-image-new-dialog",
            dialogs_image_new_new,          FALSE, TRUE, FALSE),
  TOPLEVEL ("ligma-file-open-dialog",
            dialogs_file_open_new,          TRUE,  TRUE, TRUE),
  TOPLEVEL ("ligma-file-open-location-dialog",
            dialogs_file_open_location_new, FALSE, TRUE, FALSE),
  TOPLEVEL ("ligma-file-save-dialog",
            dialogs_file_save_new,          FALSE, TRUE, TRUE),
  TOPLEVEL ("ligma-file-export-dialog",
            dialogs_file_export_new,        FALSE, TRUE, TRUE),

  /*  singleton toplevels  */
  TOPLEVEL ("ligma-preferences-dialog",
            dialogs_preferences_get,        TRUE, TRUE,  TRUE),
  TOPLEVEL ("ligma-input-devices-dialog",
            dialogs_input_devices_get,      TRUE, TRUE,  FALSE),
  TOPLEVEL ("ligma-keyboard-shortcuts-dialog",
            dialogs_keyboard_shortcuts_get, TRUE, TRUE,  TRUE),
  TOPLEVEL ("ligma-module-dialog",
            dialogs_module_get,             TRUE, TRUE,  TRUE),
  TOPLEVEL ("ligma-palette-import-dialog",
            dialogs_palette_import_get,     TRUE, TRUE,  TRUE),
  TOPLEVEL ("ligma-tips-dialog",
            dialogs_tips_get,               TRUE, FALSE, FALSE),
  TOPLEVEL ("ligma-welcome-dialog",
            dialogs_welcome_get,            TRUE, FALSE, FALSE),
  TOPLEVEL ("ligma-about-dialog",
            dialogs_about_get,              TRUE, FALSE, FALSE),
  TOPLEVEL ("ligma-action-search-dialog",
            dialogs_action_search_get,      TRUE, TRUE,  TRUE),
  TOPLEVEL ("ligma-error-dialog",
            dialogs_error_get,              TRUE, FALSE, FALSE),
  TOPLEVEL ("ligma-critical-dialog",
            dialogs_critical_get,           TRUE, FALSE, FALSE),
  TOPLEVEL ("ligma-close-all-dialog",
            dialogs_close_all_get,          TRUE, FALSE, FALSE),
  TOPLEVEL ("ligma-quit-dialog",
            dialogs_quit_get,               TRUE, FALSE, FALSE),
  TOPLEVEL ("ligma-extensions-dialog",
            dialogs_extensions_get,         TRUE, TRUE,  TRUE),

  /*  docks  */
  DOCK ("ligma-dock",
        dialogs_dock_new),
  DOCK ("ligma-toolbox",
        dialogs_toolbox_new),

  /*  dock windows  */
  DOCK_WINDOW ("ligma-dock-window",
               dialogs_dock_window_new),
  DOCK_WINDOW ("ligma-toolbox-window",
               dialogs_toolbox_dock_window_new),

  /*  singleton dockables  */
  DOCKABLE ("ligma-tool-options",
            N_("Tool Options"), NULL, LIGMA_ICON_DIALOG_TOOL_OPTIONS,
            LIGMA_HELP_TOOL_OPTIONS_DIALOG,
            dialogs_tool_options_new, 0, TRUE),
  DOCKABLE ("ligma-device-status",
            N_("Devices"), N_("Device Status"), LIGMA_ICON_DIALOG_DEVICE_STATUS,
            LIGMA_HELP_DEVICE_STATUS_DIALOG,
            dialogs_device_status_new, 0, TRUE),
  DOCKABLE ("ligma-error-console",
            N_("Errors"), N_("Error Console"), LIGMA_ICON_DIALOG_WARNING,
            LIGMA_HELP_ERRORS_DIALOG,
            dialogs_error_console_new, 0, TRUE),
  DOCKABLE ("ligma-cursor-view",
            N_("Pointer"), N_("Pointer Information"), LIGMA_ICON_CURSOR,
            LIGMA_HELP_POINTER_INFO_DIALOG,
            dialogs_cursor_view_new, 0, TRUE),
  DOCKABLE ("ligma-dashboard",
            N_("Dashboard"), N_("Dashboard"), LIGMA_ICON_DIALOG_DASHBOARD,
            LIGMA_HELP_DASHBOARD_DIALOG,
            dialogs_dashboard_new, 0, TRUE),

  /*  list & grid views  */
  LISTGRID (image, image,
            N_("Images"), NULL, LIGMA_ICON_DIALOG_IMAGES,
            LIGMA_HELP_IMAGE_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (brush, brush,
            N_("Brushes"), NULL, LIGMA_ICON_BRUSH,
            LIGMA_HELP_BRUSH_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (dynamics, dynamics,
            N_("Paint Dynamics"), NULL, LIGMA_ICON_DYNAMICS,
            LIGMA_HELP_DYNAMICS_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (mypaint-brush, mypaint_brush,
            N_("MyPaint Brushes"), NULL, LIGMA_ICON_MYPAINT_BRUSH,
            LIGMA_HELP_MYPAINT_BRUSH_DIALOG, LIGMA_VIEW_SIZE_LARGE),
  LISTGRID (pattern, pattern,
            N_("Patterns"), NULL, LIGMA_ICON_PATTERN,
            LIGMA_HELP_PATTERN_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (gradient, gradient,
            N_("Gradients"), NULL, LIGMA_ICON_GRADIENT,
            LIGMA_HELP_GRADIENT_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (palette, palette,
            N_("Palettes"), NULL, LIGMA_ICON_PALETTE,
            LIGMA_HELP_PALETTE_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (font, font,
            N_("Fonts"), NULL, LIGMA_ICON_FONT,
            LIGMA_HELP_FONT_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (buffer, buffer,
            N_("Buffers"), NULL, LIGMA_ICON_BUFFER,
            LIGMA_HELP_BUFFER_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (tool-preset, tool_preset,
            N_("Tool Presets"), NULL, LIGMA_ICON_TOOL_PRESET,
            LIGMA_HELP_TOOL_PRESET_DIALOG, LIGMA_VIEW_SIZE_MEDIUM),
  LISTGRID (document, document,
            N_("History"), N_("Document History"), LIGMA_ICON_DOCUMENT_OPEN_RECENT,
            LIGMA_HELP_DOCUMENT_DIALOG, LIGMA_VIEW_SIZE_LARGE),
  LISTGRID (template, template,
            N_("Templates"), N_("Image Templates"), LIGMA_ICON_TEMPLATE,
            LIGMA_HELP_TEMPLATE_DIALOG, LIGMA_VIEW_SIZE_SMALL),

  /*  image related  */
  DOCKABLE ("ligma-layer-list",
            N_("Layers"), NULL, LIGMA_ICON_DIALOG_LAYERS,
            LIGMA_HELP_LAYER_DIALOG,
            dialogs_layer_list_view_new, 0, FALSE),
  DOCKABLE ("ligma-channel-list",
            N_("Channels"), NULL, LIGMA_ICON_DIALOG_CHANNELS,
            LIGMA_HELP_CHANNEL_DIALOG,
            dialogs_channel_list_view_new, 0, FALSE),
  DOCKABLE ("ligma-vectors-list",
            N_("Paths"), NULL, LIGMA_ICON_DIALOG_PATHS,
            LIGMA_HELP_PATH_DIALOG,
            dialogs_vectors_list_view_new, 0, FALSE),
  DOCKABLE ("ligma-indexed-palette",
            N_("Colormap"), NULL, LIGMA_ICON_COLORMAP,
            LIGMA_HELP_INDEXED_PALETTE_DIALOG,
            dialogs_colormap_editor_new, 0, FALSE),
  DOCKABLE ("ligma-histogram-editor",
            N_("Histogram"), NULL, LIGMA_ICON_HISTOGRAM,
            LIGMA_HELP_HISTOGRAM_DIALOG,
            dialogs_histogram_editor_new, 0, FALSE),
  DOCKABLE ("ligma-selection-editor",
            N_("Selection"), N_("Selection Editor"), LIGMA_ICON_SELECTION,
            LIGMA_HELP_SELECTION_DIALOG,
            dialogs_selection_editor_new, 0, FALSE),
  DOCKABLE ("ligma-symmetry-editor",
            N_("Symmetry Painting"), NULL, LIGMA_ICON_SYMMETRY,
            LIGMA_HELP_SYMMETRY_DIALOG,
            dialogs_symmetry_editor_new, 0, FALSE),
  DOCKABLE ("ligma-undo-history",
            N_("Undo"), N_("Undo History"), LIGMA_ICON_DIALOG_UNDO_HISTORY,
            LIGMA_HELP_UNDO_DIALOG,
            dialogs_undo_editor_new, 0, FALSE),
  DOCKABLE ("ligma-sample-point-editor",
            N_("Sample Points"), N_("Sample Points"), LIGMA_ICON_SAMPLE_POINT,
            LIGMA_HELP_SAMPLE_POINT_DIALOG,
            dialogs_sample_point_editor_new, 0, FALSE),

  /*  display related  */
  DOCKABLE ("ligma-navigation-view",
            N_("Navigation"), N_("Display Navigation"), LIGMA_ICON_DIALOG_NAVIGATION,
            LIGMA_HELP_NAVIGATION_DIALOG,
            dialogs_navigation_editor_new, 0, FALSE),

  /*  editors  */
  DOCKABLE ("ligma-color-editor",
            N_("FG/BG"), N_("FG/BG Color"), LIGMA_ICON_COLORS_DEFAULT,
            LIGMA_HELP_COLOR_DIALOG,
            dialogs_color_editor_new, 0, FALSE),

  /*  singleton editors  */
  DOCKABLE ("ligma-brush-editor",
            N_("Brush Editor"), NULL, LIGMA_ICON_BRUSH,
            LIGMA_HELP_BRUSH_EDITOR_DIALOG,
            dialogs_brush_editor_get, 0, TRUE),
  DOCKABLE ("ligma-dynamics-editor",
            N_("Paint Dynamics Editor"), NULL, LIGMA_ICON_DYNAMICS,
            LIGMA_HELP_DYNAMICS_EDITOR_DIALOG,
            dialogs_dynamics_editor_get, 0, TRUE),
  DOCKABLE ("ligma-gradient-editor",
            N_("Gradient Editor"), NULL, LIGMA_ICON_GRADIENT,
            LIGMA_HELP_GRADIENT_EDITOR_DIALOG,
            dialogs_gradient_editor_get, 0, TRUE),
  DOCKABLE ("ligma-palette-editor",
            N_("Palette Editor"), NULL, LIGMA_ICON_PALETTE,
            LIGMA_HELP_PALETTE_EDITOR_DIALOG,
            dialogs_palette_editor_get, 0, TRUE),
  DOCKABLE ("ligma-tool-preset-editor",
            N_("Tool Preset Editor"), NULL, LIGMA_ICON_TOOL_PRESET,
            LIGMA_HELP_TOOL_PRESET_EDITOR_DIALOG,
            dialogs_tool_preset_editor_get, 0, TRUE),

  /*  image windows  */
  IMAGE_WINDOW ("ligma-empty-image-window",
                TRUE, TRUE),
  IMAGE_WINDOW ("ligma-single-image-window",
                TRUE, TRUE)
};

/**
 * dialogs_restore_dialog:
 * @factory:
 * @screen:
 * @info:
 *
 * Creates a top level widget based on the given session info object
 * in which other widgets later can be be put, typically also restored
 * from the same session info object.
 *
 * Returns:
 **/
static GtkWidget *
dialogs_restore_dialog (LigmaDialogFactory *factory,
                        GdkMonitor        *monitor,
                        LigmaSessionInfo   *info)
{
  GtkWidget        *dialog;
  Ligma             *ligma    = ligma_dialog_factory_get_context (factory)->ligma;
  LigmaCoreConfig   *config  = ligma->config;
  LigmaDisplay      *display = ligma_context_get_display (ligma_get_user_context (ligma));
  LigmaDisplayShell *shell   = ligma_display_get_shell (display);

  LIGMA_LOG (DIALOG_FACTORY, "restoring toplevel \"%s\" (info %p)",
            ligma_session_info_get_factory_entry (info)->identifier,
            info);

  dialog =
    ligma_dialog_factory_dialog_new (factory, monitor,
                                    NULL /*ui_manager*/,
                                    GTK_WIDGET (ligma_display_shell_get_window (shell)),
                                    ligma_session_info_get_factory_entry (info)->identifier,
                                    ligma_session_info_get_factory_entry (info)->view_size,
                                    ! LIGMA_GUI_CONFIG (config)->hide_docks);

  g_object_set_data (G_OBJECT (dialog), LIGMA_DIALOG_VISIBILITY_KEY,
                     GINT_TO_POINTER (LIGMA_GUI_CONFIG (config)->hide_docks ?
                                      LIGMA_DIALOG_VISIBILITY_HIDDEN :
                                      LIGMA_DIALOG_VISIBILITY_VISIBLE));

  return dialog;
}

/**
 * dialogs_restore_window:
 * @factory:
 * @monitor:
 * @info:
 *
 * "restores" the image window. We don't really restore anything since
 * the image window is created earlier, so we just look for and return
 * the already-created image window.
 *
 * Returns:
 **/
static GtkWidget *
dialogs_restore_window (LigmaDialogFactory *factory,
                        GdkMonitor        *monitor,
                        LigmaSessionInfo   *info)
{
  Ligma             *ligma    = ligma_dialog_factory_get_context (factory)->ligma;
  LigmaDisplay      *display = LIGMA_DISPLAY (ligma_get_empty_display (ligma));
  LigmaDisplayShell *shell   = ligma_display_get_shell (display);
  GtkWidget        *dialog;

  dialog = GTK_WIDGET (ligma_display_shell_get_window (shell));

  return dialog;
}


/*  public functions  */

void
dialogs_init (Ligma            *ligma,
              LigmaMenuFactory *menu_factory)
{
  LigmaDialogFactory *factory = NULL;
  gint               i       = 0;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory));

  factory = ligma_dialog_factory_new ("toplevel",
                                     ligma_get_user_context (ligma),
                                     menu_factory);
  ligma_dialog_factory_set_singleton (factory);

  for (i = 0; i < G_N_ELEMENTS (entries); i++)
    ligma_dialog_factory_register_entry (factory,
                                        entries[i].identifier,
                                        entries[i].name ? gettext(entries[i].name) : NULL,
                                        entries[i].blurb ? gettext(entries[i].blurb) : NULL,
                                        entries[i].icon_name,
                                        entries[i].help_id,
                                        entries[i].new_func,
                                        entries[i].restore_func,
                                        entries[i].view_size,
                                        entries[i].singleton,
                                        entries[i].session_managed,
                                        entries[i].remember_size,
                                        entries[i].remember_if_open,
                                        entries[i].hideable,
                                        entries[i].image_window,
                                        entries[i].dockable);

  global_recent_docks = ligma_list_new (LIGMA_TYPE_SESSION_INFO, FALSE);
}

void
dialogs_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (ligma_dialog_factory_get_singleton ())
    {
      /* run dispose manually so the factory destroys its dialogs, which
       * might in turn directly or indirectly ref the factory
       */
      g_object_run_dispose (G_OBJECT (ligma_dialog_factory_get_singleton ()));

      g_object_unref (ligma_dialog_factory_get_singleton ());
      ligma_dialog_factory_set_singleton (NULL);
    }

  g_clear_object (&global_recent_docks);
}

static void
dialogs_ensure_factory_entry_on_recent_dock (LigmaSessionInfo *info)
{
  if (! ligma_session_info_get_factory_entry (info))
    {
      LigmaDialogFactoryEntry *entry = NULL;

      /* The recent docks container only contains session infos for
       * dock windows
       */
      entry = ligma_dialog_factory_find_entry (ligma_dialog_factory_get_singleton (),
                                              "ligma-dock-window");

      ligma_session_info_set_factory_entry (info, entry);
    }
}

static GFile *
dialogs_get_dockrc_file (void)
{
  const gchar *basename;

  basename = g_getenv ("LIGMA_TESTING_DOCKRC_NAME");
  if (! basename)
    basename = "dockrc";

  return ligma_directory_file (basename, NULL);
}

void
dialogs_load_recent_docks (Ligma *ligma)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  file = dialogs_get_dockrc_file ();

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_deserialize_file (LIGMA_CONFIG (global_recent_docks),
                                      file,
                                      NULL, &error))
    {
      if (error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);

      g_clear_error (&error);
    }

  g_object_unref (file);

  /* In LIGMA 2.6 dockrc did not contain the factory entries for the
   * session infos, so set that up manually if needed
   */
  ligma_container_foreach (global_recent_docks,
                          (GFunc) dialogs_ensure_factory_entry_on_recent_dock,
                          NULL);

  ligma_list_reverse (LIGMA_LIST (global_recent_docks));
}

void
dialogs_save_recent_docks (Ligma *ligma)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  file = dialogs_get_dockrc_file ();

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  if (! ligma_config_serialize_to_file (LIGMA_CONFIG (global_recent_docks),
                                       file,
                                       "recently closed docks",
                                       "end of recently closed docks",
                                       NULL, &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);
}

GtkWidget *
dialogs_get_toolbox (void)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (ligma_dialog_factory_get_singleton ()), NULL);

  for (list = ligma_dialog_factory_get_open_dialogs (ligma_dialog_factory_get_singleton ());
       list;
       list = g_list_next (list))
    {
      if (LIGMA_IS_DOCK_WINDOW (list->data) &&
          ligma_dock_window_has_toolbox (list->data))
        return list->data;
    }

  return NULL;
}

GtkWidget *
dialogs_get_dialog (GObject     *attach_object,
                    const gchar *attach_key)
{
  g_return_val_if_fail (G_IS_OBJECT (attach_object), NULL);
  g_return_val_if_fail (attach_key != NULL, NULL);

  return g_object_get_data (attach_object, attach_key);
}

void
dialogs_attach_dialog (GObject     *attach_object,
                       const gchar *attach_key,
                       GtkWidget   *dialog)
{
  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (attach_key != NULL);
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  g_object_set_data (attach_object, attach_key, dialog);
  g_object_set_data (G_OBJECT (dialog), "ligma-dialogs-attach-key",
                     (gpointer) attach_key);

  g_signal_connect_object (dialog, "destroy",
                           G_CALLBACK (dialogs_detach_dialog),
                           attach_object,
                           G_CONNECT_SWAPPED);
}

void
dialogs_detach_dialog (GObject   *attach_object,
                       GtkWidget *dialog)
{
  const gchar *attach_key;

  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  attach_key = g_object_get_data (G_OBJECT (dialog),
                                  "ligma-dialogs-attach-key");

  g_return_if_fail (attach_key != NULL);

  g_object_set_data (attach_object, attach_key, NULL);

  g_signal_handlers_disconnect_by_func (dialog,
                                        dialogs_detach_dialog,
                                        attach_object);
}

void
dialogs_destroy_dialog (GObject     *attach_object,
                        const gchar *attach_key)
{
  GtkWidget *dialog;

  g_return_if_fail (G_IS_OBJECT (attach_object));
  g_return_if_fail (attach_key != NULL);

  dialog = g_object_get_data (attach_object, attach_key);

  if (dialog)
    gtk_widget_destroy (dialog);
}
