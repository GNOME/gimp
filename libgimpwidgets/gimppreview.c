/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppreview.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpwidgets.h"

#include "gimppreview.h"

#include "libgimp/libgimp-intl.h"


#define PREVIEW_SIZE (128)


enum
{
  UPDATED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_UPDATE_PREVIEW,
  PROP_SHOW_TOGGLE_PREVIEW
};

static void     gimp_preview_class_init         (GimpPreviewClass *klass);
static void     gimp_preview_init               (GimpPreview      *preview);
static void     gimp_preview_get_property       (GObject         *object,
                                                 guint            param_id,
                                                 GValue          *value,
                                                 GParamSpec      *pspec);
static void     gimp_preview_set_property       (GObject         *object,
                                                 guint            param_id,
                                                 const GValue    *value,
                                                 GParamSpec      *pspec);

static void     gimp_preview_emit_updated       (GimpPreview      *preview);
static gboolean gimp_preview_button_release     (GtkWidget        *hs,
                                                 GdkEventButton   *ev,
                                                 GimpPreview      *preview);
static void     gimp_preview_update             (GimpPreview      *preview);
static void     gimp_preview_area_size_allocate (GimpPreview      *preview);
static void     gimp_preview_h_scroll           (GtkAdjustment    *hadj,
                                                 GimpPreview      *preview);
static void     gimp_preview_v_scroll           (GtkAdjustment    *vadj,
                                                 GimpPreview      *preview);
static gboolean gimp_preview_area_event         (GtkWidget        *area,
                                                 GdkEvent         *event,
                                                 GimpPreview      *preview);
static void     gimp_preview_toggle_callback    (GtkWidget        *toggle,
                                                 GimpPreview      *preview);

static guint preview_signals[LAST_SIGNAL] = { 0 };

static GtkTableClass *parent_class = NULL;


GType
gimp_preview_get_type (void)
{
  static GType preview_type = 0;

  if (!preview_type)
    {
      static const GTypeInfo preview_info =
      {
        sizeof (GimpPreviewClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_preview_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpPreview),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_preview_init,
      };

      preview_type = g_type_register_static (GTK_TYPE_TABLE,
                                             "GimpPreview",
                                             &preview_info,
                                             G_TYPE_FLAG_ABSTRACT);
    }

  return preview_type;
}

