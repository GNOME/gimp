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
#include "boundary.h"
#include "drawable.h"
#include "channel.h"
#include "layer.h"
#include "plug_in.h"
#include "temp_buf.h"
#include "tile_manager.h"
#include "path.h"

#include "libgimp/gimpparasite.h"
#include "libgimp/gimpunit.h"


#define GIMP_TYPE_IMAGE    gimp_image_get_type()
#define GIMP_IMAGE(obj)    GTK_CHECK_CAST (obj, GIMP_TYPE_IMAGE, GimpImage)
#define GIMP_IS_IMAGE(obj) GTK_CHECK_TYPE (obj, GIMP_TYPE_IMAGE)
     
#define GIMP_IMAGE_TYPE_HAS_ALPHA(t)  ((t)==RGBA_GIMAGE || (t)==GRAYA_GIMAGE || (t)==INDEXEDA_GIMAGE)

#define GRAY_PIX         0
#define ALPHA_G_PIX      1
#define RED_PIX          0
#define GREEN_PIX        1
#define BLUE_PIX         2
#define ALPHA_PIX        3
#define INDEXED_PIX      0
#define ALPHA_I_PIX      1

#define COLORMAP_SIZE    768

typedef enum /*< skip >*/
{ 
  ORIENTATION_UNKNOWN,
  ORIENTATION_HORIZONTAL,
  ORIENTATION_VERTICAL
} InternalOrientationType;

typedef enum
{
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
  gint                     ref_count;
  gint                     position;
  InternalOrientationType  orientation;
  guint32                  guide_ID;
};


typedef struct _GimpImageRepaintArg
{
  Layer *layer;
  guint  x;
  guint  y;
  guint  width;
  guint  height;
} GimpImageRepaintArg;
	
	
/* function declarations */

GtkType         gimp_image_get_type          (void);

GimpImage     * gimp_image_new               (gint                width,
					      gint                height,
					      GimpImageBaseType   base_type);
void            gimp_image_set_filename      (GimpImage          *gimage,
					      gchar              *filename);
void            gimp_image_set_resolution    (GimpImage          *gimage,
					      gdouble             xres,
					      gdouble             yres);
void            gimp_image_get_resolution    (const GimpImage    *gimage,
					      gdouble            *xresolution,
					      gdouble            *yresolution);
void            gimp_image_set_unit          (GimpImage          *gimage,
					      GimpUnit            unit);
GimpUnit        gimp_image_get_unit          (const GimpImage    *gimage);
void            gimp_image_set_save_proc     (GimpImage          *gimage,
					      PlugInProcDef      *proc);
PlugInProcDef * gimp_image_get_save_proc     (const GimpImage    *gimage);
gint		gimp_image_get_width         (const GimpImage    *gimage);
gint		gimp_image_get_height        (const GimpImage    *gimage);
void            gimp_image_resize            (GimpImage          *gimage,
					      gint                new_width,
					      gint                new_height,
					      gint                offset_x,
					      gint                offset_y);
void            gimp_image_scale             (GimpImage          *gimage,
					      gint                new_width,
					      gint                new_height);
TileManager   * gimp_image_shadow            (GimpImage          *gimage,
					      gint                width,
					      gint                height,
					      gint                bpp);
void            gimp_image_free_shadow       (GimpImage          *gimage);
void            gimp_image_apply_image       (GimpImage          *gimage,
					      GimpDrawable       *drawable,
					      PixelRegion        *src2PR,
					      gint                undo,
					      gint                opacity,
					      LayerModeEffects    mode,
					      TileManager        *src1_tiles,
					      gint                x,
					      gint                y);
void            gimp_image_replace_image     (GimpImage          *gimage,
					      GimpDrawable       *drawable,
					      PixelRegion        *src2PR,
					      gint                undo,
					      gint                opacity,
					      PixelRegion        *maskPR,
					      gint                x,
					      gint                y);
void            gimp_image_get_foreground    (const GimpImage    *gimage,
					      const GimpDrawable *drawable,
					      guchar             *fg);
void            gimp_image_get_background    (const GimpImage    *gimage,
					      const GimpDrawable *drawable,
					      guchar             *bg);
guchar        * gimp_image_get_color_at      (GimpImage          *gimage,
					      gint                x,
					      gint                y);
void            gimp_image_get_color         (const GimpImage    *gimage,
					      GimpImageType       d_type,
					      guchar             *rgb,
					      guchar             *src);
