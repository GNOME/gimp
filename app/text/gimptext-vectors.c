/* The GIMP -- an image manipulation program
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
#include <pango/pangoft2.h>
#include <freetype/ftoutln.h>

#include "text/text-types.h"

#include "core/gimpimage.h"
#include "vectors/gimpvectors.h"

#include "gimptext.h"
#include "gimptext-private.h"
#include "gimptext-vectors.h"
#include "gimptextlayout.h"
#include "gimptextlayout-render.h"


typedef struct _RenderContext  RenderContext;

struct _RenderContext
{
  GimpVectors  *vectors;
  
  FT_Vector     current_point;
  FT_Vector     last_point;
};


static void  gimp_text_render_vectors (PangoFont     *font,
				       PangoGlyph    *glyph,
				       gint           flags,
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
    }

  return vectors;
}


static gint 
moveto (FT_Vector *to,
	gpointer   data)
{
  RenderContext *context = (RenderContext *) data;

  return 0;
}

static gint 
lineto (FT_Vector *to,
	gpointer   data)       
{
  RenderContext *context = (RenderContext *) data;

  return 0;
}

static gint 
conicto (FT_Vector *control,
	 FT_Vector *to,
	 gpointer   data)
{
  RenderContext *context = (RenderContext *) data;

  return 0;
}

static gint 
cubicto (FT_Vector *control1,
	 FT_Vector *control2,
	 FT_Vector *to,
	 gpointer   data)
{
  RenderContext *context = (RenderContext *) data;

  return 0;
}


static void
gimp_text_render_vectors (PangoFont     *font,
			  PangoGlyph    *glyph,
			  gint           flags,
			  gint           x,
			  gint           y,
			  RenderContext *context)
{
  FT_Face                 face;
  const FT_Outline_Funcs  outline_funcs = 
  {
    moveto,
    lineto,
    conicto,
    cubicto
  };

  face = pango_ft2_font_get_face (font);
  
  FT_Load_Glyph (face, (FT_UInt) glyph, flags);

  FT_Outline_Decompose ((FT_Outline *) face->glyph, &outline_funcs, &context);
}
