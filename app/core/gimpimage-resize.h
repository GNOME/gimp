#ifndef __GIMPIMAGE_H__
#define __GIMPIMAGE_H__

#include "procedural_db.h"
#include "gimpimageF.h"

#include "boundary.h"
#include "drawable.h"
#include "channel.h"
#include "layer.h"
#include <libgimp/parasiteF.h>
#include "plug_in.h"
#include "temp_buf.h"
#include "tile_manager.h"


#define GIMP_TYPE_IMAGE gimp_image_get_type()

#define GIMP_IMAGE(obj) GTK_CHECK_CAST (obj, GIMP_TYPE_IMAGE, GimpImage)
     
#define GIMP_IS_IMAGE(obj) GTK_CHECK_TYPE (obj, GIMP_TYPE_IMAGE)
     

/* the image types */
typedef enum
{
        RGB_GIMAGE,
        RGBA_GIMAGE,
        GRAY_GIMAGE,
        GRAYA_GIMAGE,  
        INDEXED_GIMAGE,
        INDEXEDA_GIMAGE
} GimpImageType;



#define TYPE_HAS_ALPHA(t)  ((t)==RGBA_GIMAGE || (t)==GRAYA_GIMAGE || (t)==INDEXEDA_GIMAGE)

#define GRAY_PIX         0
#define ALPHA_G_PIX      1
#define RED_PIX          0
#define GREEN_PIX        1
#define BLUE_PIX         2
#define ALPHA_PIX        3
#define INDEXED_PIX      0
#define ALPHA_I_PIX      1

typedef enum
{
        RGB,
        GRAY,
        INDEXED
} GimpImageBaseType;


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


/* Ugly! Move this someplace else! Prolly to gdisplay.. */

struct _Guide
{
  int ref_count;
  int position;
  int orientation;
  guint32 guide_ID;
};


typedef struct _GimpImageRepaintArg{
	Layer* layer;
	guint x;
	guint y;
	guint width;
	guint height;
} GimpImageRepaintArg;
	
	
GtkType gimp_image_get_type(void);


/* function declarations */

GimpImage *        gimp_image_new                    (int, int, int);
void            gimp_image_set_filename           (GimpImage *, char *);
void            gimp_image_set_resolution         (GimpImage *, float, float);
void            gimp_image_get_resolution         (GimpImage *,
						   float *,
						   float *);
void            gimp_image_set_save_proc    (GimpImage *, PlugInProcDef *);
PlugInProcDef * gimp_image_get_save_proc    (GimpImage *);
void            gimp_image_resize                 (GimpImage *, int, int, int, int);
void            gimp_image_scale                  (GimpImage *, int, int);
GimpImage *        gimp_image_get_named              (char *);
GimpImage *        gimp_image_get_ID                 (int);
TileManager *   gimp_image_shadow                 (GimpImage *, int, int, int);
void            gimp_image_free_shadow            (GimpImage *);
void            gimp_image_apply_image            (GimpImage *, GimpDrawable *,
						   PixelRegion *, int,
						   int, int,
						   TileManager *, int, int);
void            gimp_image_replace_image          (GimpImage *, GimpDrawable *,
						   PixelRegion *, int, int,
						   PixelRegion *, int, int);
void            gimp_image_get_foreground         (GimpImage *, GimpDrawable *,
						   unsigned char *);
void            gimp_image_get_background         (GimpImage *, GimpDrawable *,
						   unsigned char *);
void            gimp_image_get_color              (GimpImage *, int,
						   unsigned char *,
						   unsigned char *);
void            gimp_image_transform_color        (GimpImage *, GimpDrawable *,
						   unsigned char *,
						   unsigned char *, int);
Guide*          gimp_image_add_hguide             (GimpImage *);
Guide*          gimp_image_add_vguide             (GimpImage *);
void            gimp_image_add_guide              (GimpImage *, Guide *);
void            gimp_image_remove_guide           (GimpImage *, Guide *);
void            gimp_image_delete_guide           (GimpImage *, Guide *);

Parasite *      gimp_image_find_parasite          (const GimpImage *,
						   const char *name);
void            gimp_image_attach_parasite        (GimpImage *, Parasite *);
void            gimp_image_detach_parasite        (GimpImage *, const char *);

Tattoo          gimp_image_get_new_tattoo         (GimpImage *);

/* Temporary hack till colormap manipulation is encapsulated in functions.
   Call this whenever you modify an image's colormap. The ncol argument
   specifies which color has changed, or negative if there's a bigger change.
   Currently, use this also when the image's base type is changed to/from
   indexed.  */

void		gimp_image_colormap_changed	  (GimpImage * image,
						   gint ncol);


/*  layer/channel functions  */

