/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawable.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_DRAWABLE_H__
#define __GIMP_DRAWABLE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


struct _GimpDrawable
{
  gint32    drawable_id;   /* drawable ID */
  guint     width;         /* width of drawble */
  guint     height;        /* height of drawble */
  guint     bpp;           /* bytes per pixel of drawable */
  guint     ntile_rows;    /* # of tile rows */
  guint     ntile_cols;    /* # of tile columns */
  GimpTile *tiles;         /* the normal tiles */
  GimpTile *shadow_tiles;  /* the shadow tiles */
};


GimpDrawable * gimp_drawable_get                    (gint32         drawable_ID);
void           gimp_drawable_detach                 (GimpDrawable  *drawable);
void           gimp_drawable_flush                  (GimpDrawable  *drawable);
GimpTile     * gimp_drawable_get_tile               (GimpDrawable  *drawable,
                                                     gboolean       shadow,
                                                     gint           row,
                                                     gint           col);
GimpTile     * gimp_drawable_get_tile2              (GimpDrawable  *drawable,
                                                     gboolean       shadow,
                                                     gint           x,
                                                     gint           y);

void           gimp_drawable_get_color_uchar        (gint32         drawable_ID,
                                                     const GimpRGB *color,
                                                     guchar        *color_uchar);

guchar       * gimp_drawable_get_thumbnail_data     (gint32         drawable_ID,
                                                     gint          *width,
                                                     gint          *height,
                                                     gint          *bpp);
guchar       * gimp_drawable_get_sub_thumbnail_data (gint32         drawable_ID,
                                                     gint           src_x,
                                                     gint           src_y,
                                                     gint           src_width,
                                                     gint           src_height,
                                                     gint          *dest_width,
                                                     gint          *dest_height,
                                                     gint          *bpp);

#ifndef GIMP_DISABLE_DEPRECATED
gboolean       gimp_drawable_is_valid               (gint32              drawable_ID);
gboolean       gimp_drawable_is_layer               (gint32              drawable_ID);
gboolean       gimp_drawable_is_text_layer          (gint32              drawable_ID);
gboolean       gimp_drawable_is_layer_mask          (gint32              drawable_ID);
gboolean       gimp_drawable_is_channel             (gint32              drawable_ID);
gboolean       gimp_drawable_delete                 (gint32              drawable_ID);
gint32         gimp_drawable_get_image              (gint32              drawable_ID);
gchar*         gimp_drawable_get_name               (gint32              drawable_ID);
gboolean       gimp_drawable_set_name               (gint32              drawable_ID,
                                                     const gchar        *name);
gboolean       gimp_drawable_get_visible            (gint32              drawable_ID);
gboolean       gimp_drawable_set_visible            (gint32              drawable_ID,
                                                     gboolean            visible);
gboolean       gimp_drawable_get_linked             (gint32              drawable_ID);
gboolean       gimp_drawable_set_linked             (gint32              drawable_ID,
                                                     gboolean            linked);
gint           gimp_drawable_get_tattoo             (gint32              drawable_ID);
gboolean       gimp_drawable_set_tattoo             (gint32              drawable_ID,
                                                     gint                tattoo);
GimpParasite * gimp_drawable_parasite_find          (gint32              drawable_ID,
                                                     const gchar        *name);
gboolean       gimp_drawable_parasite_attach        (gint32              drawable_ID,
                                                     const GimpParasite *parasite);
gboolean       gimp_drawable_parasite_detach        (gint32              drawable_ID,
                                                     const gchar        *name);
gboolean       gimp_drawable_parasite_list          (gint32              drawable_ID,
                                                     gint               *num_parasites,
                                                     gchar            ***parasites);
gboolean       gimp_drawable_attach_new_parasite    (gint32              drawable_ID,
                                                     const gchar        *name,
                                                     gint                flags,
                                                     gint                size,
                                                     gconstpointer       data);
#endif /* GIMP_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* __GIMP_DRAWABLE_H__ */
