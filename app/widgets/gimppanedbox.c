/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppanedbox.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 * Copyright (C)      2009 Martin Nordholts <martinn@src.gnome.org>
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

#include "gimpdockseparator.h"
#include "gimppanedbox.h"


struct _GimpPanedBoxPrivate
{
  /* Widgets that are separated by panes */
  GList     *widgets;

  /* Supports drag & drop rearrangement of widgets */
  GtkWidget *first_separator;
  GtkWidget *last_separator;
};


static void      gimp_paned_box_finalize          (GObject               *object);


G_DEFINE_TYPE (GimpPanedBox, gimp_paned_box, GTK_TYPE_BOX)

#define parent_class gimp_paned_box_parent_class


/* Keep the list of instance for gimp_dock_class_show_separators() */
static GList *paned_box_instances = NULL;


static void
gimp_paned_box_class_init (GimpPanedBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_paned_box_finalize;

  g_type_class_add_private (klass, sizeof (GimpPanedBoxPrivate));
}

static void
gimp_paned_box_init (GimpPanedBox *paned_box)
{
  paned_box->p = G_TYPE_INSTANCE_GET_PRIVATE (paned_box,
                                              GIMP_TYPE_PANED_BOX,
                                              GimpPanedBoxPrivate);

  paned_box->p->first_separator = gimp_dock_separator_new (GTK_ANCHOR_NORTH);
  paned_box->p->last_separator = gimp_dock_separator_new (GTK_ANCHOR_SOUTH);

  gtk_box_pack_start (GTK_BOX (paned_box), paned_box->p->first_separator,
                      FALSE, FALSE, 0);
  gtk_box_pack_end (GTK_BOX (paned_box), paned_box->p->last_separator,
                    FALSE, FALSE, 0);

  paned_box_instances = g_list_prepend (paned_box_instances, paned_box);
}

static void
gimp_paned_box_finalize (GObject *object)
{
  paned_box_instances = g_list_remove (paned_box_instances, object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
gimp_paned_box_set_dropped_cb (GimpPanedBox                 *paned_box,
                               GimpDockSeparatorDroppedFunc  dropped_cb,
                               gpointer                      dropped_cb_data)
{
  g_return_if_fail (GIMP_IS_PANED_BOX (paned_box));

  gimp_dock_separator_set_dropped_cb (GIMP_DOCK_SEPARATOR (paned_box->p->first_separator),
                                      dropped_cb,
                                      dropped_cb_data);
  gimp_dock_separator_set_dropped_cb (GIMP_DOCK_SEPARATOR (paned_box->p->last_separator),
                                      dropped_cb,
                                      dropped_cb_data);
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

  /* Calculate length */
  old_length = g_list_length (paned_box->p->widgets);

  /* If index is invalid append at the end */
  if (index >= old_length || index < 0)
    {
      index = old_length;
    }

  /* Insert into the list */
  paned_box->p->widgets = g_list_insert (paned_box->p->widgets, widget, index);

  /* Insert into the GtkPaned hierarchy */
  if (old_length == 0)
    {
      gtk_box_pack_start (GTK_BOX (paned_box), widget, TRUE, TRUE, 0);

      /* Keep the desired widget at the end */
      if (paned_box->p->last_separator)
        gtk_box_reorder_child (GTK_BOX (paned_box),
                               paned_box->p->last_separator,
                               -1);
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

      /* Deatch the widget and bulid up a new hierarchy */
      g_object_ref (old_widget);
      gtk_container_remove (GTK_CONTAINER (parent), old_widget);

      /* GtkPaned is abstract :( */
      orientation = gtk_orientable_get_orientation (GTK_ORIENTABLE (paned_box));
      paned = (orientation == GTK_ORIENTATION_VERTICAL ?
               gtk_vpaned_new () :
               gtk_hpaned_new ());

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

  /* Calculate length and index */
  old_length = g_list_length (paned_box->p->widgets);
  index      = g_list_index (paned_box->p->widgets, widget);

  /* Remove from list */
  paned_box->p->widgets = g_list_remove (paned_box->p->widgets, widget);

  /* Remove from widget hierarchy */
  if (old_length == 1)
    {
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
    }
}

void
gimp_paned_box_show_separators (GimpPanedBox *paned_box,
                                gboolean      show)
{
  if (show)
    {
      gtk_widget_show (paned_box->p->first_separator);

      /* Only show the south separator if there are any widgets */
      if (g_list_length (paned_box->p->widgets) > 0)
        gtk_widget_show (paned_box->p->last_separator);
    }
  else /* (! show) */
    {
      /* Hide separators unconditionally so we can handle the case
       * where we remove the last dockbook while separators are shown
       */
      gtk_widget_hide (paned_box->p->first_separator);
      gtk_widget_hide (paned_box->p->last_separator);
    }
}


/**
 * gimp_dock_class_show_separators:
 * @klass:
 * @show:
 *
 * Show/hide the separators in all docks.
 **/
void
gimp_paned_box_class_show_separators (GimpPanedBoxClass *klass,
                                      gboolean           show)
{
  GList *list;

  /* Conceptually this is a class varaible */
  g_return_if_fail (GIMP_IS_PANED_BOX_CLASS (klass));

  for (list = paned_box_instances; list != NULL; list = list->next)
    {
      GimpPanedBox *paned_box = GIMP_PANED_BOX (list->data);
      gimp_paned_box_show_separators (paned_box, show);
    }
}
