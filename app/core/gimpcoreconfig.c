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

#include <glib-object.h>

#include "core-types.h"
#include "libgimptool/gimptooltypes.h"

#include "gimp.h"
#include "gimpcoreconfig.h"


void
gimp_core_config_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->config = g_new0 (GimpCoreConfig, 1);

  gimp->config->interpolation_type       = GIMP_INTERPOLATION_LINEAR;

  gimp->config->tool_plug_in_path        = NULL;
  gimp->config->plug_in_path             = NULL;
  gimp->config->module_path              = NULL;

  gimp->config->brush_path               = NULL;
  gimp->config->pattern_path             = NULL;
  gimp->config->palette_path             = NULL;
  gimp->config->gradient_path            = NULL;

  gimp->config->default_brush            = NULL;
  gimp->config->default_pattern          = NULL;
  gimp->config->default_palette          = NULL;
  gimp->config->default_gradient         = NULL;

  gimp->config->default_comment          = NULL;
  gimp->config->default_type             = GIMP_RGB;
  gimp->config->default_width            = 256;
  gimp->config->default_height           = 256;
  gimp->config->default_units            = GIMP_UNIT_INCH;
  gimp->config->default_xresolution      = 72.0;
  gimp->config->default_yresolution      = 72.0;
  gimp->config->default_resolution_units = GIMP_UNIT_INCH;

  gimp->config->levels_of_undo           = 5;
  gimp->config->pluginrc_path            = NULL;
  gimp->config->module_db_load_inhibit   = NULL;
  gimp->config->thumbnail_size           = GIMP_THUMBNAIL_SIZE_NORMAL;
}
