/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppanedbox.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2009-2011 Martin Nordholts <martinn@src.gnome.org>
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

#include <gegl.h>
#ifdef GDK_DISABLE_DEPRECATED
#undef GDK_DISABLE_DEPRECATED
#endif
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmarshal.h"

#include "gimpdialogfactory.h"
#include "gimpdnd.h"
#include "gimpdockable.h"
#include "gimpdockbook.h"
#include "gimpmenudock.h"
#include "gimppanedbox.h"
#include "gimptoolbox.h"

#include "gimp-log.h"


/**
 * Defines the size of the area that dockables can be dropped on in
 * order to be inserted and get space on their own (rather than
 * inserted among others and sharing space)
 */
#define DROP_AREA_SIZE                  6

#define DROP_HIGHLIGHT_MIN_SIZE         32
#define DROP_HIGHLIGHT_COLOR            "#215d9c"
#define DROP_HIGHLIGHT_OPACITY_ACTIVE   1.0
#define DROP_HIGHLIGHT_OPACITY_INACTIVE 0.5

#define INSERT_INDEX_UNUSED             G_MININT


struct _GimpPanedBoxPrivate
{
  /* Widgets that are separated by panes */
  GList                  *widgets;

  /* Windows used for drag-and-drop output */
  GdkWindow              *dnd_windows[3];
  GdkDragContext         *dnd_context;
  gint                    dnd_paned_position;
  gint                    dnd_idle_id;

  /* The insert index to use on drop */
  gint                    insert_index;

  /* Callback on drop */
  GimpPanedBoxDroppedFunc dropped_cb;
  gpointer                dropped_cb_data;

  /* A drag handler offered to handle drag events */
  GimpPanedBox           *drag_handler;
};


static void      gimp_paned_box_dispose                 (GObject        *object);

static void      gimp_paned_box_drag_leave              (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         guint           time);
static gboolean  gimp_paned_box_drag_motion             (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         gint            x,
                                                         gint            y,
                                                         guint           time);
static gboolean  gimp_paned_box_drag_drop               (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         gint            x,
                                                         gint            y,
                                                         guint           time);
static void      gimp_paned_box_realize                 (GtkWidget      *widget);
static void      gimp_paned_box_unrealize               (GtkWidget      *widget);
static void      gimp_paned_box_set_widget_drag_handler (GtkWidget      *widget,
                                                         GimpPanedBox   *handler);
static gint      gimp_paned_box_get_drop_area_size      (GimpPanedBox   *paned_box);

static void      gimp_paned_box_drag_callback           (GdkDragContext *context,
                                                         gboolean        begin,
                                                         GimpPanedBox   *paned_box);


G_DEFINE_TYPE_WITH_PRIVATE (GimpPanedBox, gimp_paned_box, GTK_TYPE_BOX)

#define parent_class gimp_paned_box_parent_class

static const GtkTargetEntry dialog_target_table[] = { GIMP_TARGET_DIALOG };


static void
gimp_paned_box_class_init (GimpPanedBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose     = gimp_paned_box_dispose;

  widget_class->drag_leave  = gimp_paned_box_drag_leave;
  widget_class->drag_motion = gimp_paned_box_drag_motion;
  widget_class->drag_drop   = gimp_paned_box_drag_drop;
  widget_class->realize     = gimp_paned_box_realize;
  widget_class->unrealize   = gimp_paned_box_unrealize;
}

static void
gimp_paned_box_init (GimpPanedBox *paned_box)
{
  paned_box->p = gimp_paned_box_get_instance_private (paned_box);

  /* Setup DND */
  gtk_drag_dest_set (GTK_WIDGET (paned_box),
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);

  gimp_dockbook_add_drag_callback (
    (GimpDockbookDragCallback) gimp_paned_box_drag_callback,
    paned_box);
}

static void
gimp_paned_box_dispose (GObject *object)
{
  GimpPanedBox *paned_box = GIMP_PANED_BOX (object);

  if (paned_box->p->dnd_idle_id)
    {
      g_source_remove (paned_box->p->dnd_idle_id);

      paned_box->p->dnd_idle_id = 0;
    }

  while (paned_box->p->widgets)
    {
      GtkWidget *widget = paned_box->p->widgets->data;

      g_object_ref (widget);
      gimp_paned_box_remove_widget (paned_box, widget);
      gtk_widget_destroy (widget);
      g_object_unref (widget);
    }

  gimp_dockbook_remove_drag_callback (
    (GimpDockbookDragCallback) gimp_paned_box_drag_callback,
    paned_box);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_paned_box_realize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (parent_class)->realize (widget);

  /* We realize() dnd_window on demand in
   * gimp_paned_box_show_separators()
   */
}

