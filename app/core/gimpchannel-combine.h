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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

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

typedef struct _Channel Channel;

struct _Channel
{
  char * name;                  /*  name of the channel          */

  TileManager *tiles;           /*  tiles for channel data       */
  int visible;                  /*  controls visibility          */

  int width, height;            /*  size of channel              */
  int bytes;                    /*  bytes per pixel              */

  unsigned char col[3];         /*  RGB triplet for channel color*/
  int opacity;                  /*  Channel opacity              */
  int show_masked;              /*  Show masked areas--as        */
                                /*  opposed to selected areas    */

  int dirty;                    /*  dirty bit                    */

  int ID;                       /*  provides a unique ID         */
  int layer_ID;                 /*  ID of layer-if a layer mask  */
  int gimage_ID;                /*  ID of gimage owner           */

  /*  Selection mask variables  */
  int boundary_known;           /*  is the current boundary valid*/
  BoundSeg  *segs_in;           /*  outline of selected region   */
  BoundSeg  *segs_out;          /*  outline of selected region   */
  int num_segs_in;              /*  number of lines in boundary  */
  int num_segs_out;             /*  number of lines in boundary  */
  int empty;                    /*  is the region empty?         */
  int bounds_known;             /*  recalculate the bounds?      */
  int x1, y1;                   /*  coordinates for bounding box */
  int x2, y2;                   /*  lower right hand coordinate  */

  /*  Preview variables  */
  TempBuf *preview;             /*  preview of the channel       */
  int preview_valid;            /*  is the preview valid?        */
};


/*  Special undo type  */
typedef struct _channel_undo ChannelUndo;

struct _channel_undo
{
  Channel * channel;   /*  the actual channel         */
  int prev_position;   /*  former position in list    */
  int prev_channel;    /*  previous active channel    */
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

void            channel_allocate (Channel *, int, int);
void            channel_deallocate (Channel *);
Channel *       channel_new (int, int, int, char *, int, unsigned char *);
Channel *       channel_copy (Channel *);
Channel *       channel_get_ID (int);
void            channel_delete (Channel *);
void            channel_apply_image (Channel *, int, int, int, int, TileManager *, int);
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
void            channel_layer_alpha     (Channel *, int);
void            channel_layer_mask      (Channel *, int);
void            channel_load            (Channel *, Channel *);

#endif /* __CHANNEL_H__ */
