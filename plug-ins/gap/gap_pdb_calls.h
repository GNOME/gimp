/*  gap_pdb_calls.h
 *
 *
 */
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

/* revision history:
 * version 0.98.00; 1998/11/30  hof: all PDB-calls of GIMP PDB-Procedures
 */

#ifndef _GAP_PDB_CALLS_H
#define _GAP_PDB_CALLS_H

#include "libgimp/gimp.h"

gint p_pdb_procedure_available(char *proc_name);
gint p_get_gimp_selection_bounds (gint32 image_id, gint32 *x1, gint32 *y1, gint32 *x2, gint32 *y2);
gint p_gimp_selection_load (gint32 image_id, gint32 channel_id);
int  p_layer_set_linked (gint32 layer_id, gint32 new_state);
gint p_layer_get_linked(gint32 layer_id);

gint32 p_gimp_image_floating_sel_attached_to(gint32 image_id);
gint   p_gimp_floating_sel_attach(gint32 layer_id, gint32 drawable_id);
gint   p_gimp_floating_sel_rigor(gint32 layer_id, gint32 undo);
gint   p_gimp_floating_sel_relax(gint32 layer_id, gint32 undo);

gint32 p_gimp_image_add_guide(gint32 image_id, gint32 position, gint32 orientation);
gint32 p_gimp_image_findnext_guide(gint32 image_id, gint32 guide_id);
gint32 p_gimp_image_get_guide_position(gint32 image_id, gint32 guide_id);
gint32 p_gimp_image_get_guide_orientation(gint32 image_id, gint32 guide_id);
gint32 p_gimp_image_delete_guide(gint32 image_id, gint32 guide_id);
gint   p_gimp_selection_none(gint32 image_id);

gint   p_gimp_rotate(gint32 image_id, gint32 drawable_id, gint32 interpolation, gdouble angle_deg);
gint32 p_gimp_channel_ops_duplicate  (gint32     image_ID);


gint   p_gimp_drawable_set_image(gint32 drawable_id, gint32 image_id);

#endif
