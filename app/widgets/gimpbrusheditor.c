/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrusheditor.c
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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimpbrushgenerated.h"

#include "gimpbrusheditor.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static void   gimp_brush_editor_class_init (GimpBrushEditorClass *klass);
static void   gimp_brush_editor_init       (GimpBrushEditor      *editor);

static void   gimp_brush_editor_set_data       (GimpDataEditor  *editor,
                                                GimpData        *data);

static void   gimp_brush_editor_update_brush   (GtkAdjustment   *adjustment,
                                                GimpBrushEditor *editor);
static void   gimp_brush_editor_preview_resize (GtkWidget       *widget,
                                                GtkAllocation   *allocation,
                                                GimpBrushEditor *editor);
static void   gimp_brush_editor_clear_preview  (GimpBrushEditor *editor);
static void   gimp_brush_editor_brush_dirty    (GimpBrush       *brush,
                                                GimpBrushEditor *editor);


static GimpDataEditorClass *parent_class = NULL;


GType
gimp_brush_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
      {
        sizeof (GimpBrushEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_brush_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpBrushEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_brush_editor_init,
      };

      type = g_type_register_static (GIMP_TYPE_DATA_EDITOR,
                                     "GimpBrushEditor",
                                     &info, 0);
    }

  return type;
}

static void
gimp_brush_editor_class_init (GimpBrushEditorClass *klass)
{
  GimpDataEditorClass *editor_class;

  editor_class = GIMP_DATA_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->set_data = gimp_brush_editor_set_data;
}

