/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppath.h
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "core/gimpitem.h"


#define GIMP_TYPE_PATH            (gimp_path_get_type ())
#define GIMP_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PATH, GimpPath))
#define GIMP_PATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PATH, GimpPathClass))
#define GIMP_IS_PATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PATH))
#define GIMP_IS_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PATH))
#define GIMP_PATH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PATH, GimpPathClass))


typedef struct _GimpPathClass  GimpPathClass;

struct _GimpPath
{
  GimpItem        parent_instance;

  GQueue         *strokes;        /* Queue of GimpStrokes         */
  GHashTable     *stroke_to_list; /* Map from GimpStroke to strokes listnode */
  gint            last_stroke_id;

  gint            freeze_count;
  gdouble         precision;

  GimpBezierDesc *bezier_desc;    /* Cached bezier representation */

  gboolean        bounds_valid;   /* Cached bounding box          */
  gboolean        bounds_empty;
  gdouble         bounds_x1;
  gdouble         bounds_y1;
  gdouble         bounds_x2;
  gdouble         bounds_y2;
};

struct _GimpPathClass
{
  GimpItemClass  parent_class;

  /*  signals  */
  void          (* freeze)            (GimpPath       *path);
  void          (* thaw)              (GimpPath       *path);

  /*  virtual functions  */
  void          (* stroke_add)        (GimpPath          *path,
                                       GimpStroke        *stroke);
  void          (* stroke_remove)     (GimpPath          *path,
                                       GimpStroke        *stroke);
  GimpStroke  * (* stroke_get)        (GimpPath          *path,
                                       const GimpCoords  *coord);
  GimpStroke  * (* stroke_get_next)   (GimpPath          *path,
                                       GimpStroke        *prev);
  gdouble       (* stroke_get_length) (GimpPath          *path,
                                       GimpStroke        *stroke);
  GimpAnchor  * (* anchor_get)        (GimpPath          *path,
                                       const GimpCoords  *coord,
                                       GimpStroke       **ret_stroke);
  void          (* anchor_delete)     (GimpPath          *path,
                                       GimpAnchor        *anchor);
  gdouble       (* get_length)        (GimpPath          *path,
                                       const GimpAnchor  *start);
  gdouble       (* get_distance)      (GimpPath          *path,
                                       const GimpCoords  *coord);
  gint          (* interpolate)       (GimpPath          *path,
                                       GimpStroke        *stroke,
                                       gdouble            precision,
                                       gint               max_points,
                                       GimpCoords        *ret_coords);
  GimpBezierDesc * (* make_bezier)    (GimpPath          *path);
};


/*  path utility functions  */

GType           gimp_path_get_type              (void) G_GNUC_CONST;

GimpPath      * gimp_path_new                   (GimpImage   *image,
                                                 const gchar *name);

GimpPath      * gimp_path_get_parent            (GimpPath    *path);

void            gimp_path_freeze                (GimpPath    *path);
void            gimp_path_thaw                  (GimpPath    *path);

void            gimp_path_copy_strokes          (GimpPath    *src_path,
                                                 GimpPath    *dest_path);
void            gimp_path_add_strokes           (GimpPath    *src_path,
                                                 GimpPath    *dest_path);


/* accessing / modifying the anchors */

GimpAnchor    * gimp_path_anchor_get            (GimpPath          *path,
                                                 const GimpCoords  *coord,
                                                 GimpStroke       **ret_stroke);

/* prev == NULL: "first" anchor */
GimpAnchor    * gimp_path_anchor_get_next       (GimpPath           *path,
                                                 const GimpAnchor   *prev);

/* type will be an xorable enum:
 * VECTORS_NONE, VECTORS_FIX_ANGLE, VECTORS_FIX_RATIO, VECTORS_RESTRICT_ANGLE
 *  or so.
 */
void          gimp_path_anchor_move_relative    (GimpPath           *path,
                                                 GimpAnchor         *anchor,
                                                 const GimpCoords   *deltacoord,
                                                 gint                type);
void          gimp_path_anchor_move_absolute    (GimpPath           *path,
                                                 GimpAnchor         *anchor,
                                                 const GimpCoords   *coord,
                                                 gint                type);

void          gimp_path_anchor_delete           (GimpPath           *path,
                                                 GimpAnchor         *anchor);

void          gimp_path_anchor_select           (GimpPath           *path,
                                                 GimpStroke         *target_stroke,
                                                 GimpAnchor         *anchor,
                                                 gboolean            selected,
                                                 gboolean            exclusive);


/* GimpStroke is a connected component of a GimpPath object */

void            gimp_path_stroke_add            (GimpPath           *path,
                                                 GimpStroke         *stroke);
void            gimp_path_stroke_remove         (GimpPath           *path,
                                                 GimpStroke         *stroke);
gint            gimp_path_get_n_strokes         (GimpPath           *path);
GimpStroke    * gimp_path_stroke_get            (GimpPath           *path,
                                                 const GimpCoords   *coord);
GimpStroke    * gimp_path_stroke_get_by_id      (GimpPath           *path,
                                                 gint                id);

/* prev == NULL: "first" stroke */
GimpStroke    * gimp_path_stroke_get_next       (GimpPath           *path,
                                                 GimpStroke         *prev);
gdouble         gimp_path_stroke_get_length     (GimpPath           *path,
                                                 GimpStroke         *stroke);

/* accessing the shape of the curve */

gdouble         gimp_path_get_length            (GimpPath           *path,
                                                 const GimpAnchor   *start);
gdouble         gimp_path_get_distance          (GimpPath           *path,
                                                 const GimpCoords   *coord);

/* returns the number of valid coordinates */

gint            gimp_path_interpolate           (GimpPath           *path,
                                                 GimpStroke         *stroke,
                                                 gdouble             precision,
                                                 gint                max_points,
                                                 GimpCoords         *ret_coords);

gboolean        gimp_path_attached_to_vector_layer
                                                (GimpPath           *path,
                                                 GimpImage          *image);
/* usually overloaded */

/* returns a bezier representation */
const GimpBezierDesc * gimp_path_get_bezier     (GimpPath           *path);
