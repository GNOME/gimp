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

#include "config.h"

#include <gtk/gtk.h>

#include "core-types.h"

#include "gimpcoreconfig.h"


static GimpCoreConfig  static_core_config =
{
  /* plug_in_path             */ NULL,
  /* module_path              */ NULL,

  /* brush_path               */ NULL,
  /* pattern_path             */ NULL,
  /* palette_path             */ NULL,
  /* gradient_path            */ NULL,

  /* default_brush            */ NULL,
  /* default_pattern          */ NULL,
  /* default_palette          */ NULL,
  /* default_gradient         */ NULL,

  /* default_comment          */ NULL,
  /* default_type             */ RGB,
  /* default_width            */ 256,
  /* default_height           */ 256,
  /* default_units            */ GIMP_UNIT_INCH,
  /* default_xresolution      */ 72.0,
  /* default_yresolution      */ 72.0,
  /* default_resolution_units */ GIMP_UNIT_INCH,

  /* levels_of_undo           */ 5,
  /* pluginrc_path            */ NULL,
  /* module_db_load_inhibit   */ NULL,
  /* thumbnail_mode           */ 1
};


GimpCoreConfig *core_config = &static_core_config;
