/* GIMP - The GNU Image Manipulation Program Copyright (C) 1995
 * Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpimagewindow.h"
#include "gimpscalecombobox.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


/*  maximal width of the string holding the cursor-coordinates  */
#define CURSOR_LEN        256

/*  the spacing of the hbox                                     */
#define HBOX_SPACING        1

/*  spacing between the icon and the statusbar label            */
#define ICON_SPACING        2

/*  timeout (in milliseconds) for temporary statusbar messages  */
#define MESSAGE_TIMEOUT  8000


typedef struct _GimpStatusbarMsg GimpStatusbarMsg;

struct _GimpStatusbarMsg
{
  guint  context_id;
  gchar *icon_name;
  gchar *text;
};


static void     gimp_statusbar_progress_iface_init (GimpProgressInterface *iface);

static void     gimp_statusbar_dispose            (GObject           *object);
static void     gimp_statusbar_finalize           (GObject           *object);

static void     gimp_statusbar_screen_changed     (GtkWidget         *widget,
                                                   GdkScreen         *previous);
static void     gimp_statusbar_style_set          (GtkWidget         *widget,
                                                   GtkStyle          *prev_style);

static void     gimp_statusbar_hbox_style_set     (GtkWidget         *widget,
                                                   GtkStyle          *prev_style,
                                                   GimpStatusbar     *statusbar);

static GimpProgress *
                gimp_statusbar_progress_start     (GimpProgress      *progress,
                                                   gboolean           cancellable,
                                                   const gchar       *message);
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

static gboolean gimp_statusbar_label_draw         (GtkWidget         *widget,
                                                   cairo_t           *cr,
                                                   GimpStatusbar     *statusbar);

