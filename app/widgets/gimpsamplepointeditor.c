/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsamplepointeditor.c
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"

#include "gimpcolorframe.h"
#include "gimpmenufactory.h"
#include "gimpsamplepointeditor.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


static void   gimp_sample_point_editor_class_init (GimpSamplePointEditorClass *klass);
static void   gimp_sample_point_editor_init       (GimpSamplePointEditor      *editor);

static GObject * gimp_sample_point_editor_constructor (GType                  type,
                                                       guint                  n_params,
                                                       GObjectConstructParam *params);

static void   gimp_sample_point_editor_style_set      (GtkWidget             *widget,
                                                       GtkStyle              *prev_style);
static void   gimp_sample_point_editor_set_image      (GimpImageEditor       *editor,
                                                       GimpImage             *gimage);

static void   gimp_sample_point_editor_point_added    (GimpImage             *gimage,
                                                       GimpSamplePoint       *sample_point,
                                                       GimpSamplePointEditor *editor);
static void   gimp_sample_point_editor_point_removed  (GimpImage             *gimage,
                                                       GimpSamplePoint       *sample_point,
                                                       GimpSamplePointEditor *editor);
static void   gimp_sample_point_editor_points_changed (GimpSamplePointEditor *editor);


static GimpImageEditorClass *parent_class = NULL;


GType
gimp_sample_point_editor_get_type (void)
{
  static GType editor_type = 0;

  if (! editor_type)
    {
      static const GTypeInfo editor_info =
      {
        sizeof (GimpSamplePointEditorClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_sample_point_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpSamplePointEditor),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_sample_point_editor_init,
      };

      editor_type = g_type_register_static (GIMP_TYPE_IMAGE_EDITOR,
                                            "GimpSamplePointEditor",
                                            &editor_info, 0);
    }

  return editor_type;
}

static void
gimp_sample_point_editor_class_init (GimpSamplePointEditorClass* klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  GimpImageEditorClass *image_editor_class = GIMP_IMAGE_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor     = gimp_sample_point_editor_constructor;

  widget_class->style_set       = gimp_sample_point_editor_style_set;

  image_editor_class->set_image = gimp_sample_point_editor_set_image;
}

static void
gimp_sample_point_editor_init (GimpSamplePointEditor *editor)
{
  gint content_spacing;
  gint i;
  gint row    = 0;
  gint column = 0;

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "content_spacing", &content_spacing,
			NULL);

  editor->table = gtk_table_new (2, 2, TRUE);
  gtk_table_set_row_spacing (GTK_TABLE (editor->table), 0, content_spacing);
  gtk_table_set_col_spacing (GTK_TABLE (editor->table), 0, content_spacing);
  gtk_box_pack_start (GTK_BOX (editor), editor->table, FALSE, FALSE, 0);
  gtk_widget_show (editor->table);

  for (i = 0; i < 4; i++)
    {
      editor->color_frames[i] = gimp_color_frame_new ();
      gimp_color_frame_set_mode (GIMP_COLOR_FRAME (editor->color_frames[i]),
                                 GIMP_COLOR_FRAME_MODE_PIXEL);
      gtk_widget_set_sensitive (editor->color_frames[i], FALSE);
      gtk_table_attach (GTK_TABLE (editor->table), editor->color_frames[i],
                        column, column + 1, row, row + 1,
                        GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
      gtk_widget_show (editor->color_frames[i]);

      column++;

      if (column > 1)
        {
          column = 0;
          row++;
        }
    }
}

static GObject *
gimp_sample_point_editor_constructor (GType                  type,
                                      guint                  n_params,
                                      GObjectConstructParam *params)
{
  GObject               *object;
  GimpSamplePointEditor *editor;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  editor = GIMP_SAMPLE_POINT_EDITOR (object);

  return object;
}

static void
gimp_sample_point_editor_style_set (GtkWidget *widget,
                                    GtkStyle  *prev_style)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (widget);
  gint                   content_spacing;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "content_spacing", &content_spacing,
			NULL);

  gtk_table_set_row_spacing (GTK_TABLE (editor->table), 0, content_spacing);
  gtk_table_set_col_spacing (GTK_TABLE (editor->table), 0, content_spacing);
}

static void
gimp_sample_point_editor_set_image (GimpImageEditor *image_editor,
                                    GimpImage       *gimage)
{
  GimpSamplePointEditor *editor = GIMP_SAMPLE_POINT_EDITOR (image_editor);

  if (image_editor->gimage)
    {
      g_signal_handlers_disconnect_by_func (image_editor->gimage,
                                            gimp_sample_point_editor_point_added,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->gimage,
                                            gimp_sample_point_editor_point_removed,
                                            editor);
    }

  GIMP_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, gimage);

  if (gimage)
    {
      g_signal_connect (gimage, "sample-point-added",
                        G_CALLBACK (gimp_sample_point_editor_point_added),
                        editor);
      g_signal_connect (gimage, "sample-point-removed",
                        G_CALLBACK (gimp_sample_point_editor_point_removed),
                        editor);
    }

  gimp_sample_point_editor_points_changed (editor);
}


/*  public functions  */

GtkWidget *
gimp_sample_point_editor_new (GimpMenuFactory *menu_factory)
{
  g_return_val_if_fail (GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (GIMP_TYPE_SAMPLE_POINT_EDITOR,
#if 0
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<SamplePointEditor>",
                       "ui-path",         "/selection-editor-popup",
#endif
                       NULL);
}


/*  private functions  */

static void
gimp_sample_point_editor_point_added (GimpImage             *gimage,
                                      GimpSamplePoint       *sample_point,
                                      GimpSamplePointEditor *editor)
{
  gimp_sample_point_editor_points_changed (editor);
}

static void
gimp_sample_point_editor_point_removed (GimpImage             *gimage,
                                        GimpSamplePoint       *sample_point,
                                        GimpSamplePointEditor *editor)
{
  gimp_sample_point_editor_points_changed (editor);
}

static void
gimp_sample_point_editor_points_changed (GimpSamplePointEditor *editor)
{
  GimpImageEditor *image_editor = GIMP_IMAGE_EDITOR (editor);
  gint             n_points     = 0;
  gint             i;

  if (image_editor->gimage)
    n_points = MIN (4, g_list_length (image_editor->gimage->sample_points));

  for (i = 0; i < n_points; i++)
    {
      gtk_widget_set_sensitive (editor->color_frames[i], TRUE);
    }

  for (i = n_points; i < 4; i++)
    {
      gtk_widget_set_sensitive (editor->color_frames[i], FALSE);
      gimp_color_frame_set_invalid (GIMP_COLOR_FRAME (editor->color_frames[i]));
    }
}
