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
#ifndef __GIMPIMAGE_H__
#define __GIMPIMAGE_H__

#include "apptypes.h"
#include "procedural_db.h"
#include "gimpimageF.h"
#include "boundary.h"
#include "drawable.h"
#include "channel.h"
#include "layer.h"
#include <libgimp/parasiteF.h>
#include <libgimp/gimpunit.h>
#include "plug_in.h"
#include "temp_buf.h"
#include "tile_manager.h"
#include "pathsP.h"


#define GIMP_TYPE_IMAGE    gimp_image_get_type()
#define GIMP_IMAGE(obj)    GTK_CHECK_CAST (obj, GIMP_TYPE_IMAGE, GimpImage)
#define GIMP_IS_IMAGE(obj) GTK_CHECK_TYPE (obj, GIMP_TYPE_IMAGE)
     
#define TYPE_HAS_ALPHA(t)  ((t)==RGBA_GIMAGE || (t)==GRAYA_GIMAGE || (t)==INDEXEDA_GIMAGE)

#define GRAY_PIX         0
#define ALPHA_G_PIX      1
#define RED_PIX          0
#define GREEN_PIX        1
#define BLUE_PIX         2
#define ALPHA_PIX        3
#define INDEXED_PIX      0
#define ALPHA_I_PIX      1

#define COLORMAP_SIZE    768

typedef enum {
  ORIENTATION_UNKNOWN,
  ORIENTATION_HORIZONTAL,
  ORIENTATION_VERTICAL
} InternalOrientationType;

typedef enum {
  HORIZONTAL,
  VERTICAL,
  UNKNOWN
} OrientationType;

typedef enum
{
  RED_CHANNEL,
  GREEN_CHANNEL,
  BLUE_CHANNEL,
  GRAY_CHANNEL,
  INDEXED_CHANNEL,
  AUXILLARY_CHANNEL
} ChannelType;

typedef enum
{
  EXPAND_AS_NECESSARY,
  CLIP_TO_IMAGE,
  CLIP_TO_BOTTOM_LAYER,
  FLATTEN_IMAGE
} MergeType;


/* Ugly! Move this someplace else! Prolly to gdisplay.. */

struct _Guide
{
  int ref_count;
  int position;
  int orientation;
  guint32 guide_ID;
};


typedef struct _GimpImageRepaintArg
{
  Layer *layer;
  guint  x;
  guint  y;
  guint  width;
  guint  height;
} GimpImageRepaintArg;
	
	
GtkType gimp_image_get_type (void);


/* function declarations */

GimpImage *     gimp_image_new                    (gint, gint,
						   GimpImageBaseType);
void            gimp_image_set_filename           (GimpImage *, gchar *);
void            gimp_image_set_resolution         (GimpImage *,
						   gdouble, gdouble);
void            gimp_image_get_resolution         (GimpImage *,
						   gdouble *, gdouble *);
void            gimp_image_set_unit               (GimpImage *, GUnit);
GUnit           gimp_image_get_unit               (GimpImage *);
void            gimp_image_set_save_proc	  (GimpImage *,
						   PlugInProcDef *);
PlugInProcDef * gimp_image_get_save_proc	  (GimpImage *);
gint		gimp_image_get_width		  (const GimpImage *);
gint		gimp_image_get_height		  (const GimpImage *);
void            gimp_image_resize                 (GimpImage *,
						   gint, gint, gint, gint);
void            gimp_image_scale                  (GimpImage *, gint, gint);
GimpImage *     gimp_image_get_named              (gchar *);
GimpImage *     gimp_image_get_ID                 (gint);
TileManager *   gimp_image_shadow                 (GimpImage *,
						   gint, gint, gint);
void            gimp_image_free_shadow            (GimpImage *);
void            gimp_image_apply_image            (GimpImage *, GimpDrawable *,
						   PixelRegion *, gint,
						   gint,
						   LayerModeEffects,
						   TileManager *, gint, gint);
void            gimp_image_replace_image          (GimpImage *, GimpDrawable *,
						   PixelRegion *, gint, gint,
						   PixelRegion *, gint, gint);
void            gimp_image_get_foreground         (GimpImage *, GimpDrawable *,
						   guchar *);
void            gimp_image_get_background         (GimpImage *, GimpDrawable *,
						   guchar *);
guchar *        gimp_image_get_color_at           (GimpImage *, gint, gint);
						   
void            gimp_image_get_color              (GimpImage *,
						   GimpImageType,
						   guchar *,
						   guchar *);
void            gimp_image_transform_color        (GimpImage *,
						   GimpDrawable *,
						   guchar *,
						   guchar *,
						   GimpImageBaseType);
Guide*          gimp_image_add_hguide             (GimpImage *);
Guide*          gimp_image_add_vguide             (GimpImage *);
void            gimp_image_add_guide              (GimpImage *, Guide *);
void            gimp_image_remove_guide           (GimpImage *, Guide *);
void            gimp_image_delete_guide           (GimpImage *, Guide *);

Parasite *      gimp_image_find_parasite          (const GimpImage *,
						   const gchar *name);
gchar **        gimp_image_parasite_list          (GimpImage *image,
                                                   gint *count);
void            gimp_image_attach_parasite        (GimpImage *, Parasite *);
void            gimp_image_detach_parasite        (GimpImage *, const gchar *);

Tattoo          gimp_image_get_new_tattoo         (GimpImage *);

void            gimp_image_set_paths              (GimpImage *, PathsList *);
PathsList *     gimp_image_get_paths              (GimpImage *);

/* Temporary hack till colormap manipulation is encapsulated in functions.
   Call this whenever you modify an image's colormap. The ncol argument
   specifies which color has changed, or negative if there's a bigger change.
   Currently, use this also when the image's base type is changed to/from
   indexed.  */

