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

#include "apptypes.h"
#include "gimpobject.h"
#include "gimpdrawableF.h"
#include "tile_manager.h"
#include "temp_buf.h"
#include "gimpimageF.h"

#include <libgimp/parasiteF.h>

#define GIMP_TYPE_DRAWABLE     (gimp_drawable_get_type ())
#define GIMP_DRAWABLE(obj)     (GTK_CHECK_CAST ((obj), GIMP_TYPE_DRAWABLE, GimpDrawable))
#define GIMP_IS_DRAWABLE(obj)  (GTK_CHECK_TYPE ((obj), GIMP_TYPE_DRAWABLE))

GtkType gimp_drawable_get_type (void);

/*  drawable access functions  */

void             gimp_drawable_merge_shadow       (GimpDrawable *, gint);
void             gimp_drawable_fill               (GimpDrawable *drawable,
						   guchar r, guchar g,
						   guchar b, guchar a);

gboolean         gimp_drawable_mask_bounds        (GimpDrawable *,
						   gint *, gint *,
						   gint *, gint *);

void             gimp_drawable_invalidate_preview (GimpDrawable *);
gint             gimp_drawable_dirty              (GimpDrawable *);
gint             gimp_drawable_clean              (GimpDrawable *);
gboolean         gimp_drawable_has_alpha          (GimpDrawable *);
GimpImageType    gimp_drawable_type               (GimpDrawable *);
GimpImageType    gimp_drawable_type_with_alpha    (GimpDrawable *);
gboolean         gimp_drawable_color              (GimpDrawable *);
gboolean         gimp_drawable_gray               (GimpDrawable *);
gboolean         gimp_drawable_indexed            (GimpDrawable *);
TileManager *    gimp_drawable_data               (GimpDrawable *);
TileManager *    gimp_drawable_shadow             (GimpDrawable *);
gint             gimp_drawable_bytes              (GimpDrawable *);
gint             gimp_drawable_width              (GimpDrawable *);
gint             gimp_drawable_height             (GimpDrawable *);
gboolean	 gimp_drawable_visible	          (GimpDrawable *);
void             gimp_drawable_offsets            (GimpDrawable *, 
						   gint *, gint *);

guchar *         gimp_drawable_cmap               (GimpDrawable *);
gchar *		 gimp_drawable_get_name	          (GimpDrawable *);
void 		 gimp_drawable_set_name	          (GimpDrawable *, gchar *);

guchar *         gimp_drawable_get_color_at       (GimpDrawable *,
						   gint x, gint y);

Parasite *       gimp_drawable_find_parasite      (const GimpDrawable *,
						   const gchar *name);
void             gimp_drawable_attach_parasite    (GimpDrawable *, Parasite *);
void             gimp_drawable_detach_parasite    (GimpDrawable *,
						   const gchar *);
Parasite *       gimp_drawable_find_parasite      (const GimpDrawable *,
						   const gchar *);
gchar **         gimp_drawable_parasite_list      (GimpDrawable *drawable,
                                                   gint *count);
Tattoo           gimp_drawable_get_tattoo         (const GimpDrawable *);

GimpDrawable *   gimp_drawable_get_ID             (gint);
void		 gimp_drawable_deallocate         (GimpDrawable *);
GimpImage *      gimp_drawable_gimage             (GimpDrawable*);
void             gimp_drawable_set_gimage         (GimpDrawable*, GimpImage *);

#endif /* __GIMPDRAWABLE_H__ */
