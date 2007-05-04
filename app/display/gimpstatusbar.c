/* GIMP - The GNU Image Manipulation Program Copyright (C) 1995
 * Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpimage.h"
#include "core/gimpunit.h"
#include "core/gimpprogress.h"

#include "widgets/gimpuimanager.h"
#include "widgets/gimpunitstore.h"
#include "widgets/gimpunitcombobox.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpscalecombobox.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


/* maximal width of the string holding the cursor-coordinates for
 * the status line
 */
#define CURSOR_LEN       256

#define MESSAGE_TIMEOUT  5000


typedef struct _GimpStatusbarMsg GimpStatusbarMsg;

struct _GimpStatusbarMsg
{
  guint  context_id;
  gchar *stock_id;
  gchar *text;
};


static void     gimp_statusbar_progress_iface_init (GimpProgressInterface *iface);

static void     gimp_statusbar_finalize           (GObject           *object);

static void     gimp_statusbar_destroy            (GtkObject         *object);

static GimpProgress *
                gimp_statusbar_progress_start     (GimpProgress      *progress,
                                                   const gchar       *message,
                                                   gboolean           cancelable);
static void     gimp_statusbar_progress_end       (GimpProgress      *progress);
static gboolean gimp_statusbar_progress_is_active (GimpProgress      *progress);
static void     gimp_statusbar_progress_set_text  (GimpProgress      *progress,
                                                   const gchar       *message);
static void     gimp_statusbar_progress_set_value (GimpProgress      *progress,
                                                   gdouble            percentage);
static gdouble  gimp_statusbar_progress_get_value (GimpProgress      *progress);
static void     gimp_statusbar_progress_pulse     (GimpProgress      *progress);
static gboolean gimp_statusbar_progress_message   (GimpProgress      *progress,
                                                   Gimp              *gimp,
                                                   GimpMessageSeverity severity,
                                                   const gchar       *domain,
                                                   const gchar       *message);
static void     gimp_statusbar_progress_canceled  (GtkWidget         *button,
                                                   GimpStatusbar     *statusbar);

static void     gimp_statusbar_progress_style_set (GtkWidget         *widget,
                                                   GtkStyle          *prev_style,
                                                   GimpStatusbar     *statusbar);
static gboolean gimp_statusbar_progress_expose    (GtkWidget         *widget,
                                                   GdkEventExpose    *event,
                                                   GimpStatusbar     *statusbar);

static void     gimp_statusbar_update             (GimpStatusbar     *statusbar);
static void     gimp_statusbar_unit_changed       (GimpUnitComboBox  *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_scale_changed      (GimpScaleComboBox *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_scaled       (GimpDisplayShell  *shell,
                                                   GimpStatusbar     *statusbar);
static guint    gimp_statusbar_get_context_id     (GimpStatusbar     *statusbar,
                                                   const gchar       *context);
static gboolean gimp_statusbar_temp_timeout       (GimpStatusbar     *statusbar);

static void     gimp_statusbar_msg_free           (GimpStatusbarMsg  *msg);

static gchar *  gimp_statusbar_vprintf            (const gchar       *format,
                                                   va_list            args);


G_DEFINE_TYPE_WITH_CODE (GimpStatusbar, gimp_statusbar, GTK_TYPE_HBOX,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_statusbar_progress_iface_init))

#define parent_class gimp_statusbar_parent_class


static void
gimp_statusbar_class_init (GimpStatusbarClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);

  object_class->finalize    = gimp_statusbar_finalize;

  gtk_object_class->destroy = gimp_statusbar_destroy;
}

static void
gimp_statusbar_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start     = gimp_statusbar_progress_start;
  iface->end       = gimp_statusbar_progress_end;
  iface->is_active = gimp_statusbar_progress_is_active;
  iface->set_text  = gimp_statusbar_progress_set_text;
  iface->set_value = gimp_statusbar_progress_set_value;
  iface->get_value = gimp_statusbar_progress_get_value;
  iface->pulse     = gimp_statusbar_progress_pulse;
  iface->message   = gimp_statusbar_progress_message;
}

