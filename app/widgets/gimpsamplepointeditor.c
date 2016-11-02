/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsamplepointeditor.c
 * Copyright (C) 2005-2016 Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimpsamplepoint.h"

#include "gimpcolorframe.h"
#include "gimpmenufactory.h"
#include "gimpsamplepointeditor.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


#define N_POINTS 4


enum
{
  PROP_0,
  PROP_SAMPLE_MERGED
};


static void   gimp_sample_point_editor_constructed    (GObject               *object);
static void   gimp_sample_point_editor_dispose        (GObject               *object);
static void   gimp_sample_point_editor_set_property   (GObject               *object,
                                                       guint                  property_id,
                                                       const GValue          *value,
                                                       GParamSpec            *pspec);
static void   gimp_sample_point_editor_get_property   (GObject               *object,
                                                       guint                  property_id,
                                                       GValue                *value,
                                                       GParamSpec            *pspec);

static void   gimp_sample_point_editor_style_set      (GtkWidget             *widget,
                                                       GtkStyle              *prev_style);
static void   gimp_sample_point_editor_set_image      (GimpImageEditor       *editor,
                                                       GimpImage             *image);

static void   gimp_sample_point_editor_point_added    (GimpImage             *image,
                                                       GimpSamplePoint       *sample_point,
                                                       GimpSamplePointEditor *editor);
static void   gimp_sample_point_editor_point_removed  (GimpImage             *image,
                                                       GimpSamplePoint       *sample_point,
                                                       GimpSamplePointEditor *editor);
static void   gimp_sample_point_editor_point_moved    (GimpImage             *image,
                                                       GimpSamplePoint       *sample_point,
                                                       GimpSamplePointEditor *editor);
static void   gimp_sample_point_editor_proj_update    (GimpImage             *image,
                                                       gboolean               now,
                                                       gint                   x,
                                                       gint                   y,
                                                       gint                   width,
                                                       gint                   height,
                                                       GimpSamplePointEditor *editor);
static void   gimp_sample_point_editor_points_changed (GimpSamplePointEditor *editor);
static void   gimp_sample_point_editor_dirty          (GimpSamplePointEditor *editor,
                                                       gint                   index);
static gboolean gimp_sample_point_editor_update       (GimpSamplePointEditor *editor);


G_DEFINE_TYPE (GimpSamplePointEditor, gimp_sample_point_editor,
               GIMP_TYPE_IMAGE_EDITOR)

#define parent_class gimp_sample_point_editor_parent_class


