/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "paint-funcs/paint-funcs.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpbuffer.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-new.h"
#include "gimplayer.h"

#include "gimp-intl.h"


GimpImageNewValues *
gimp_image_new_values_new (Gimp      *gimp,
			   GimpImage *gimage)
{
  GimpImageNewValues *values;

  values = g_new0 (GimpImageNewValues, 1);

  if (gimage)
    {
      values->width  = gimp_image_get_width (gimage);
      values->height = gimp_image_get_height (gimage);
      values->unit   = gimp_image_get_unit (gimage);

      gimp_image_get_resolution (gimage,
				 &values->xresolution, 
				 &values->yresolution);

      values->type = gimp_image_base_type (gimage);

      if (values->type == GIMP_INDEXED)
        values->type = GIMP_RGB; /* no indexed images */
      
      values->fill_type = GIMP_BACKGROUND_FILL;
    }
  else
    {
      *values = gimp->image_new_last_values;
    }

  if (gimp->global_buffer && gimp->have_current_cut_buffer)
    {
      values->width  = gimp_buffer_get_width (gimp->global_buffer);
      values->height = gimp_buffer_get_height (gimp->global_buffer);
    }

  return values;
}

void
gimp_image_new_set_default_values (Gimp               *gimp,
				   GimpImageNewValues *values)
{
  g_return_if_fail (values != NULL);

  gimp->image_new_last_values = *values;

  gimp->have_current_cut_buffer = FALSE;
}

void
gimp_image_new_values_free (GimpImageNewValues *values)
{
  g_return_if_fail (values != NULL);

  g_free (values);
}

gsize
gimp_image_new_calculate_memsize (GimpImageNewValues *values)
{
  gint channels;

  channels = ((values->type == GIMP_RGB ? 3 : 1)           /* color     */ +
              (values->fill_type == GIMP_TRANSPARENT_FILL) /* alpha     */ +
              1                                            /* selection */);

  return channels * values->width * values->height;
}

gchar *
gimp_image_new_get_memsize_string (gsize memsize)
{
  if (memsize < 4096)
    {
      return g_strdup_printf (_("%d Bytes"), (gint) memsize);
    }
  else if (memsize < 1024 * 10)
    {
      return g_strdup_printf (_("%.2f KB"), (gdouble) memsize / 1024.0);
    }
  else if (memsize < 1024 * 100)
    {
      return g_strdup_printf (_("%.1f KB"), (gdouble) memsize / 1024.0);
    }
  else if (memsize < 1024 * 1024)
    {
      return g_strdup_printf (_("%d KB"), (gint) memsize / 1024);
    }
  else if (memsize < 1024 * 1024 * 10)
    {
      return g_strdup_printf (_("%.2f MB"), (gdouble) memsize / 1024.0 / 1024.0);
    }
  else
    {
      return g_strdup_printf (_("%.1f MB"), (gdouble) memsize / 1024.0 / 1024.0);
    }
}

GimpImage *
gimp_image_new_create_image (Gimp               *gimp,
			     GimpImageNewValues *values)
{
  GimpImage     *gimage;
  GimpLayer     *layer;
  GimpImageType  type;
  gint           width, height;

  g_return_val_if_fail (values != NULL, NULL);

  gimp_image_new_set_default_values (gimp, values);

  switch (values->fill_type)
    {
    case GIMP_FOREGROUND_FILL:
    case GIMP_BACKGROUND_FILL:
    case GIMP_WHITE_FILL:
      type = (values->type == GIMP_RGB) ? GIMP_RGB_IMAGE : GIMP_GRAY_IMAGE;
      break;
    case GIMP_TRANSPARENT_FILL:
      type = (values->type == GIMP_RGB) ? GIMP_RGBA_IMAGE : GIMP_GRAYA_IMAGE;
      break;
    default:
      type = GIMP_RGB_IMAGE;
      break;
    }

  gimage = gimp_create_image (gimp,
			      values->width, values->height,
			      values->type,
			      TRUE);

  gimp_image_set_resolution (gimage, values->xresolution, values->yresolution);
  gimp_image_set_unit (gimage, values->unit);

  width  = gimp_image_get_width (gimage);
  height = gimp_image_get_height (gimage);

  layer = gimp_layer_new (gimage, width, height,
			  type, _("Background"),
			  GIMP_OPACITY_OPAQUE, GIMP_NORMAL_MODE);
 
  if (layer)
    {
      gimp_image_undo_disable (gimage);
      gimp_image_add_layer (gimage, layer, 0);
      gimp_image_undo_enable (gimage);

      gimp_drawable_fill_by_type (GIMP_DRAWABLE (layer),
				  gimp_get_current_context (gimp),
				  values->fill_type);

      gimp_image_clean_all (gimage);

      gimp_create_display (gimp, gimage, 0x0101);

      g_object_unref (gimage);
    }

  return gimage;
}
