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

struct _PixelArea;
struct _Canvas;


#define GRAY_PIX         0
#define ALPHA_G_PIX      1
#define RED_PIX          0
#define GREEN_PIX        1
#define BLUE_PIX         2
#define ALPHA_PIX        3
#define INDEXED_PIX      0
#define ALPHA_I_PIX      1

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

  Tag tag;
  int width, height;		      /*  width and height attributes  */

  unsigned char * cmap;               /*  colormap--for indexed        */
  int num_cols;                       /*  number of cols--for indexed  */

  int dirty;                          /*  dirty flag -- # of ops       */
  int undo_on;                        /*  Is undo enabled?             */

  int instance_count;                 /*  number of instances          */
  int ref_count;                      /*  number of references         */

  struct _Canvas *shadow;                /*  shadow buffer tiles          */

  int ID;                             /*  Unique gimage identifier     */

                                      /*  Projection attributes  */
  int construct_flag;                 /*  flag for construction        */
  struct _Canvas *projection;         /*  The projection--layers &     */
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
  struct _Canvas *comp_preview;       /*  the composite preview        */
  int comp_preview_valid[3];          /*  preview valid-1/channel      */
};


/* function declarations */

GImage *        gimage_new                    (int, int, Tag);
void            gimage_set_filename           (GImage *, char *);
void            gimage_resize                 (GImage *, int, int, int, int);
void            gimage_scale                  (GImage *, int, int);
GImage *        gimage_get_ID                 (int);
struct _Canvas *gimage_shadow                 (GImage *, int, int, Tag);
void            gimage_free_shadow            (GImage *);
void            gimage_delete                 (GImage *);

void            gimage_apply_painthit         (GImage *, GimpDrawable *,
                                               struct _Canvas *,
                                               struct _PixelArea *,
                                               int undo,
                                               gfloat, int mode,
                                               int x, int y);
void            gimage_replace_painthit       (GImage *, GimpDrawable *,
                                               struct _Canvas *, int undo,
                                               gfloat, struct _Canvas *,
                                               int x, int y);

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


/*  Access functions  */

int             gimage_is_empty               (GImage *);
GimpDrawable *  gimage_active_drawable        (GImage *);
Tag             gimage_tag                    (GImage *);
Format          gimage_format                 (GImage *);
char *          gimage_filename               (GImage *);
int             gimage_enable_undo            (GImage *);
int             gimage_disable_undo           (GImage *);
int             gimage_dirty                  (GImage *);
int             gimage_clean                  (GImage *);
void            gimage_clean_all              (GImage *);
Layer *         gimage_floating_sel           (GImage *);
unsigned char * gimage_cmap                   (GImage *);


/*  projection access functions  */

struct _Canvas *gimage_projection             (GImage *);
int             gimage_projection_opacity     (GImage *);
void            gimage_projection_realloc     (GImage *);

/*  composite access functions  */

struct _Canvas *gimage_composite_preview      (GImage *, ChannelType, int, int);
int             gimage_preview_valid          (GImage *, ChannelType);
void            gimage_invalidate_preview     (GImage *);

void            gimage_invalidate_previews    (void);

/* from drawable.c */
GImage *        drawable_gimage               (GimpDrawable*);

#endif /* __GIMAGE_H__ */
