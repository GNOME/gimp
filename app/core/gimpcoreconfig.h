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

#ifndef __GIMP_CORE_CONFIG_H__
#define __GIMP_CORE_CONFIG_H__


struct _GimpCoreConfig
{
  gchar             *plug_in_path;
  gchar             *module_path;

  gchar             *brush_path;
  gchar             *pattern_path;
  gchar             *palette_path;
  gchar             *gradient_path;

  gchar             *default_brush;
  gchar             *default_pattern;
  gchar             *default_palette;
  gchar             *default_gradient;

  gchar             *default_comment;
  GimpImageBaseType  default_type;
  guint              default_width;
  guint              default_height;
  GimpUnit           default_units;
  gdouble            default_xresolution;
  gdouble            default_yresolution;
  GimpUnit           default_resolution_units;

  guint              levels_of_undo;
  gchar             *pluginrc_path;
  gchar             *module_db_load_inhibit;
  guint              thumbnail_mode;
};


void   gimp_core_config_init (Gimp *gimp);


#endif  /*  __GIMP_CORE_CONFIG_H__  */