static void
gimp_statusbar_init (GimpStatusbar *statusbar)
{
  GtkBox        *box = GTK_BOX (statusbar);
  GimpUnitStore *store;

  statusbar->shell          = NULL;
  statusbar->messages       = NULL;
  statusbar->context_ids    = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     g_free, NULL);
  statusbar->seq_context_id = 1;

  statusbar->temp_context_id =
    gimp_statusbar_get_context_id (statusbar, "gimp-statusbar-temp");

  statusbar->cursor_format_str[0] = '\0';
  statusbar->length_format_str[0] = '\0';

  statusbar->progress_active      = FALSE;

  box->spacing     = 2;
  box->homogeneous = FALSE;

  statusbar->cursor_frame = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (box, statusbar->cursor_frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (box, statusbar->cursor_frame, 0);
  gtk_widget_show (statusbar->cursor_frame);

  statusbar->cursor_label = gtk_label_new ("0, 0");
  gtk_misc_set_alignment (GTK_MISC (statusbar->cursor_label), 0.5, 0.5);
  gtk_container_add (GTK_CONTAINER (statusbar->cursor_frame),
                     statusbar->cursor_label);
  gtk_widget_show (statusbar->cursor_label);

  store = gimp_unit_store_new (2);
  statusbar->unit_combo = gimp_unit_combo_box_new_with_model (store);
  g_object_unref (store);

  GTK_WIDGET_UNSET_FLAGS (statusbar->unit_combo, GTK_CAN_FOCUS);
  g_object_set (statusbar->unit_combo, "focus-on-click", FALSE, NULL);
  gtk_container_add (GTK_CONTAINER (statusbar->cursor_frame),
                     statusbar->unit_combo);
  gtk_widget_show (statusbar->unit_combo);

  g_signal_connect (statusbar->unit_combo, "changed",
                    G_CALLBACK (gimp_statusbar_unit_changed),
                    statusbar);

  statusbar->scale_combo = gimp_scale_combo_box_new ();
  GTK_WIDGET_UNSET_FLAGS (statusbar->scale_combo, GTK_CAN_FOCUS);
  g_object_set (statusbar->scale_combo, "focus-on-click", FALSE, NULL);
  gtk_box_pack_start (box, statusbar->scale_combo, FALSE, FALSE, 0);
  gtk_widget_show (statusbar->scale_combo);

  g_signal_connect (statusbar->scale_combo, "changed",
                    G_CALLBACK (gimp_statusbar_scale_changed),
                    statusbar);

  statusbar->progressbar = gtk_progress_bar_new ();
  gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (statusbar->progressbar),
                                  PANGO_ELLIPSIZE_END);
  g_object_set (statusbar->progressbar,
                "text-xalign", 0.0,
                "text-yalign", 0.5,
                NULL);
  gtk_box_pack_start (box, statusbar->progressbar, TRUE, TRUE, 0);
  gtk_widget_show (statusbar->progressbar);

  g_signal_connect_after (statusbar->progressbar, "style-set",
                          G_CALLBACK (gimp_statusbar_progress_style_set),
                          statusbar);
  g_signal_connect_after (statusbar->progressbar, "expose-event",
                          G_CALLBACK (gimp_statusbar_progress_expose),
                          statusbar);

  statusbar->cancel_button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_sensitive (statusbar->cancel_button, FALSE);
  gtk_box_pack_start (box, statusbar->cancel_button, FALSE, FALSE, 0);
  GTK_WIDGET_UNSET_FLAGS (statusbar->cancel_button, GTK_CAN_FOCUS);

  g_signal_connect (statusbar->cancel_button, "clicked",
                    G_CALLBACK (gimp_statusbar_progress_canceled),
                    statusbar);
}

