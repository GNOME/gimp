/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_CHANNEL_H__
#define __LIGMA_CHANNEL_H__

#include "ligmadrawable.h"


#define LIGMA_TYPE_CHANNEL            (ligma_channel_get_type ())
#define LIGMA_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_CHANNEL, LigmaChannel))
#define LIGMA_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_CHANNEL, LigmaChannelClass))
#define LIGMA_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_CHANNEL))
#define LIGMA_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_CHANNEL))
#define LIGMA_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_CHANNEL, LigmaChannelClass))


typedef struct _LigmaChannelClass LigmaChannelClass;

struct _LigmaChannel
{
  LigmaDrawable  parent_instance;

  LigmaRGB       color;             /*  Also stores the opacity        */
  gboolean      show_masked;       /*  Show masked areas--as          */
                                   /*  opposed to selected areas      */

  GeglNode     *color_node;
  GeglNode     *invert_node;
  GeglNode     *mask_node;

  /*  Selection mask variables  */
  gboolean      boundary_known;    /*  is the current boundary valid  */
  LigmaBoundSeg *segs_in;           /*  outline of selected region     */
  LigmaBoundSeg *segs_out;          /*  outline of selected region     */
  gint          num_segs_in;       /*  number of lines in boundary    */
  gint          num_segs_out;      /*  number of lines in boundary    */
  gboolean      empty;             /*  is the region empty?           */
  gboolean      bounds_known;      /*  recalculate the bounds?        */
  gint          x1, y1;            /*  coordinates for bounding box   */
  gint          x2, y2;            /*  lower right hand coordinate    */
};

struct _LigmaChannelClass
{
  LigmaDrawableClass  parent_class;

  /*  signals  */
  void     (* color_changed) (LigmaChannel             *channel);

  /*  virtual functions  */
  gboolean (* boundary)      (LigmaChannel             *channel,
                              const LigmaBoundSeg     **segs_in,
                              const LigmaBoundSeg     **segs_out,
                              gint                    *num_segs_in,
                              gint                    *num_segs_out,
                              gint                     x1,
                              gint                     y1,
                              gint                     x2,
                              gint                     y2);
  gboolean (* is_empty)      (LigmaChannel             *channel);

  void     (* feather)       (LigmaChannel             *channel,
                              gdouble                  radius_x,
                              gdouble                  radius_y,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* sharpen)       (LigmaChannel             *channel,
                              gboolean                 push_undo);
  void     (* clear)         (LigmaChannel             *channel,
                              const gchar             *undo_desc,
                              gboolean                 push_undo);
  void     (* all)           (LigmaChannel             *channel,
                              gboolean                 push_undo);
  void     (* invert)        (LigmaChannel             *channel,
                              gboolean                 push_undo);
  void     (* border)        (LigmaChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              LigmaChannelBorderStyle   style,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* grow)          (LigmaChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              gboolean                 push_undo);
  void     (* shrink)        (LigmaChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* flood)         (LigmaChannel             *channel,
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

GType         ligma_channel_get_type           (void) G_GNUC_CONST;

LigmaChannel * ligma_channel_new                (LigmaImage         *image,
                                               gint               width,
                                               gint               height,
                                               const gchar       *name,
                                               const LigmaRGB     *color);
LigmaChannel * ligma_channel_new_from_buffer    (LigmaImage         *image,
                                               GeglBuffer        *buffer,
                                               const gchar       *name,
                                               const LigmaRGB     *color);
LigmaChannel * ligma_channel_new_from_alpha     (LigmaImage         *image,
                                               LigmaDrawable      *drawable,
                                               const gchar       *name,
                                               const LigmaRGB     *color);
LigmaChannel * ligma_channel_new_from_component (LigmaImage         *image,
                                               LigmaChannelType    type,
                                               const gchar       *name,
                                               const LigmaRGB     *color);

LigmaChannel * ligma_channel_get_parent         (LigmaChannel       *channel);

gdouble       ligma_channel_get_opacity        (LigmaChannel       *channel);
void          ligma_channel_set_opacity        (LigmaChannel       *channel,
                                               gdouble            opacity,
                                               gboolean           push_undo);

void          ligma_channel_get_color          (LigmaChannel       *channel,
                                               LigmaRGB           *color);
void          ligma_channel_set_color          (LigmaChannel       *channel,
                                               const LigmaRGB     *color,
                                               gboolean           push_undo);

gboolean      ligma_channel_get_show_masked    (LigmaChannel       *channel);
void          ligma_channel_set_show_masked    (LigmaChannel       *channel,
                                               gboolean           show_masked);

void          ligma_channel_push_undo          (LigmaChannel       *mask,
                                               const gchar       *undo_desc);


/*  selection mask functions  */

LigmaChannel * ligma_channel_new_mask           (LigmaImage              *image,
                                               gint                    width,
                                               gint                    height);

gboolean      ligma_channel_boundary           (LigmaChannel            *mask,
                                               const LigmaBoundSeg    **segs_in,
                                               const LigmaBoundSeg    **segs_out,
                                               gint                   *num_segs_in,
                                               gint                   *num_segs_out,
                                               gint                    x1,
                                               gint                    y1,
                                               gint                    x2,
                                               gint                    y2);
gboolean      ligma_channel_is_empty           (LigmaChannel            *mask);

void          ligma_channel_feather            (LigmaChannel            *mask,
                                               gdouble                 radius_x,
                                               gdouble                 radius_y,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          ligma_channel_sharpen            (LigmaChannel            *mask,
                                               gboolean                push_undo);

void          ligma_channel_clear              (LigmaChannel            *mask,
                                               const gchar            *undo_desc,
                                               gboolean                push_undo);
void          ligma_channel_all                (LigmaChannel            *mask,
                                               gboolean                push_undo);
void          ligma_channel_invert             (LigmaChannel            *mask,
                                               gboolean                push_undo);

void          ligma_channel_border             (LigmaChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               LigmaChannelBorderStyle  style,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          ligma_channel_grow               (LigmaChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               gboolean                push_undo);
void          ligma_channel_shrink             (LigmaChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          ligma_channel_flood              (LigmaChannel            *mask,
                                               gboolean                push_undo);


#endif /* __LIGMA_CHANNEL_H__ */
