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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimp-parallel.h"
#include "core/gimp-spawn.h"
#include "core/gimp-utils.h"
#include "core/gimpasync.h"
#include "core/gimpbrush.h"
#include "core/gimpcancelable.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpdrawable.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"
#include "core/gimplist.h"
#include "core/gimppalette.h"
#include "core/gimppattern.h"
#include "core/gimpprogress.h"
#include "core/gimpwaitable.h"

#include "text/gimpfont.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"

#include "plug-in/gimppluginmanager-file.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpbrushselect.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpfontselect.h"
#include "widgets/gimpgradientselect.h"
#include "widgets/gimphelp.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimppaletteselect.h"
#include "widgets/gimppatternselect.h"
#include "widgets/gimppickableselect.h"
#include "widgets/gimpprogressdialog.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplay-foreach.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpsinglewindowstrategy.h"
#include "display/gimpmultiwindowstrategy.h"

#include "actions/plug-in-actions.h"

#include "menus/menus.h"

#include "dialogs/color-profile-import-dialog.h"
#include "dialogs/metadata-rotation-import-dialog.h"

#include "gui.h"
#include "gui-message.h"
#include "gui-vtable.h"
#include "icon-themes.h"
#include "themes.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void           gui_ungrab                 (Gimp                *gimp);

static void           gui_set_busy               (Gimp                *gimp);
static void           gui_unset_busy             (Gimp                *gimp);

static void           gui_help                   (Gimp                *gimp,
                                                  GimpProgress        *progress,
                                                  const gchar         *help_domain,
                                                  const gchar         *help_id);
static const gchar  * gui_get_program_class      (Gimp                *gimp);
static gchar        * gui_get_display_name       (Gimp                *gimp,
                                                  gint                 display_id,
                                                  GObject            **monitor,
                                                  gint                *monitor_number);
static guint32        gui_get_user_time          (Gimp                *gimp);
static GFile        * gui_get_theme_dir          (Gimp                *gimp);
static GFile        * gui_get_icon_theme_dir     (Gimp                *gimp);
static GimpObject   * gui_get_window_strategy    (Gimp                *gimp);
static GimpDisplay  * gui_get_empty_display      (Gimp                *gimp);
static GBytes       * gui_display_get_window_id  (GimpDisplay         *display);
static GimpDisplay  * gui_display_create         (Gimp                *gimp,
                                                  GimpImage           *image,
                                                  GimpUnit            *unit,
                                                  gdouble              scale,
                                                  GObject             *monitor);
static void           gui_display_delete         (GimpDisplay         *display);
static void           gui_displays_reconnect     (Gimp                *gimp,
                                                  GimpImage           *old_image,
                                                  GimpImage           *new_image);
static gboolean       gui_wait                   (Gimp                *gimp,
                                                  GimpWaitable        *waitable,
                                                  const gchar         *message);
static GimpProgress * gui_new_progress           (Gimp                *gimp,
                                                  GimpDisplay         *display);
static void           gui_free_progress          (Gimp                *gimp,
                                                  GimpProgress        *progress);
static gboolean       gui_pdb_dialog_new         (Gimp                *gimp,
                                                  GimpContext         *context,
                                                  GimpProgress        *progress,
                                                  GType                object_type,
                                                  GBytes              *parent_handle,
                                                  const gchar         *title,
                                                  const gchar         *callback_name,
                                                  GimpObject          *object,
                                                  va_list              args);
static gboolean       gui_pdb_dialog_set         (Gimp                *gimp,
                                                  GType                contents_type,
                                                  const gchar         *callback_name,
                                                  GimpObject          *object,
                                                  va_list              args);
static gboolean       gui_pdb_dialog_close       (Gimp                *gimp,
                                                  GType                contents_type,
                                                  const gchar         *callback_name);
static gboolean       gui_recent_list_add_file   (Gimp                *gimp,
                                                  GFile               *file,
                                                  const gchar         *mime_type);
static void           gui_recent_list_load       (Gimp                *gimp);

static GMountOperation
                    * gui_get_mount_operation    (Gimp                *gimp,
                                                  GimpProgress        *progress);

static GimpColorProfilePolicy
                      gui_query_profile_policy   (Gimp                *gimp,
                                                  GimpImage           *image,
                                                  GimpContext         *context,
                                                  GimpColorProfile   **dest_profile,
                                                  GimpColorRenderingIntent *intent,
                                                  gboolean            *bpc,
                                                  gboolean            *dont_ask);
