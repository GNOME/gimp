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
#ifndef __LAYER_H__
#define __LAYER_H__

#include "drawable.h"

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

#define GIMP_TYPE_LAYER                  (gimp_layer_get_type ())
#define GIMP_LAYER(obj)                  (GTK_CHECK_CAST ((obj), GIMP_TYPE_LAYER, GimpLayer))
#define GIMP_LAYER_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER, GimpLayerClass))
#define GIMP_IS_LAYER(obj)               (GTK_CHECK_TYPE ((obj), GIMP_TYPE_LAYER))
#define GIMP_IS_LAYER_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER))

#define GIMP_TYPE_LAYER_MASK                  (gimp_layer_mask_get_type ())
#define GIMP_LAYER_MASK(obj)                  (GTK_CHECK_CAST ((obj), GIMP_TYPE_LAYER_MASK, GimpLayerMask))
#define GIMP_LAYER_MASK_CLASS(klass)          (GTK_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LAYER_MASK, GimpLayerMaskClass))
#define GIMP_IS_LAYER_MASK(obj)               (GTK_CHECK_TYPE ((obj), GIMP_TYPE_LAYER_MASK))
#define GIMP_IS_LAYER_MASK_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LAYER_MASK))

typedef struct _GimpLayer      GimpLayer;
typedef struct _GimpLayerClass GimpLayerClass;
typedef struct _GimpLayerMask      GimpLayerMask;
typedef struct _GimpLayerMaskClass GimpLayerMaskClass;

typedef GimpLayer Layer;		/* convenience */
typedef GimpLayerMask LayerMask;	/* convenience */

GtkType gimp_layer_get_type (void);
GtkType gimp_layer_mask_get_type (void);

/*  Special undo types  */

typedef struct _layer_undo LayerUndo;

struct _layer_undo
{
  Layer * layer;     /*  the actual layer         */
  int prev_position; /*  former position in list  */
  Layer * prev_layer;    /*  previous active layer    */
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
  LayerMask *mask;   /*  the layer mask           */
  int mode;          /*  the application mode     */
  int undo_type;     /*  is this a new layer mask */
                     /*  or a remove layer mask   */
};

typedef struct _fs_to_layer_undo FStoLayerUndo;

struct _fs_to_layer_undo
{
  Layer * layer;     /*  the layer                */
  GimpDrawable * drawable;      /*  drawable of floating sel */
};

/* function declarations */

Layer *         layer_new (GimpImage*, int, int, int, char *, int, int);
Layer *         layer_copy (Layer *, int);
Layer *		layer_ref (Layer *);
void   		layer_unref (Layer *);

Layer *         layer_from_tiles (void *, GimpDrawable *, TileManager *, char *, int, int);
LayerMask *     layer_add_mask (Layer *, LayerMask *);
LayerMask *     layer_create_mask (Layer *, AddMaskType);
Layer *         layer_get_ID (int);
void            layer_delete (Layer *);
void            layer_apply_mask (Layer *, int);
void            layer_translate (Layer *, int, int);
void            layer_add_alpha (Layer *);
void            layer_scale (Layer *, int, int, int);
void            layer_resize (Layer *, int, int, int, int);
BoundSeg *      layer_boundary (Layer *, int *);
void            layer_invalidate_boundary (Layer *);
int             layer_pick_correlate (Layer *, int, int);

LayerMask *     layer_mask_new	(GimpImage*, int, int, char *, 
				 int, unsigned char *);
LayerMask *	layer_mask_copy	(LayerMask *);
void		layer_mask_delete	(LayerMask *);
LayerMask *	layer_mask_get_ID    (int);
LayerMask *	layer_mask_ref (LayerMask *);
void   		layer_mask_unref (LayerMask *);

/* access functions */

unsigned char * layer_data (Layer *);
LayerMask *     layer_mask (Layer *);
int             layer_has_alpha (Layer *);
int             layer_is_floating_sel (Layer *);
int		layer_linked (Layer *);
TempBuf *       layer_preview (Layer *, int, int);
TempBuf *       layer_mask_preview (Layer *, int, int);

void            layer_invalidate_previews  (GimpImage*);


/* from drawable.c */
Layer *          drawable_layer              (GimpDrawable *);
LayerMask *      drawable_layer_mask         (GimpDrawable *);

/* from channel.c */
void            channel_layer_alpha     (Channel *, Layer *);
void            channel_layer_mask      (Channel *, Layer *);

#endif /* __LAYER_H__ */
