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
#include "indicator_area.h"
#include "interface.h"
#include "gimpbrushlist.h"
#include "gimpbrushpreview.h"
#include "gimppatternpreview.h"

#define CELL_SIZE 23 /* The size of the previews */
#define CELL_PADDING 2 /* How much between brush and pattern cells */

/*  Static variables  */
static GtkWidget *indicator_table;
static GtkWidget *brush_preview;
static GtkWidget *pattern_preview;


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
      g_warning("No gimp brush found\n");
      return;
    }
  gimp_brush_preview_update (GIMP_BRUSH_PREVIEW (brush_preview), brush);
}

static gint
brush_preview_clicked (GtkWidget *widget, 
		       gpointer   data)
{
  create_brush_dialog();
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
      g_warning("No gimp pattern found\n");
      return;
    }
  gimp_pattern_preview_update (GIMP_PATTERN_PREVIEW (pattern_preview), pattern);
}

static gint
pattern_preview_clicked (GtkWidget *widget, 
			 gpointer   data)
{
  create_pattern_dialog(); 
  return TRUE;
}

GtkWidget *
indicator_area_create (int width,
		       int height)
{
  indicator_table = gtk_table_new (1, 3, FALSE);
  
  brush_preview = gimp_brush_preview_new (CELL_SIZE, CELL_SIZE);
  gimp_brush_preview_set_tooltips (GIMP_BRUSH_PREVIEW (brush_preview), tool_tips);
  gtk_signal_connect (GTK_OBJECT (brush_preview), "clicked",
                     (GtkSignalFunc) brush_preview_clicked, NULL);
  gtk_table_attach_defaults (GTK_TABLE(indicator_table), brush_preview,
                            0, 1, 0, 1);
                            
  pattern_preview = gimp_pattern_preview_new (CELL_SIZE, CELL_SIZE);
  gimp_pattern_preview_set_tooltips (GIMP_PATTERN_PREVIEW (pattern_preview), tool_tips);
  gtk_signal_connect (GTK_OBJECT (pattern_preview), "clicked",
                     (GtkSignalFunc) pattern_preview_clicked, NULL);
  gtk_table_attach_defaults (GTK_TABLE(indicator_table), pattern_preview,
			     1, 2, 0, 1);

  brush_area_update ();
  pattern_area_update ();

  gtk_widget_show (brush_preview);
  gtk_widget_show (pattern_preview);
  gtk_widget_show (indicator_table);

  return (indicator_table);
}