static void
gimp_brush_editor_init (GimpBrushEditor *editor)
{
  /* brush's preview widget w/frame  */
  editor->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (editor->frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), editor->frame,
                      TRUE, TRUE, 0);
  gtk_widget_show (editor->frame);

  editor->preview = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
  gtk_preview_size (GTK_PREVIEW (editor->preview), 125, 100);

  /*  Enable auto-resizing of the preview but ensure a minimal size  */
  gtk_widget_set_size_request (editor->preview, 125, 100);
  gtk_preview_set_expand (GTK_PREVIEW (editor->preview), TRUE);
  gtk_container_add (GTK_CONTAINER (editor->frame), editor->preview);
  gtk_widget_show (editor->preview);

  g_signal_connect_after (G_OBJECT (editor->frame), "size_allocate",
			  G_CALLBACK (gimp_brush_editor_preview_resize),
			  editor);

  /* table for sliders/labels */
  editor->scale_label = gtk_label_new ("-1:1");
  gtk_box_pack_start (GTK_BOX (editor), editor->scale_label, FALSE, FALSE, 0);
  gtk_widget_show (editor->scale_label);

  editor->scale = -1;

  /* table for sliders/labels */
  editor->options_table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (editor->options_table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (editor->options_table), 2);
  gtk_box_pack_start (GTK_BOX (editor), editor->options_table, FALSE, FALSE, 0);
  gtk_widget_show (editor->options_table);

  /*  brush radius scale  */
  editor->radius_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, 0,
                                          _("Radius:"), -1, 50,
                                          0.0, 1.0, 100.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (G_OBJECT (editor->radius_data), "value_changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush hardness scale  */
  editor->hardness_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, 1,
                                          _("Hardness:"), -1, 50,
                                          0.0, 0.0, 1.0, 0.01, 0.1, 2,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (G_OBJECT (editor->hardness_data), "value_changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush aspect ratio scale  */
  editor->aspect_ratio_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, 2,
                                          _("Aspect Ratio:"), -1, 50,
                                          0.0, 1.0, 20.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (G_OBJECT (editor->aspect_ratio_data),"value_changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush angle scale  */
  editor->angle_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, 3,
                                          _("Angle:"), -1, 50,
                                          0.0, 0.0, 180.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (G_OBJECT (editor->angle_data), "value_changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);
}

static void
gimp_brush_editor_set_data (GimpDataEditor *editor,
                            GimpData       *data)
{
  GimpBrushEditor *brush_editor;

  brush_editor = GIMP_BRUSH_EDITOR (editor);

  if (editor->data)
    {
      g_signal_handlers_disconnect_by_func (G_OBJECT (editor->data),
                                            gimp_brush_editor_brush_dirty,
                                            editor);
    }

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    {
      gdouble radius   = 0.0;
      gdouble hardness = 0.0;
      gdouble angle    = 0.0;
      gdouble ratio    = 0.0;

      g_signal_connect (G_OBJECT (editor->data), "invalidate_preview",
                        G_CALLBACK (gimp_brush_editor_brush_dirty),
                        editor);

      if (GIMP_IS_BRUSH_GENERATED (editor->data))
        {
          GimpBrushGenerated *brush;

          brush = GIMP_BRUSH_GENERATED (editor->data);

          radius   = gimp_brush_generated_get_radius (brush);
          hardness = gimp_brush_generated_get_hardness (brush);
          angle    = gimp_brush_generated_get_angle (brush);
          ratio    = gimp_brush_generated_get_aspect_ratio (brush);

          gtk_widget_set_sensitive (brush_editor->options_table, TRUE);
        }
      else
        {
          gtk_widget_set_sensitive (brush_editor->options_table, FALSE);
        }

      gtk_adjustment_set_value (brush_editor->radius_data,       radius);
      gtk_adjustment_set_value (brush_editor->hardness_data,     hardness);
      gtk_adjustment_set_value (brush_editor->angle_data,        angle);
      gtk_adjustment_set_value (brush_editor->aspect_ratio_data, ratio);

      gimp_brush_editor_brush_dirty (GIMP_BRUSH (editor->data), brush_editor);
    }
}


/*  public functions  */

GimpDataEditor *
gimp_brush_editor_new (Gimp *gimp)
{
  GimpBrushEditor *brush_editor;

  brush_editor = g_object_new (GIMP_TYPE_BRUSH_EDITOR, NULL);

  gimp_data_editor_construct (GIMP_DATA_EDITOR (brush_editor),
                              gimp,
                              GIMP_TYPE_BRUSH);

  return GIMP_DATA_EDITOR (brush_editor);
}


/*  private functions  */

static void
gimp_brush_editor_update_brush (GtkAdjustment   *adjustment,
                                GimpBrushEditor *editor)
{
  GimpBrushGenerated *brush = NULL;

  if (GIMP_IS_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data))
    brush = GIMP_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data);

  if (brush &&
      ((editor->radius_data->value  
	!= gimp_brush_generated_get_radius (brush))
       || (editor->hardness_data->value
	   != gimp_brush_generated_get_hardness (brush))
       || (editor->aspect_ratio_data->value
	   != gimp_brush_generated_get_aspect_ratio (brush))
       || (editor->angle_data->value
	   != gimp_brush_generated_get_angle (brush))))
    {
      gimp_brush_generated_freeze (brush);

      gimp_brush_generated_set_radius       (brush,
					     editor->radius_data->value);
      gimp_brush_generated_set_hardness     (brush,
					     editor->hardness_data->value);
      gimp_brush_generated_set_aspect_ratio (brush,
					     editor->aspect_ratio_data->value);
      gimp_brush_generated_set_angle        (brush,
					     editor->angle_data->value);

      gimp_brush_generated_thaw (brush);
    }
}

static void
gimp_brush_editor_preview_resize (GtkWidget       *widget,
                                  GtkAllocation   *allocation,
                                  GimpBrushEditor *editor)
{
  if (GIMP_DATA_EDITOR (editor)->data)
    gimp_brush_editor_brush_dirty (GIMP_BRUSH (GIMP_DATA_EDITOR (editor)->data),
                                   editor);
}

static void
gimp_brush_editor_clear_preview (GimpBrushEditor *editor)
{
  guchar *buf;
  gint    i;

  buf = g_new (guchar, editor->preview->allocation.width);

  /*  Set the buffer to white  */
  memset (buf, 255, editor->preview->allocation.width);

  /*  Set the image buffer to white  */
  for (i = 0; i < editor->preview->allocation.height; i++)
    gtk_preview_draw_row (GTK_PREVIEW (editor->preview), buf, 0, i,
			  editor->preview->allocation.width);

  g_free (buf);
}

static void
gimp_brush_editor_brush_dirty (GimpBrush       *brush,
                               GimpBrushEditor *editor)
{
  gint    x, y, width, yend, ystart, xo;
  gint    scale;
  guchar *src, *buf;

  gimp_brush_editor_clear_preview (editor);

  if (! (brush && brush->mask))
    return;

  scale = MAX (ceil (brush->mask->width/
		     (float) editor->preview->allocation.width),
	       ceil (brush->mask->height/
		     (float) editor->preview->allocation.height));

  ystart = 0;
  xo     = editor->preview->allocation.width  / 2 - brush->mask->width  / (2 * scale);
  ystart = editor->preview->allocation.height / 2 - brush->mask->height / (2 * scale);
  yend   = ystart + brush->mask->height / scale;
  width  = CLAMP (brush->mask->width/scale, 0, editor->preview->allocation.width);

  buf = g_new (guchar, width);
  src = (guchar *) temp_buf_data (brush->mask);

  for (y = ystart; y < yend; y++)
    {
      /*  Invert the mask for display.
       */
      for (x = 0; x < width; x++)
	buf[x] = 255 - src[x * scale];

      gtk_preview_draw_row (GTK_PREVIEW (editor->preview), buf,
			    xo, y, width);

      src += brush->mask->width * scale;
    }

  g_free (buf);

  if (editor->scale != scale)
    {
      gchar str[255];

      editor->scale = scale;
      g_snprintf (str, sizeof (str), "%d:1", scale);
      gtk_label_set_text (GTK_LABEL (editor->scale_label), str);
    }

  gtk_widget_queue_draw (editor->preview);
}
