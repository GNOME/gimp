/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * brush-editor.c
 * Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/core-types.h"

#include "base/temp-buf.h"

#include "core/gimpbrushgenerated.h"

#include "brush-editor.h"

#include "libgimp/gimpintl.h"


struct _BrushEditor
{
  GtkWidget     *shell;
  GtkWidget     *frame;
  GtkWidget     *preview;
  GtkWidget     *scale_label;
  GtkWidget     *options_box;
  GtkWidget     *name;
  GtkAdjustment *radius_data;
  GtkAdjustment *hardness_data;
  GtkAdjustment *angle_data;
  GtkAdjustment *aspect_ratio_data;

  /*  Brush preview  */
  GtkWidget          *brush_preview;
  GimpBrushGenerated *brush;
  gint                scale;
};


static void       brush_editor_close_callback     (GtkWidget     *widget,
						   gpointer       data);
static void       brush_editor_name_activate      (GtkWidget     *widget,
						   BrushEditor   *brush_editor);
static gboolean   brush_editor_name_focus_out     (GtkWidget     *widget,
						   GdkEventFocus *fevent,
						   BrushEditor   *brush_editor);
static void       brush_editor_update_brush       (GtkAdjustment *adjustment,
						   BrushEditor   *brush_editor);
static void       brush_editor_preview_resize     (GtkWidget     *widget,
						   GtkAllocation *allocation,
						   BrushEditor   *brush_editor);

static void       brush_editor_clear_preview      (BrushEditor   *brush_editor);
static void       brush_editor_brush_dirty        (GimpBrush     *brush,
						   BrushEditor   *brush_editor);
static void       brush_editor_brush_name_changed (GtkWidget     *widget,
						   BrushEditor   *brush_editor);


/*  public functions  */

BrushEditor *
brush_editor_new (Gimp *gimp)
{
  BrushEditor *brush_editor;
  GtkWidget   *vbox;
  GtkWidget   *slider;
  GtkWidget   *table;

  brush_editor = g_new0 (BrushEditor, 1);

  brush_editor->shell =
    gimp_dialog_new (_("Brush Editor"), "generated_brush_editor",
		     gimp_standard_help_func,
		     "dialogs/brush_editor.html",
		     GTK_WIN_POS_NONE,
		     FALSE, TRUE, FALSE,

		     "_delete_event_", brush_editor_close_callback,
		     brush_editor, NULL, NULL, TRUE, TRUE,

		     NULL);

  gtk_dialog_set_has_separator (GTK_DIALOG (brush_editor->shell), FALSE);
  gtk_widget_hide (GTK_DIALOG (brush_editor->shell)->action_area);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (brush_editor->shell)->vbox),
		     vbox);

  /* Brush's name */
  brush_editor->name = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (vbox), brush_editor->name, FALSE, FALSE, 0);
  gtk_widget_show (brush_editor->name);

  g_signal_connect (G_OBJECT (brush_editor->name), "activate",
                    G_CALLBACK (brush_editor_name_activate),
                    brush_editor);
  g_signal_connect (G_OBJECT (brush_editor->name), "focus_out_event",
                    G_CALLBACK (brush_editor_name_focus_out),
                    brush_editor);

  /* brush's preview widget w/frame  */
  brush_editor->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (brush_editor->frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), brush_editor->frame, TRUE, TRUE, 0);
  gtk_widget_show (brush_editor->frame);

  brush_editor->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (brush_editor->preview), 125, 100);

  /*  Enable auto-resizing of the preview but ensure a minimal size  */
  gtk_widget_set_usize (brush_editor->preview, 125, 100);
  gtk_preview_set_expand (GTK_PREVIEW (brush_editor->preview), TRUE);
  gtk_container_add (GTK_CONTAINER (brush_editor->frame), brush_editor->preview);
  gtk_widget_show (brush_editor->preview);

  g_signal_connect_after (G_OBJECT (brush_editor->frame), "size_allocate",
			  G_CALLBACK (brush_editor_preview_resize),
			  brush_editor);

  /* table for sliders/labels */
  brush_editor->scale_label = gtk_label_new ("-1:1");
  gtk_box_pack_start (GTK_BOX (vbox), brush_editor->scale_label,
		      FALSE, FALSE, 0);
  gtk_widget_show (brush_editor->scale_label);

  brush_editor->scale = -1;

  /* table for sliders/labels */
  table = gtk_table_new (4, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  /*  brush radius scale  */
  brush_editor->radius_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (10.0, 0.0, 100.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (brush_editor->radius_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  g_signal_connect (G_OBJECT (brush_editor->radius_data), "value_changed",
                    G_CALLBACK (brush_editor_update_brush),
                    brush_editor);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Radius:"), 1.0, 1.0,
			     slider, 1, FALSE);

  /*  brush hardness scale  */
  brush_editor->hardness_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 1.0, 0.01, 0.01, 0.0));
  slider = gtk_hscale_new (brush_editor->hardness_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  g_signal_connect (G_OBJECT (brush_editor->hardness_data), "value_changed",
                    G_CALLBACK (brush_editor_update_brush),
                    brush_editor);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Hardness:"), 1.0, 1.0,
			     slider, 1, FALSE);

  /*  brush aspect ratio scale  */
  brush_editor->aspect_ratio_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (1.0, 1.0, 20.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (brush_editor->aspect_ratio_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  g_signal_connect (G_OBJECT (brush_editor->aspect_ratio_data),"value_changed",
                    G_CALLBACK (brush_editor_update_brush),
                    brush_editor);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Aspect Ratio:"), 1.0, 1.0,
			     slider, 1, FALSE);

  /*  brush angle scale  */
  brush_editor->angle_data =
    GTK_ADJUSTMENT (gtk_adjustment_new (00.0, 0.0, 180.0, 0.1, 1.0, 0.0));
  slider = gtk_hscale_new (brush_editor->angle_data);
  gtk_scale_set_value_pos (GTK_SCALE (slider), GTK_POS_TOP);
  gtk_range_set_update_policy (GTK_RANGE (slider), GTK_UPDATE_DELAYED);
  g_signal_connect (G_OBJECT (brush_editor->angle_data), "value_changed",
                    G_CALLBACK (brush_editor_update_brush),
                    brush_editor);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Angle:"), 1.0, 1.0,
			     slider, 1, FALSE);

  gtk_table_set_row_spacings (GTK_TABLE (table), 3);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 4);
  gtk_widget_show (table);

  gtk_widget_show (vbox);
  gtk_widget_show (brush_editor->shell);

  return brush_editor;
}