static void
gimp_sample_point_editor_class_init (GimpSamplePointEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = gimp_sample_point_editor_constructed;
  object_class->dispose         = gimp_sample_point_editor_dispose;
  object_class->get_property    = gimp_sample_point_editor_get_property;
  object_class->set_property    = gimp_sample_point_editor_set_property;

  widget_class->style_set       = gimp_sample_point_editor_style_set;

  image_editor_class->set_image = gimp_sample_point_editor_set_image;

  g_object_class_install_property (object_class, PROP_SAMPLE_MERGED,
                                   g_param_spec_boolean ("sample-merged",
                                                         NULL, NULL,
                                                         TRUE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_sample_point_editor_init (GimpSamplePointEditor *editor)
{
  gint content_spacing;
  gint i;
  gint row    = 0;
  gint column = 0;

  editor->sample_merged = TRUE;

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "content-spacing", &content_spacing,
                        NULL);

  editor->table = gtk_table_new (2, 2, TRUE);
  gtk_table_set_row_spacing (GTK_TABLE (editor->table), 0, content_spacing);
  gtk_table_set_col_spacing (GTK_TABLE (editor->table), 0, content_spacing);
  gtk_box_pack_start (GTK_BOX (editor), editor->table, FALSE, FALSE, 0);
  gtk_widget_show (editor->table);

  for (i = 0; i < N_POINTS; i++)
    {
      GtkWidget *frame;

      frame = g_object_new (GIMP_TYPE_COLOR_FRAME,
                            "mode",           GIMP_COLOR_FRAME_MODE_PIXEL,
                            "has-number",     TRUE,
                            "number",         i + 1,
                            "has-color-area", TRUE,
                            "sensitive",      FALSE,
                            NULL);

      gtk_table_attach (GTK_TABLE (editor->table), frame,
                        column, column + 1, row, row + 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (frame);

      editor->color_frames[i] = frame;

      column++;

      if (column > 1)
        {
          column = 0;
          row++;
        }
    }
}

static void
gimp_sample_point_editor_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
gimp_sample_point_editor_dispose (GObject *object)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (object);

  if (editor->dirty_idle_id)
    {
      g_source_remove (editor->dirty_idle_id);
      editor->dirty_idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_sample_point_editor_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      gimp_sample_point_editor_set_sample_merged (editor,
                                                  g_value_get_boolean (value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_sample_point_editor_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, editor->sample_merged);
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_sample_point_editor_style_set (GtkWidget *widget,
                                    GtkStyle  *prev_style)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (widget);
  gint                   content_spacing;

  GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "content-spacing", &content_spacing,
                        NULL);

  gtk_table_set_row_spacing (GTK_TABLE (editor->table), 0, content_spacing);
  gtk_table_set_col_spacing (GTK_TABLE (editor->table), 0, content_spacing);
}

static void
gimp_sample_point_editor_set_image (GimpImageEditor *image_editor,
                                    GimpImage       *image)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (image_editor);
  GimpColorConfig       *config = NULL;
  gint                   i;

  if (image_editor->image)
    {
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_sample_point_editor_point_added,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_sample_point_editor_point_removed,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            gimp_sample_point_editor_point_moved,
                                            editor);

      g_signal_handlers_disconnect_by_func (gimp_image_get_projection (image_editor->image),
                                            gimp_sample_point_editor_proj_update,
                                            editor);
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image)
    {
      g_signal_connect (image, "sample-point-added",
                        G_CALLBACK (gimp_sample_point_editor_point_added),
                        editor);
      g_signal_connect (image, "sample-point-removed",
                        G_CALLBACK (gimp_sample_point_editor_point_removed),
                        editor);
      g_signal_connect (image, "sample-point-moved",
                        G_CALLBACK (gimp_sample_point_editor_point_moved),
                        editor);

      g_signal_connect (gimp_image_get_projection (image), "update",
                        G_CALLBACK (gimp_sample_point_editor_proj_update),
                        editor);

      config = image->gimp->config->color_management;
    }

  for (i = 0; i < N_POINTS; i++)
    {
      gimp_color_frame_set_color_config (GIMP_COLOR_FRAME (editor->color_frames[i]),
                                         config);
    }

  gimp_sample_point_editor_points_changed (editor);
}


/*  public functions  */

GtkWidget *
gimp_sample_point_editor_new (GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_SAMPLE_POINT_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<SamplePoints>",
                       "ui-path",         "/sample-points-popup",
                       NULL);
}

void
gimp_sample_point_editor_set_sample_merged (GimpSamplePointEditor *editor,
                                            gboolean               sample_merged)
{
  g_return_if_fail (GIMP_IS_SAMPLE_POINT_EDITOR (editor));

  sample_merged = sample_merged ? TRUE : FALSE;

  if (editor->sample_merged != sample_merged)
    {
      gint i;

      editor->sample_merged = sample_merged;

      for (i = 0; i < N_POINTS; i++)
        editor->dirty[i] = TRUE;

      gimp_sample_point_editor_dirty (editor, -1);

      g_object_notify (G_OBJECT (editor), "sample-merged");
    }
}

gboolean
gimp_sample_point_editor_get_sample_merged (GimpSamplePointEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_SAMPLE_POINT_EDITOR (editor), FALSE);

  return editor->sample_merged;
}

/*  private functions  */

static void
gimp_sample_point_editor_point_added (GimpImage             *image,
                                      GimpSamplePoint       *sample_point,
                                      GimpSamplePointEditor *editor)
{
  gimp_sample_point_editor_points_changed (editor);
}

static void
gimp_sample_point_editor_point_removed (GimpImage             *image,
                                        GimpSamplePoint       *sample_point,
                                        GimpSamplePointEditor *editor)
{
  gimp_sample_point_editor_points_changed (editor);
}