int             gimp_image_get_layer_index        (GimpImage *, Layer *);
int             gimp_image_get_channel_index      (GimpImage *, Channel *);
Layer *         gimp_image_get_active_layer       (GimpImage *);
Channel *       gimp_image_get_active_channel     (GimpImage *);
Layer *         gimp_image_get_layer_by_tattoo    (GimpImage *, Tattoo);
Channel *       gimp_image_get_channel_by_tattoo  (GimpImage *, Tattoo);
Channel *       gimp_image_get_mask               (GimpImage *);
int             gimp_image_get_component_active   (GimpImage *, ChannelType);
int             gimp_image_get_component_visible  (GimpImage *, ChannelType);
int             gimp_image_layer_boundary         (GimpImage *, BoundSeg **, int *);
Layer *         gimp_image_set_active_layer       (GimpImage *, Layer *);
Channel *       gimp_image_set_active_channel     (GimpImage *, Channel *);
Channel *       gimp_image_unset_active_channel   (GimpImage *);
void            gimp_image_set_component_active   (GimpImage *, ChannelType, int);
void            gimp_image_set_component_visible  (GimpImage *, ChannelType, int);
Layer *         gimp_image_pick_correlate_layer   (GimpImage *, int, int);
void            gimp_image_set_layer_mask_apply   (GimpImage *, int);
void            gimp_image_set_layer_mask_edit    (GimpImage *, Layer *, int);
void            gimp_image_set_layer_mask_show    (GimpImage *, int);
Layer *         gimp_image_raise_layer            (GimpImage *, Layer *);
Layer *         gimp_image_lower_layer            (GimpImage *, Layer *);
Layer *         gimp_image_raise_layer_to_top     (GimpImage *, Layer *);
Layer *         gimp_image_lower_layer_to_bottom  (GimpImage *, Layer *);
Layer *         gimp_image_merge_visible_layers   (GimpImage *, MergeType);
Layer *         gimp_image_merge_down   (GimpImage *, Layer *, MergeType);
Layer *         gimp_image_flatten                (GimpImage *);
Layer *         gimp_image_merge_layers           (GimpImage *, GSList *, MergeType);
Layer *         gimp_image_add_layer              (GimpImage *, Layer *, int);
Layer *         gimp_image_remove_layer           (GimpImage *, Layer *);
LayerMask *     gimp_image_add_layer_mask         (GimpImage *, Layer *, LayerMask *);
Channel *       gimp_image_remove_layer_mask      (GimpImage *, Layer *, int);
Channel *       gimp_image_raise_channel          (GimpImage *, Channel *);
Channel *       gimp_image_lower_channel          (GimpImage *, Channel *);
Channel *       gimp_image_add_channel            (GimpImage *, Channel *, int);
Channel *       gimp_image_remove_channel         (GimpImage *, Channel *);
void            gimp_image_construct              (GimpImage *, int, int, int, int, gboolean);
void            gimp_image_invalidate_without_render (GimpImage *, int, int,
						      int, int, int,
						      int, int, int);
void            gimp_image_invalidate             (GimpImage *, int, int, int, int, int, int, int, int);
void            gimp_image_validate               (TileManager *, Tile *);

/*  Access functions  */

int             gimp_image_is_empty               (GimpImage *);
GimpDrawable *  gimp_image_active_drawable        (GimpImage *);
int             gimp_image_base_type              (GimpImage *);
int             gimp_image_base_type_with_alpha   (GimpImage *);
char *          gimp_image_filename               (GimpImage *);
int             gimp_image_enable_undo            (GimpImage *);
int             gimp_image_disable_undo           (GimpImage *);
int             gimp_image_dirty                  (GimpImage *);
int             gimp_image_clean                  (GimpImage *);
void            gimp_image_clean_all              (GimpImage *);
Layer *         gimp_image_floating_sel           (GimpImage *);
unsigned char * gimp_image_cmap                   (GimpImage *);


/*  projection access functions  */

TileManager *   gimp_image_projection             (GimpImage *);
int             gimp_image_projection_type        (GimpImage *);
int             gimp_image_projection_bytes       (GimpImage *);
int             gimp_image_projection_opacity     (GimpImage *);
void            gimp_image_projection_realloc     (GimpImage *);


/*  composite access functions  */

TileManager *   gimp_image_composite              (GimpImage *);
int             gimp_image_composite_type         (GimpImage *);
int             gimp_image_composite_bytes        (GimpImage *);
TempBuf *       gimp_image_composite_preview      (GimpImage *, ChannelType, int, int);
int             gimp_image_preview_valid          (GimpImage *, ChannelType);
void            gimp_image_invalidate_preview     (GimpImage *);

void            gimp_image_invalidate_previews    (void);
TempBuf *       gimp_image_construct_composite_preview (GimpImage *gimage, int width, int height);

#endif