void
brush_editor_set_brush (BrushEditor *brush_editor,
			GimpBrush   *gbrush)
{
  GimpBrushGenerated *brush = NULL;

  g_return_if_fail (brush_editor != NULL);
  g_return_if_fail (GIMP_IS_BRUSH_GENERATED (gbrush));

  if (brush_editor->brush)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (brush_editor->brush),
                                            brush_editor_brush_dirty,
                                            brush_editor);
      g_signal_handlers_disconnect_by_func (G_OBJECT (brush_editor->brush),
                                            brush_editor_brush_name_changed,
                                            brush_editor);
      
      g_object_unref (G_OBJECT (brush_editor->brush));
      brush_editor->brush = NULL;
    }

  brush = GIMP_BRUSH_GENERATED (gbrush);

  g_signal_connect (G_OBJECT (brush), "invalidate_preview",
                    G_CALLBACK (brush_editor_brush_dirty),
                    brush_editor);
  g_signal_connect (G_OBJECT (brush), "name_changed",
                    G_CALLBACK (brush_editor_brush_name_changed),
                    brush_editor);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_editor->radius_data),
			    gimp_brush_generated_get_radius (brush));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_editor->hardness_data),
			    gimp_brush_generated_get_hardness (brush));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_editor->angle_data),
			    gimp_brush_generated_get_angle (brush));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_editor->aspect_ratio_data),
			    gimp_brush_generated_get_aspect_ratio (brush));
  gtk_entry_set_text (GTK_ENTRY (brush_editor->name),
		      gimp_object_get_name (GIMP_OBJECT (gbrush)));

  brush_editor->brush = brush;
  g_object_ref (G_OBJECT (brush_editor->brush));

  brush_editor_brush_dirty (GIMP_BRUSH (brush), brush_editor);
}


/*  private functions  */

static void
brush_editor_close_callback (GtkWidget *widget,
			     gpointer   data)
{
  BrushEditor *brush_editor = (BrushEditor *) data;

  if (GTK_WIDGET_VISIBLE (brush_editor->shell))
    gtk_widget_hide (brush_editor->shell);
}

static void
brush_editor_name_activate (GtkWidget   *widget,
			    BrushEditor *brush_editor)
{
  const gchar *entry_text;

  entry_text = gtk_entry_get_text (GTK_ENTRY (widget));
  gimp_object_set_name (GIMP_OBJECT (brush_editor->brush), entry_text);
}

