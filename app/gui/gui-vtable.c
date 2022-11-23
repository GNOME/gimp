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

#include <string.h>
#include <errno.h>

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#ifdef G_OS_WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>

#ifndef pipe
#define pipe(fds) _pipe(fds, 4096, _O_BINARY)
#endif
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#endif

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "gui-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligma-parallel.h"
#include "core/ligma-spawn.h"
#include "core/ligma-utils.h"
#include "core/ligmaasync.h"
#include "core/ligmabrush.h"
#include "core/ligmacancelable.h"
#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmagradient.h"
#include "core/ligmaimage.h"
#include "core/ligmaimagefile.h"
#include "core/ligmalist.h"
#include "core/ligmapalette.h"
#include "core/ligmapattern.h"
#include "core/ligmaprogress.h"
#include "core/ligmawaitable.h"

#include "text/ligmafont.h"

#include "pdb/ligmapdb.h"
#include "pdb/ligmaprocedure.h"

#include "plug-in/ligmapluginmanager-file.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmabrushselect.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadocked.h"
#include "widgets/ligmafontselect.h"
#include "widgets/ligmagradientselect.h"
#include "widgets/ligmahelp.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmapaletteselect.h"
#include "widgets/ligmapatternselect.h"
#include "widgets/ligmaprogressdialog.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplay-foreach.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmasinglewindowstrategy.h"
#include "display/ligmamultiwindowstrategy.h"

#include "actions/plug-in-actions.h"

#include "menus/menus.h"

#include "dialogs/color-profile-import-dialog.h"
#include "dialogs/metadata-rotation-import-dialog.h"

#include "gui.h"
#include "gui-message.h"
#include "gui-vtable.h"
#include "icon-themes.h"
#include "themes.h"


/*  local function prototypes  */

static void           gui_ungrab                 (Ligma                *ligma);

static void           gui_set_busy               (Ligma                *ligma);
static void           gui_unset_busy             (Ligma                *ligma);

static void           gui_help                   (Ligma                *ligma,
                                                  LigmaProgress        *progress,
                                                  const gchar         *help_domain,
                                                  const gchar         *help_id);
static const gchar  * gui_get_program_class      (Ligma                *ligma);
static gchar        * gui_get_display_name       (Ligma                *ligma,
                                                  gint                 display_id,
                                                  GObject            **monitor,
                                                  gint                *monitor_number);
static guint32        gui_get_user_time          (Ligma                *ligma);
static GFile        * gui_get_theme_dir          (Ligma                *ligma);
static GFile        * gui_get_icon_theme_dir     (Ligma                *ligma);
static LigmaObject   * gui_get_window_strategy    (Ligma                *ligma);
static LigmaDisplay  * gui_get_empty_display      (Ligma                *ligma);
static guint32        gui_display_get_window_id  (LigmaDisplay         *display);
static LigmaDisplay  * gui_display_create         (Ligma                *ligma,
                                                  LigmaImage           *image,
                                                  LigmaUnit             unit,
                                                  gdouble              scale,
                                                  GObject             *monitor);
static void           gui_display_delete         (LigmaDisplay         *display);
static void           gui_displays_reconnect     (Ligma                *ligma,
                                                  LigmaImage           *old_image,
                                                  LigmaImage           *new_image);
static gboolean       gui_wait                   (Ligma                *ligma,
                                                  LigmaWaitable        *waitable,
                                                  const gchar         *message);
static LigmaProgress * gui_new_progress           (Ligma                *ligma,
                                                  LigmaDisplay         *display);
static void           gui_free_progress          (Ligma                *ligma,
                                                  LigmaProgress        *progress);
static gboolean       gui_pdb_dialog_new         (Ligma                *ligma,
                                                  LigmaContext         *context,
                                                  LigmaProgress        *progress,
                                                  LigmaContainer       *container,
                                                  const gchar         *title,
                                                  const gchar         *callback_name,
                                                  const gchar         *object_name,
                                                  va_list              args);
static gboolean       gui_pdb_dialog_set         (Ligma                *ligma,
                                                  LigmaContainer       *container,
                                                  const gchar         *callback_name,
                                                  const gchar         *object_name,
                                                  va_list              args);
static gboolean       gui_pdb_dialog_close       (Ligma                *ligma,
                                                  LigmaContainer       *container,
                                                  const gchar         *callback_name);
static gboolean       gui_recent_list_add_file   (Ligma                *ligma,
                                                  GFile               *file,
                                                  const gchar         *mime_type);