static void
gimp_statusbar_finalize (GObject *object)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (object);

  g_slist_foreach (statusbar->messages, (GFunc) gimp_statusbar_msg_free, NULL);
  g_slist_free (statusbar->messages);
  statusbar->messages = NULL;

  g_hash_table_destroy (statusbar->context_ids);
  statusbar->context_ids = NULL;

  g_free (statusbar->temp_spaces);
  statusbar->temp_spaces = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_statusbar_destroy (GtkObject *object)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (object);

  if (statusbar->temp_timeout_id)
    {
      g_source_remove (statusbar->temp_timeout_id);
      statusbar->temp_timeout_id = 0;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static GimpProgress *
gimp_statusbar_progress_start (GimpProgress *progress,
                               const gchar  *message,
                               gboolean      cancelable)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (! statusbar->progress_active)
    {
      GtkWidget *bar = statusbar->progressbar;

      gimp_statusbar_push (statusbar, "progress", "%s", message);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);
      gtk_widget_set_sensitive (statusbar->cancel_button, cancelable);

      if (cancelable)
        gtk_widget_show (statusbar->cancel_button);

      statusbar->progress_active = TRUE;

      if (GTK_WIDGET_DRAWABLE (bar))
        gdk_window_process_updates (bar->window, TRUE);

      return progress;
    }

  return NULL;
}

static void
gimp_statusbar_progress_end (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      GtkWidget *bar = statusbar->progressbar;

      statusbar->progress_active = FALSE;

      gimp_statusbar_pop (statusbar, "progress");
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);
      gtk_widget_set_sensitive (statusbar->cancel_button, FALSE);
      gtk_widget_hide (statusbar->cancel_button);
    }
}

static gboolean
gimp_statusbar_progress_is_active (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  return statusbar->progress_active;
}

static void
gimp_statusbar_progress_set_text (GimpProgress *progress,
                                  const gchar  *message)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      GtkWidget *bar = statusbar->progressbar;

      gimp_statusbar_replace (statusbar, "progress", "%s", message);

      if (GTK_WIDGET_DRAWABLE (bar))
        gdk_window_process_updates (bar->window, TRUE);
    }
}

static void
gimp_statusbar_progress_set_value (GimpProgress *progress,
                                   gdouble       percentage)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      GtkWidget *bar = statusbar->progressbar;

      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), percentage);

      if (GTK_WIDGET_DRAWABLE (bar))
        gdk_window_process_updates (bar->window, TRUE);
    }
}

static gdouble
gimp_statusbar_progress_get_value (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      GtkWidget *bar = statusbar->progressbar;

      return gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (bar));
    }

  return 0.0;
}

static void
gimp_statusbar_progress_pulse (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      GtkWidget *bar = statusbar->progressbar;

      gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));

      if (GTK_WIDGET_DRAWABLE (bar))
        gdk_window_process_updates (bar->window, TRUE);
    }
}

static gboolean
gimp_statusbar_progress_message (GimpProgress        *progress,
                                 Gimp                *gimp,
                                 GimpMessageSeverity  severity,
                                 const gchar         *domain,
                                 const gchar         *message)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  gimp_statusbar_push_temp (statusbar,
                            gimp_get_message_stock_id (severity),
                            "%s", message);

  return TRUE;
}

static void
gimp_statusbar_progress_canceled (GtkWidget     *button,
                                  GimpStatusbar *statusbar)
{
  if (statusbar->progress_active)
    gimp_progress_cancel (GIMP_PROGRESS (statusbar));
}

static void
gimp_statusbar_update (GimpStatusbar *statusbar)
{
  const gchar *text = NULL;

  if (statusbar->messages)
    {
      GimpStatusbarMsg *msg = statusbar->messages->data;

      /*  only allow progress messages while the progress is active  */
      if (statusbar->progress_active)
        {
          guint context_id = gimp_statusbar_get_context_id (statusbar,
                                                            "progress");

          if (context_id != msg->context_id)
            return;
        }

      text = msg->text;
    }

  if (text && statusbar->temp_timeout_id)
    {
      gchar *temp = g_strconcat (statusbar->temp_spaces, text, NULL);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progressbar),
                                 temp);
      g_free (temp);
    }
  else
    {
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progressbar),
                                 text ? text : "");
    }
}


/*  public functions  */

GtkWidget *
gimp_statusbar_new (GimpDisplayShell *shell)
{
  GimpStatusbar *statusbar;
  GtkAction     *action;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  statusbar = g_object_new (GIMP_TYPE_STATUSBAR, NULL);

  statusbar->shell = shell;

  action = gimp_ui_manager_find_action (shell->menubar_manager,
                                        "view", "view-zoom-other");

  if (action)
    {
      GimpScaleComboBox *combo = GIMP_SCALE_COMBO_BOX (statusbar->scale_combo);

      gimp_scale_combo_box_add_action (combo, action, _("Other..."));
    }

  g_signal_connect_object (shell, "scaled",
                           G_CALLBACK (gimp_statusbar_shell_scaled),
                           statusbar, 0);

  return GTK_WIDGET (statusbar);
}

