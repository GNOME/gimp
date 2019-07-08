/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __MAPOBJECT_STOCK_H__
#define __MAPOBJECT_STOCK_H__


#define STOCK_INTENSITY_AMBIENT_LOW        "intensity-ambient-low"
#define STOCK_INTENSITY_AMBIENT_HIGH       "intensity-ambient-high"
#define STOCK_INTENSITY_DIFFUSE_LOW        "intensity-diffuse-low"
#define STOCK_INTENSITY_DIFFUSE_HIGH       "intensity-diffuse-high"
#define STOCK_REFLECTIVITY_DIFFUSE_LOW     "reflectivity-diffuse-low"
#define STOCK_REFLECTIVITY_DIFFUSE_HIGH    "reflectivity-diffuse-high"
#define STOCK_REFLECTIVITY_SPECULAR_LOW    "reflectivity-specular-low"
#define STOCK_REFLECTIVITY_SPECULAR_HIGH   "reflectivity-specular-high"
#define STOCK_REFLECTIVITY_HIGHLIGHT_LOW   "reflectivity-highlight-low"
#define STOCK_REFLECTIVITY_HIGHLIGHT_HIGH  "reflectivity-highlight-high"


void  mapobject_stock_init (void);


#endif /* __MAPOBJECT_STOCK_H__ */
