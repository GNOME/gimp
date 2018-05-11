/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppreview.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgets.h"

#include "gimppreview.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimppreview
 * @title: GimpPreview
 * @short_description: A widget providing a #GimpPreviewArea plus
 *                     framework to update the preview.
 *
 * A widget providing a #GimpPreviewArea plus framework to update the
 * preview.
 **/


#define DEFAULT_SIZE     200
#define PREVIEW_TIMEOUT  200


enum
{
  INVALIDATED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_UPDATE
};


struct _GimpPreviewPrivate
{
  GtkWidget *area;
  GtkWidget *grid;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *controls;
  GdkCursor *cursor_busy;
  GdkCursor *default_cursor;

  gint       xoff, yoff;
  gint       xmin, xmax, ymin, ymax;
  gint       width, height;

  gboolean   update_preview;
  guint      timeout_id;
};

#define GET_PRIVATE(obj) (((GimpPreview *) (obj))->priv)


static void      gimp_preview_class_init          (GimpPreviewClass *klass);
static void      gimp_preview_init                (GimpPreview      *preview);
static void      gimp_preview_dispose             (GObject          *object);
static void      gimp_preview_get_property        (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);
static void      gimp_preview_set_property        (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);

static void      gimp_preview_direction_changed   (GtkWidget        *widget,
                                                   GtkTextDirection  prev_dir);
static gboolean  gimp_preview_popup_menu          (GtkWidget        *widget);

static void      gimp_preview_area_realize        (GtkWidget        *widget,
                                                   GimpPreview      *preview);
static void      gimp_preview_area_unrealize      (GtkWidget        *widget,
                                                   GimpPreview      *preview);
static void      gimp_preview_area_size_allocate  (GtkWidget        *widget,
                                                   GtkAllocation    *allocation,
                                                   GimpPreview      *preview);
static void      gimp_preview_area_set_cursor     (GimpPreview      *preview);
static gboolean  gimp_preview_area_event          (GtkWidget        *area,
                                                   GdkEvent         *event,
                                                   GimpPreview      *preview);

static void      gimp_preview_toggle_callback     (GtkWidget        *toggle,
                                                   GimpPreview      *preview);

static void      gimp_preview_notify_checks       (GimpPreview      *preview);

static gboolean  gimp_preview_invalidate_now      (GimpPreview      *preview);
static void      gimp_preview_real_set_cursor     (GimpPreview      *preview);
static void      gimp_preview_real_transform      (GimpPreview      *preview,
                                                   gint              src_x,
                                                   gint              src_y,
                                                   gint             *dest_x,
                                                   gint             *dest_y);
static void      gimp_preview_real_untransform    (GimpPreview      *preview,
                                                   gint              src_x,
                                                   gint              src_y,
                                                   gint             *dest_x,
                                                   gint             *dest_y);


/* FIXME G_DEFINE_TYPE */

static guint preview_signals[LAST_SIGNAL] = { 0 };

static GtkBoxClass *parent_class = NULL;


GType
gimp_preview_get_type (void)
{
  static GType preview_type = 0;

  if (! preview_type)
    {
      const GTypeInfo preview_info =
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

      preview_type = g_type_register_static (GTK_TYPE_BOX,
                                             "GimpPreview",
                                             &preview_info,
                                             G_TYPE_FLAG_ABSTRACT);
    }

  return preview_type;
}

