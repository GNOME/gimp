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

#ifndef __GIMP_CHANNEL_H__
#define __GIMP_CHANNEL_H__


#include "gimpdrawable.h"


/*  Half way point where a region is no longer visible in a selection  */
#define HALF_WAY 127


#define GIMP_TYPE_CHANNEL            (gimp_channel_get_type ())
#define GIMP_CHANNEL(obj)            (GTK_CHECK_CAST ((obj), GIMP_TYPE_CHANNEL, GimpChannel))
#define GIMP_CHANNEL_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHANNEL, GimpChannelClass))
#define GIMP_IS_CHANNEL(obj)         (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CHANNEL))
#define GIMP_IS_CHANNEL_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHANNEL))


typedef struct _GimpChannelClass GimpChannelClass;

struct _GimpChannel
{
  GimpDrawable  parent_instance;

  GimpRGB       color;             /*  Also stored the opacity        */
  gboolean      show_masked;       /*  Show masked areas--as          */
                                   /*  opposed to selected areas      */

  /*  Selection mask variables  */
  gboolean      boundary_known;    /*  is the current boundary valid  */
  BoundSeg     *segs_in;           /*  outline of selected region     */
  BoundSeg     *segs_out;          /*  outline of selected region     */
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
};


/*  Special undo types  */

typedef struct _ChannelUndo ChannelUndo;
typedef struct _MaskUndo    MaskUndo;

struct _ChannelUndo
{
  GimpChannel *channel;         /*  the actual channel          */
  gint         prev_position;   /*  former position in list     */
  GimpChannel *prev_channel;    /*  previous active channel     */
};

struct _MaskUndo
{
  TileManager *tiles;    /*  the actual mask  */
  gint         x, y;     /*  offsets          */
};


/*  function declarations  */

GtkType         gimp_channel_get_type          (void);

GimpChannel   * gimp_channel_new               (GimpImage         *gimage,
						gint               width,
						gint               height,
						const gchar       *name,
						const GimpRGB     *color);
GimpChannel   * gimp_channel_copy              (const GimpChannel *channel);

gint            gimp_channel_get_opacity       (const GimpChannel *channel);
void            gimp_channel_set_opacity       (GimpChannel       *channel,
						gint               opacity);

const GimpRGB * gimp_channel_get_color         (const GimpChannel *channel);
void 		gimp_channel_set_color         (GimpChannel       *channel, 
						const GimpRGB     *color);

void            gimp_channel_scale             (GimpChannel       *channel, 
						gint               new_width, 
						gint               new_height);
void            gimp_channel_resize            (GimpChannel       *channel, 
						gint               new_width,
						gint               new_height,
						gint               offx,
						gint               offy);
void            gimp_channel_update            (GimpChannel       *channel);

gboolean        gimp_channel_toggle_visibility (GimpChannel       *channel);

/* selection mask functions  */

GimpChannel   * gimp_channel_new_mask          (GimpImage         *gimage,
						gint               width,
						gint               height);
gboolean        gimp_channel_boundary          (GimpChannel       *mask,
						BoundSeg         **segs_in,
						BoundSeg         **segs_out,
						gint              *num_segs_in,
						gint              *num_segs_out,
						gint               x1,
						gint               y1,
						gint               x2,
						gint               y2);
gboolean        gimp_channel_bounds            (GimpChannel       *mask,
						gint              *x1,
						gint              *y1,
						gint              *x2,
						gint              *y2);
gint            gimp_channel_value             (GimpChannel       *mask, 
						gint               x, 
						gint               y);
gboolean        gimp_channel_is_empty          (GimpChannel       *mask);
void            gimp_channel_add_segment       (GimpChannel       *mask,
						gint               x,
						gint               y,
						gint               width,
						gint               value);
void            gimp_channel_sub_segment       (GimpChannel       *mask,
						gint               x,
						gint               y,
						gint               width,
						gint               value);
void            gimp_channel_combine_rect      (GimpChannel       *mask,
						ChannelOps         op,
						gint               x,
						gint               y,
						gint               w,
						gint               h);
void            gimp_channel_combine_ellipse   (GimpChannel       *mask,
						ChannelOps         op,
						gint               x,
						gint               y,
						gint               w,
						gint               h,
						gboolean           antialias);
void            gimp_channel_combine_mask      (GimpChannel       *mask,
						GimpChannel       *add_on,
						ChannelOps         op,
						gint               off_x,
						gint               off_y);
void            gimp_channel_feather           (GimpChannel       *input,
						GimpChannel       *output,
						gdouble            radius_x,
						gdouble            radius_y,
						ChannelOps         op,
						gint               off_x,
						gint               off_y);

void            gimp_channel_push_undo         (GimpChannel       *mask);
void            gimp_channel_clear             (GimpChannel       *mask);
void            gimp_channel_invert            (GimpChannel       *mask);
void            gimp_channel_sharpen           (GimpChannel       *mask);
void            gimp_channel_all               (GimpChannel       *mask);

void            gimp_channel_border            (GimpChannel      *mask,
						gint              radius_x,
						gint              radius_y);
void            gimp_channel_grow              (GimpChannel      *mask,
						gint              radius_x,
						gint              radius_y);
void            gimp_channel_shrink            (GimpChannel      *mask,
						gint              radius_x,
						gint              radius_y,
						gboolean          edge_lock);

void            gimp_channel_translate         (GimpChannel      *mask,
						gint              off_x,
						gint              off_y);
void            gimp_channel_load              (GimpChannel      *mask,
						GimpChannel      *channel);

void            gimp_channel_layer_alpha       (GimpChannel      *mask, 
						GimpLayer        *layer);
void            gimp_channel_layer_mask        (GimpChannel      *mask, 
						GimpLayer        *layer);

void		gimp_channel_invalidate_bounds (GimpChannel      *channel);


#endif /* __GIMP_CHANNEL_H__ */
