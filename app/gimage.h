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
#ifndef __GIMAGE_H__
#define __GIMAGE_H__

#include "boundary.h"
#include "drawable.h"
#include "channel.h"
#include "layer.h"
#include "tag.h"
#include "temp_buf.h"
#include "tile_manager.h"

struct _PixelArea;
struct _Canvas;

/*
 * NOTE!! These values are duplicated in tag.h
*/

/* the image types */
#define RGB_GIMAGE          0
#define RGBA_GIMAGE         1
#define GRAY_GIMAGE         2
#define GRAYA_GIMAGE        3
#define INDEXED_GIMAGE      4
#define INDEXEDA_GIMAGE     5

/* These are the 16bit types */
#define U16_RGB_GIMAGE         6
#define U16_RGBA_GIMAGE        7
#define U16_GRAY_GIMAGE        8
#define U16_GRAYA_GIMAGE       9
#define U16_INDEXED_GIMAGE     10 
#define U16_INDEXEDA_GIMAGE    11 

/* These are the float types */
#define FLOAT_RGB_GIMAGE          12 
#define FLOAT_RGBA_GIMAGE         13 
#define FLOAT_GRAY_GIMAGE         14 
#define FLOAT_GRAYA_GIMAGE        15 

#define TYPE_HAS_ALPHA(t)  ((t)==RGBA_GIMAGE || (t)==GRAYA_GIMAGE || (t)==INDEXEDA_GIMAGE)

#define GRAY_PIX         0
#define ALPHA_G_PIX      1
#define RED_PIX          0
#define GREEN_PIX        1
#define BLUE_PIX         2
#define ALPHA_PIX        3
#define INDEXED_PIX      0
#define ALPHA_I_PIX      1

#define RGB              0
#define GRAY             1
#define INDEXED          2
#define U16_RGB     	 3    /*16bit*/
#define U16_GRAY    	 4
#define U16_INDEXED		 5
#define FLOAT_RGB 		 6    /*float*/
#define FLOAT_GRAY 		 7

#define MAX_CHANNELS     4

/* the image fill types */
#define BACKGROUND_FILL  0
#define WHITE_FILL       1
#define TRANSPARENT_FILL 2
#define NO_FILL          3
#define FOREGROUND_FILL  4

#define COLORMAP_SIZE    768

#define HORIZONTAL_GUIDE 1
#define VERTICAL_GUIDE   2

typedef enum
{
  Red,
  Green,
  Blue,
  Gray,
  Indexed,
  Auxillary
} ChannelType;

typedef enum
{
  ExpandAsNecessary,
  ClipToImage,
  ClipToBottomLayer,
  FlattenImage
} MergeType;

/* structure declarations */

typedef struct _Guide  Guide;
typedef struct _GImage GImage;

struct _Guide
{
  int ref_count;
  int position;
  int orientation;
};

struct _GImage
{
  char *filename;		      /*  original filename            */
  int has_filename;                   /*  has a valid filename         */

  int width, height;		      /*  width and height attributes  */
  int base_type;                      /*  base gimage type             */
  Tag base_tag;

  unsigned char * cmap;               /*  colormap--for indexed        */
  int num_cols;                       /*  number of cols--for indexed  */

  int dirty;                          /*  dirty flag -- # of ops       */
  int undo_on;                        /*  Is undo enabled?             */

  int instance_count;                 /*  number of instances          */
  int ref_count;                      /*  number of references         */

  TileManager *shadow;                /*  shadow buffer tiles          */
  struct _Canvas *shadow_canvas;

  int ID;                             /*  Unique gimage identifier     */

                                      /*  Projection attributes  */
  int flat;                           /*  Is the gimage flat?          */
  int construct_flag;                 /*  flag for construction        */
  int proj_type;                      /*  type of the projection image */
  int proj_bytes;                     /*  bpp in projection image      */
  int proj_level;                     /*  projection level             */
  TileManager *projection;            /*  The projection--layers &     */
      
                                      /*  channels                     */
  struct _Canvas *projection_canvas;  /*  The projection--layers &     */
                                      /*  channels                     */

  GList *guides;                      /*  guides                       */

                                      /*  Layer/Channel attributes  */
  GSList *layers;                     /*  the list of layers           */
  GSList *channels;                   /*  the list of masks            */
  GSList *layer_stack;                /*  the layers in MRU order      */

  Layer * active_layer;               /*  ID of active layer           */
  Channel * active_channel;	      /*  ID of active channel         */
  Layer * floating_sel;               /*  ID of fs layer               */
  Channel * selection_mask;           /*  selection mask channel       */

  int visible [MAX_CHANNELS];         /*  visible channels             */
  int active  [MAX_CHANNELS];         /*  active channels              */

  int by_color_select;                /*  TRUE if there's an active    */
                                      /*  "by color" selection dialog  */

                                      /*  Undo apparatus  */
  GSList *undo_stack;                 /*  stack for undo operations    */
  GSList *redo_stack;                 /*  stack for redo operations    */
  int undo_bytes;                     /*  bytes in undo stack          */
  int undo_levels;                    /*  levels in undo stack         */
  int pushing_undo_group;             /*  undo group status flag       */

                                      /*  Composite preview  */
  TempBuf *comp_preview;              /*  the composite preview        */
  int comp_preview_valid[3];          /*  preview valid-1/channel      */
};


/* function declarations */