static void
gimp_preview_class_init (GimpPreviewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  preview_signals[INVALIDATED] =
    g_signal_new ("invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpPreviewClass, invalidated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose           = gimp_preview_dispose;
  object_class->get_property      = gimp_preview_get_property;
  object_class->set_property      = gimp_preview_set_property;

  widget_class->direction_changed = gimp_preview_direction_changed;
  widget_class->popup_menu        = gimp_preview_popup_menu;

  klass->draw                     = NULL;
  klass->draw_thumb               = NULL;
  klass->draw_buffer              = NULL;
  klass->set_cursor               = gimp_preview_real_set_cursor;
  klass->transform                = gimp_preview_real_transform;
  klass->untransform              = gimp_preview_real_untransform;

  g_type_class_add_private (object_class, sizeof (GimpPreviewPrivate));

  g_object_class_install_property (object_class,
                                   PROP_UPDATE,
                                   g_param_spec_boolean ("update",
                                                         "Update",
                                                         "Whether the preview should update automatically",
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("size",
                                                             "Size",
                                                             "The preview's size",
                                                             1, 1024,
                                                             DEFAULT_SIZE,
                                                             GIMP_PARAM_READABLE));
}

static void
gimp_preview_init (GimpPreview *preview)
{
  GimpPreviewPrivate *priv;
  GtkWidget          *frame;
  gdouble             xalign = 0.0;

  preview->priv = G_TYPE_INSTANCE_GET_PRIVATE (preview,
                                               GIMP_TYPE_PREVIEW,
                                               GimpPreviewPrivate);

  priv = preview->priv;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (preview),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_homogeneous (GTK_BOX (preview), FALSE);
  gtk_box_set_spacing (GTK_BOX (preview), 6);

  if (gtk_widget_get_direction (GTK_WIDGET (preview)) == GTK_TEXT_DIR_RTL)
    xalign = 1.0;

  priv->frame = gtk_aspect_frame_new (NULL, xalign, 0.0, 1.0, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (priv->frame), GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (preview), priv->frame, TRUE, TRUE, 0);
  gtk_widget_show (priv->frame);

  priv->grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (priv->frame), priv->grid);
  gtk_widget_show (priv->grid);

  priv->timeout_id = 0;

  priv->xmin   = priv->ymin = 0;
  priv->xmax   = priv->ymax = 1;
  priv->width  = priv->xmax - priv->xmin;
  priv->height = priv->ymax - priv->ymin;

  priv->xoff   = 0;
  priv->yoff   = 0;

  priv->default_cursor = NULL;

  /*  preview area  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (frame, TRUE);
  gtk_widget_set_vexpand (frame, TRUE);
  gtk_grid_attach (GTK_GRID (priv->grid), frame, 0, 0, 1, 1);
  gtk_widget_show (frame);

  priv->area = gimp_preview_area_new ();
  gtk_container_add (GTK_CONTAINER (frame), priv->area);
  gtk_widget_show (priv->area);

  g_signal_connect_swapped (priv->area, "notify::check-size",
                            G_CALLBACK (gimp_preview_notify_checks),
                            preview);
  g_signal_connect_swapped (priv->area, "notify::check-type",
                            G_CALLBACK (gimp_preview_notify_checks),
                            preview);

  gtk_widget_add_events (priv->area,
                         GDK_BUTTON_PRESS_MASK        |
                         GDK_BUTTON_RELEASE_MASK      |
                         GDK_SCROLL_MASK              |
                         GDK_SMOOTH_SCROLL_MASK       |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON_MOTION_MASK);

  g_signal_connect (priv->area, "event",
                    G_CALLBACK (gimp_preview_area_event),
                    preview);

  g_signal_connect (priv->area, "realize",
                    G_CALLBACK (gimp_preview_area_realize),
                    preview);
  g_signal_connect (priv->area, "unrealize",
                    G_CALLBACK (gimp_preview_area_unrealize),
                    preview);

  g_signal_connect_data (priv->area, "realize",
                         G_CALLBACK (gimp_preview_area_set_cursor),
                         preview, NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  g_signal_connect (priv->area, "size-allocate",
                    G_CALLBACK (gimp_preview_area_size_allocate),
                    preview);

  g_signal_connect_data (priv->area, "size-allocate",
                         G_CALLBACK (gimp_preview_area_set_cursor),
                         preview, NULL, G_CONNECT_AFTER | G_CONNECT_SWAPPED);

  priv->controls = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_margin_top (priv->controls, 3);
  gtk_grid_attach (GTK_GRID (priv->grid), priv->controls, 0, 2, 2, 1);
  gtk_widget_show (priv->controls);

  /*  toggle button to (de)activate the instant preview  */
  priv->toggle = gtk_check_button_new_with_mnemonic (_("_Preview"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->toggle),
                                priv->update_preview);
  gtk_box_pack_start (GTK_BOX (priv->controls), priv->toggle, TRUE, TRUE, 0);
  gtk_widget_show (priv->toggle);

  g_signal_connect (priv->toggle, "toggled",
                    G_CALLBACK (gimp_preview_toggle_callback),
                    preview);
}

static void
gimp_preview_dispose (GObject *object)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (object);

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_preview_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_UPDATE:
      g_value_set_boolean (value, priv->update_preview);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_preview_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_UPDATE:
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->toggle),
                                    g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_preview_direction_changed (GtkWidget        *widget,
                                GtkTextDirection  prev_dir)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (widget);
  gdouble             xalign  = 0.0;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    xalign = 1.0;

  gtk_aspect_frame_set (GTK_ASPECT_FRAME (priv->frame),
                        xalign, 0.0, 1.0, TRUE);
}