static void     gimp_statusbar_update             (GimpStatusbar     *statusbar);
static void     gimp_statusbar_unit_changed       (GimpUnitComboBox  *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_scale_changed      (GimpScaleComboBox *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_scale_activated    (GimpScaleComboBox *combo,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_scaled       (GimpDisplayShell  *shell,
                                                   GimpStatusbar     *statusbar);
static void     gimp_statusbar_shell_status_notify(GimpDisplayShell  *shell,
                                                   const GParamSpec  *pspec,
                                                   GimpStatusbar     *statusbar);
static guint    gimp_statusbar_get_context_id     (GimpStatusbar     *statusbar,
                                                   const gchar       *context);
static gboolean gimp_statusbar_temp_timeout       (GimpStatusbar     *statusbar);

static void     gimp_statusbar_add_message        (GimpStatusbar     *statusbar,
                                                   guint              context_id,
                                                   const gchar       *icon_name,
                                                   const gchar       *format,
                                                   va_list            args,
                                                   gboolean           move_to_front) G_GNUC_PRINTF (4, 0);
static void     gimp_statusbar_remove_message     (GimpStatusbar     *statusbar,
                                                   guint              context_id);
static void     gimp_statusbar_msg_free           (GimpStatusbarMsg  *msg);

static gchar *  gimp_statusbar_vprintf            (const gchar       *format,
                                                   va_list            args) G_GNUC_PRINTF (1, 0);

static GdkPixbuf * gimp_statusbar_load_icon       (GimpStatusbar     *statusbar,
                                                   const gchar       *icon_name);


G_DEFINE_TYPE_WITH_CODE (GimpStatusbar, gimp_statusbar, GTK_TYPE_STATUSBAR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_statusbar_progress_iface_init))

#define parent_class gimp_statusbar_parent_class


static void
gimp_statusbar_class_init (GimpStatusbarClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose        = gimp_statusbar_dispose;
  object_class->finalize       = gimp_statusbar_finalize;

  widget_class->screen_changed = gimp_statusbar_screen_changed;
  widget_class->style_set      = gimp_statusbar_style_set;
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
  GtkWidget     *hbox;
  GtkWidget     *hbox2;
  GtkWidget     *image;
  GtkWidget     *label;
  GimpUnitStore *store;
  GList         *children;

  statusbar->shell          = NULL;
  statusbar->messages       = NULL;
  statusbar->context_ids    = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     g_free, NULL);
  statusbar->seq_context_id = 1;

  statusbar->temp_context_id =
    gimp_statusbar_get_context_id (statusbar, "gimp-statusbar-temp");

  statusbar->cursor_format_str[0]   = '\0';
  statusbar->cursor_format_str_f[0] = '\0';
  statusbar->length_format_str[0]   = '\0';

  statusbar->progress_active      = FALSE;
  statusbar->progress_shown       = FALSE;

  /*  remove the message label from the message area  */
  hbox = gtk_statusbar_get_message_area (GTK_STATUSBAR (statusbar));
  gtk_box_set_spacing (GTK_BOX (hbox), HBOX_SPACING);

  children = gtk_container_get_children (GTK_CONTAINER (hbox));
  statusbar->label = g_object_ref (children->data);
  g_list_free (children);

  gtk_container_remove (GTK_CONTAINER (hbox), statusbar->label);

  g_signal_connect_after (hbox, "style-set",
                          G_CALLBACK (gimp_statusbar_hbox_style_set),
                          statusbar);

  statusbar->cursor_label = gtk_label_new ("8888, 8888");
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->cursor_label, FALSE, FALSE, 0);
  gtk_widget_show (statusbar->cursor_label);

  store = gimp_unit_store_new (2);
  statusbar->unit_combo = gimp_unit_combo_box_new_with_model (store);
  g_object_unref (store);

  gtk_widget_set_can_focus (statusbar->unit_combo, FALSE);
  g_object_set (statusbar->unit_combo, "focus-on-click", FALSE, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->unit_combo, FALSE, FALSE, 0);
  gtk_widget_show (statusbar->unit_combo);

  g_signal_connect (statusbar->unit_combo, "changed",
                    G_CALLBACK (gimp_statusbar_unit_changed),
                    statusbar);

  statusbar->scale_combo = gimp_scale_combo_box_new ();
  gtk_widget_set_can_focus (statusbar->scale_combo, FALSE);
  g_object_set (statusbar->scale_combo, "focus-on-click", FALSE, NULL);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->scale_combo, FALSE, FALSE, 0);
  gtk_widget_show (statusbar->scale_combo);

  g_signal_connect (statusbar->scale_combo, "changed",
                    G_CALLBACK (gimp_statusbar_scale_changed),
                    statusbar);

  g_signal_connect (statusbar->scale_combo, "entry-activated",
                    G_CALLBACK (gimp_statusbar_scale_activated),
                    statusbar);

  /*  put the label back into the message area  */
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->label, TRUE, TRUE, 1);

  g_object_unref (statusbar->label);

  g_signal_connect_after (statusbar->label, "draw",
                          G_CALLBACK (gimp_statusbar_label_draw),
                          statusbar);

  statusbar->progressbar = g_object_new (GTK_TYPE_PROGRESS_BAR,
                                         "text-xalign", 0.0,
                                         "text-yalign", 0.5,
                                         "ellipsize",   PANGO_ELLIPSIZE_END,
                                         NULL);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->progressbar, TRUE, TRUE, 0);
  /*  don't show the progress bar  */

  /*  construct the cancel button's contents manually because we
   *  always want image and label regardless of settings, and we want
   *  a menu size image.
   */
  statusbar->cancel_button = gtk_button_new ();
  gtk_widget_set_can_focus (statusbar->cancel_button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (statusbar->cancel_button),
                         GTK_RELIEF_NONE);
  gtk_widget_set_sensitive (statusbar->cancel_button, FALSE);
  gtk_box_pack_end (GTK_BOX (hbox),
                    statusbar->cancel_button, FALSE, FALSE, 0);
  /*  don't show the cancel button  */

  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (statusbar->cancel_button), hbox2);
  gtk_widget_show (hbox2);

  image = gtk_image_new_from_icon_name ("gtk-cancel", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (hbox2), image, FALSE, FALSE, 2);
  gtk_widget_show (image);

  label = gtk_label_new ("Cancel");
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

  g_signal_connect (statusbar->cancel_button, "clicked",
                    G_CALLBACK (gimp_statusbar_progress_canceled),
                    statusbar);
}

static void
gimp_statusbar_dispose (GObject *object)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (object);

  if (statusbar->temp_timeout_id)
    {
      g_source_remove (statusbar->temp_timeout_id);
      statusbar->temp_timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_statusbar_finalize (GObject *object)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (object);

  if (statusbar->icon)
    {
      g_object_unref (statusbar->icon);
      statusbar->icon = NULL;
    }

  if (statusbar->icon_hash)
    {
      g_hash_table_unref (statusbar->icon_hash);
      statusbar->icon_hash = NULL;
    }

  g_slist_free_full (statusbar->messages,
                     (GDestroyNotify) gimp_statusbar_msg_free);
  statusbar->messages = NULL;

  if (statusbar->context_ids)
    {
      g_hash_table_destroy (statusbar->context_ids);
      statusbar->context_ids = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_statusbar_screen_changed (GtkWidget *widget,
                               GdkScreen *previous)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (widget);

  if (GTK_WIDGET_CLASS (parent_class)->screen_changed)
    GTK_WIDGET_CLASS (parent_class)->screen_changed (widget, previous);

  if (statusbar->icon_hash)
    {
      g_hash_table_unref (statusbar->icon_hash);
      statusbar->icon_hash = NULL;
    }
}

static void
gimp_statusbar_style_set (GtkWidget *widget,
                          GtkStyle  *prev_style)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (widget);

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  if (statusbar->icon_hash)
    {
      g_hash_table_unref (statusbar->icon_hash);
      statusbar->icon_hash = NULL;
    }
}

