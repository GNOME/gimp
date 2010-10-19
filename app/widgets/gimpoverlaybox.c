/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpOverlayBox
 * Copyright (C) 2009 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpoverlaybox.h"
#include "gimpoverlaychild.h"


/*  local function prototypes  */

static void        gimp_overlay_box_set_property        (GObject        *object,
                                                         guint           property_id,
                                                         const GValue   *value,
                                                         GParamSpec     *pspec);
static void        gimp_overlay_box_get_property        (GObject        *object,
                                                         guint           property_id,
                                                         GValue         *value,
                                                         GParamSpec     *pspec);

static void        gimp_overlay_box_realize             (GtkWidget      *widget);
static void        gimp_overlay_box_unrealize           (GtkWidget      *widget);
static void        gimp_overlay_box_size_request        (GtkWidget      *widget,
                                                         GtkRequisition *requisition);
static void        gimp_overlay_box_size_allocate       (GtkWidget      *widget,
                                                         GtkAllocation  *allocation);
static gboolean    gimp_overlay_box_draw                (GtkWidget      *widget,
                                                         cairo_t        *cr);
static gboolean    gimp_overlay_box_damage              (GtkWidget      *widget,
                                                         GdkEventExpose *event);

static void        gimp_overlay_box_add                 (GtkContainer   *container,
                                                         GtkWidget      *widget);
static void        gimp_overlay_box_remove              (GtkContainer   *container,
                                                         GtkWidget      *widget);
static void        gimp_overlay_box_forall              (GtkContainer   *container,
                                                         gboolean        include_internals,
                                                         GtkCallback     callback,
                                                         gpointer        callback_data);
static GType       gimp_overlay_box_child_type          (GtkContainer   *container);

static GdkWindow * gimp_overlay_box_pick_embedded_child (GdkWindow      *window,
                                                         gdouble         x,
                                                         gdouble         y,
                                                         GimpOverlayBox *box);


G_DEFINE_TYPE (GimpOverlayBox, gimp_overlay_box, GTK_TYPE_CONTAINER)

#define parent_class gimp_overlay_box_parent_class


static void
gimp_overlay_box_class_init (GimpOverlayBoxClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->set_property  = gimp_overlay_box_set_property;
  object_class->get_property  = gimp_overlay_box_get_property;

  widget_class->realize       = gimp_overlay_box_realize;
  widget_class->unrealize     = gimp_overlay_box_unrealize;
  widget_class->size_request  = gimp_overlay_box_size_request;
  widget_class->size_allocate = gimp_overlay_box_size_allocate;
  widget_class->draw          = gimp_overlay_box_draw;

  g_signal_override_class_handler ("damage-event",
                                   GIMP_TYPE_OVERLAY_BOX,
                                   G_CALLBACK (gimp_overlay_box_damage));

  container_class->add        = gimp_overlay_box_add;
  container_class->remove     = gimp_overlay_box_remove;
  container_class->forall     = gimp_overlay_box_forall;
  container_class->child_type = gimp_overlay_box_child_type;
}

static void
gimp_overlay_box_init (GimpOverlayBox *box)
{
  gtk_widget_set_has_window (GTK_WIDGET (box), TRUE);
}

