/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * brush_edit module Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include <string.h>

#include "appenv.h"
#include "gimpbrushgenerated.h"
#include "brush_edit.h"
#include "gimpui.h"

#include "libgimp/gimpmath.h"

#include "libgimp/gimpintl.h"


static void brush_edit_close_callback (GtkWidget                *widget,
				       gpointer                  data);
static gint brush_edit_preview_resize (GtkWidget                *widget,
				       GdkEvent                 *event, 
				       BrushEditGeneratedWindow *begw);


static void
update_brush_callback (GtkAdjustment            *adjustment,
		       BrushEditGeneratedWindow *begw)
{
  if (begw->brush &&
      ((begw->radius_data->value  
	!= gimp_brush_generated_get_radius (begw->brush))
       || (begw->hardness_data->value
	   != gimp_brush_generated_get_hardness (begw->brush))
       || (begw->aspect_ratio_data->value
	   != gimp_brush_generated_get_aspect_ratio (begw->brush))
       || (begw->angle_data->value
	   != gimp_brush_generated_get_angle (begw->brush))))
    {
      gimp_brush_generated_freeze (begw->brush);
      gimp_brush_generated_set_radius       (begw->brush,
					     begw->radius_data->value);
      gimp_brush_generated_set_hardness     (begw->brush,
					     begw->hardness_data->value);
      gimp_brush_generated_set_aspect_ratio (begw->brush,
					     begw->aspect_ratio_data->value);
      gimp_brush_generated_set_angle        (begw->brush,
					     begw->angle_data->value);
      gimp_brush_generated_thaw (begw->brush);
    }
}

static void
brush_edit_clear_preview (BrushEditGeneratedWindow *begw)
{
  guchar *buf;
  gint    i;

  buf = g_new (guchar, begw->preview->requisition.width);

  /*  Set the buffer to white  */
  memset (buf, 255, begw->preview->requisition.width);

  /*  Set the image buffer to white  */
  for (i = 0; i < begw->preview->requisition.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (begw->preview), buf, 0, i,
			  begw->preview->requisition.width);

  g_free (buf);
}

static gint 
brush_edit_brush_dirty_callback (GimpBrush                *brush,
				 BrushEditGeneratedWindow *begw)
{
  gint x, y, width, yend, ystart, xo;
  gint scale;
  gchar *src, *buf;

  brush_edit_clear_preview (begw);
  if (brush == NULL || brush->mask == NULL)
    return TRUE;
  scale = MAX (ceil (brush->mask->width/
		     (float) begw->preview->requisition.width),
	       ceil (brush->mask->height/
		     (float) begw->preview->requisition.height));

  ystart = 0;
  xo = begw->preview->requisition.width/2 - brush->mask->width/(2*scale);
  ystart = begw->preview->requisition.height/2 - brush->mask->height/(2*scale);
  yend = ystart + brush->mask->height/(scale);
  width = CLAMP (brush->mask->width/scale, 0, begw->preview->requisition.width);

  buf = g_new (gchar, width);
  src = (gchar *) temp_buf_data (brush->mask);

  for (y = ystart; y < yend; y++)
    {
      /*  Invert the mask for display.
       */
      for (x = 0; x < width; x++)
	buf[x] = 255 - src[x*scale];
      gtk_preview_draw_row (GTK_PREVIEW (begw->preview), (guchar *)buf, xo, y,
			    width);
      src += brush->mask->width*scale;
    }
  g_free (buf);
  if (begw->scale != scale)
    {
      gchar str[255];
      begw->scale = scale;
      g_snprintf (str, sizeof (str), "%d:1", scale);
      gtk_label_set_text (GTK_LABEL (begw->scale_label), str);
      gtk_widget_draw (begw->scale_label, NULL);
    }
  gtk_widget_draw (begw->preview, NULL);
  return TRUE;
}

static void
brush_renamed_callback (GtkWidget                *widget,
			BrushEditGeneratedWindow *begw)
{
  gtk_entry_set_text (GTK_ENTRY (begw->name),
		      gimp_brush_get_name (GIMP_BRUSH (begw->brush)));
}

void
brush_edit_generated_set_brush (BrushEditGeneratedWindow *begw,
				GimpBrush                *gbrush)
{
  GimpBrushGenerated *brush = NULL;

  g_return_if_fail (begw != NULL);

  if (begw->brush == (GimpBrushGenerated *) gbrush)
    return;

  if (begw->brush)
    {
      gtk_signal_disconnect_by_data (GTK_OBJECT (begw->brush), begw);
      gtk_object_unref (GTK_OBJECT (begw->brush));
      begw->brush = NULL;
    }

  if (!gbrush || !GIMP_IS_BRUSH_GENERATED (gbrush))
    {
      begw->brush = NULL;
      if (GTK_WIDGET_VISIBLE (begw->shell))
	gtk_widget_hide (begw->shell);
      return;
    }

  brush = GIMP_BRUSH_GENERATED (gbrush);

  gtk_signal_connect (GTK_OBJECT (brush), "dirty",
		      GTK_SIGNAL_FUNC (brush_edit_brush_dirty_callback),
		      begw);
  gtk_signal_connect (GTK_OBJECT (brush), "rename",
		      GTK_SIGNAL_FUNC (brush_renamed_callback),
		      begw);

  begw->brush = NULL;
  gtk_adjustment_set_value (GTK_ADJUSTMENT (begw->radius_data),
			    gimp_brush_generated_get_radius (brush));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (begw->hardness_data),
			    gimp_brush_generated_get_hardness (brush));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (begw->angle_data),
			    gimp_brush_generated_get_angle (brush));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (begw->aspect_ratio_data),
			    gimp_brush_generated_get_aspect_ratio (brush));
  gtk_entry_set_text (GTK_ENTRY (begw->name), gimp_brush_get_name (gbrush));
  begw->brush = brush;

  gtk_object_ref (GTK_OBJECT (begw->brush));
  brush_edit_brush_dirty_callback (GIMP_BRUSH (brush), begw);
}

