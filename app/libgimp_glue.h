/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __LIBGIMP_GLUE_H__
#define __LIBGIMP_GLUE_H__


/*  This files lets various libgimp files link against the application.
 *
 *  NEVER include this header, it's only here for documentation.
 */


gboolean   gimp_palette_set_foreground            (const GimpRGB *color);
gboolean   gimp_palette_get_foreground            (GimpRGB       *color);
gboolean   gimp_palette_set_background            (const GimpRGB *color);
gboolean   gimp_palette_get_background            (GimpRGB       *color);

gint       gimp_unit_get_number_of_units          (void);
gint       gimp_unit_get_number_of_built_in_units (void);
GimpUnit   gimp_unit_new                          (gchar         *identifier,
						   gdouble        factor,
						   gint           digits,
						   gchar         *symbol,
						   gchar         *abbreviation,
						   gchar         *singular,
						   gchar         *plural);
gboolean   gimp_unit_get_deletion_flag            (GimpUnit       unit);
void       gimp_unit_set_deletion_flag            (GimpUnit       unit,
						   gboolean       deletion_flag);
gdouble    gimp_unit_get_factor                   (GimpUnit       unit);
gint       gimp_unit_get_digits                   (GimpUnit       unit);
gchar    * gimp_unit_get_identifier               (GimpUnit       unit);
gchar    * gimp_unit_get_symbol                   (GimpUnit       unit);
gchar    * gimp_unit_get_abbreviation             (GimpUnit       unit);
gchar    * gimp_unit_get_singular                 (GimpUnit       unit);
gchar    * gimp_unit_get_plural                   (GimpUnit       unit);


#endif /* __LIBGIMP_GLUE_H__ */