static void
gimp_paned_box_unrealize (GtkWidget *widget)
{
  GimpPanedBox *paned_box = GIMP_PANED_BOX (widget);
  gint          i;

  for (i = 0; i < G_N_ELEMENTS (paned_box->p->dnd_windows); i++)
    {
      GdkWindow *window = paned_box->p->dnd_windows[i];

      if (window)
        {
          gdk_window_set_user_data (window, NULL);
          gdk_window_destroy (window);

          paned_box->p->dnd_windows[i] = NULL;
        }
    }

  GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
gimp_paned_box_set_widget_drag_handler (GtkWidget    *widget,
                                        GimpPanedBox *drag_handler)
{
  /* Hook us in for drag events. We could abstract this properly and
   * put gimp_paned_box_will_handle_drag() in an interface for
   * example, but it doesn't feel worth it at this point
   *
   * Note that we don't have 'else if's because a widget can be both a
   * dock and a toolbox for example, in which case we want to set a
   * drag handler in two ways
   *
   * We so need to introduce some abstractions here...
   */

  if (GIMP_IS_DOCKBOOK (widget))
    {
      gimp_dockbook_set_drag_handler (GIMP_DOCKBOOK (widget),
                                      drag_handler);
    }

  if (GIMP_IS_DOCK (widget))
    {
      GimpPanedBox *dock_paned_box = NULL;
      dock_paned_box = GIMP_PANED_BOX (gimp_dock_get_vbox (GIMP_DOCK (widget)));
      gimp_paned_box_set_drag_handler (dock_paned_box, drag_handler);
    }

  if (GIMP_IS_TOOLBOX (widget))
    {
      GimpToolbox *toolbox = GIMP_TOOLBOX (widget);
      gimp_toolbox_set_drag_handler (toolbox, drag_handler);
    }
}

static gint
gimp_paned_box_get_drop_area_size (GimpPanedBox *paned_box)
{
  gint drop_area_size = 0;

  if (! paned_box->p->widgets)
    {
      GtkAllocation  allocation;
      GtkOrientation orientation;

      gtk_widget_get_allocation (GTK_WIDGET (paned_box), &allocation);
      orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (paned_box));

      if (orientation == GTK_ORIENTATION_HORIZONTAL)
        drop_area_size = allocation.width;
      else if (orientation == GTK_ORIENTATION_VERTICAL)
        drop_area_size = allocation.height;
    }

  drop_area_size = MAX (drop_area_size, DROP_AREA_SIZE);

  return drop_area_size;
}

static gboolean
gimp_paned_box_get_handle_drag (GimpPanedBox   *paned_box,
                                GdkDragContext *context,
                                gint            x,
                                gint            y,
                                guint           time,
                                gint           *insert_index,
                                GeglRectangle  *area)
{
  gint           index          = INSERT_INDEX_UNUSED;
  GtkAllocation  allocation     = { 0, };
  gint           area_x         = 0;
  gint           area_y         = 0;
  gint           area_w         = 0;
  gint           area_h         = 0;
  GtkOrientation orientation    = 0;
  gint           drop_area_size = gimp_paned_box_get_drop_area_size (paned_box);

  if (gimp_paned_box_will_handle_drag (paned_box->p->drag_handler,
                                       GTK_WIDGET (paned_box),
                                       context,
                                       x, y,
                                       time))
    {
      return FALSE;
    }

  gtk_widget_get_allocation (GTK_WIDGET (paned_box), &allocation);

  /* See if we're at the edge of the dock If there are no dockables,
   * the entire paned box is a drop area
   */
  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (paned_box));
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      area_y = 0;
      area_h = allocation.height;

      /* If there are no widgets, the drop area is as big as the paned
       * box
       */
      if (! paned_box->p->widgets)
        area_w = allocation.width;
      else
        area_w = drop_area_size;

      if (x < drop_area_size)
        {
          index = 0;
          area_x = 0;
        }
      if (x > allocation.width - drop_area_size)
        {
          index = -1;
          area_x = allocation.width - drop_area_size;
        }
    }
  else /* if (orientation = GTK_ORIENTATION_VERTICAL) */
    {
      area_x = 0;
      area_w = allocation.width;

      /* If there are no widgets, the drop area is as big as the paned
       * box
       */
      if (! paned_box->p->widgets)
        area_h = allocation.height;
      else
        area_h = drop_area_size;

      if (y < drop_area_size)
        {
          index = 0;
          area_y = 0;
        }
      if (y > allocation.height - drop_area_size)
        {
          index = -1;
          area_y = allocation.height - drop_area_size;
        }
    }

  if (area)
    {
      area->x      = allocation.x + area_x;
      area->y      = allocation.y + area_y;
      area->width  = area_w;
      area->height = area_h;
    }

  if (insert_index)
    *insert_index = index;

  return index != INSERT_INDEX_UNUSED;
}

