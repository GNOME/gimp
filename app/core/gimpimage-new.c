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

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimp.h"
#include "gimpcoreconfig.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-new.h"
#include "gimplayer.h"

#include "libgimp/gimpintl.h"


void
gimp_image_new_init (Gimp *gimp)
{
  GimpImageBaseTypeName *new_type;
  GimpFillTypeName      *new_fill_type;

  /* Available Image Base Types */

  new_type = g_new (GimpImageBaseTypeName, 1);
  new_type->type = RGB;
  new_type->name = _("RGB");

  gimp->image_base_type_names = g_list_append (gimp->image_base_type_names,
					       new_type);

  new_type = g_new (GimpImageBaseTypeName, 1);
  new_type->type = GRAY;
  new_type->name = _("Grayscale");

  gimp->image_base_type_names = g_list_append (gimp->image_base_type_names,
					       new_type);
  
  /* Available Fill Types */

  new_fill_type = g_new (GimpFillTypeName, 1);
  new_fill_type->type = FOREGROUND_FILL;
  new_fill_type->name = _("Foreground");

  gimp->fill_type_names = g_list_append (gimp->fill_type_names, new_fill_type);

  new_fill_type = g_new (GimpFillTypeName, 1);
  new_fill_type->type = BACKGROUND_FILL;
  new_fill_type->name = _("Background");

  gimp->fill_type_names = g_list_append (gimp->fill_type_names, new_fill_type);

  new_fill_type = g_new (GimpFillTypeName, 1);
  new_fill_type->type = WHITE_FILL;
  new_fill_type->name = _("White");

  gimp->fill_type_names = g_list_append (gimp->fill_type_names, new_fill_type);

  new_fill_type = g_new (GimpFillTypeName, 1);
  new_fill_type->type = TRANSPARENT_FILL;
  new_fill_type->name = _("Transparent");

  gimp->fill_type_names = g_list_append (gimp->fill_type_names, new_fill_type);

  /* Set the last values used to default values. */

  gimp->image_new_last_values.width       = core_config->default_width;
  gimp->image_new_last_values.height      = core_config->default_height;
  gimp->image_new_last_values.unit        = core_config->default_units;
  gimp->image_new_last_values.xresolution = core_config->default_xresolution;
  gimp->image_new_last_values.yresolution = core_config->default_yresolution;
  gimp->image_new_last_values.res_unit    = core_config->default_resolution_units;
  gimp->image_new_last_values.type        = core_config->default_type;
  gimp->image_new_last_values.fill_type   = BACKGROUND_FILL;

  gimp->have_current_cut_buffer = FALSE;
}

void
gimp_image_new_exit (Gimp *gimp)
{
  g_list_foreach (gimp->image_base_type_names, (GFunc) g_free, NULL);
  g_list_free (gimp->image_base_type_names);
  gimp->image_base_type_names = NULL;

  g_list_foreach (gimp->fill_type_names, (GFunc) g_free, NULL);
  g_list_free (gimp->fill_type_names);
  gimp->fill_type_names = NULL;
}

GList *
gimp_image_new_get_base_type_names (Gimp *gimp)
{
  return gimp->image_base_type_names;
}

GList *
gimp_image_new_get_fill_type_names (Gimp *gimp)
{
  return gimp->fill_type_names;
}

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

      if (values->type == INDEXED)
        values->type = RGB; /* no indexed images */
    }
  else
    {
      memcpy (values, &gimp->image_new_last_values, sizeof (GimpImageNewValues));
    }

  if (gimp->global_buffer && gimp->have_current_cut_buffer)
    {
      values->width  = tile_manager_width (gimp->global_buffer);
      values->height = tile_manager_height (gimp->global_buffer);
    }

  return values;
}

void
gimp_image_new_set_default_values (Gimp               *gimp,
				   GimpImageNewValues *values)
{
  g_return_if_fail (values != NULL);

  memcpy (&gimp->image_new_last_values, values, sizeof (GimpImageNewValues));

  gimp->have_current_cut_buffer = FALSE;
}

void
gimp_image_new_values_free (GimpImageNewValues *values)
{
  g_return_if_fail (values != NULL);

  g_free (values);
}

gdouble
gimp_image_new_calculate_size (GimpImageNewValues *values)
{
  gdouble width, height;
  gdouble size;

  width  = (gdouble) values->width;
  height = (gdouble) values->height;

  size = 
    width * height *
      ((values->type == RGB ? 3 : 1) +                   /* bytes per pixel */
       (values->fill_type == TRANSPARENT_FILL ? 1 : 0)); /* alpha channel */

  return size;
}

gchar *
gimp_image_new_get_size_string (gdouble size)
{
  if (size < 4096)
    return g_strdup_printf (_("%d Bytes"), (gint) size);
  else if (size < 1024 * 10)
    return g_strdup_printf (_("%.2f KB"), size / 1024);
  else if (size < 1024 * 100)
    return g_strdup_printf (_("%.1f KB"), size / 1024);
  else if (size < 1024 * 1024)
    return g_strdup_printf (_("%d KB"), (gint) size / 1024);
  else if (size < 1024 * 1024 * 10)
    return g_strdup_printf (_("%.2f MB"), size / 1024 / 1024);
  else
    return g_strdup_printf (_("%.1f MB"), size / 1024 / 1024);
}

void
gimp_image_new_set_have_current_cut_buffer (Gimp *gimp)
{
  gimp->have_current_cut_buffer = TRUE;
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
    case FOREGROUND_FILL:
    case BACKGROUND_FILL:
    case WHITE_FILL:
      type = (values->type == RGB) ? RGB_GIMAGE : GRAY_GIMAGE;
      break;
    case TRANSPARENT_FILL:
      type = (values->type == RGB) ? RGBA_GIMAGE : GRAYA_GIMAGE;
      break;
    default:
      type = RGB_GIMAGE;
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
			  OPAQUE_OPACITY, NORMAL_MODE);
 
  if (layer)
    {
      gimp_image_undo_disable (gimage);
      gimp_image_add_layer (gimage, layer, 0);
      gimp_image_undo_enable (gimage);

      gimp_drawable_fill_by_type (GIMP_DRAWABLE (layer),
				  gimp_get_current_context (gimp),
				  values->fill_type);

      gimp_image_clean_all (gimage);

      gimp_create_display (gimp, gimage);
    }

  return gimage;
}
