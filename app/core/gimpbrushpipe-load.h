/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Adrian Likins and Tor Lillqvist
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

#ifndef __LIGMA_BRUSH_PIPE_LOAD_H__
#define __LIGMA_BRUSH_PIPE_LOAD_H__


#define LIGMA_BRUSH_PIPE_FILE_EXTENSION ".gih"


GList * ligma_brush_pipe_load (LigmaContext   *context,
                              GFile         *file,
                              GInputStream  *input,
                              GError       **error);


#endif  /* __LIGMA_BRUSH_PIPE_LOAD_H__ */