static void
gimp_paned_box_position_drop_indicator (GimpPanedBox *paned_box,
                                        gint          index,
                                        gint          x,
                                        gint          y,
                                        gint          width,
                                        gint          height,
                                        gdouble       opacity)
{
  GtkWidget    *widget = GTK_WIDGET (paned_box);
  GdkWindow    *window = paned_box->p->dnd_windows[index];
  GtkStyle     *style  = gtk_widget_get_style (widget);
  GtkStateType  state  = gtk_widget_get_state (widget);
  GimpRGB       bg;
  GimpRGB       fg;
  GdkColor      color;

  if (! gtk_widget_is_drawable (widget))
    return;

  /* Create or move the GdkWindow in place */
  if (! window)
    {
      GtkAllocation allocation;
      GdkWindowAttr attributes;

      gtk_widget_get_allocation (widget, &allocation);

      attributes.x           = x;
      attributes.y           = y;
      attributes.width       = width;
      attributes.height      = height;
      attributes.window_type = GDK_WINDOW_CHILD;
      attributes.wclass      = GDK_INPUT_OUTPUT;
      attributes.event_mask  = gtk_widget_get_events (widget);

      window = gdk_window_new (gtk_widget_get_window (widget),
                               &attributes,
                               GDK_WA_X | GDK_WA_Y);
      gdk_window_set_user_data (window, widget);

      paned_box->p->dnd_windows[index] = window;
    }
  else
    {
      gdk_window_move_resize (window,
                              x, y,
                              width, height);
    }

  gimp_rgb_set_uchar (&bg,
                      style->bg[state].red   >> 8,
                      style->bg[state].green >> 8,
                      style->bg[state].blue  >> 8);
  bg.a = 1.0;

  gimp_rgb_parse_hex (&fg, DROP_HIGHLIGHT_COLOR, -1);
  fg.a = opacity;

  gimp_rgb_composite (&bg, &fg, GIMP_RGB_COMPOSITE_NORMAL);

  color.red   = bg.r * 0xffff;
  color.green = bg.g * 0xffff;
  color.blue  = bg.b * 0xffff;

  gdk_rgb_find_color (gtk_widget_get_colormap (widget), &color);

  gdk_window_set_background (window, &color);

  gdk_window_show (window);
}

static void
gimp_paned_box_hide_drop_indicator (GimpPanedBox *paned_box,
                                    gint          index)
{
  if (! paned_box->p->dnd_windows[index])
    return;

  gdk_window_hide (paned_box->p->dnd_windows[index]);
}

static void
gimp_paned_box_drag_leave (GtkWidget      *widget,
                           GdkDragContext *context,
                           guint           time)
{
  GimpPanedBox *paned_box = GIMP_PANED_BOX (widget);

  gimp_paned_box_hide_drop_indicator (paned_box, 0);
}

static gboolean
gimp_paned_box_drag_motion (GtkWidget      *widget,
                            GdkDragContext *context,
                            gint            x,
                            gint            y,
                            guint           time)
{
  GimpPanedBox  *paned_box = GIMP_PANED_BOX (widget);
  gint           insert_index;
  GeglRectangle  area;
  gboolean       handle;

  handle = gimp_paned_box_get_handle_drag (paned_box, context, x, y, time,
                                           &insert_index, &area);

  /* If we are at the edge, show a GdkWindow to communicate that a
   * drop will create a new dock column
   */
  if (handle)
    {
      gimp_paned_box_position_drop_indicator (paned_box,
                                              0,
                                              area.x,
                                              area.y,
                                              area.width,
                                              area.height,
                                              DROP_HIGHLIGHT_OPACITY_ACTIVE);
    }
  else
    {
      gimp_paned_box_hide_drop_indicator (paned_box, 0);
    }

  /* Save the insert index for drag-drop */
  paned_box->p->insert_index = insert_index;

  gdk_drag_status (context, handle ? GDK_ACTION_MOVE : 0, time);

  /* Return TRUE so drag_leave() is called */
  return handle;
}

