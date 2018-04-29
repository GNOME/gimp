/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpguiconfig.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpsessioninfo-aux.h"
#include "widgets/gimpsessionmanaged.h"
#include "widgets/gimptoolbox.h"

#include "dialogs.h"
#include "dialogs-constructors.h"

#include "gimp-log.h"

#include "gimp-intl.h"


GimpContainer *global_recent_docks = NULL;


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
  { "gimp-"#id"-list"             /* identifier       */,  \
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
  { "gimp-"#id"-grid"             /* identifier       */,  \
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
  { "gimp-"#id"-list"                   /* identifier       */, \
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


static GtkWidget * dialogs_restore_dialog (GimpDialogFactory *factory,
                                           GdkMonitor        *monitor,
                                           GimpSessionInfo   *info);
static GtkWidget * dialogs_restore_window (GimpDialogFactory *factory,
                                           GdkMonitor        *monitor,
                                           GimpSessionInfo   *info);


static const GimpDialogFactoryEntry entries[] =
{
  /*  foreign toplevels without constructor  */
  FOREIGN ("gimp-brightness-contrast-tool-dialog", TRUE,  FALSE),
  FOREIGN ("gimp-color-balance-tool-dialog",       TRUE,  FALSE),
  FOREIGN ("gimp-color-picker-tool-dialog",        TRUE,  TRUE),
  FOREIGN ("gimp-colorize-tool-dialog",            TRUE,  FALSE),
  FOREIGN ("gimp-crop-tool-dialog",                TRUE,  FALSE),
  FOREIGN ("gimp-curves-tool-dialog",              TRUE,  TRUE),
  FOREIGN ("gimp-desaturate-tool-dialog",          TRUE,  FALSE),
  FOREIGN ("gimp-foreground-select-tool-dialog",   TRUE,  FALSE),
  FOREIGN ("gimp-gegl-tool-dialog",                TRUE,  FALSE),
  FOREIGN ("gimp-gradient-tool-dialog",            TRUE,  FALSE),
  FOREIGN ("gimp-hue-saturation-tool-dialog",      TRUE,  FALSE),
  FOREIGN ("gimp-levels-tool-dialog",              TRUE,  TRUE),
  FOREIGN ("gimp-measure-tool-dialog",             TRUE,  FALSE),
  FOREIGN ("gimp-operation-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("gimp-posterize-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("gimp-rotate-tool-dialog",              TRUE,  FALSE),
  FOREIGN ("gimp-scale-tool-dialog",               TRUE,  FALSE),
  FOREIGN ("gimp-shear-tool-dialog",               TRUE,  FALSE),
  FOREIGN ("gimp-text-tool-dialog",                TRUE,  TRUE),
  FOREIGN ("gimp-threshold-tool-dialog",           TRUE,  FALSE),
  FOREIGN ("gimp-perspective-tool-dialog",         TRUE,  FALSE),
  FOREIGN ("gimp-unified-transform-tool-dialog",   TRUE,  FALSE),
  FOREIGN ("gimp-handle-transform-tool-dialog",    TRUE,  FALSE),

  FOREIGN ("gimp-toolbox-color-dialog",            TRUE,  FALSE),
  FOREIGN ("gimp-gradient-editor-color-dialog",    TRUE,  FALSE),
  FOREIGN ("gimp-palette-editor-color-dialog",     TRUE,  FALSE),
  FOREIGN ("gimp-colormap-editor-color-dialog",    TRUE,  FALSE),

  FOREIGN ("gimp-controller-editor-dialog",        FALSE, TRUE),
  FOREIGN ("gimp-controller-action-dialog",        FALSE, TRUE),

  /*  ordinary toplevels  */
  TOPLEVEL ("gimp-image-new-dialog",
            dialogs_image_new_new,          FALSE, TRUE, FALSE),
  TOPLEVEL ("gimp-file-open-dialog",
            dialogs_file_open_new,          TRUE,  TRUE, TRUE),
  TOPLEVEL ("gimp-file-open-location-dialog",
            dialogs_file_open_location_new, FALSE, TRUE, FALSE),
  TOPLEVEL ("gimp-file-save-dialog",
            dialogs_file_save_new,          FALSE, TRUE, TRUE),
  TOPLEVEL ("gimp-file-export-dialog",
            dialogs_file_export_new,        FALSE, TRUE, TRUE),

  /*  singleton toplevels  */
  TOPLEVEL ("gimp-preferences-dialog",
            dialogs_preferences_get,        TRUE, TRUE,  TRUE),
  TOPLEVEL ("gimp-input-devices-dialog",
            dialogs_input_devices_get,      TRUE, TRUE,  FALSE),
  TOPLEVEL ("gimp-keyboard-shortcuts-dialog",
            dialogs_keyboard_shortcuts_get, TRUE, TRUE,  TRUE),
  TOPLEVEL ("gimp-module-dialog",
            dialogs_module_get,             TRUE, TRUE,  TRUE),
  TOPLEVEL ("gimp-palette-import-dialog",
            dialogs_palette_import_get,     TRUE, TRUE,  TRUE),
  TOPLEVEL ("gimp-tips-dialog",
            dialogs_tips_get,               TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-about-dialog",
            dialogs_about_get,              TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-action-search-dialog",
            dialogs_action_search_get,      TRUE, TRUE,  TRUE),
  TOPLEVEL ("gimp-error-dialog",
            dialogs_error_get,              TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-critical-dialog",
            dialogs_critical_get,           TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-close-all-dialog",
            dialogs_close_all_get,          TRUE, FALSE, FALSE),
  TOPLEVEL ("gimp-quit-dialog",
            dialogs_quit_get,               TRUE, FALSE, FALSE),

  /*  docks  */
  DOCK ("gimp-dock",
        dialogs_dock_new),
  DOCK ("gimp-toolbox",
        dialogs_toolbox_new),

  /*  dock windows  */
  DOCK_WINDOW ("gimp-dock-window",
               dialogs_dock_window_new),
  DOCK_WINDOW ("gimp-toolbox-window",
               dialogs_toolbox_dock_window_new),

  /*  singleton dockables  */
  DOCKABLE ("gimp-tool-options",
            N_("Tool Options"), NULL, GIMP_ICON_DIALOG_TOOL_OPTIONS,
            GIMP_HELP_TOOL_OPTIONS_DIALOG,
            dialogs_tool_options_new, 0, TRUE),
  DOCKABLE ("gimp-device-status",
            N_("Devices"), N_("Device Status"), GIMP_ICON_DIALOG_DEVICE_STATUS,
            GIMP_HELP_DEVICE_STATUS_DIALOG,
            dialogs_device_status_new, 0, TRUE),
  DOCKABLE ("gimp-error-console",
            N_("Errors"), N_("Error Console"), GIMP_ICON_DIALOG_WARNING,
            GIMP_HELP_ERRORS_DIALOG,
            dialogs_error_console_new, 0, TRUE),
  DOCKABLE ("gimp-cursor-view",
            N_("Pointer"), N_("Pointer Information"), GIMP_ICON_CURSOR,
            GIMP_HELP_POINTER_INFO_DIALOG,
            dialogs_cursor_view_new, 0, TRUE),
  DOCKABLE ("gimp-dashboard",
            N_("Dashboard"), N_("Dashboard"), GIMP_ICON_DIALOG_DASHBOARD,
            GIMP_HELP_DASHBOARD_DIALOG,
            dialogs_dashboard_new, 0, TRUE),

  /*  list & grid views  */
  LISTGRID (image, image,
            N_("Images"), NULL, GIMP_ICON_DIALOG_IMAGES,
            GIMP_HELP_IMAGE_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (brush, brush,
            N_("Brushes"), NULL, GIMP_ICON_BRUSH,
            GIMP_HELP_BRUSH_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (dynamics, dynamics,
            N_("Paint Dynamics"), NULL, GIMP_ICON_DYNAMICS,
            GIMP_HELP_DYNAMICS_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (mypaint-brush, mypaint_brush,
            N_("MyPaint Brushes"), NULL, GIMP_ICON_MYPAINT_BRUSH,
            GIMP_HELP_MYPAINT_BRUSH_DIALOG, GIMP_VIEW_SIZE_LARGE),
  LISTGRID (pattern, pattern,
            N_("Patterns"), NULL, GIMP_ICON_PATTERN,
            GIMP_HELP_PATTERN_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (gradient, gradient,
            N_("Gradients"), NULL, GIMP_ICON_GRADIENT,
            GIMP_HELP_GRADIENT_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (palette, palette,
            N_("Palettes"), NULL, GIMP_ICON_PALETTE,
            GIMP_HELP_PALETTE_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (font, font,
            N_("Fonts"), NULL, GIMP_ICON_FONT,
            GIMP_HELP_FONT_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (buffer, buffer,
            N_("Buffers"), NULL, GIMP_ICON_BUFFER,
            GIMP_HELP_BUFFER_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (tool-preset, tool_preset,
            N_("Tool Presets"), NULL, GIMP_ICON_TOOL_PRESET,
            GIMP_HELP_TOOL_PRESET_DIALOG, GIMP_VIEW_SIZE_MEDIUM),
  LISTGRID (document, document,
            N_("History"), N_("Document History"), GIMP_ICON_DOCUMENT_OPEN_RECENT,
            GIMP_HELP_DOCUMENT_DIALOG, GIMP_VIEW_SIZE_LARGE),
  LISTGRID (template, template,
            N_("Templates"), N_("Image Templates"), GIMP_ICON_TEMPLATE,
            GIMP_HELP_TEMPLATE_DIALOG, GIMP_VIEW_SIZE_SMALL),

  /*  image related  */
  DOCKABLE ("gimp-layer-list",
            N_("Layers"), NULL, GIMP_ICON_DIALOG_LAYERS,
            GIMP_HELP_LAYER_DIALOG,
            dialogs_layer_list_view_new, 0, FALSE),
  DOCKABLE ("gimp-channel-list",
            N_("Channels"), NULL, GIMP_ICON_DIALOG_CHANNELS,
            GIMP_HELP_CHANNEL_DIALOG,
            dialogs_channel_list_view_new, 0, FALSE),
  DOCKABLE ("gimp-vectors-list",
            N_("Paths"), NULL, GIMP_ICON_DIALOG_PATHS,
            GIMP_HELP_PATH_DIALOG,
            dialogs_vectors_list_view_new, 0, FALSE),
  DOCKABLE ("gimp-indexed-palette",
            N_("Colormap"), NULL, GIMP_ICON_COLORMAP,
            GIMP_HELP_INDEXED_PALETTE_DIALOG,
            dialogs_colormap_editor_new, 0, FALSE),
  DOCKABLE ("gimp-histogram-editor",
            N_("Histogram"), NULL, GIMP_ICON_HISTOGRAM,
            GIMP_HELP_HISTOGRAM_DIALOG,
            dialogs_histogram_editor_new, 0, FALSE),
  DOCKABLE ("gimp-selection-editor",
            N_("Selection"), N_("Selection Editor"), GIMP_ICON_SELECTION,
            GIMP_HELP_SELECTION_DIALOG,
            dialogs_selection_editor_new, 0, FALSE),
  DOCKABLE ("gimp-symmetry-editor",
            N_("Symmetry Painting"), NULL, GIMP_ICON_SYMMETRY,
            GIMP_HELP_SYMMETRY_DIALOG,
            dialogs_symmetry_editor_new, 0, FALSE),
  DOCKABLE ("gimp-undo-history",
            N_("Undo"), N_("Undo History"), GIMP_ICON_DIALOG_UNDO_HISTORY,
            GIMP_HELP_UNDO_DIALOG,
            dialogs_undo_editor_new, 0, FALSE),
  DOCKABLE ("gimp-sample-point-editor",
            N_("Sample Points"), N_("Sample Points"), GIMP_ICON_SAMPLE_POINT,
            GIMP_HELP_SAMPLE_POINT_DIALOG,
            dialogs_sample_point_editor_new, 0, FALSE),

  /*  display related  */
  DOCKABLE ("gimp-navigation-view",
            N_("Navigation"), N_("Display Navigation"), GIMP_ICON_DIALOG_NAVIGATION,
            GIMP_HELP_NAVIGATION_DIALOG,
            dialogs_navigation_editor_new, 0, FALSE),

  /*  editors  */
  DOCKABLE ("gimp-color-editor",
            N_("FG/BG"), N_("FG/BG Color"), GIMP_ICON_COLORS_DEFAULT,
            GIMP_HELP_COLOR_DIALOG,
            dialogs_color_editor_new, 0, FALSE),

  /*  singleton editors  */
  DOCKABLE ("gimp-brush-editor",
            N_("Brush Editor"), NULL, GIMP_ICON_BRUSH,
            GIMP_HELP_BRUSH_EDITOR_DIALOG,
            dialogs_brush_editor_get, 0, TRUE),
  DOCKABLE ("gimp-dynamics-editor",
            N_("Paint Dynamics Editor"), NULL, GIMP_ICON_DYNAMICS,
            GIMP_HELP_DYNAMICS_EDITOR_DIALOG,
            dialogs_dynamics_editor_get, 0, TRUE),
  DOCKABLE ("gimp-gradient-editor",
            N_("Gradient Editor"), NULL, GIMP_ICON_GRADIENT,
            GIMP_HELP_GRADIENT_EDITOR_DIALOG,
            dialogs_gradient_editor_get, 0, TRUE),
  DOCKABLE ("gimp-palette-editor",
            N_("Palette Editor"), NULL, GIMP_ICON_PALETTE,
            GIMP_HELP_PALETTE_EDITOR_DIALOG,
            dialogs_palette_editor_get, 0, TRUE),
  DOCKABLE ("gimp-tool-preset-editor",
            N_("Tool Preset Editor"), NULL, GIMP_ICON_TOOL_PRESET,
            GIMP_HELP_TOOL_PRESET_EDITOR_DIALOG,
            dialogs_tool_preset_editor_get, 0, TRUE),

  /*  image windows  */
  IMAGE_WINDOW ("gimp-empty-image-window",
                TRUE, TRUE),
  IMAGE_WINDOW ("gimp-single-image-window",
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
dialogs_restore_dialog (GimpDialogFactory *factory,
                        GdkMonitor        *monitor,
                        GimpSessionInfo   *info)
{
  GtkWidget      *dialog;
  GimpCoreConfig *config = gimp_dialog_factory_get_context (factory)->gimp->config;

  GIMP_LOG (DIALOG_FACTORY, "restoring toplevel \"%s\" (info %p)",
            gimp_session_info_get_factory_entry (info)->identifier,
            info);

  dialog =
    gimp_dialog_factory_dialog_new (factory, monitor,
                                    NULL /*ui_manager*/,
                                    gimp_session_info_get_factory_entry (info)->identifier,
                                    gimp_session_info_get_factory_entry (info)->view_size,
                                    ! GIMP_GUI_CONFIG (config)->hide_docks);

  g_object_set_data (G_OBJECT (dialog), GIMP_DIALOG_VISIBILITY_KEY,
                     GINT_TO_POINTER (GIMP_GUI_CONFIG (config)->hide_docks ?
                                      GIMP_DIALOG_VISIBILITY_HIDDEN :
                                      GIMP_DIALOG_VISIBILITY_VISIBLE));

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
dialogs_restore_window (GimpDialogFactory *factory,
                        GdkMonitor        *monitor,
                        GimpSessionInfo   *info)
{
  Gimp             *gimp    = gimp_dialog_factory_get_context (factory)->gimp;
  GimpDisplay      *display = GIMP_DISPLAY (gimp_get_empty_display (gimp));
  GimpDisplayShell *shell   = gimp_display_get_shell (display);
  GtkWidget        *dialog;

  dialog = GTK_WIDGET (gimp_display_shell_get_window (shell));

  return dialog;
}


/*  public functions  */

void
dialogs_init (Gimp            *gimp,
              GimpMenuFactory *menu_factory)
{
  GimpDialogFactory *factory = NULL;
  gint               i       = 0;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_MENU_FACTORY (menu_factory));

  factory = gimp_dialog_factory_new ("toplevel",
                                     gimp_get_user_context (gimp),
                                     menu_factory);
  gimp_dialog_factory_set_singleton (factory);

  for (i = 0; i < G_N_ELEMENTS (entries); i++)
    gimp_dialog_factory_register_entry (factory,
                                        entries[i].identifier,
                                        gettext (entries[i].name),
                                        gettext (entries[i].blurb),
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

  global_recent_docks = gimp_list_new (GIMP_TYPE_SESSION_INFO, FALSE);
}

void
dialogs_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp_dialog_factory_get_singleton ())
    {
      /* run dispose manually so the factory destroys its dialogs, which
       * might in turn directly or indirectly ref the factory
       */
      g_object_run_dispose (G_OBJECT (gimp_dialog_factory_get_singleton ()));

      g_object_unref (gimp_dialog_factory_get_singleton ());
      gimp_dialog_factory_set_singleton (NULL);
    }

  g_clear_object (&global_recent_docks);
}

static void
dialogs_ensure_factory_entry_on_recent_dock (GimpSessionInfo *info)
{
  if (! gimp_session_info_get_factory_entry (info))
    {
      GimpDialogFactoryEntry *entry = NULL;

      /* The recent docks container only contains session infos for
       * dock windows
       */
      entry = gimp_dialog_factory_find_entry (gimp_dialog_factory_get_singleton (),
                                              "gimp-dock-window");

      gimp_session_info_set_factory_entry (info, entry);
    }
}

static GFile *
dialogs_get_dockrc_file (void)
{
  const gchar *basename;

  basename = g_getenv ("GIMP_TESTING_DOCKRC_NAME");
  if (! basename)
    basename = "dockrc";

  return gimp_directory_file (basename, NULL);
}

void
dialogs_load_recent_docks (Gimp *gimp)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file = dialogs_get_dockrc_file ();

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_deserialize_gfile (GIMP_CONFIG (global_recent_docks),
                                       file,
                                       NULL, &error))
    {
      if (error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);

      g_clear_error (&error);
    }

  g_object_unref (file);

  /* In GIMP 2.6 dockrc did not contain the factory entries for the
   * session infos, so set that up manually if needed
   */
  gimp_container_foreach (global_recent_docks,
                          (GFunc) dialogs_ensure_factory_entry_on_recent_dock,
                          NULL);

  gimp_list_reverse (GIMP_LIST (global_recent_docks));
}

void
dialogs_save_recent_docks (Gimp *gimp)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file = dialogs_get_dockrc_file ();

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  if (! gimp_config_serialize_to_gfile (GIMP_CONFIG (global_recent_docks),
                                        file,
                                        "recently closed docks",
                                        "end of recently closed docks",
                                        NULL, &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);
}

GtkWidget *
dialogs_get_toolbox (void)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_DIALOG_FACTORY (gimp_dialog_factory_get_singleton ()), NULL);

  for (list = gimp_dialog_factory_get_open_dialogs (gimp_dialog_factory_get_singleton ());
       list;
       list = g_list_next (list))
    {
      if (GIMP_IS_DOCK_WINDOW (list->data) &&
          gimp_dock_window_has_toolbox (list->data))
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
  g_object_set_data (G_OBJECT (dialog), "gimp-dialogs-attach-key",
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
                                  "gimp-dialogs-attach-key");

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
