/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#ifndef __LIGMA_TOOLS_H__
#define __LIGMA_TOOLS_H__


void       ligma_tools_init        (Ligma              *ligma);
void       ligma_tools_exit        (Ligma              *ligma);

void       ligma_tools_restore     (Ligma              *ligma);
void       ligma_tools_save        (Ligma              *ligma,
                                   gboolean           save_tool_options,
                                   gboolean           always_save);

gboolean   ligma_tools_clear       (Ligma              *ligma,
                                   GError           **error);

gboolean   ligma_tools_serialize   (Ligma              *ligma,
                                   LigmaContainer     *container,
                                   LigmaConfigWriter  *writer);
gboolean   ligma_tools_deserialize (Ligma              *ligma,
                                   LigmaContainer     *container,
                                   GScanner          *scanner);

void       ligma_tools_reset       (Ligma              *ligma,
                                   LigmaContainer     *container,
                                   gboolean           user_toolrc);


#endif  /* __LIGMA_TOOLS_H__ */
