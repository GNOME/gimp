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
#include "gimpbrushlist.h"
#include "gimpcontextpreview.h"
#include "gradient.h"
#include "gradient_header.h"
#include "indicator_area.h"
#include "interface.h"       /*  for tool_tips  */
#include "libgimp/gimpintl.h"

#define CELL_SIZE 23         /*  The size of the previews  */
#define GRAD_CELL_WIDTH 48   /*  The width of the gradient preview  */
#define GRAD_CELL_HEIGHT 12  /*  The height of the gradient preview  */
#define CELL_PADDING 2       /*  How much between brush and pattern cells  */

/*  Static variables  */
static GtkWidget *brush_preview;
static GtkWidget *pattern_preview;
static GtkWidget *gradient_preview;


/* The _area_update () functions should be called _preview_update(),
   but I've left the old function names in for now since they are
   called from devices.c
 */

void
brush_area_update ()
{
  GimpBrush *brush;

  if (no_data || no_interface) return;

  brush = get_active_brush();
  if (!brush) 
    {
      g_warning ("No gimp brush found\n");
      return;
    }
  gimp_context_preview_update (GIMP_CONTEXT_PREVIEW (brush_preview), brush);
}

static gint
brush_preview_clicked (GtkWidget *widget, 
		       gpointer   data)
{
  create_brush_dialog ();
  return TRUE;
}

void
pattern_area_update ()
{
  GPattern *pattern;

  if (no_data || no_interface) return;

  pattern = get_active_pattern();
  
  if (!pattern) 
    {
      g_warning ("No gimp pattern found\n");
      return;
    }
  gimp_context_preview_update (GIMP_CONTEXT_PREVIEW (pattern_preview), pattern);
}

static gint
pattern_preview_clicked (GtkWidget *widget, 
			 gpointer   data)
{
  create_pattern_dialog(); 
  return TRUE;
}

void
gradient_area_update ()
{
  gimp_context_preview_update (GIMP_CONTEXT_PREVIEW (gradient_preview), curr_gradient);
}

static gint
gradient_preview_clicked (GtkWidget *widget, 
			  gpointer   data)
{
  grad_create_gradient_editor ();
  return TRUE;
}

GtkWidget *
indicator_area_create ()
{
  GtkWidget *indicator_table;

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
                     (GtkSignalFunc) brush_preview_clicked, NULL);
  gtk_table_attach_defaults (GTK_TABLE(indicator_table), brush_preview,
                            0, 1, 0, 1);
                            
  pattern_preview = gimp_context_preview_new (GCP_PATTERN, 
					      CELL_SIZE, CELL_SIZE, 
					      TRUE, FALSE, FALSE);
  gtk_tooltips_set_tip (tool_tips, pattern_preview, 
			_("The active pattern.\nClick to open the Patterns Dialog."), 
			NULL);
  gtk_signal_connect (GTK_OBJECT (pattern_preview), "clicked",
                     (GtkSignalFunc) pattern_preview_clicked, NULL);
  gtk_table_attach_defaults (GTK_TABLE(indicator_table), pattern_preview,
			     1, 2, 0, 1);

  gradient_preview = gimp_context_preview_new (GCP_GRADIENT, 
					       GRAD_CELL_WIDTH, GRAD_CELL_HEIGHT, 
					       TRUE, FALSE, FALSE);
  gtk_tooltips_set_tip (tool_tips, gradient_preview, 
			_("The active gradient.\nClick to open the Gradients Dialog."), 
			NULL);
  gtk_signal_connect (GTK_OBJECT (gradient_preview), "clicked",
                     (GtkSignalFunc) gradient_preview_clicked, NULL);
  gtk_table_attach_defaults (GTK_TABLE(indicator_table), gradient_preview,
			     0, 2, 1, 2);

  brush_area_update ();
  pattern_area_update ();
  gradient_area_update ();

  gtk_widget_show (brush_preview);
  gtk_widget_show (pattern_preview);
  gtk_widget_show (gradient_preview);
  gtk_widget_show (indicator_table);

  return (indicator_table);
}


