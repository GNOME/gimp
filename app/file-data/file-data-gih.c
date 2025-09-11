/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpparasiteio.h"
#include "libgimpcolor/gimpcolor.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpbrushpipe.h"
#include "core/gimpbrushpipe-load.h"
#include "core/gimpbrush-private.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimplayer-new.h"
#include "core/gimpparamspecs.h"
#include "core/gimptempbuf.h"

#include "pdb/gimpprocedure.h"

#include "file-data-gbr.h"
#include "file-data-gih.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GimpImage     * file_gih_pipe_to_image (Gimp          *gimp,
                                               GimpBrushPipe *pipe);
static GimpBrushPipe * file_gih_image_to_pipe (GimpImage     *image,
                                               const gchar   *name,
                                               gdouble        spacing,
                                               const gchar   *paramstring);


/*  public functions  */

GimpValueArray *
file_gih_load_invoker (GimpProcedure         *procedure,
                       Gimp                  *gimp,
                       GimpContext           *context,
                       GimpProgress          *progress,
                       const GimpValueArray  *args,
                       GError               **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image = NULL;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  gimp_set_busy (gimp);

  file = g_value_get_object (gimp_value_array_index (args, 1));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      GList *list = gimp_brush_pipe_load (context, file, input, error);

      if (list)
        {
          GimpBrushPipe *pipe = list->data;

          g_list_free (list);

          image = file_gih_pipe_to_image (gimp, pipe);
          g_object_unref (pipe);
        }

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  gimp_file_get_utf8_name (file));
    }

  return_vals = gimp_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    g_value_set_object (gimp_value_array_index (return_vals, 1), image);

  gimp_unset_busy (gimp);

  return return_vals;
}

GimpValueArray *
file_gih_save_invoker (GimpProcedure         *procedure,
                       Gimp                  *gimp,
                       GimpContext           *context,
                       GimpProgress          *progress,
                       const GimpValueArray  *args,
                       GError               **error)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GimpBrushPipe  *pipe;
  const gchar    *name;
  const gchar    *params;
  GFile          *file;
  gint            spacing;
  gboolean        success;

  gimp_set_busy (gimp);

  image   = g_value_get_object (gimp_value_array_index (args, 1));
  /* XXX: drawable list is currently unused. GIH saving just uses the
   * whole layer list.
   */
  file    = g_value_get_object (gimp_value_array_index (args, 3));
  spacing = g_value_get_int    (gimp_value_array_index (args, 4));
  name    = g_value_get_string (gimp_value_array_index (args, 5));
  params  = g_value_get_string (gimp_value_array_index (args, 6));

  pipe = file_gih_image_to_pipe (image, name, spacing, params);

  gimp_data_set_file (GIMP_DATA (pipe), file, TRUE, TRUE);

  success = gimp_data_save (GIMP_DATA (pipe), error);

  g_object_unref (pipe);

  return_vals = gimp_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  gimp_unset_busy (gimp);

  return return_vals;
}


/*  private functions  */

static GimpImage *
file_gih_pipe_to_image (Gimp          *gimp,
                        GimpBrushPipe *pipe)
{
  GimpImage         *image;
  const gchar       *name;
  GimpImageBaseType  base_type;
  GimpParasite      *parasite;
  gchar              spacing[8];
  gint               i;

  if (gimp_brush_get_pixmap (pipe->current))
    base_type = GIMP_RGB;
  else
    base_type = GIMP_GRAY;

  name = gimp_object_get_name (pipe);

  image = gimp_image_new (gimp, 1, 1, base_type,
                          GIMP_PRECISION_U8_NON_LINEAR);

  parasite = gimp_parasite_new ("gimp-brush-pipe-name",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (name) + 1, name);
  gimp_image_parasite_attach (image, parasite, FALSE);
  gimp_parasite_free (parasite);

  g_snprintf (spacing, sizeof (spacing), "%d",
              gimp_brush_get_spacing (GIMP_BRUSH (pipe)));

  parasite = gimp_parasite_new ("gimp-brush-pipe-spacing",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (spacing) + 1, spacing);
  gimp_image_parasite_attach (image, parasite, FALSE);
  gimp_parasite_free (parasite);

  for (i = 0; i < pipe->n_brushes; i++)
    {
      GimpLayer *layer;

      layer = file_gbr_brush_to_layer (image, pipe->brushes[i]);
      gimp_image_add_layer (image, layer, NULL, i, FALSE);
    }

  if (pipe->params)
    {
      GimpPixPipeParams  params;
      gchar             *paramstring;

      /* Since we do not (yet) load the pipe as described in the
       * header, but use one layer per brush, we have to alter the
       * paramstring before attaching it as a parasite.
       *
       * (this comment copied over from file-gih, whatever "as
       * described in the header" means) -- mitch
       */

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      gimp_pixpipe_params_init (&params);
      gimp_pixpipe_params_parse (pipe->params, &params);
      G_GNUC_END_IGNORE_DEPRECATIONS

      params.cellwidth  = gimp_image_get_width  (image);
      params.cellheight = gimp_image_get_height (image);
      params.cols       = 1;
      params.rows       = 1;

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      paramstring = gimp_pixpipe_params_build (&params);
      G_GNUC_END_IGNORE_DEPRECATIONS
      if (paramstring)
        {
          parasite = gimp_parasite_new ("gimp-brush-pipe-parameters",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (paramstring) + 1,
                                        paramstring);
          gimp_image_parasite_attach (image, parasite, FALSE);
          gimp_parasite_free (parasite);
          g_free (paramstring);
        }

      G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      gimp_pixpipe_params_free (&params);
      G_GNUC_END_IGNORE_DEPRECATIONS
    }

  return image;
}