static GimpMetadataRotationPolicy
                      gui_query_rotation_policy  (Gimp                *gimp,
                                                  GimpImage           *image,
                                                  GimpContext         *context,
                                                  gboolean            *dont_ask);

static void           gui_inhibit                (Gimp                *gimp);
static void           gui_image_disconnect       (GimpImage           *image,
                                                  Gimp                *gimp);


/*  public functions  */

void
gui_vtable_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->gui.ungrab                 = gui_ungrab;
  gimp->gui.set_busy               = gui_set_busy;
  gimp->gui.unset_busy             = gui_unset_busy;
  gimp->gui.show_message           = gui_message;
  gimp->gui.help                   = gui_help;
  gimp->gui.get_program_class      = gui_get_program_class;
  gimp->gui.get_display_name       = gui_get_display_name;
  gimp->gui.get_user_time          = gui_get_user_time;
  gimp->gui.get_theme_dir          = gui_get_theme_dir;
  gimp->gui.get_icon_theme_dir     = gui_get_icon_theme_dir;
  gimp->gui.get_window_strategy    = gui_get_window_strategy;
  gimp->gui.get_empty_display      = gui_get_empty_display;
  gimp->gui.display_get_window_id  = gui_display_get_window_id;
  gimp->gui.display_create         = gui_display_create;
  gimp->gui.display_delete         = gui_display_delete;
  gimp->gui.displays_reconnect     = gui_displays_reconnect;
  gimp->gui.wait                   = gui_wait;
  gimp->gui.progress_new           = gui_new_progress;
  gimp->gui.progress_free          = gui_free_progress;
  gimp->gui.pdb_dialog_new         = gui_pdb_dialog_new;
  gimp->gui.pdb_dialog_set         = gui_pdb_dialog_set;
  gimp->gui.pdb_dialog_close       = gui_pdb_dialog_close;
  gimp->gui.recent_list_add_file   = gui_recent_list_add_file;
  gimp->gui.recent_list_load       = gui_recent_list_load;
  gimp->gui.get_mount_operation    = gui_get_mount_operation;
  gimp->gui.query_profile_policy   = gui_query_profile_policy;
  gimp->gui.query_rotation_policy  = gui_query_rotation_policy;
}


/*  private functions  */

static void
gui_ungrab (Gimp *gimp)
{
  GdkDisplay *display = gdk_display_get_default ();

  if (display)
    gdk_seat_ungrab (gdk_display_get_default_seat (display));
}

static void
gui_set_busy (Gimp *gimp)
{
  gimp_displays_set_busy (gimp);
  gimp_dialog_factory_set_busy (gimp_dialog_factory_get_singleton ());

  gdk_display_flush (gdk_display_get_default ());
}

static void
gui_unset_busy (Gimp *gimp)
{
  gimp_displays_unset_busy (gimp);
  gimp_dialog_factory_unset_busy (gimp_dialog_factory_get_singleton ());

  gdk_display_flush (gdk_display_get_default ());
}

static void
gui_help (Gimp         *gimp,
          GimpProgress *progress,
          const gchar  *help_domain,
          const gchar  *help_id)
{
  gimp_help_show (gimp, progress, help_domain, help_id);
}

static const gchar *
gui_get_program_class (Gimp *gimp)
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
gui_get_display_name (Gimp     *gimp,
                      gint      display_id,
                      GObject **monitor,
                      gint     *monitor_number)
{
  GimpDisplay *display = NULL;
  GdkDisplay  *gdk_display;

  if (display_id > 0)
    display = gimp_display_get_by_id (gimp, display_id);

  if (display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (display);

      gdk_display = gtk_widget_get_display (GTK_WIDGET (shell));

      *monitor = G_OBJECT (gimp_widget_get_monitor (GTK_WIDGET (shell)));
    }
  else
    {
      *monitor = G_OBJECT (gui_get_initial_monitor (gimp));

      if (! *monitor)
        *monitor = G_OBJECT (gimp_get_monitor_at_pointer ());

      gdk_display = gdk_monitor_get_display (GDK_MONITOR (*monitor));
    }

  *monitor_number = get_monitor_number (GDK_MONITOR (*monitor));

  return g_strdup (gdk_display_get_name (gdk_display));
}

