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

#ifndef __GIMP_DRAWABLE_H__
#define __GIMP_DRAWABLE_H__


#include "gimpitem.h"


#define GIMP_TYPE_DRAWABLE            (gimp_drawable_get_type ())
#define GIMP_DRAWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DRAWABLE, GimpDrawable))
#define GIMP_DRAWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DRAWABLE, GimpDrawableClass))
#define GIMP_IS_DRAWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DRAWABLE))
#define GIMP_IS_DRAWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DRAWABLE))
#define GIMP_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DRAWABLE, GimpDrawableClass))


typedef struct _GimpDrawableClass GimpDrawableClass;

struct _GimpDrawable
{
  GimpItem       parent_instance;

  TileManager   *tiles;              /* tiles for drawable data        */

  gint           bytes;              /* bytes per pixel                */
  GimpImageType  type;               /* type of drawable               */
  gboolean       has_alpha;          /* drawable has alpha             */

  /*  Preview variables  */
  GSList        *preview_cache;      /* preview caches of the channel  */
  gboolean       preview_valid;      /* is the preview valid?          */
};

struct _GimpDrawableClass
{
  GimpItemClass  parent_class;

  /*  signals  */
  void   (* update)                (GimpDrawable         *drawable,
                                    gint                  x,
                                    gint                  y,
                                    gint                  width,
                                    gint                  height);
  void   (* alpha_changed)         (GimpDrawable         *drawable);

  /*  virtual functions  */
  gint64 (* estimate_memsize)      (const GimpDrawable   *drawable,
                                    gint                  width,
                                    gint                  height);
  void   (* invalidate_boundary)   (GimpDrawable         *drawable);
  void   (* get_active_components) (const GimpDrawable   *drawable,
                                    gboolean             *active);
  void   (* apply_region)          (GimpDrawable         *drawable,
                                    PixelRegion          *src2PR,
                                    gboolean              push_undo,
                                    const gchar          *undo_desc,
                                    gdouble               opacity,
                                    GimpLayerModeEffects  mode,
                                    TileManager          *src1_tiles,
                                    gint                  x,
                                    gint                  y);
  void   (* replace_region)        (GimpDrawable         *drawable,
                                    PixelRegion          *src2PR,
                                    gboolean              push_undo,
                                    const gchar          *undo_desc,
                                    gdouble               opacity,
                                    PixelRegion          *maskPR,
                                    gint                  x,
                                    gint                  y);
  void   (* set_tiles)             (GimpDrawable         *drawable,
                                    gboolean              push_undo,
                                    const gchar          *undo_desc,
                                    TileManager          *tiles,
                                    GimpImageType         type,
                                    gint                  offset_x,
                                    gint                  offset_y);

  void   (* push_undo)             (GimpDrawable         *drawable,
                                    const gchar          *undo_desc,
                                    TileManager          *tiles,
                                    gboolean              sparse,
                                    gint                  x,
                                    gint                  y,
                                    gint                  width,
                                    gint                  height);
  void   (* swap_pixels)           (GimpDrawable         *drawable,
                                    TileManager          *tiles,
                                    gboolean              sparse,
                                    gint                  x,
                                    gint                  y,
                                    gint                  width,
                                    gint                  height);
};


GType           gimp_drawable_get_type           (void) G_GNUC_CONST;

gint64          gimp_drawable_estimate_memsize   (const GimpDrawable *drawable,
                                                  gint                width,
                                                  gint                height);

void            gimp_drawable_configure          (GimpDrawable       *drawable,
                                                  GimpImage          *image,
                                                  gint                offset_x,
                                                  gint                offset_y,
                                                  gint                width,
                                                  gint                height,
                                                  GimpImageType       type,
                                                  const gchar        *name);

void            gimp_drawable_update             (GimpDrawable       *drawable,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height);
void            gimp_drawable_alpha_changed      (GimpDrawable       *drawable);

void           gimp_drawable_invalidate_boundary (GimpDrawable       *drawable);
void         gimp_drawable_get_active_components (const GimpDrawable *drawable,
                                                  gboolean           *active);

void            gimp_drawable_apply_region       (GimpDrawable       *drawable,
                                                  PixelRegion        *src2PR,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc,
                                                  gdouble             opacity,
                                                  GimpLayerModeEffects  mode,
                                                  TileManager        *src1_tiles,
                                                  gint                x,
                                                  gint                y);
void            gimp_drawable_replace_region     (GimpDrawable       *drawable,
                                                  PixelRegion        *src2PR,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc,
                                                  gdouble             opacity,
                                                  PixelRegion        *maskPR,
                                                  gint                x,
                                                  gint                y);

TileManager   * gimp_drawable_get_tiles          (const GimpDrawable *drawable);
void            gimp_drawable_set_tiles          (GimpDrawable       *drawable,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc,
                                                  TileManager        *tiles);
void            gimp_drawable_set_tiles_full     (GimpDrawable       *drawable,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc,
                                                  TileManager        *tiles,
                                                  GimpImageType       type,
                                                  gint                offset_x,
                                                  gint                offset_y);

void            gimp_drawable_swap_pixels        (GimpDrawable       *drawable,
                                                  TileManager        *tiles,
                                                  gboolean            sparse,
                                                  gint                x,
                                                  gint                y,
                                                  gint                width,
                                                  gint                height);

void            gimp_drawable_push_undo          (GimpDrawable       *drawable,
                                                  const gchar        *undo_desc,
                                                  gint                x1,
                                                  gint                y1,
                                                  gint                x2,
                                                  gint                y2,
                                                  TileManager        *tiles,
                                                  gboolean            sparse);

TileManager   * gimp_drawable_get_shadow_tiles   (GimpDrawable       *drawable);
void            gimp_drawable_merge_shadow       (GimpDrawable       *drawable,
                                                  gboolean            push_undo,
                                                  const gchar        *undo_desc);

void            gimp_drawable_fill               (GimpDrawable       *drawable,
                                                  const GimpRGB      *color,
                                                  const GimpPattern  *pattern);
void            gimp_drawable_fill_by_type       (GimpDrawable       *drawable,
                                                  GimpContext        *context,
                                                  GimpFillType        fill_type);

gboolean        gimp_drawable_mask_bounds        (GimpDrawable       *drawable,
                                                  gint               *x1,
                                                  gint               *y1,
                                                  gint               *x2,
                                                  gint               *y2);
gboolean        gimp_drawable_mask_intersect     (GimpDrawable       *drawable,
                                                  gint               *x,
                                                  gint               *y,
                                                  gint               *width,
                                                  gint               *height);

gboolean        gimp_drawable_has_alpha          (const GimpDrawable *drawable);
GimpImageType   gimp_drawable_type               (const GimpDrawable *drawable);
GimpImageType   gimp_drawable_type_with_alpha    (const GimpDrawable *drawable);
GimpImageType   gimp_drawable_type_without_alpha (const GimpDrawable *drawable);
gboolean        gimp_drawable_is_rgb             (const GimpDrawable *drawable);
gboolean        gimp_drawable_is_gray            (const GimpDrawable *drawable);
gboolean        gimp_drawable_is_indexed         (const GimpDrawable *drawable);
gint            gimp_drawable_bytes              (const GimpDrawable *drawable);
gint            gimp_drawable_bytes_with_alpha   (const GimpDrawable *drawable);
gint            gimp_drawable_bytes_without_alpha(const GimpDrawable *drawable);

gboolean        gimp_drawable_has_floating_sel   (const GimpDrawable *drawable);

const guchar  * gimp_drawable_get_colormap       (const GimpDrawable *drawable);


#endif /* __GIMP_DRAWABLE_H__ */