static void
gimp_statusbar_hbox_style_set (GtkWidget     *widget,
                               GtkStyle      *prev_style,
                               GimpStatusbar *statusbar)
{
  GtkRequisition  requisition;
  GtkRequisition  child_requisition;
  gint            width = 0;
  gint            height;

  gtk_widget_get_preferred_size (widget, &requisition, NULL);

  height = requisition.height;

  /*  also consider the children which can be invisible  */

  gtk_widget_get_preferred_size (statusbar->cursor_label,
                                 &child_requisition, NULL);
  width += child_requisition.width;
  height = MAX (height, child_requisition.height);

  gtk_widget_get_preferred_size (statusbar->unit_combo,
                                 &child_requisition, NULL);
  width += child_requisition.width;
  height = MAX (height, child_requisition.height);

  gtk_widget_get_preferred_size (statusbar->scale_combo,
                                 &child_requisition, NULL);
  width += child_requisition.width;
  height = MAX (height, child_requisition.height);

  gtk_widget_get_preferred_size (statusbar->progressbar,
                                 &child_requisition, NULL);
  height = MAX (height, child_requisition.height);

  gtk_widget_get_preferred_size (statusbar->label,
                                 &child_requisition, NULL);
  requisition->height = MAX (requisition->height,
                             child_requisition.height);

  gtk_widget_get_preferred_size (statusbar->cancel_button,
                                 &child_requisition, NULL);
  height = MAX (height, child_requisition.height);

  width = MAX (requisition.width, width + 32);

  gtk_widget_set_size_request (widget, width, height);
}

static GimpProgress *
gimp_statusbar_progress_start (GimpProgress *progress,
                               gboolean      cancellable,
                               const gchar  *message)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (! statusbar->progress_active)
    {
      GtkWidget     *bar = statusbar->progressbar;
      GtkAllocation  allocation;

      statusbar->progress_active = TRUE;
      statusbar->progress_value  = 0.0;

      gimp_statusbar_push (statusbar, "progress", NULL, "%s", message);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);
      gtk_widget_set_sensitive (statusbar->cancel_button, cancellable);

      if (cancellable)
        {
          if (message)
            {
              gchar *tooltip = g_strdup_printf (_("Cancel <i>%s</i>"), message);

              gimp_help_set_help_data_with_markup (statusbar->cancel_button,
                                                   tooltip, NULL);
              g_free (tooltip);
            }

          gtk_widget_show (statusbar->cancel_button);
        }

      gtk_widget_get_allocation (statusbar->label, &allocation);

      gtk_widget_show (statusbar->progressbar);
      gtk_widget_hide (statusbar->label);

      /*  This shit is needed so that the progress bar is drawn in the
       *  correct place in the cases where we suck completely and run
       *  an operation that blocks the GUI and doesn't let the main
       *  loop run.
       */
      gtk_container_resize_children (GTK_CONTAINER (statusbar));
      gtk_widget_size_allocate (statusbar->progressbar, &allocation);

      if (! gtk_widget_get_visible (GTK_WIDGET (statusbar)))
        {
          gtk_widget_show (GTK_WIDGET (statusbar));
          statusbar->progress_shown = TRUE;
        }

      gimp_widget_flush_expose (bar);

      gimp_statusbar_override_window_title (statusbar);

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

      if (statusbar->progress_shown)
        {
          gtk_widget_hide (GTK_WIDGET (statusbar));
          statusbar->progress_shown = FALSE;
        }

      statusbar->progress_active = FALSE;
      statusbar->progress_value  = 0.0;

      gtk_widget_hide (bar);
      gtk_widget_show (statusbar->label);

      gimp_statusbar_pop (statusbar, "progress");

      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);
      gtk_widget_set_sensitive (statusbar->cancel_button, FALSE);
      gtk_widget_hide (statusbar->cancel_button);

      gimp_statusbar_restore_window_title (statusbar);
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

      gimp_statusbar_replace (statusbar, "progress", NULL, "%s", message);

      gimp_widget_flush_expose (bar);

      gimp_statusbar_override_window_title (statusbar);
    }
}

static void
gimp_statusbar_progress_set_value (GimpProgress *progress,
                                   gdouble       percentage)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    {
      GtkWidget     *bar = statusbar->progressbar;
      GtkAllocation  allocation;

      gtk_widget_get_allocation (bar, &allocation);

      statusbar->progress_value = percentage;

      /* only update the progress bar if this causes a visible change */
      if (fabs (allocation.width *
                (percentage -
                 gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (bar)))) > 1.0)
        {
          gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), percentage);

          gimp_widget_flush_expose (bar);
        }
    }
}

static gdouble
gimp_statusbar_progress_get_value (GimpProgress *progress)
{
  GimpStatusbar *statusbar = GIMP_STATUSBAR (progress);

  if (statusbar->progress_active)
    return statusbar->progress_value;

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

      gimp_widget_flush_expose (bar);
    }
}