void
gimp_statusbar_push (GimpStatusbar *statusbar,
                     const gchar   *context,
                     const gchar   *format,
                     ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  gimp_statusbar_push_valist (statusbar, context, format, args);
  va_end (args);
}

void
gimp_statusbar_push_valist (GimpStatusbar *statusbar,
                            const gchar   *context,
                            const gchar   *format,
                            va_list        args)
{
  GimpStatusbarMsg *msg;
  guint             context_id;
  GSList           *list;
  gchar            *message;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  message = gimp_statusbar_vprintf (format, args);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  if (statusbar->messages)
    {
      msg = statusbar->messages->data;

      if (msg->context_id == context_id && strcmp (msg->text, message) == 0)
        {
          g_free (message);
          return;
        }
    }

  for (list = statusbar->messages; list; list = g_slist_next (list))
    {
      msg = list->data;

      if (msg->context_id == context_id)
        {
          statusbar->messages = g_slist_remove (statusbar->messages, msg);
          gimp_statusbar_msg_free (msg);

          break;
        }
    }

  msg = g_new0 (GimpStatusbarMsg, 1);

  msg->context_id = context_id;
  msg->text       = message;

  if (statusbar->temp_timeout_id)
    statusbar->messages = g_slist_insert (statusbar->messages, msg, 1);
  else
    statusbar->messages = g_slist_prepend (statusbar->messages, msg);

  gimp_statusbar_update (statusbar);
}

void
gimp_statusbar_push_coords (GimpStatusbar *statusbar,
                            const gchar   *context,
                            const gchar   *title,
                            gdouble        x,
                            const gchar   *separator,
                            gdouble        y,
                            const gchar   *help)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (title != NULL);
  g_return_if_fail (separator != NULL);
  if (help == NULL)
    {
      help = "";
    }

  shell = statusbar->shell;

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      gimp_statusbar_push (statusbar, context,
                           statusbar->cursor_format_str,
                           title,
                           (gint) RINT (x),
                           separator,
                           (gint) RINT (y),
                           help);
    }
  else /* show real world units */
    {
      GimpImage *image       = shell->display->image;
      gdouble    unit_factor = _gimp_unit_get_factor (image->gimp,
                                                      shell->unit);

      gimp_statusbar_push (statusbar, context,
                           statusbar->cursor_format_str,
                           title,
                           x * unit_factor / image->xresolution,
                           separator,
                           y * unit_factor / image->yresolution,
                           help);
    }
}

void
gimp_statusbar_push_length (GimpStatusbar       *statusbar,
                            const gchar         *context,
                            const gchar         *title,
                            GimpOrientationType  axis,
                            gdouble              value,
                            const gchar         *help)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (title != NULL);
  if (help == NULL)
    {
      help = "";
    }

  shell = statusbar->shell;

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      gimp_statusbar_push (statusbar, context,
                           statusbar->length_format_str,
                           title,
                           (gint) RINT (value),
                           help);
    }
  else /* show real world units */
    {
      GimpImage *image       = shell->display->image;
      gdouble    resolution;
      gdouble    unit_factor = _gimp_unit_get_factor (image->gimp,
                                                      shell->unit);

      switch (axis)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          resolution = image->xresolution;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          resolution = image->yresolution;
          break;

        default:
          g_return_if_reached ();
          break;
        }

      gimp_statusbar_push (statusbar, context,
                           statusbar->length_format_str,
                           title,
                           value * unit_factor / resolution,
                           help);
    }
}

void
gimp_statusbar_replace (GimpStatusbar *statusbar,
                        const gchar   *context,
                        const gchar   *format,
                        ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  gimp_statusbar_replace_valist (statusbar, context, format, args);
  va_end (args);
}

