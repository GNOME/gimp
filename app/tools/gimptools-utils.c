/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmalayer.h"

#include "vectors/ligmavectors.h"

#include "widgets/ligmacontainerview.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmaitemtreeview.h"
#include "widgets/ligmawidgets-utils.h"
#include "widgets/ligmawindowstrategy.h"

#include "ligmatools-utils.h"



/*  public functions  */

void
ligma_tools_blink_lock_box (Ligma     *ligma,
                           LigmaItem *item)
{
  GtkWidget        *dockable;
  LigmaItemTreeView *view;
  GdkMonitor       *monitor;
  const gchar      *identifier;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_ITEM (item));

  if (LIGMA_IS_LAYER (item))
    identifier = "ligma-layer-list";
  else if (LIGMA_IS_CHANNEL (item))
    identifier = "ligma-channel-list";
  else if (LIGMA_IS_VECTORS (item))
    identifier = "ligma-vectors-list";
  else
    return;

  monitor = ligma_get_monitor_at_pointer ();

  dockable = ligma_window_strategy_show_dockable_dialog (
    LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (ligma)),
    ligma,
    ligma_dialog_factory_get_singleton (),
    monitor,
    identifier);

  if (! dockable)
    return;

  view = LIGMA_ITEM_TREE_VIEW (gtk_bin_get_child (GTK_BIN (dockable)));
  ligma_item_tree_view_blink_lock (view, item);
}

void
ligma_tools_show_tool_options (Ligma *ligma)
{
  GdkMonitor *monitor;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  monitor = ligma_get_monitor_at_pointer ();

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (ligma)),
                                             ligma,
                                             ligma_dialog_factory_get_singleton (),
                                             monitor, "ligma-tool-options");
}
