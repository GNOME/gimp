/* The GIMP -- an image manipulation program
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

#include "display-types.h"

#include "core/gimpimage.h"
#include "core/gimpunit.h"
#include "core/gimpmarshal.h"

#include "widgets/gimpunitstore.h"
#include "widgets/gimpunitcombobox.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scale.h"
#include "gimpscalecombobox.h"
#include "gimpstatusbar.h"

#include "gimp-intl.h"


typedef struct _GimpStatusbarMsg GimpStatusbarMsg;

struct _GimpStatusbarMsg
{
  gchar *text;
  guint  context_id;
  guint  message_id;
};

enum
{
  SIGNAL_TEXT_PUSHED,
  SIGNAL_TEXT_POPPED,
  SIGNAL_LAST
};

/* maximal width of the string holding the cursor-coordinates for
 * the status line
 */
#define CURSOR_LEN 256


static void   gimp_statusbar_class_init    (GimpStatusbarClass *klass);
static void   gimp_statusbar_init          (GimpStatusbar      *statusbar);
static void   gimp_statusbar_destroy       (GtkObject          *object);

static void   gimp_statusbar_update        (GimpStatusbar      *statusbar,
                                            const gchar        *context_id,
                                            const gchar        *text);
static void   gimp_statusbar_unit_changed  (GimpUnitComboBox   *combo,
                                            GimpStatusbar      *statusbar);
static void   gimp_statusbar_scale_changed (GimpScaleComboBox  *combo,
                                            GimpStatusbar      *statusbar);
static void   gimp_statusbar_shell_scaled  (GimpDisplayShell   *shell,
                                            GimpStatusbar      *statusbar);


static GtkHBoxClass *parent_class;
static guint         statusbar_signals[SIGNAL_LAST] = { 0 };


GType
gimp_statusbar_get_type (void)
{
  static GType statusbar_type = 0;

  if (! statusbar_type)
    {
      static const GTypeInfo statusbar_info =
      {
        sizeof (GimpStatusbarClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_statusbar_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpStatusbar),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_statusbar_init,
      };

      statusbar_type = g_type_register_static (GTK_TYPE_HBOX,
                                               "GimpStatusbar",
                                               &statusbar_info, 0);
    }

  return statusbar_type;
}

static void
gimp_statusbar_class_init (GimpStatusbarClass *klass)
{


  GObjectClass      *g_object_class      = G_OBJECT_CLASS (klass);
  GtkObjectClass    *gtk_object_class    = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass    *gtk_widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *gtk_container_class = GTK_CONTAINER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gtk_object_class->destroy = gimp_statusbar_destroy;

  klass->messages_mem_chunk = g_mem_chunk_new ("GimpStatusbar messages mem chunk",
                                               sizeof (GimpStatusbarMsg),
                                               sizeof (GimpStatusbarMsg) * 64,
                                               G_ALLOC_AND_FREE);

  statusbar_signals[SIGNAL_TEXT_PUSHED] =
    g_signal_new ("text_pushed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpStatusbarClass, text_pushed),
                  NULL, NULL,
                  gimp_marshal_VOID__UINT_STRING,
                  G_TYPE_NONE, 2,
                  G_TYPE_UINT,
                  G_TYPE_STRING);
  statusbar_signals[SIGNAL_TEXT_POPPED] =
    g_signal_new ("text_popped",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpStatusbarClass, text_popped),
                  NULL, NULL,
                  gimp_marshal_VOID__UINT_STRING,
                  G_TYPE_NONE, 2,
                  G_TYPE_UINT,
                  G_TYPE_STRING);

  gtk_widget_class_install_style_property (gtk_widget_class,
                                           g_param_spec_enum ("shadow_type",
                                           _("Shadow type"),
                                           _("Style of bevel around the statusbar text"),
                                           GTK_TYPE_SHADOW_TYPE,
                                           GTK_SHADOW_IN,
                                           G_PARAM_READABLE));

  klass->text_pushed = gimp_statusbar_update;
  klass->text_popped = gimp_statusbar_update;
}