static gboolean
gimp_preview_popup_menu (GtkWidget *widget)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (widget);

  gimp_preview_area_menu_popup (GIMP_PREVIEW_AREA (priv->area), NULL);

  return TRUE;
}

static void
gimp_preview_area_realize (GtkWidget   *widget,
                           GimpPreview *preview)
{
  GimpPreviewPrivate *priv    = GET_PRIVATE (preview);
  GdkDisplay         *display = gtk_widget_get_display (widget);

  g_return_if_fail (priv->cursor_busy == NULL);

  priv->cursor_busy = gdk_cursor_new_for_display (display, GDK_WATCH);

}

static void
gimp_preview_area_unrealize (GtkWidget   *widget,
                             GimpPreview *preview)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (preview);

  g_clear_object (&priv->cursor_busy);
}

static void
gimp_preview_area_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation,
                                 GimpPreview   *preview)
{
  GimpPreviewPrivate *priv   = GET_PRIVATE (preview);
  gint                width  = priv->xmax - priv->xmin;
  gint                height = priv->ymax - priv->ymin;

  priv->width  = MIN (width,  allocation->width);
  priv->height = MIN (height, allocation->height);

  gimp_preview_draw (preview);
  gimp_preview_invalidate (preview);
}

static void
gimp_preview_area_set_cursor (GimpPreview *preview)
{
  GIMP_PREVIEW_GET_CLASS (preview)->set_cursor (preview);
}

static gboolean
gimp_preview_area_event (GtkWidget   *area,
                         GdkEvent    *event,
                         GimpPreview *preview)
{
  GdkEventButton *button_event = (GdkEventButton *) event;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      switch (button_event->button)
        {
        case 3:
          gimp_preview_area_menu_popup (GIMP_PREVIEW_AREA (area), button_event);
          return TRUE;
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
  GimpPreviewPrivate *priv = GET_PRIVATE (preview);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle)))
    {
      priv->update_preview = TRUE;

      g_object_notify (G_OBJECT (preview), "update");

      if (priv->timeout_id)
        g_source_remove (priv->timeout_id);

      gimp_preview_invalidate_now (preview);
    }
  else
    {
      priv->update_preview = FALSE;

      g_object_notify (G_OBJECT (preview), "update");

      gimp_preview_draw (preview);
    }
}

static void
gimp_preview_notify_checks (GimpPreview *preview)
{
  gimp_preview_draw (preview);
  gimp_preview_invalidate (preview);
}

