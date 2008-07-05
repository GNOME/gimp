/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include <glib-object.h>

#define PANGO_ENABLE_ENGINE
#include <cairo.h>
#include <pango/pangocairo.h>


#include "text-types.h"

#include "core/gimpimage.h"

#include "vectors/gimpbezierstroke.h"
#include "vectors/gimpvectors.h"
#include "vectors/gimpanchor.h"

#include "gimptext.h"
#include "gimptext-private.h"
#include "gimptext-vectors.h"
#include "gimptextlayout.h"
#include "gimptextlayout-render.h"


/* for compatibility with older freetype versions */
#ifndef FT_GLYPH_FORMAT_OUTLINE
#define FT_GLYPH_FORMAT_OUTLINE ft_glyph_format_outline
#endif

typedef struct _RenderContext  RenderContext;

struct _RenderContext
{
  GimpVectors  *vectors;
  GimpStroke   *stroke;
  GimpAnchor   *anchor;
  gdouble       offset_x;
  gdouble       offset_y;
};


static void  gimp_text_render_vectors (PangoFont            *font,
                                       PangoGlyph            glyph,
                                       cairo_font_options_t *options,
                                       cairo_matrix_t       *cmatrix,
                                       gint                  x,
                                       gint                  y,
                                       RenderContext        *context);


GimpVectors *
gimp_text_vectors_new (GimpImage *image,
                       GimpText  *text)
{
  GimpVectors    *vectors;
  GimpTextLayout *layout;
  RenderContext   context = { 0, };

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  vectors = gimp_vectors_new (image, NULL);

  if (text->text)
    {
      gimp_object_set_name_safe (GIMP_OBJECT (vectors), text->text);

      layout = gimp_text_layout_new (text, image);

      context.vectors = vectors;

      gimp_text_layout_render (layout,
                               (GimpTextRenderFunc) gimp_text_render_vectors,
                               &context);

      g_object_unref (layout);

      if (context.stroke)
        gimp_stroke_close (context.stroke);
    }

  return vectors;
}


static inline void
gimp_text_vector_coords (RenderContext   *context,
                         const double x,
                         const double y,
                         GimpCoords      *coords)
{
  coords->x        = context->offset_x + (gdouble) x;
  coords->y        = context->offset_y + (gdouble) y;
  coords->pressure = GIMP_COORDS_DEFAULT_PRESSURE;
  coords->xtilt    = GIMP_COORDS_DEFAULT_TILT;
  coords->ytilt    = GIMP_COORDS_DEFAULT_TILT;
  coords->wheel    = GIMP_COORDS_DEFAULT_WHEEL;
}

static gint
moveto (RenderContext *context,
        const double x,
        const double y)
{
  GimpCoords     start;

#if GIMP_TEXT_DEBUG
  g_printerr ("moveto  %f, %f\n", x, y);
#endif

  gimp_text_vector_coords (context, x, y, &start);

  if (context->stroke)
    gimp_stroke_close (context->stroke);

  context->stroke = gimp_bezier_stroke_new_moveto (&start);

  gimp_vectors_stroke_add (context->vectors, context->stroke);
  g_object_unref (context->stroke);

  return 0;
}

static gint
lineto (RenderContext *context,
        const double x,
        const double y)
{
  GimpCoords     end;

#if GIMP_TEXT_DEBUG
  g_printerr ("lineto  %f, %f\n", x, y);
#endif

  if (! context->stroke)
    return 0;

  gimp_text_vector_coords (context, x, y, &end);

  gimp_bezier_stroke_lineto (context->stroke, &end);

  return 0;
}

static gint
cubicto (RenderContext* context,
         const double x1,
         const double y1,
         const double x2,
         const double y2,
         const double x3,
         const double y3)
{
  GimpCoords     control1;
  GimpCoords     control2;
  GimpCoords     end;

#if GIMP_TEXT_DEBUG
  g_printerr ("cubicto %f, %f\n", x3, y3);
#endif

  if (! context->stroke)
    return 0;

  gimp_text_vector_coords (context, x1, y1, &control1);
  gimp_text_vector_coords (context, x2, y2, &control2);
  gimp_text_vector_coords (context, x3, y3, &end);

  gimp_bezier_stroke_cubicto (context->stroke, &control1, &control2, &end);

  return 0;
}

static gint
closepath (RenderContext *context)
{

#if GIMP_TEXT_DEBUG
  g_printerr ("moveto\n");
#endif

  if (!context->stroke)
    return 0;

  gimp_stroke_close (context->stroke);

  context->stroke = NULL;

  return 0;
}


static void
gimp_text_render_vectors (PangoFont            *font,
                          PangoGlyph            pango_glyph,
                          cairo_font_options_t *options,
                          cairo_matrix_t       *matrix,
                          gint                  x,
                          gint                  y,
                          RenderContext        *context)
{
  cairo_surface_t     *surface;
  cairo_t             *cr;
  cairo_path_t        *cpath;
  cairo_scaled_font_t *cfont;
  cairo_glyph_t        cglyph;

  int i;

  context->offset_x = (gdouble) x / PANGO_SCALE;
  context->offset_y = (gdouble) y / PANGO_SCALE;

  cglyph.x =     0;
  cglyph.y =     0;
  cglyph.index = pango_glyph;
    
  /* A cairo_t needs an image surface to function, so "surface" is created
   * temporarily for this purpose. Nothing is drawn to "surface", but it is
   * still needed to be connected to "cr" for "cr" to execute 
   * cr_glyph_path(). The size of surface is therefore irrelevant.
   */
  surface = cairo_image_surface_create ( CAIRO_FORMAT_A8, 2, 2);
  cr = cairo_create (surface);

  cfont = pango_cairo_font_get_scaled_font ( (PangoCairoFont*) font);

  cairo_set_scaled_font (cr, cfont);
  
  cairo_set_font_options (cr, options);

  cairo_transform (cr, matrix);

  cairo_glyph_path (cr, &cglyph, 1);

  cpath = cairo_copy_path (cr);

  for (i = 0; i < cpath->num_data; i += cpath->data[i].header.length)
  {
    cairo_path_data_t   *data;

    data = &cpath->data[i];

    /* if the drawing operation is the final moveto of the glyph,
     * break to avoid creating an empty point. This is because cairo
     * always adds a moveto after each closepath.
     */
    if (i + data->header.length >= cpath->num_data) break;

    switch (data->header.type)
    {
    case CAIRO_PATH_MOVE_TO:
      moveto (context, data[1].point.x, data[1].point.y);
      break;
    case CAIRO_PATH_LINE_TO:
      lineto (context, data[1].point.x, data[1].point.y);
      break;
    case CAIRO_PATH_CURVE_TO:
      cubicto (context,
               data[1].point.x, data[1].point.y,
               data[2].point.x, data[2].point.y,
               data[3].point.x, data[3].point.y);
      break;
    case CAIRO_PATH_CLOSE_PATH:
      closepath (context);
      break;
    }
  }

  cairo_path_destroy (cpath);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);

}