static guint32
gui_get_user_time (Gimp *gimp)
{
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    return gdk_x11_display_get_user_time (gdk_display_get_default ());
#endif

  return gtk_get_current_event_time ();
}

static GFile *
gui_get_theme_dir (Gimp *gimp)
{
  return themes_get_theme_dir (gimp, GIMP_GUI_CONFIG (gimp->config)->theme);
}

static GFile *
gui_get_icon_theme_dir (Gimp *gimp)
{
  return icon_themes_get_theme_dir (gimp, GIMP_GUI_CONFIG (gimp->config)->icon_theme);
}

static GimpObject *
gui_get_window_strategy (Gimp *gimp)
{
  if (GIMP_GUI_CONFIG (gimp->config)->single_window_mode)
    return gimp_single_window_strategy_get_singleton ();
  else
    return gimp_multi_window_strategy_get_singleton ();
}

static GimpDisplay *
gui_get_empty_display (Gimp *gimp)
{
  GimpDisplay *display = NULL;

  if (gimp_container_get_n_children (gimp->displays) == 1)
    {
      display = (GimpDisplay *) gimp_container_get_first_child (gimp->displays);

      if (gimp_display_get_image (display))
        {
          /* The display was not empty */
          display = NULL;
        }
    }

  return display;
}

static GBytes *
gui_display_get_window_id (GimpDisplay *display)
{
  GimpDisplay      *disp  = GIMP_DISPLAY (display);
  GimpDisplayShell *shell = gimp_display_get_shell (disp);

  if (shell)
    {
      if (shell)
        return g_bytes_ref (shell->window_handle);
    }

  return NULL;
}

static GimpDisplay *
gui_display_create (Gimp      *gimp,
                    GimpImage *image,
                    GimpUnit  *unit,
                    gdouble    scale,
                    GObject   *monitor)
{
  GimpContext *context = gimp_get_user_context (gimp);
  GimpDisplay *display = GIMP_DISPLAY (gui_get_empty_display (gimp));

  if (! monitor)
    monitor = G_OBJECT (gimp_get_monitor_at_pointer ());

  if (display)
    {
      gimp_display_fill (display, image, unit, scale);
    }
  else
    {
      GList *image_managers = gimp_ui_managers_from_name ("<Image>");

      g_return_val_if_fail (image_managers != NULL, NULL);

      display = gimp_display_new (gimp, image, unit, scale,
                                  image_managers->data,
                                  gimp_dialog_factory_get_singleton (),
                                  GDK_MONITOR (monitor));
   }

  if (gimp_context_get_display (context) == display)
    {
      gimp_context_set_image (context, image);
      gimp_context_display_changed (context);
    }
  else
    {
      gimp_context_set_display (context, display);
    }

  if (image)
    {
      g_signal_handlers_disconnect_by_func (image,
                                            G_CALLBACK (gui_inhibit),
                                            gimp);
      g_signal_handlers_disconnect_by_func (image,
                                            G_CALLBACK (gui_image_disconnect),
                                            gimp);
      g_signal_connect_swapped (image, "dirty",
                                G_CALLBACK (gui_inhibit),
                                gimp);
      g_signal_connect_swapped (image, "clean",
                                G_CALLBACK (gui_inhibit),
                                gimp);
      g_signal_connect_after (image, "disconnect",
                              G_CALLBACK (gui_image_disconnect),
                              gimp);
    }

  return display;
}

static void
gui_display_delete (GimpDisplay *display)
{
  gimp_display_close (display);
}

static void
gui_displays_reconnect (Gimp      *gimp,
                        GimpImage *old_image,
                        GimpImage *new_image)
{
  gimp_displays_reconnect (gimp, old_image, new_image);
}

static void
gui_wait_input_async (GimpAsync  *async,
                      const gint  input_pipe[2])
{
  guint8 buffer[1];

  while (read (input_pipe[0], buffer, sizeof (buffer)) == -1 &&
         errno == EINTR);

  gimp_async_finish (async, NULL);
}