void            gimp_image_transform_color   (const GimpImage    *gimage,
					      const GimpDrawable *drawable,
					      guchar             *src,
					      guchar             *dest,
					      GimpImageBaseType   type);
Guide         * gimp_image_add_hguide        (GimpImage          *gimage);
Guide         * gimp_image_add_vguide        (GimpImage          *gimage);
void            gimp_image_add_guide         (GimpImage          *gimage,
					      Guide              *guide);
void            gimp_image_remove_guide      (GimpImage          *gimage,
					      Guide              *guide);
void            gimp_image_delete_guide      (GimpImage          *gimage,
					      Guide              *guide);

GimpParasite  * gimp_image_parasite_find     (const GimpImage    *gimage,
					      const gchar        *name);
gchar        ** gimp_image_parasite_list     (const GimpImage    *gimage,
					      gint               *count);
void            gimp_image_parasite_attach   (GimpImage          *gimage,
					      GimpParasite       *parasite);
void            gimp_image_parasite_detach   (GimpImage          *gimage,
					      const gchar        *parasite);

Tattoo          gimp_image_get_new_tattoo    (GimpImage          *gimage);
gboolean        gimp_image_set_tattoo_state  (GimpImage          *gimage,
					      Tattoo              val);
Tattoo          gimp_image_get_tattoo_state  (GimpImage          *gimage);

void            gimp_image_set_paths         (GimpImage          *gimage,
					      PathList           *paths);
PathList      * gimp_image_get_paths         (const GimpImage    *gimage);

/* Temporary hack till colormap manipulation is encapsulated in functions.
   Call this whenever you modify an image's colormap. The ncol argument
   specifies which color has changed, or negative if there's a bigger change.
   Currently, use this also when the image's base type is changed to/from
   indexed.  */

void		gimp_image_colormap_changed  (const GimpImage    *image,
					      gint                col);


/*  layer/channel functions  */

gint          gimp_image_get_layer_index       (const GimpImage *gimage,
						const Layer     *layer_arg);
Layer       * gimp_image_get_layer_by_index    (const GimpImage *gimage,
						gint             layer_index);
gint          gimp_image_get_channel_index     (const GimpImage *gimage,
						const Channel   *channel_arg);
Layer       * gimp_image_get_active_layer      (const GimpImage *gimage);
Channel     * gimp_image_get_active_channel    (const GimpImage *gimage);
Layer       * gimp_image_get_layer_by_tattoo   (const GimpImage *gimage,
						Tattoo           tatoo);
Channel     * gimp_image_get_channel_by_tattoo (const GimpImage *gimage,
						Tattoo           tatoo);
Channel     * gimp_image_get_channel_by_name   (const GimpImage *gimage,
						const gchar     *name);
Channel     * gimp_image_get_mask              (const GimpImage *gimage);
gboolean      gimp_image_get_component_active  (const GimpImage *gimage,
						ChannelType      type);
gboolean      gimp_image_get_component_visible (const GimpImage *gimage,
						ChannelType      type);
gboolean      gimp_image_layer_boundary        (const GimpImage *gimage,
						BoundSeg       **segs,
						gint            *n_segs);
Layer       * gimp_image_set_active_layer      (GimpImage       *gimage,
						Layer           *layer);
Channel     * gimp_image_set_active_channel    (GimpImage       *gimage,
						Channel         *channel);
Channel     * gimp_image_unset_active_channel  (GimpImage       *gimage);
void          gimp_image_set_component_active  (GimpImage       *gimage,
						ChannelType      type,
						gboolean         active);
void          gimp_image_set_component_visible (GimpImage       *gimage,
						ChannelType      type,
						gboolean         visible);
Layer       * gimp_image_pick_correlate_layer  (const GimpImage *gimage,
						gint             x,
						gint             y);
Layer       * gimp_image_raise_layer           (GimpImage       *gimage,
						Layer           *layer_arg);
Layer       * gimp_image_lower_layer           (GimpImage       *gimage,
						Layer           *layer_arg);
Layer       * gimp_image_raise_layer_to_top    (GimpImage       *gimage,
						Layer           *layer_arg);
Layer       * gimp_image_lower_layer_to_bottom (GimpImage       *gimage,
						Layer           *layer_arg);
Layer       * gimp_image_position_layer        (GimpImage       *gimage,
						Layer           *layer_arg,
						gint             new_index,
						gboolean         push_undo);
Layer       * gimp_image_merge_visible_layers  (GimpImage       *gimage,
						MergeType        merge_type);
