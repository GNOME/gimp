/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_PICKABLE_AUTO_SHRINK_H__
#define __LIGMA_PICKABLE_AUTO_SHRINK_H__


typedef enum
{
  LIGMA_AUTO_SHRINK_SHRINK,
  LIGMA_AUTO_SHRINK_EMPTY,
  LIGMA_AUTO_SHRINK_UNSHRINKABLE
} LigmaAutoShrink;


LigmaAutoShrink   ligma_pickable_auto_shrink (LigmaPickable *pickable,
                                            gint         x,
                                            gint         y,
                                            gint         width,
                                            gint         height,
                                            gint        *shrunk_x,
                                            gint        *shrunk_y,
                                            gint        *shrunk_width,
                                            gint        *shrunk_height);


#endif  /* __LIGMA_PICKABLE_AUTO_SHRINK_H__ */
