/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __PDB_GLUE_H__
#define __PDB_GLUE_H__


#define gimp_layer_get_apply_mask(l)   (l)->mask ? gimp_layer_mask_get_apply((l)->mask) : FALSE;
#define gimp_layer_get_show_mask(l)    (l)->mask ? gimp_layer_mask_get_show((l)->mask) : FALSE;
#define gimp_layer_get_edit_mask(l)    (l)->mask ? gimp_layer_mask_get_edit((l)->mask) : FALSE;

#define gimp_layer_set_apply_mask(l,a) { if((l)->mask) gimp_layer_mask_set_apply((l)->mask,(a)); else success = FALSE; }
#define gimp_layer_set_show_mask(l,s)  { if((l)->mask) gimp_layer_mask_set_show((l)->mask,(s)); else success = FALSE; }
#define gimp_layer_set_edit_mask(l,e)  { if((l)->mask) gimp_layer_mask_set_edit((l)->mask,(e)); else success = FALSE; }


#endif /* __PDB_GLUE_H__ */
