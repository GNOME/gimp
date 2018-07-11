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
#include <gtk/gtk.h>

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
#include "gimpwidgets-utils.h"

#include "gimp-log.h"

#include "gimp-intl.h"


/**
 * Defines the size of the area that dockables can be dropped on in
 * order to be inserted and get space on their own (rather than
 * inserted among others and sharing space)
 */
#define DROP_AREA_SIZE            12

#define INSERT_INDEX_UNUSED       G_MININT

#define INSTRUCTIONS_TEXT_PADDING 6
#define INSTRUCTIONS_TEXT         _("You can drop dockable dialogs here")


struct _GimpPanedBoxPrivate
{
  /* Widgets that are separated by panes */
  GList                  *widgets;

  /* Displays INSTRUCTIONS_TEXT when there are no dockables */
  GtkWidget              *instructions;

  /* Is the DND highlight shown */
  gboolean                dnd_highlight;
  GdkRectangle            dnd_rectangle;

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
static void      gimp_paned_box_drag_data_received      (GtkWidget      *widget,
                                                         GdkDragContext *context,
                                                         gint            x,
                                                         gint            y,
                                                         GtkSelectionData *data,
                                                         guint           info,
                                                         guint           time);
static void      gimp_paned_box_set_widget_drag_handler (GtkWidget      *widget,
                                                         GimpPanedBox   *handler);
static gint      gimp_paned_box_get_drop_area_size      (GimpPanedBox   *paned_box);
static void      gimp_paned_box_hide_drop_indicator     (GimpPanedBox   *paned_box);


G_DEFINE_TYPE (GimpPanedBox, gimp_paned_box, GTK_TYPE_BOX)

#define parent_class gimp_paned_box_parent_class

static const GtkTargetEntry dialog_target_table[] = { GIMP_TARGET_NOTEBOOK_TAB };


static void
gimp_paned_box_class_init (GimpPanedBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose            = gimp_paned_box_dispose;

  widget_class->drag_leave         = gimp_paned_box_drag_leave;
  widget_class->drag_motion        = gimp_paned_box_drag_motion;
  widget_class->drag_drop          = gimp_paned_box_drag_drop;
  widget_class->drag_data_received = gimp_paned_box_drag_data_received;

  g_type_class_add_private (klass, sizeof (GimpPanedBoxPrivate));
}

static void
gimp_paned_box_init (GimpPanedBox *paned_box)
{
  paned_box->p = G_TYPE_INSTANCE_GET_PRIVATE (paned_box,
                                              GIMP_TYPE_PANED_BOX,
                                              GimpPanedBoxPrivate);

  /* Instructions label
   *
   * Size a small size request so it don't mess up dock window layouts
   * during startup; in particular, set its height request to 0 so it
   * doesn't contribute to the minimum height of the toolbox.
   */
  paned_box->p->instructions = gtk_label_new (INSTRUCTIONS_TEXT);
  g_object_set (paned_box->p->instructions,
                "margin-start",  INSTRUCTIONS_TEXT_PADDING,
                "margin-end",    INSTRUCTIONS_TEXT_PADDING,
                "margin-top",    INSTRUCTIONS_TEXT_PADDING,
                "margin-bottom", INSTRUCTIONS_TEXT_PADDING,
                NULL);
  gtk_label_set_line_wrap (GTK_LABEL (paned_box->p->instructions), TRUE);
  gtk_label_set_justify (GTK_LABEL (paned_box->p->instructions),
                         GTK_JUSTIFY_CENTER);
  gtk_widget_set_size_request (paned_box->p->instructions, 1, DROP_AREA_SIZE);
  gimp_label_set_attributes (GTK_LABEL (paned_box->p->instructions),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (paned_box), paned_box->p->instructions,
                      TRUE, TRUE, 0);
  gtk_widget_show (paned_box->p->instructions);


  /* Setup DND */
  gtk_drag_dest_set (GTK_WIDGET (paned_box),
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
}

static void
gimp_paned_box_dispose (GObject *object)
{
  GimpPanedBox *paned_box = GIMP_PANED_BOX (object);

  if (paned_box->p->dnd_highlight)
    gimp_paned_box_hide_drop_indicator (paned_box);

  while (paned_box->p->widgets)
    {
      GtkWidget *widget = paned_box->p->widgets->data;

      g_object_ref (widget);
      gimp_paned_box_remove_widget (paned_box, widget);
      gtk_widget_destroy (widget);
      g_object_unref (widget);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
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
gimp_paned_box_drop_indicator_draw (GtkWidget *widget,
                                    cairo_t   *cr,
                                    gpointer   data)
{
  GimpPanedBox *paned_box = GIMP_PANED_BOX (widget);

  cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 0.2);
  cairo_rectangle (cr,
                   paned_box->p->dnd_rectangle.x      + 0.5,
                   paned_box->p->dnd_rectangle.y      + 0.5,
                   paned_box->p->dnd_rectangle.width  - 1.0,
                   paned_box->p->dnd_rectangle.height - 1.0);
  cairo_fill (cr);

  return FALSE;
}

static void
gimp_paned_box_position_drop_indicator (GimpPanedBox *paned_box,
                                        gint          x,
                                        gint          y,
                                        gint          width,
                                        gint          height)
{
  GtkWidget *widget = GTK_WIDGET (paned_box);

  paned_box->p->dnd_rectangle.x      = x;
  paned_box->p->dnd_rectangle.y      = y;
  paned_box->p->dnd_rectangle.width  = width;
  paned_box->p->dnd_rectangle.height = height;

  if (! paned_box->p->dnd_highlight)
    {
      g_signal_connect_after (widget, "draw",
                              G_CALLBACK (gimp_paned_box_drop_indicator_draw),
                              NULL);
      paned_box->p->dnd_highlight = TRUE;
    }

  gtk_widget_queue_draw (widget);
}

static void
gimp_paned_box_hide_drop_indicator (GimpPanedBox *paned_box)
{
  GtkWidget *widget = GTK_WIDGET (paned_box);

  if (paned_box->p->dnd_highlight)
    {
      g_signal_handlers_disconnect_by_func (widget,
                                            gimp_paned_box_drop_indicator_draw,
                                            NULL);
      paned_box->p->dnd_highlight = FALSE;
    }

  gtk_widget_queue_draw (widget);
}

static void
gimp_paned_box_drag_leave (GtkWidget      *widget,
                           GdkDragContext *context,
                           guint           time)
{
  gimp_paned_box_hide_drop_indicator (GIMP_PANED_BOX (widget));
}

static gboolean
gimp_paned_box_drag_motion (GtkWidget      *widget,
                            GdkDragContext *context,
                            gint            x,
                            gint            y,
                            guint           time)
{
  GimpPanedBox  *paned_box    = GIMP_PANED_BOX (widget);
  gint           insert_index = INSERT_INDEX_UNUSED;
  gint           dnd_window_x;
  gint           dnd_window_y;
  gint           dnd_window_w;
  gint           dnd_window_h;
  GtkAllocation  allocation;
  GtkOrientation orientation;
  gboolean       handle;
  gint           drop_area_size;

  if (gimp_paned_box_will_handle_drag (paned_box->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      gdk_drag_status (context, 0, time);

      return FALSE;
    }

  drop_area_size = gimp_paned_box_get_drop_area_size (paned_box);

  gtk_widget_get_allocation (widget, &allocation);

  /* See if we're at the edge of the dock If there are no dockables,
   * the entire paned box is a drop area
   */
  orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (paned_box));

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      dnd_window_y = 0;
      dnd_window_h = allocation.height;

      /* If there are no widgets, the drop area is as big as the paned
       * box
       */
      if (! paned_box->p->widgets)
        dnd_window_w = allocation.width;
      else
        dnd_window_w = drop_area_size;

      if (x < drop_area_size)
        {
          insert_index = 0;
          dnd_window_x = 0;
        }

      if (x > allocation.width - drop_area_size)
        {
          insert_index = -1;
          dnd_window_x = allocation.width - drop_area_size;
        }
    }
  else /* if (orientation = GTK_ORIENTATION_VERTICAL) */
    {
      dnd_window_x = 0;
      dnd_window_w = allocation.width;

      /* If there are no widgets, the drop area is as big as the paned
       * box
       */
      if (! paned_box->p->widgets)
        dnd_window_h = allocation.height;
      else
        dnd_window_h = drop_area_size;

      if (y < drop_area_size)
        {
          insert_index = 0;
          dnd_window_y = 0;
        }

      if (y > allocation.height - drop_area_size)
        {
          insert_index = -1;
          dnd_window_y = allocation.height - drop_area_size;
        }
    }

  /* If we are at the edge, show a GdkWindow to communicate that a
   * drop will create a new dock column
   */
  handle = (insert_index != INSERT_INDEX_UNUSED);

  if (handle)
    {
      gimp_paned_box_position_drop_indicator (paned_box,
                                              dnd_window_x,
                                              dnd_window_y,
                                              dnd_window_w,
                                              dnd_window_h);
    }
  else
    {
      gimp_paned_box_hide_drop_indicator (paned_box);
    }

  /* Save the insert index for drag-drop */
  paned_box->p->insert_index = insert_index;

  gdk_drag_status (context, handle ? GDK_ACTION_MOVE : 0, time);

  /* Return TRUE so drag_leave() is called */
  return TRUE;
}

static gboolean
gimp_paned_box_drag_drop (GtkWidget      *widget,
                          GdkDragContext *context,
                          gint            x,
                          gint            y,
                          guint           time)
{
  GimpPanedBox *paned_box = GIMP_PANED_BOX (widget);
  GdkAtom       target;

  if (gimp_paned_box_will_handle_drag (paned_box->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      return FALSE;
    }

  target = gtk_drag_dest_find_target (widget, context, NULL);

  gtk_drag_get_data (widget, context, target, time);

  return TRUE;
}

static void
gimp_paned_box_drag_data_received (GtkWidget        *widget,
                                   GdkDragContext   *context,
                                   gint              x,
                                   gint              y,
                                   GtkSelectionData *data,
                                   guint             info,
                                   guint             time)
{
  GimpPanedBox  *paned_box = GIMP_PANED_BOX (widget);
  GtkWidget     *notebook  = gtk_drag_get_source_widget (context);
  GtkWidget    **child     = (gpointer) gtk_selection_data_get_data (data);
  gboolean      dropped    = FALSE;

  if (paned_box->p->dropped_cb)
    {
      dropped = paned_box->p->dropped_cb (notebook,
                                          *child,
                                          paned_box->p->insert_index,
                                          paned_box->p->dropped_cb_data);
    }

  gtk_drag_finish (context, dropped, TRUE, time);
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
      /* A widget is added, hide the instructions */
      gtk_widget_hide (paned_box->p->instructions);

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
      gtk_paned_set_wide_handle (GTK_PANED (paned), TRUE);

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

      /* The last widget is removed, show the instructions */
      gtk_widget_show (paned_box->p->instructions);
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