static void
gimp_overlay_box_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_overlay_box_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_overlay_box_realize (GtkWidget *widget)
{
  GimpOverlayBox *box = GIMP_OVERLAY_BOX (widget);
  GtkAllocation   allocation;
  GdkWindowAttr   attributes;
  gint            attributes_mask;
  GList          *list;

  gtk_widget_set_realized (widget, TRUE);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.x           = allocation.x;
  attributes.y           = allocation.y;
  attributes.width       = allocation.width;
  attributes.height      = allocation.height;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.wclass      = GDK_INPUT_OUTPUT;
  attributes.visual      = gtk_widget_get_visual (widget);
  attributes.event_mask  = gtk_widget_get_events (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

  gtk_widget_set_window (widget,
                         gdk_window_new (gtk_widget_get_parent_window (widget),
                                         &attributes, attributes_mask));
  gdk_window_set_user_data (gtk_widget_get_window (widget), widget);

  g_signal_connect (gtk_widget_get_window (widget), "pick-embedded-child",
                    G_CALLBACK (gimp_overlay_box_pick_embedded_child),
                    widget);

  gtk_widget_style_attach (widget);
  gtk_style_set_background (gtk_widget_get_style (widget),
                            gtk_widget_get_window (widget),
                            GTK_STATE_NORMAL);

  for (list = box->children; list; list = g_list_next (list))
    gimp_overlay_child_realize (box, list->data);
}

static void
gimp_overlay_box_unrealize (GtkWidget *widget)
{
  GimpOverlayBox *box = GIMP_OVERLAY_BOX (widget);
  GList          *list;

  for (list = box->children; list; list = g_list_next (list))
    gimp_overlay_child_unrealize (box, list->data);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_overlay_box_size_request (GtkWidget      *widget,
                               GtkRequisition *requisition)
{
  GimpOverlayBox *box = GIMP_OVERLAY_BOX (widget);
  GList          *list;
  gint            border_width;

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  requisition->width  = 1 + 2 * border_width;
  requisition->height = 1 + 2 * border_width;

  for (list = box->children; list; list = g_list_next (list))
    gimp_overlay_child_size_request (box, list->data);
}

static void
gimp_overlay_box_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  GimpOverlayBox *box = GIMP_OVERLAY_BOX (widget);
  GList          *list;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  for (list = box->children; list; list = g_list_next (list))
    gimp_overlay_child_size_allocate (box, list->data);
}

static gboolean
gimp_overlay_box_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  GimpOverlayBox *box = GIMP_OVERLAY_BOX (widget);
  GList          *list;

  for (list = box->children; list; list = g_list_next (list))
    {
      if (gimp_overlay_child_draw (box, list->data, cr))
        return FALSE;
    }

  return FALSE;
}

static gboolean
gimp_overlay_box_damage (GtkWidget      *widget,
                         GdkEventExpose *event)
{
  GimpOverlayBox *box = GIMP_OVERLAY_BOX (widget);
  GList          *list;

  for (list = box->children; list; list = g_list_next (list))
    {
      if (gimp_overlay_child_damage (box, list->data, event))
        return FALSE;
    }

  return FALSE;
}

static void
gimp_overlay_box_add (GtkContainer *container,
                      GtkWidget    *widget)
{
  gimp_overlay_box_add_child (GIMP_OVERLAY_BOX (container), widget, 0.5, 0.5);
}

static void
gimp_overlay_box_remove (GtkContainer *container,
                         GtkWidget    *widget)
{
  GimpOverlayBox   *box   = GIMP_OVERLAY_BOX (container);
  GimpOverlayChild *child = gimp_overlay_child_find (box, widget);

  if (child)
    {
      if (gtk_widget_get_visible (widget))
        gimp_overlay_child_invalidate (box, child);

      box->children = g_list_remove (box->children, child);

      gimp_overlay_child_free (box, child);
    }
}

static void
gimp_overlay_box_forall (GtkContainer *container,
                         gboolean      include_internals,
                         GtkCallback   callback,
                         gpointer      callback_data)
{
  GimpOverlayBox *box = GIMP_OVERLAY_BOX (container);
  GList          *list;

  list = box->children;
  while (list)
    {
      GimpOverlayChild *child = list->data;

      list = list->next;

      (* callback) (child->widget, callback_data);
    }
}

static GType
gimp_overlay_box_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

static GdkWindow *
gimp_overlay_box_pick_embedded_child (GdkWindow      *parent,
                                      gdouble         parent_x,
                                      gdouble         parent_y,
                                      GimpOverlayBox *box)
{
  GList *list;

  for (list = box->children; list; list = g_list_next (list))
    {
      GimpOverlayChild *child = list->data;

      if (gimp_overlay_child_pick (box, child, parent_x, parent_y))
        return child->window;
    }

  return NULL;
}


/*  public functions  */

/**
 * gimp_overlay_box_new:
 *
 * Creates a new #GimpOverlayBox widget.
 *
 * Return value: a new #GimpOverlayBox widget
 **/