static gboolean
gui_wait (Gimp         *gimp,
          GimpWaitable *waitable,
          const gchar  *message)
{
  GimpProcedure  *procedure;
  GimpValueArray *args;
  GimpAsync      *input_async = NULL;
  GError         *error       = NULL;
  gint            input_pipe[2];
  gint            output_pipe[2];

  procedure = gimp_pdb_lookup_procedure (gimp->pdb, "plug-in-busy-dialog");

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

  gimp_spawn_set_cloexec (input_pipe[0]);
  gimp_spawn_set_cloexec (output_pipe[1]);

  args = gimp_procedure_get_arguments (procedure);
  gimp_value_array_truncate (args, 5);

  g_value_set_enum    (gimp_value_array_index (args, 0),
                       GIMP_RUN_INTERACTIVE);
  g_value_set_int     (gimp_value_array_index (args, 1),
                       output_pipe[0]);
  g_value_set_int     (gimp_value_array_index (args, 2),
                       input_pipe[1]);
  g_value_set_string  (gimp_value_array_index (args, 3),
                       message);
  g_value_set_boolean (gimp_value_array_index (args, 4),
                       GIMP_IS_CANCELABLE (waitable));

  gimp_procedure_execute_async (procedure, gimp,
                                gimp_get_user_context (gimp),
                                NULL, args, NULL, &error);

  gimp_value_array_unref (args);

  close (input_pipe[1]);
  close (output_pipe[0]);

  if (error)
    {
      g_clear_error (&error);

      close (input_pipe[0]);
      close (output_pipe[1]);

      return FALSE;
    }

  if (GIMP_IS_CANCELABLE (waitable))
    {
      /* listens for a cancellation request */
      input_async = gimp_parallel_run_async_independent (
        (GimpRunAsyncFunc) gui_wait_input_async,
        input_pipe);

      while (! gimp_waitable_wait_for (waitable, 0.1 * G_TIME_SPAN_SECOND))
        {
          /* check for a cancellation request */
          if (gimp_waitable_try_wait (GIMP_WAITABLE (input_async)))
            {
              gimp_cancelable_cancel (GIMP_CANCELABLE (waitable));

              break;
            }
        }
    }

  gimp_waitable_wait (waitable);

  /* signal completion to the plug-in */
  close (output_pipe[1]);

  if (input_async)
    {
      gimp_waitable_wait (GIMP_WAITABLE (input_async));

      g_object_unref (input_async);
    }

  close (input_pipe[0]);

  return TRUE;
}

static GimpProgress *
gui_new_progress (Gimp        *gimp,
                  GimpDisplay *display)
{
  g_return_val_if_fail (display == NULL || GIMP_IS_DISPLAY (display), NULL);

  if (display)
    return GIMP_PROGRESS (display);

  return GIMP_PROGRESS (gimp_progress_dialog_new ());
}

static void
gui_free_progress (Gimp          *gimp,
                   GimpProgress  *progress)
{
  g_return_if_fail (GIMP_IS_PROGRESS (progress));

  if (GIMP_IS_PROGRESS_DIALOG (progress))
    gtk_widget_destroy (GTK_WIDGET (progress));
}

static gboolean
gui_pdb_dialog_present (GtkWindow *window)
{
  gtk_window_present (window);

  return FALSE;
}

