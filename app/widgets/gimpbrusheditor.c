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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpbrushgenerated.h"

#include "gimpbrusheditor.h"
#include "gimppreview.h"

#include "gimp-intl.h"


#define BRUSH_PREVIEW_WIDTH  128
#define BRUSH_PREVIEW_HEIGHT  96


/*  local function prototypes  */

static void   gimp_brush_editor_class_init   (GimpBrushEditorClass *klass);
static void   gimp_brush_editor_init         (GimpBrushEditor      *editor);

static void   gimp_brush_editor_set_data     (GimpDataEditor       *editor,
                                              GimpData             *data);

static void   gimp_brush_editor_update_brush (GtkAdjustment        *adjustment,
                                              GimpBrushEditor      *editor);
static void   gimp_brush_editor_notify_brush (GimpBrushGenerated   *brush,
                                              GParamSpec           *pspec,
                                              GimpBrushEditor      *editor);


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
  GimpDataEditorClass *editor_class = GIMP_DATA_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  editor_class->set_data = gimp_brush_editor_set_data;
}

static void
gimp_brush_editor_init (GimpBrushEditor *editor)
{
  GtkWidget *frame;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor->preview = gimp_preview_new_full_by_types (GIMP_TYPE_PREVIEW,
                                                    GIMP_TYPE_BRUSH,
                                                    BRUSH_PREVIEW_WIDTH,
                                                    BRUSH_PREVIEW_HEIGHT, 0,
                                                    FALSE, FALSE, TRUE);
  gtk_widget_set_size_request (editor->preview,
                               BRUSH_PREVIEW_WIDTH, BRUSH_PREVIEW_HEIGHT);
  gimp_preview_set_expand (GIMP_PREVIEW (editor->preview), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), editor->preview);
  gtk_widget_show (editor->preview);

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
                                          _("Radius:"), -1, 5,
                                          0.0, 1.0, 1000.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (editor->radius_data, "value_changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush hardness scale  */
  editor->hardness_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, 1,
                                          _("Hardness:"), -1, 5,
                                          0.0, 0.0, 1.0, 0.01, 0.1, 2,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (editor->hardness_data, "value_changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush aspect ratio scale  */
  editor->aspect_ratio_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, 2,
                                          _("Aspect ratio:"), -1, 5,
                                          0.0, 1.0, 20.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (editor->aspect_ratio_data,"value_changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);

  /*  brush angle scale  */
  editor->angle_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, 3,
                                          _("Angle:"), -1, 5,
                                          0.0, 0.0, 180.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_signal_connect (editor->angle_data, "value_changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);
}

static void
gimp_brush_editor_set_data (GimpDataEditor *editor,
                            GimpData       *data)
{
  GimpBrushEditor *brush_editor;
  gdouble          radius   = 0.0;
  gdouble          hardness = 0.0;
  gdouble          ratio    = 0.0;
  gdouble          angle    = 0.0;

  brush_editor = GIMP_BRUSH_EDITOR (editor);

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          gimp_brush_editor_notify_brush,
                                          editor);

  GIMP_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    g_signal_connect (editor->data, "notify",
                      G_CALLBACK (gimp_brush_editor_notify_brush),
                      editor);

  gimp_preview_set_viewable (GIMP_PREVIEW (brush_editor->preview),
                             (GimpViewable *) data);

  if (editor->data && GIMP_IS_BRUSH_GENERATED (editor->data))
    {
      GimpBrushGenerated *brush = GIMP_BRUSH_GENERATED (editor->data);

      radius   = gimp_brush_generated_get_radius       (brush);
      hardness = gimp_brush_generated_get_hardness     (brush);
      ratio    = gimp_brush_generated_get_aspect_ratio (brush);
      angle    = gimp_brush_generated_get_angle        (brush);
    }

  gtk_widget_set_sensitive (brush_editor->options_table,
                            editor->data_editable);

  gtk_adjustment_set_value (brush_editor->radius_data,       radius);
  gtk_adjustment_set_value (brush_editor->hardness_data,     hardness);
  gtk_adjustment_set_value (brush_editor->aspect_ratio_data, ratio);
  gtk_adjustment_set_value (brush_editor->angle_data,        angle);
}


/*  public functions  */

GimpDataEditor *
gimp_brush_editor_new (Gimp *gimp)
{
  GimpBrushEditor *brush_editor;

  brush_editor = g_object_new (GIMP_TYPE_BRUSH_EDITOR, NULL);

  if (! gimp_data_editor_construct (GIMP_DATA_EDITOR (brush_editor),
                                    gimp->brush_factory,
                                    NULL, NULL, NULL))
    {
      g_object_unref (brush_editor);
      return NULL;
    }

  return GIMP_DATA_EDITOR (brush_editor);
}


/*  private functions  */

static void
gimp_brush_editor_update_brush (GtkAdjustment   *adjustment,
                                GimpBrushEditor *editor)
{
  GimpBrushGenerated *brush;
  gdouble             radius;
  gdouble             hardness;
  gdouble             ratio;
  gdouble             angle;

  if (! GIMP_IS_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data))
    return;

  brush = GIMP_BRUSH_GENERATED (GIMP_DATA_EDITOR (editor)->data);

  radius   = editor->radius_data->value;
  hardness = editor->hardness_data->value;
  ratio    = editor->aspect_ratio_data->value;
  angle    = editor->angle_data->value;

  if (angle    != gimp_brush_generated_get_radius       (brush) ||
      hardness != gimp_brush_generated_get_hardness     (brush) ||
      ratio    != gimp_brush_generated_get_aspect_ratio (brush) ||
      angle    != gimp_brush_generated_get_angle        (brush))
    {
      g_signal_handlers_block_by_func (brush,
                                       gimp_brush_editor_notify_brush,
                                       editor);

      gimp_data_freeze (GIMP_DATA (brush));
      g_object_freeze_notify (G_OBJECT (brush));

      gimp_brush_generated_set_radius       (brush, radius);
      gimp_brush_generated_set_hardness     (brush, hardness);
      gimp_brush_generated_set_aspect_ratio (brush, ratio);
      gimp_brush_generated_set_angle        (brush, angle);

      g_object_thaw_notify (G_OBJECT (brush));
      gimp_data_thaw (GIMP_DATA (brush));

      g_signal_handlers_unblock_by_func (brush,
                                         gimp_brush_editor_notify_brush,
                                         editor);
    }
}

static void
gimp_brush_editor_notify_brush (GimpBrushGenerated   *brush,
                                GParamSpec           *pspec,
                                GimpBrushEditor      *editor)
{
  GtkAdjustment *adj   = NULL;
  gdouble        value = 0.0;

  if (! strcmp (pspec->name, "radius"))
    {
      adj   = editor->radius_data;
      value = brush->radius;
    }
  else if (! strcmp (pspec->name, "hardness"))
    {
      adj   = editor->hardness_data;
      value = brush->hardness;
    }
  else if (! strcmp (pspec->name, "angle"))
    {
      adj   = editor->angle_data;
      value = brush->angle;
    }
  else if (! strcmp (pspec->name, "aspect-ratio"))
    {
      adj   = editor->aspect_ratio_data;
      value = brush->aspect_ratio;
    }

  if (adj)
    {
      g_signal_handlers_block_by_func (adj,
                                       gimp_brush_editor_update_brush,
                                       editor);

      gtk_adjustment_set_value (adj, value);

      g_signal_handlers_unblock_by_func (adj,
                                         gimp_brush_editor_update_brush,
                                         editor);
    }
}