static gboolean
gimp_statusbar_progress_message (GimpProgress        *progress,
                                 Gimp                *gimp,
                                 GimpMessageSeverity  severity,
                                 const gchar         *domain,
                                 const gchar         *message)
{
  GimpStatusbar *statusbar  = GIMP_STATUSBAR (progress);
  PangoLayout   *layout;
  const gchar   *icon_name;
  gboolean       handle_msg = FALSE;

  /*  don't accept a message if we are already displaying a more severe one  */
  if (statusbar->temp_timeout_id && statusbar->temp_severity > severity)
    return FALSE;

  /*  we can only handle short one-liners  */
  layout = gtk_widget_create_pango_layout (statusbar->label, message);

  icon_name = gimp_get_message_icon_name (severity);

  if (pango_layout_get_line_count (layout) == 1)
    {
      GtkAllocation label_allocation;
      gint          width;

      gtk_widget_get_allocation (statusbar->label, &label_allocation);

      pango_layout_get_pixel_size (layout, &width, NULL);

      if (width < label_allocation.width)
        {
          if (icon_name)
            {
              GdkPixbuf *pixbuf;

              pixbuf = gimp_statusbar_load_icon (statusbar, icon_name);

              width += ICON_SPACING + gdk_pixbuf_get_width (pixbuf);

              g_object_unref (pixbuf);

              handle_msg = (width < label_allocation.width);
            }
          else
            {
              handle_msg = TRUE;
            }
        }
    }

  g_object_unref (layout);

  if (handle_msg)
    gimp_statusbar_push_temp (statusbar, severity, icon_name, "%s", message);

  return handle_msg;
}

static void
gimp_statusbar_progress_canceled (GtkWidget     *button,
                                  GimpStatusbar *statusbar)
{
  if (statusbar->progress_active)
    gimp_progress_cancel (GIMP_PROGRESS (statusbar));
}

static void
gimp_statusbar_set_text (GimpStatusbar *statusbar,
                         const gchar   *icon_name,
                         const gchar   *text)
{
  if (statusbar->progress_active)
    {
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progressbar),
                                 text);
    }
  else
    {
      if (statusbar->icon)
        {
          g_object_unref (statusbar->icon);
          statusbar->icon = NULL;
        }

      if (icon_name)
        statusbar->icon = gimp_statusbar_load_icon (statusbar, icon_name);

      if (statusbar->icon)
        {
          PangoAttrList  *attrs;
          PangoAttribute *attr;
          PangoRectangle  rect;
          gchar          *tmp;

          tmp = g_strconcat (" ", text, NULL);
          gtk_label_set_text (GTK_LABEL (statusbar->label), tmp);
          g_free (tmp);

          rect.x      = 0;
          rect.y      = 0;
          rect.width  = PANGO_SCALE * (gdk_pixbuf_get_width (statusbar->icon) +
                                       ICON_SPACING);
          rect.height = 0;

          attrs = pango_attr_list_new ();

          attr = pango_attr_shape_new (&rect, &rect);
          attr->start_index = 0;
          attr->end_index   = 1;
          pango_attr_list_insert (attrs, attr);

          gtk_label_set_attributes (GTK_LABEL (statusbar->label), attrs);
          pango_attr_list_unref (attrs);
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (statusbar->label), text);
          gtk_label_set_attributes (GTK_LABEL (statusbar->label), NULL);
        }
    }
}

static void
gimp_statusbar_update (GimpStatusbar *statusbar)
{
  GimpStatusbarMsg *msg = NULL;

  if (statusbar->messages)
    msg = statusbar->messages->data;

  if (msg && msg->text)
    {
      gimp_statusbar_set_text (statusbar, msg->icon_name, msg->text);
    }
  else
    {
      gimp_statusbar_set_text (statusbar, NULL, "");
    }
}


/*  public functions  */

GtkWidget *
gimp_statusbar_new (void)
{
  return g_object_new (GIMP_TYPE_STATUSBAR, NULL);
}

void
gimp_statusbar_set_shell (GimpStatusbar    *statusbar,
                          GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell == statusbar->shell)
    return;

  if (statusbar->shell)
    {
      g_signal_handlers_disconnect_by_func (statusbar->shell,
                                            gimp_statusbar_shell_scaled,
                                            statusbar);

      g_signal_handlers_disconnect_by_func (statusbar->shell,
                                            gimp_statusbar_shell_status_notify,
                                            statusbar);
    }

  statusbar->shell = shell;

  g_signal_connect_object (statusbar->shell, "scaled",
                           G_CALLBACK (gimp_statusbar_shell_scaled),
                           statusbar, 0);

  g_signal_connect_object (statusbar->shell, "notify::status",
                           G_CALLBACK (gimp_statusbar_shell_status_notify),
                           statusbar, 0);
}