static void           gui_recent_list_load       (Ligma                *ligma);

static GMountOperation
                    * gui_get_mount_operation    (Ligma                *ligma,
                                                  LigmaProgress        *progress);

static LigmaColorProfilePolicy
                      gui_query_profile_policy   (Ligma                *ligma,
                                                  LigmaImage           *image,
                                                  LigmaContext         *context,
                                                  LigmaColorProfile   **dest_profile,
                                                  LigmaColorRenderingIntent *intent,
                                                  gboolean            *bpc,
                                                  gboolean            *dont_ask);
static LigmaMetadataRotationPolicy
                      gui_query_rotation_policy  (Ligma                *ligma,
                                                  LigmaImage           *image,
                                                  LigmaContext         *context,
                                                  gboolean            *dont_ask);


/*  public functions  */

void
gui_vtable_init (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma->gui.ungrab                 = gui_ungrab;
  ligma->gui.set_busy               = gui_set_busy;
  ligma->gui.unset_busy             = gui_unset_busy;
  ligma->gui.show_message           = gui_message;
  ligma->gui.help                   = gui_help;
  ligma->gui.get_program_class      = gui_get_program_class;
  ligma->gui.get_display_name       = gui_get_display_name;
  ligma->gui.get_user_time          = gui_get_user_time;
  ligma->gui.get_theme_dir          = gui_get_theme_dir;
  ligma->gui.get_icon_theme_dir     = gui_get_icon_theme_dir;
  ligma->gui.get_window_strategy    = gui_get_window_strategy;
  ligma->gui.get_empty_display      = gui_get_empty_display;
  ligma->gui.display_get_window_id  = gui_display_get_window_id;
  ligma->gui.display_create         = gui_display_create;
  ligma->gui.display_delete         = gui_display_delete;
  ligma->gui.displays_reconnect     = gui_displays_reconnect;
  ligma->gui.wait                   = gui_wait;
  ligma->gui.progress_new           = gui_new_progress;
  ligma->gui.progress_free          = gui_free_progress;
  ligma->gui.pdb_dialog_new         = gui_pdb_dialog_new;
  ligma->gui.pdb_dialog_set         = gui_pdb_dialog_set;
  ligma->gui.pdb_dialog_close       = gui_pdb_dialog_close;
  ligma->gui.recent_list_add_file   = gui_recent_list_add_file;
  ligma->gui.recent_list_load       = gui_recent_list_load;
  ligma->gui.get_mount_operation    = gui_get_mount_operation;
  ligma->gui.query_profile_policy   = gui_query_profile_policy;
  ligma->gui.query_rotation_policy  = gui_query_rotation_policy;
}


/*  private functions  */

static void
gui_ungrab (Ligma *ligma)
{
  GdkDisplay *display = gdk_display_get_default ();

  if (display)
    gdk_seat_ungrab (gdk_display_get_default_seat (display));
}

static void
gui_set_busy (Ligma *ligma)
{
  ligma_displays_set_busy (ligma);
  ligma_dialog_factory_set_busy (ligma_dialog_factory_get_singleton ());

  gdk_display_flush (gdk_display_get_default ());
}

static void
gui_unset_busy (Ligma *ligma)
{
  ligma_displays_unset_busy (ligma);
  ligma_dialog_factory_unset_busy (ligma_dialog_factory_get_singleton ());

  gdk_display_flush (gdk_display_get_default ());
}

static void
gui_help (Ligma         *ligma,
          LigmaProgress *progress,
          const gchar  *help_domain,
          const gchar  *help_id)
{
  ligma_help_show (ligma, progress, help_domain, help_id);
}

static const gchar *
gui_get_program_class (Ligma *ligma)
{
  return gdk_get_program_class ();
}

static gint
get_monitor_number (GdkMonitor *monitor)
{
  GdkDisplay *display    = gdk_monitor_get_display (monitor);
  gint        n_monitors = gdk_display_get_n_monitors (display);
  gint        i;

  for (i = 0; i < n_monitors; i++)
    if (gdk_display_get_monitor (display, i) == monitor)
      return i;

  return 0;
}

