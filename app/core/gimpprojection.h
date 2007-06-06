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

#ifndef __GIMP_PROJECTION_H__
#define __GIMP_PROJECTION_H__


#include "gimpobject.h"


typedef struct _GimpProjectionIdleRender GimpProjectionIdleRender;

struct _GimpProjectionIdleRender
{
  gint    width;
  gint    height;
  gint    x;
  gint    y;
  gint    base_x;
  gint    base_y;
  guint   idle_id;
  GSList *update_areas;   /*  flushed update areas */
};


#define GIMP_TYPE_PROJECTION            (gimp_projection_get_type ())
#define GIMP_PROJECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PROJECTION, GimpProjection))
#define GIMP_PROJECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PROJECTION, GimpProjectionClass))
#define GIMP_IS_PROJECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PROJECTION))
#define GIMP_IS_PROJECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PROJECTION))
#define GIMP_PROJECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PROJECTION, GimpProjectionClass))

#define PYRAMID_MAX_LEVELS              10
#define PYRAMID_BASE_LEVEL              0


typedef struct _GimpProjectionClass GimpProjectionClass;

struct _GimpProjection
{
  GimpObject                parent_instance;

  GimpImage                *image;

  GimpImageType             type;
  gint                      bytes;

  /* An image pyramid. Level n + 1 has half the width and height of level n. */
  TileManager              *pyramid[PYRAMID_MAX_LEVELS];
  gint                      top_level;

  GSList                   *update_areas;
  GimpProjectionIdleRender  idle_render;

  gboolean                  construct_flag;
};

struct _GimpProjectionClass
{
  GimpObjectClass  parent_class;

  void (* update) (GimpProjection *image,
                   gboolean        now,
                   gint            x,
                   gint            y,
                   gint            width,
                   gint            height);
};


GType            gimp_projection_get_type         (void) G_GNUC_CONST;

GimpProjection * gimp_projection_new              (GimpImage            *image);

TileManager    * gimp_projection_get_tiles        (GimpProjection       *proj);

TileManager    * gimp_projection_get_tiles_at_level
                                                  (GimpProjection       *proj,
                                                   gint                  level);
void             gimp_projection_level_size_from_scale
                                                  (GimpProjection       *proj,
                                                   gdouble               scalex,
                                                   gint                 *level,
                                                   gint                 *width,
                                                   gint                 *height);

gint             gimp_projection_scale_to_level   (GimpProjection       *proj,
                                                   gdouble               scalex);

GimpImage      * gimp_projection_get_image        (const GimpProjection *proj);
GimpImageType    gimp_projection_get_image_type   (const GimpProjection *proj);
gint             gimp_projection_get_bytes        (const GimpProjection *proj);
gdouble          gimp_projection_get_opacity      (const GimpProjection *proj);

void             gimp_projection_flush            (GimpProjection       *proj);
void             gimp_projection_flush_now        (GimpProjection       *proj);
void             gimp_projection_finish_draw      (GimpProjection       *proj);

gint64           gimp_projection_estimate_memsize (GimpImageBaseType     type,
                                                   gint                  width,
                                                   gint                  height);


#endif /*  __GIMP_PROJECTION_H__  */