static GimpBrushPipe *
file_gih_image_to_pipe (GimpImage   *image,
                        const gchar *name,
                        gdouble      spacing,
                        const gchar *paramstring)
{
  GimpBrushPipe     *pipe;
  GimpPixPipeParams  params;
  GList             *layers;
  GList             *list;
  GList             *brushes = NULL;
  gint               image_width;
  gint               image_height;
  gint               i;

  pipe = g_object_new (GIMP_TYPE_BRUSH_PIPE,
                       "name",      name,
                       "mime-type", "image/x-gimp-gih",
                       "spacing",   spacing,
                       NULL);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gimp_pixpipe_params_init (&params);
  gimp_pixpipe_params_parse (paramstring, &params);
  G_GNUC_END_IGNORE_DEPRECATIONS

  image_width  = gimp_image_get_width  (image);
  image_height = gimp_image_get_height (image);

  layers = gimp_image_get_layer_iter (image);

  for (list = layers; list; list = g_list_next (list))
    {
      GimpLayer *layer = list->data;
      gint       width;
      gint       height;
      gint       offset_x;
      gint       offset_y;
      gint       row;

      width  = gimp_item_get_width  (GIMP_ITEM (layer));
      height = gimp_item_get_height (GIMP_ITEM (layer));

      gimp_item_get_offset (GIMP_ITEM (layer), &offset_x, &offset_y);

      /* Since we assume positive layer offsets we need to make sure this
       * is always the case or we will crash for grayscale layers.
       * See issue #6436. */
      if (offset_x < 0)
        {
          g_warning (_("Negative x offset: %d for layer %s corrected."),
                     offset_x, gimp_object_get_name (layer));
          width += offset_x;
          offset_x = 0;
        }
      if (offset_y < 0)
        {
          g_warning (_("Negative y offset: %d for layer %s corrected."),
                     offset_y, gimp_object_get_name (layer));
          height += offset_y;
          offset_y = 0;
        }

      for (row = 0; row < params.rows; row++)
        {
          gint y, ynext;
          gint thisy, thish;
          gint col;

          y     = (row * image_height) / params.rows;
          ynext = ((row + 1) * image_height / params.rows);

          /* Assume layer is offset to positive direction in x and y.
           * That's reasonable, as otherwise all of the layer
           * won't be visible.
           * thisy and thisx are in the drawable's coordinate space.
           */
          thisy = MAX (0, y - offset_y);
          thish = (ynext - offset_y) - thisy;
          thish = MIN (thish, height - thisy);

          for (col = 0; col < params.cols; col++)
            {
              GimpBrush *brush;
              gint       x, xnext;
              gint       thisx, thisw;

              x     = (col * image_width / params.cols);
              xnext = ((col + 1) * image_width / params.cols);
              thisx = MAX (0, x - offset_x);
              thisw = (xnext - offset_x) - thisx;
              thisw = MIN (thisw, width - thisx);

              brush = file_gbr_drawable_to_brush (GIMP_DRAWABLE (layer),
                                                  GEGL_RECTANGLE (thisx, thisy,
                                                                  thisw, thish),
                                                  gimp_object_get_name (layer),
                                                  spacing);

              brushes = g_list_prepend (brushes, brush);
            }
        }
    }

  brushes = g_list_reverse (brushes);

  pipe->n_brushes = g_list_length (brushes);
  pipe->brushes   = g_new0 (GimpBrush *, pipe->n_brushes);

  for (list = brushes, i = 0; list; list = g_list_next (list), i++)
    pipe->brushes[i] = list->data;

  g_list_free (brushes);

  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  gimp_pixpipe_params_free (&params);
  G_GNUC_END_IGNORE_DEPRECATIONS

  gimp_brush_pipe_set_params (pipe, paramstring);

  return pipe;
}