void
gimp_statusbar_replace_valist (GimpStatusbar *statusbar,
                               const gchar   *context,
                               const gchar   *format,
                               va_list        args)
{
  GimpStatusbarMsg *msg;
  GSList           *list;
  guint             context_id;
  gchar            *message;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  message =  gimp_statusbar_vprintf (format, args);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  for (list = statusbar->messages; list; list = g_slist_next (list))
    {
      msg = list->data;

      if (msg->context_id == context_id)
        {
          g_free (msg->text);
          msg->text = message;

          if (list == statusbar->messages)
            gimp_statusbar_update (statusbar);

          return;
        }
    }

  msg = g_new0 (GimpStatusbarMsg, 1);

  msg->context_id = context_id;
  msg->text       = message;

  if (statusbar->temp_timeout_id)
    statusbar->messages = g_slist_insert (statusbar->messages, msg, 1);
  else
    statusbar->messages = g_slist_prepend (statusbar->messages, msg);

  gimp_statusbar_update (statusbar);
}

void
gimp_statusbar_pop (GimpStatusbar *statusbar,
                    const gchar   *context)
{
  GSList *list;
  guint   context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  for (list = statusbar->messages; list; list = list->next)
    {
      GimpStatusbarMsg *msg = list->data;

      if (msg->context_id == context_id)
        {
          statusbar->messages = g_slist_remove (statusbar->messages, msg);
          gimp_statusbar_msg_free (msg);

          break;
        }
    }

  gimp_statusbar_update (statusbar);
}

void
gimp_statusbar_push_temp (GimpStatusbar *statusbar,
                          const gchar   *stock_id,
                          const gchar   *format,
                          ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (format != NULL);

  va_start (args, format);
  gimp_statusbar_push_temp_valist (statusbar, stock_id, format, args);
  va_end (args);
}

void
gimp_statusbar_push_temp_valist (GimpStatusbar *statusbar,
                                 const gchar   *stock_id,
                                 const gchar   *format,
                                 va_list        args)
{
  GimpStatusbarMsg *msg = NULL;
  gchar            *message;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (format != NULL);

  message = gimp_statusbar_vprintf (format, args);

  if (statusbar->temp_timeout_id)
    g_source_remove (statusbar->temp_timeout_id);

  statusbar->temp_timeout_id =
    g_timeout_add (MESSAGE_TIMEOUT,
                   (GSourceFunc) gimp_statusbar_temp_timeout, statusbar);

  if (statusbar->messages)
    {
      msg = statusbar->messages->data;

      if (msg->context_id == statusbar->temp_context_id)
        {
          if (strcmp (msg->text, message) == 0)
            {
              g_free (message);
              return;
            }

          g_free (msg->stock_id);
          msg->stock_id = g_strdup (stock_id);

          g_free (msg->text);
          msg->text = message;

          gimp_statusbar_update (statusbar);
          return;
        }
    }

  msg = g_new0 (GimpStatusbarMsg, 1);

  msg->context_id = statusbar->temp_context_id;
  msg->stock_id   = g_strdup (stock_id);
  msg->text       = message;

  statusbar->messages = g_slist_prepend (statusbar->messages, msg);

  gimp_statusbar_update (statusbar);
}

void
gimp_statusbar_pop_temp (GimpStatusbar *statusbar)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  if (statusbar->temp_timeout_id)
    {
      g_source_remove (statusbar->temp_timeout_id);
      statusbar->temp_timeout_id = 0;
    }

  if (statusbar->messages)
    {
      GimpStatusbarMsg *msg = statusbar->messages->data;

      if (msg->context_id == statusbar->temp_context_id)
        {
          statusbar->messages = g_slist_remove (statusbar->messages, msg);
          gimp_statusbar_msg_free (msg);

          gimp_statusbar_update (statusbar);
        }
    }
}

