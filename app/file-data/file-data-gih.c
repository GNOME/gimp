/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmabase/ligmaparasiteio.h"
#include "libligmacolor/ligmacolor.h"

#include "core/core-types.h"

#include "core/ligma.h"
#include "core/ligmabrushpipe.h"
#include "core/ligmabrushpipe-load.h"
#include "core/ligmabrush-private.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer-new.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmatempbuf.h"

#include "pdb/ligmaprocedure.h"

#include "file-data-gbr.h"
#include "file-data-gih.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static LigmaImage     * file_gih_pipe_to_image (Ligma          *ligma,
                                               LigmaBrushPipe *pipe);
static LigmaBrushPipe * file_gih_image_to_pipe (LigmaImage     *image,
                                               const gchar   *name,
                                               gdouble        spacing,
                                               const gchar   *paramstring);


/*  public functions  */

LigmaValueArray *
file_gih_load_invoker (LigmaProcedure         *procedure,
                       Ligma                  *ligma,
                       LigmaContext           *context,
                       LigmaProgress          *progress,
                       const LigmaValueArray  *args,
                       GError               **error)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image = NULL;
  GFile          *file;
  GInputStream   *input;
  GError         *my_error = NULL;

  ligma_set_busy (ligma);

  file = g_value_get_object (ligma_value_array_index (args, 1));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (input)
    {
      GList *list = ligma_brush_pipe_load (context, file, input, error);

      if (list)
        {
          LigmaBrushPipe *pipe = list->data;

          g_list_free (list);

          image = file_gih_pipe_to_image (ligma, pipe);
          g_object_unref (pipe);
        }

      g_object_unref (input);
    }
  else
    {
      g_propagate_prefixed_error (error, my_error,
                                  _("Could not open '%s' for reading: "),
                                  ligma_file_get_utf8_name (file));
    }

  return_vals = ligma_procedure_get_return_values (procedure, image != NULL,
                                                  error ? *error : NULL);

  if (image)
    g_value_set_object (ligma_value_array_index (return_vals, 1), image);

  ligma_unset_busy (ligma);

  return return_vals;
}

LigmaValueArray *
file_gih_save_invoker (LigmaProcedure         *procedure,
                       Ligma                  *ligma,
                       LigmaContext           *context,
                       LigmaProgress          *progress,
                       const LigmaValueArray  *args,
                       GError               **error)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  LigmaBrushPipe  *pipe;
  const gchar    *name;
  const gchar    *params;
  GFile          *file;
  gint            spacing;
  gboolean        success;

  ligma_set_busy (ligma);

  image   = g_value_get_object (ligma_value_array_index (args, 1));
  /* XXX: drawable list is currently unused. GIH saving just uses the
   * whole layer list.
   */
  file    = g_value_get_object (ligma_value_array_index (args, 4));
  spacing = g_value_get_int    (ligma_value_array_index (args, 5));
  name    = g_value_get_string (ligma_value_array_index (args, 6));
  params  = g_value_get_string (ligma_value_array_index (args, 7));

  pipe = file_gih_image_to_pipe (image, name, spacing, params);

  ligma_data_set_file (LIGMA_DATA (pipe), file, TRUE, TRUE);

  success = ligma_data_save (LIGMA_DATA (pipe), error);

  g_object_unref (pipe);

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  ligma_unset_busy (ligma);

  return return_vals;
}


/*  private functions  */

