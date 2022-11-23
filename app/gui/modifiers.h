/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * modifiers.h
 * Copyright (C) 2022 Jehan
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

#ifndef __MODIFIERS_H__
#define __MODIFIERS_H__


void       modifiers_init    (Ligma       *ligma);
void       modifiers_exit    (Ligma       *ligma);

void       modifiers_restore (Ligma       *ligma);
void       modifiers_save    (Ligma       *ligma,
                              gboolean    always_save);

gboolean   modifiers_clear   (Ligma       *ligma,
                              GError    **error);


#endif  /*  __MODIFIERS_H__  */