gboolean
gimp_statusbar_get_visible (GimpStatusbar *statusbar)
{
  g_return_val_if_fail (GIMP_IS_STATUSBAR (statusbar), FALSE);

  if (statusbar->progress_shown)
    return FALSE;

  return gtk_widget_get_visible (GTK_WIDGET (statusbar));
}

void
gimp_statusbar_set_visible (GimpStatusbar *statusbar,
                            gboolean       visible)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  if (statusbar->progress_shown)
    {
      if (visible)
        {
          statusbar->progress_shown = FALSE;
          return;
        }
    }

  gtk_widget_set_visible (GTK_WIDGET (statusbar), visible);
}

void
gimp_statusbar_empty (GimpStatusbar *statusbar)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  gtk_widget_hide (statusbar->cursor_label);
  gtk_widget_hide (statusbar->unit_combo);
  gtk_widget_hide (statusbar->scale_combo);
}

void
gimp_statusbar_fill (GimpStatusbar *statusbar)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  gtk_widget_show (statusbar->cursor_label);
  gtk_widget_show (statusbar->unit_combo);
  gtk_widget_show (statusbar->scale_combo);
}

void
gimp_statusbar_override_window_title (GimpStatusbar *statusbar)
{
  GtkWidget *toplevel;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (statusbar));

  if (gimp_image_window_is_iconified (GIMP_IMAGE_WINDOW (toplevel)))
    {
      const gchar *message = gimp_statusbar_peek (statusbar, "progress");

      if (message)
        gtk_window_set_title (GTK_WINDOW (toplevel), message);
    }
}

void
gimp_statusbar_restore_window_title (GimpStatusbar *statusbar)
{
  GtkWidget *toplevel;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (statusbar));

  if (gimp_image_window_is_iconified (GIMP_IMAGE_WINDOW (toplevel)))
    {
      g_object_notify (G_OBJECT (statusbar->shell), "title");
    }
}

void
gimp_statusbar_push (GimpStatusbar *statusbar,
                     const gchar   *context,
                     const gchar   *icon_name,
                     const gchar   *format,
                     ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  gimp_statusbar_push_valist (statusbar, context, icon_name, format, args);
  va_end (args);
}

void
gimp_statusbar_push_valist (GimpStatusbar *statusbar,
                            const gchar   *context,
                            const gchar   *icon_name,
                            const gchar   *format,
                            va_list        args)
{
  guint context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  gimp_statusbar_add_message (statusbar,
                              context_id,
                              icon_name, format, args,
                              /*  move_to_front =  */ TRUE);
}

void
gimp_statusbar_push_coords (GimpStatusbar       *statusbar,
                            const gchar         *context,
                            const gchar         *icon_name,
                            GimpCursorPrecision  precision,
                            const gchar         *title,
                            gdouble              x,
                            const gchar         *separator,
                            gdouble              y,
                            const gchar         *help)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (title != NULL);
  g_return_if_fail (separator != NULL);

  if (help == NULL)
    help = "";

  shell = statusbar->shell;

  switch (precision)
    {
    case GIMP_CURSOR_PRECISION_PIXEL_CENTER:
      x = (gint) x;
      y = (gint) y;
      break;

    case GIMP_CURSOR_PRECISION_PIXEL_BORDER:
      x = RINT (x);
      y = RINT (y);
      break;

    case GIMP_CURSOR_PRECISION_SUBPIXEL:
      break;
    }

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      if (precision == GIMP_CURSOR_PRECISION_SUBPIXEL)
        {
          gimp_statusbar_push (statusbar, context,
                               icon_name,
                               statusbar->cursor_format_str_f,
                               title,
                               x,
                               separator,
                               y,
                               help);
        }
      else
        {
          gimp_statusbar_push (statusbar, context,
                               icon_name,
                               statusbar->cursor_format_str,
                               title,
                               (gint) RINT (x),
                               separator,
                               (gint) RINT (y),
                               help);
        }
    }
  else /* show real world units */
    {
      gdouble xres;
      gdouble yres;

      gimp_image_get_resolution (gimp_display_get_image (shell->display),
                                 &xres, &yres);

      gimp_statusbar_push (statusbar, context,
                           icon_name,
                           statusbar->cursor_format_str,
                           title,
                           gimp_pixels_to_units (x, shell->unit, xres),
                           separator,
                           gimp_pixels_to_units (y, shell->unit, yres),
                           help);
    }
}