static void
gimp_preview_class_init (GimpPreviewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[UPDATED] =
    g_signal_new ("updated",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPreviewClass, updated),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  klass->update = NULL;

  object_class->get_property = gimp_preview_get_property;
  object_class->set_property = gimp_preview_set_property;

  g_object_class_install_property (object_class,
                                   PROP_UPDATE_PREVIEW,
                                   g_param_spec_boolean ("update_preview",
                                                         NULL, NULL,
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class,
                                   PROP_SHOW_TOGGLE_PREVIEW,
                                   g_param_spec_boolean ("show_toggle_preview",
                                                         NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

}

static void
gimp_preview_init (GimpPreview *preview)
{
  GtkWidget *frame;
  gint       sel_width;
  gint       sel_height;

  preview->xoff           = 0;
  preview->yoff           = 0;
  preview->in_drag        = FALSE;
  preview->update_preview = TRUE;

  preview->xmin = preview->ymin = 0;
  preview->xmax = preview->ymax = 1;

  sel_width       = preview->xmax - preview->xmin;
  sel_height      = preview->ymax - preview->ymin;
  preview->width  = MIN (sel_width, PREVIEW_SIZE);
  preview->height = MIN (sel_height, PREVIEW_SIZE);

  gtk_table_resize (GTK_TABLE (preview), 3, 2);
  gtk_table_set_homogeneous (GTK_TABLE (preview), FALSE);

  preview->hadj = gtk_adjustment_new (0, 0, sel_width - 1, 1.0,
                                      MIN (preview->width, sel_width),
                                      MIN (preview->width, sel_width));

  g_signal_connect (preview->hadj, "value_changed",
                    G_CALLBACK (gimp_preview_h_scroll),
                    preview);

  preview->hscr = gtk_hscrollbar_new (GTK_ADJUSTMENT (preview->hadj));
  gtk_range_set_update_policy (GTK_RANGE (preview->hscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (preview), preview->hscr, 0,1, 1,2,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (preview->hscr);

  g_signal_connect (preview->hscr, "button-release-event",
                    G_CALLBACK (gimp_preview_button_release), preview);

  preview->vadj = gtk_adjustment_new (0, 0, sel_height - 1, 1.0,
                                      MIN (preview->height, sel_height),
                                      MIN (preview->height, sel_height));

  g_signal_connect (preview->vadj, "value_changed",
                    G_CALLBACK (gimp_preview_v_scroll),
                    preview);

  preview->vscr = gtk_vscrollbar_new (GTK_ADJUSTMENT (preview->vadj));
  gtk_range_set_update_policy (GTK_RANGE (preview->vscr),
                               GTK_UPDATE_CONTINUOUS);
  gtk_table_attach (GTK_TABLE (preview), preview->vscr, 1,2, 0,1,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (preview->vscr);

  g_signal_connect (preview->vscr, "button-release-event",
                    G_CALLBACK (gimp_preview_button_release), preview);

  /* the area itself */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_table_attach (GTK_TABLE (preview), frame,
                    0,1, 0,1,
                    GTK_FILL, GTK_FILL, 0,0);
  gtk_widget_show (frame);

  preview->area = gimp_preview_area_new ();
  gtk_widget_set_size_request (preview->area,
                               preview->width, preview->height);
  gtk_container_add (GTK_CONTAINER (frame), preview->area);
  gtk_widget_show (preview->area);

  gtk_widget_set_events (preview->area,
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON_MOTION_MASK);

  g_signal_connect (preview->area, "event",
                    G_CALLBACK (gimp_preview_area_event), preview);
  g_signal_connect_swapped (preview->area, "size_allocate",
                            G_CALLBACK (gimp_preview_area_size_allocate),
                            preview);

  /* a toggle button to (des)activate the instant preview */

  preview->toggle_update = gtk_check_button_new_with_mnemonic (_("_Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preview->toggle_update),
                                preview->update_preview);
  gtk_table_attach (GTK_TABLE (preview), preview->toggle_update,
                    0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
  g_signal_connect_after (preview->toggle_update, "toggled",
                          G_CALLBACK (gimp_preview_toggle_callback),
                          preview);
}

static void
gimp_preview_get_property (GObject    *object,
                           guint       param_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpPreview *preview = GIMP_PREVIEW (object);

  switch (param_id)
    {
    case PROP_UPDATE_PREVIEW:
      g_value_set_boolean (value, preview->update_preview);
      break;

    case PROP_SHOW_TOGGLE_PREVIEW:
      g_value_set_boolean (value, GTK_WIDGET_VISIBLE (preview->toggle_update));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_preview_set_property (GObject      *object,
                           guint         param_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpPreview *preview = GIMP_PREVIEW (object);

  switch (param_id)
    {
    case PROP_UPDATE_PREVIEW:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (preview->toggle_update),
                                    g_value_get_boolean (value));
      break;

    case PROP_SHOW_TOGGLE_PREVIEW:
      if (g_value_get_boolean (value))
        gtk_widget_show (preview->toggle_update);
      else
        gtk_widget_hide (preview->toggle_update);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
gimp_preview_emit_updated (GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  if (preview->update_preview)
    g_signal_emit (preview, preview_signals[UPDATED], 0);
}

static gboolean
gimp_preview_button_release (GtkWidget      *hs,
                             GdkEventButton *ev,
                             GimpPreview    *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), FALSE);

  gimp_preview_emit_updated (preview);

  return FALSE;
}

static void
gimp_preview_update (GimpPreview *preview)
{
  GimpPreviewClass *class;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  class = GIMP_PREVIEW_GET_CLASS (preview);

  if (class->update)
    class->update (preview);
}

static void
gimp_preview_area_size_allocate (GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  gimp_preview_update (preview);
  gimp_preview_emit_updated (preview);
}

static void
gimp_preview_h_scroll (GtkAdjustment *hadj, GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  preview->xoff = hadj->value;

  if (!preview->in_drag)
    gimp_preview_update (preview);
}

static void
gimp_preview_v_scroll (GtkAdjustment *vadj, GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  preview->yoff = vadj->value;

  if (!preview->in_drag)
    gimp_preview_update (preview);
}

static gboolean
gimp_preview_area_event (GtkWidget   *area,
                         GdkEvent    *event,
                         GimpPreview *preview)
{
  GdkEventButton *button_event;
  gint x, y;
  gint dx, dy;

  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), FALSE);

  button_event = (GdkEventButton *) event;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (button_event->button == 2)
        {
          gtk_widget_get_pointer (area, &x, &y);

          preview->in_drag = TRUE;
          preview->drag_x = x;
          preview->drag_y = y;
          preview->drag_xoff = preview->xoff;
          preview->drag_yoff = preview->yoff;
          gtk_grab_add (area);
        }
      break;

    case GDK_BUTTON_RELEASE:
      if (preview->in_drag && button_event->button == 2)
        {
          gtk_grab_remove (area);
          preview->in_drag = FALSE;
          gimp_preview_emit_updated (preview);
        }
      break;

    case GDK_MOTION_NOTIFY:
      if (preview->in_drag)
        {
          gint xoff, yoff;
          gtk_widget_get_pointer (area, &x, &y);

          dx = x - preview->drag_x;
          dy = y - preview->drag_y;

          xoff = CLAMP (preview->drag_xoff - dx,
                        0, preview->xmax - preview->xmin - preview->width);
          yoff = CLAMP (preview->drag_yoff - dy,
                        0, preview->ymax - preview->ymin - preview->height);
          gtk_adjustment_set_value (GTK_ADJUSTMENT (preview->hadj), xoff);
          gtk_adjustment_set_value (GTK_ADJUSTMENT (preview->vadj), yoff);
          gimp_preview_update (preview);
        }
      break;

    default:
      break;
    }

  return FALSE;
}

static void
gimp_preview_toggle_callback (GtkWidget   *toggle,
                              GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    {
      preview->update_preview = TRUE;
      gimp_preview_emit_updated (preview);
    }
  else
    {
      preview->update_preview = FALSE;
    }

  g_object_notify (G_OBJECT (toggle), "update_preview");
}

/**
 * gimp_preview_get_width:
 * @preview: a #GimpPreview widget
 *
 * Return value: the @preview's width
 *
 * Since: GIMP 2.2
 **/
gint
gimp_preview_get_width (GimpPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), 0);

  return preview->width;
}


/**
 * gimp_preview_get_height:
 * @preview: a #GimpPreview widget
 *
 * Return value: the @preview's height
 *
 * Since: GIMP 2.2
 **/
gint
gimp_preview_get_height (GimpPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), 0);

  return preview->height;
}

/**
 * gimp_preview_get_posistion:
 * @preview: a #GimpPreview widget
 * @x:       return location for horizontal offset
 * @y:       return location for vertical offset
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_get_position (GimpPreview  *preview,
                           gint         *x,
                           gint         *y)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  if (x)
    *x = preview->xoff + preview->xmin;

  if (y)
    *y = preview->yoff + preview->ymin;
}

/**
 * gimp_preview_show_update_toggle:
 * @preview:     a #GimpPreview widget
 * @show_update: whether to show the "Update Preview" toggle button
 *
 * Since: GIMP 2.2
 **/
void
gimp_preview_show_update_toggle (GimpPreview *preview,
                                 gboolean     show_update)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  g_object_set (preview,
                "show_toggle_preview", show_update,
                NULL);
}

