/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"
#include "widgets/widgets-types.h"

#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpgradient.h"
#include "core/gimppattern.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdnd.h"
#include "widgets/gimppreview.h"

#include "dialogs.h"
#include "indicator-area.h"

#include "libgimp/gimpintl.h"


#define CELL_SIZE        23  /*  The size of the previews  */
#define GRAD_CELL_WIDTH  48  /*  The width of the gradient preview  */
#define GRAD_CELL_HEIGHT 12  /*  The height of the gradient preview  */
#define CELL_PADDING      2  /*  How much between brush and pattern cells  */


/*  Static variables  */
static GtkWidget *brush_preview;
static GtkWidget *pattern_preview;
static GtkWidget *gradient_preview;


static void
brush_preview_clicked (GtkWidget *widget, 
		       gpointer   data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
				    "gimp:brush-grid");
}

static void
brush_preview_drop_brush (GtkWidget    *widget,
			  GimpViewable *viewable,
			  gpointer      data)
{
  GimpContext *context;

  context = GIMP_CONTEXT (data);

  gimp_context_set_brush (context, GIMP_BRUSH (viewable));
}

static void
pattern_preview_clicked (GtkWidget *widget, 
			 gpointer   data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
				    "gimp:pattern-grid");
}

static void
pattern_preview_drop_pattern (GtkWidget    *widget,
			      GimpViewable *viewable,
			      gpointer      data)
{
  GimpContext *context;

  context = GIMP_CONTEXT (data);

  gimp_context_set_pattern (context, GIMP_PATTERN (viewable));
}

static void
gradient_preview_clicked (GtkWidget *widget, 
			  gpointer   data)
{
  gimp_dialog_factory_dialog_raise (global_dock_factory,
				    "gimp:gradient-list");
}

static void
gradient_preview_drop_gradient (GtkWidget    *widget,
				GimpViewable *viewable,
				gpointer      data)
{
  GimpContext *context;

  context = GIMP_CONTEXT (data);

  gimp_context_set_gradient (context, GIMP_GRADIENT (viewable));
}

GtkWidget *
indicator_area_create (GimpContext *context)
{
  GtkWidget *indicator_table;

  g_return_val_if_fail (context != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  indicator_table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (indicator_table), 0, CELL_PADDING);
  gtk_table_set_col_spacing (GTK_TABLE (indicator_table), 0, CELL_PADDING);

  /*  brush preview  */

  brush_preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_brush (context)),
                           CELL_SIZE, CELL_SIZE, 0,
                           FALSE, TRUE, TRUE);
  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "brush_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (brush_preview));

  gimp_help_set_help_data (brush_preview, 
			   _("The active brush.\n"
			     "Click to open the Brushes Dialog."), NULL);

  gtk_signal_connect (GTK_OBJECT (brush_preview), "clicked",
		      GTK_SIGNAL_FUNC (brush_preview_clicked),
		      NULL);
  gimp_gtk_drag_dest_set_by_type (brush_preview,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_BRUSH,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (brush_preview,
                              GIMP_TYPE_BRUSH,
                              brush_preview_drop_brush,
                              context);

  gtk_table_attach_defaults (GTK_TABLE (indicator_table), brush_preview,
			     0, 1, 0, 1);


  /*  pattern preview  */

  pattern_preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_pattern (context)),
                           CELL_SIZE, CELL_SIZE, 0,
                           FALSE, TRUE, TRUE);
  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "pattern_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (pattern_preview));

  gimp_help_set_help_data (pattern_preview, 
			   _("The active pattern.\n"
			     "Click to open the Patterns Dialog."), NULL);

  gtk_signal_connect (GTK_OBJECT (pattern_preview), "clicked",
		      GTK_SIGNAL_FUNC (pattern_preview_clicked),
		      NULL);
  gimp_gtk_drag_dest_set_by_type (pattern_preview,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_PATTERN,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (pattern_preview,
                              GIMP_TYPE_PATTERN,
                              pattern_preview_drop_pattern,
                              context);

  gtk_table_attach_defaults (GTK_TABLE (indicator_table), pattern_preview,
			     1, 2, 0, 1);


  /*  gradient preview  */

  gradient_preview =
    gimp_preview_new_full (GIMP_VIEWABLE (gimp_context_get_gradient (context)),
                           GRAD_CELL_WIDTH, GRAD_CELL_HEIGHT, 0,
                           FALSE, TRUE, TRUE);
  gtk_signal_connect_object_while_alive
    (GTK_OBJECT (context),
     "gradient_changed",
     GTK_SIGNAL_FUNC (gimp_preview_set_viewable),
     GTK_OBJECT (gradient_preview));

  gimp_help_set_help_data (gradient_preview, 
			   _("The active gradient.\n"
			     "Click to open the Gradients Dialog."), NULL);

  gtk_signal_connect (GTK_OBJECT (gradient_preview), "clicked",
		      GTK_SIGNAL_FUNC (gradient_preview_clicked),
		      NULL);
  gimp_gtk_drag_dest_set_by_type (gradient_preview,
                                  GTK_DEST_DEFAULT_ALL,
                                  GIMP_TYPE_GRADIENT,
                                  GDK_ACTION_COPY);
  gimp_dnd_viewable_dest_set (gradient_preview,
                              GIMP_TYPE_GRADIENT,
                              gradient_preview_drop_gradient,
                              context);

  gtk_table_attach_defaults (GTK_TABLE (indicator_table), gradient_preview,
			     0, 2, 1, 2);

  gtk_widget_show (brush_preview);
  gtk_widget_show (pattern_preview);
  gtk_widget_show (gradient_preview);
  gtk_widget_show (indicator_table);

  return indicator_table;
}
