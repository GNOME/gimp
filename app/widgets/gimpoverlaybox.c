/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaOverlayBox
 * Copyright (C) 2009 Michael Natterer <mitch@ligma.org>
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "ligmaoverlaybox.h"
#include "ligmaoverlaychild.h"


/*  local function prototypes  */

static void        ligma_overlay_box_set_property        (GObject        *object,
                                                         guint           property_id,
                                                         const GValue   *value,
                                                         GParamSpec     *pspec);
static void        ligma_overlay_box_get_property        (GObject        *object,
                                                         guint           property_id,
                                                         GValue         *value,
                                                         GParamSpec     *pspec);

static void        ligma_overlay_box_realize             (GtkWidget      *widget);
static void        ligma_overlay_box_unrealize           (GtkWidget      *widget);
static void        ligma_overlay_box_get_preferred_width (GtkWidget      *widget,
                                                         gint           *minimum_width,
                                                         gint           *natural_width);
static void        ligma_overlay_box_get_preferred_height(GtkWidget      *widget,
                                                         gint           *minimum_height,
                                                         gint           *natural_height);
static void        ligma_overlay_box_size_allocate       (GtkWidget      *widget,
                                                         GtkAllocation  *allocation);
static gboolean    ligma_overlay_box_draw                (GtkWidget      *widget,
                                                         cairo_t        *cr);
static gboolean    ligma_overlay_box_damage              (GtkWidget      *widget,
                                                         GdkEventExpose *event);

static void        ligma_overlay_box_add                 (GtkContainer   *container,
                                                         GtkWidget      *widget);
static void        ligma_overlay_box_remove              (GtkContainer   *container,
                                                         GtkWidget      *widget);
static void        ligma_overlay_box_forall              (GtkContainer   *container,
                                                         gboolean        include_internals,
                                                         GtkCallback     callback,
                                                         gpointer        callback_data);
static GType       ligma_overlay_box_child_type          (GtkContainer   *container);

static GdkWindow * ligma_overlay_box_pick_embedded_child (GdkWindow      *window,
                                                         gdouble         x,
                                                         gdouble         y,
                                                         LigmaOverlayBox *box);


G_DEFINE_TYPE (LigmaOverlayBox, ligma_overlay_box, GTK_TYPE_CONTAINER)

#define parent_class ligma_overlay_box_parent_class


static void
ligma_overlay_box_class_init (LigmaOverlayBoxClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->set_property         = ligma_overlay_box_set_property;
  object_class->get_property         = ligma_overlay_box_get_property;

  widget_class->realize              = ligma_overlay_box_realize;
  widget_class->unrealize            = ligma_overlay_box_unrealize;
  widget_class->get_preferred_width  = ligma_overlay_box_get_preferred_width;
  widget_class->get_preferred_height = ligma_overlay_box_get_preferred_height;
  widget_class->size_allocate        = ligma_overlay_box_size_allocate;
  widget_class->draw                 = ligma_overlay_box_draw;

  g_signal_override_class_handler ("damage-event",
                                   LIGMA_TYPE_OVERLAY_BOX,
                                   G_CALLBACK (ligma_overlay_box_damage));

  container_class->add        = ligma_overlay_box_add;
  container_class->remove     = ligma_overlay_box_remove;
  container_class->forall     = ligma_overlay_box_forall;
  container_class->child_type = ligma_overlay_box_child_type;
}

static void
ligma_overlay_box_init (LigmaOverlayBox *box)
{
  gtk_widget_set_has_window (GTK_WIDGET (box), TRUE);
}