void
gimp_statusbar_push_length (GimpStatusbar       *statusbar,
                            const gchar         *context,
                            const gchar         *icon_name,
                            const gchar         *title,
                            GimpOrientationType  axis,
                            gdouble              value,
                            const gchar         *help)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (title != NULL);

  if (help == NULL)
    help = "";

  shell = statusbar->shell;

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      gimp_statusbar_push (statusbar, context,
                           icon_name,
                           statusbar->length_format_str,
                           title,
                           (gint) RINT (value),
                           help);
    }
  else /* show real world units */
    {
      gdouble xres;
      gdouble yres;
      gdouble resolution;

      gimp_image_get_resolution (gimp_display_get_image (shell->display),
                                 &xres, &yres);

      switch (axis)
        {
        case GIMP_ORIENTATION_HORIZONTAL:
          resolution = xres;
          break;

        case GIMP_ORIENTATION_VERTICAL:
          resolution = yres;
          break;

        default:
          g_return_if_reached ();
          break;
        }

      gimp_statusbar_push (statusbar, context,
                           icon_name,
                           statusbar->length_format_str,
                           title,
                           gimp_pixels_to_units (value, shell->unit, resolution),
                           help);
    }
}

void
gimp_statusbar_replace (GimpStatusbar *statusbar,
                        const gchar   *context,
                        const gchar   *icon_name,
                        const gchar   *format,
                        ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  va_start (args, format);
  gimp_statusbar_replace_valist (statusbar, context, icon_name, format, args);
  va_end (args);
}

void
gimp_statusbar_replace_valist (GimpStatusbar *statusbar,
                               const gchar   *context,
                               const gchar   *icon_name,
                               const gchar   *format,
                               va_list        args)
{
  guint context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);
  g_return_if_fail (format != NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  gimp_statusbar_add_message (statusbar,
                              context_id,
                              icon_name, format, args,
                              /*  move_to_front =  */ FALSE);
}

const gchar *
gimp_statusbar_peek (GimpStatusbar *statusbar,
                     const gchar   *context)
{
  GSList *list;
  guint   context_id;

  g_return_val_if_fail (GIMP_IS_STATUSBAR (statusbar), NULL);
  g_return_val_if_fail (context != NULL, NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  for (list = statusbar->messages; list; list = list->next)
    {
      GimpStatusbarMsg *msg = list->data;

      if (msg->context_id == context_id)
        {
          return msg->text;
        }
    }

  return NULL;
}

void
gimp_statusbar_pop (GimpStatusbar *statusbar,
                    const gchar   *context)
{
  guint context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (context != NULL);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  gimp_statusbar_remove_message (statusbar,
                                 context_id);
}

void
gimp_statusbar_push_temp (GimpStatusbar       *statusbar,
                          GimpMessageSeverity  severity,
                          const gchar         *icon_name,
                          const gchar         *format,
                          ...)
{
  va_list args;

  va_start (args, format);
  gimp_statusbar_push_temp_valist (statusbar, severity, icon_name, format, args);
  va_end (args);
}

void
gimp_statusbar_push_temp_valist (GimpStatusbar       *statusbar,
                                 GimpMessageSeverity  severity,
                                 const gchar         *icon_name,
                                 const gchar         *format,
                                 va_list              args)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (severity <= GIMP_MESSAGE_WARNING);
  g_return_if_fail (format != NULL);

  /*  don't accept a message if we are already displaying a more severe one  */
  if (statusbar->temp_timeout_id && statusbar->temp_severity > severity)
    return;

  if (statusbar->temp_timeout_id)
    g_source_remove (statusbar->temp_timeout_id);

  statusbar->temp_timeout_id =
    g_timeout_add (MESSAGE_TIMEOUT,
                   (GSourceFunc) gimp_statusbar_temp_timeout, statusbar);

  statusbar->temp_severity = severity;

  gimp_statusbar_add_message (statusbar,
                              statusbar->temp_context_id,
                              icon_name, format, args,
                              /*  move_to_front =  */ TRUE);
}

void
gimp_statusbar_pop_temp (GimpStatusbar *statusbar)
{
  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  if (statusbar->temp_timeout_id)
    {
      g_source_remove (statusbar->temp_timeout_id);
      statusbar->temp_timeout_id = 0;

      gimp_statusbar_remove_message (statusbar,
                                     statusbar->temp_context_id);
    }
}

