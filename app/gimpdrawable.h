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
#ifndef __GIMPDRAWABLE_H__
#define __GIMPDRAWABLE_H__

#include "gimpobject.h"
#include "gimpdrawableF.h"
#include "tile_manager.h"
#include "temp_buf.h"
#include "gimpimageF.h"

#define GIMP_TYPE_DRAWABLE                  (gimp_drawable_get_type ())
#define GIMP_DRAWABLE(obj)                  (GTK_CHECK_CAST ((obj), GIMP_TYPE_DRAWABLE, GimpDrawable))
#define GIMP_IS_DRAWABLE(obj)               (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DRAWABLE))

GtkType gimp_drawable_get_type (void);

typedef enum
{
	FOREGROUND_FILL,
	BACKGROUND_FILL,
	WHITE_FILL,
	TRANSPARENT_FILL,
	NO_FILL
} GimpFillType;


/*  drawable access functions  */
void             gimp_drawable_merge_shadow       (GimpDrawable *, int);
void             gimp_drawable_fill               (GimpDrawable *drawable,
						   guchar r, guchar g,
						   guchar b, guchar a);

int              gimp_drawable_mask_bounds        (GimpDrawable *,
						   int *, int *,
						   int *, int *);

void             gimp_drawable_invalidate_preview (GimpDrawable *);
int              gimp_drawable_dirty              (GimpDrawable *);
int              gimp_drawable_clean              (GimpDrawable *);
int              gimp_drawable_type               (GimpDrawable *);
int              gimp_drawable_has_alpha          (GimpDrawable *);
int              gimp_drawable_type_with_alpha    (GimpDrawable *);
int              gimp_drawable_color              (GimpDrawable *);
int              gimp_drawable_gray               (GimpDrawable *);
int              gimp_drawable_indexed            (GimpDrawable *);
TileManager *    gimp_drawable_data               (GimpDrawable *);
TileManager *    gimp_drawable_shadow             (GimpDrawable *);
int              gimp_drawable_bytes              (GimpDrawable *);
int              gimp_drawable_width              (GimpDrawable *);
int              gimp_drawable_height             (GimpDrawable *);
int		 gimp_drawable_visible	          (GimpDrawable *);
void             gimp_drawable_offsets            (GimpDrawable *, 
						   int *, int *);

unsigned char *  gimp_drawable_cmap               (GimpDrawable *);
char *		 gimp_drawable_get_name	          (GimpDrawable *);
void 		 gimp_drawable_set_name	          (GimpDrawable *, char *);

GimpDrawable *   gimp_drawable_get_ID             (int);
void		 gimp_drawable_deallocate         (GimpDrawable *);
GimpImage *      gimp_drawable_gimage             (GimpDrawable*);

#endif /* __GIMPDRAWABLE_H__ */