static gboolean
gimp_preview_invalidate_now (GimpPreview *preview)
{
  GimpPreviewPrivate *priv     = GET_PRIVATE (preview);
  GtkWidget          *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (preview));
  GimpPreviewClass   *class    = GIMP_PREVIEW_GET_CLASS (preview);

  gimp_preview_draw (preview);

  priv->timeout_id = 0;

  if (toplevel && gtk_widget_get_realized (toplevel))
    {
      gdk_window_set_cursor (gtk_widget_get_window (toplevel),
                             priv->cursor_busy);
      gdk_window_set_cursor (gtk_widget_get_window (priv->area),
                             priv->cursor_busy);

      gdk_display_flush (gtk_widget_get_display (toplevel));

      g_signal_emit (preview, preview_signals[INVALIDATED], 0);

      class->set_cursor (preview);
      gdk_window_set_cursor (gtk_widget_get_window (toplevel), NULL);
    }
  else
    {
      g_signal_emit (preview, preview_signals[INVALIDATED], 0);
    }

  return FALSE;
}

static void
gimp_preview_real_set_cursor (GimpPreview *preview)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (preview);

  if (gtk_widget_get_realized (priv->area))
    gdk_window_set_cursor (gtk_widget_get_window (priv->area),
                           priv->default_cursor);
}

static void
gimp_preview_real_transform (GimpPreview *preview,
                             gint         src_x,
                             gint         src_y,
                             gint        *dest_x,
                             gint        *dest_y)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (preview);

  *dest_x = src_x - priv->xoff - priv->xmin;
  *dest_y = src_y - priv->yoff - priv->ymin;
}

static void
gimp_preview_real_untransform (GimpPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  GimpPreviewPrivate *priv = GET_PRIVATE (preview);

  *dest_x = src_x + priv->xoff + priv->xmin;
  *dest_y = src_y + priv->yoff + priv->ymin;
}

/**
 * gimp_preview_set_update:
 * @preview: a #GimpPreview widget
 * @update: %TRUE if the preview should invalidate itself when being
 *          scrolled or when gimp_preview_invalidate() is being called
 *
 * Sets the state of the "Preview" check button.
 *
 * Since: 2.2
 **/
void
gimp_preview_set_update (GimpPreview *preview,
                         gboolean     update)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  g_object_set (preview,
                "update", update,
                NULL);
}

/**
 * gimp_preview_get_update:
 * @preview: a #GimpPreview widget
 *
 * Return value: the state of the "Preview" check button.
 *
 * Since: 2.2
 **/
gboolean
gimp_preview_get_update (GimpPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), FALSE);

  return GET_PRIVATE (preview)->update_preview;
}

/**
 * gimp_preview_set_bounds:
 * @preview: a #GimpPreview widget
 * @xmin:    the minimum X value
 * @ymin:    the minimum Y value
 * @xmax:    the maximum X value
 * @ymax:    the maximum Y value
 *
 * Sets the lower and upper limits for the previewed area. The
 * difference between the upper and lower value is used to set the
 * maximum size of the #GimpPreviewArea used in the @preview.
 *
 * Since: 2.2
 **/
void
gimp_preview_set_bounds (GimpPreview *preview,
                         gint         xmin,
                         gint         ymin,
                         gint         xmax,
                         gint         ymax)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (xmax > xmin);
  g_return_if_fail (ymax > ymin);

  priv = GET_PRIVATE (preview);

  priv->xmin = xmin;
  priv->ymin = ymin;
  priv->xmax = xmax;
  priv->ymax = ymax;

  gimp_preview_area_set_max_size (GIMP_PREVIEW_AREA (priv->area),
                                  xmax - xmin,
                                  ymax - ymin);
}

void
gimp_preview_get_bounds (GimpPreview *preview,
                         gint        *xmin,
                         gint        *ymin,
                         gint        *xmax,
                         gint        *ymax)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  priv = GET_PRIVATE (preview);

  if (xmin) *xmin = priv->xmin;
  if (ymin) *ymin = priv->ymin;
  if (xmax) *xmax = priv->xmax;
  if (ymax) *ymax = priv->ymax;
}

void
gimp_preview_set_size (GimpPreview *preview,
                       gint         width,
                       gint         height)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  priv = GET_PRIVATE (preview);

  priv->width  = width;
  priv->height = height;

  gtk_widget_set_size_request (priv->area, width, height);
}