void
gimp_statusbar_update_cursor (GimpStatusbar       *statusbar,
                              GimpCursorPrecision  precision,
                              gdouble              x,
                              gdouble              y)
{
  GimpDisplayShell *shell;
  GimpImage        *image;
  gchar             buffer[CURSOR_LEN];

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  shell = statusbar->shell;
  image = gimp_display_get_image (shell->display);

  if (! image                            ||
      x <  0                             ||
      y <  0                             ||
      x >= gimp_image_get_width  (image) ||
      y >= gimp_image_get_height (image))
    {
      gtk_widget_set_sensitive (statusbar->cursor_label, FALSE);
    }
  else
    {
      gtk_widget_set_sensitive (statusbar->cursor_label, TRUE);
    }

  switch (precision)
    {
    case GIMP_CURSOR_PRECISION_PIXEL_CENTER:
      x = (gint) x;
      y = (gint) y;
      break;

    case GIMP_CURSOR_PRECISION_PIXEL_BORDER:
      x = RINT (x);
      y = RINT (y);
      break;

    case GIMP_CURSOR_PRECISION_SUBPIXEL:
      break;
    }

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      if (precision == GIMP_CURSOR_PRECISION_SUBPIXEL)
        {
          g_snprintf (buffer, sizeof (buffer),
                      statusbar->cursor_format_str_f,
                      "", x, ", ", y, "");
        }
      else
        {
          g_snprintf (buffer, sizeof (buffer),
                      statusbar->cursor_format_str,
                      "", (gint) RINT (x), ", ", (gint) RINT (y), "");
        }
    }
  else /* show real world units */
    {
      GtkTreeModel  *model;
      GimpUnitStore *store;

      model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
      store = GIMP_UNIT_STORE (model);

      gimp_unit_store_set_pixel_values (store, x, y);
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

static gboolean
gimp_statusbar_label_draw (GtkWidget     *widget,
                           cairo_t       *cr,
                           GimpStatusbar *statusbar)
{
  if (statusbar->icon)
    {
      PangoRectangle  rect;
      GtkAllocation   allocation;
      gint            x, y;

      gtk_label_get_layout_offsets (GTK_LABEL (widget), &x, &y);

      gtk_widget_get_allocation (widget, &allocation);
      x -= allocation.x;
      y -= allocation.y;

      pango_layout_index_to_pos (gtk_label_get_layout (GTK_LABEL (widget)), 0,
                                 &rect);

      /*  the rectangle width is negative when rendering right-to-left  */
      x += PANGO_PIXELS (rect.x) + (rect.width < 0 ?
                                    PANGO_PIXELS (rect.width) : 0);
      y += PANGO_PIXELS (rect.y);

      gdk_cairo_set_source_pixbuf (cr, statusbar->icon, x, y);
      cairo_paint (cr);
    }

  return FALSE;
}

static void
gimp_statusbar_shell_scaled (GimpDisplayShell *shell,
                             GimpStatusbar    *statusbar)
{
  static PangoLayout *layout = NULL;

  GimpImage    *image = gimp_display_get_image (shell->display);
  GtkTreeModel *model;
  const gchar  *text;
  gint          image_width;
  gint          image_height;
  gdouble       image_xres;
  gdouble       image_yres;
  gint          width;

  if (image)
    {
      image_width  = gimp_image_get_width  (image);
      image_height = gimp_image_get_height (image);
      gimp_image_get_resolution (image, &image_xres, &image_yres);
    }
  else
    {
      image_width  = shell->disp_width;
      image_height = shell->disp_height;
      image_xres   = shell->display->config->monitor_xres;
      image_yres   = shell->display->config->monitor_yres;
    }

  g_signal_handlers_block_by_func (statusbar->scale_combo,
                                   gimp_statusbar_scale_changed, statusbar);
  gimp_scale_combo_box_set_scale (GIMP_SCALE_COMBO_BOX (statusbar->scale_combo),
                                  gimp_zoom_model_get_factor (shell->zoom));
  g_signal_handlers_unblock_by_func (statusbar->scale_combo,
                                     gimp_statusbar_scale_changed, statusbar);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
  gimp_unit_store_set_resolutions (GIMP_UNIT_STORE (model),
                                   image_xres, image_yres);

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
      g_snprintf (statusbar->cursor_format_str_f,
                  sizeof (statusbar->cursor_format_str_f),
                  "%%s%%.1f%%s%%.1f%%s");
      g_snprintf (statusbar->length_format_str,
                  sizeof (statusbar->length_format_str),
                  "%%s%%d%%s");
    }
  else /* show real world units */
    {
      gint w_digits;
      gint h_digits;

      w_digits = gimp_unit_get_scaled_digits (shell->unit, image_xres);
      h_digits = gimp_unit_get_scaled_digits (shell->unit, image_yres);

      g_snprintf (statusbar->cursor_format_str,
                  sizeof (statusbar->cursor_format_str),
                  "%%s%%.%df%%s%%.%df%%s",
                  w_digits, h_digits);
      strcpy (statusbar->cursor_format_str_f, statusbar->cursor_format_str);
      g_snprintf (statusbar->length_format_str,
                  sizeof (statusbar->length_format_str),
                  "%%s%%.%df%%s", MAX (w_digits, h_digits));
    }

  gimp_statusbar_update_cursor (statusbar, GIMP_CURSOR_PRECISION_SUBPIXEL,
                                -image_width, -image_height);

  text = gtk_label_get_text (GTK_LABEL (statusbar->cursor_label));

  /* one static layout for all displays should be fine */
  if (! layout)
    layout = gtk_widget_create_pango_layout (statusbar->cursor_label, NULL);

  pango_layout_set_text (layout, text, -1);
  pango_layout_get_pixel_size (layout, &width, NULL);

  gtk_widget_set_size_request (statusbar->cursor_label, width, -1);

  gimp_statusbar_clear_cursor (statusbar);
}