void
name_changed_func (GtkWidget                *widget,
		   BrushEditGeneratedWindow *begw)
{
  gchar *entry_text;

  entry_text = gtk_entry_get_text (GTK_ENTRY (widget));
  gimp_brush_set_name (GIMP_BRUSH (begw->brush), entry_text);
}

void
focus_out_func (GtkWidget                *wid1,
		GtkWidget                *wid2,
		BrushEditGeneratedWindow *begw)
{
  name_changed_func (wid1, begw);
}

BrushEditGeneratedWindow *
brush_edit_generated_new (void)
{
  BrushEditGeneratedWindow *begw;
  GtkWidget *vbox;
  GtkWidget *slider;
  GtkWidget *table;

  begw = g_new0 (BrushEditGeneratedWindow, 1);

  begw->shell = gimp_dialog_new (_("Brush Editor"), "generated_brush_editor",
				 gimp_standard_help_func,
				 "dialogs/brush_editor.html",
				 GTK_WIN_POS_NONE,
				 FALSE, TRUE, FALSE,

				 _("Close"), brush_edit_close_callback,
				 begw, NULL, NULL, TRUE, TRUE,

				 NULL);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (begw->shell)->vbox), vbox);

  /* Brush's name */
  begw->name = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), begw->name, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (begw->name), "activate",
		      GTK_SIGNAL_FUNC (name_changed_func),
		      begw);
  gtk_signal_connect (GTK_OBJECT (begw->name), "focus_out_event",
		      GTK_SIGNAL_FUNC (focus_out_func),
		      begw);

  gtk_widget_show (begw->name);

  /* brush's preview widget w/frame  */
  begw->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (begw->frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), begw->frame, TRUE, TRUE, 0);

  begw->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (begw->preview), 125, 100);
  gtk_signal_connect_after (GTK_OBJECT (begw->frame), "size_allocate",
			    GTK_SIGNAL_FUNC (brush_edit_preview_resize),
			    begw);
  gtk_container_add (GTK_CONTAINER (begw->frame), begw->preview);

  gtk_widget_show (begw->preview);
  gtk_widget_show (begw->frame);

  /* table for sliders/labels */
  begw->scale_label = gtk_label_new ("-1:1");
  gtk_box_pack_start (GTK_BOX (vbox), begw->scale_label, FALSE, FALSE, 0);
  gtk_widget_show (begw->scale_label);

  begw->scale = -1;

  /* table for sliders/labels */
  table = gtk_table_new (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  brush radius scale  */
  begw->radius_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (10.0, 0.0, 100.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->radius_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->radius_data), "value_changed",
		      GTK_SIGNAL_FUNC (update_brush_callback),
		      begw);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Radius:"), 1.0, 1.0,
			     slider, 1, FALSE);

  /*  brush hardness scale  */
  begw->hardness_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.01, 0.0));
  slider = gtk_hscale_new (begw->hardness_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->hardness_data), "value_changed",
		      GTK_SIGNAL_FUNC (update_brush_callback),
		      begw);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Hardness:"), 1.0, 1.0,
			     slider, 1, FALSE);

  /*  brush aspect ratio scale  */
  begw->aspect_ratio_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 1.0, 20.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->aspect_ratio_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->aspect_ratio_data), "value_changed",
		      GTK_SIGNAL_FUNC (update_brush_callback),
		      begw);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Aspect Ratio:"), 1.0, 1.0,
			     slider, 1, FALSE);

  /*  brush angle scale  */
  begw->angle_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (00.0, 0.0, 180.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (begw->angle_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (begw->angle_data), "value_changed",
		      GTK_SIGNAL_FUNC (update_brush_callback),
		      begw);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Angle:"), 1.0, 1.0,
			     slider, 1, FALSE);

  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_widget_show (table);

  gtk_widget_show (vbox);
  gtk_widget_show (begw->shell);

  return begw;
}

static gint 
brush_edit_preview_resize (GtkWidget                *widget,
			   GdkEvent                 *event,
			   BrushEditGeneratedWindow *begw)
{
  gtk_preview_size (GTK_PREVIEW (begw->preview),
		    widget->allocation.width - 4,
		    widget->allocation.height - 4);

  /*  update the display  */
  if (begw->brush)
    brush_edit_brush_dirty_callback (GIMP_BRUSH (begw->brush), begw);

  return FALSE;
}
 
static void
brush_edit_close_callback (GtkWidget *widget,
			   gpointer   data)
{
  BrushEditGeneratedWindow *begw = (BrushEditGeneratedWindow *) data;

  if (GTK_WIDGET_VISIBLE (begw->shell))
    gtk_widget_hide (begw->shell);
}