/**
 * gimp_preview_get_size:
 * @preview: a #GimpPreview widget
 * @width:   return location for the preview area width
 * @height:  return location for the preview area height
 *
 * Since: 2.2
 **/
void
gimp_preview_get_size (GimpPreview *preview,
                       gint        *width,
                       gint        *height)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  priv = GET_PRIVATE (preview);

  if (width)  *width  = priv->width;
  if (height) *height = priv->height;
}

void
gimp_preview_set_offsets (GimpPreview *preview,
                          gint         xoff,
                          gint         yoff)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  priv = GET_PRIVATE (preview);

  priv->xoff = xoff;
  priv->yoff = yoff;
}

void
gimp_preview_get_offsets (GimpPreview *preview,
                          gint        *xoff,
                          gint        *yoff)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  priv = GET_PRIVATE (preview);

  if (xoff) *xoff = priv->xoff;
  if (yoff) *yoff = priv->yoff;
}

/**
 * gimp_preview_get_position:
 * @preview: a #GimpPreview widget
 * @x:       return location for the horizontal offset
 * @y:       return location for the vertical offset
 *
 * Since: 2.2
 **/
void
gimp_preview_get_position (GimpPreview *preview,
                           gint        *x,
                           gint        *y)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  priv = GET_PRIVATE (preview);

  if (x) *x = priv->xoff + priv->xmin;
  if (y) *y = priv->yoff + priv->ymin;
}

/**
 * gimp_preview_transform:
 * @preview: a #GimpPreview widget
 * @src_x:   horizontal position on the previewed image
 * @src_y:   vertical position on the previewed image
 * @dest_x:  returns the transformed horizontal position
 * @dest_y:  returns the transformed vertical position
 *
 * Transforms from image to widget coordinates.
 *
 * Since: 2.4
 **/
void
gimp_preview_transform (GimpPreview *preview,
                        gint         src_x,
                        gint         src_y,
                        gint        *dest_x,
                        gint        *dest_y)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (dest_x != NULL && dest_y != NULL);

  GIMP_PREVIEW_GET_CLASS (preview)->transform (preview,
                                               src_x, src_y, dest_x, dest_y);
}

/**
 * gimp_preview_untransform:
 * @preview: a #GimpPreview widget
 * @src_x:   horizontal position relative to the preview area's origin
 * @src_y:   vertical position relative to  preview area's origin
 * @dest_x:  returns the untransformed horizontal position
 * @dest_y:  returns the untransformed vertical position
 *
 * Transforms from widget to image coordinates.
 *
 * Since: 2.4
 **/
void
gimp_preview_untransform (GimpPreview *preview,
                          gint         src_x,
                          gint         src_y,
                          gint        *dest_x,
                          gint        *dest_y)
{
  g_return_if_fail (GIMP_IS_PREVIEW (preview));
  g_return_if_fail (dest_x != NULL && dest_y != NULL);

  GIMP_PREVIEW_GET_CLASS (preview)->untransform (preview,
                                                 src_x, src_y, dest_x, dest_y);
}

/**
 * gimp_preview_get_frame:
 * @preview: a #GimpPreview widget
 *
 * Return value: a pointer to the #GtkAspectFrame used in the @preview.
 *
 * Since: 3.0
 **/
GtkWidget *
gimp_preview_get_frame (GimpPreview  *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->frame;
}

/**
 * gimp_preview_get_grid:
 * @preview: a #GimpPreview widget
 *
 * Return value: a pointer to the #GtkGrid used in the @preview.
 *
 * Since: 3.0
 **/
GtkWidget *
gimp_preview_get_grid (GimpPreview  *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->grid;
}

