/*
 * Copyright (C) 2025 Ondřej Míchal <harrymichal@seznam.cz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define GIMP_GEGL_FILTER_TYPE_INFO gimp_gegl_filter_info_get_type ()
G_DECLARE_FINAL_TYPE (GimpGeglFilterInfo,
                      gimp_gegl_filter_info,
                      GIMP, GEGL_FILTER_INFO,
                      GObject)

G_END_DECLS
