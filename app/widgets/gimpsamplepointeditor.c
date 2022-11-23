/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasamplepointeditor.c
 * Copyright (C) 2005-2016 Michael Natterer <mitch@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmacoreconfig.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-pick-color.h"
#include "core/ligmaimage-sample-points.h"
#include "core/ligmasamplepoint.h"

#include "ligmacolorframe.h"
#include "ligmamenufactory.h"
#include "ligmasamplepointeditor.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_MERGED
};


static void   ligma_sample_point_editor_constructed    (GObject               *object);
static void   ligma_sample_point_editor_dispose        (GObject               *object);
static void   ligma_sample_point_editor_set_property   (GObject               *object,
                                                       guint                  property_id,
                                                       const GValue          *value,
                                                       GParamSpec            *pspec);
static void   ligma_sample_point_editor_get_property   (GObject               *object,
                                                       guint                  property_id,
                                                       GValue                *value,
                                                       GParamSpec            *pspec);

static void   ligma_sample_point_editor_style_updated  (GtkWidget             *widget);
static void   ligma_sample_point_editor_set_image      (LigmaImageEditor       *editor,
                                                       LigmaImage             *image);

static void   ligma_sample_point_editor_point_added    (LigmaImage             *image,
                                                       LigmaSamplePoint       *sample_point,
                                                       LigmaSamplePointEditor *editor);
static void   ligma_sample_point_editor_point_removed  (LigmaImage             *image,
                                                       LigmaSamplePoint       *sample_point,
                                                       LigmaSamplePointEditor *editor);
static void   ligma_sample_point_editor_point_moved    (LigmaImage             *image,
                                                       LigmaSamplePoint       *sample_point,
                                                       LigmaSamplePointEditor *editor);
static void   ligma_sample_point_editor_proj_update    (LigmaImage             *image,
                                                       gboolean               now,
                                                       gint                   x,
                                                       gint                   y,
                                                       gint                   width,
                                                       gint                   height,
                                                       LigmaSamplePointEditor *editor);
static void   ligma_sample_point_editor_points_changed (LigmaSamplePointEditor *editor);
static void   ligma_sample_point_editor_dirty          (LigmaSamplePointEditor *editor,
                                                       gint                   index);
static gboolean ligma_sample_point_editor_update       (LigmaSamplePointEditor *editor);
static void     ligma_sample_point_editor_mode_notify  (LigmaColorFrame        *frame,
                                                       const GParamSpec      *pspec,
                                                       LigmaSamplePointEditor *editor);


G_DEFINE_TYPE (LigmaSamplePointEditor, ligma_sample_point_editor,
               LIGMA_TYPE_IMAGE_EDITOR)

#define parent_class ligma_sample_point_editor_parent_class


static void
ligma_sample_point_editor_class_init (LigmaSamplePointEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  GtkWidgetClass       *widget_class       = GTK_WIDGET_CLASS (klass);
  LigmaImageEditorClass *image_editor_class = LIGMA_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = ligma_sample_point_editor_constructed;
  object_class->dispose         = ligma_sample_point_editor_dispose;
  object_class->get_property    = ligma_sample_point_editor_get_property;
  object_class->set_property    = ligma_sample_point_editor_set_property;

  widget_class->style_updated   = ligma_sample_point_editor_style_updated;

  image_editor_class->set_image = ligma_sample_point_editor_set_image;

  g_object_class_install_property (object_class, PROP_SAMPLE_MERGED,
                                   g_param_spec_boolean ("sample-merged",
                                                         NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
ligma_sample_point_editor_init (LigmaSamplePointEditor *editor)
{
  GtkWidget *scrolled_window;
  GtkWidget *viewport;
  GtkWidget *vbox;
  gint       content_spacing;

  editor->sample_merged = TRUE;

  gtk_widget_style_get (GTK_WIDGET (editor),
                        "content-spacing", &content_spacing,
                        NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_NONE);
  gtk_box_pack_start (GTK_BOX (editor), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_show (viewport);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (viewport), vbox);
  gtk_widget_show (vbox);

  editor->empty_icon = gtk_image_new_from_icon_name (LIGMA_ICON_SAMPLE_POINT,
                                                     GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (vbox), editor->empty_icon, TRUE, TRUE, 0);
  gtk_widget_show (editor->empty_icon);

  editor->empty_label = gtk_label_new (_("This image\nhas no\nsample points"));
  gtk_label_set_justify (GTK_LABEL (editor->empty_label), GTK_JUSTIFY_CENTER);
  ligma_label_set_attributes (GTK_LABEL (editor->empty_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), editor->empty_label, TRUE, TRUE, 0);

  editor->grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (editor->grid), content_spacing);
  gtk_grid_set_column_spacing (GTK_GRID (editor->grid), content_spacing);
  gtk_box_pack_start (GTK_BOX (vbox), editor->grid, FALSE, FALSE, 0);
  gtk_widget_show (editor->grid);
}

static void
ligma_sample_point_editor_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);
}

