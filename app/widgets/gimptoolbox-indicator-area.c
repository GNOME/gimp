/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmabrush.h"
#include "core/ligmacontext.h"
#include "core/ligmagradient.h"
#include "core/ligmapattern.h"

#include "ligmadnd.h"
#include "ligmahelp-ids.h"
#include "ligmatoolbox.h"
#include "ligmatoolbox-indicator-area.h"
#include "ligmaview.h"
#include "ligmawidgets-utils.h"
#include "ligmawindowstrategy.h"

#include "ligma-intl.h"


#define CELL_SIZE        24  /*  The size of the previews                  */
#define GRAD_CELL_WIDTH  52  /*  The width of the gradient preview         */
#define GRAD_CELL_HEIGHT 12  /*  The height of the gradient preview        */
#define CELL_SPACING      2  /*  How much between brush and pattern cells  */


static void
brush_preview_clicked (GtkWidget       *widget,
                       GdkModifierType  state,
                       LigmaToolbox     *toolbox)
{
  LigmaContext *context = ligma_toolbox_get_context (toolbox);

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (context->ligma)),
                                             context->ligma,
                                             ligma_dock_get_dialog_factory (LIGMA_DOCK (toolbox)),
                                             ligma_widget_get_monitor (widget),
                                             "ligma-brush-grid|ligma-brush-list");
}

static void
brush_preview_drop_brush (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          LigmaViewable *viewable,
                          gpointer      data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);

  ligma_context_set_brush (context, LIGMA_BRUSH (viewable));
}

static void
pattern_preview_clicked (GtkWidget       *widget,
                         GdkModifierType  state,
                         LigmaToolbox     *toolbox)
{
  LigmaContext *context = ligma_toolbox_get_context (toolbox);

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (context->ligma)),
                                             context->ligma,
                                             ligma_dock_get_dialog_factory (LIGMA_DOCK (toolbox)),
                                             ligma_widget_get_monitor (widget),
                                             "ligma-pattern-grid|ligma-pattern-list");
}

static void
pattern_preview_drop_pattern (GtkWidget    *widget,
                              gint          x,
                              gint          y,
                              LigmaViewable *viewable,
                              gpointer      data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);

  ligma_context_set_pattern (context, LIGMA_PATTERN (viewable));
}

static void
gradient_preview_clicked (GtkWidget       *widget,
                          GdkModifierType  state,
                          LigmaToolbox     *toolbox)
{
  LigmaContext *context = ligma_toolbox_get_context (toolbox);

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (context->ligma)),
                                             context->ligma,
                                             ligma_dock_get_dialog_factory (LIGMA_DOCK (toolbox)),
                                             ligma_widget_get_monitor (widget),
                                             "ligma-gradient-list|ligma-gradient-grid");
}

static void
gradient_preview_drop_gradient (GtkWidget    *widget,
                                gint          x,
                                gint          y,
                                LigmaViewable *viewable,
                                gpointer      data)
{
  LigmaContext *context = LIGMA_CONTEXT (data);

  ligma_context_set_gradient (context, LIGMA_GRADIENT (viewable));
}


/*  public functions  */

GtkWidget *
ligma_toolbox_indicator_area_create (LigmaToolbox *toolbox)
{
  LigmaContext *context;
  GtkWidget   *grid;
  GtkWidget   *brush_view;
  GtkWidget   *pattern_view;
  GtkWidget   *gradient_view;

  g_return_val_if_fail (LIGMA_IS_TOOLBOX (toolbox), NULL);

  context = ligma_toolbox_get_context (toolbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), CELL_SPACING);
  gtk_grid_set_column_spacing (GTK_GRID (grid), CELL_SPACING);

  ligma_help_set_help_data (grid, NULL, LIGMA_HELP_TOOLBOX_INDICATOR_AREA);

  /*  brush view  */

  brush_view =
    ligma_view_new_full_by_types (context,
                                 LIGMA_TYPE_VIEW, LIGMA_TYPE_BRUSH,
                                 CELL_SIZE, CELL_SIZE, 1,
                                 FALSE, TRUE, TRUE);
  ligma_view_set_viewable (LIGMA_VIEW (brush_view),
                          LIGMA_VIEWABLE (ligma_context_get_brush (context)));
  gtk_grid_attach (GTK_GRID (grid), brush_view, 0, 0, 1, 1);
  gtk_widget_show (brush_view);

  ligma_help_set_help_data (brush_view,
                           _("The active brush.\n"
                             "Click to open the Brush Dialog."), NULL);

  g_signal_connect_object (context, "brush-changed",
                           G_CALLBACK (ligma_view_set_viewable),
                           brush_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (brush_view, "clicked",
                    G_CALLBACK (brush_preview_clicked),
                    toolbox);

  ligma_dnd_viewable_dest_add (brush_view,
                              LIGMA_TYPE_BRUSH,
                              brush_preview_drop_brush,
                              context);

  /*  pattern view  */

  pattern_view =
    ligma_view_new_full_by_types (context,
                                 LIGMA_TYPE_VIEW, LIGMA_TYPE_PATTERN,
                                 CELL_SIZE, CELL_SIZE, 1,
                                 FALSE, TRUE, TRUE);
  ligma_view_set_viewable (LIGMA_VIEW (pattern_view),
                          LIGMA_VIEWABLE (ligma_context_get_pattern (context)));

  gtk_grid_attach (GTK_GRID (grid), pattern_view, 1, 0, 1, 1);
  gtk_widget_show (pattern_view);

  ligma_help_set_help_data (pattern_view,
                           _("The active pattern.\n"
                             "Click to open the Pattern Dialog."), NULL);

  g_signal_connect_object (context, "pattern-changed",
                           G_CALLBACK (ligma_view_set_viewable),
                           pattern_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (pattern_view, "clicked",
                    G_CALLBACK (pattern_preview_clicked),
                    toolbox);

  ligma_dnd_viewable_dest_add (pattern_view,
                              LIGMA_TYPE_PATTERN,
                              pattern_preview_drop_pattern,
                              context);

  /*  gradient view  */

  gradient_view =
    ligma_view_new_full_by_types (context,
                                 LIGMA_TYPE_VIEW, LIGMA_TYPE_GRADIENT,
                                 GRAD_CELL_WIDTH, GRAD_CELL_HEIGHT, 1,
                                 FALSE, TRUE, TRUE);
  ligma_view_set_viewable (LIGMA_VIEW (gradient_view),
                          LIGMA_VIEWABLE (ligma_context_get_gradient (context)));

  gtk_grid_attach (GTK_GRID (grid), gradient_view, 0, 1, 2, 1);
  gtk_widget_show (gradient_view);

  ligma_help_set_help_data (gradient_view,
                           _("The active gradient.\n"
                             "Click to open the Gradient Dialog."), NULL);

  g_signal_connect_object (context, "gradient-changed",
                           G_CALLBACK (ligma_view_set_viewable),
                           gradient_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (gradient_view, "clicked",
                    G_CALLBACK (gradient_preview_clicked),
                    toolbox);

  ligma_dnd_viewable_dest_add (gradient_view,
                              LIGMA_TYPE_GRADIENT,
                              gradient_preview_drop_gradient,
                              context);

  gtk_widget_show (grid);

  return grid;
}