static gboolean
gimp_paned_box_drag_drop (GtkWidget      *widget,
                          GdkDragContext *context,
                          gint            x,
                          gint            y,
                          guint           time)
{
  GimpPanedBox *paned_box = GIMP_PANED_BOX (widget);
  gboolean      dropped   = FALSE;

  if (gimp_paned_box_will_handle_drag (paned_box->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      return FALSE;
    }

  if (paned_box->p->dropped_cb)
    {
      GtkWidget *source = gtk_drag_get_source_widget (context);

      if (source)
        dropped = paned_box->p->dropped_cb (source,
                                            paned_box->p->insert_index,
                                            paned_box->p->dropped_cb_data);
    }

  gtk_drag_finish (context, dropped, TRUE, time);

  return TRUE;
}

static gboolean
gimp_paned_box_drag_callback_idle (GimpPanedBox *paned_box)
{
  GtkAllocation  allocation;
  GtkOrientation orientation;
  GeglRectangle  area;

  paned_box->p->dnd_idle_id = 0;

  gtk_widget_get_allocation (GTK_WIDGET (paned_box), &allocation);
  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (paned_box));

  #define ADD_AREA(index, left, top)               \
    if (gimp_paned_box_get_handle_drag (           \
        paned_box,                                 \
        paned_box->p->dnd_context,                 \
        (left), (top),                             \
        0,                                         \
        NULL, &area))                              \
      {                                            \
        gimp_paned_box_position_drop_indicator (   \
          paned_box,                               \
          index,                                   \
          area.x, area.y, area.width, area.height, \
          DROP_HIGHLIGHT_OPACITY_INACTIVE);        \
      }

  if (! paned_box->p->widgets)
    {
      ADD_AREA (1, allocation.width / 2, allocation.height / 2)
    }
  else if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      ADD_AREA (1, 0,                    allocation.height / 2)
      ADD_AREA (2, allocation.width - 1, allocation.height / 2)
    }
  else
    {
      ADD_AREA (1, allocation.width / 2, 0)
      ADD_AREA (2, allocation.width / 2, allocation.height - 1)
    }

  #undef ADD_AREA

  return G_SOURCE_REMOVE;
}

static void
gimp_paned_box_drag_callback (GdkDragContext *context,
                              gboolean        begin,
                              GimpPanedBox   *paned_box)
{
  GtkWidget *paned;
  gint       position;

  if (! gtk_widget_get_sensitive (GTK_WIDGET (paned_box)))
    return;

  paned = gtk_widget_get_ancestor (GTK_WIDGET (paned_box),
                                   GTK_TYPE_PANED);

  if (begin)
    {
      paned_box->p->dnd_context = context;

      if (paned)
        {
          GtkAllocation allocation;

          gtk_widget_get_allocation (paned, &allocation);

          position = gtk_paned_get_position (GTK_PANED (paned));

          paned_box->p->dnd_paned_position = position;

          if (position < 0)
            {
              position = 0;
            }
          else if (gtk_widget_is_ancestor (
                     GTK_WIDGET (paned_box),
                     gtk_paned_get_child2 (GTK_PANED (paned))))
            {
              position = allocation.width - position;
            }

          if (position < DROP_HIGHLIGHT_MIN_SIZE)
            {
              position = DROP_HIGHLIGHT_MIN_SIZE;

              if (gtk_widget_is_ancestor (
                    GTK_WIDGET (paned_box),
                    gtk_paned_get_child2 (GTK_PANED (paned))))
                {
                  position = allocation.width - position;
                }

              gtk_paned_set_position (GTK_PANED (paned), position);
            }
        }

      paned_box->p->dnd_idle_id = g_idle_add (
        (GSourceFunc) gimp_paned_box_drag_callback_idle,
        paned_box);
    }
  else
    {
      if (paned_box->p->dnd_idle_id)
        {
          g_source_remove (paned_box->p->dnd_idle_id);

          paned_box->p->dnd_idle_id = 0;
        }

      paned_box->p->dnd_context = NULL;

      gimp_paned_box_hide_drop_indicator (paned_box, 1);
      gimp_paned_box_hide_drop_indicator (paned_box, 2);

      if (paned)
        {
          gtk_paned_set_position (GTK_PANED (paned),
                                  paned_box->p->dnd_paned_position);
        }
    }
}


