/* GIMP - The GNU Image Manipulation Program
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

#ifndef __SCRIPT_FU_RESOURCE_H__
#define __SCRIPT_FU_RESOURCE_H__


/* ScriptFu internally stores Resources as int IDs.
 * In binding to and from the PDB, converted to GimpResource *
 * (which is a proxy object.)
 */
typedef struct {
  gint32  history;  /* The current value.  Can be -1 meaning invalid. */
  gchar  *declared_name_of_default;
  GType   resource_type;
} SFResourceType;


#endif /*  __SCRIPT_FU_RESOURCE__  */
