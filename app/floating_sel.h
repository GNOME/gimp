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


void       floating_sel_attach       (Layer        *,
				      GimpDrawable *);
void       floating_sel_remove       (Layer        *);
void       floating_sel_anchor       (Layer        *);
void       floating_sel_reset        (Layer        *);
void       floating_sel_to_layer     (Layer        *);
void       floating_sel_store        (Layer        *,
				      gint          ,
				      gint          ,
				      gint          ,
				      gint          );
void       floating_sel_restore      (Layer        *,
				      gint          ,
				      gint          ,
				      gint          ,
				      gint          );
void       floating_sel_rigor        (Layer        *,
				      gint          );
void       floating_sel_relax        (Layer        *,
				      gint          );
void       floating_sel_composite    (Layer        *,
				      gint          ,
				      gint          ,
				      gint          ,
				      gint          ,
				      gint          );
BoundSeg * floating_sel_boundary     (Layer        *,
				      gint         *);
void       floating_sel_bounds       (Layer        *,
				      gint         *,
				      gint         *,
				      gint         *,
				      gint         *);
void       floating_sel_invalidate   (Layer        *);


#endif /* __FLOATING_SEL_H__ */
