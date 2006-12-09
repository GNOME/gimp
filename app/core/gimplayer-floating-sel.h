/* GIMP - The GNU Image Manipulation Program
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

#ifndef __GIMP_LAYER_FLOATING_SEL_H__
#define __GIMP_LAYER_FLOATING_SEL_H__


void             floating_sel_attach            (GimpLayer    *layer,
                                                 GimpDrawable *drawable);
void             floating_sel_remove            (GimpLayer    *layer);
void             floating_sel_anchor            (GimpLayer    *layer);
void             floating_sel_activate_drawable (GimpLayer    *layer);
void             floating_sel_to_layer          (GimpLayer    *layer);
void             floating_sel_store             (GimpLayer    *layer,
                                                 gint          x,
                                                 gint          y,
                                                 gint          w,
                                                 gint          h);
void             floating_sel_restore           (GimpLayer    *layer,
                                                 gint          x,
                                                 gint          y,
                                                 gint          w,
                                                 gint          h);
void             floating_sel_rigor             (GimpLayer    *layer,
                                                 gboolean      push_undo);
void             floating_sel_relax             (GimpLayer    *layer,
                                                 gboolean      push_undo);
void             floating_sel_composite         (GimpLayer    *layer,
                                                 gint          x,
                                                 gint          y,
                                                 gint          w,
                                                 gint          h,
                                                 gboolean      push_undo);
const BoundSeg * floating_sel_boundary          (GimpLayer    *layer,
                                                 gint         *n_segs);
void             floating_sel_invalidate        (GimpLayer    *layer);


#endif /* __GIMP_LAYER_FLOATING_SEL_H__ */
