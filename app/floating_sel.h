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

#ifndef __FLOATING_SEL_H__
#define __FLOATING_SEL_H__


void       floating_sel_attach       (GimpLayer    *layer,
				      GimpDrawable *drawable);
void       floating_sel_remove       (GimpLayer    *layer);
void       floating_sel_anchor       (GimpLayer    *layer);
void       floating_sel_reset        (GimpLayer    *layer);
void       floating_sel_to_layer     (GimpLayer    *layer);
void       floating_sel_store        (GimpLayer    *layer,
				      gint          ,
				      gint          ,
				      gint          ,
				      gint          );
void       floating_sel_restore      (GimpLayer    *layer,
				      gint          ,
				      gint          ,
				      gint          ,
				      gint          );
void       floating_sel_rigor        (GimpLayer    *layer,
				      gint          );
void       floating_sel_relax        (GimpLayer    *layer,
				      gint          );
void       floating_sel_composite    (GimpLayer    *layer,
				      gint          ,
				      gint          ,
				      gint          ,
				      gint          ,
				      gint          );
BoundSeg * floating_sel_boundary     (GimpLayer    *layer,
				      gint         *);
void       floating_sel_bounds       (GimpLayer    *layer,
				      gint         *,
				      gint         *,
				      gint         *,
				      gint         *);
void       floating_sel_invalidate   (GimpLayer    *layer);


#endif /* __FLOATING_SEL_H__ */