static void
gimp_sample_point_editor_point_moved (GimpImage             *image,
                                      GimpSamplePoint       *sample_point,
                                      GimpSamplePointEditor *editor)
{
  gint i = g_list_index (gimp_image_get_sample_points (image), sample_point);

  if (i < N_POINTS)
    gimp_sample_point_editor_dirty (editor, i);
}

static void
gimp_sample_point_editor_proj_update (GimpImage             *image,
                                      gboolean               now,
                                      gint                   x,
                                      gint                   y,
                                      gint                   width,
                                      gint                   height,
                                      GimpSamplePointEditor *editor)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);
  GList           *sample_points;
  gint             n_points     = 0;
  GList           *list;
  gint             i;

  sample_points = gimp_image_get_sample_points (image_editor->image);

  n_points = MIN (N_POINTS, g_list_length (sample_points));

  for (i = 0, list = sample_points;
       i < n_points;
       i++, list = g_list_next (list))
    {
      GimpSamplePoint *sample_point = list->data;
      gint             sp_x;
      gint             sp_y;

      gimp_sample_point_get_position (sample_point, &sp_x, &sp_y);

      if (sp_x >= x && sp_x < (x + width) &&
          sp_y >= y && sp_y < (y + height))
        {
          gimp_sample_point_editor_dirty (editor, i);
        }
    }
}

static void
gimp_sample_point_editor_points_changed (GimpSamplePointEditor *editor)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);
  GList           *sample_points;
  gint             n_points     = 0;
  gint             i;

  if (image_editor->image)
    {
      sample_points = gimp_image_get_sample_points (image_editor->image);
      n_points = MIN (N_POINTS, g_list_length (sample_points));
    }

  for (i = 0; i < n_points; i++)
    {
      gtk_widget_set_sensitive (editor->color_frames[i], TRUE);
      editor->dirty[i] = TRUE;
    }

  for (i = n_points; i < N_POINTS; i++)
    {
      gtk_widget_set_sensitive (editor->color_frames[i], FALSE);
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (editor->color_frames[i]));
      editor->dirty[i] = FALSE;
    }

  if (n_points > 0)
    gimp_sample_point_editor_dirty (editor, -1);
}

static void
gimp_sample_point_editor_dirty (GimpSamplePointEditor *editor,
                                gint                   index)
{
  if (index >= 0)
    editor->dirty[index] = TRUE;

  if (editor->dirty_idle_id)
    g_source_remove (editor->dirty_idle_id);

  editor->dirty_idle_id =
    g_idle_add ((GSourceFunc) gimp_sample_point_editor_update,
                editor);
}

static gboolean
gimp_sample_point_editor_update (GimpSamplePointEditor *editor)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);
  GList           *sample_points;
  gint             n_points     = 0;
  GList           *list;
  gint             i;

  editor->dirty_idle_id = 0;

  if (! image_editor->image)
    return FALSE;

  sample_points = gimp_image_get_sample_points (image_editor->image);

  n_points = MIN (N_POINTS, g_list_length (sample_points));

  for (i = 0, list = sample_points;
       i < n_points;
       i++, list = g_list_next (list))
    {
      if (editor->dirty[i])
        {
          GimpSamplePoint *sample_point = list->data;
          GimpColorFrame  *color_frame;
          const Babl      *format;
          guchar           pixel[32];
          GimpRGB          color;
          gint             x;
          gint             y;

          editor->dirty[i] = FALSE;

          color_frame = GIMP_COLOR_FRAME (editor->color_frames[i]);

          gimp_sample_point_get_position (sample_point, &x, &y);

          if (gimp_image_pick_color (image_editor->image, NULL,
                                     x, y,
                                     editor->sample_merged,
                                     FALSE, 0.0,
                                     &format,
                                     pixel,
                                     &color))
            {
              gimp_color_frame_set_color (color_frame, FALSE,
                                          format, pixel, &color);
            }
          else
            {
              gimp_color_frame_set_invalid (color_frame);
            }
        }
    }

  return FALSE;
}
