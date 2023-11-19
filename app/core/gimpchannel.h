/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_CHANNEL_H__
#define __GIMP_CHANNEL_H__

#include "gimpdrawable.h"


#define GIMP_TYPE_CHANNEL            (gimp_channel_get_type ())
#define GIMP_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CHANNEL, GimpChannel))
#define GIMP_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHANNEL, GimpChannelClass))
#define GIMP_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CHANNEL))
#define GIMP_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHANNEL))
#define GIMP_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CHANNEL, GimpChannelClass))


typedef struct _GimpChannelClass GimpChannelClass;

struct _GimpChannel
{
  GimpDrawable  parent_instance;

  GeglColor    *color;             /*  Also stores the opacity        */
  gboolean      show_masked;       /*  Show masked areas--as          */
                                   /*  opposed to selected areas      */

  GeglNode     *color_node;
  GeglNode     *invert_node;
  GeglNode     *mask_node;

  /*  Selection mask variables  */
  gboolean      boundary_known;    /*  is the current boundary valid  */
  GimpBoundSeg *segs_in;           /*  outline of selected region     */
  GimpBoundSeg *segs_out;          /*  outline of selected region     */
  gint          num_segs_in;       /*  number of lines in boundary    */
  gint          num_segs_out;      /*  number of lines in boundary    */
  gboolean      empty;             /*  is the region empty?           */
  gboolean      bounds_known;      /*  recalculate the bounds?        */
  gint          x1, y1;            /*  coordinates for bounding box   */
  gint          x2, y2;            /*  lower right hand coordinate    */
};

struct _GimpChannelClass
{
  GimpDrawableClass  parent_class;

  /*  signals  */
  void     (* color_changed) (GimpChannel             *channel);

  /*  virtual functions  */
  gboolean (* boundary)      (GimpChannel             *channel,
                              const GimpBoundSeg     **segs_in,
                              const GimpBoundSeg     **segs_out,
                              gint                    *num_segs_in,
                              gint                    *num_segs_out,
                              gint                     x1,
                              gint                     y1,
                              gint                     x2,
                              gint                     y2);
  gboolean (* is_empty)      (GimpChannel             *channel);
  gboolean (* is_full)       (GimpChannel             *channel);

  void     (* feather)       (GimpChannel             *channel,
                              gdouble                  radius_x,
                              gdouble                  radius_y,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* sharpen)       (GimpChannel             *channel,
                              gboolean                 push_undo);
  void     (* clear)         (GimpChannel             *channel,
                              const gchar             *undo_desc,
                              gboolean                 push_undo);
  void     (* all)           (GimpChannel             *channel,
                              gboolean                 push_undo);
  void     (* invert)        (GimpChannel             *channel,
                              gboolean                 push_undo);
  void     (* border)        (GimpChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              GimpChannelBorderStyle   style,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* grow)          (GimpChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              gboolean                 push_undo);
  void     (* shrink)        (GimpChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* flood)         (GimpChannel             *channel,
                              gboolean                 push_undo);

  const gchar *feather_desc;
  const gchar *sharpen_desc;
  const gchar *clear_desc;
  const gchar *all_desc;
  const gchar *invert_desc;
  const gchar *border_desc;
  const gchar *grow_desc;
  const gchar *shrink_desc;
  const gchar *flood_desc;
};


/*  function declarations  */

GType         gimp_channel_get_type           (void) G_GNUC_CONST;

GimpChannel * gimp_channel_new                (GimpImage         *image,
                                               gint               width,
                                               gint               height,
                                               const gchar       *name,
                                               GeglColor         *color);
GimpChannel * gimp_channel_new_from_buffer    (GimpImage         *image,
                                               GeglBuffer        *buffer,
                                               const gchar       *name,
                                               GeglColor         *color);
GimpChannel * gimp_channel_new_from_alpha     (GimpImage         *image,
                                               GimpDrawable      *drawable,
                                               const gchar       *name,
                                               GeglColor         *color);
GimpChannel * gimp_channel_new_from_component (GimpImage         *image,
                                               GimpChannelType    type,
                                               const gchar       *name,
                                               GeglColor         *color);

GimpChannel * gimp_channel_get_parent         (GimpChannel       *channel);

gdouble       gimp_channel_get_opacity        (GimpChannel       *channel);
void          gimp_channel_set_opacity        (GimpChannel       *channel,
                                               gdouble            opacity,
                                               gboolean           push_undo);

GeglColor   * gimp_channel_get_color          (GimpChannel       *channel);
void          gimp_channel_set_color          (GimpChannel       *channel,
                                               GeglColor         *color,
                                               gboolean           push_undo);

gboolean      gimp_channel_get_show_masked    (GimpChannel       *channel);
void          gimp_channel_set_show_masked    (GimpChannel       *channel,
                                               gboolean           show_masked);

void          gimp_channel_push_undo          (GimpChannel       *mask,
                                               const gchar       *undo_desc);


/*  selection mask functions  */

GimpChannel * gimp_channel_new_mask           (GimpImage              *image,
                                               gint                    width,
                                               gint                    height);

gboolean      gimp_channel_boundary           (GimpChannel            *mask,
                                               const GimpBoundSeg    **segs_in,
                                               const GimpBoundSeg    **segs_out,
                                               gint                   *num_segs_in,
                                               gint                   *num_segs_out,
                                               gint                    x1,
                                               gint                    y1,
                                               gint                    x2,
                                               gint                    y2);
gboolean      gimp_channel_is_empty           (GimpChannel            *mask);

gboolean      gimp_channel_is_full            (GimpChannel            *mask);

void          gimp_channel_feather            (GimpChannel            *mask,
                                               gdouble                 radius_x,
                                               gdouble                 radius_y,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          gimp_channel_sharpen            (GimpChannel            *mask,
                                               gboolean                push_undo);

void          gimp_channel_clear              (GimpChannel            *mask,
                                               const gchar            *undo_desc,
                                               gboolean                push_undo);
void          gimp_channel_all                (GimpChannel            *mask,
                                               gboolean                push_undo);
void          gimp_channel_invert             (GimpChannel            *mask,
                                               gboolean                push_undo);

void          gimp_channel_border             (GimpChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               GimpChannelBorderStyle  style,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          gimp_channel_grow               (GimpChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               gboolean                push_undo);
void          gimp_channel_shrink             (GimpChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          gimp_channel_flood              (GimpChannel            *mask,
                                               gboolean                push_undo);


#endif /* __GIMP_CHANNEL_H__ */