GtkWidget *
gimp_paned_box_new (gboolean       homogeneous,
                    gint           spacing,
                    GtkOrientation orientation)
{
  return g_object_new (GIMP_TYPE_PANED_BOX,
                       "homogeneous",  homogeneous,
                       "spacing",      0,
                       "orientation",  orientation,
                       NULL);
}

void
gimp_paned_box_set_dropped_cb (GimpPanedBox            *paned_box,
                               GimpPanedBoxDroppedFunc  dropped_cb,
                               gpointer                 dropped_cb_data)
{
  g_return_if_fail (GIMP_IS_PANED_BOX (paned_box));

  paned_box->p->dropped_cb      = dropped_cb;
  paned_box->p->dropped_cb_data = dropped_cb_data;
}

/**
 * gimp_paned_box_add_widget:
 * @paned_box: A #GimpPanedBox
 * @widget:    The #GtkWidget to add
 * @index:     Where to add the @widget
 *
 * Add a #GtkWidget to the #GimpPanedBox in a hierarchy of #GtkPaned:s
 * so the space can be manually distributed between the widgets.
 **/
void
gimp_paned_box_add_widget (GimpPanedBox *paned_box,
                           GtkWidget    *widget,
                           gint          index)
{
  gint old_length = 0;

  g_return_if_fail (GIMP_IS_PANED_BOX (paned_box));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  GIMP_LOG (DND, "Adding GtkWidget %p to GimpPanedBox %p", widget, paned_box);

  /* Calculate length */
  old_length = g_list_length (paned_box->p->widgets);

  /* If index is invalid append at the end */
  if (index >= old_length || index < 0)
    {
      index = old_length;
    }

  /* Insert into the list */
  paned_box->p->widgets = g_list_insert (paned_box->p->widgets, widget, index);

  /* Hook us in for drag events. We could abstract this but it doesn't
   * seem worth it at this point
   */
  gimp_paned_box_set_widget_drag_handler (widget, paned_box);

  /* Insert into the GtkPaned hierarchy */
  if (old_length == 0)
    {
      gtk_box_pack_start (GTK_BOX (paned_box), widget, TRUE, TRUE, 0);
    }
  else
    {
      GtkWidget      *old_widget;
      GtkWidget      *parent;
      GtkWidget      *paned;
      GtkOrientation  orientation;

      /* Figure out what widget to detach */
      if (index == 0)
        {
          old_widget = g_list_nth_data (paned_box->p->widgets, index + 1);
        }
      else
        {
          old_widget = g_list_nth_data (paned_box->p->widgets, index - 1);
        }

      parent = gtk_widget_get_parent (old_widget);

      if (old_length > 1 && index > 0)
        {
          GtkWidget *grandparent = gtk_widget_get_parent (parent);

          old_widget = parent;
          parent     = grandparent;
        }

      /* Detach the widget and build up a new hierarchy */
      g_object_ref (old_widget);
      gtk_container_remove (GTK_CONTAINER (parent), old_widget);

      /* GtkPaned is abstract :( */
      orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (paned_box));
      paned = gtk_paned_new (orientation);

      if (GTK_IS_PANED (parent))
        {
          gtk_paned_pack1 (GTK_PANED (parent), paned, TRUE, FALSE);
        }
      else
        {
          gtk_box_pack_start (GTK_BOX (parent), paned, TRUE, TRUE, 0);
        }
      gtk_widget_show (paned);

      if (index == 0)
        {
          gtk_paned_pack1 (GTK_PANED (paned), widget,
                           TRUE, FALSE);
          gtk_paned_pack2 (GTK_PANED (paned), old_widget,
                           TRUE, FALSE);
        }
      else
        {
          gtk_paned_pack1 (GTK_PANED (paned), old_widget,
                           TRUE, FALSE);
          gtk_paned_pack2 (GTK_PANED (paned), widget,
                           TRUE, FALSE);
        }

      g_object_unref (old_widget);
    }
}

/**
 * gimp_paned_box_remove_widget:
 * @paned_box: A #GimpPanedBox
 * @widget:    The #GtkWidget to remove
 *
 * Remove a #GtkWidget from a #GimpPanedBox added with
 * gimp_widgets_add_paned_widget().
 **/