static gchar *
gui_get_display_name (Ligma     *ligma,
                      gint      display_id,
                      GObject **monitor,
                      gint     *monitor_number)
{
  LigmaDisplay *display = NULL;
  GdkDisplay  *gdk_display;

  if (display_id > 0)
    display = ligma_display_get_by_id (ligma, display_id);

  if (display)
    {
      LigmaDisplayShell *shell = ligma_display_get_shell (display);

      gdk_display = gtk_widget_get_display (GTK_WIDGET (shell));

      *monitor = G_OBJECT (ligma_widget_get_monitor (GTK_WIDGET (shell)));
    }
  else
    {
      *monitor = G_OBJECT (gui_get_initial_monitor (ligma));

      if (! *monitor)
        *monitor = G_OBJECT (ligma_get_monitor_at_pointer ());

      gdk_display = gdk_monitor_get_display (GDK_MONITOR (*monitor));
    }

  *monitor_number = get_monitor_number (GDK_MONITOR (*monitor));

  return gdk_screen_make_display_name (gdk_display_get_default_screen (gdk_display));
}

static guint32
gui_get_user_time (Ligma *ligma)
{
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return gdk_x11_display_get_user_time (gdk_display_get_default ());
#endif

  return gtk_get_current_event_time ();
}

static GFile *
gui_get_theme_dir (Ligma *ligma)
{
  return themes_get_theme_dir (ligma, LIGMA_GUI_CONFIG (ligma->config)->theme);
}

static GFile *
gui_get_icon_theme_dir (Ligma *ligma)
{
  return icon_themes_get_theme_dir (ligma, LIGMA_GUI_CONFIG (ligma->config)->icon_theme);
}

static LigmaObject *
gui_get_window_strategy (Ligma *ligma)
{
  if (LIGMA_GUI_CONFIG (ligma->config)->single_window_mode)
    return ligma_single_window_strategy_get_singleton ();
  else
    return ligma_multi_window_strategy_get_singleton ();
}

static LigmaDisplay *
gui_get_empty_display (Ligma *ligma)
{
  LigmaDisplay *display = NULL;

  if (ligma_container_get_n_children (ligma->displays) == 1)
    {
      display = (LigmaDisplay *) ligma_container_get_first_child (ligma->displays);

      if (ligma_display_get_image (display))
        {
          /* The display was not empty */
          display = NULL;
        }
    }

  return display;
}

static guint32
gui_display_get_window_id (LigmaDisplay *display)
{
  LigmaDisplay      *disp  = LIGMA_DISPLAY (display);
  LigmaDisplayShell *shell = ligma_display_get_shell (disp);

  if (shell)
    {
      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

      if (GTK_IS_WINDOW (toplevel))
        return ligma_window_get_native_id (GTK_WINDOW (toplevel));
    }

  return 0;
}

static LigmaDisplay *
gui_display_create (Ligma      *ligma,
                    LigmaImage *image,
                    LigmaUnit   unit,
                    gdouble    scale,
                    GObject   *monitor)
{
  LigmaContext *context = ligma_get_user_context (ligma);
  LigmaDisplay *display = LIGMA_DISPLAY (gui_get_empty_display (ligma));

  if (! monitor)
    monitor = G_OBJECT (ligma_get_monitor_at_pointer ());

  if (display)
    {
      ligma_display_fill (display, image, unit, scale);
    }
  else
    {
      GList *image_managers = ligma_ui_managers_from_name ("<Image>");

      g_return_val_if_fail (image_managers != NULL, NULL);

      display = ligma_display_new (ligma, image, unit, scale,
                                  image_managers->data,
                                  ligma_dialog_factory_get_singleton (),
                                  GDK_MONITOR (monitor));
   }

  if (ligma_context_get_display (context) == display)
    {
      ligma_context_set_image (context, image);
      ligma_context_display_changed (context);
    }
  else
    {
      ligma_context_set_display (context, display);
    }

  return display;
}

static void
gui_display_delete (LigmaDisplay *display)
{
  ligma_display_close (display);
}

static void
gui_displays_reconnect (Ligma      *ligma,
                        LigmaImage *old_image,
                        LigmaImage *new_image)
{
  ligma_displays_reconnect (ligma, old_image, new_image);
}

static void
gui_wait_input_async (LigmaAsync  *async,
                      const gint  input_pipe[2])
{
  guint8 buffer[1];

  while (read (input_pipe[0], buffer, sizeof (buffer)) == -1 &&
         errno == EINTR);

  ligma_async_finish (async, NULL);
}

