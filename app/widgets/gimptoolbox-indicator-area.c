/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpgradient.h"
#include "core/gimppattern.h"

#include "gimpdnd.h"
#include "gimphelp-ids.h"
#include "gimptoolbox.h"
#include "gimptoolbox-indicator-area.h"
#include "gimpview.h"
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


#define CELL_SPACING 2  /*  How much between brush and pattern cells  */


static void
brush_preview_clicked (GtkWidget       *widget,
                       GdkModifierType  state,
                       GimpToolbox     *toolbox)
{
  GimpContext *context = gimp_toolbox_get_context (toolbox);

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (context->gimp)),
                                             context->gimp,
                                             gimp_dock_get_dialog_factory (GIMP_DOCK (toolbox)),
                                             gimp_widget_get_monitor (widget),
                                             "gimp-brush-grid|gimp-brush-list");
}

static void
brush_preview_drop_brush (GtkWidget    *widget,
                          gint          x,
                          gint          y,
                          GimpViewable *viewable,
                          gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  gimp_context_set_brush (context, GIMP_BRUSH (viewable));
}

static void
pattern_preview_clicked (GtkWidget       *widget,
                         GdkModifierType  state,
                         GimpToolbox     *toolbox)
{
  GimpContext *context = gimp_toolbox_get_context (toolbox);

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (context->gimp)),
                                             context->gimp,
                                             gimp_dock_get_dialog_factory (GIMP_DOCK (toolbox)),
                                             gimp_widget_get_monitor (widget),
                                             "gimp-pattern-grid|gimp-pattern-list");
}

static void
pattern_preview_drop_pattern (GtkWidget    *widget,
                              gint          x,
                              gint          y,
                              GimpViewable *viewable,
                              gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  gimp_context_set_pattern (context, GIMP_PATTERN (viewable));
}

static void
gradient_preview_clicked (GtkWidget       *widget,
                          GdkModifierType  state,
                          GimpToolbox     *toolbox)
{
  GimpContext *context = gimp_toolbox_get_context (toolbox);

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (context->gimp)),
                                             context->gimp,
                                             gimp_dock_get_dialog_factory (GIMP_DOCK (toolbox)),
                                             gimp_widget_get_monitor (widget),
                                             "gimp-gradient-list|gimp-gradient-grid");
}

static void
gradient_preview_drop_gradient (GtkWidget    *widget,
                                gint          x,
                                gint          y,
                                GimpViewable *viewable,
                                gpointer      data)
{
  GimpContext *context = GIMP_CONTEXT (data);

  gimp_context_set_gradient (context, GIMP_GRADIENT (viewable));
}


/*  public functions  */

GtkWidget *
gimp_toolbox_indicator_area_create (GimpToolbox *toolbox)
{
  GimpContext *context;
  GtkWidget   *grid;
  GtkWidget   *brush_view;
  GtkWidget   *pattern_view;
  GtkWidget   *gradient_view;
  GtkIconSize  tool_icon_size;
  gint         pixel_size;
  gint         gradient_width;

  g_return_val_if_fail (GIMP_IS_TOOLBOX (toolbox), NULL);

  context = gimp_toolbox_get_context (toolbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), CELL_SPACING);
  gtk_grid_set_column_spacing (GTK_GRID (grid), CELL_SPACING);

  gimp_help_set_help_data (grid, NULL, GIMP_HELP_TOOLBOX_INDICATOR_AREA);

  gtk_widget_style_get (GTK_WIDGET (toolbox),
                        "tool-icon-size", &tool_icon_size,
                        NULL);
  gtk_icon_size_lookup (tool_icon_size, &pixel_size, NULL);
  gradient_width = pixel_size * (13/6.0f);

  /*  brush view  */

  brush_view =
    gimp_view_new_full_by_types (context,
                                 GIMP_TYPE_VIEW, GIMP_TYPE_BRUSH,
                                 pixel_size, pixel_size, 1,
                                 FALSE, TRUE, TRUE);
  gimp_view_set_viewable (GIMP_VIEW (brush_view),
                          GIMP_VIEWABLE (gimp_context_get_brush (context)));
  gtk_grid_attach (GTK_GRID (grid), brush_view, 0, 0, 1, 1);
  gtk_widget_set_visible (brush_view, TRUE);

  gimp_help_set_help_data (brush_view,
                           _("The active brush.\n"
                             "Click to open the Brush Dialog."), NULL);

  g_signal_connect_object (context, "brush-changed",
                           G_CALLBACK (gimp_view_set_viewable),
                           brush_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (brush_view, "clicked",
                    G_CALLBACK (brush_preview_clicked),
                    toolbox);

  gimp_dnd_viewable_dest_add (brush_view,
                              GIMP_TYPE_BRUSH,
                              brush_preview_drop_brush,
                              context);

  /*  pattern view  */

  pattern_view =
    gimp_view_new_full_by_types (context,
                                 GIMP_TYPE_VIEW, GIMP_TYPE_PATTERN,
                                 pixel_size, pixel_size, 1,
                                 FALSE, TRUE, TRUE);
  gimp_view_set_viewable (GIMP_VIEW (pattern_view),
                          GIMP_VIEWABLE (gimp_context_get_pattern (context)));

  gtk_grid_attach (GTK_GRID (grid), pattern_view, 1, 0, 1, 1);
  gtk_widget_set_visible (pattern_view, TRUE);

  gimp_help_set_help_data (pattern_view,
                           _("The active pattern.\n"
                             "Click to open the Pattern Dialog."), NULL);

  g_signal_connect_object (context, "pattern-changed",
                           G_CALLBACK (gimp_view_set_viewable),
                           pattern_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (pattern_view, "clicked",
                    G_CALLBACK (pattern_preview_clicked),
                    toolbox);

  gimp_dnd_viewable_dest_add (pattern_view,
                              GIMP_TYPE_PATTERN,
                              pattern_preview_drop_pattern,
                              context);

  /*  gradient view  */

  gradient_view =
    gimp_view_new_full_by_types (context,
                                 GIMP_TYPE_VIEW, GIMP_TYPE_GRADIENT,
                                 gradient_width, (pixel_size / 2), 1,
                                 FALSE, TRUE, TRUE);
  gimp_view_set_viewable (GIMP_VIEW (gradient_view),
                          GIMP_VIEWABLE (gimp_context_get_gradient (context)));

  gtk_grid_attach (GTK_GRID (grid), gradient_view, 0, 1, 2, 1);
  gtk_widget_set_visible (gradient_view, TRUE);

  gimp_help_set_help_data (gradient_view,
                           _("The active gradient.\n"
                             "Click to open the Gradient Dialog."), NULL);

  g_signal_connect_object (context, "gradient-changed",
                           G_CALLBACK (gimp_view_set_viewable),
                           gradient_view,
                           G_CONNECT_SWAPPED);

  g_signal_connect (gradient_view, "clicked",
                    G_CALLBACK (gradient_preview_clicked),
                    toolbox);

  gimp_dnd_viewable_dest_add (gradient_view,
                              GIMP_TYPE_GRADIENT,
                              gradient_preview_drop_gradient,
                              context);

  gtk_widget_set_visible (grid, TRUE);

  return grid;
}
