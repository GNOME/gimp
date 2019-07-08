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

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpitemtreeview.h"
#include "widgets/gimpwidgets-utils.h"
#include "widgets/gimpwindowstrategy.h"

#include "gimptools-utils.h"


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

  gimp_widget_blink (gimp_item_tree_view_get_lock_box (view));
}
