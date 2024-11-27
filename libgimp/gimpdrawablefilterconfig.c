/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablefilterconfig.c
 * Copyright (C) 2024 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"


/**
 * GimpDrawableFilterConfig:
 *
 * The base class for [class@DrawableFilter] specific config objects.
 *
 * A drawable filter config is created by a [class@DrawableFilter] using
 * [method@DrawableFilter.get_config] and its properties match the
 * filter's arguments in number, order and type.
 *
 * Since: 3.0
 **/


G_DEFINE_ABSTRACT_TYPE (GimpDrawableFilterConfig, gimp_drawable_filter_config, G_TYPE_OBJECT)

#define parent_class gimp_drawable_filter_config_parent_class


static void
gimp_drawable_filter_config_class_init (GimpDrawableFilterConfigClass *klass)
{
}

static void
gimp_drawable_filter_config_init (GimpDrawableFilterConfig *config)
{
}
