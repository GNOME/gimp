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

#include "drawable.h"

#include "boundary.h"
#include "temp_buf.h"
#include "tile_manager.h"

/* OPERATIONS */

#define ADD       0
#define SUB       1
#define REPLACE   2
#define INTERSECT 3

/*  Half way point where a region is no longer visible in a selection  */
#define HALF_WAY 127

/* structure declarations */

#define GIMP_CHANNEL(obj)        GTK_CHECK_CAST (obj, gimp_channel_get_type (), GimpChannel)
#define GIMP_CHANNEL_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gimp_channel_get_type(), GimpChannelClass)
#define GIMP_IS_CHANNEL(obj)     GTK_CHECK_TYPE (obj, gimp_channel_get_type())

typedef struct _GimpChannel      GimpChannel;
typedef struct _GimpChannelClass GimpChannelClass;

typedef GimpChannel Channel;		/* convenience */

guint gimp_channel_get_type (void);

/*  Special undo type  */
typedef struct _channel_undo ChannelUndo;

struct _channel_undo
{
  Channel * channel;   /*  the actual channel         */
  int prev_position;   /*  former position in list    */
  Channel * prev_channel;    /*  previous active channel    */
  int undo_type;       /*  is this a new channel undo */
                       /*  or a remove channel undo?  */
};

/*  Special undo type  */
typedef struct _mask_undo MaskUndo;

struct _mask_undo
{
  TileManager * tiles; /*  the actual mask  */
  int x, y;            /*  offsets          */
};


/* function declarations */

Channel *       channel_new (int, int, int, char *, int, unsigned char *);
Channel *       channel_copy (Channel *);
Channel *	channel_ref (Channel *);
void   		channel_unref (Channel *);

Channel *       channel_get_ID (int);
void            channel_delete (Channel *);
void            channel_scale (Channel *, int, int);
void            channel_resize (Channel *, int, int, int, int);

/* access functions */

unsigned char * channel_data (Channel *);
int             channel_toggle_visibility (Channel *);
TempBuf *       channel_preview (Channel *, int, int);

void            channel_invalidate_previews (int);

/* selection mask functions  */

Channel *       channel_new_mask        (int, int, int);
int             channel_boundary        (Channel *, BoundSeg **, BoundSeg **,
					 int *, int *, int, int, int, int);
int             channel_bounds          (Channel *, int *, int *, int *, int *);
int             channel_value           (Channel *, int, int);
int             channel_is_empty        (Channel *);
void            channel_add_segment     (Channel *, int, int, int, int);
void            channel_sub_segment     (Channel *, int, int, int, int);
void            channel_inter_segment   (Channel *, int, int, int, int);
void            channel_combine_rect    (Channel *, int, int, int, int, int);
void            channel_combine_ellipse (Channel *, int, int, int, int, int, int);
void            channel_combine_mask    (Channel *, Channel *, int, int, int);
void            channel_feather         (Channel *, Channel *, double, int, int, int);
void            channel_push_undo       (Channel *);
void            channel_clear           (Channel *);
void            channel_invert          (Channel *);
void            channel_sharpen         (Channel *);
void            channel_all             (Channel *);
void            channel_border          (Channel *, int);
void            channel_grow            (Channel *, int);
void            channel_shrink          (Channel *, int);
void            channel_translate       (Channel *, int, int);
void            channel_load            (Channel *, Channel *);
void		channel_invalidate_bounds (Channel *);

extern int channel_get_count;

/* from drawable.c */
Channel *        drawable_channel       (GimpDrawable *);

#endif /* __CHANNEL_H__ */