static void
gimp_statusbar_init (GimpStatusbar *statusbar)
{
  GtkBox        *box = GTK_BOX (statusbar);
  GtkWidget     *hbox;
  GtkWidget     *frame;
  GimpUnitStore *store;
  GtkShadowType  shadow_type;

  box->spacing = 2;
  box->homogeneous = FALSE;

  statusbar->shell                 = NULL;
  statusbar->cursor_format_str[0]  = '\0';
  statusbar->progressid            = 0;

  gtk_box_set_spacing (box, 1);

  gtk_widget_style_get (GTK_WIDGET (statusbar),
                        "shadow_type", &shadow_type,
                        NULL);

  statusbar->cursor_frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (statusbar->cursor_frame), shadow_type);
  gtk_box_pack_start (box, statusbar->cursor_frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (box, statusbar->cursor_frame, 0);
  gtk_widget_show (statusbar->cursor_frame);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_add (GTK_CONTAINER (statusbar->cursor_frame), hbox);
  gtk_widget_show (hbox);

  statusbar->cursor_label = gtk_label_new ("0, 0");
  gtk_misc_set_alignment (GTK_MISC (statusbar->cursor_label), 0.5, 0.5);
  gtk_container_add (GTK_CONTAINER (hbox), statusbar->cursor_label);
  gtk_widget_show (statusbar->cursor_label);

  store = gimp_unit_store_new (2);
  statusbar->unit_combo = gimp_unit_combo_box_new_with_model (store);
  g_object_unref (store);

  gtk_container_add (GTK_CONTAINER (hbox), statusbar->unit_combo);
  gtk_widget_show (statusbar->unit_combo);

  g_signal_connect (statusbar->unit_combo, "changed",
                    G_CALLBACK (gimp_statusbar_unit_changed),
                    statusbar);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), shadow_type);
  gtk_box_pack_start (box, frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  statusbar->scale_combo = gimp_scale_combo_box_new ();
  gtk_container_add (GTK_CONTAINER (frame), statusbar->scale_combo);
  gtk_widget_show (statusbar->scale_combo);

  g_signal_connect (statusbar->scale_combo, "changed",
                    G_CALLBACK (gimp_statusbar_scale_changed),
                    statusbar);

  statusbar->progressbar = gtk_progress_bar_new ();
  gtk_box_pack_start (box, statusbar->progressbar, TRUE, TRUE, 0);
  gtk_widget_show (statusbar->progressbar);

  GTK_PROGRESS_BAR (statusbar->progressbar)->progress.x_align = 0.0;
  GTK_PROGRESS_BAR (statusbar->progressbar)->progress.y_align = 0.5;

  statusbar->cancelbutton = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_sensitive (statusbar->cancelbutton, FALSE);
  gtk_box_pack_start (box, statusbar->cancelbutton, FALSE, FALSE, 0);
  gtk_widget_show (statusbar->cancelbutton);

  /* Update the statusbar once to work around a canvas size problem:
   *
   *  The first update of the statusbar used to queue a resize which
   *  in term caused the canvas to be resized. That made it shrink by
   *  one pixel in height resulting in the last row not being displayed.
   *  Shrink-wrapping the display used to fix this reliably. With the
   *  next call the resize doesn't seem to happen any longer.
   */

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progressbar),
                             "GIMP");

  statusbar->seq_context_id = 1;
  statusbar->seq_message_id = 1;
  statusbar->messages       = NULL;
  statusbar->keys           = NULL;
}


static void
gimp_statusbar_destroy (GtkObject *object)
{
  GimpStatusbar      *statusbar = GIMP_STATUSBAR (object);
  GimpStatusbarClass *class     = GIMP_STATUSBAR_GET_CLASS (statusbar);
  GSList             *list;

  for (list = statusbar->messages; list; list = list->next)
    {
      GimpStatusbarMsg *msg = list->data;

      g_free (msg->text);
      g_mem_chunk_free (class->messages_mem_chunk, msg);
    }

  g_slist_free (statusbar->messages);
  statusbar->messages = NULL;

  for (list = statusbar->keys; list; list = list->next)
    g_free (list->data);

  g_slist_free (statusbar->keys);
  statusbar->keys = NULL;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_statusbar_update (GimpStatusbar *statusbar,
                       const gchar   *context_id,
                       const gchar   *text)
{
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progressbar),
                             text ? text : "");
}