static gboolean
gui_wait (Ligma         *ligma,
          LigmaWaitable *waitable,
          const gchar  *message)
{
  LigmaProcedure  *procedure;
  LigmaValueArray *args;
  LigmaAsync      *input_async = NULL;
  GError         *error       = NULL;
  gint            input_pipe[2];
  gint            output_pipe[2];

  procedure = ligma_pdb_lookup_procedure (ligma->pdb, "plug-in-busy-dialog");

  if (! procedure)
    return FALSE;

  if (pipe (input_pipe))
    return FALSE;

  if (pipe (output_pipe))
    {
      close (input_pipe[0]);
      close (input_pipe[1]);

      return FALSE;
    }

  ligma_spawn_set_cloexec (input_pipe[0]);
  ligma_spawn_set_cloexec (output_pipe[1]);

  args = ligma_procedure_get_arguments (procedure);
  ligma_value_array_truncate (args, 5);

  g_value_set_enum   (ligma_value_array_index (args, 0),
                      LIGMA_RUN_INTERACTIVE);
  g_value_set_int    (ligma_value_array_index (args, 1),
                      output_pipe[0]);
  g_value_set_int    (ligma_value_array_index (args, 2),
                      input_pipe[1]);
  g_value_set_string (ligma_value_array_index (args, 3),
                      message);
  g_value_set_int    (ligma_value_array_index (args, 4),
                      LIGMA_IS_CANCELABLE (waitable));

  ligma_procedure_execute_async (procedure, ligma,
                                ligma_get_user_context (ligma),
                                NULL, args, NULL, &error);

  ligma_value_array_unref (args);

  close (input_pipe[1]);
  close (output_pipe[0]);

  if (error)
    {
      g_clear_error (&error);

      close (input_pipe[0]);
      close (output_pipe[1]);

      return FALSE;
    }

  if (LIGMA_IS_CANCELABLE (waitable))
    {
      /* listens for a cancellation request */
      input_async = ligma_parallel_run_async_independent (
        (LigmaRunAsyncFunc) gui_wait_input_async,
        input_pipe);

      while (! ligma_waitable_wait_for (waitable, 0.1 * G_TIME_SPAN_SECOND))
        {
          /* check for a cancellation request */
          if (ligma_waitable_try_wait (LIGMA_WAITABLE (input_async)))
            {
              ligma_cancelable_cancel (LIGMA_CANCELABLE (waitable));

              break;
            }
        }
    }

  ligma_waitable_wait (waitable);

  /* signal completion to the plug-in */
  close (output_pipe[1]);

  if (input_async)
    {
      ligma_waitable_wait (LIGMA_WAITABLE (input_async));

      g_object_unref (input_async);
    }

  close (input_pipe[0]);

  return TRUE;
}

static LigmaProgress *
gui_new_progress (Ligma        *ligma,
                  LigmaDisplay *display)
{
  g_return_val_if_fail (display == NULL || LIGMA_IS_DISPLAY (display), NULL);

  if (display)
    return LIGMA_PROGRESS (display);

  return LIGMA_PROGRESS (ligma_progress_dialog_new ());
}

static void
gui_free_progress (Ligma          *ligma,
                   LigmaProgress  *progress)
{
  g_return_if_fail (LIGMA_IS_PROGRESS (progress));

  if (LIGMA_IS_PROGRESS_DIALOG (progress))
    gtk_widget_destroy (GTK_WIDGET (progress));
}

static gboolean
gui_pdb_dialog_present (GtkWindow *window)
{
  gtk_window_present (window);

  return FALSE;
}