static void
gimp_statusbar_shell_status_notify (GimpDisplayShell *shell,
                                    const GParamSpec *pspec,
                                    GimpStatusbar    *statusbar)
{
  gimp_statusbar_replace (statusbar, "title",
                          NULL, "%s", shell->status);
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
                            gimp_scale_combo_box_get_scale (combo),
                            GIMP_ZOOM_FOCUS_BEST_GUESS);
}

static void
gimp_statusbar_scale_activated (GimpScaleComboBox *combo,
                                GimpStatusbar     *statusbar)
{
  gtk_widget_grab_focus (statusbar->shell->canvas);
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

  return FALSE;
}

static void
gimp_statusbar_add_message (GimpStatusbar *statusbar,
                            guint          context_id,
                            const gchar   *icon_name,
                            const gchar   *format,
                            va_list        args,
                            gboolean       move_to_front)
{
  gchar            *message;
  GSList           *list;
  GimpStatusbarMsg *msg;
  gint              position;

  message = gimp_statusbar_vprintf (format, args);

  for (list = statusbar->messages; list; list = g_slist_next (list))
    {
      msg = list->data;

      if (msg->context_id == context_id)
        {
          gboolean is_front_message = (list == statusbar->messages);

          if ((is_front_message || ! move_to_front) &&
              strcmp (msg->text, message) == 0      &&
              g_strcmp0 (msg->icon_name, icon_name) == 0)
            {
              g_free (message);
              return;
            }

          if (move_to_front)
            {
              statusbar->messages = g_slist_remove (statusbar->messages, msg);
              gimp_statusbar_msg_free (msg);

              break;
            }
          else
            {
              g_free (msg->icon_name);
              msg->icon_name = g_strdup (icon_name);

              g_free (msg->text);
              msg->text = message;

              if (is_front_message)
                gimp_statusbar_update (statusbar);

              return;
            }
        }
    }

  msg = g_slice_new (GimpStatusbarMsg);

  msg->context_id = context_id;
  msg->icon_name  = g_strdup (icon_name);
  msg->text       = message;

  /*  find the position at which to insert the new message  */
  position = 0;
  /*  progress messages are always at the front of the list  */
  if (! (statusbar->progress_active &&
         context_id == gimp_statusbar_get_context_id (statusbar, "progress")))
    {
      if (statusbar->progress_active)
        position++;

      /*  temporary messages are in front of all other non-progress messages  */
      if (statusbar->temp_timeout_id &&
          context_id != statusbar->temp_context_id)
        position++;
    }

  statusbar->messages = g_slist_insert (statusbar->messages, msg, position);

  if (position == 0)
    gimp_statusbar_update (statusbar);
}

static void
gimp_statusbar_remove_message (GimpStatusbar *statusbar,
                               guint          context_id)
{
  GSList   *list;
  gboolean  needs_update = FALSE;

  for (list = statusbar->messages; list; list = g_slist_next (list))
    {
      GimpStatusbarMsg *msg = list->data;

      if (msg->context_id == context_id)
        {
          needs_update = (list == statusbar->messages);

          statusbar->messages = g_slist_remove (statusbar->messages, msg);
          gimp_statusbar_msg_free (msg);

          break;
        }
    }

  if (needs_update)
    gimp_statusbar_update (statusbar);
}

static void
gimp_statusbar_msg_free (GimpStatusbarMsg *msg)
{
  g_free (msg->icon_name);
  g_free (msg->text);

  g_slice_free (GimpStatusbarMsg, msg);
}

static gchar *
gimp_statusbar_vprintf (const gchar *format,
                        va_list      args)
{
  gchar *message;
  gchar *newline;

  message = g_strdup_vprintf (format, args);

  /*  guard us from multi-line strings  */
  newline = strchr (message, '\r');
  if (newline)
    *newline = '\0';

  newline = strchr (message, '\n');
  if (newline)
    *newline = '\0';

  return message;
}

static GdkPixbuf *
gimp_statusbar_load_icon (GimpStatusbar *statusbar,
                          const gchar   *icon_name)
{
  GdkPixbuf *icon;

  if (G_UNLIKELY (! statusbar->icon_hash))
    {
      statusbar->icon_hash =
        g_hash_table_new_full (g_str_hash,
                               g_str_equal,
                               (GDestroyNotify) g_free,
                               (GDestroyNotify) g_object_unref);
    }

  icon = g_hash_table_lookup (statusbar->icon_hash, icon_name);

  if (icon)
    return g_object_ref (icon);

  icon = gimp_widget_load_icon (statusbar->label, icon_name, 16);

  /* this is not optimal but so what */
  if (g_hash_table_size (statusbar->icon_hash) > 16)
    g_hash_table_remove_all (statusbar->icon_hash);

  g_hash_table_insert (statusbar->icon_hash,
                       g_strdup (icon_name), g_object_ref (icon));

  return icon;
}
