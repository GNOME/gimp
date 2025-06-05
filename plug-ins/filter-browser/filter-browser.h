/*
 * GIMP plug-in for browsing available GEGL filters.
 *
 * Copyright (C) 2025 Ondřej Míchal <harrymichal@seznam.cz>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

G_BEGIN_DECLS

#define GEGL_FILTER_TYPE_INFO gegl_filter_info_get_type ()
G_DECLARE_FINAL_TYPE (GeglFilterInfo, gegl_filter_info, GEGL, FILTER_INFO, GObject)

G_END_DECLS

#endif
