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
#ifndef __LAYER_H__
#define __LAYER_H__

#include "boundary.h"
#include "channel.h"
#include "temp_buf.h"
#include "tile_manager.h"

#define APPLY   0
#define DISCARD 1

typedef enum
{
  WhiteMask,
  BlackMask,
  AlphaMask
} AddMaskType;


/* structure declarations */

typedef struct _Layer Layer;

struct _Layer
{
  char * name;                  /*  name of the layer            */

  TileManager * tiles;          /*  Tiles for layer data         */
  int visible;                  /*  controls visibility          */
  int linked;                   /*  control linkage              */
  int preserve_trans;           /*  preserve transparency?       */

  Channel * mask;               /*  possible layer mask          */
  int apply_mask;               /*  controls mask application    */
  int edit_mask;                /*  edit mask or layer?          */
  int show_mask;                /*  show mask or layer?          */

  int offset_x, offset_y;       /*  offset of layer in image     */

  int type;                     /*  type of image                */
  int width, height;            /*  size of layer                */
  int bytes;                    /*  bytes per pixel              */

  int opacity;                  /*  layer opacity                */
  int mode;                     /*  layer combination mode       */

  int dirty;                    /*  dirty bit                    */

  int ID;                       /*  provides a unique ID         */
  int gimage_ID;                /*  ID of gimage owner           */

  /*  Preview variables  */
  TempBuf *preview;             /*  preview of the channel       */
  int preview_valid;            /*  is the preview valid?        */

  /*  Floating selections  */
  struct
  {
    TileManager *backing_store; /*  for obscured regions         */
    int drawable;               /*  floating sel is attached to  */
    int initial;                /*  is fs composited yet?        */

    int boundary_known;         /*  is the current boundary valid*/
    BoundSeg *segs;             /*  boundary of floating sel     */
    int num_segs;               /*  number of segs in boundary   */
  } fs;
};


/*  Special undo types  */

typedef struct _layer_undo LayerUndo;

struct _layer_undo
{
  Layer * layer;     /*  the actual layer         */
  int prev_position; /*  former position in list  */
  int prev_layer;    /*  previous active layer    */
  int undo_type;     /*  is this a new layer undo */
                     /*  or a remove layer undo?  */
};

typedef struct _layer_mask_undo LayerMaskUndo;

struct _layer_mask_undo
{
  Layer * layer;     /*  the layer                */
  int apply_mask;    /*  apply mask?              */
  int edit_mask;     /*  edit mask or layer?      */
  int show_mask;     /*  show the mask?           */
  Channel *mask;     /*  the layer mask           */
  int mode;          /*  the application mode     */
  int undo_type;     /*  is this a new layer mask */
                     /*  or a remove layer mask   */
};

typedef struct _fs_to_layer_undo FStoLayerUndo;

struct _fs_to_layer_undo
{
  Layer * layer;     /*  the layer                */
  int drawable;      /*  drawable of floating sel */
};

/* function declarations */

void            layer_allocate (Layer *, int, int, int);
void            layer_deallocate (Layer *);
Layer *         layer_new (int, int, int, int, char *, int, int);
Layer *         layer_copy (Layer *, int);
Layer *         layer_from_tiles (void *, int, TileManager *, char *, int, int);
Channel *       layer_add_mask (Layer *, int);
Channel *       layer_create_mask (Layer *, AddMaskType);
Layer *         layer_get_ID (int);
void            layer_delete (Layer *);
void            layer_apply_mask (Layer *, int);
void            layer_translate (Layer *, int, int);
void            layer_apply_image (Layer *, int, int, int, int, TileManager *, int);
void            layer_add_alpha (Layer *);
void            layer_scale (Layer *, int, int, int);
void            layer_resize (Layer *, int, int, int, int);
BoundSeg *      layer_boundary (Layer *, int *);
void            layer_invalidate_boundary (Layer *);
int             layer_pick_correlate (Layer *, int, int);

/* access functions */

unsigned char * layer_data (Layer *);
Channel *       layer_mask (Layer *);
int             layer_has_alpha (Layer *);
int             layer_is_floating_sel (Layer *);
TempBuf *       layer_preview (Layer *, int, int);
TempBuf *       layer_mask_preview (Layer *, int, int);

void            layer_invalidate_previews  (int);

#endif /* __LAYER_H__ */
