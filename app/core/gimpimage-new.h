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

#ifndef __GIMP_IMAGE_NEW_H__
#define __GIMP_IMAGE_NEW_H__


typedef struct
{
  GimpImageBaseType  type;
  gchar             *name;
} GimpImageBaseTypeName;

typedef struct
{
  GimpFillType  type;
  gchar        *name;
} GimpFillTypeName;


struct _GimpImageNewValues
{
  gint               width;
  gint               height;
  GimpUnit           unit;

  gdouble            xresolution;
  gdouble            yresolution;
  GimpUnit           res_unit;
  
  GimpImageBaseType  type;
  GimpFillType       fill_type;
};


void         gimp_image_new_init                  (Gimp               *gimp);
void         gimp_image_new_exit                  (Gimp               *gimp);

GList      * gimp_image_new_get_base_type_names   (Gimp               *gimp);
GList      * gimp_image_new_get_fill_type_names   (Gimp               *gimp);

GimpImageNewValues * gimp_image_new_values_new    (Gimp               *gimp,
						   GimpImage          *gimage);

void         gimp_image_new_set_default_values    (Gimp               *gimp,
						   GimpImageNewValues *values);
void         gimp_image_new_values_free           (GimpImageNewValues *values);

gsize        gimp_image_new_calculate_memsize     (GimpImageNewValues *values);
gchar      * gimp_image_new_get_memsize_string    (gsize               memsize);

void   gimp_image_new_set_have_current_cut_buffer (Gimp               *gimp);

GimpImage  * gimp_image_new_create_image          (Gimp               *gimp,
						   GimpImageNewValues *values);


#endif /* __GIMP_IMAGE_NEW__ */