static LigmaImage *
file_gih_pipe_to_image (Ligma          *ligma,
                        LigmaBrushPipe *pipe)
{
  LigmaImage         *image;
  const gchar       *name;
  LigmaImageBaseType  base_type;
  LigmaParasite      *parasite;
  gchar              spacing[8];
  gint               i;

  if (ligma_brush_get_pixmap (pipe->current))
    base_type = LIGMA_RGB;
  else
    base_type = LIGMA_GRAY;

  name = ligma_object_get_name (pipe);

  image = ligma_image_new (ligma, 1, 1, base_type,
                          LIGMA_PRECISION_U8_NON_LINEAR);

  parasite = ligma_parasite_new ("ligma-brush-pipe-name",
                                LIGMA_PARASITE_PERSISTENT,
                                strlen (name) + 1, name);
  ligma_image_parasite_attach (image, parasite, FALSE);
  ligma_parasite_free (parasite);

  g_snprintf (spacing, sizeof (spacing), "%d",
              ligma_brush_get_spacing (LIGMA_BRUSH (pipe)));

  parasite = ligma_parasite_new ("ligma-brush-pipe-spacing",
                                LIGMA_PARASITE_PERSISTENT,
                                strlen (spacing) + 1, spacing);
  ligma_image_parasite_attach (image, parasite, FALSE);
  ligma_parasite_free (parasite);

  for (i = 0; i < pipe->n_brushes; i++)
    {
      LigmaLayer *layer;

      layer = file_gbr_brush_to_layer (image, pipe->brushes[i]);
      ligma_image_add_layer (image, layer, NULL, i, FALSE);
    }

  if (pipe->params)
    {
      LigmaPixPipeParams  params;
      gchar             *paramstring;

      /* Since we do not (yet) load the pipe as described in the
       * header, but use one layer per brush, we have to alter the
       * paramstring before attaching it as a parasite.
       *
       * (this comment copied over from file-gih, whatever "as
       * described in the header" means) -- mitch
       */

      ligma_pixpipe_params_init (&params);
      ligma_pixpipe_params_parse (pipe->params, &params);

      params.cellwidth  = ligma_image_get_width  (image);
      params.cellheight = ligma_image_get_height (image);
      params.cols       = 1;
      params.rows       = 1;

      paramstring = ligma_pixpipe_params_build (&params);
      if (paramstring)
        {
          parasite = ligma_parasite_new ("ligma-brush-pipe-parameters",
                                        LIGMA_PARASITE_PERSISTENT,
                                        strlen (paramstring) + 1,
                                        paramstring);
          ligma_image_parasite_attach (image, parasite, FALSE);
          ligma_parasite_free (parasite);
          g_free (paramstring);
        }

      ligma_pixpipe_params_free (&params);
    }

  return image;
}

static LigmaBrushPipe *
file_gih_image_to_pipe (LigmaImage   *image,
                        const gchar *name,
                        gdouble      spacing,
                        const gchar *paramstring)
{
  LigmaBrushPipe     *pipe;
  LigmaPixPipeParams  params;
  GList             *layers;
  GList             *list;
  GList             *brushes = NULL;
  gint               image_width;
  gint               image_height;
  gint               i;

  pipe = g_object_new (LIGMA_TYPE_BRUSH_PIPE,
                       "name",      name,
                       "mime-type", "image/x-ligma-gih",
                       "spacing",   spacing,
                       NULL);

  ligma_pixpipe_params_init (&params);
  ligma_pixpipe_params_parse (paramstring, &params);

  image_width  = ligma_image_get_width  (image);
  image_height = ligma_image_get_height (image);

  layers = ligma_image_get_layer_iter (image);

  for (list = layers; list; list = g_list_next (list))
    {
      LigmaLayer *layer = list->data;
      gint       width;
      gint       height;
      gint       offset_x;
      gint       offset_y;
      gint       row;

      width  = ligma_item_get_width  (LIGMA_ITEM (layer));
      height = ligma_item_get_height (LIGMA_ITEM (layer));

      ligma_item_get_offset (LIGMA_ITEM (layer), &offset_x, &offset_y);

      /* Since we assume positive layer offsets we need to make sure this
       * is always the case or we will crash for grayscale layers.
       * See issue #6436. */
      if (offset_x < 0)
        {
          g_warning (_("Negative x offset: %d for layer %s corrected."),
                     offset_x, ligma_object_get_name (layer));
          width += offset_x;
          offset_x = 0;
        }
      if (offset_y < 0)
        {
          g_warning (_("Negative y offset: %d for layer %s corrected."),
                     offset_y, ligma_object_get_name (layer));
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
              LigmaBrush *brush;
              gint       x, xnext;
              gint       thisx, thisw;

              x     = (col * image_width / params.cols);
              xnext = ((col + 1) * image_width / params.cols);
              thisx = MAX (0, x - offset_x);
              thisw = (xnext - offset_x) - thisx;
              thisw = MIN (thisw, width - thisx);

              brush = file_gbr_drawable_to_brush (LIGMA_DRAWABLE (layer),
                                                  GEGL_RECTANGLE (thisx, thisy,
                                                                  thisw, thish),
                                                  ligma_object_get_name (layer),
                                                  spacing);

              brushes = g_list_prepend (brushes, brush);
            }
        }
    }

  brushes = g_list_reverse (brushes);

  pipe->n_brushes = g_list_length (brushes);
  pipe->brushes   = g_new0 (LigmaBrush *, pipe->n_brushes);

  for (list = brushes, i = 0; list; list = g_list_next (list), i++)
    pipe->brushes[i] = list->data;

  g_list_free (brushes);

  ligma_pixpipe_params_free (&params);

  ligma_brush_pipe_set_params (pipe, paramstring);

  return pipe;
}