static gboolean
gui_pdb_dialog_new (Gimp          *gimp,
                    GimpContext   *context,
                    GimpProgress  *progress,
                    GType          contents_type,
                    GBytes        *parent_handle,
                    const gchar   *title,
                    const gchar   *callback_name,
                    GimpObject    *object,
                    va_list        args)
{
  GType        dialog_type = G_TYPE_NONE;
  const gchar *dialog_role = NULL;
  const gchar *help_id     = NULL;

  if (contents_type == GIMP_TYPE_BRUSH)
    {
      dialog_type = GIMP_TYPE_BRUSH_SELECT;
      dialog_role = "gimp-brush-selection";
      help_id     = GIMP_HELP_BRUSH_DIALOG;
    }
  else if (contents_type == GIMP_TYPE_FONT)
    {
      dialog_type = GIMP_TYPE_FONT_SELECT;
      dialog_role = "gimp-font-selection";
      help_id     = GIMP_HELP_FONT_DIALOG;
    }
  else if (contents_type == GIMP_TYPE_GRADIENT)
    {
      dialog_type = GIMP_TYPE_GRADIENT_SELECT;
      dialog_role = "gimp-gradient-selection";
      help_id     = GIMP_HELP_GRADIENT_DIALOG;
    }
  else if (contents_type == GIMP_TYPE_PALETTE)
    {
      dialog_type = GIMP_TYPE_PALETTE_SELECT;
      dialog_role = "gimp-palette-selection";
      help_id     = GIMP_HELP_PALETTE_DIALOG;
    }
  else if (contents_type == GIMP_TYPE_PATTERN)
    {
      dialog_type = GIMP_TYPE_PATTERN_SELECT;
      dialog_role = "gimp-pattern-selection";
      help_id     = GIMP_HELP_PATTERN_DIALOG;
    }
  else if (g_type_is_a (contents_type, GIMP_TYPE_DRAWABLE))
    {
      dialog_type = GIMP_TYPE_PICKABLE_SELECT;
      dialog_role = "gimp-pickable-selection";
    }
  else
    {
      g_return_val_if_reached (FALSE);
    }

  if (dialog_type != G_TYPE_NONE)
    {
      if (! object && ! g_type_is_a (contents_type, GIMP_TYPE_DRAWABLE))
        object = gimp_context_get_by_type (context, contents_type);

      if (object || g_type_is_a (contents_type, GIMP_TYPE_DRAWABLE))
        {
          gint        n_properties = 0;
          gchar     **names        = NULL;
          GValue     *values       = NULL;
          GtkWidget  *dialog;
          GtkWidget  *view;
          gboolean    use_header_bar;

          g_object_get (gtk_settings_get_default (),
                        "gtk-dialogs-use-header", &use_header_bar,
                        NULL);

          names = gimp_properties_append (dialog_type,
                                          &n_properties, names, &values,
                                          "title",          title,
                                          "role",           dialog_role,
                                          "help-func",      gimp_standard_help_func,
                                          "help-id",        help_id,
                                          "pdb",            gimp->pdb,
                                          "context",        context,
                                          "select-type",    contents_type,
                                          "initial-object", object,
                                          "callback-name",  callback_name,
                                          "menu-factory",   menus_get_global_menu_factory (gimp),
                                          "use-header-bar", use_header_bar,
                                          NULL);

          names = gimp_properties_append_valist (dialog_type,
                                                 &n_properties, names, &values,
                                                 args);

          dialog = (GtkWidget *)
            g_object_new_with_properties (dialog_type,
                                          n_properties,
                                          (const gchar **) names,
                                          (const GValue *) values);

          gimp_properties_free (n_properties, names, values);

          view = GIMP_PDB_DIALOG (dialog)->view;
          if (view)
            gimp_docked_set_show_button_bar (GIMP_DOCKED (view), FALSE);

          if (progress)
            gimp_window_set_transient_for (GTK_WINDOW (dialog), progress);

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

          if (parent_handle != NULL)
            gimp_window_set_transient_for_handle (GTK_WINDOW (dialog), parent_handle);

          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gui_pdb_dialog_set (Gimp          *gimp,
                    GType          contents_type,
                    const gchar   *callback_name,
                    GimpObject    *object,
                    va_list        args)
{
  GimpPdbDialogClass *klass     = NULL;
  GimpContainer      *container = NULL;
  GimpPdbDialog      *dialog;

  if (contents_type == GIMP_TYPE_BRUSH)
    {
      klass = g_type_class_peek (GIMP_TYPE_BRUSH_SELECT);
      container = gimp_data_factory_get_container (gimp->brush_factory);
    }
  else if (contents_type == GIMP_TYPE_FONT)
    {
      klass = g_type_class_peek (GIMP_TYPE_FONT_SELECT);
      container = gimp_data_factory_get_container (gimp->font_factory);
    }
  else if (contents_type == GIMP_TYPE_GRADIENT)
    {
      klass = g_type_class_peek (GIMP_TYPE_GRADIENT_SELECT);
      container = gimp_data_factory_get_container (gimp->gradient_factory);
    }
  else if (contents_type == GIMP_TYPE_PALETTE)
    {
      klass = g_type_class_peek (GIMP_TYPE_PALETTE_SELECT);
      container = gimp_data_factory_get_container (gimp->palette_factory);
    }
  else if (contents_type == GIMP_TYPE_PATTERN)
    {
      klass = g_type_class_peek (GIMP_TYPE_PATTERN_SELECT);
      container = gimp_data_factory_get_container (gimp->pattern_factory);
    }
  else if (contents_type == GIMP_TYPE_DRAWABLE)
    {
      klass = g_type_class_peek (GIMP_TYPE_PICKABLE_SELECT);
    }

  g_return_val_if_fail (klass != NULL, FALSE);

  dialog = gimp_pdb_dialog_get_by_callback (klass, callback_name);

  if (dialog != NULL                       &&
      dialog->select_type == contents_type &&
      (container == NULL || gimp_container_get_child_index (container, object) != -1))
    {
      const gchar *prop_name = va_arg (args, const gchar *);

      if (g_type_is_a (contents_type, GIMP_TYPE_RESOURCE))
        {
          g_return_val_if_fail (container != NULL, FALSE);
          gimp_context_set_by_type (dialog->context, dialog->select_type, object);
        }
      else
        {
          g_return_val_if_fail (klass->set_object != NULL, FALSE);
          klass->set_object (dialog, object);
        }

      if (prop_name)
        g_object_set_valist (G_OBJECT (dialog), prop_name, args);

      gtk_window_present (GTK_WINDOW (dialog));

      return TRUE;
    }

  return FALSE;
}

static gboolean
gui_pdb_dialog_close (Gimp        *gimp,
                      GType        contents_type,
                      const gchar *callback_name)
{
  GimpPdbDialogClass *klass = NULL;

  if (contents_type == GIMP_TYPE_BRUSH)
    klass = g_type_class_peek (GIMP_TYPE_BRUSH_SELECT);
  else if (contents_type == GIMP_TYPE_FONT)
    klass = g_type_class_peek (GIMP_TYPE_FONT_SELECT);
  else if (contents_type == GIMP_TYPE_GRADIENT)
    klass = g_type_class_peek (GIMP_TYPE_GRADIENT_SELECT);
  else if (contents_type == GIMP_TYPE_PALETTE)
    klass = g_type_class_peek (GIMP_TYPE_PALETTE_SELECT);
  else if (contents_type == GIMP_TYPE_PATTERN)
    klass = g_type_class_peek (GIMP_TYPE_PATTERN_SELECT);
  else if (contents_type == GIMP_TYPE_DRAWABLE)
    klass = g_type_class_peek (GIMP_TYPE_PICKABLE_SELECT);

  if (klass)
    {
      GimpPdbDialog *dialog;

      dialog = gimp_pdb_dialog_get_by_callback (klass, callback_name);

      if (dialog && dialog->select_type == contents_type)
        {
          gtk_widget_destroy (GTK_WIDGET (dialog));
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
gui_recent_list_add_file (Gimp        *gimp,
                          GFile       *file,
                          const gchar *mime_type)
{
  GtkRecentData  recent;
  const gchar   *groups[2] = { "Graphics", NULL };
  gchar         *uri;
  gboolean       success;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);

  /* use last part of the URI */
  recent.display_name = NULL;

  /* no special description */
  recent.description  = NULL;
  recent.mime_type    = (mime_type ?
                         (gchar *) mime_type : "application/octet-stream");
  recent.app_name     = "GNU Image Manipulation Program";
  recent.app_exec     = GIMP_COMMAND " %u";
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
gui_recent_list_load (Gimp *gimp)
{
  GList *items;
  GList *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_container_freeze (gimp->documents);
  gimp_container_clear (gimp->documents);

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
              gimp_plug_in_manager_file_procedure_find_by_mime_type (gimp->plug_in_manager,
                                                                     GIMP_FILE_PROCEDURE_GROUP_OPEN,
                                                                     mime_type))
            {
              GimpImagefile *imagefile;
              GFile         *file;

              file = g_file_new_for_uri (gtk_recent_info_get_uri (info));
              imagefile = gimp_imagefile_new (gimp, file);
              g_object_unref (file);

              gimp_imagefile_set_mime_type (imagefile, mime_type);

              gimp_container_add (gimp->documents, GIMP_OBJECT (imagefile));
              g_object_unref (imagefile);
            }
        }

      gtk_recent_info_unref (info);
    }

  g_list_free (items);

  gimp_container_thaw (gimp->documents);
}

static GMountOperation *
gui_get_mount_operation (Gimp         *gimp,
                         GimpProgress *progress)
{
  GtkWidget *toplevel = NULL;

  if (GTK_IS_WIDGET (progress))
    toplevel = gtk_widget_get_toplevel (GTK_WIDGET (progress));

  return gtk_mount_operation_new (GTK_WINDOW (toplevel));
}

static GimpColorProfilePolicy
gui_query_profile_policy (Gimp                      *gimp,
                          GimpImage                 *image,
                          GimpContext               *context,
                          GimpColorProfile         **dest_profile,
                          GimpColorRenderingIntent  *intent,
                          gboolean                  *bpc,
                          gboolean                  *dont_ask)
{
  return color_profile_import_dialog_run (image, context, NULL,
                                          dest_profile,
                                          intent, bpc,
                                          dont_ask);
}

static GimpMetadataRotationPolicy
gui_query_rotation_policy (Gimp        *gimp,
                           GimpImage   *image,
                           GimpContext *context,
                           gboolean    *dont_ask)
{
  return metadata_rotation_import_dialog_run (image, context,
                                              NULL, dont_ask);
}

static void
gui_inhibit (Gimp *gimp)
{
  static gboolean  in_test      = TRUE;
  static gint      cookie       = 0;
  static gint      n_images     = 0;
  static gint64    last_failure = 0;
  GtkApplication  *app;
  GimpContainer   *images  = NULL;

  if (in_test)
    {
      /* Do not call inhibit code while unit-testing the UI code. */
      in_test = (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR") != NULL);
      if (in_test)
        return;
    }

  app = GTK_APPLICATION (g_application_get_default ());
  if (app == NULL)
    /* This may happen when quitting. The GtkApplication is finalized
     * faster than the images without display.
     */
    return;

  if (gimp_displays_dirty (gimp))
    {
      gint n_dirty_images;

      images         = gimp_displays_get_dirty_images (gimp);
      n_dirty_images = gimp_container_get_n_children (images);

      if (cookie && n_images == n_dirty_images)
        {
          g_object_unref (images);
          return;
        }

      n_images = n_dirty_images;
    }

  if (cookie != 0)
    {
      gtk_application_uninhibit (app, cookie);
      cookie = 0;
    }

  if (last_failure != 0 && g_get_monotonic_time () - last_failure < G_TIME_SPAN_MINUTE * 10)
    {
      /* Don't repeatedly try to inhibit when we are constantly failing.
       * Especially as in some case, it may lock the thread.
       */
      g_clear_object (&images);
      return;
    }
  last_failure = 0;

  if (gimp_displays_dirty (gimp))
    {
      GimpImage *image;
      GList     *list;
      GtkWindow *window = NULL;
      gchar     *reason;

      image = (GimpImage *) gimp_container_get_first_child (images);
      g_object_unref (images);

      for (list = gimp_get_display_iter (gimp); list; list = list->next)
        {
          GimpDisplay *display = list->data;

          if (gimp_display_get_image (display) == image)
            {
              GimpDisplayShell *shell;
              GtkWidget        *toplevel;

              shell    = gimp_display_get_shell (display);
              toplevel = gtk_widget_get_toplevel (GTK_WIDGET (shell));

              if (GTK_IS_WINDOW (toplevel))
                {
                  window = GTK_WINDOW (toplevel);
                  break;
                }
            }
        }

      /* TRANSLATORS: unless your language msgstr[0] applies to 1 only (as in
       * English), replace "one" with %d.
       */
      reason = g_strdup_printf (ngettext ("There is one image with unsaved changes!",
                                          "There are %d images with unsaved changes!",
                                          n_images),
                                n_images);
      cookie = gtk_application_inhibit (app, window,
                                        GTK_APPLICATION_INHIBIT_LOGOUT,
                                        reason);
      g_free (reason);

      if (cookie == 0)
        last_failure = g_get_monotonic_time ();
    }

  if (cookie == 0)
    n_images = 0;
}

static void
gui_image_disconnect (GimpImage *image,
                      Gimp      *gimp)
{
  gui_inhibit (gimp);

  g_signal_handlers_disconnect_by_func (image,
                                        G_CALLBACK (gui_inhibit),
                                        gimp);
  g_signal_handlers_disconnect_by_func (image,
                                        G_CALLBACK (gui_image_disconnect),
                                        gimp);
}
