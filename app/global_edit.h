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
#ifndef __GLOBAL_EDIT_H__
#define __GLOBAL_EDIT_H__

#include "gimage.h"
struct _Canvas;

/*  The interface functions  */
struct _Canvas *  crop_buffer_16            (struct _Canvas *, int);
struct _Canvas *  edit_cut_16               (GImage *, GimpDrawable *);
struct _Canvas *  edit_copy_16              (GImage *, GimpDrawable *);
int            edit_paste_16             (GImage *, GimpDrawable *, struct _Canvas *, int);
int            edit_clear             (GImage *, GimpDrawable *);
int            edit_fill              (GImage *, GimpDrawable *);

int            global_edit_cut        (void *);
int            global_edit_copy       (void *);
int            global_edit_paste      (void *, int);
void           global_edit_free       (void);

int            named_edit_cut         (void *);
int            named_edit_copy        (void *);
int            named_edit_paste       (void *);
void           named_buffers_free     (void);

#endif  /*  __GLOBAL_EDIT_H__  */