GtkWidget *
gimp_overlay_box_new (void)
{
  return g_object_new (GIMP_TYPE_OVERLAY_BOX, NULL);
}

void
gimp_overlay_box_add_child (GimpOverlayBox *box,
                            GtkWidget      *widget,
                            gdouble         xalign,
                            gdouble         yalign)
{
  GimpOverlayChild *child;

  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  child = gimp_overlay_child_new (box, widget, xalign, yalign, 0.0, 0.85);

  box->children = g_list_append (box->children, child);
}

void
gimp_overlay_box_set_child_alignment (GimpOverlayBox *box,
                                      GtkWidget      *widget,
                                      gdouble         xalign,
                                      gdouble         yalign)
{
  GimpOverlayChild *child = gimp_overlay_child_find (box, widget);

  if (child)
    {
      xalign = CLAMP (xalign, 0.0, 1.0);
      yalign = CLAMP (yalign, 0.0, 1.0);

      if (child->has_position     ||
          child->xalign != xalign ||
          child->yalign != yalign)
        {
          gimp_overlay_child_invalidate (box, child);

          child->has_position = FALSE;
          child->xalign       = xalign;
          child->yalign       = yalign;

          gtk_widget_queue_resize (widget);
        }
    }
}

void
gimp_overlay_box_set_child_position (GimpOverlayBox *box,
                                     GtkWidget      *widget,
                                     gdouble         x,
                                     gdouble         y)
{
  GimpOverlayChild *child = gimp_overlay_child_find (box, widget);

  if (child)
    {
      if (! child->has_position ||
          child->x != x         ||
          child->y != y)
        {
          gimp_overlay_child_invalidate (box, child);

          child->has_position = TRUE;
          child->x            = x;
          child->y            = y;

          gtk_widget_queue_resize (widget);
        }
    }
}

void
gimp_overlay_box_set_child_angle (GimpOverlayBox *box,
                                  GtkWidget      *widget,
                                  gdouble         angle)
{
  GimpOverlayChild *child = gimp_overlay_child_find (box, widget);

  if (child)
    {
      if (child->angle != angle)
        {
          gimp_overlay_child_invalidate (box, child);

          child->angle = angle;

          gtk_widget_queue_draw (widget);
        }
    }
}

void
gimp_overlay_box_set_child_opacity (GimpOverlayBox *box,
                                    GtkWidget      *widget,
                                    gdouble         opacity)
{
  GimpOverlayChild *child = gimp_overlay_child_find (box, widget);

  if (child)
    {
      opacity = CLAMP (opacity, 0.0, 1.0);

      if (child->opacity != opacity)
        {
          child->opacity = opacity;

          gtk_widget_queue_draw (widget);
        }
    }
}

/**
 * gimp_overlay_box_scroll:
 * @box: the #GimpOverlayBox widget to scroll.
 * @offset_x: the x scroll amount.
 * @offset_y: the y scroll amount.
 *
 * Scrolls the box using gdk_window_scroll() and makes sure the result
 * is displayed immediately by calling gdk_window_process_updates().
 **/
void
gimp_overlay_box_scroll (GimpOverlayBox *box,
                         gint            offset_x,
                         gint            offset_y)
{
  GtkWidget *widget;
  GdkWindow *window;
  GList     *list;

  g_return_if_fail (GIMP_IS_OVERLAY_BOX (box));

  widget = GTK_WIDGET (box);

  /* bug 761118 */
  if (! gtk_widget_get_realized (widget))
    return;

  window = gtk_widget_get_window (widget);

  /*  Undraw all overlays  */
  for (list = box->children; list; list = g_list_next (list))
    {
      GimpOverlayChild *child = list->data;

      gimp_overlay_child_invalidate (box, child);
    }

  gdk_window_scroll (window, offset_x, offset_y);

  /*  Re-draw all overlays  */
  for (list = box->children; list; list = g_list_next (list))
    {
      GimpOverlayChild *child = list->data;

      gimp_overlay_child_invalidate (box, child);
    }

  /*  Make sure expose events are processed before scrolling again  */
  gdk_window_process_updates (window, FALSE);
}