GImage *        gimage_new_tag                (int, int, Tag);
GImage *        gimage_new                    (int, int, int);
void            gimage_set_filename           (GImage *, char *);
void            gimage_resize                 (GImage *, int, int, int, int);
void            gimage_scale                  (GImage *, int, int);
GImage *        gimage_get_named              (char *);
GImage *        gimage_get_ID                 (int);
TileManager *   gimage_shadow                 (GImage *, int, int, int);
struct _Canvas *gimage_shadow_canvas          (GImage *, int, int, Tag);
void            gimage_free_shadow            (GImage *);
void            gimage_free_shadow_canvas     (GImage *);
void            gimage_delete                 (GImage *);

void            gimage_apply_image            ();
void            gimage_apply_painthit         (GImage *, GimpDrawable *,
                                               struct _Canvas *,
                                               struct _PixelArea *,
                                               int undo,
                                               gfloat, int mode,
                                               int x, int y);
void            gimage_replace_image          ();
void            gimage_replace_painthit       (GImage *, GimpDrawable *,
                                               struct _Canvas *, int undo,
                                               gfloat, struct _Canvas *,
                                               int x, int y);

void            gimage_get_foreground         (GImage *, GimpDrawable *, unsigned char *);
void            gimage_get_background         (GImage *, GimpDrawable *, unsigned char *);
void            gimage_get_color              (GImage *, int, unsigned char *,
					       unsigned char *);
void            gimage_transform_color        (GImage *, GimpDrawable *, unsigned char *,
					       unsigned char *, int);
Guide*          gimage_add_hguide             (GImage *);
Guide*          gimage_add_vguide             (GImage *);
void            gimage_add_guide              (GImage *, Guide *);
void            gimage_remove_guide           (GImage *, Guide *);
void            gimage_delete_guide           (GImage *, Guide *);


/*  layer/channel functions  */

int             gimage_get_layer_index        (GImage *, Layer *);
int             gimage_get_channel_index      (GImage *, Channel *);
Layer *         gimage_get_active_layer       (GImage *);
Channel *       gimage_get_active_channel     (GImage *);
Channel *       gimage_get_mask               (GImage *);
int             gimage_get_component_active   (GImage *, ChannelType);
int             gimage_get_component_visible  (GImage *, ChannelType);
int             gimage_layer_boundary         (GImage *, BoundSeg **, int *);
Layer *         gimage_set_active_layer       (GImage *, Layer *);
Channel *       gimage_set_active_channel     (GImage *, Channel *);
Channel *       gimage_unset_active_channel   (GImage *);
void            gimage_set_component_active   (GImage *, ChannelType, int);
void            gimage_set_component_visible  (GImage *, ChannelType, int);
Layer *         gimage_pick_correlate_layer   (GImage *, int, int);
void            gimage_set_layer_mask_apply   (GImage *, int);
void            gimage_set_layer_mask_edit    (GImage *, Layer *, int);
void            gimage_set_layer_mask_show    (GImage *, int);
Layer *         gimage_raise_layer            (GImage *, Layer *);
Layer *         gimage_lower_layer            (GImage *, Layer *);
Layer *         gimage_merge_visible_layers   (GImage *, MergeType);
Layer *         gimage_flatten                (GImage *);
Layer *         gimage_merge_layers           (GImage *, GSList *, MergeType);
Layer *         gimage_add_layer              (GImage *, Layer *, int);
Layer *         gimage_remove_layer           (GImage *, Layer *);
LayerMask *     gimage_add_layer_mask         (GImage *, Layer *, LayerMask *);
Channel *       gimage_remove_layer_mask      (GImage *, Layer *, int);
Channel *       gimage_raise_channel          (GImage *, Channel *);
Channel *       gimage_lower_channel          (GImage *, Channel *);
Channel *       gimage_add_channel            (GImage *, Channel *, int);
Channel *       gimage_remove_channel         (GImage *, Channel *);
void            gimage_construct              (GImage *, int, int, int, int);
void            gimage_invalidate             (GImage *, int, int, int, int, int, int, int, int);
void            gimage_validate               (TileManager *, Tile *, int);
void            gimage_inflate                (GImage *);
void            gimage_deflate                (GImage *);


/*  Access functions  */

int             gimage_is_flat                (GImage *);
int             gimage_is_empty               (GImage *);
GimpDrawable *  gimage_active_drawable        (GImage *);
Tag             gimage_tag                    (GImage *);
int             gimage_base_type              (GImage *);
int             gimage_base_type_with_alpha   (GImage *);
char *          gimage_filename               (GImage *);
int             gimage_enable_undo            (GImage *);
int             gimage_disable_undo           (GImage *);
int             gimage_dirty                  (GImage *);
int             gimage_clean                  (GImage *);
void            gimage_clean_all              (GImage *);
Layer *         gimage_floating_sel           (GImage *);
unsigned char * gimage_cmap                   (GImage *);


/*  projection access functions  */

TileManager *   gimage_projection             (GImage *);
struct _Canvas *gimage_projection_canvas      (GImage *);
int             gimage_projection_type        (GImage *);
int             gimage_projection_bytes       (GImage *);
int             gimage_projection_opacity     (GImage *);
void            gimage_projection_realloc     (GImage *);


/*  composite access functions  */

TileManager    *gimage_composite              (GImage *);
struct _Canvas *gimage_composite_canvas       (GImage *);
int             gimage_composite_type         (GImage *);
int             gimage_composite_bytes        (GImage *);
TempBuf *       gimage_composite_preview      (GImage *, ChannelType, int, int);
int             gimage_preview_valid          (GImage *, ChannelType);
void            gimage_invalidate_preview     (GImage *);

void            gimage_invalidate_previews    (void);

/* from drawable.c */
GImage *        drawable_gimage               (GimpDrawable*);

#endif /* __GIMAGE_H__ */