void
gimp_statusbar_update_cursor (GimpStatusbar *statusbar,
                              gdouble        x,
                              gdouble        y)
{
  GimpDisplayShell *shell;
  GtkTreeModel     *model;
  GimpUnitStore    *store;
  gchar             buffer[CURSOR_LEN];

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  shell = statusbar->shell;

  if (x <  0 ||
      y <  0 ||
      x >= shell->display->image->width ||
      y >= shell->display->image->height)
    {
      gtk_widget_set_sensitive (statusbar->cursor_label, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (statusbar->cursor_label, TRUE);
    }

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
  store = GIMP_UNIT_STORE (model);

  gimp_unit_store_set_pixel_values (store, x, y);

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (buffer, sizeof (buffer),
                  statusbar->cursor_format_str,
                  "", (gint) RINT (x), ", ", (gint) RINT (y), "");
    }
  else /* show real world units */
    {
      gimp_unit_store_get_values (store, shell->unit, &x, &y);

      g_snprintf (buffer, sizeof (buffer),
                  statusbar->cursor_format_str,
                  "", x, ", ", y, "");
    }

  gtk_label_set_text (GTK_LABEL (statusbar->cursor_label), buffer);
}

void
gimp_statusbar_clear_cursor (GimpStatusbar *statusbar)
{
  gtk_label_set_text (GTK_LABEL (statusbar->cursor_label), "");
  gtk_widget_set_sensitive (statusbar->cursor_label, TRUE);
}


/*  private functions  */

static void
gimp_statusbar_progress_style_set (GtkWidget     *widget,
                                   GtkStyle      *prev_style,
                                   GimpStatusbar *statusbar)
{
  PangoLayout *layout;
  GdkPixbuf   *pixbuf;
  gint         n_spaces = 1;
  gint         layout_width;

  layout = gtk_widget_create_pango_layout (widget, " ");
  pixbuf = gtk_widget_render_icon (widget, GIMP_STOCK_WARNING,
                                   GTK_ICON_SIZE_MENU, NULL);

  pango_layout_get_pixel_size (layout, &layout_width, NULL);

  while (layout_width < gdk_pixbuf_get_width (pixbuf) + 4)
    {
      n_spaces++;

      statusbar->temp_spaces = g_realloc (statusbar->temp_spaces, n_spaces + 1);

      memset (statusbar->temp_spaces, ' ', n_spaces);
      statusbar->temp_spaces[n_spaces] = '\0';

      pango_layout_set_text (layout, statusbar->temp_spaces, -1);
      pango_layout_get_pixel_size (layout, &layout_width, NULL);
    }

  g_object_unref (layout);
  g_object_unref (pixbuf);
}

static gboolean
gimp_statusbar_progress_expose (GtkWidget      *widget,
                                GdkEventExpose *event,
                                GimpStatusbar  *statusbar)
{
  GdkPixbuf   *pixbuf;
  const gchar *stock_id = NULL;
  gint         text_xalign;
  gint         text_yalign;
  gint         x, y;

  if (statusbar->messages)
    {
      GimpStatusbarMsg *msg = statusbar->messages->data;

      stock_id = msg->stock_id;
    }

  if (! stock_id)
    return FALSE;

  pixbuf = gtk_widget_render_icon (widget, stock_id,
                                   GTK_ICON_SIZE_MENU, NULL);

  g_object_get (widget,
                "text-xalign", &text_xalign,
                "text-yalign", &text_yalign,
                NULL);

  x = (widget->style->xthickness + 2);
  y = ((widget->allocation.height - gdk_pixbuf_get_height (pixbuf)) / 2);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    x += (widget->allocation.width - 2 * (widget->style->xthickness + 1) -
          gdk_pixbuf_get_width (pixbuf));

  gdk_draw_pixbuf (widget->window, widget->style->black_gc,
                   pixbuf,
                   0, 0, x, y,
                   gdk_pixbuf_get_width (pixbuf),
                   gdk_pixbuf_get_height (pixbuf),
                   GDK_RGB_DITHER_NORMAL, 0, 0);

  g_object_unref (pixbuf);

  return FALSE;
}

