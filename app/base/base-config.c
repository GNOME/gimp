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

#include <glib.h>

#include "base-types.h"

#include "base-config.h"


static GimpBaseConfig  static_base_config =
{
  .temp_path          = NULL,
  .swap_path          = NULL,

  .tile_cache_size    = 33554432,
  .stingy_memory_use  = FALSE,
  .interpolation_type = LINEAR_INTERPOLATION,
  .num_processors     = 1
};


GimpBaseConfig *base_config = &static_base_config;
