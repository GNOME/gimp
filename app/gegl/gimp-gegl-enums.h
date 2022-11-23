/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-gegl-enums.h
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

#ifndef __LIGMA_GEGL_ENUMS_H__
#define __LIGMA_GEGL_ENUMS_H__


#define LIGMA_TYPE_CAGE_MODE (ligma_cage_mode_get_type ())

GType ligma_cage_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_CAGE_MODE_CAGE_CHANGE, /*< desc="Create or adjust the cage"            >*/
  LIGMA_CAGE_MODE_DEFORM       /*< desc="Deform the cage\nto deform the image" >*/
} LigmaCageMode;


#endif /* __LIGMA_GEGL_ENUMS_H__ */