static void
gimp_statusbar_shell_scaled (GimpDisplayShell *shell,
                             GimpStatusbar    *statusbar)
{
  static PangoLayout *layout = NULL;

  GimpImage    *image = shell->display->image;
  GtkTreeModel *model;
  const gchar  *text;
  gint          width;
  gint          diff;

  g_signal_handlers_block_by_func (statusbar->scale_combo,
                                   gimp_statusbar_scale_changed, statusbar);
  gimp_scale_combo_box_set_scale (GIMP_SCALE_COMBO_BOX (statusbar->scale_combo),
                                  gimp_zoom_model_get_factor (shell->zoom));
  g_signal_handlers_unblock_by_func (statusbar->scale_combo,
                                     gimp_statusbar_scale_changed, statusbar);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
  gimp_unit_store_set_resolutions (GIMP_UNIT_STORE (model),
                                   image->xresolution, image->yresolution);

  g_signal_handlers_block_by_func (statusbar->unit_combo,
                                   gimp_statusbar_unit_changed, statusbar);
  gimp_unit_combo_box_set_active (GIMP_UNIT_COMBO_BOX (statusbar->unit_combo),
                                  shell->unit);
  g_signal_handlers_unblock_by_func (statusbar->unit_combo,
                                     gimp_statusbar_unit_changed, statusbar);

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (statusbar->cursor_format_str,
                  sizeof (statusbar->cursor_format_str),
                  "%%s%%d%%s%%d%%s");
      g_snprintf (statusbar->length_format_str,
                  sizeof (statusbar->length_format_str),
                  "%%s%%d%%s");
    }
  else /* show real world units */
    {
      g_snprintf (statusbar->cursor_format_str,
                  sizeof (statusbar->cursor_format_str),
                  "%%s%%.%df%%s%%.%df%%s",
                  _gimp_unit_get_digits (image->gimp, shell->unit),
                  _gimp_unit_get_digits (image->gimp, shell->unit));
      g_snprintf (statusbar->length_format_str,
                  sizeof (statusbar->length_format_str),
                  "%%s%%.%df%%s",
                  _gimp_unit_get_digits (image->gimp, shell->unit));
    }

  gimp_statusbar_update_cursor (statusbar, -image->width, -image->height);

  text = gtk_label_get_text (GTK_LABEL (statusbar->cursor_label));

  /* one static layout for all displays should be fine */
  if (! layout)
    layout = gtk_widget_create_pango_layout (statusbar->cursor_label, text);
  else
    pango_layout_set_text (layout, text, -1);

  pango_layout_get_pixel_size (layout, &width, NULL);

  /*  find out how many pixels the label's parent frame is bigger than
   *  the label itself
   */
  diff = (statusbar->cursor_frame->allocation.width -
          statusbar->cursor_label->allocation.width);

  gtk_widget_set_size_request (statusbar->cursor_label, width, -1);

  /* don't resize if this is a new display */
  if (diff)
    gtk_widget_set_size_request (statusbar->cursor_frame, width + diff, -1);

  gimp_statusbar_clear_cursor (statusbar);
}

static void
gimp_statusbar_unit_changed (GimpUnitComboBox *combo,
                             GimpStatusbar    *statusbar)
{
  gimp_display_shell_set_unit (statusbar->shell,
                               gimp_unit_combo_box_get_active (combo));
}

static void
gimp_statusbar_scale_changed (GimpScaleComboBox *combo,
                              GimpStatusbar     *statusbar)
{
  gimp_display_shell_scale (statusbar->shell,
                            GIMP_ZOOM_TO,
                            gimp_scale_combo_box_get_scale (combo));
}

static guint
gimp_statusbar_get_context_id (GimpStatusbar *statusbar,
                               const gchar   *context)
{
  guint id = GPOINTER_TO_UINT (g_hash_table_lookup (statusbar->context_ids,
                                                    context));

  if (! id)
    {
      id = statusbar->seq_context_id++;

      g_hash_table_insert (statusbar->context_ids,
                           g_strdup (context), GUINT_TO_POINTER (id));
    }

  return id;
}

static gboolean
gimp_statusbar_temp_timeout (GimpStatusbar *statusbar)
{
  gimp_statusbar_pop_temp (statusbar);

  statusbar->temp_timeout_id = 0;

  return FALSE;
}

static void
gimp_statusbar_msg_free (GimpStatusbarMsg *msg)
{
  g_free (msg->stock_id);
  g_free (msg->text);
  g_free (msg);
}

static gchar *
gimp_statusbar_vprintf (const gchar *format,
                        va_list      args)
{
  gchar *message;
  gchar *newline;

  message = g_strdup_vprintf (format, args);

  /*  guard us from multi-line strings  */
  newline = strchr (message, '\n');
  if (newline)
    *newline = '\0';

  return message;
}
