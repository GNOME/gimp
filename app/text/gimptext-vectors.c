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

#include "text/text-types.h"

#include "core/gimpimage.h"
#include "vectors/gimpvectors.h"

#include "gimptext.h"
#include "gimptext-private.h"
#include "gimptext-vectors.h"
#include "gimptextlayout.h"
#include "gimptextlayout-render.h"


static void  gimp_text_render_vectors (PangoFont   *font,
				       PangoGlyph  *glyph,
				       gint         flags,
				       gint         x,
				       gint         y,
				       GimpVectors *vectors);


GimpVectors *
gimp_text_vectors_new (GimpImage *image,
		       GimpText  *text)
{
  GimpVectors    *vectors;
  GimpTextLayout *layout;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_TEXT (text), NULL);

  vectors = gimp_vectors_new (image, NULL);

  if (text->text)
    {
      gimp_object_set_name_safe (GIMP_OBJECT (vectors), text->text);

      layout = gimp_text_layout_new (text, image);

      gimp_text_layout_render (layout,
			       (GimpTextRenderFunc) gimp_text_render_vectors,
			       vectors);

      g_object_unref (layout);
    }

  return vectors;
}


static void
gimp_text_render_vectors (PangoFont   *font,
			  PangoGlyph  *glyph,
			  gint         flags,
			  gint         x,
			  gint         y,
			  GimpVectors *vectors)
{
  /*  FIXME: implement  */
}