void
gimp_paned_box_remove_widget (GimpPanedBox *paned_box,
                              GtkWidget    *widget)
{
  gint       old_length   = 0;
  gint       index        = 0;
  GtkWidget *other_widget = NULL;
  GtkWidget *parent       = NULL;
  GtkWidget *grandparent  = NULL;

  g_return_if_fail (GIMP_IS_PANED_BOX (paned_box));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  GIMP_LOG (DND, "Removing GtkWidget %p from GimpPanedBox %p", widget, paned_box);

  /* Calculate length and index */
  old_length = g_list_length (paned_box->p->widgets);
  index      = g_list_index (paned_box->p->widgets, widget);

  /* Remove from list */
  paned_box->p->widgets = g_list_remove (paned_box->p->widgets, widget);

  /* Reset the drag events hook */
  gimp_paned_box_set_widget_drag_handler (widget, NULL);

  /* Remove from widget hierarchy */
  if (old_length == 1)
    {
      /* The widget might already be parent-less if we are in
       * destruction, .e.g when closing a dock window.
       */
      if (gtk_widget_get_parent (widget) != NULL)
        gtk_container_remove (GTK_CONTAINER (paned_box), widget);
    }
  else
    {
      g_object_ref (widget);

      parent      = gtk_widget_get_parent (GTK_WIDGET (widget));
      grandparent = gtk_widget_get_parent (parent);

      if (index == 0)
        other_widget = gtk_paned_get_child2 (GTK_PANED (parent));
      else
        other_widget = gtk_paned_get_child1 (GTK_PANED (parent));

      g_object_ref (other_widget);

      gtk_container_remove (GTK_CONTAINER (parent), other_widget);
      gtk_container_remove (GTK_CONTAINER (parent), GTK_WIDGET (widget));

      gtk_container_remove (GTK_CONTAINER (grandparent), parent);

      if (GTK_IS_PANED (grandparent))
        gtk_paned_pack1 (GTK_PANED (grandparent), other_widget, TRUE, FALSE);
      else
        gtk_box_pack_start (GTK_BOX (paned_box), other_widget, TRUE, TRUE, 0);

      g_object_unref (other_widget);

      g_object_unref (widget);
    }
}

/**
 * gimp_paned_box_will_handle_drag:
 * @paned_box: A #GimpPanedBox
 * @widget:    The widget that got the drag event
 * @context:   Context from drag event
 * @x:         x from drag event
 * @y:         y from drag event
 * @time:      time from drag event
 *
 * Returns: %TRUE if the drag event on @widget will be handled by
 *          @paned_box.
 **/
gboolean
gimp_paned_box_will_handle_drag (GimpPanedBox   *paned_box,
                                 GtkWidget      *widget,
                                 GdkDragContext *context,
                                 gint            x,
                                 gint            y,
                                 gint            time)
{
  gint           paned_box_x    = 0;
  gint           paned_box_y    = 0;
  GtkAllocation  allocation     = { 0, };
  GtkOrientation orientation    = 0;
  gboolean       will_handle    = FALSE;
  gint           drop_area_size = 0;

  g_return_val_if_fail (paned_box == NULL ||
                        GIMP_IS_PANED_BOX (paned_box), FALSE);

  /* Check for NULL to allow cleaner client code */
  if (paned_box == NULL)
    return FALSE;

  /* Our handler might handle it */
  if (gimp_paned_box_will_handle_drag (paned_box->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      /* Return TRUE so the client will pass on the drag event */
      return TRUE;
    }

  /* If we don't have a common ancenstor we will not handle it */
  if (! gtk_widget_translate_coordinates (widget,
                                          GTK_WIDGET (paned_box),
                                          x, y,
                                          &paned_box_x, &paned_box_y))
    {
      /* Return FALSE so the client can take care of the drag event */
      return FALSE;
    }

  /* We now have paned_box coordinates, see if the paned_box will
   * handle the event
   */
  gtk_widget_get_allocation (GTK_WIDGET (paned_box), &allocation);
  orientation    = gtk_orientable_get_orientation (GTK_ORIENTABLE (paned_box));
  drop_area_size = gimp_paned_box_get_drop_area_size (paned_box);
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      will_handle = (paned_box_x < drop_area_size ||
                     paned_box_x > allocation.width - drop_area_size);
    }
  else /*if (orientation = GTK_ORIENTATION_VERTICAL)*/
    {
      will_handle = (paned_box_y < drop_area_size ||
                     paned_box_y > allocation.height - drop_area_size);
    }

  return will_handle;
}

void
gimp_paned_box_set_drag_handler (GimpPanedBox *paned_box,
                                 GimpPanedBox *drag_handler)
{
  g_return_if_fail (GIMP_IS_PANED_BOX (paned_box));

  paned_box->p->drag_handler = drag_handler;
}