Layer       * gimp_image_merge_down            (GimpImage       *gimage,
						Layer           *current_layer,
						MergeType        merge_type);
Layer       * gimp_image_flatten               (GimpImage       *gimage);
Layer       * gimp_image_merge_layers          (GimpImage       *gimage,
						GSList          *merge_list,
						MergeType        merge_type);
Layer       * gimp_image_add_layer             (GimpImage       *gimage,
						Layer           *float_layer,
						gint             position);
Layer       * gimp_image_remove_layer          (GimpImage       *gimage,
						Layer           *layer);
LayerMask   * gimp_image_add_layer_mask        (GimpImage       *gimage,
						Layer           *layer,
						LayerMask       *mask);
Channel     * gimp_image_remove_layer_mask     (GimpImage       *gimage,
						Layer           *layer,
						MaskApplyMode    mode);
Channel     * gimp_image_raise_channel         (GimpImage       *gimage,
						Channel         *channel_arg);
Channel     * gimp_image_lower_channel         (GimpImage       *gimage,
						Channel         *channel_arg);
Channel     * gimp_image_position_channel      (GimpImage       *gimage,
						Channel         *channel_arg,
						gint             position);
Channel     * gimp_image_add_channel           (GimpImage       *gimage,
						Channel         *channel,
						gint             position);
Channel     * gimp_image_remove_channel        (GimpImage       *gimage,
						Channel         *channel);
void          gimp_image_construct             (GimpImage       *gimage,
						gint             x,
						gint             y,
						gint             w,
						gint             h);


void          gimp_image_invalidate_without_render (GimpImage   *gimage,
						    gint         x,
						    gint         y,
						    gint         w,
						    gint         h,
						    gint         x1,
						    gint         y1,
						    gint         x2,
						    gint         y2);
void          gimp_image_invalidate                (GimpImage   *gimage,
						    gint         x,
						    gint         y,
						    gint         w,
						    gint         h,
						    gint         x1,
						    gint         y1,
						    gint         x2,
						    gint         y2);
void          gimp_image_validate                  (TileManager *tm,
						    Tile        *tile);

/*  Access functions  */

gboolean           gimp_image_is_empty             (const GimpImage *gimage);
GimpDrawable *     gimp_image_active_drawable      (const GimpImage *gimage);

GimpImageBaseType  gimp_image_base_type            (const GimpImage *gimage);
GimpImageType	   gimp_image_base_type_with_alpha (const GimpImage *gimage);

const gchar      * gimp_image_filename             (const GimpImage *gimage);
gboolean           gimp_image_undo_is_enabled      (const GimpImage *gimage);
gboolean           gimp_image_undo_enable          (GimpImage       *gimage);
gboolean           gimp_image_undo_disable         (GimpImage       *gimage);
gboolean           gimp_image_undo_freeze          (GimpImage       *gimage);
gboolean           gimp_image_undo_thaw            (GimpImage       *gimage);
void		   gimp_image_undo_event	   (GimpImage       *gimage,
						    gint             event);
gint               gimp_image_dirty                (GimpImage       *gimage);
gint               gimp_image_clean                (GimpImage       *gimage);
void               gimp_image_clean_all            (GimpImage       *gimage);
Layer            * gimp_image_floating_sel         (const GimpImage *gimage);
guchar           * gimp_image_cmap                 (const GimpImage *gimage);

/*  projection access functions  */

TileManager   * gimp_image_projection              (GimpImage       *gimage);
GimpImageType	gimp_image_projection_type         (const GimpImage *gimage);
gint            gimp_image_projection_bytes        (const GimpImage *gimage);
gint            gimp_image_projection_opacity      (const GimpImage *gimage);
void            gimp_image_projection_realloc      (GimpImage       *gimage);


/*  composite access functions  */

TileManager   * gimp_image_composite               (GimpImage       *gimage);
GimpImageType	gimp_image_composite_type          (const GimpImage *gimage);
gint            gimp_image_composite_bytes         (const GimpImage *gimage);
TempBuf       * gimp_image_composite_preview       (GimpImage       *gimage,
						    ChannelType      type,
						    gint             width,
						    gint             height);

gint            gimp_image_preview_valid           (const GimpImage *gimage,
						    ChannelType      type);
void            gimp_image_invalidate_preview      (GimpImage       *gimage);

void            gimp_image_invalidate_previews     (void);

TempBuf       * gimp_image_construct_composite_preview (GimpImage *gimage,
							gint       width,
							gint       height);

#endif /* __GIMPIMAGE_H__ */