static gboolean
gui_pdb_dialog_new (Ligma          *ligma,
                    LigmaContext   *context,
                    LigmaProgress  *progress,
                    LigmaContainer *container,
                    const gchar   *title,
                    const gchar   *callback_name,
                    const gchar   *object_name,
                    va_list        args)
{
  GType        dialog_type = G_TYPE_NONE;
  const gchar *dialog_role = NULL;
  const gchar *help_id     = NULL;

  if (ligma_container_get_children_type (container) == LIGMA_TYPE_BRUSH)
    {
      dialog_type = LIGMA_TYPE_BRUSH_SELECT;
      dialog_role = "ligma-brush-selection";
      help_id     = LIGMA_HELP_BRUSH_DIALOG;
    }
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_FONT)
    {
      dialog_type = LIGMA_TYPE_FONT_SELECT;
      dialog_role = "ligma-font-selection";
      help_id     = LIGMA_HELP_FONT_DIALOG;
    }
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_GRADIENT)
    {
      dialog_type = LIGMA_TYPE_GRADIENT_SELECT;
      dialog_role = "ligma-gradient-selection";
      help_id     = LIGMA_HELP_GRADIENT_DIALOG;
    }
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_PALETTE)
    {
      dialog_type = LIGMA_TYPE_PALETTE_SELECT;
      dialog_role = "ligma-palette-selection";
      help_id     = LIGMA_HELP_PALETTE_DIALOG;
    }
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_PATTERN)
    {
      dialog_type = LIGMA_TYPE_PATTERN_SELECT;
      dialog_role = "ligma-pattern-selection";
      help_id     = LIGMA_HELP_PATTERN_DIALOG;
    }

  if (dialog_type != G_TYPE_NONE)
    {
      LigmaObject *object = NULL;

      if (object_name && strlen (object_name))
        object = ligma_container_get_child_by_name (container, object_name);

      if (! object)
        object = ligma_context_get_by_type (context,
                                           ligma_container_get_children_type (container));

      if (object)
        {
          gint        n_properties = 0;
          gchar     **names        = NULL;
          GValue     *values       = NULL;
          GtkWidget  *dialog;
          GtkWidget  *view;

          names = ligma_properties_append (dialog_type,
                                          &n_properties, names, &values,
                                          "title",          title,
                                          "role",           dialog_role,
                                          "help-func",      ligma_standard_help_func,
                                          "help-id",        help_id,
                                          "pdb",            ligma->pdb,
                                          "context",        context,
                                          "select-type",    ligma_container_get_children_type (container),
                                          "initial-object", object,
                                          "callback-name",  callback_name,
                                          "menu-factory",   global_menu_factory,
                                          NULL);

          names = ligma_properties_append_valist (dialog_type,
                                                 &n_properties, names, &values,
                                                 args);

          dialog = (GtkWidget *)
            g_object_new_with_properties (dialog_type,
                                          n_properties,
                                          (const gchar **) names,
                                          (const GValue *) values);

          ligma_properties_free (n_properties, names, values);

          view = LIGMA_PDB_DIALOG (dialog)->view;
          if (view)
            ligma_docked_set_show_button_bar (LIGMA_DOCKED (view), FALSE);

          if (progress)
            {
              guint32 window_id = ligma_progress_get_window_id (progress);

              if (window_id)
                ligma_window_set_transient_for (GTK_WINDOW (dialog), window_id);
            }

          gtk_widget_show (dialog);

          /*  workaround for bug #360106  */
          {
            GSource  *source = g_timeout_source_new (100);
            GClosure *closure;

            closure = g_cclosure_new_object (G_CALLBACK (gui_pdb_dialog_present),
                                             G_OBJECT (dialog));

            g_source_set_closure (source, closure);
            g_source_attach (source, NULL);
            g_source_unref (source);
          }

          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gui_pdb_dialog_set (Ligma          *ligma,
                    LigmaContainer *container,
                    const gchar   *callback_name,
                    const gchar   *object_name,
                    va_list        args)
{
  LigmaPdbDialogClass *klass = NULL;

  if (ligma_container_get_children_type (container) == LIGMA_TYPE_BRUSH)
    klass = g_type_class_peek (LIGMA_TYPE_BRUSH_SELECT);
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_FONT)
    klass = g_type_class_peek (LIGMA_TYPE_FONT_SELECT);
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_GRADIENT)
    klass = g_type_class_peek (LIGMA_TYPE_GRADIENT_SELECT);
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_PALETTE)
    klass = g_type_class_peek (LIGMA_TYPE_PALETTE_SELECT);
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_PATTERN)
    klass = g_type_class_peek (LIGMA_TYPE_PATTERN_SELECT);

  if (klass)
    {
      LigmaPdbDialog *dialog;

      dialog = ligma_pdb_dialog_get_by_callback (klass, callback_name);

      if (dialog && dialog->select_type == ligma_container_get_children_type (container))
        {
          LigmaObject *object;

          object = ligma_container_get_child_by_name (container, object_name);

          if (object)
            {
              const gchar *prop_name = va_arg (args, const gchar *);

              ligma_context_set_by_type (dialog->context, dialog->select_type,
                                        object);

              if (prop_name)
                g_object_set_valist (G_OBJECT (dialog), prop_name, args);

              gtk_window_present (GTK_WINDOW (dialog));

              return TRUE;
            }
        }
    }

  return FALSE;
}

