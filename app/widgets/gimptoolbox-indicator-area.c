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
#include <stdlib.h>
#include <string.h>

#include "appenv.h"
#include "brush_select.h"
#include "gimpcontextpreview.h"
#include "gimpdnd.h"
#include "gradient.h"
#include "gradient_header.h"
#include "indicator_area.h"
#include "interface.h"       /*  for tool_tips  */
#include "pattern_select.h"

#include "libgimp/gimpintl.h"

#define CELL_SIZE 23         /*  The size of the previews  */
#define GRAD_CELL_WIDTH 48   /*  The width of the gradient preview  */
#define GRAD_CELL_HEIGHT 12  /*  The height of the gradient preview  */
#define CELL_PADDING 2       /*  How much between brush and pattern cells  */

/*  Static variables  */
static GtkWidget *brush_preview;
static GtkWidget *pattern_preview;
static GtkWidget *gradient_preview;

/*  dnd stuff  */
static GtkTargetEntry brush_area_target_table[] =
{
  GIMP_TARGET_BRUSH
};
static guint n_brush_area_targets = (sizeof (brush_area_target_table) /
				     sizeof (brush_area_target_table[0]));

static GtkTargetEntry pattern_area_target_table[] =
{
  GIMP_TARGET_PATTERN
};
static guint n_pattern_area_targets = (sizeof (pattern_area_target_table) /
				       sizeof (pattern_area_target_table[0]));

static GtkTargetEntry gradient_area_target_table[] =
{
  GIMP_TARGET_GRADIENT
};
static guint n_gradient_area_targets = (sizeof (gradient_area_target_table) /
					sizeof (gradient_area_target_table[0]));


static void
brush_area_update (GimpContext *context,
		   GimpBrush   *brush,
		   gpointer     data)
{
  if (brush) 
    gimp_context_preview_update (GIMP_CONTEXT_PREVIEW (brush_preview), brush);
}

static void
brush_preview_clicked (GtkWidget *widget, 
		       gpointer   data)
{
  brush_dialog_create ();
}

static void
brush_preview_drag_drop (GtkWidget      *widget,
			 GdkDragContext *context,
			 gint            x,
			 gint            y,
			 guint           time,
			 gpointer        data)
{
  GtkWidget *src;
  GimpBrush *brush;
 
  if ((src = gtk_drag_get_source_widget (context)))
    {
      brush = (GimpBrush *) gtk_object_get_data (GTK_OBJECT (src),
						 "gimp_brush");

      if (brush)
	gimp_context_set_brush (gimp_context_get_user (), brush);
    }
}

static void
pattern_area_update (GimpContext *context,
		     GPattern    *pattern,
		     gpointer     data)
{
  if (pattern) 
    gimp_context_preview_update (GIMP_CONTEXT_PREVIEW (pattern_preview),
				 pattern);
}

static void
pattern_preview_clicked (GtkWidget *widget, 
			 gpointer   data)
{
  pattern_dialog_create (); 
}

static void
pattern_preview_drag_drop (GtkWidget      *widget,
			   GdkDragContext *context,
			   gint            x,
			   gint            y,
			   guint           time,
			   gpointer        data)
{
  GtkWidget *src;
  GPattern  *pattern;
 
  if ((src = gtk_drag_get_source_widget (context)))
    {
      pattern = (GPattern *) gtk_object_get_data (GTK_OBJECT (src),
						  "gimp_pattern");

      if (pattern)
	gimp_context_set_pattern (gimp_context_get_user (), pattern);
    }
}

static void
gradient_area_update (GimpContext *context,
		      gradient_t  *gradient,
		      gpointer     data)
{
  if (gradient)
    gimp_context_preview_update (GIMP_CONTEXT_PREVIEW (gradient_preview),
				 gradient);
}

static void
gradient_preview_clicked (GtkWidget *widget, 
			  gpointer   data)
{
  grad_create_gradient_editor ();
}

GtkWidget *
indicator_area_create ()
{
  GimpContext *context;
  GtkWidget *indicator_table;

  context = gimp_context_get_user ();

  indicator_table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (indicator_table), 0, CELL_PADDING);
  gtk_table_set_col_spacing (GTK_TABLE (indicator_table), 0, CELL_PADDING);
  
  brush_preview = gimp_context_preview_new (GCP_BRUSH, 
					    CELL_SIZE, CELL_SIZE, 
					    TRUE, FALSE, FALSE);
  gtk_tooltips_set_tip (tool_tips, brush_preview, 
			_("The active brush.\nClick to open the Brushes Dialog."), 
			NULL);
  gtk_signal_connect (GTK_OBJECT (brush_preview), "clicked",
		      GTK_SIGNAL_FUNC (brush_preview_clicked),
		      NULL);
  gtk_drag_dest_set (brush_preview,
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     brush_area_target_table, n_brush_area_targets,
		     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (brush_preview), "drag_drop",
		      GTK_SIGNAL_FUNC (brush_preview_drag_drop),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (context), "brush_changed",
		      GTK_SIGNAL_FUNC (brush_area_update),
		      NULL);
  gtk_table_attach_defaults (GTK_TABLE (indicator_table), brush_preview,
			     0, 1, 0, 1);
                            
  pattern_preview = gimp_context_preview_new (GCP_PATTERN, 
					      CELL_SIZE, CELL_SIZE, 
					      TRUE, FALSE, FALSE);
  gtk_tooltips_set_tip (tool_tips, pattern_preview, 
			_("The active pattern.\n"
			  "Click to open the Patterns Dialog."), NULL);
  gtk_signal_connect (GTK_OBJECT (pattern_preview), "clicked",
		      GTK_SIGNAL_FUNC (pattern_preview_clicked),
		      NULL);
  gtk_drag_dest_set (pattern_preview,
		     GTK_DEST_DEFAULT_HIGHLIGHT |
		     GTK_DEST_DEFAULT_MOTION |
		     GTK_DEST_DEFAULT_DROP,
		     pattern_area_target_table, n_pattern_area_targets,
		     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (pattern_preview), "drag_drop",
		      GTK_SIGNAL_FUNC (pattern_preview_drag_drop),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (context), "pattern_changed",
		      GTK_SIGNAL_FUNC (pattern_area_update),
		      NULL);
  gtk_table_attach_defaults (GTK_TABLE (indicator_table), pattern_preview,
			     1, 2, 0, 1);

  gradient_preview = gimp_context_preview_new (GCP_GRADIENT, 
					       GRAD_CELL_WIDTH,
					       GRAD_CELL_HEIGHT, 
					       TRUE, FALSE, FALSE);
  gtk_tooltips_set_tip (tool_tips, gradient_preview, 
			_("The active gradient.\n"
			  "Click to open the Gradients Dialog."), 
			NULL);
  gtk_signal_connect (GTK_OBJECT (gradient_preview), "clicked",
		      GTK_SIGNAL_FUNC (gradient_preview_clicked),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (context), "gradient_changed",
		      GTK_SIGNAL_FUNC (gradient_area_update),
		      NULL);
  gtk_table_attach_defaults (GTK_TABLE (indicator_table), gradient_preview,
			     0, 2, 1, 2);

  brush_area_update (NULL, gimp_context_get_brush (context), NULL);
  pattern_area_update (NULL, gimp_context_get_pattern (context), NULL);
  gradient_area_update (NULL, gimp_context_get_gradient (context), NULL);

  gtk_widget_show (brush_preview);
  gtk_widget_show (pattern_preview);
  gtk_widget_show (gradient_preview);
  gtk_widget_show (indicator_table);

  return (indicator_table);
}