static gboolean
brush_editor_name_focus_out (GtkWidget     *widget,
			     GdkEventFocus *fevent,
			     BrushEditor   *brush_editor)
{
  brush_editor_name_activate (widget, brush_editor);

  return FALSE;
}

static void
brush_editor_update_brush (GtkAdjustment *adjustment,
			   BrushEditor   *brush_editor)
{
  if (brush_editor->brush &&
      ((brush_editor->radius_data->value  
	!= gimp_brush_generated_get_radius (brush_editor->brush))
       || (brush_editor->hardness_data->value
	   != gimp_brush_generated_get_hardness (brush_editor->brush))
       || (brush_editor->aspect_ratio_data->value
	   != gimp_brush_generated_get_aspect_ratio (brush_editor->brush))
       || (brush_editor->angle_data->value
	   != gimp_brush_generated_get_angle (brush_editor->brush))))
    {
      gimp_brush_generated_freeze (brush_editor->brush);
      gimp_brush_generated_set_radius       (brush_editor->brush,
					     brush_editor->radius_data->value);
      gimp_brush_generated_set_hardness     (brush_editor->brush,
					     brush_editor->hardness_data->value);
      gimp_brush_generated_set_aspect_ratio (brush_editor->brush,
					     brush_editor->aspect_ratio_data->value);
      gimp_brush_generated_set_angle        (brush_editor->brush,
					     brush_editor->angle_data->value);
      gimp_brush_generated_thaw (brush_editor->brush);
    }
}

static void
brush_editor_preview_resize (GtkWidget     *widget,
			     GtkAllocation *allocation,
			     BrushEditor   *brush_editor)
{
  if (brush_editor->brush)
    brush_editor_brush_dirty (GIMP_BRUSH (brush_editor->brush), brush_editor);
}

static void
brush_editor_clear_preview (BrushEditor *brush_editor)
{
  guchar *buf;
  gint    i;

  buf = g_new (guchar, brush_editor->preview->allocation.width);

  /*  Set the buffer to white  */
  memset (buf, 255, brush_editor->preview->allocation.width);

  /*  Set the image buffer to white  */
  for (i = 0; i < brush_editor->preview->allocation.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (brush_editor->preview), buf, 0, i,
			  brush_editor->preview->allocation.width);

  g_free (buf);
}

static void
brush_editor_brush_dirty (GimpBrush   *brush,
			  BrushEditor *brush_editor)
{
  gint    x, y, width, yend, ystart, xo;
  gint    scale;
  guchar *src, *buf;

  brush_editor_clear_preview (brush_editor);

  if (brush == NULL || brush->mask == NULL)
    return;

  scale = MAX (ceil (brush->mask->width/
		     (float) brush_editor->preview->allocation.width),
	       ceil (brush->mask->height/
		     (float) brush_editor->preview->allocation.height));

  ystart = 0;
  xo     = brush_editor->preview->allocation.width  / 2 - brush->mask->width  / (2 * scale);
  ystart = brush_editor->preview->allocation.height / 2 - brush->mask->height / (2 * scale);
  yend   = ystart + brush->mask->height / scale;
  width  = CLAMP (brush->mask->width/scale, 0, brush_editor->preview->allocation.width);

  buf = g_new (guchar, width);
  src = (guchar *) temp_buf_data (brush->mask);

  for (y = ystart; y < yend; y++)
    {
      /*  Invert the mask for display.
       */
      for (x = 0; x < width; x++)
	buf[x] = 255 - src[x * scale];

      gtk_preview_draw_row (GTK_PREVIEW (brush_editor->preview), buf,
			    xo, y, width);

      src += brush->mask->width * scale;
    }

  g_free (buf);

  if (brush_editor->scale != scale)
    {
      gchar str[255];

      brush_editor->scale = scale;
      g_snprintf (str, sizeof (str), "%d:1", scale);
      gtk_label_set_text (GTK_LABEL (brush_editor->scale_label), str);
    }

  gtk_widget_queue_draw (brush_editor->preview);
}

static void
brush_editor_brush_name_changed (GtkWidget   *widget,
				 BrushEditor *brush_editor)
{
  gtk_entry_set_text (GTK_ENTRY (brush_editor->name),
		      gimp_object_get_name (GIMP_OBJECT (brush_editor->brush)));
}