/**
 * gimp_preview_get_area:
 * @preview: a #GimpPreview widget
 *
 * In most cases, you shouldn't need to access the #GimpPreviewArea
 * that is being used in the @preview. Sometimes however, you need to.
 * For example if you want to receive mouse events from the area. In
 * such cases, use gimp_preview_get_area().
 *
 * Return value: a pointer to the #GimpPreviewArea used in the @preview.
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_preview_get_area (GimpPreview  *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->area;
}

/**
 * gimp_preview_draw:
 * @preview: a #GimpPreview widget
 *
 * Calls the GimpPreview::draw method. GimpPreview itself doesn't
 * implement a default draw method so the behaviour is determined by
 * the derived class implementing this method.
 *
 * #GimpDrawablePreview implements gimp_preview_draw() by drawing the
 * original, unmodified drawable to the @preview.
 *
 * Since: 2.2
 **/
void
gimp_preview_draw (GimpPreview *preview)
{
  GimpPreviewClass *class = GIMP_PREVIEW_GET_CLASS (preview);

  if (class->draw)
    class->draw (preview);
}

/**
 * gimp_preview_draw_buffer:
 * @preview:   a #GimpPreview widget
 * @buffer:    a pixel buffer the size of the preview
 * @rowstride: the @buffer's rowstride
 *
 * Calls the GimpPreview::draw_buffer method. GimpPreview itself
 * doesn't implement this method so the behaviour is determined by the
 * derived class implementing this method.
 *
 * Since: 2.2
 **/
void
gimp_preview_draw_buffer (GimpPreview  *preview,
                          const guchar *buffer,
                          gint          rowstride)
{
  GimpPreviewClass *class = GIMP_PREVIEW_GET_CLASS (preview);

  if (class->draw_buffer)
    class->draw_buffer (preview, buffer, rowstride);
}

/**
 * gimp_preview_invalidate:
 * @preview: a #GimpPreview widget
 *
 * This function starts or renews a short low-priority timeout. When
 * the timeout expires, the GimpPreview::invalidated signal is emitted
 * which will usually cause the @preview to be updated.
 *
 * This function does nothing unless the "Preview" button is checked.
 *
 * During the emission of the signal a busy cursor is set on the
 * toplevel window containing the @preview and on the preview area
 * itself.
 *
 * Since: 2.2
 **/
void
gimp_preview_invalidate (GimpPreview *preview)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  priv = GET_PRIVATE (preview);

  if (priv->update_preview)
    {
      if (priv->timeout_id)
        g_source_remove (priv->timeout_id);

      priv->timeout_id =
        g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, PREVIEW_TIMEOUT,
                            (GSourceFunc) gimp_preview_invalidate_now,
                            preview, NULL);
    }
}

/**
 * gimp_preview_set_default_cursor:
 * @preview: a #GimpPreview widget
 * @cursor:  a #GdkCursor or %NULL
 *
 * Sets the default mouse cursor for the preview.  Note that this will
 * be overridden by a %GDK_FLEUR if the preview has scrollbars, or by a
 * %GDK_WATCH when the preview is invalidated.
 *
 * Since: 2.2
 **/
void
gimp_preview_set_default_cursor (GimpPreview *preview,
                                 GdkCursor   *cursor)
{
  GimpPreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_PREVIEW (preview));

  priv = GET_PRIVATE (preview);

  if (cursor != priv->default_cursor)
    {
      if (priv->default_cursor)
        g_object_unref (priv->default_cursor);

      if (cursor)
        g_object_ref (cursor);

      priv->default_cursor = cursor;
    }
}

GdkCursor *
gimp_preview_get_default_cursor (GimpPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->default_cursor;
}

/**
 * gimp_preview_get_controls:
 * @preview: a #GimpPreview widget
 *
 * Gives access to the #GtkHBox at the bottom of the preview that
 * contains the update toggle. Derived widgets can use this function
 * if they need to add controls to this area.
 *
 * Return value: the #GtkHBox at the bottom of the preview.
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_preview_get_controls (GimpPreview *preview)
{
  g_return_val_if_fail (GIMP_IS_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->controls;
}
