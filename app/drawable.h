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
#ifndef __DRAWABLE_H__
#define __DRAWABLE_H__

#include <gtk/gtkdata.h>

#include "tag.h"
#include "canvas.h"

struct _Canvas;


#define GIMP_DRAWABLE(obj)         GTK_CHECK_CAST (obj, gimp_drawable_get_type (), GimpDrawable)
#define GIMP_DRAWABLE_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_drawable_get_type(), GimpDrawableClass)
#define GIMP_IS_DRAWABLE(obj)      GTK_CHECK_TYPE (obj, gimp_drawable_get_type())

typedef struct _GimpDrawable      GimpDrawable;
typedef struct _GimpDrawableClass GimpDrawableClass;

Tag drawable_tag (GimpDrawable *);

guint gimp_drawable_get_type (void);

/*  drawable access functions  */
int		 drawable_ID		     (GimpDrawable *);
void             drawable_apply_image        (GimpDrawable *, 
					      int, int, int, int, 
					      struct _Canvas *);
void             drawable_merge_shadow       (GimpDrawable *, int);
void             drawable_fill               (GimpDrawable *, int);
void             drawable_update             (GimpDrawable *, 
					      int, int, int, int);
int              drawable_mask_bounds        (GimpDrawable *,
					      int *, int *, int *, int *);
void             drawable_invalidate_preview (GimpDrawable *);
int              drawable_dirty              (GimpDrawable *);
int              drawable_clean              (GimpDrawable *);
int              drawable_type               (GimpDrawable *);
int              drawable_has_alpha          (GimpDrawable *);
int              drawable_type_with_alpha    (GimpDrawable *);
int              drawable_color              (GimpDrawable *);
int              drawable_gray               (GimpDrawable *);
int              drawable_indexed            (GimpDrawable *);
struct _Canvas * drawable_data               (GimpDrawable *);
struct _Canvas * drawable_shadow             (GimpDrawable *);
int              drawable_bytes              (GimpDrawable *);
int              drawable_width              (GimpDrawable *);
int              drawable_height             (GimpDrawable *);
int		 drawable_visible	     (GimpDrawable *);
void             drawable_offsets            (GimpDrawable *, int *, int *);
unsigned char *  drawable_cmap               (GimpDrawable *);
char *		 drawable_name		     (GimpDrawable *);

GimpDrawable *   drawable_get_ID             (int);
void		 drawable_deallocate	     (GimpDrawable *);
void		 gimp_drawable_configure     (GimpDrawable *,
					      int, int, int, int, char *);
void             gimp_drawable_configure_tag (GimpDrawable *, 
					      int, int, int, Tag, Storage, char *);

#endif /* __DRAWABLE_H__ */
