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

Channel *       channel_new         (GimpImage*,
				     gint, gint, gchar *, gint, guchar *);
Channel *       channel_copy        (Channel *);
Channel *	channel_ref         (Channel *);
void   		channel_unref       (Channel *);

gchar *         channel_get_name    (Channel *);
void            channel_set_name    (Channel *, gchar *);

gint	        channel_get_opacity (Channel *);
void		channel_set_opacity (Channel *, gint);

gchar *		channel_get_color   (Channel *);
void 		channel_set_color   (Channel *, gchar *);

Channel *       channel_get_ID      (gint);
void            channel_delete      (Channel *);
void            channel_scale       (Channel *, gint, gint);
void            channel_resize      (Channel *, gint, gint, gint, gint);
void            channel_update      (Channel *);

/*  access functions  */

gboolean        channel_toggle_visibility   (Channel *);
TempBuf *       channel_preview             (Channel *, gint, gint);

void            channel_invalidate_previews (GimpImage*);
Tattoo          channel_get_tattoo          (const Channel *);

/* selection mask functions  */

Channel *       channel_new_mask        (GimpImage *, gint, gint);
gboolean        channel_boundary        (Channel *, BoundSeg **, BoundSeg **,
					 gint *, gint *, gint, gint, gint, gint);
gboolean        channel_bounds          (Channel *,
					 gint *, gint *, gint *, gint *);
gint            channel_value           (Channel *, gint, gint);
gboolean        channel_is_empty        (Channel *);
void            channel_add_segment     (Channel *, gint, gint, gint, gint);
void            channel_sub_segment     (Channel *, gint, gint, gint, gint);
void            channel_inter_segment   (Channel *, gint, gint, gint, gint);
void            channel_combine_rect    (Channel *,
					 ChannelOps, gint, gint, gint, gint);
void            channel_combine_ellipse (Channel *,
					 ChannelOps,
					 gint, gint, gint, gint, gboolean);
void            channel_combine_mask    (Channel *, Channel *,
					 ChannelOps, gint, gint);
void            channel_feather         (Channel *, Channel *,
					 gdouble, gdouble,
					 ChannelOps, gint, gint);
void            channel_push_undo       (Channel *);
void            channel_clear           (Channel *);
void            channel_invert          (Channel *);
void            channel_sharpen         (Channel *);
void            channel_all             (Channel *);

void            channel_border          (Channel * channel,
					 gint      radius_x,
					 gint      radius_y);
void            channel_grow            (Channel * channel,
					 gint      radius_x,
					 gint      radius_y);
void            channel_shrink          (Channel * channel,
					 gint      radius_x,
					 gint      radius_y,
					 gboolean  edge_lock);

void            channel_translate       (Channel *, gint, gint);
void            channel_load            (Channel *, Channel *);

void		channel_invalidate_bounds (Channel *);

#define drawable_channel GIMP_IS_CHANNEL

#endif /* __CHANNEL_H__ */