void		gimp_image_colormap_changed	  (GimpImage * image,
						   gint ncol);


/*  layer/channel functions  */

gint            gimp_image_get_layer_index        (GimpImage *, Layer *);
gint            gimp_image_get_channel_index      (GimpImage *, Channel *);
Layer *         gimp_image_get_active_layer       (GimpImage *);
Channel *       gimp_image_get_active_channel     (GimpImage *);
Layer *         gimp_image_get_layer_by_tattoo    (GimpImage *, Tattoo);
Channel *       gimp_image_get_channel_by_tattoo  (GimpImage *, Tattoo);
Channel *       gimp_image_get_channel_by_name    (GimpImage *, gchar *);
Channel *       gimp_image_get_mask               (GimpImage *);
gint            gimp_image_get_component_active   (GimpImage *, ChannelType);
gint            gimp_image_get_component_visible  (GimpImage *, ChannelType);
gboolean        gimp_image_layer_boundary         (GimpImage *, BoundSeg **,
						   gint *);
Layer *         gimp_image_set_active_layer       (GimpImage *, Layer *);
Channel *       gimp_image_set_active_channel     (GimpImage *, Channel *);
Channel *       gimp_image_unset_active_channel   (GimpImage *);
void            gimp_image_set_component_active   (GimpImage *, ChannelType,
						   gint);
void            gimp_image_set_component_visible  (GimpImage *, ChannelType,
						   gint);
Layer *         gimp_image_pick_correlate_layer   (GimpImage *, gint, gint);
void            gimp_image_set_layer_mask_apply   (GimpImage *, gint);
void            gimp_image_set_layer_mask_edit    (GimpImage *, Layer *, gint);
void            gimp_image_set_layer_mask_show    (GimpImage *, gint);
Layer *         gimp_image_raise_layer            (GimpImage *, Layer *);
Layer *         gimp_image_lower_layer            (GimpImage *, Layer *);
Layer *         gimp_image_raise_layer_to_top     (GimpImage *, Layer *);
Layer *         gimp_image_lower_layer_to_bottom  (GimpImage *, Layer *);
Layer *         gimp_image_position_layer         (GimpImage *, Layer *, gint);
Layer *         gimp_image_merge_visible_layers   (GimpImage *, MergeType);
Layer *         gimp_image_merge_down             (GimpImage *, Layer *,
						   MergeType);
Layer *         gimp_image_flatten                (GimpImage *);
Layer *         gimp_image_merge_layers           (GimpImage *, GSList *,
						   MergeType);
Layer *         gimp_image_add_layer              (GimpImage *, Layer *, gint);
Layer *         gimp_image_remove_layer           (GimpImage *, Layer *);
LayerMask *     gimp_image_add_layer_mask         (GimpImage *, Layer *,
						   LayerMask *);
Channel *       gimp_image_remove_layer_mask      (GimpImage *, Layer *,
						   MaskApplyMode);
Channel *       gimp_image_raise_channel          (GimpImage *, Channel *);
Channel *       gimp_image_lower_channel          (GimpImage *, Channel *);
Channel *       gimp_image_position_channel       (GimpImage *, Channel *, gint);
Channel *       gimp_image_add_channel            (GimpImage *, Channel *, gint);
Channel *       gimp_image_remove_channel         (GimpImage *, Channel *);
void            gimp_image_construct              (GimpImage *,
						   gint, gint, gint, gint,
						   gboolean);
void            gimp_image_invalidate_without_render (GimpImage *, gint, gint,
						      gint, gint, gint,
						      gint, gint, gint);
void            gimp_image_invalidate             (GimpImage *, gint, gint,
						   gint, gint, gint,
						   gint, gint, gint);
void            gimp_image_validate               (TileManager *, Tile *);

/*  Access functions  */

gboolean           gimp_image_is_empty             (GimpImage *);
GimpDrawable *     gimp_image_active_drawable      (GimpImage *);
GimpImageBaseType  gimp_image_base_type            (GimpImage *);
GimpImageType	   gimp_image_base_type_with_alpha (GimpImage *);
gchar *            gimp_image_filename             (GimpImage *);
gboolean           gimp_image_enable_undo          (GimpImage *);
gboolean           gimp_image_disable_undo         (GimpImage *);
gboolean           gimp_image_freeze_undo          (GimpImage *);
gboolean           gimp_image_thaw_undo            (GimpImage *);
gint               gimp_image_dirty                (GimpImage *);
gint               gimp_image_clean                (GimpImage *);
void               gimp_image_clean_all            (GimpImage *);
Layer *            gimp_image_floating_sel         (GimpImage *);
guchar *           gimp_image_cmap                 (GimpImage *);

/*  projection access functions  */

TileManager *   gimp_image_projection             (GimpImage *);
GimpImageType	gimp_image_projection_type        (GimpImage *);
gint            gimp_image_projection_bytes       (GimpImage *);
gint            gimp_image_projection_opacity     (GimpImage *);
void            gimp_image_projection_realloc     (GimpImage *);


/*  composite access functions  */

TileManager *   gimp_image_composite              (GimpImage *);
GimpImageType	gimp_image_composite_type         (GimpImage *);
gint            gimp_image_composite_bytes        (GimpImage *);
TempBuf *       gimp_image_composite_preview      (GimpImage *, ChannelType,
						   gint, gint);
gint            gimp_image_preview_valid          (GimpImage *, ChannelType);
void            gimp_image_invalidate_preview     (GimpImage *);

void            gimp_image_invalidate_previews    (void);
TempBuf *       gimp_image_construct_composite_preview (GimpImage *,
							gint , gint);

#endif /* __GIMPIMAGE_H__ */
