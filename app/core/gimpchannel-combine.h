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
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "apptypes.h"
#include "drawable.h"
#include "boundary.h"
#include "temp_buf.h"
#include "tile_manager.h"
#include "gimpimageF.h"

/* OPERATIONS */

typedef enum
{
  ADD,
  SUB,
  REPLACE,
  INTERSECT
} ChannelOps;

/*  Half way point where a region is no longer visible in a selection  */
#define HALF_WAY 127

/*  structure declarations  */

#define GIMP_TYPE_CHANNEL             (gimp_channel_get_type ())
#define GIMP_CHANNEL(obj)             (GTK_CHECK_CAST ((obj), GIMP_TYPE_CHANNEL, GimpChannel))
#define GIMP_CHANNEL_CLASS(klass)     (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CHANNEL, GimpChannelClass))
#define GIMP_IS_CHANNEL(obj)          (GTK_CHECK_TYPE ((obj), GIMP_TYPE_CHANNEL))
#define GIMP_IS_CHANNEL_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CHANNEL))

GtkType gimp_channel_get_type (void);


/*  Special undo type  */
typedef struct _ChannelUndo ChannelUndo;

struct _ChannelUndo
{
  Channel          *channel;         /*  the actual channel          */
  gint              prev_position;   /*  former position in list     */
  Channel          *prev_channel;    /*  previous active channel     */
};

/*  Special undo type  */
typedef struct _MaskUndo MaskUndo;

struct _MaskUndo
{
  TileManager *tiles;       /*  the actual mask            */
  gint         x, y;        /*  offsets                    */
};


/*  function declarations  */

Channel *       channel_new         (GimpImage *gimage,
				     gint       width,
				     gint       height,
				     gchar     *name,
				     gint       opacity,
				     guchar    *col);
Channel *       channel_copy        (Channel   *channel);
Channel *	channel_ref         (Channel   *channel);
void   		channel_unref       (Channel   *channel);

gchar *         channel_get_name    (Channel   *channel);
void            channel_set_name    (Channel   *channel, 
				     gchar     *name);

gint	        channel_get_opacity (Channel   *channel);
void		channel_set_opacity (Channel   *channel, 
				     gint       opacity);

guchar *        channel_get_color   (Channel   *channel);
void 		channel_set_color   (Channel   *channel, 
				     guchar    *color);

Channel *       channel_get_ID      (gint       ID);
void            channel_delete      (Channel   *channel);
void            channel_removed     (Channel   *channel, 
				     gpointer   data);
void            channel_scale       (Channel   *channel, 
				     gint       new_width, 
				     gint       new_height);
void            channel_resize      (Channel   *channel, 
				     gint       new_width,
				     gint       new_height,
				     gint       offx,
				     gint       offy);
void            channel_update      (Channel   *channel);

/*  access functions  */

gboolean        channel_toggle_visibility   (Channel *channel);
TempBuf *       channel_preview             (Channel *channel, 
					     gint     width,
					     gint     height);

void            channel_invalidate_previews (GimpImage *gimage);

Tattoo          channel_get_tattoo          (const Channel *channel);
void            channel_set_tattoo          (const Channel *channel,
					     Tattoo         value);

/* selection mask functions  */

Channel *       channel_new_mask        (GimpImage  *gimage,
					 gint        width,
					 gint        height);
gboolean        channel_boundary        (Channel    *mask,
					 BoundSeg  **segs_in,
					 BoundSeg  **segs_out,
					 gint       *num_segs_in,
					 gint       *num_segs_out,
					 gint        x1,
					 gint        y1,
					 gint        x2,
					 gint        y2);
gboolean        channel_bounds          (Channel    *mask,
					 gint       *x1,
					 gint       *y1,
					 gint       *x2,
					 gint       *y2);
gint            channel_value           (Channel    *mask, 
					 gint        x, 
					 gint        y);
gboolean        channel_is_empty        (Channel    *mask);
void            channel_add_segment     (Channel    *mask,
					 gint        x,
					 gint        y,
					 gint        width,
					 gint        value);
void            channel_sub_segment     (Channel    *mask,
					 gint        x,
					 gint        y,
					 gint        width,
					 gint        value);
void            channel_combine_rect    (Channel    *mask,
					 ChannelOps  op,
					 gint        x,
					 gint        y,
					 gint        w,
					 gint        h);
void            channel_combine_ellipse (Channel    *mask,
					 ChannelOps  op,
					 gint        x,
					 gint        y,
					 gint        w,
					 gint        h,
					 gboolean    antialias);
void            channel_combine_mask    (Channel    *mask,
					 Channel    *add_on,
					 ChannelOps  op,
					 gint        off_x,
					 gint        off_y);
void            channel_feather         (Channel    *input,
					 Channel    *output,
					 gdouble     radius_x,
					 gdouble     radius_y,
					 ChannelOps  op,
					 gint        off_x,
					 gint        off_y);

void            channel_push_undo       (Channel    *mask);
void            channel_clear           (Channel    *mask);
void            channel_invert          (Channel    *mask);
void            channel_sharpen         (Channel    *mask);
void            channel_all             (Channel    *mask);

void            channel_border          (Channel   *mask,
					 gint       radius_x,
					 gint       radius_y);
void            channel_grow            (Channel   *mask,
					 gint       radius_x,
					 gint       radius_y);
void            channel_shrink          (Channel   *mask,
					 gint       radius_x,
					 gint       radius_y,
					 gboolean   edge_lock);

void            channel_translate       (Channel   *mask,
					 gint       off_x,
					 gint       off_y);
void            channel_load            (Channel   *mask,
					 Channel   *channel);

void		channel_invalidate_bounds (Channel *channel);

#define drawable_channel GIMP_IS_CHANNEL

#endif /* __CHANNEL_H__ */