GtkWidget *
gimp_statusbar_new (GimpDisplayShell *shell)
{
  GimpStatusbar *statusbar;

  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  statusbar = g_object_new (GIMP_TYPE_STATUSBAR, NULL);

  statusbar->shell = shell;

  g_signal_connect_object (shell, "scaled",
                           G_CALLBACK (gimp_statusbar_shell_scaled),
                           statusbar, 0);

  return GTK_WIDGET (statusbar);
}

guint
gimp_statusbar_push (GimpStatusbar *statusbar,
                     const gchar   *context,
                     const gchar   *message)
{
  GimpStatusbarMsg   *msg;
  GimpStatusbarClass *class;
  guint               context_id;

  g_return_val_if_fail (GIMP_IS_STATUSBAR (statusbar), 0);
  g_return_val_if_fail (message != NULL, 0);

  class = GIMP_STATUSBAR_GET_CLASS (statusbar);
  context_id = gimp_statusbar_get_context_id (statusbar, context);

  msg = g_chunk_new (GimpStatusbarMsg, class->messages_mem_chunk);

  msg->text       = g_strdup (message);
  msg->context_id = context_id;
  msg->message_id = statusbar->seq_message_id++;

  statusbar->messages = g_slist_prepend (statusbar->messages, msg);

  g_signal_emit (statusbar,
                 statusbar_signals[SIGNAL_TEXT_PUSHED],
                 0,
                 msg->context_id,
                 msg->text);

  return msg->message_id;
}

void
gimp_statusbar_pop (GimpStatusbar *statusbar,
                    const gchar   *context)
{
  GimpStatusbarClass *class;
  GimpStatusbarMsg   *msg;
  guint               context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  class = GIMP_STATUSBAR_GET_CLASS (statusbar);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  if (statusbar->messages)
    {
      GSList *list;

      for (list = statusbar->messages; list; list = list->next)
        {
           msg = list->data;

           if (msg->context_id == context_id)
             {
               statusbar->messages = g_slist_remove_link (statusbar->messages,
                                                          list);
               g_free (msg->text);
               g_mem_chunk_free (class->messages_mem_chunk, msg);
               g_slist_free_1 (list);
               break;
             }
        }
    }

  msg = statusbar->messages ? statusbar->messages->data : NULL;

  g_signal_emit (statusbar,
                 statusbar_signals[SIGNAL_TEXT_POPPED],
                 0,
                 (guint) (msg ? msg->context_id : 0),
                 msg ? msg->text : NULL);
}


void
gimp_statusbar_remove (GimpStatusbar *statusbar,
                       const gchar   *context,
                       guint          message_id)
{
  GimpStatusbarClass *class;
  GimpStatusbarMsg   *msg;
  guint               context_id;

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));
  g_return_if_fail (message_id > 0);

  class = GIMP_STATUSBAR_GET_CLASS (statusbar);

  context_id = gimp_statusbar_get_context_id (statusbar, context);

  msg = statusbar->messages ? statusbar->messages->data : NULL;
  if (msg)
    {
      GSList *list;

      /* care about signal emission if the topmost item is removed */
      if (msg->context_id == context_id &&
          msg->message_id == message_id)
        {
           gimp_statusbar_pop (statusbar, context);
           return;
        }

      for (list = statusbar->messages; list; list = list->next)
        {
           msg = list->data;

           if (msg->context_id == context_id &&
               msg->message_id == message_id)
             {
                statusbar->messages = g_slist_remove_link (statusbar->messages,
                                                           list);
                g_free (msg->text);
                g_mem_chunk_free (class->messages_mem_chunk, msg);
                g_slist_free_1 (list);
                break;
             }
        }
    }
}