static void
ligma_sample_point_editor_dispose (GObject *object)
{
  LigmaSamplePointEditor *editor = LIGMA_SAMPLE_POINT_EDITOR (object);

  g_clear_pointer (&editor->color_frames, g_free);

  if (editor->dirty_idle_id)
    {
      g_source_remove (editor->dirty_idle_id);
      editor->dirty_idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_sample_point_editor_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  LigmaSamplePointEditor *editor = LIGMA_SAMPLE_POINT_EDITOR (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      ligma_sample_point_editor_set_sample_merged (editor,
                                                  g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_sample_point_editor_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  LigmaSamplePointEditor *editor = LIGMA_SAMPLE_POINT_EDITOR (object);

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
ligma_sample_point_editor_style_updated (GtkWidget *widget)
{
  LigmaSamplePointEditor *editor = LIGMA_SAMPLE_POINT_EDITOR (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (editor->grid)
    {
      gint content_spacing;

      gtk_widget_style_get (widget,
                            "content-spacing", &content_spacing,
                            NULL);

      gtk_grid_set_row_spacing (GTK_GRID (editor->grid), content_spacing);
      gtk_grid_set_column_spacing (GTK_GRID (editor->grid), content_spacing);
    }
}

static void
ligma_sample_point_editor_set_image (LigmaImageEditor *image_editor,
                                    LigmaImage       *image)
{
  LigmaSamplePointEditor *editor = LIGMA_SAMPLE_POINT_EDITOR (image_editor);

  if (image_editor->image)
    {
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            ligma_sample_point_editor_point_added,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            ligma_sample_point_editor_point_removed,
                                            editor);
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            ligma_sample_point_editor_point_moved,
                                            editor);

      g_signal_handlers_disconnect_by_func (ligma_image_get_projection (image_editor->image),
                                            ligma_sample_point_editor_proj_update,
                                            editor);
    }

  LIGMA_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image)
    {
      g_signal_connect (image, "sample-point-added",
                        G_CALLBACK (ligma_sample_point_editor_point_added),
                        editor);
      g_signal_connect (image, "sample-point-removed",
                        G_CALLBACK (ligma_sample_point_editor_point_removed),
                        editor);
      g_signal_connect (image, "sample-point-moved",
                        G_CALLBACK (ligma_sample_point_editor_point_moved),
                        editor);

      g_signal_connect (ligma_image_get_projection (image), "update",
                        G_CALLBACK (ligma_sample_point_editor_proj_update),
                        editor);
    }

  gtk_widget_set_visible (editor->empty_icon,
                          image_editor->image == NULL);

  ligma_sample_point_editor_points_changed (editor);
}


/*  public functions  */

GtkWidget *
ligma_sample_point_editor_new (LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (LIGMA_TYPE_SAMPLE_POINT_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<SamplePoints>",
                       "ui-path",         "/sample-points-popup",
                       NULL);
}

void
ligma_sample_point_editor_set_sample_merged (LigmaSamplePointEditor *editor,
                                            gboolean               sample_merged)
{
  g_return_if_fail (LIGMA_IS_SAMPLE_POINT_EDITOR (editor));

  sample_merged = sample_merged ? TRUE : FALSE;

  if (editor->sample_merged != sample_merged)
    {
      editor->sample_merged = sample_merged;

      ligma_sample_point_editor_dirty (editor, -1);

      g_object_notify (G_OBJECT (editor), "sample-merged");
    }
}

gboolean
ligma_sample_point_editor_get_sample_merged (LigmaSamplePointEditor *editor)
{
  g_return_val_if_fail (LIGMA_IS_SAMPLE_POINT_EDITOR (editor), FALSE);

  return editor->sample_merged;
}

/*  private functions  */

static void
ligma_sample_point_editor_point_added (LigmaImage             *image,
                                      LigmaSamplePoint       *sample_point,
                                      LigmaSamplePointEditor *editor)
{
  ligma_sample_point_editor_points_changed (editor);
}

static void
ligma_sample_point_editor_point_removed (LigmaImage             *image,
                                        LigmaSamplePoint       *sample_point,
                                        LigmaSamplePointEditor *editor)
{
  ligma_sample_point_editor_points_changed (editor);
}

static void
ligma_sample_point_editor_point_moved (LigmaImage             *image,
                                      LigmaSamplePoint       *sample_point,
                                      LigmaSamplePointEditor *editor)
{
  gint i = g_list_index (ligma_image_get_sample_points (image), sample_point);

  ligma_sample_point_editor_dirty (editor, i);
}

static void
ligma_sample_point_editor_proj_update (LigmaImage             *image,
                                      gboolean               now,
                                      gint                   x,
                                      gint                   y,
                                      gint                   width,
                                      gint                   height,
                                      LigmaSamplePointEditor *editor)
{
  LigmaImageEditor *image_editor = LIGMA_IMAGE_EDITOR (editor);
  GList           *sample_points;
  gint             n_points     = 0;
  GList           *list;
  gint             i;

  sample_points = ligma_image_get_sample_points (image_editor->image);

  n_points = MIN (editor->n_color_frames, g_list_length (sample_points));

  for (i = 0, list = sample_points;
       i < n_points;
       i++, list = g_list_next (list))
    {
      LigmaSamplePoint *sample_point = list->data;
      gint             sp_x;
      gint             sp_y;

      ligma_sample_point_get_position (sample_point, &sp_x, &sp_y);

      if (sp_x >= x && sp_x < (x + width) &&
          sp_y >= y && sp_y < (y + height))
        {
          ligma_sample_point_editor_dirty (editor, i);
        }
    }
}

static void
ligma_sample_point_editor_points_changed (LigmaSamplePointEditor *editor)
{
  LigmaImageEditor *image_editor = LIGMA_IMAGE_EDITOR (editor);
  GList           *sample_points;
  gint             n_points     = 0;
  gint             i;

  if (image_editor->image)
    {
      sample_points = ligma_image_get_sample_points (image_editor->image);
      n_points = g_list_length (sample_points);
    }

  gtk_widget_set_visible (editor->empty_label,
                          image_editor->image && n_points == 0);

  /*  Keep that many color frames around so they remember their color
   *  model. Let's hope nobody uses more and notices they get reset to
   *  "pixel". See https://gitlab.gnome.org/GNOME/ligma/issues/1805
   */
#define RANDOM_MAGIC 16

  if (n_points < editor->n_color_frames &&
      n_points < RANDOM_MAGIC           &&
      editor->n_color_frames > RANDOM_MAGIC)
    {
      for (i = RANDOM_MAGIC; i < editor->n_color_frames; i++)
        {
          gtk_widget_destroy (editor->color_frames[i]);
        }

      editor->color_frames = g_renew (GtkWidget *, editor->color_frames,
                                      RANDOM_MAGIC);

      editor->n_color_frames = RANDOM_MAGIC;
    }
  else if (n_points > editor->n_color_frames)
    {
      LigmaColorConfig *config;

      config = image_editor->image->ligma->config->color_management;

      editor->color_frames = g_renew (GtkWidget *, editor->color_frames,
                                      n_points);

      for (i = editor->n_color_frames; i < n_points; i++)
        {
          gint row    = i / 2;
          gint column = i % 2;

          editor->color_frames[i] =
            g_object_new (LIGMA_TYPE_COLOR_FRAME,
                          "ligma",           LIGMA (image_editor->image->ligma),
                          "mode",           LIGMA_COLOR_PICK_MODE_PIXEL,
                          "has-number",     TRUE,
                          "number",         i + 1,
                          "has-color-area", TRUE,
                          "has-coords",     TRUE,
                          "hexpand",        TRUE,
                          NULL);

          ligma_color_frame_set_color_config (LIGMA_COLOR_FRAME (editor->color_frames[i]),
                                             config);

          gtk_grid_attach (GTK_GRID (editor->grid), editor->color_frames[i],
                           column, row, 1, 1);

          g_signal_connect_object (editor->color_frames[i], "notify::mode",
                                   G_CALLBACK (ligma_sample_point_editor_mode_notify),
                                   editor, 0);

          g_object_set_data (G_OBJECT (editor->color_frames[i]),
                             "dirty", GINT_TO_POINTER (TRUE));
        }

      editor->n_color_frames = n_points;
    }

  for (i = 0; i < editor->n_color_frames; i++)
    {
      gtk_widget_set_visible (editor->color_frames[i], i < n_points);
    }

  if (n_points > 0)
    ligma_sample_point_editor_dirty (editor, -1);
}

static void
ligma_sample_point_editor_dirty (LigmaSamplePointEditor *editor,
                                gint                   index)
{
  if (index >= 0)
    {
      g_object_set_data (G_OBJECT (editor->color_frames[index]),
                         "dirty", GINT_TO_POINTER (TRUE));
    }
  else
    {
      gint i;

      for (i = 0; i < editor->n_color_frames; i++)
        g_object_set_data (G_OBJECT (editor->color_frames[i]),
                           "dirty", GINT_TO_POINTER (TRUE));
    }

  if (editor->dirty_idle_id)
    g_source_remove (editor->dirty_idle_id);

  editor->dirty_idle_id =
    g_idle_add ((GSourceFunc) ligma_sample_point_editor_update,
                editor);
}

static gboolean
ligma_sample_point_editor_update (LigmaSamplePointEditor *editor)
{
  LigmaImageEditor *image_editor = LIGMA_IMAGE_EDITOR (editor);
  GList           *sample_points;
  gint             n_points;
  GList           *list;
  gint             i;

  editor->dirty_idle_id = 0;

  if (! image_editor->image)
    return FALSE;

  sample_points = ligma_image_get_sample_points (image_editor->image);

  n_points = MIN (editor->n_color_frames, g_list_length (sample_points));

  for (i = 0, list = sample_points;
       i < n_points;
       i++, list = g_list_next (list))
    {
      LigmaColorFrame *color_frame = LIGMA_COLOR_FRAME (editor->color_frames[i]);

      if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (color_frame),
                                              "dirty")))
        {
          LigmaSamplePoint   *sample_point = list->data;
          const Babl        *format;
          gdouble            pixel[4];
          LigmaRGB            color;
          LigmaColorPickMode  pick_mode;
          gint               x;
          gint               y;

          g_object_set_data (G_OBJECT (color_frame),
                             "dirty", GINT_TO_POINTER (FALSE));

          ligma_sample_point_get_position (sample_point, &x, &y);

          if (ligma_image_pick_color (image_editor->image, NULL,
                                     x, y,
                                     FALSE,
                                     editor->sample_merged,
                                     FALSE, 0.0,
                                     &format,
                                     pixel,
                                     &color))
            {
              ligma_color_frame_set_color (color_frame, FALSE,
                                          format, pixel, &color,
                                          x, y);
            }
          else
            {
              ligma_color_frame_set_invalid (color_frame);
            }

          pick_mode = ligma_sample_point_get_pick_mode (sample_point);

          ligma_color_frame_set_mode (color_frame, pick_mode);
        }
    }

  return FALSE;
}

static void
ligma_sample_point_editor_mode_notify (LigmaColorFrame        *frame,
                                      const GParamSpec      *pspec,
                                      LigmaSamplePointEditor *editor)
{
  LigmaImageEditor *image_editor = LIGMA_IMAGE_EDITOR (editor);
  GList           *sample_points;
  gint             n_points;
  GList           *list;
  gint             i;

  sample_points = ligma_image_get_sample_points (image_editor->image);

  n_points = MIN (editor->n_color_frames, g_list_length (sample_points));

  for (i = 0, list = sample_points;
       i < n_points;
       i++, list = g_list_next (list))
    {
      if (LIGMA_COLOR_FRAME (editor->color_frames[i]) == frame)
        {
          LigmaSamplePoint   *sample_point = list->data;
          LigmaColorPickMode  pick_mode;

          g_object_get (frame, "mode", &pick_mode, NULL);

          if (pick_mode != ligma_sample_point_get_pick_mode (sample_point))
            ligma_image_set_sample_point_pick_mode (image_editor->image,
                                                   sample_point,
                                                   pick_mode,
                                                   TRUE);
          break;
        }
    }
}
