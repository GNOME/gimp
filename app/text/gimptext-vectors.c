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
#include <pango/pangoft2.h>

#include <ft2build.h>
#include FT_GLYPH_H
#include FT_OUTLINE_H

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


static void  gimp_text_render_vectors (PangoFont     *font,
                                       PangoGlyph     glyph,
                                       FT_Int32       flags,
                                       FT_Matrix     *matrix,
                                       gint           x,
                                       gint           y,
                                       RenderContext *context);


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
                         const FT_Vector *vector,
                         GimpCoords      *coords)
{
  coords->x        = context->offset_x + (gdouble) vector->x / 64.0;
  coords->y        = context->offset_y - (gdouble) vector->y / 64.0;
  coords->pressure = GIMP_COORDS_DEFAULT_PRESSURE;
  coords->xtilt    = GIMP_COORDS_DEFAULT_TILT;
  coords->ytilt    = GIMP_COORDS_DEFAULT_TILT;
  coords->wheel    = GIMP_COORDS_DEFAULT_WHEEL;
}

static gint
moveto (const FT_Vector *to,
        gpointer         data)
{
  RenderContext *context = data;
  GimpCoords     start;

#if GIMP_TEXT_DEBUG
  g_printerr ("moveto  %f, %f\n", to->x / 64.0, to->y / 64.0);
#endif

  gimp_text_vector_coords (context, to, &start);

  if (context->stroke)
    gimp_stroke_close (context->stroke);

  context->stroke = gimp_bezier_stroke_new_moveto (&start);

  gimp_vectors_stroke_add (context->vectors, context->stroke);
  g_object_unref (context->stroke);

  return 0;
}

static gint
lineto (const FT_Vector *to,
        gpointer         data)
{
  RenderContext *context = data;
  GimpCoords     end;

#if GIMP_TEXT_DEBUG
  g_printerr ("lineto  %f, %f\n", to->x / 64.0, to->y / 64.0);
#endif

  if (! context->stroke)
    return 0;

  gimp_text_vector_coords (context, to, &end);

  gimp_bezier_stroke_lineto (context->stroke, &end);

  return 0;
}

static gint
conicto (const FT_Vector *ftcontrol,
         const FT_Vector *to,
         gpointer         data)
{
  RenderContext *context = data;
  GimpCoords     control;
  GimpCoords     end;

#if GIMP_TEXT_DEBUG
  g_printerr ("conicto %f, %f\n", to->x / 64.0, to->y / 64.0);
#endif

  if (! context->stroke)
    return 0;

  gimp_text_vector_coords (context, ftcontrol, &control);
  gimp_text_vector_coords (context, to, &end);

  gimp_bezier_stroke_conicto (context->stroke, &control, &end);

  return 0;
}

static gint
cubicto (const FT_Vector *ftcontrol1,
         const FT_Vector *ftcontrol2,
         const FT_Vector *to,
         gpointer         data)
{
  RenderContext *context = data;
  GimpCoords     control1;
  GimpCoords     control2;
  GimpCoords     end;

#if GIMP_TEXT_DEBUG
  g_printerr ("cubicto %f, %f\n", to->x / 64.0, to->y / 64.0);
#endif

  if (! context->stroke)
    return 0;

  gimp_text_vector_coords (context, ftcontrol1, &control1);
  gimp_text_vector_coords (context, ftcontrol2, &control2);
  gimp_text_vector_coords (context, to, &end);

  gimp_bezier_stroke_cubicto (context->stroke, &control1, &control2, &end);

  return 0;
}


static void
gimp_text_render_vectors (PangoFont     *font,
                          PangoGlyph     pango_glyph,
                          FT_Int32       flags,
                          FT_Matrix     *trafo,
                          gint           x,
                          gint           y,
                          RenderContext *context)
{
  const FT_Outline_Funcs  outline_funcs =
  {
    moveto,
    lineto,
    conicto,
    cubicto,
    0,
    0
  };

  FT_Face   face;
  FT_Glyph  glyph;

  face = pango_fc_font_lock_face (PANGO_FC_FONT (font));

  FT_Load_Glyph (face, (FT_UInt) pango_glyph, flags);

  FT_Get_Glyph (face->glyph, &glyph);

  if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
    {
      FT_OutlineGlyph outline_glyph = (FT_OutlineGlyph) glyph;

      context->offset_x = (gdouble) x / PANGO_SCALE;
      context->offset_y = (gdouble) y / PANGO_SCALE;

      FT_Outline_Decompose (&outline_glyph->outline, &outline_funcs, context);
    }

  FT_Done_Glyph (glyph);

  pango_fc_font_unlock_face (PANGO_FC_FONT (font));
}