static gboolean
gui_pdb_dialog_close (Ligma          *ligma,
                      LigmaContainer *container,
                      const gchar   *callback_name)
{
  LigmaPdbDialogClass *klass = NULL;

  if (ligma_container_get_children_type (container) == LIGMA_TYPE_BRUSH)
    klass = g_type_class_peek (LIGMA_TYPE_BRUSH_SELECT);
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_FONT)
    klass = g_type_class_peek (LIGMA_TYPE_FONT_SELECT);
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_GRADIENT)
    klass = g_type_class_peek (LIGMA_TYPE_GRADIENT_SELECT);
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_PALETTE)
    klass = g_type_class_peek (LIGMA_TYPE_PALETTE_SELECT);
  else if (ligma_container_get_children_type (container) == LIGMA_TYPE_PATTERN)
    klass = g_type_class_peek (LIGMA_TYPE_PATTERN_SELECT);

  if (klass)
    {
      LigmaPdbDialog *dialog;

      dialog = ligma_pdb_dialog_get_by_callback (klass, callback_name);

      if (dialog && dialog->select_type == ligma_container_get_children_type (container))
        {
          gtk_widget_destroy (GTK_WIDGET (dialog));
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gui_recent_list_add_file (Ligma        *ligma,
                          GFile       *file,
                          const gchar *mime_type)
{
  GtkRecentData  recent;
  const gchar   *groups[2] = { "Graphics", NULL };
  gchar         *uri;
  gboolean       success;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  /* use last part of the URI */
  recent.display_name = NULL;

  /* no special description */
  recent.description  = NULL;
  recent.mime_type    = (mime_type ?
                         (gchar *) mime_type : "application/octet-stream");
  recent.app_name     = "GNU Image Manipulation Program";
  recent.app_exec     = LIGMA_COMMAND " %u";
  recent.groups       = (gchar **) groups;
  recent.is_private   = FALSE;

  uri = g_file_get_uri (file);

  success = gtk_recent_manager_add_full (gtk_recent_manager_get_default (),
                                         uri, &recent);

  g_free (uri);

  return success;
}

static gint
gui_recent_list_compare (gconstpointer a,
                         gconstpointer b)
{
  return (gtk_recent_info_get_modified ((GtkRecentInfo *) a) -
          gtk_recent_info_get_modified ((GtkRecentInfo *) b));
}

static void
gui_recent_list_load (Ligma *ligma)
{
  GList *items;
  GList *list;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_container_freeze (ligma->documents);
  ligma_container_clear (ligma->documents);

  items = gtk_recent_manager_get_items (gtk_recent_manager_get_default ());

  items = g_list_sort (items, gui_recent_list_compare);

  for (list = items; list; list = list->next)
    {
      GtkRecentInfo *info = list->data;

      if (gtk_recent_info_has_application (info,
                                           "GNU Image Manipulation Program"))
        {
          const gchar *mime_type = gtk_recent_info_get_mime_type (info);

          if (mime_type &&
              ligma_plug_in_manager_file_procedure_find_by_mime_type (ligma->plug_in_manager,
                                                                     LIGMA_FILE_PROCEDURE_GROUP_OPEN,
                                                                     mime_type))
            {
              LigmaImagefile *imagefile;
              GFile         *file;

              file = g_file_new_for_uri (gtk_recent_info_get_uri (info));
              imagefile = ligma_imagefile_new (ligma, file);
              g_object_unref (file);

              ligma_imagefile_set_mime_type (imagefile, mime_type);

              ligma_container_add (ligma->documents, LIGMA_OBJECT (imagefile));
              g_object_unref (imagefile);
            }
        }

      gtk_recent_info_unref (info);
    }

  g_list_free (items);

  ligma_container_thaw (ligma->documents);
}

static GMountOperation *
gui_get_mount_operation (Ligma         *ligma,
                         LigmaProgress *progress)
{
  GtkWidget *toplevel = NULL;

  if (GTK_IS_WIDGET (progress))
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (progress));

  return gtk_mount_operation_new (GTK_WINDOW (toplevel));
}

static LigmaColorProfilePolicy
gui_query_profile_policy (Ligma                      *ligma,
                          LigmaImage                 *image,
                          LigmaContext               *context,
                          LigmaColorProfile         **dest_profile,
                          LigmaColorRenderingIntent  *intent,
                          gboolean                  *bpc,
                          gboolean                  *dont_ask)
{
  return color_profile_import_dialog_run (image, context, NULL,
                                          dest_profile,
                                          intent, bpc,
                                          dont_ask);
}

static LigmaMetadataRotationPolicy
gui_query_rotation_policy (Ligma        *ligma,
                           LigmaImage   *image,
                           LigmaContext *context,
                           gboolean    *dont_ask)
{
  return metadata_rotation_import_dialog_run (image, context,
                                              NULL, dont_ask);
}
