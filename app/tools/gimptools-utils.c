/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimplayer.h"

#include "vectors/gimpvectors.h"

#include "widgets/gimpcontainerview.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpitemtreeview.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"

#include "gimptools-utils.h"


static void gimp_tools_search_widget_rec (GtkWidget   *widget,
                                          const gchar *searched_id);


/*  public functions  */

void
gimp_tools_blink_lock_box (Gimp     *gimp,
                           GimpItem *item)
{
  GtkWidget        *dockable;
  GimpItemTreeView *view;
  GdkMonitor       *monitor;
  const gchar      *identifier;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_ITEM (item));

  if (GIMP_IS_LAYER (item))
    identifier = "gimp-layer-list";
  else if (GIMP_IS_CHANNEL (item))
    identifier = "gimp-channel-list";
  else if (GIMP_IS_VECTORS (item))
    identifier = "gimp-vectors-list";
  else
    return;

  monitor = gimp_get_monitor_at_pointer ();

  dockable = gimp_window_strategy_show_dockable_dialog (
    GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (gimp)),
    gimp,
    gimp_dialog_factory_get_singleton (),
    monitor,
    identifier);

  if (! dockable)
    return;

  view = GIMP_ITEM_TREE_VIEW (gtk_bin_get_child (GTK_BIN (dockable)));
  gimp_item_tree_view_blink_lock (view, item);
}


/**
 * gimp_tools_blink_property:
 * @gimp:
 * @dockable_identifier:
 * @widget_identifier:
 *
 * This function will raise the dockable identified by
 * @dockable_identifier. The list of dockable identifiers can be found
 * in `app/dialogs/dialogs.c`.
 *
 * Then it will find the widget identified by @widget_identifier. Note
 * that for propwidgets, this is usually the associated property name.
 * Finally it will blink this widget to raise attention.
 */
void
gimp_tools_blink_widget (Gimp        *gimp,
                         const gchar *dockable_identifier,
                         const gchar *widget_identifier)
{
  GtkWidget  *dockable;
  GdkMonitor *monitor;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  monitor = gimp_get_monitor_at_pointer ();

  dockable = gimp_window_strategy_show_dockable_dialog (
    GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (gimp)),
    gimp,
    gimp_dialog_factory_get_singleton (),
    monitor,
    dockable_identifier);

  if (! dockable)
    return;

  gtk_container_foreach (GTK_CONTAINER (dockable),
                         (GtkCallback) gimp_tools_search_widget_rec,
                         (gpointer) widget_identifier);
}


/*  private functions  */

static void
gimp_tools_search_widget_rec (GtkWidget   *widget,
                              const gchar *searched_id)
{
  if (gtk_widget_is_visible (widget))
    {
      const gchar *id;

      id = g_object_get_data (G_OBJECT (widget),
                              "gimp-widget-identifier");

      if (id && g_strcmp0 (id, searched_id) == 0)
        gimp_widget_blink (widget);
      else if (GTK_IS_CONTAINER (widget))
        gtk_container_foreach (GTK_CONTAINER (widget),
                               (GtkCallback) gimp_tools_search_widget_rec,
                               (gpointer) searched_id);
    }
}