static void
ligma_overlay_box_set_property (GObject      *object,
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
ligma_overlay_box_get_property (GObject    *object,
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
ligma_overlay_box_realize (GtkWidget *widget)
{
  LigmaOverlayBox *box = LIGMA_OVERLAY_BOX (widget);
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
                    G_CALLBACK (ligma_overlay_box_pick_embedded_child),
                    widget);

  for (list = box->children; list; list = g_list_next (list))
    ligma_overlay_child_realize (box, list->data);
}

static void
ligma_overlay_box_unrealize (GtkWidget *widget)
{
  LigmaOverlayBox *box = LIGMA_OVERLAY_BOX (widget);
  GList          *list;

  for (list = box->children; list; list = g_list_next (list))
    ligma_overlay_child_unrealize (box, list->data);

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
ligma_overlay_box_get_preferred_width (GtkWidget *widget,
                                      gint      *minimum_width,
                                      gint      *natural_width)
{
  LigmaOverlayBox *box = LIGMA_OVERLAY_BOX (widget);
  GList          *list;

  *minimum_width = *natural_width = 0;

  for (list = box->children; list; list = g_list_next (list))
    {
      gint minimum;
      gint natural;

      ligma_overlay_child_get_preferred_width (box, list->data,
                                              &minimum, &natural);

      *minimum_width = MAX (*minimum_width, minimum);
      *natural_width = MAX (*natural_width, natural);
    }

  *minimum_width = 0;

  *minimum_width +=
    2 * gtk_container_get_border_width (GTK_CONTAINER (widget)) + 1;

  *natural_width +=
    2 * gtk_container_get_border_width (GTK_CONTAINER (widget)) + 1;
}

static void
ligma_overlay_box_get_preferred_height (GtkWidget *widget,
                                       gint      *minimum_height,
                                       gint      *natural_height)
{
  LigmaOverlayBox *box = LIGMA_OVERLAY_BOX (widget);
  GList          *list;

  *minimum_height = *natural_height = 0;

  for (list = box->children; list; list = g_list_next (list))
    {
      gint minimum;
      gint natural;

      ligma_overlay_child_get_preferred_height (box, list->data,
                                               &minimum, &natural);

      *minimum_height = MAX (*minimum_height, minimum);
      *natural_height = MAX (*natural_height, natural);
    }

  *minimum_height = 0;

  *minimum_height +=
    2 * gtk_container_get_border_width (GTK_CONTAINER (widget)) + 1;

  *natural_height +=
    2 * gtk_container_get_border_width (GTK_CONTAINER (widget)) + 1;
}

static void
ligma_overlay_box_size_allocate (GtkWidget     *widget,
                                GtkAllocation *allocation)
{
  LigmaOverlayBox *box = LIGMA_OVERLAY_BOX (widget);
  GList          *list;

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  for (list = box->children; list; list = g_list_next (list))
    ligma_overlay_child_size_allocate (box, list->data);
}

static gboolean
ligma_overlay_box_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  LigmaOverlayBox *box = LIGMA_OVERLAY_BOX (widget);
  GList          *list;

  for (list = box->children; list; list = g_list_next (list))
    {
      if (ligma_overlay_child_draw (box, list->data, cr))
        return FALSE;
    }

  return FALSE;
}

static gboolean
ligma_overlay_box_damage (GtkWidget      *widget,
                         GdkEventExpose *event)
{
  LigmaOverlayBox *box = LIGMA_OVERLAY_BOX (widget);
  GList          *list;

  for (list = box->children; list; list = g_list_next (list))
    {
      if (ligma_overlay_child_damage (box, list->data, event))
        return FALSE;
    }

  return FALSE;
}

static void
ligma_overlay_box_add (GtkContainer *container,
                      GtkWidget    *widget)
{
  ligma_overlay_box_add_child (LIGMA_OVERLAY_BOX (container), widget, 0.5, 0.5);
}

static void
ligma_overlay_box_remove (GtkContainer *container,
                         GtkWidget    *widget)
{
  LigmaOverlayBox   *box   = LIGMA_OVERLAY_BOX (container);
  LigmaOverlayChild *child = ligma_overlay_child_find (box, widget);

  if (child)
    {
      if (gtk_widget_get_visible (widget))
        ligma_overlay_child_invalidate (box, child);

      box->children = g_list_remove (box->children, child);

      ligma_overlay_child_free (box, child);
    }
}

static void
ligma_overlay_box_forall (GtkContainer *container,
                         gboolean      include_internals,
                         GtkCallback   callback,
                         gpointer      callback_data)
{
  LigmaOverlayBox *box = LIGMA_OVERLAY_BOX (container);
  GList          *list;

  list = box->children;
  while (list)
    {
      LigmaOverlayChild *child = list->data;

      list = list->next;

      (* callback) (child->widget, callback_data);
    }
}

static GType
ligma_overlay_box_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

static GdkWindow *
ligma_overlay_box_pick_embedded_child (GdkWindow      *parent,
                                      gdouble         parent_x,
                                      gdouble         parent_y,
                                      LigmaOverlayBox *box)
{
  GList *list;

  for (list = box->children; list; list = g_list_next (list))
    {
      LigmaOverlayChild *child = list->data;

      if (ligma_overlay_child_pick (box, child, parent_x, parent_y))
        return child->window;
    }

  return NULL;
}


/*  public functions  */

/**
 * ligma_overlay_box_new:
 *
 * Creates a new #LigmaOverlayBox widget.
 *
 * Returns: a new #LigmaOverlayBox widget
 **/
GtkWidget *
ligma_overlay_box_new (void)
{
  return g_object_new (LIGMA_TYPE_OVERLAY_BOX, NULL);
}

void
ligma_overlay_box_add_child (LigmaOverlayBox *box,
                            GtkWidget      *widget,
                            gdouble         xalign,
                            gdouble         yalign)
{
  LigmaOverlayChild *child;

  g_return_if_fail (LIGMA_IS_OVERLAY_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  child = ligma_overlay_child_new (box, widget, xalign, yalign, 0.0, 0.85);

  box->children = g_list_append (box->children, child);
}

void
ligma_overlay_box_set_child_alignment (LigmaOverlayBox *box,
                                      GtkWidget      *widget,
                                      gdouble         xalign,
                                      gdouble         yalign)
{
  LigmaOverlayChild *child = ligma_overlay_child_find (box, widget);

  if (child)
    {
      xalign = CLAMP (xalign, 0.0, 1.0);
      yalign = CLAMP (yalign, 0.0, 1.0);

      if (child->has_position     ||
          child->xalign != xalign ||
          child->yalign != yalign)
        {
          ligma_overlay_child_invalidate (box, child);

          child->has_position = FALSE;
          child->xalign       = xalign;
          child->yalign       = yalign;

          gtk_widget_queue_resize (widget);
        }
    }
}

void
ligma_overlay_box_set_child_position (LigmaOverlayBox *box,
                                     GtkWidget      *widget,
                                     gdouble         x,
                                     gdouble         y)
{
  LigmaOverlayChild *child = ligma_overlay_child_find (box, widget);

  if (child)
    {
      if (! child->has_position ||
          child->x != x         ||
          child->y != y)
        {
          ligma_overlay_child_invalidate (box, child);

          child->has_position = TRUE;
          child->x            = x;
          child->y            = y;

          gtk_widget_queue_resize (widget);
        }
    }
}

void
ligma_overlay_box_set_child_angle (LigmaOverlayBox *box,
                                  GtkWidget      *widget,
                                  gdouble         angle)
{
  LigmaOverlayChild *child = ligma_overlay_child_find (box, widget);

  if (child)
    {
      if (child->angle != angle)
        {
          ligma_overlay_child_invalidate (box, child);

          child->angle = angle;

          gtk_widget_queue_draw (widget);
        }
    }
}

void
ligma_overlay_box_set_child_opacity (LigmaOverlayBox *box,
                                    GtkWidget      *widget,
                                    gdouble         opacity)
{
  LigmaOverlayChild *child = ligma_overlay_child_find (box, widget);

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
 * ligma_overlay_box_scroll:
 * @box: the #LigmaOverlayBox widget to scroll.
 * @offset_x: the x scroll amount.
 * @offset_y: the y scroll amount.
 *
 * Scrolls the box using gdk_window_scroll(), taking care of properly
 * handling overlay children.
 **/
void
ligma_overlay_box_scroll (LigmaOverlayBox *box,
                         gint            offset_x,
                         gint            offset_y)
{
  GtkWidget *widget;
  GdkWindow *window;
  GList     *list;

  g_return_if_fail (LIGMA_IS_OVERLAY_BOX (box));

  widget = GTK_WIDGET (box);

  /* bug 761118 */
  if (! gtk_widget_get_realized (widget))
    return;

  window = gtk_widget_get_window (widget);

  /*  Undraw all overlays  */
  for (list = box->children; list; list = g_list_next (list))
    {
      LigmaOverlayChild *child = list->data;

      ligma_overlay_child_invalidate (box, child);
    }

  gdk_window_scroll (window, offset_x, offset_y);

  /*  Re-draw all overlays  */
  for (list = box->children; list; list = g_list_next (list))
    {
      LigmaOverlayChild *child = list->data;

      ligma_overlay_child_invalidate (box, child);
    }
}