guint
gimp_statusbar_push_coords (GimpStatusbar *statusbar,
                            const gchar   *context,
                            const gchar   *title,
                            gdouble        x,
                            const gchar   *separator,
                            gdouble        y)
{
  GimpDisplayShell *shell;
  gchar             buf[CURSOR_LEN];

  g_return_val_if_fail  (GIMP_IS_STATUSBAR (statusbar), 0);
  g_return_val_if_fail (title != NULL, 0);
  g_return_val_if_fail (separator != NULL, 0);

  shell = statusbar->shell;

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (buf, sizeof (buf), statusbar->cursor_format_str,
               title,
                  ROUND (x),
                  separator,
                  ROUND (y));
    }
  else /* show real world units */
    {
      GimpImage *image       = shell->gdisp->gimage;
      gdouble    unit_factor = _gimp_unit_get_factor (image->gimp,
                                                      shell->unit);

      g_snprintf (buf, sizeof (buf), statusbar->cursor_format_str,
                  title,
                  x * unit_factor / image->xresolution,
                  separator,
                  y * unit_factor / image->yresolution);
    }

  return gimp_statusbar_push (statusbar, context, buf);
}


void
gimp_statusbar_set_cursor (GimpStatusbar *statusbar,
                           gdouble        x,
                           gdouble        y)
{
  GimpDisplayShell *shell;
  GtkTreeModel     *model;
  GimpUnitStore    *store;
  gchar             buffer[CURSOR_LEN];

  g_return_if_fail (GIMP_IS_STATUSBAR (statusbar));

  shell = statusbar->shell;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (statusbar->unit_combo));
  store = GIMP_UNIT_STORE (model);

  gimp_unit_store_set_pixel_values (store, x, y);

  if (shell->unit == GIMP_UNIT_PIXEL)
    {
      g_snprintf (buffer, sizeof (buffer),
                  statusbar->cursor_format_str,
		  "", ROUND (x), ", ", ROUND (y));
    }
  else /* show real world units */
    {
      gimp_unit_store_get_values (store, shell->unit, &x, &y);

      g_snprintf (buffer, sizeof (buffer),
                  statusbar->cursor_format_str,
		  "", x, ", ", y);
    }

  gtk_label_set_text (GTK_LABEL (statusbar->cursor_label), buffer);
}

void
gimp_statusbar_clear_cursor (GimpStatusbar *statusbar)
{
  gtk_label_set_text (GTK_LABEL (statusbar->cursor_label), "");
}

static void
gimp_statusbar_shell_scaled (GimpDisplayShell *shell,
                             GimpStatusbar    *statusbar)
{
  static PangoLayout *layout = NULL;

  GimpImage    *image = shell->gdisp->gimage;
  GtkTreeModel *model;
  const gchar  *text;
  gint          width;
  gint          diff;

  g_signal_handlers_block_by_func (statusbar->scale_combo,
                                   gimp_statusbar_scale_changed, statusbar);
  gimp_scale_combo_box_set_scale (GIMP_SCALE_COMBO_BOX (statusbar->scale_combo),
                                  shell->scale);
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
		  "%%s%%d%%s%%d");
    }
  else /* show real world units */
    {
      g_snprintf (statusbar->cursor_format_str,
                  sizeof (statusbar->cursor_format_str),
		  "%%s%%.%df%%s%%.%df",
                  _gimp_unit_get_digits (image->gimp, shell->unit),
                  _gimp_unit_get_digits (image->gimp, shell->unit));
    }

  gimp_statusbar_set_cursor (statusbar, - image->width, - image->height);

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

guint
gimp_statusbar_get_context_id (GimpStatusbar *statusbar,
                               const gchar   *context_description)
{
  gchar *string;
  guint *id;

  g_return_val_if_fail (GIMP_IS_STATUSBAR (statusbar), 0);
  g_return_val_if_fail (context_description != NULL, 0);

  /* we need to preserve namespaces on object datas */
  string = g_strconcat ("gimp-status-bar-context:", context_description, NULL);

  id = g_object_get_data (G_OBJECT (statusbar), string);
  if (!id)
    {
      id = g_new (guint, 1);
      *id = statusbar->seq_context_id++;
      g_object_set_data_full (G_OBJECT (statusbar), string, id, g_free);
      statusbar->keys = g_slist_prepend (statusbar->keys, string);
    }
  else
    {
      g_free (string);
    }

  return *id;
}
