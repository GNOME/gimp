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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __FLOATING_SEL_H__
#define __FLOATING_SEL_H__

#include "layer.h"

/*  Functions  */

void       floating_sel_attach       (Layer *, GimpDrawable *);
void       floating_sel_remove       (Layer *);
void       floating_sel_anchor       (Layer *);
void       floating_sel_reset        (Layer *);
void       floating_sel_to_layer     (Layer *);
void       floating_sel_store        (Layer *, int, int, int, int);
void       floating_sel_restore      (Layer *, int, int, int, int);
void       floating_sel_rigor        (Layer *, int);
void       floating_sel_relax        (Layer *, int);
void       floating_sel_composite    (Layer *, int, int, int, int, int);
BoundSeg * floating_sel_boundary     (Layer *, int *);
void       floating_sel_bounds       (Layer *, int *, int *, int *, int *);
void       floating_sel_invalidate   (Layer *);

#endif /* __FLOATING_SEL_H__ */
