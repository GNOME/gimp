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
#include <string.h>

#include "gimpimageP.h"
#include "cursorutil.h"
#include "drawable.h"
#include "floating_sel.h"
#include "gdisplay.h"
#include "general.h"
#include "gimage_mask.h"
#include "paint_funcs.h"
#include "palette.h"
#include "libgimp/parasite.h"
#include "parasitelist.h"
#include "undo.h"
#include "gimpsignal.h"
#include "gimpparasite.h"
#include "pathsP.h"
#include "gimprc.h"

#include "libgimp/gimpintl.h"

#include "tile_manager.h"
#include "tile.h"
#include "layer_pvt.h"
#include "drawable_pvt.h"		/* ick ick. */



/*  Local function declarations  */
static void     gimp_image_free_projection       (GimpImage *);
static void     gimp_image_allocate_shadow       (GimpImage *, int, int, int);
static void     gimp_image_allocate_projection   (GimpImage *);
static void     gimp_image_free_layers           (GimpImage *);
static void     gimp_image_free_channels         (GimpImage *);
static void     gimp_image_construct_layers      (GimpImage *, int, int, int, int);
static void     gimp_image_construct_channels    (GimpImage *, int, int, int, int);
static void     gimp_image_initialize_projection (GimpImage *, int, int, int, int);
static void     gimp_image_get_active_channels   (GimpImage *, GimpDrawable *, int *);

/*  projection functions  */
static void     project_intensity            (GimpImage *, Layer *, PixelRegion *,
					      PixelRegion *, PixelRegion *);
static void     project_intensity_alpha      (GimpImage *, Layer *, PixelRegion *,
					      PixelRegion *, PixelRegion *);
static void     project_indexed              (GimpImage *, Layer *, PixelRegion *,
					      PixelRegion *);
static void     project_channel              (GimpImage *, Channel *, PixelRegion *,
					      PixelRegion *);

/*
 *  Global variables
 */
int valid_combinations[][MAX_CHANNELS + 1] =
{
  /* RGB GIMAGE */
  { -1, -1, -1, COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A },
  /* RGBA GIMAGE */
  { -1, -1, -1, COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A },
  /* GRAY GIMAGE */
  { -1, COMBINE_INTEN_INTEN, COMBINE_INTEN_INTEN_A, -1, -1 },
  /* GRAYA GIMAGE */
  { -1, COMBINE_INTEN_A_INTEN, COMBINE_INTEN_A_INTEN_A, -1, -1 },
  /* INDEXED GIMAGE */
  { -1, COMBINE_INDEXED_INDEXED, COMBINE_INDEXED_INDEXED_A, -1, -1 },
  /* INDEXEDA GIMAGE */
  { -1, -1, COMBINE_INDEXED_A_INDEXED_A, -1, -1 },
};

guint32 next_guide_id = 1; /* For generating guide_ID handles for PDB stuff */


/*
 *  Static variables
 */

enum{
  DIRTY,
  REPAINT,
  RENAME,
  RESIZE,
  RESTRUCTURE,
  COLORMAP_CHANGED,
  LAST_SIGNAL
};
static void            gimp_image_destroy                 (GtkObject *);

static guint gimp_image_signals[LAST_SIGNAL];
static GimpObjectClass* parent_class;

static void
gimp_image_class_init (GimpImageClass *klass)
{
  GtkObjectClass *object_class;
  GtkType type;
  
  object_class = GTK_OBJECT_CLASS(klass);
  parent_class = gtk_type_class (gimp_object_get_type ());
  
  type=object_class->type;

  object_class->destroy =  gimp_image_destroy;

  gimp_image_signals[DIRTY] =
	  gimp_signal_new ("dirty", GTK_RUN_FIRST, type, 0,
			   gimp_sigtype_void);
  gimp_image_signals[REPAINT] =
	  gimp_signal_new ("repaint", GTK_RUN_FIRST, type, 0,
			   gimp_sigtype_int_int_int_int);
  gimp_image_signals[RENAME] =
	  gimp_signal_new ("rename", GTK_RUN_FIRST, type, 0,
			   gimp_sigtype_void);
  gimp_image_signals[RESIZE] =
	  gimp_signal_new ("resize", GTK_RUN_FIRST, type, 0,
			   gimp_sigtype_void);
  gimp_image_signals[RESTRUCTURE] =
	  gimp_signal_new ("restructure", GTK_RUN_FIRST, type, 0,
			   gimp_sigtype_void);
  gimp_image_signals[COLORMAP_CHANGED] =
	  gimp_signal_new ("colormap_changed", GTK_RUN_FIRST, type, 0,
			   gimp_sigtype_int);
  
  gtk_object_class_add_signals (object_class, gimp_image_signals, LAST_SIGNAL);
}


/* static functions */

static void 
gimp_image_init (GimpImage *gimage)
{
  gimage->has_filename = 0;
  gimage->num_cols = 0;
  gimage->cmap = NULL;
  /* ID and ref_count handled in gimage.c */
  gimage->instance_count = 0;
  gimage->shadow = NULL;
  gimage->dirty = 1;
  gimage->undo_on = TRUE;
  gimage->construct_flag = -1;
  gimage->tattoo_state = 0;
  gimage->projection = NULL;
  gimage->guides = NULL;
  gimage->layers = NULL;
  gimage->channels = NULL;
  gimage->layer_stack = NULL;
  gimage->undo_stack = NULL;
  gimage->redo_stack = NULL;
  gimage->undo_bytes = 0;
  gimage->undo_levels = 0;
  gimage->pushing_undo_group = 0;
  gimage->comp_preview_valid[0] = FALSE;
  gimage->comp_preview_valid[1] = FALSE;
  gimage->comp_preview_valid[2] = FALSE;
  gimage->comp_preview = NULL;
  gimage->parasites = parasite_list_new();
  gimage->xresolution = default_xresolution;
  gimage->yresolution = default_yresolution;
  gimage->unit = default_units;
  gimage->save_proc= NULL;
  gimage->paths = NULL;
}

GtkType 
gimp_image_get_type (void) 
{
  static GtkType type;

  GIMP_TYPE_INIT(type,
		 GimpImage,
		 GimpImageClass,
		 gimp_image_init,
		 gimp_image_class_init,
		 GIMP_TYPE_OBJECT);
  return type;
}


/* static functions */

static void
gimp_image_allocate_projection (GimpImage *gimage)
{
  if (gimage->projection)
    gimp_image_free_projection (gimage);

  /*  Find the number of bytes required for the projection.
   *  This includes the intensity channels and an alpha channel
   *  if one doesn't exist.
   */
  switch (gimp_image_base_type (gimage))
    {
    case RGB:
    case INDEXED:
      gimage->proj_bytes = 4;
      gimage->proj_type = RGBA_GIMAGE;
      break;
    case GRAY:
      gimage->proj_bytes = 2;
      gimage->proj_type = GRAYA_GIMAGE;
      break;
    default:
      g_assert_not_reached();
    }

  /*  allocate the new projection  */
  gimage->projection = tile_manager_new (gimage->width, gimage->height, gimage->proj_bytes);
  tile_manager_set_user_data (gimage->projection, (void *) gimage);
  tile_manager_set_validate_proc (gimage->projection, gimp_image_validate);
}

static void
gimp_image_free_projection (GimpImage *gimage)
{
  if (gimage->projection)
    tile_manager_destroy (gimage->projection);

  gimage->projection = NULL;
}

static void
gimp_image_allocate_shadow (GimpImage *gimage, 
			    int        width, 
			    int        height, 
			    int        bpp)
{
  /*  allocate the new projection  */
  gimage->shadow = tile_manager_new (width, height, bpp);
}


/* function definitions */

GimpImage *
gimp_image_new (int width, 
		int height, 
		int base_type)
{
  GimpImage *gimage=GIMP_IMAGE(gtk_type_new(gimp_image_get_type ()));
  int i;

  gimage->filename = NULL;
  gimage->width = width;
  gimage->height = height;

  gimage->base_type = base_type;

  switch (base_type)
    {
    case RGB:
    case GRAY:
      break;
    case INDEXED:
      /* always allocate 256 colors for the colormap */
      gimage->num_cols = 0;
      gimage->cmap = (unsigned char *) g_malloc0 (COLORMAP_SIZE);
      break;
    default:
      break;
    }

  /*  configure the active pointers  */
  gimage->active_layer = NULL;
  gimage->active_channel = NULL;  /* no default active channel */
  gimage->floating_sel = NULL;

  /*  set all color channels visible and active  */
  for (i = 0; i < MAX_CHANNELS; i++)
    {
      gimage->visible[i] = 1;
      gimage->active[i] = 1;
    }

  /* create the selection mask */
  gimage->selection_mask = channel_new_mask (gimage, gimage->width, gimage->height);


  return gimage;
}


void
gimp_image_set_filename (GimpImage *gimage, 
			 char      *filename)
{
  char *new_filename;

  /* 
   * WARNING: this function will free the current filename even if you are 
   * setting it to itself so any pointer you hold to the filename will be
   * invalid after this call.  So please use with care.
   */

  new_filename = g_strdup (filename);

  if (gimage->has_filename)
    g_free (gimage->filename);

  if (filename && filename[0])
    {
      gimage->filename = new_filename;
      gimage->has_filename = TRUE;
    }
  else
    {
      gimage->filename = NULL;
      gimage->has_filename = FALSE;
    }

  gtk_signal_emit (GTK_OBJECT (gimage), gimp_image_signals[RENAME]);
}


void
gimp_image_set_resolution (GimpImage *gimage,
			   double     xresolution,
			   double     yresolution)
{
  gimage->xresolution = xresolution;
  gimage->yresolution = yresolution;
}

void
gimp_image_get_resolution (GimpImage *gimage,
			   double    *xresolution,
			   double    *yresolution)
{
  g_return_if_fail(xresolution && yresolution);
  *xresolution = gimage->xresolution;
  *yresolution = gimage->yresolution;
}


void
gimp_image_set_unit (GimpImage *gimage,
		     GUnit      unit)
{
  gimage->unit = unit;
}

GUnit
gimp_image_get_unit (GimpImage *gimage)
{
  return gimage->unit;
}


void
gimp_image_set_save_proc (GimpImage     *gimage, 
			  PlugInProcDef *proc)
{
  gimage->save_proc = proc;
}

PlugInProcDef *
gimp_image_get_save_proc (GimpImage *gimage)
{
  return gimage->save_proc;
}

void
gimp_image_resize (GimpImage *gimage, 
		   int        new_width, 
		   int        new_height,
		   int        offset_x, 
		   int        offset_y)
{
  Channel *channel;
  Layer   *layer;
  Layer   *floating_layer;
  GSList  *list;
  GList   *guide_list;

  gimp_add_busy_cursors();

  g_assert (new_width > 0 && new_height > 0);

  /*  Get the floating layer if one exists  */
  floating_layer = gimp_image_floating_sel (gimage);

  undo_push_group_start (gimage, GIMAGE_MOD_UNDO);

  /*  Relax the floating selection  */
  if (floating_layer)
    floating_sel_relax (floating_layer, TRUE);

  /*  Push the image size to the stack  */
  undo_push_gimage_mod (gimage);

  /*  Set the new width and height  */
  gimage->width = new_width;
  gimage->height = new_height;

  /*  Resize all channels  */
  list = gimage->channels;
  while (list)
    {
      channel = (Channel *) list->data;
      channel_resize (channel, new_width, new_height, offset_x, offset_y);
      list = g_slist_next (list);

    }

  /*  Reposition or remove any guides  */
  guide_list = gimage->guides;
  while (guide_list)
    {
      Guide *guide;

      guide = (Guide*) guide_list->data;
      guide_list = g_list_next (guide_list);

      switch (guide->orientation)
	{
	case ORIENTATION_HORIZONTAL:
	  guide->position += offset_y;
	  if (guide->position < 0 || guide->position > new_height)
	    gimp_image_delete_guide (gimage, guide);
	  break;
	case ORIENTATION_VERTICAL:
	  guide->position += offset_x;
	  if (guide->position < 0 || guide->position > new_width)
	    gimp_image_delete_guide (gimage, guide);
	  break;
	default:
	  g_error("Unknown guide orientation I.\n");
	}
    }

  /*  Don't forget the selection mask!  */
  channel_resize (gimage->selection_mask, new_width, new_height, offset_x, offset_y);
  gimage_mask_invalidate (gimage);

  /*  Reposition all layers  */
  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;
      layer_translate (layer, offset_x, offset_y);
      list = g_slist_next (list);
    }

  /*  Make sure the projection matches the gimage size  */
  gimp_image_projection_realloc (gimage);

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  gtk_signal_emit (GTK_OBJECT (gimage), gimp_image_signals[RESIZE]);

  gimp_remove_busy_cursors(NULL);
}


void
gimp_image_scale (GimpImage *gimage, 
		  int        new_width, 
		  int        new_height)
{
  Channel *channel;
  Layer   *layer;
  Layer   *floating_layer;
  GSList  *list;
  GList   *glist;
  Guide   *guide;
  int old_width, old_height;
  int layer_width, layer_height;

  gimp_add_busy_cursors();

  /*  Get the floating layer if one exists  */
  floating_layer = gimp_image_floating_sel (gimage);

  undo_push_group_start (gimage, GIMAGE_MOD_UNDO);

  /*  Relax the floating selection  */
  if (floating_layer)
    floating_sel_relax (floating_layer, TRUE);

  /*  Push the image size to the stack  */
  undo_push_gimage_mod (gimage);

  /*  Set the new width and height  */
  old_width = gimage->width;
  old_height = gimage->height;
  gimage->width = new_width;
  gimage->height = new_height;

  /*  Scale all channels  */
  list = gimage->channels;
  while (list)
    {
      channel = (Channel *) list->data;
      channel_scale (channel, new_width, new_height);
      list = g_slist_next (list);
    }

  /*  Don't forget the selection mask!  */
  channel_scale (gimage->selection_mask, new_width, new_height);
  gimage_mask_invalidate (gimage);

  /*  Scale all layers  */
  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;

      layer_width = (new_width * drawable_width (GIMP_DRAWABLE(layer))) / old_width;
      layer_height = (new_height * drawable_height (GIMP_DRAWABLE(layer))) / old_height;
      layer_scale (layer, layer_width, layer_height, FALSE);
      list = g_slist_next (list);
    }

  /*  Scale any Guides  */
  glist = gimage->guides;
  while (glist)
    {
      guide = (Guide*) glist->data;
      glist = g_list_next (glist);

      switch (guide->orientation)
	{
	case ORIENTATION_HORIZONTAL:
	  guide->position = (guide->position * new_height) / old_height;
	  break;
	case ORIENTATION_VERTICAL:
	  guide->position = (guide->position * new_width) / old_width;
	  break;
	default:
	  g_error("Unknown guide orientation II.\n");
	}
    }

  /*  Make sure the projection matches the gimage size  */
  gimp_image_projection_realloc (gimage);

  /*  Rigor the floating selection  */
  if (floating_layer)
    floating_sel_rigor (floating_layer, TRUE);

  gtk_signal_emit (GTK_OBJECT (gimage), gimp_image_signals[RESIZE]);

  gimp_remove_busy_cursors(NULL);
}



TileManager *
gimp_image_shadow (GimpImage *gimage, 
		   int        width, 
		   int        height, 
		   int        bpp)
{
  if (gimage->shadow &&
      ((width != tile_manager_level_width (gimage->shadow)) ||
       (height != tile_manager_level_height (gimage->shadow)) ||
       (bpp != tile_manager_level_bpp (gimage->shadow))))
    gimp_image_free_shadow (gimage);
  else if (gimage->shadow)
    return gimage->shadow;

  gimp_image_allocate_shadow (gimage, width, height, bpp);

  return gimage->shadow;
}


void
gimp_image_free_shadow (GimpImage *gimage)
{
  /*  Free the shadow buffer from the specified gimage if it exists  */
  if (gimage->shadow)
    tile_manager_destroy (gimage->shadow);

  gimage->shadow = NULL;
}


static void
gimp_image_destroy (GtkObject *object)
{
  GimpImage* gimage = GIMP_IMAGE(object);

  gimp_image_free_projection (gimage);
  gimp_image_free_shadow (gimage);
  
  if (gimage->cmap)
    g_free (gimage->cmap);
  
  if (gimage->has_filename)
    g_free (gimage->filename);
  
  gimp_image_free_layers (gimage);
  gimp_image_free_channels (gimage);
  channel_delete (gimage->selection_mask);
  if (gimage->parasites)
    gtk_object_unref(GTK_OBJECT(gimage->parasites));
}

void
gimp_image_apply_image (GimpImage    *gimage, 
			GimpDrawable *drawable, 
			PixelRegion  *src2PR,
			int           undo, 
			int           opacity, 
			int           mode,
			/*  alternative to using drawable tiles as src1: */
			TileManager  *src1_tiles,
			int           x, 
			int           y)
{
  Channel *mask;
  int x1, y1, x2, y2;
  int offset_x, offset_y;
  PixelRegion src1PR, destPR, maskPR;
  int operation;
  int active [MAX_CHANNELS];

  /*  get the selection mask if one exists  */
  mask = (gimage_mask_is_empty (gimage)) ?
    NULL : gimp_image_get_mask (gimage);

  /*  configure the active channel array  */
  gimp_image_get_active_channels (gimage, drawable, active);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = valid_combinations [drawable_type (drawable)][src2PR->bytes];
  if (operation == -1)
    {
      g_message (_("gimp_image_apply_image sent illegal parameters"));
      return;
    }

  /*  get the layer offsets  */
  drawable_offsets (drawable, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within gimage bounds  */
  x1 = CLAMP (x, 0, drawable_width (drawable));
  y1 = CLAMP (y, 0, drawable_height (drawable));
  x2 = CLAMP (x + src2PR->w, 0, drawable_width (drawable));
  y2 = CLAMP (y + src2PR->h, 0, drawable_height (drawable));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y1 = CLAMP (y1, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
      x2 = CLAMP (x2, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y2 = CLAMP (y2, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (undo)
    undo_push_image (gimp_drawable_gimage(drawable), drawable, x1, y1, x2, y2);

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  if (src1_tiles)
    pixel_region_init (&src1PR, src1_tiles, x1, y1, (x2 - x1), (y2 - y1), FALSE);
  else
    pixel_region_init (&src1PR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR, src2PR->x + (x1 - x), src2PR->y + (y1 - y), (x2 - x1), (y2 - y1));

  if (mask)
    {
      int mx, my;

      /*  configure the mask pixel region
       *  don't use x1 and y1 because they are in layer
       *  coordinate system.  Need mask coordinate system
       */
      mx = x1 + offset_x;
      my = y1 + offset_y;

      pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(mask)), mx, my, (x2 - x1), (y2 - y1), FALSE);
      combine_regions (&src1PR, src2PR, &destPR, &maskPR, NULL,
		       opacity, mode, active, operation);
    }
  else
    combine_regions (&src1PR, src2PR, &destPR, NULL, NULL,
		     opacity, mode, active, operation);
}

/* Similar to gimp_image_apply_image but works in "replace" mode (i.e.
   transparent pixels in src2 make the result transparent rather
   than opaque.

   Takes an additional mask pixel region as well.

*/
void
gimp_image_replace_image (GimpImage    *gimage, 
			  GimpDrawable *drawable, 
			  PixelRegion  *src2PR,
			  int           undo, 
			  int           opacity,
			  PixelRegion  *maskPR,
			  int           x, 
			  int           y)
{
  Channel *mask;
  int x1, y1, x2, y2;
  int offset_x, offset_y;
  PixelRegion src1PR, destPR;
  PixelRegion mask2PR, tempPR;
  unsigned char *temp_data;
  int operation;
  int active [MAX_CHANNELS];

  /*  get the selection mask if one exists  */
  mask = (gimage_mask_is_empty (gimage)) ?
    NULL : gimp_image_get_mask (gimage);

  /*  configure the active channel array  */
  gimp_image_get_active_channels (gimage, drawable, active);

  /*  determine what sort of operation is being attempted and
   *  if it's actually legal...
   */
  operation = valid_combinations [drawable_type (drawable)][src2PR->bytes];
  if (operation == -1)
    {
      g_message (_("gimp_image_apply_image sent illegal parameters"));
      return;
    }

  /*  get the layer offsets  */
  drawable_offsets (drawable, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within gimage bounds  */
  x1 = CLAMP (x, 0, drawable_width (drawable));
  y1 = CLAMP (y, 0, drawable_height (drawable));
  x2 = CLAMP (x + src2PR->w, 0, drawable_width (drawable));
  y2 = CLAMP (y + src2PR->h, 0, drawable_height (drawable));

  if (mask)
    {
      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      x1 = CLAMP (x1, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y1 = CLAMP (y1, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
      x2 = CLAMP (x2, -offset_x, drawable_width (GIMP_DRAWABLE(mask)) - offset_x);
      y2 = CLAMP (y2, -offset_y, drawable_height (GIMP_DRAWABLE(mask)) - offset_y);
    }

  /*  If the calling procedure specified an undo step...  */
  if (undo)
    drawable_apply_image (drawable, x1, y1, x2, y2, NULL, FALSE);

  /* configure the pixel regions
   *  If an alternative to using the drawable's data as src1 was provided...
   */
  pixel_region_init (&src1PR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), FALSE);
  pixel_region_init (&destPR, drawable_data (drawable), x1, y1, (x2 - x1), (y2 - y1), TRUE);
  pixel_region_resize (src2PR, src2PR->x + (x1 - x), src2PR->y + (y1 - y), (x2 - x1), (y2 - y1));

  if (mask)
    {
      int mx, my;

      /*  configure the mask pixel region
       *  don't use x1 and y1 because they are in layer
       *  coordinate system.  Need mask coordinate system
       */
      mx = x1 + offset_x;
      my = y1 + offset_y;

      pixel_region_init (&mask2PR, drawable_data (GIMP_DRAWABLE(mask)), mx, my, (x2 - x1), (y2 - y1), FALSE);

      tempPR.bytes = 1;
      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.rowstride = mask2PR.rowstride;
      temp_data = g_malloc (tempPR.h * tempPR.rowstride);
      tempPR.data = temp_data;

      copy_region (&mask2PR, &tempPR);

      /* apparently, region operations can mutate some PR data. */
      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.data = temp_data;

      apply_mask_to_region (&tempPR, maskPR, OPAQUE_OPACITY);

      tempPR.x = 0;
      tempPR.y = 0;
      tempPR.w = x2 - x1;
      tempPR.h = y2 - y1;
      tempPR.data = temp_data;

      combine_regions_replace (&src1PR, src2PR, &destPR, &tempPR, NULL,
		       opacity, active, operation);

      g_free (temp_data);
    }
  else
    combine_regions_replace (&src1PR, src2PR, &destPR, maskPR, NULL,
		     opacity, active, operation);
}

/* Get rid of these! A "foreground" is an UI concept.. */

void
gimp_image_get_foreground (GimpImage     *gimage, 
			   GimpDrawable  *drawable, 
			   unsigned char *fg)
{
  unsigned char pfg[3];

  /*  Get the palette color  */
  palette_get_foreground (&pfg[0], &pfg[1], &pfg[2]);

  gimp_image_transform_color (gimage, drawable, pfg, fg, RGB);
}


void
gimp_image_get_background (GimpImage     *gimage, 
			   GimpDrawable  *drawable, 
			   unsigned char *bg)
{
  unsigned char pbg[3];

  /*  Get the palette color  */
  palette_get_background (&pbg[0], &pbg[1], &pbg[2]);

  gimp_image_transform_color (gimage, drawable, pbg, bg, RGB);
}

unsigned char *
gimp_image_get_color_at (GimpImage *gimage, 
			 int        x, 
			 int        y)
{
  Tile *tile;
  unsigned char *src;
  unsigned char *dest;

  if (x < 0 || y < 0 || x >= gimage->width || y >= gimage->height)
  {
    return NULL;
  }
  dest = g_new(unsigned char, 5);
  tile = tile_manager_get_tile (gimp_image_composite (gimage), x, y,
				TRUE, FALSE);
  src = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);
  gimp_image_get_color (gimage, gimp_image_composite_type (gimage), dest, src);
  if(TYPE_HAS_ALPHA(gimp_image_composite_type (gimage)))
    dest[3] = src[gimp_image_composite_bytes (gimage) - 1];
  else
    dest[3] = 255;
  dest[4] = 0;
  tile_release (tile, FALSE);
  return dest;
}

void
gimp_image_get_color (GimpImage     *gimage, 
		      int            d_type,
		      unsigned char *rgb, 
		      unsigned char *src)
{
  switch (d_type)
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      map_to_color (0, NULL, src, rgb);
      break;
    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      map_to_color (1, NULL, src, rgb);
      break;
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      map_to_color (2, gimage->cmap, src, rgb);
      break;
    }
}


void
gimp_image_transform_color (GimpImage     *gimage, 
			    GimpDrawable  *drawable,
			    unsigned char *src, 
			    unsigned char *dest, 
			    int            type)
{
#define INTENSITY(r,g,b) (r * 0.30 + g * 0.59 + b * 0.11 + 0.001)
  int d_type;

  d_type = (drawable != NULL) ? drawable_type (drawable) :
    gimp_image_base_type_with_alpha (gimage);

  switch (type)
    {
    case RGB:
      switch (d_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  /*  Straight copy  */
	  *dest++ = *src++;
	  *dest++ = *src++;
	  *dest++ = *src++;
	  break;
	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  /*  NTSC conversion  */
	  *dest = INTENSITY (src[RED_PIX],
			     src[GREEN_PIX],
			     src[BLUE_PIX]);
	  break;
	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  /*  Least squares method  */
	  *dest = map_rgb_to_indexed (gimage->cmap,
				      gimage->num_cols,
				      gimage,
				      src[RED_PIX],
				      src[GREEN_PIX],
				      src[BLUE_PIX]);
	  break;
	}
      break;
    case GRAY:
      switch (d_type)
	{
	case RGB_GIMAGE: case RGBA_GIMAGE:
	  /*  Gray to RG&B */
	  *dest++ = *src;
	  *dest++ = *src;
	  *dest++ = *src;
	  break;
	case GRAY_GIMAGE: case GRAYA_GIMAGE:
	  /*  Straight copy  */
	  *dest = *src;
	  break;
	case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
	  /*  Least squares method  */
	  *dest = map_rgb_to_indexed (gimage->cmap,
				      gimage->num_cols,
				      gimage,
				      src[GRAY_PIX],
				      src[GRAY_PIX],
				      src[GRAY_PIX]);
	  break;
	}
      break;
    default:
      break;
    }
}



Guide*
gimp_image_add_hguide (GimpImage *gimage)
{
  Guide *guide;

  guide = g_new (Guide, 1);
  guide->ref_count = 0;
  guide->position = -1;
  guide->guide_ID = next_guide_id++;
  guide->orientation = ORIENTATION_HORIZONTAL;

  gimage->guides = g_list_prepend (gimage->guides, guide);

  return guide;
}

Guide*
gimp_image_add_vguide (GimpImage *gimage)
{
  Guide *guide;

  guide = g_new (Guide, 1);
  guide->ref_count = 0;
  guide->position = -1;
  guide->guide_ID = next_guide_id++;
  guide->orientation = ORIENTATION_VERTICAL;

  gimage->guides = g_list_prepend (gimage->guides, guide);

  return guide;
}

void
gimp_image_add_guide (GimpImage *gimage,
		      Guide     *guide)
{
  gimage->guides = g_list_prepend (gimage->guides, guide);
}

void
gimp_image_remove_guide (GimpImage *gimage,
			 Guide     *guide)
{
  gimage->guides = g_list_remove (gimage->guides, guide);
}

void
gimp_image_delete_guide (GimpImage *gimage,
			 Guide     *guide) 
{
  gimage->guides = g_list_remove (gimage->guides, guide);
  g_free (guide);
}


Parasite *
gimp_image_find_parasite (const GimpImage *gimage, 
			  const char      *name)
{
  return parasite_list_find(gimage->parasites, name);
}

static void list_func (char       *key, 
		       Parasite   *p, 
		       char     ***cur)
{
  *(*cur)++ = (char *) g_strdup (key);
}

char **
gimp_image_parasite_list (GimpImage *image, 
			  gint      *count)
{
  char **list, **cur;

  *count = parasite_list_length (image->parasites);
  cur = list = (char **) g_malloc (sizeof (char *) * *count);

  parasite_list_foreach (image->parasites, (GHFunc)list_func, &cur);
  
  return list;
}

void
gimp_image_attach_parasite (GimpImage *gimage, 
			    Parasite  *parasite)
{
  /* only set the dirty bit manually if we can be saved and the new
     parasite differs from the current one and we aren't undoable */
  if (parasite_is_undoable(parasite))
    undo_push_image_parasite (gimage, parasite);
  if (parasite_is_persistent(parasite)
      && !parasite_compare(parasite, gimp_image_find_parasite(gimage,
						 parasite_name(parasite))))
    gimp_image_dirty(gimage);
  parasite_list_add(gimage->parasites, parasite);
  if (parasite_has_flag(parasite, PARASITE_ATTACH_PARENT))
  {
    parasite_shift_parent(parasite);
    gimp_attach_parasite(parasite);
  }
}

void
gimp_image_detach_parasite (GimpImage  *gimage, 
			    const char *parasite)
{
  Parasite *p;

  if (!(p = parasite_list_find(gimage->parasites, parasite)))
    return;
  if (parasite_is_undoable(p))
    undo_push_image_parasite_remove (gimage, parasite_name(p));
  else if (parasite_is_persistent(p))
    gimp_image_dirty(gimage);
  parasite_list_remove(gimage->parasites, parasite);
}

Tattoo
gimp_image_get_new_tattoo (GimpImage *image)
{
  image->tattoo_state++;
  if (image->tattoo_state <= 0)
    g_warning("Tattoo state has become corrupt (2.1 billion operation limit exceded)");
  return (image->tattoo_state);
}

void
gimp_image_colormap_changed (GimpImage *image, 
			     gint       col)
{
  g_return_if_fail (image);
  g_return_if_fail (col < image->num_cols);
  gtk_signal_emit (GTK_OBJECT(image),
			 gimp_image_signals[COLORMAP_CHANGED],
			 col);
}

void
gimp_image_set_paths (GimpImage *gimage,
		      PathsList *paths)
{
  gimage->paths = paths;
}

PathsList *
gimp_image_get_paths (GimpImage *gimage)
{
  return gimage->paths;
}
	
/************************************************************/
/*  Projection functions                                    */
/************************************************************/

static void
project_intensity (GimpImage   *gimage, 
		   Layer       *layer,
		   PixelRegion *src, 
		   PixelRegion *dest, 
		   PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, NULL, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INTENSITY);
  else
    combine_regions (dest, src, dest, mask, NULL, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INTEN);
}


static void
project_intensity_alpha (GimpImage   *gimage, 
			 Layer       *layer,
			 PixelRegion *src, 
			 PixelRegion *dest,
			 PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, NULL, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INTENSITY_ALPHA);
  else
    combine_regions (dest, src, dest, mask, NULL, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INTEN_A);
}


static void
project_indexed (GimpImage   *gimage, 
		 Layer       *layer,
		 PixelRegion *src, 
		 PixelRegion *dest)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, NULL, gimage->cmap, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INDEXED);
  else
    g_message (_("Unable to project indexed image."));
}


static void
project_indexed_alpha (GimpImage   *gimage, 
		       Layer       *layer,
		       PixelRegion *src, 
		       PixelRegion *dest,
		       PixelRegion *mask)
{
  if (! gimage->construct_flag)
    initial_region (src, dest, mask, gimage->cmap, layer->opacity,
		    layer->mode, gimage->visible, INITIAL_INDEXED_ALPHA);
  else
    combine_regions (dest, src, dest, mask, gimage->cmap, layer->opacity,
		     layer->mode, gimage->visible, COMBINE_INTEN_A_INDEXED_A);
}


static void
project_channel (GimpImage   *gimage, 
		 Channel     *channel,
		 PixelRegion *src, 
		 PixelRegion *src2)
{
  int type;

  if (! gimage->construct_flag)
    {
      type = (channel->show_masked) ?
	INITIAL_CHANNEL_MASK : INITIAL_CHANNEL_SELECTION;
      initial_region (src2, src, NULL, channel->col, channel->opacity,
		      NORMAL, NULL, type);
    }
  else
    {
      type = (channel->show_masked) ?
	COMBINE_INTEN_A_CHANNEL_MASK : COMBINE_INTEN_A_CHANNEL_SELECTION;
      combine_regions (src, src2, src, NULL, channel->col, channel->opacity,
		       NORMAL, NULL, type);
    }
}


/************************************************************/
/*  Layer/Channel functions                                 */
/************************************************************/


static void
gimp_image_free_layers (GimpImage *gimage)
{
  GSList *list = gimage->layers;
  Layer  *layer;

  while (list)
    {
      layer = (Layer *) list->data;
      layer_delete (layer);
      list = g_slist_next (list);
    }
  g_slist_free (gimage->layers);
  g_slist_free (gimage->layer_stack);
}


static void
gimp_image_free_channels (GimpImage *gimage)
{
  GSList  *list = gimage->channels;
  Channel *channel;

  while (list)
    {
      channel = (Channel *) list->data;
      channel_delete (channel);
      list = g_slist_next (list);
    }
  g_slist_free (gimage->channels);
}


static void
gimp_image_construct_layers (GimpImage *gimage, 
			     int        x, 
			     int        y, 
			     int        w, 
			     int        h)
{
  Layer * layer;
  int x1, y1, x2, y2;
  PixelRegion src1PR, src2PR, maskPR;
  PixelRegion * mask;
  GSList *list = gimage->layers;
  GSList *reverse_list = NULL;
  int off_x, off_y;

  /*  composite the floating selection if it exists  */
  if ((layer = gimp_image_floating_sel (gimage)))
    floating_sel_composite (layer, x, y, w, h, FALSE);

  if (!list)
    {
      /*      g_warning("g_i_c_l on layerless image."); */
    }

  /* Note added by Raph Levien, 27 Jan 1998

     This looks it was intended as an optimization, but it seems to
     have correctness problems. In particular, if all channels are
     turned off, the screen simply does not update the projected
     image. It should be black. Turning off this optimization seems to
     restore correct behavior. At some future point, it may be
     desirable to turn the optimization back on.

     */
#if 0
  /*  If all channels are not visible, simply return  */
  switch (gimp_image_base_type (gimage))
    {
    case RGB:
      if (! gimp_image_get_component_visible (gimage, RED_CHANNEL) &&
	  ! gimp_image_get_component_visible (gimage, GREEN_CHANNEL) &&
	  ! gimp_image_get_component_visible (gimage, BLUE_CHANNEL))
	return;
      break;
    case GRAY:
      if (! gimp_image_get_component_visible (gimage, GRAY_CHANNEL))
	return;
      break;
    case INDEXED:
      if (! gimp_image_get_component_visible (gimage, INDEXED_CHANNEL))
	return;
      break;
    }
#endif

  while (list)
    {
      layer = (Layer *) list->data;

      /*  only add layers that are visible and not floating selections to the list  */
      if (!layer_is_floating_sel (layer) && drawable_visible (GIMP_DRAWABLE(layer)))
	reverse_list = g_slist_prepend (reverse_list, layer);

      list = g_slist_next (list);
    }

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);

      x1 = CLAMP (off_x, x, x + w);
      y1 = CLAMP (off_y, y, y + h);
      x2 = CLAMP (off_x + drawable_width (GIMP_DRAWABLE(layer)), x, x + w);
      y2 = CLAMP (off_y + drawable_height (GIMP_DRAWABLE(layer)), y, y + h);

      /* configure the pixel regions  */
      pixel_region_init (&src1PR, gimp_image_projection (gimage), x1, y1, (x2 - x1), (y2 - y1), TRUE);

      /*  If we're showing the layer mask instead of the layer...  */
      if (layer->mask && layer->show_mask)
	{
	  pixel_region_init (&src2PR, drawable_data (GIMP_DRAWABLE(layer->mask)),
			     (x1 - off_x), (y1 - off_y),
			     (x2 - x1), (y2 - y1), FALSE);

	  copy_gray_to_region (&src2PR, &src1PR);
	}
      /*  Otherwise, normal  */
      else
	{
	  pixel_region_init (&src2PR, drawable_data (GIMP_DRAWABLE(layer)),
			     (x1 - off_x), (y1 - off_y),
			     (x2 - x1), (y2 - y1), FALSE);

	  if (layer->mask && layer->apply_mask)
	    {
	      pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(layer->mask)),
				 (x1 - off_x), (y1 - off_y),
				 (x2 - x1), (y2 - y1), FALSE);
	      mask = &maskPR;
	    }
	  else
	    mask = NULL;

	  /*  Based on the type of the layer, project the layer onto the
	   *  projection image...
	   */
	  switch (drawable_type (GIMP_DRAWABLE(layer)))
	    {
	    case RGB_GIMAGE: case GRAY_GIMAGE:
	      /* no mask possible */
	      project_intensity (gimage, layer, &src2PR, &src1PR, mask);
	      break;
	    case RGBA_GIMAGE: case GRAYA_GIMAGE:
	      project_intensity_alpha (gimage, layer, &src2PR, &src1PR, mask);
	      break;
	    case INDEXED_GIMAGE:
	      /* no mask possible */
	      project_indexed (gimage, layer, &src2PR, &src1PR);
	      break;
	    case INDEXEDA_GIMAGE:
	      project_indexed_alpha (gimage, layer, &src2PR, &src1PR, mask);
	      break;
	    default:
	      break;
	    }
	}
      gimage->construct_flag = 1;  /*  something was projected  */

      reverse_list = g_slist_next (reverse_list);
    }

  g_slist_free (reverse_list);
}


static void
gimp_image_construct_channels (GimpImage *gimage, 
			       int        x, 
			       int        y, 
			       int        w, 
			       int        h)
{
  Channel * channel;
  PixelRegion src1PR, src2PR;
  GSList *list = gimage->channels;
  GSList *reverse_list = NULL;

  if (!list)
    {
      /*      g_warning("g_i_c_c on channelless image."); */
    }

  /*  reverse the channel list  */
  while (list)
    {
      reverse_list = g_slist_prepend (reverse_list, list->data);
      list = g_slist_next (list);
    }

  while (reverse_list)
    {
      channel = (Channel *) reverse_list->data;

      if (drawable_visible (GIMP_DRAWABLE(channel)))
	{
	  /* configure the pixel regions  */
	  pixel_region_init (&src1PR, gimp_image_projection (gimage), x, y, w, h, TRUE);
	  pixel_region_init (&src2PR, drawable_data (GIMP_DRAWABLE(channel)), x, y, w, h, FALSE);

	  project_channel (gimage, channel, &src1PR, &src2PR);

	  gimage->construct_flag = 1;
	}

      reverse_list = g_slist_next (reverse_list);
    }

  g_slist_free (reverse_list);
}


static void
gimp_image_initialize_projection (GimpImage *gimage, 
				  int        x, 
				  int        y, 
				  int        w, 
				  int        h)
{
  GSList *list;
  Layer *layer;
  int coverage = 0;
  PixelRegion PR;
  unsigned char clear[4] = { 0, 0, 0, 0 };

  /*  this function determines whether a visible layer
   *  provides complete coverage over the image.  If not,
   *  the projection is initialized to transparent
   */
  list = gimage->layers;
  while (list)
    {
      int off_x, off_y;
      layer = (Layer *) list->data;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      if (drawable_visible (GIMP_DRAWABLE(layer)) &&
	  ! layer_has_alpha (layer) &&
	  (off_x <= x) &&
	  (off_y <= y) &&
	  (off_x + drawable_width (GIMP_DRAWABLE(layer)) >= x + w) &&
	  (off_y + drawable_height (GIMP_DRAWABLE(layer)) >= y + h))
	{
	  coverage = 1;
	  break;
	}

      list = g_slist_next (list);
    }

  if (!coverage)
    {
      pixel_region_init (&PR, gimp_image_projection (gimage), x, y, w, h, TRUE);
      color_region (&PR, clear);
    }
}


static void
gimp_image_get_active_channels (GimpImage    *gimage, 
				GimpDrawable *drawable, 
				int          *active)
{
  Layer * layer;
  int i;

  /*  first, blindly copy the gimage active channels  */
  for (i = 0; i < MAX_CHANNELS; i++)
    active[i] = gimage->active[i];

  /*  If the drawable is a channel (a saved selection, etc.)
   *  make sure that the alpha channel is not valid
   */
  if (GIMP_IS_CHANNEL (drawable))
    active[ALPHA_G_PIX] = 0;  /*  no alpha values in channels  */
  else
    {
      /*  otherwise, check whether preserve transparency is
       *  enabled in the layer and if the layer has alpha
       */
      if (GIMP_IS_LAYER (drawable)){
	      layer=GIMP_LAYER(drawable);
	      if (layer_has_alpha (layer) && layer->preserve_trans)
		      active[drawable_bytes (drawable) - 1] = 0;
      }
    }
}



void
gimp_image_construct (GimpImage *gimage, 
		      int        x, 
		      int        y, 
		      int        w, 
		      int        h,
		      gboolean   can_use_cowproject)
{
#if 0
      int xoff, yoff;
  
      /*  set the construct flag, used to determine if anything
       *  has been written to the gimage raw image yet.
       */
      gimage->construct_flag = 0;
      
      if (gimage->layers)
	{
	  gimp_drawable_offsets (GIMP_DRAWABLE((Layer*)(gimage->layers->data)),
				 &xoff, &yoff);
	}

      if (/*can_use_cowproject &&*/
      (gimage->layers) &&                         /* There's a layer.      */
      (!g_slist_next(gimage->layers)) &&          /* It's the only layer.  */
      (layer_has_alpha((Layer*)(gimage->layers->data))) && /* It's !flat.  */
                                                  /* It's visible.         */
      (drawable_visible (GIMP_DRAWABLE((Layer*)(gimage->layers->data)))) &&
      (drawable_width (GIMP_DRAWABLE((Layer*)(gimage->layers->data))) ==
       gimage->width) &&
      (drawable_height (GIMP_DRAWABLE((Layer*)(gimage->layers->data))) ==
       gimage->height) &&                         /* Covers all.           */
                                                  /* Not indexed.          */
      (!drawable_indexed (GIMP_DRAWABLE((Layer*)(gimage->layers->data)))) &&
      (((Layer*)(gimage->layers->data))->opacity == OPAQUE_OPACITY) /*opaq */
      )
    {
      int xoff, yoff;
      
      gimp_drawable_offsets (GIMP_DRAWABLE((Layer*)(gimage->layers->data)),
			     &xoff, &yoff);


      if ((xoff==0) && (yoff==0)) /* Starts at 0,0         */
      {
	PixelRegion srcPR, destPR;
	void * pr;
	
	g_warning("Can use cow-projection hack.  Yay!");

	pixel_region_init (&srcPR, gimp_drawable_data
			   (GIMP_DRAWABLE
			    ((Layer*)(gimage->layers->data))),
			   x, y, w,h, FALSE);
	pixel_region_init (&destPR,
			   gimp_image_projection (gimage),
			   x, y, w,h, TRUE);

	for (pr = pixel_regions_register (2, &srcPR, &destPR);
	     pr != NULL;
	     pr = pixel_regions_process (pr))
	  {
	    tile_manager_map_over_tile (destPR.tiles,
					destPR.curtile, srcPR.curtile);
	  }

	gimage->construct_flag = 1;
	gimp_image_construct_channels (gimage, x, y, w, h);
	return;
      }
    }
#else
  gimage->construct_flag = 0;
#endif
  
  /*  First, determine if the projection image needs to be
   *  initialized--this is the case when there are no visible
   *  layers that cover the entire canvas--either because layers
   *  are offset or only a floating selection is visible
   */
  gimp_image_initialize_projection (gimage, x, y, w, h);
  
  /*  call functions which process the list of layers and
   *  the list of channels
   */
  gimp_image_construct_layers (gimage, x, y, w, h);
  gimp_image_construct_channels (gimage, x, y, w, h);
}

void
gimp_image_invalidate_without_render (GimpImage *gimage, 
				      int  x, int  y, int  w, int  h,
				      int x1, int y1, int x2, int y2)
{
  Tile *tile;
  TileManager *tm;
  int i, j;

  tm = gimp_image_projection (gimage);

  /*  invalidate all tiles which are located outside of the displayed area
   *   all tiles inside the displayed area are constructed.
   */
  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
      {
	tile = tile_manager_get_tile (tm, j, i, FALSE, FALSE);

        /*  check if the tile is outside the bounds  */
        if ((MIN ((j + tile_ewidth(tile)), x2) - MAX (j, x1)) <= 0)
          {
            tile_invalidate_tile (&tile, tm, j, i);
          }
        else if (MIN ((i + tile_eheight(tile)), y2) - MAX (i, y1) <= 0)
          {
            tile_invalidate_tile (&tile, tm, j, i);
          }
      }
}

void
gimp_image_invalidate (GimpImage *gimage, 
		       int  x, int  y, int  w, int  h,
		       int x1, int y1, int x2, int y2)
{
  Tile *tile;
  TileManager *tm;
  int i, j;
  int startx, starty;
  int endx, endy;
  int tilex, tiley;

  tm = gimp_image_projection (gimage);

  startx = x;
  starty = y;
  endx = x + w;
  endy = y + h;

  /*  invalidate all tiles which are located outside of the displayed area
   *   all tiles inside the displayed area are constructed.
   */
  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT)))
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH)))
      {
	tile = tile_manager_get_tile (tm, j, i, FALSE, FALSE);

	/*  invalidate all lower level tiles  */
	/*tile_manager_invalidate_tiles (gimp_image_projection (gimage), tile);*/

        /*  check if the tile is outside the bounds  */
        if ((MIN ((j + tile_ewidth(tile)), x2) - MAX (j, x1)) <= 0)
          {
            tile_invalidate_tile (&tile, tm, j, i);
            if (j < x1)
              startx = MAX (startx, (j + tile_ewidth(tile)));
            else
              endx = MIN (endx, j);
          }
        else if (MIN ((i + tile_eheight(tile)), y2) - MAX (i, y1) <= 0)
          {
            tile_invalidate_tile (&tile, tm, j, i);
            if (i < y1)
              starty = MAX (starty, (i + tile_eheight(tile)));
            else
              endy = MIN (endy, i);
          }
        else
          {
            /*  If the tile is not valid, make sure we get the entire tile
             *   in the construction extents
             */
            if (tile_is_valid(tile) == FALSE)
              {
                tilex = j - (j % TILE_WIDTH);
                tiley = i - (i % TILE_HEIGHT);
                
                startx = MIN (startx, tilex);
                endx = MAX (endx, tilex + tile_ewidth(tile));
                starty = MIN (starty, tiley);
                endy = MAX (endy, tiley + tile_eheight(tile));
                
                tile_mark_valid (tile); /* hmmmmmmm..... */
              }
          }
      }

  if ((endx - startx) > 0 && (endy - starty) > 0)
    gimp_image_construct (gimage, startx, starty, (endx - startx), (endy - starty), TRUE);
}

void
gimp_image_validate (TileManager *tm, 
		     Tile        *tile)
{
  GimpImage *gimage;
  int x, y;
  int w, h;

  gimp_add_busy_cursors_until_idle();

  /*  Get the gimage from the tilemanager  */
  gimage = (GimpImage *) tile_manager_get_user_data (tm);

  /*  Find the coordinates of this tile  */
  tile_manager_get_tile_coordinates (tm, tile, &x, &y);
  w = tile_ewidth(tile);
  h = tile_eheight(tile);
  
  gimp_image_construct (gimage, x, y, w, h, FALSE);
}

int
gimp_image_get_layer_index (GimpImage *gimage, 
			    Layer     *layer_arg)
{
  Layer *layer;
  GSList *layers = gimage->layers;
  int index = 0;

  while (layers)
    {
      layer = (Layer *) layers->data;
      if (layer == layer_arg)
	return index;

      index++;
      layers = g_slist_next (layers);
    }

  return -1;
}

int
gimp_image_get_channel_index (GimpImage *gimage, 
			      Channel   *channel_ID)
{
  Channel *channel;
  GSList *channels = gimage->channels;
  int index = 0;

  while (channels)
    {
      channel = (Channel *) channels->data;
      if (channel == channel_ID)
	return index;

      index++;
      channels = g_slist_next (channels);
    }

  return -1;
}


Layer *
gimp_image_get_active_layer (GimpImage *gimage)
{
  return gimage->active_layer;
}


Channel *
gimp_image_get_active_channel (GimpImage *gimage)
{
  return gimage->active_channel;
}


Layer *
gimp_image_get_layer_by_tattoo (GimpImage *gimage, 
				Tattoo     tattoo)
{
  Layer *layer;
  GSList *layers = gimage->layers;

  while (layers)
  {
    layer = (Layer *) layers->data;
    if (layer_get_tattoo(layer) == tattoo)
      return layer;
    layers = g_slist_next (layers);
  }

  return NULL;
}

Channel *
gimp_image_get_channel_by_tattoo (GimpImage *gimage, 
				  Tattoo     tattoo)
{
  Channel *channel;
  GSList *channels = gimage->channels;

  while (channels)
  {
    channel = (Channel *) channels->data;
    if (channel_get_tattoo(channel) == tattoo)
      return channel;
    channels = g_slist_next (channels);
  }

  return NULL;
}


Channel *
gimp_image_get_channel_by_name (GimpImage *gimage, 
				char      *name)
{
  Channel *channel;
  GSList *channels = gimage->channels;

  while (channels)
  {
    channel = (Channel *) channels->data;
    if (! strcmp(channel_get_name(channel),name) )
      return channel;
    channels = g_slist_next (channels);
  }

  return NULL;
}
int
gimp_image_get_component_active (GimpImage   *gimage, 
				 ChannelType  type)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case RED_CHANNEL:     return gimage->active[RED_PIX]; break;
    case GREEN_CHANNEL:   return gimage->active[GREEN_PIX]; break;
    case BLUE_CHANNEL:    return gimage->active[BLUE_PIX]; break;
    case GRAY_CHANNEL:    return gimage->active[GRAY_PIX]; break;
    case INDEXED_CHANNEL: return gimage->active[INDEXED_PIX]; break;
    default:      return 0; break;
    }
}


int
gimp_image_get_component_visible (GimpImage   *gimage, 
				  ChannelType  type)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case RED_CHANNEL:     return gimage->visible[RED_PIX]; break;
    case GREEN_CHANNEL:   return gimage->visible[GREEN_PIX]; break;
    case BLUE_CHANNEL:    return gimage->visible[BLUE_PIX]; break;
    case GRAY_CHANNEL:    return gimage->visible[GRAY_PIX]; break;
    case INDEXED_CHANNEL: return gimage->visible[INDEXED_PIX]; break;
    default:      return 0; break;
    }
}


Channel *
gimp_image_get_mask (GimpImage *gimage)
{
  return gimage->selection_mask;
}


int
gimp_image_layer_boundary (GimpImage  *gimage, 
			   BoundSeg  **segs, 
			   int        *num_segs)
{
  Layer *layer;

  /*  The second boundary corresponds to the active layer's
   *  perimeter...
   */
  if ((layer = gimage->active_layer))
    {
      *segs = layer_boundary (layer, num_segs);
      return 1;
    }
  else
    {
      *segs = NULL;
      *num_segs = 0;
      return 0;
    }
}


Layer *
gimp_image_set_active_layer (GimpImage *gimage, 
			     Layer     *layer)
{

  /*  First, find the layer in the gimage
   *  If it isn't valid, find the first layer that is
   */
  if (gimp_image_get_layer_index (gimage, layer) == -1)
    {
      if (! gimage->layers)
	return NULL;
      layer = (Layer *) gimage->layers->data;
    }

  if (! layer)
    return NULL;

  /*  Configure the layer stack to reflect this change  */
  gimage->layer_stack = g_slist_remove (gimage->layer_stack, (void *) layer);
  gimage->layer_stack = g_slist_prepend (gimage->layer_stack, (void *) layer);

  /*  invalidate the selection boundary because of a layer modification  */
  layer_invalidate_boundary (layer);

  /*  Set the active layer  */
  gimage->active_layer = layer;
  gimage->active_channel = NULL;

  /*  return the layer  */
  return layer;
}


Channel *
gimp_image_set_active_channel (GimpImage *gimage, 
			       Channel   *channel)
{

  /*  Not if there is a floating selection  */
  if (gimp_image_floating_sel (gimage))
    return NULL;

  /*  First, find the channel
   *  If it doesn't exist, find the first channel that does
   */
  if (! channel) 
    {
      if (! gimage->channels)
	{
	  gimage->active_channel = NULL;
	  return NULL;
	}
      channel = (Channel *) gimage->channels->data;
    }

  /*  Set the active channel  */
  gimage->active_channel = channel;

  /*  return the channel  */
  return channel;
}


Channel *
gimp_image_unset_active_channel (GimpImage *gimage)
{
  Channel *channel;

  /*  make sure there is an active channel  */
  if (! (channel = gimage->active_channel))
    return NULL;

  /*  Set the active channel  */
  gimage->active_channel = NULL;

  return channel;
}


void
gimp_image_set_component_active (GimpImage   *gimage, 
				 ChannelType  type, 
				 int          value)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case RED_CHANNEL:     gimage->active[RED_PIX] = value; break;
    case GREEN_CHANNEL:   gimage->active[GREEN_PIX] = value; break;
    case BLUE_CHANNEL:    gimage->active[BLUE_PIX] = value; break;
    case GRAY_CHANNEL:    gimage->active[GRAY_PIX] = value; break;
    case INDEXED_CHANNEL: gimage->active[INDEXED_PIX] = value; break;
    case AUXILLARY_CHANNEL: break;
    }

  /*  If there is an active channel and we mess with the components,
   *  the active channel gets unset...
   */
  if (type != AUXILLARY_CHANNEL)
    gimp_image_unset_active_channel (gimage);
}


void
gimp_image_set_component_visible (GimpImage   *gimage, 
				  ChannelType  type, 
				  int          value)
{
  /*  No sanity checking here...  */
  switch (type)
    {
    case RED_CHANNEL:     gimage->visible[RED_PIX] = value; break;
    case GREEN_CHANNEL:   gimage->visible[GREEN_PIX] = value; break;
    case BLUE_CHANNEL:    gimage->visible[BLUE_PIX] = value; break;
    case GRAY_CHANNEL:    gimage->visible[GRAY_PIX] = value; break;
    case INDEXED_CHANNEL: gimage->visible[INDEXED_PIX] = value; break;
    default:      break;
    }
}


Layer *
gimp_image_pick_correlate_layer (GimpImage *gimage, 
				 int        x, 
				 int        y)
{
  Layer *layer;
  GSList *list;

  list = gimage->layers;
  while (list)
    {
      layer = (Layer *) list->data;
      if (layer_pick_correlate (layer, x, y))
	return layer;

      list = g_slist_next (list);
    }

  return NULL;
}


Layer *
gimp_image_raise_layer (GimpImage *gimage, 
			Layer     *layer_arg)
{
  Layer *layer;
  Layer *prev_layer;
  GSList *list;
  GSList *prev;
  int x1, y1, x2, y2;
  int index = -1;
  int off_x, off_y;
  int off2_x, off2_y;

  list = gimage->layers;
  prev = NULL; prev_layer = NULL;

  while (list)
    {
      layer = (Layer *) list->data;
      if (prev)
	prev_layer = (Layer *) prev->data;

      if (layer == layer_arg)
	{
	  /*  We can only raise a layer if it has an alpha channel &&
	   *  If it's not already the top layer
	   */
	  if (prev && layer_has_alpha (layer) && layer_has_alpha (prev_layer))
	    {
	      list->data = prev_layer;
	      prev->data = layer;
	      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
	      drawable_offsets (GIMP_DRAWABLE(prev_layer), &off2_x, &off2_y);

	      /*  calculate minimum area to update  */
	      x1 = MAX (off_x, off2_x);
	      y1 = MAX (off_y, off2_y);
	      x2 = MIN (off_x + drawable_width (GIMP_DRAWABLE(layer)),
			off2_x + drawable_width (GIMP_DRAWABLE(prev_layer)));
	      y2 = MIN (off_y + drawable_height (GIMP_DRAWABLE(layer)),
			off2_y + drawable_height (GIMP_DRAWABLE(prev_layer)));

	      if ((x2 - x1) > 0 && (y2 - y1) > 0)
		gtk_signal_emit(GTK_OBJECT(gimage),
				gimp_image_signals[REPAINT],
				x1, y1, x2-x1, y2-y1);

	      /*  invalidate the composite preview  */
	      gimp_image_invalidate_preview (gimage);

	      gimp_image_dirty(gimage);

	      return prev_layer;
	    }
	  else
	    {
	      g_message (_("Layer cannot be raised any further"));
	      return NULL;
	    }
	}

      prev = list;
      index++;
      list = g_slist_next (list);
    }

  return NULL;
}


Layer *
gimp_image_lower_layer (GimpImage *gimage, 
			Layer     *layer_arg)
{
  Layer *layer;
  Layer *next_layer;
  GSList *list;
  GSList *next;
  int x1, y1, x2, y2;
  int index = 0;
  int off_x, off_y;
  int off2_x, off2_y;

  next_layer = NULL;

  list = gimage->layers;

  while (list)
    {
      layer = (Layer *) list->data;
      next = g_slist_next (list);

      if (next)
	next_layer = (Layer *) next->data;
      index++;

      if (layer == layer_arg)
	{
	  /*  We can only lower a layer if it has an alpha channel &&
	   *  The layer beneath it has an alpha channel &&
	   *  If it's not already the bottom layer
	   */
	  if (next && layer_has_alpha (layer) && layer_has_alpha (next_layer))
	    {
	      list->data = next_layer;
	      next->data = layer;
	      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
	      drawable_offsets (GIMP_DRAWABLE(next_layer), &off2_x, &off2_y);
	      
	      /*  calculate minimum area to update  */
	      x1 = MAX (off_x, off2_x);
	      y1 = MAX (off_y, off2_y);
	      x2 = MIN (off_x + drawable_width (GIMP_DRAWABLE(layer)),
			off2_x + drawable_width (GIMP_DRAWABLE(next_layer)));
	      y2 = MIN (off_y + drawable_height (GIMP_DRAWABLE(layer)),
			off2_y + drawable_height (GIMP_DRAWABLE(next_layer)));

	      if ((x2 - x1) > 0 && (y2 - y1) > 0)
		gtk_signal_emit(GTK_OBJECT(gimage),
				gimp_image_signals[REPAINT],
				x1, y1, x2-x1, y2-y1);
	      
	      /*  invalidate the composite preview  */
	      gimp_image_invalidate_preview (gimage);

	      gimp_image_dirty(gimage);

	      return next_layer;
	    }
	  else
	    {
	      g_message (_("Layer cannot be lowered any further"));
	      return NULL;
	    }
	}

      list = next;
    }

  return NULL;
}


Layer *
gimp_image_raise_layer_to_top (GimpImage *gimage, 
			       Layer     *layer_arg)
{
  Layer *layer;
  GSList *list;
  int x_min, y_min, x_max, y_max;
  int off_x, off_y;

  list = gimage->layers;
  if (list == NULL)
    {
      /* the layers list is empty */
      return NULL;
    }
  layer = (Layer *) list->data;
  if (layer == layer_arg)
    {
      /* layer_arg is already the top_layer */
      g_message (_("Layer is already on top"));
      return NULL;
    }
  if (! layer_has_alpha (layer_arg))
    {
      g_message (_("Can't raise Layer without alpha"));
      return NULL;
    }
  
  list = g_slist_next (list);

  /* search for layer_arg */
  while (list)
    {
      layer = (Layer *) list->data;
      if (layer == layer_arg)
	{
	  break;
	}
      list = g_slist_next (list);
    }
  
  if (layer != layer_arg)
    {
      /* The requested layer was not found in the layerstack
       * Return without changing anything
       */
      return NULL;
    }
  
  list = g_slist_remove (gimage->layers, layer_arg);
  gimage->layers = g_slist_prepend (list, layer_arg);
  
  /* update the affected area (== area of layer_arg) */
  drawable_offsets (GIMP_DRAWABLE(layer_arg), &off_x, &off_y);
  x_min = off_x;
  y_min = off_y;
  x_max = off_x + drawable_width (GIMP_DRAWABLE(layer_arg));
  y_max = off_y + drawable_height (GIMP_DRAWABLE(layer_arg));
  gtk_signal_emit(GTK_OBJECT(gimage),
		  gimp_image_signals[REPAINT],
		  x_min, y_min, x_max, y_max);
  
  /*  invalidate the composite preview  */
  gimp_image_invalidate_preview (gimage);

  gimp_image_dirty (gimage);
  
  return layer;
}


Layer *
gimp_image_lower_layer_to_bottom (GimpImage *gimage, 
				  Layer     *layer_arg)
{
  Layer *layer;
  GSList *list;
  GSList *next;
  GSList *pos;
  int x_min, y_min, x_max, y_max;
  int off_x, off_y;
  int index;
  int ex_flag;
  
  list = gimage->layers;
  next = NULL; layer = NULL;
  ex_flag = 0;
  index = 0;
  
  /* 1. loop find layer_arg */
  while (list)
    {
      layer = (Layer *) list->data;
      if (layer == layer_arg)
	{
	  break;
	}
      list = g_slist_next (list);
      index++;
    }
  
  if (layer != layer_arg)
    {
      /* The requested layer was not found in the layerstack
       * Return without changing anything
       */
      return NULL;
    }
  pos = list;
  
  /* 2. loop: search for the bottom layer and check for alpha */
  while (list)
    {
      next = g_slist_next (list);
      if (next == NULL)
	{
	  if (layer == layer_arg)
	    {
	      /* there is no next layer below layer_arg */
	      g_message (_("Layer is already on bottom"));
	    }
	  /* bottom is reached, we can stop now */
	  break;
	}
      
      layer = (Layer *) next->data;
      if (layer_has_alpha (layer))
	{
	  ex_flag = 1;
	}
      else
	{
	  g_message (_("BG has no alpha, layer was placed above"));
	  break;
	}
      
      list = next;
      index++;
    }
  
  if (ex_flag == 0)
    {
      return NULL;
    }

  list = g_slist_remove (gimage->layers, layer_arg);
  gimage->layers = g_slist_insert (list, layer_arg, index);

  /* update the affected area (== area of layer_arg) */
  drawable_offsets (GIMP_DRAWABLE(layer_arg), &off_x, &off_y);
  x_min = off_x;
  y_min = off_y;
  x_max = off_x + drawable_width (GIMP_DRAWABLE(layer_arg));
  y_max = off_y + drawable_height (GIMP_DRAWABLE(layer_arg));
  gtk_signal_emit(GTK_OBJECT(gimage),
		  gimp_image_signals[REPAINT],
		  x_min, y_min, x_max, y_max);

  /*  invalidate the composite preview  */
  gimp_image_invalidate_preview (gimage);

  gimp_image_dirty (gimage);

  return layer_arg;
}


Layer *
gimp_image_position_layer (GimpImage *gimage, 
			   Layer     *layer_arg,
			   gint       new_index)
{
  Layer *layer;
  GSList *list;
  GSList *next;
  gint x_min, y_min, x_max, y_max;
  gint off_x, off_y;
  gint index;
  gint list_length;

  list = gimage->layers;
  list_length = g_slist_length (list);

  next = NULL;
  layer = NULL;
  index = 0;

  /* find layer_arg */
  while (list)
    {
      layer = (Layer *) list->data;
      if (layer == layer_arg)
	{
	  break;
	}
      list = g_slist_next (list);
      index++;
    }

  if (layer != layer_arg)
    {
      /* The requested layer was not found in the layerstack
       * Return without changing anything
       */
      return NULL;
    }

  if (new_index < 0)
    new_index = 0;

  if (new_index >= list_length)
    new_index = list_length - 1;

  if (new_index == index)
    return NULL;

  /* check if we want to move it below a bottom layer without alpha */
  layer = (Layer *) g_slist_last (list)->data;
  if (new_index == list_length - 1 &&
      !layer_has_alpha (layer))
    {
      g_message (_("BG has no alpha, layer was placed above"));
      new_index--;
    }

  list = g_slist_remove (gimage->layers, layer_arg);
  gimage->layers = g_slist_insert (list, layer_arg, new_index);

  /* update the affected area (== area of layer_arg) */
  drawable_offsets (GIMP_DRAWABLE(layer_arg), &off_x, &off_y);
  x_min = off_x;
  y_min = off_y;
  x_max = off_x + drawable_width (GIMP_DRAWABLE (layer_arg));
  y_max = off_y + drawable_height (GIMP_DRAWABLE (layer_arg));
  gtk_signal_emit (GTK_OBJECT (gimage),
		   gimp_image_signals[REPAINT],
		   x_min, y_min, x_max, y_max);

  /*  invalidate the composite preview  */
  gimp_image_invalidate_preview (gimage);

  gimp_image_dirty (gimage);

  return layer_arg;
}

Layer *
gimp_image_merge_visible_layers (GimpImage *gimage, 
				 MergeType  merge_type)
{
  GSList *layer_list;
  GSList *merge_list = NULL;
  Layer *layer;

  layer_list = gimage->layers;
  while (layer_list)
    {
      layer = (Layer *) layer_list->data;
      if (drawable_visible (GIMP_DRAWABLE(layer)))
	merge_list = g_slist_append (merge_list, layer);

      layer_list = g_slist_next (layer_list);
    }

  if (merge_list && merge_list->next)
    {
      gimp_add_busy_cursors();
      layer = gimp_image_merge_layers (gimage, merge_list, merge_type);
      g_slist_free (merge_list);
      gimp_remove_busy_cursors(NULL);
      return layer;
    }
  else
    {
      g_message (_("There are not enough visible layers for a merge.\nThere must be at least two."));
      g_slist_free (merge_list);
      return NULL;
    }
}


Layer *
gimp_image_flatten (GimpImage *gimage)
{
  GSList *layer_list;
  GSList *merge_list = NULL;
  Layer *layer;

  gimp_add_busy_cursors();

  layer_list = gimage->layers;
  while (layer_list)
    {
      layer = (Layer *) layer_list->data;
      if (drawable_visible (GIMP_DRAWABLE(layer)))
	merge_list = g_slist_append (merge_list, layer);

      layer_list = g_slist_next (layer_list);
    }

  layer = gimp_image_merge_layers (gimage, merge_list, FLATTEN_IMAGE);
  g_slist_free (merge_list);

  gimp_remove_busy_cursors(NULL);

  return layer;
}

Layer *
gimp_image_merge_down (GimpImage *gimage,
		       Layer     *current_layer,
		       MergeType  merge_type)
{
  GSList *layer_list;
  GSList *merge_list= NULL;
  Layer *layer = NULL;

  
  layer_list = gimage->layers;
  while (layer_list)
    {
      layer = (Layer *) layer_list->data;
      if (layer == current_layer)
	{
	  layer_list = g_slist_next (layer_list);
	  while (layer_list)
	    {
	      layer = (Layer *) layer_list->data;
	      if (drawable_visible (GIMP_DRAWABLE(layer)))
		{
		  merge_list = g_slist_append (merge_list, layer);
		  layer_list = NULL;
		}
	      else 
		layer_list = g_slist_next (layer_list);
	    }
	  merge_list = g_slist_prepend (merge_list, current_layer);
	}
      else
	layer_list = g_slist_next (layer_list);
    }
  
  if (merge_list && merge_list->next)
    {
      gimp_add_busy_cursors();
      layer = gimp_image_merge_layers (gimage, merge_list, merge_type);
      g_slist_free (merge_list);
      gimp_remove_busy_cursors(NULL);
      return layer;
    }
  else 
    {
      g_message (_("There are not enough visible layers for a merge down."));
      g_slist_free (merge_list);
      return NULL;
    }
}

Layer *
gimp_image_merge_layers (GimpImage *gimage, 
			 GSList    *merge_list, 
			 MergeType  merge_type)
{
  GSList *reverse_list = NULL;
  PixelRegion src1PR, src2PR, maskPR;
  PixelRegion * mask;
  Layer *merge_layer;
  Layer *layer;
  Layer *bottom;
  unsigned char bg[4] = {0, 0, 0, 0};
  int type;
  int count;
  int x1, y1, x2, y2;
  int x3, y3, x4, y4;
  int operation;
  int position;
  int active[MAX_CHANNELS] = {1, 1, 1, 1};
  int off_x, off_y;

  layer = NULL;
  type  = RGBA_GIMAGE;
  x1 = y1 = x2 = y2 = 0;
  bottom = NULL;

  /*  Get the layer extents  */
  count = 0;
  while (merge_list)
    {
      layer = (Layer *) merge_list->data;
      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);

      switch (merge_type)
	{
	case EXPAND_AS_NECESSARY:
	case CLIP_TO_IMAGE:
	  if (!count)
	    {
	      x1 = off_x;
	      y1 = off_y;
	      x2 = off_x + drawable_width (GIMP_DRAWABLE(layer));
	      y2 = off_y + drawable_height (GIMP_DRAWABLE(layer));
	    }
	  else
	    {
	      if (off_x < x1)
		x1 = off_x;
	      if (off_y < y1)
		y1 = off_y;
	      if ((off_x + drawable_width (GIMP_DRAWABLE(layer))) > x2)
		x2 = (off_x + drawable_width (GIMP_DRAWABLE(layer)));
	      if ((off_y + drawable_height (GIMP_DRAWABLE(layer))) > y2)
		y2 = (off_y + drawable_height (GIMP_DRAWABLE(layer)));
	    }
	  if (merge_type == CLIP_TO_IMAGE)
	    {
	      x1 = CLAMP (x1, 0, gimage->width);
	      y1 = CLAMP (y1, 0, gimage->height);
	      x2 = CLAMP (x2, 0, gimage->width);
	      y2 = CLAMP (y2, 0, gimage->height);
	    }
	  break;
	case CLIP_TO_BOTTOM_LAYER:
	  if (merge_list->next == NULL)
	    {
	      x1 = off_x;
	      y1 = off_y;
	      x2 = off_x + drawable_width (GIMP_DRAWABLE(layer));
	      y2 = off_y + drawable_height (GIMP_DRAWABLE(layer));
	    }
	  break;
	case FLATTEN_IMAGE:
	  if (merge_list->next == NULL)
	    {
	      x1 = 0;
	      y1 = 0;
	      x2 = gimage->width;
	      y2 = gimage->height;
	    }
	  break;
	}

      count ++;
      reverse_list = g_slist_prepend (reverse_list, layer);
      merge_list = g_slist_next (merge_list);
    }

  if ((x2 - x1) == 0 || (y2 - y1) == 0)
    return NULL;

  /*  Start a merge undo group  */
  undo_push_group_start (gimage, LAYER_MERGE_UNDO);

  if (merge_type == FLATTEN_IMAGE ||
      drawable_type (GIMP_DRAWABLE (layer)) == INDEXED_GIMAGE)
    {
      switch (gimp_image_base_type (gimage))
	{
	case RGB: type = RGB_GIMAGE; break;
	case GRAY: type = GRAY_GIMAGE; break;
	case INDEXED: type = INDEXED_GIMAGE; break;
	}
      merge_layer = layer_new (gimage, (x2 - x1), (y2 - y1),
			       type, drawable_get_name (GIMP_DRAWABLE(layer)), OPAQUE_OPACITY, NORMAL_MODE);

      if (!merge_layer) {
	g_message (_("gimp_image_merge_layers: could not allocate merge layer"));
	return NULL;
      }

      GIMP_DRAWABLE(merge_layer)->offset_x = x1;
      GIMP_DRAWABLE(merge_layer)->offset_y = y1;

      /*  get the background for compositing  */
      gimp_image_get_background (gimage, GIMP_DRAWABLE(merge_layer), bg);

      /*  init the pixel region  */
      pixel_region_init (&src1PR, drawable_data (GIMP_DRAWABLE(merge_layer)), 0, 0, gimage->width, gimage->height, TRUE);

      /*  set the region to the background color  */
      color_region (&src1PR, bg);

      position = 0;
    }
  else
    {
      /*  The final merged layer inherits the name of the bottom most layer
       *  and the resulting layer has an alpha channel
       *  whether or not the original did
       *  Opacity is set to 100% and the MODE is set to normal
       */
      merge_layer = layer_new (gimage, (x2 - x1), (y2 - y1),
			       drawable_type_with_alpha (GIMP_DRAWABLE(layer)),
			       drawable_get_name (GIMP_DRAWABLE(layer)),
			       OPAQUE_OPACITY, NORMAL_MODE);
	

      
      if (!merge_layer) {
	g_message (_("gimp_image_merge_layers: could not allocate merge layer"));
	return NULL;
      }

      GIMP_DRAWABLE(merge_layer)->offset_x = x1;
      GIMP_DRAWABLE(merge_layer)->offset_y = y1;

      /*  Set the layer to transparent  */
      pixel_region_init (&src1PR, drawable_data (GIMP_DRAWABLE(merge_layer)), 0, 0, (x2 - x1), (y2 - y1), TRUE);

      /*  set the region to 0's  */
      color_region (&src1PR, bg);

      /*  Find the index in the layer list of the bottom layer--we need this
       *  in order to add the final, merged layer to the layer list correctly
       */
      layer = (Layer *) reverse_list->data;
      position = g_slist_length (gimage->layers) - gimp_image_get_layer_index (gimage, layer);
      
      /* set the mode of the bottom layer to normal so that the contents
       *  aren't lost when merging with the all-alpha merge_layer
       *  Keep a pointer to it so that we can set the mode right after it's been
       *  merged so that undo works correctly.
       */
      layer -> mode =NORMAL;
      bottom = layer;

    }

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;

      /*  determine what sort of operation is being attempted and
       *  if it's actually legal...
       */
      operation = valid_combinations [drawable_type (GIMP_DRAWABLE(merge_layer))][drawable_bytes (GIMP_DRAWABLE(layer))];
      if (operation == -1)
	{
	  g_message (_("gimp_image_merge_layers attempting to merge incompatible layers\n"));
	  return NULL;
	}

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      x3 = CLAMP (off_x, x1, x2);
      y3 = CLAMP (off_y, y1, y2);
      x4 = CLAMP (off_x + drawable_width (GIMP_DRAWABLE(layer)), x1, x2);
      y4 = CLAMP (off_y + drawable_height (GIMP_DRAWABLE(layer)), y1, y2);

      /* configure the pixel regions  */
      pixel_region_init (&src1PR, drawable_data (GIMP_DRAWABLE(merge_layer)), (x3 - x1), (y3 - y1), (x4 - x3), (y4 - y3), TRUE);
      pixel_region_init (&src2PR, drawable_data (GIMP_DRAWABLE(layer)), (x3 - off_x), (y3 - off_y),
			 (x4 - x3), (y4 - y3), FALSE);

      if (layer->mask)
	{
	  pixel_region_init (&maskPR, drawable_data (GIMP_DRAWABLE(layer->mask)), (x3 - off_x), (y3 - off_y),
			     (x4 - x3), (y4 - y3), FALSE);
	  mask = &maskPR;
	}
      else
	mask = NULL;

      combine_regions (&src1PR, &src2PR, &src1PR, mask, NULL,
		       layer->opacity, layer->mode, active, operation);

      gimp_image_remove_layer (gimage, layer);
      reverse_list = g_slist_next (reverse_list);
    }

  /* Save old mode in undo */
  if (bottom)
    bottom -> mode = merge_layer -> mode;

  g_slist_free (reverse_list);

  /*  if the type is flatten, remove all the remaining layers  */
  if (merge_type == FLATTEN_IMAGE)
    {
      merge_list = gimage->layers;
      while (merge_list)
	{
	  layer = (Layer *) merge_list->data;
	  merge_list = g_slist_next (merge_list);
	  gimp_image_remove_layer (gimage, layer);
	}

      gimp_image_add_layer (gimage, merge_layer, position);
    }
  else
    {
      /*  Add the layer to the gimage  */
      gimp_image_add_layer (gimage, merge_layer, (g_slist_length (gimage->layers) - position + 1));
    }

  /*  End the merge undo group  */
  undo_push_group_end (gimage);

  /*  Update the gimage  */
  GIMP_DRAWABLE(merge_layer)->visible = TRUE;

  gtk_signal_emit(GTK_OBJECT(gimage), gimp_image_signals[RESTRUCTURE]);

  drawable_update (GIMP_DRAWABLE(merge_layer), 0, 0, drawable_width (GIMP_DRAWABLE(merge_layer)), drawable_height (GIMP_DRAWABLE(merge_layer)));

  /*reinit_layer_idlerender (gimage, merge_layer);*/

  return merge_layer;
}


Layer *
gimp_image_add_layer (GimpImage *gimage, 
		      Layer     *float_layer, 
		      int        position)
{
  LayerUndo * lu;

  if (GIMP_DRAWABLE(float_layer)->gimage != NULL && 
      GIMP_DRAWABLE(float_layer)->gimage != gimage) 
    {
      g_message (_("gimp_image_add_layer: attempt to add layer to wrong image"));
      return NULL;
    }

  {
    GSList *ll = gimage->layers;
    while (ll) 
      {
	if (ll->data == float_layer) 
	  {
	    g_message (_("gimp_image_add_layer: trying to add layer to image twice"));
	    return NULL;
	  }
	ll = g_slist_next(ll);
      }
  }  

  /*  Prepare a layer undo and push it  */
  lu = (LayerUndo *) g_malloc (sizeof (LayerUndo));
  lu->layer = float_layer;
  lu->prev_position = 0;
  lu->prev_layer = gimage->active_layer;
  lu->undo_type = 0;
  undo_push_layer (gimage, lu);

  /*  If the layer is a floating selection, set the ID  */
  if (layer_is_floating_sel (float_layer))
    gimage->floating_sel = float_layer;

  /*  let the layer know about the gimage  */
  gimp_drawable_set_gimage(GIMP_DRAWABLE(float_layer), gimage);
  
  /*  add the layer to the list at the specified position  */
  if (position == -1)
    position = gimp_image_get_layer_index (gimage, gimage->active_layer);
  if (position != -1)
    {
      /*  If there is a floating selection (and this isn't it!),
       *  make sure the insert position is greater than 0
       */
      if (gimp_image_floating_sel (gimage) && (gimage->floating_sel != float_layer) && position == 0)
	position = 1;
      gimage->layers = g_slist_insert (gimage->layers, layer_ref (float_layer), position);
    }
  else
    gimage->layers = g_slist_prepend (gimage->layers, layer_ref (float_layer));
  gimage->layer_stack = g_slist_prepend (gimage->layer_stack, float_layer);

  /*  notify the layers dialog of the currently active layer  */
  gimp_image_set_active_layer (gimage, float_layer);

  /*  update the new layer's area  */
  drawable_update (GIMP_DRAWABLE(float_layer), 0, 0, drawable_width (GIMP_DRAWABLE(float_layer)), drawable_height (GIMP_DRAWABLE(float_layer)));

  /*  invalidate the composite preview  */
  gimp_image_invalidate_preview (gimage);

  return float_layer;
}


Layer *
gimp_image_remove_layer (GimpImage *gimage, 
			 Layer     *layer)
{
  LayerUndo *lu;
  int off_x, off_y;

  if (layer)
    {
      /*  Prepare a layer undo--push it at the end  */
      lu = (LayerUndo *) g_malloc (sizeof (LayerUndo));
      lu->layer = layer;
      lu->prev_position = gimp_image_get_layer_index (gimage, layer);
      lu->prev_layer = layer;
      lu->undo_type = 1;

      gimage->layers = g_slist_remove (gimage->layers, layer);
      gimage->layer_stack = g_slist_remove (gimage->layer_stack, layer);

      /*  If this was the floating selection, reset the fs pointer  */
      if (gimage->floating_sel == layer)
	{
	  gimage->floating_sel = NULL;

	  floating_sel_reset (layer);
	}
      if (gimage->active_layer == layer)
	{
	  if (gimage->layers)
	    gimage->active_layer = (Layer *) gimage->layer_stack->data;
	  else
	    gimage->active_layer = NULL;
	}

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      gtk_signal_emit(GTK_OBJECT(gimage),
		      gimp_image_signals[REPAINT],
		      off_x, off_y,
		      drawable_width (GIMP_DRAWABLE(layer)),
		      drawable_height (GIMP_DRAWABLE(layer)));

      /* Send out REMOVED signal from layer */
      layer_removed (layer, gimage);

      /*  Push the layer undo--It is important it goes here since layer might
       *   be immediately destroyed if the undo push fails
       */
      undo_push_layer (gimage, lu);

      /*  invalidate the composite preview  */
      gimp_image_invalidate_preview (gimage);

      return NULL;
    }
  else
    return NULL;
}


LayerMask *
gimp_image_add_layer_mask (GimpImage *gimage, 
			   Layer     *layer, 
			   LayerMask *mask)
{
  LayerMaskUndo *lmu;

  if (layer->mask != NULL)
  {
    g_message(_("Unable to add a layer mask since\nthe layer already has one."));
  }
  if (drawable_indexed (GIMP_DRAWABLE(layer)))
  {
    g_message(_("Unable to add a layer mask to a\nlayer in an indexed image."));
  }
  if (! layer_has_alpha (layer))
  {
    g_message(_("Cannot add layer mask to a layer\nwith no alpha channel."));
  }
  if (drawable_width (GIMP_DRAWABLE(layer)) != drawable_width (GIMP_DRAWABLE(mask)) || drawable_height (GIMP_DRAWABLE(layer)) != drawable_height (GIMP_DRAWABLE(mask)))
  {
    g_message(_("Cannot add layer mask of different dimensions than specified layer."));
    return NULL;
  }

  layer_add_mask (layer, mask);

  /*  Prepare a layer undo and push it  */
  lmu = (LayerMaskUndo *) g_malloc (sizeof (LayerMaskUndo));
  lmu->layer = layer;
  lmu->mask = mask;
  lmu->undo_type = 0;
  lmu->apply_mask = layer->apply_mask;
  lmu->edit_mask = layer->edit_mask;
  lmu->show_mask = layer->show_mask;
  undo_push_layer_mask (gimage, lmu);

  
  return mask;
}


Channel *
gimp_image_remove_layer_mask (GimpImage *gimage, 
			      Layer     *layer, 
			      int        mode)
{
  LayerMaskUndo *lmu;
  int off_x, off_y;

  if (! (layer) )
    return NULL;
  if (! layer->mask)
    return NULL;

  /*  Start an undo group  */
  undo_push_group_start (gimage, LAYER_APPLY_MASK_UNDO);

  /*  Prepare a layer mask undo--push it below  */
  lmu = (LayerMaskUndo *) g_malloc (sizeof (LayerMaskUndo));
  lmu->layer = layer;
  lmu->mask = layer->mask;
  lmu->undo_type = 1;
  lmu->mode = mode;
  lmu->apply_mask = layer->apply_mask;
  lmu->edit_mask = layer->edit_mask;
  lmu->show_mask = layer->show_mask;

  layer_apply_mask (layer, mode);

  /*  Push the undo--Important to do it here, AFTER the call
   *   to layer_apply_mask, in case the undo push fails and the
   *   mask is delete : NULL)d
   */
  undo_push_layer_mask (gimage, lmu);

  /*  end the undo group  */
  undo_push_group_end (gimage);

  /*  If the layer mode is discard, update the layer--invalidate gimage also  */
  if (mode == DISCARD)
    {
      gimp_image_invalidate_preview (gimage);

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);
      gtk_signal_emit(GTK_OBJECT(gimage),
		      gimp_image_signals[REPAINT],
		      off_x, off_y,
		      drawable_width (GIMP_DRAWABLE(layer)),
		      drawable_height (GIMP_DRAWABLE(layer)));
    }

  gdisplays_flush ();
  return NULL;
}


Channel *
gimp_image_raise_channel (GimpImage *gimage, 
			  Channel   *channel_arg)
{
  Channel *channel;
  Channel *prev_channel;
  GSList *list;
  GSList *prev;
  int index = -1;

  list = gimage->channels;
  prev = NULL;
  prev_channel = NULL;

  while (list)
    {
      channel = (Channel *) list->data;
      if (prev)
	prev_channel = (Channel *) prev->data;

      if (channel == channel_arg)
	{
	  if (prev)
	    {
	      list->data = prev_channel;
	      prev->data = channel;
	      drawable_update (GIMP_DRAWABLE(channel), 0, 0, drawable_width (GIMP_DRAWABLE(channel)), drawable_height (GIMP_DRAWABLE(channel)));
	      return prev_channel;
	    }
	  else
	    {
	      g_message (_("Channel cannot be raised any further"));
	      return NULL;
	    }
	}

      prev = list;
      index++;
      list = g_slist_next (list);
    }

  return NULL;
}


Channel *
gimp_image_lower_channel (GimpImage *gimage, 
			  Channel   *channel_arg)
{
  Channel *channel;
  Channel *next_channel;
  GSList *list;
  GSList *next;
  int index = 0;

  list = gimage->channels;
  next_channel = NULL;

  while (list)
    {
      channel = (Channel *) list->data;
      next = g_slist_next (list);

      if (next)
	next_channel = (Channel *) next->data;
      index++;

      if (channel == channel_arg)
	{
	  if (next)
	    {
	      list->data = next_channel;
	      next->data = channel;
	      drawable_update (GIMP_DRAWABLE(channel), 0, 0, drawable_width (GIMP_DRAWABLE(channel)), drawable_height (GIMP_DRAWABLE(channel)));

	      return next_channel;
	    }
	  else
	    {
	      g_message (_("Channel cannot be lowered any further"));
	      return NULL;
	    }
	}

      list = next;
    }

  return NULL;
}


Channel *
gimp_image_add_channel (GimpImage *gimage, 
			Channel   *channel, 
			int        position)
{
  ChannelUndo * cu;

  if (GIMP_DRAWABLE(channel)->gimage != NULL &&
      GIMP_DRAWABLE(channel)->gimage != gimage)
    {
      g_message (_("gimp_image_add_channel: attempt to add channel to wrong image"));
      return NULL;
    }

  {
    GSList *cc = gimage->channels;
    while (cc) 
      {
	if (cc->data == channel) 
	  {
	    g_message (_("gimp_image_add_channel: trying to add channel to image twice"));
	    return NULL;
	  }
	cc = g_slist_next (cc);
      }
  }  


  /*  Prepare a channel undo and push it  */
  cu = (ChannelUndo *) g_malloc (sizeof (ChannelUndo));
  cu->channel = channel;
  cu->prev_position = 0;
  cu->prev_channel = gimage->active_channel;
  cu->undo_type = 0;
  undo_push_channel (gimage, cu);

  /*  add the channel to the list  */
  gimage->channels = g_slist_prepend (gimage->channels, channel_ref (channel));

  /*  notify this gimage of the currently active channel  */
  gimp_image_set_active_channel (gimage, channel);

  /*  if channel is visible, update the image  */
  if (drawable_visible (GIMP_DRAWABLE(channel)))
    drawable_update (GIMP_DRAWABLE(channel), 0, 0, drawable_width (GIMP_DRAWABLE(channel)), drawable_height (GIMP_DRAWABLE(channel)));

  return channel;
}


Channel *
gimp_image_remove_channel (GimpImage *gimage, 
			   Channel   *channel)
{
  ChannelUndo * cu;

  if (channel)
    {
      /*  Prepare a channel undo--push it below  */
      cu = (ChannelUndo *) g_malloc (sizeof (ChannelUndo));
      cu->channel = channel;
      cu->prev_position = gimp_image_get_channel_index (gimage, channel);
      cu->prev_channel = gimage->active_channel;
      cu->undo_type = 1;

      gimage->channels = g_slist_remove (gimage->channels, channel);

      if (gimage->active_channel == channel)
	{
	  if (gimage->channels)
	    gimage->active_channel = (((Channel *) gimage->channels->data));
	  else
	    gimage->active_channel = NULL;
	}

      if (drawable_visible (GIMP_DRAWABLE(channel)))
	drawable_update (GIMP_DRAWABLE(channel), 0, 0, drawable_width (GIMP_DRAWABLE(channel)), drawable_height (GIMP_DRAWABLE(channel)));

      /*  Important to push the undo here in case the push fails  */
      undo_push_channel (gimage, cu);

      return channel;
    }
  else
    return NULL;
}


/************************************************************/
/*  Access functions                                        */
/************************************************************/

int
gimp_image_is_empty (GimpImage *gimage)
{
  return (! gimage->layers);
}

GimpDrawable *
gimp_image_active_drawable (GimpImage *gimage)
{
  Layer *layer;

  /*  If there is an active channel (a saved selection, etc.),
   *  we ignore the active layer
   */
  if (gimage->active_channel != NULL)
    return GIMP_DRAWABLE (gimage->active_channel);
  else if (gimage->active_layer != NULL)
    {
      layer = gimage->active_layer;
      if (layer->mask && layer->edit_mask)
	return GIMP_DRAWABLE(layer->mask);
      else
	return GIMP_DRAWABLE(layer);
    }
  else
    return NULL;
}

int
gimp_image_base_type (GimpImage *gimage)
{
  return gimage->base_type;
}

int
gimp_image_base_type_with_alpha (GimpImage *gimage)
{
  switch (gimage->base_type)
    {
    case RGB:
      return RGBA_GIMAGE;
    case GRAY:
      return GRAYA_GIMAGE;
    case INDEXED:
      return INDEXEDA_GIMAGE;
    }
  return RGB_GIMAGE;
}

char *
gimp_image_filename (GimpImage *gimage)
{
  if (gimage->has_filename)
    return gimage->filename;
  else
    return _("Untitled");
}

int
gimp_image_enable_undo (GimpImage *gimage)
{
  /*  Free all undo steps as they are now invalidated  */
  undo_free (gimage);

  gimage->undo_on = TRUE;

  return TRUE;
}

int
gimp_image_disable_undo (GimpImage *gimage)
{
  gimage->undo_on = FALSE;
  return TRUE;
}

int
gimp_image_dirty (GimpImage *gimage)
{
/*  if (gimage->dirty < 0)
    gimage->dirty = 2;
  else */
    gimage->dirty ++;
  gtk_signal_emit(GTK_OBJECT(gimage), gimp_image_signals[DIRTY]);

  return gimage->dirty;
}

int
gimp_image_clean (GimpImage *gimage)
{
/*  if (gimage->dirty <= 0)
    gimage->dirty = 0;
  else */
    gimage->dirty --;
  return gimage->dirty;
}

void
gimp_image_clean_all (GimpImage *gimage)
{
  gimage->dirty = 0;
}

Layer *
gimp_image_floating_sel (GimpImage *gimage)
{
  if (gimage->floating_sel == NULL)
    return NULL;
  else return gimage->floating_sel;
}

unsigned char *
gimp_image_cmap (GimpImage *gimage)
{
  return drawable_cmap (gimp_image_active_drawable (gimage));
}


/************************************************************/
/*  Projection access functions                             */
/************************************************************/

TileManager *
gimp_image_projection (GimpImage *gimage)
{
  if ((gimage->projection == NULL) ||
      (tile_manager_level_width (gimage->projection) != gimage->width) ||
      (tile_manager_level_height (gimage->projection) != gimage->height))
    gimp_image_allocate_projection (gimage);
  
  return gimage->projection;
}

int
gimp_image_projection_type (GimpImage *gimage)
{
  return gimage->proj_type;
}

int
gimp_image_projection_bytes (GimpImage *gimage)
{
  return gimage->proj_bytes;
}

int
gimp_image_projection_opacity (GimpImage *gimage)
{
  return OPAQUE_OPACITY;
}

void
gimp_image_projection_realloc (GimpImage *gimage)
{
  gimp_image_allocate_projection (gimage);
}

/************************************************************/
/*  Composition access functions                            */
/************************************************************/

TileManager *
gimp_image_composite (GimpImage *gimage)
{
  return gimp_image_projection (gimage);
}

int
gimp_image_composite_type (GimpImage *gimage)
{
  return gimp_image_projection_type (gimage);
}

int
gimp_image_composite_bytes (GimpImage *gimage)
{
  return gimp_image_projection_bytes (gimage);
}

TempBuf *
gimp_image_construct_composite_preview (GimpImage *gimage, 
					int        width, 
					int        height)
{
  Layer * layer;
  PixelRegion src1PR, src2PR, maskPR;
  PixelRegion * mask;
  TempBuf *comp;
  TempBuf *layer_buf;
  TempBuf *mask_buf;
  GSList *list = gimage->layers;
  GSList *reverse_list = NULL;
  double ratio;
  int x, y, w, h;
  int x1, y1, x2, y2;
  int bytes;
  int construct_flag;
  int visible[MAX_CHANNELS] = {1, 1, 1, 1};
  int off_x, off_y;

  ratio = (double) width / (double) gimage->width;

  switch (gimp_image_base_type (gimage))
    {
    case RGB:
    case INDEXED:
      bytes = 4;
      break;
    case GRAY:
      bytes = 2;
      break;
    default:
      bytes = 0;
      break;
    }

  /*  The construction buffer  */
  comp = temp_buf_new (width, height, bytes, 0, 0, NULL);
  memset (temp_buf_data (comp), 0, comp->width * comp->height * comp->bytes);

  while (list)
    {
      layer = (Layer *) list->data;

      /*  only add layers that are visible and not floating selections to the list  */
      if (!layer_is_floating_sel (layer) && drawable_visible (GIMP_DRAWABLE(layer)))
	reverse_list = g_slist_prepend (reverse_list, layer);

      list = g_slist_next (list);
    }

  construct_flag = 0;

  while (reverse_list)
    {
      layer = (Layer *) reverse_list->data;

      drawable_offsets (GIMP_DRAWABLE(layer), &off_x, &off_y);

      x = (int) (ratio * off_x + 0.5);
      y = (int) (ratio * off_y + 0.5);
      w = (int) (ratio * drawable_width (GIMP_DRAWABLE(layer)) + 0.5);
      h = (int) (ratio * drawable_height (GIMP_DRAWABLE(layer)) + 0.5);

      x1 = CLAMP (x, 0, width);
      y1 = CLAMP (y, 0, height);
      x2 = CLAMP (x + w, 0, width);
      y2 = CLAMP (y + h, 0, height);

      src1PR.bytes = comp->bytes;
      src1PR.x = x1;  src1PR.y = y1;
      src1PR.w = (x2 - x1);
      src1PR.h = (y2 - y1);
      src1PR.rowstride = comp->width * src1PR.bytes;
      src1PR.data = temp_buf_data (comp) + y1 * src1PR.rowstride + x1 * src1PR.bytes;

      layer_buf = layer_preview (layer, w, h);
      src2PR.bytes = layer_buf->bytes;
      src2PR.w = src1PR.w;  src2PR.h = src1PR.h;
      src2PR.x = src1PR.x;  src2PR.y = src1PR.y;
      src2PR.rowstride = layer_buf->width * src2PR.bytes;
      src2PR.data = temp_buf_data (layer_buf) +
	(y1 - y) * src2PR.rowstride + (x1 - x) * src2PR.bytes;

      if (layer->mask && layer->apply_mask)
	{
	  mask_buf = layer_mask_preview (layer, w, h);
	  maskPR.bytes = mask_buf->bytes;
	  maskPR.rowstride = mask_buf->width;
	  maskPR.data = mask_buf_data (mask_buf) +
	    (y1 - y) * maskPR.rowstride + (x1 - x) * maskPR.bytes;
	  mask = &maskPR;
	}
      else
	mask = NULL;

      /*  Based on the type of the layer, project the layer onto the
       *   composite preview...
       *  Indexed images are actually already converted to RGB and RGBA,
       *   so just project them as if they were type "intensity"
       *  Send in all TRUE for visible since that info doesn't matter for previews
       */
      switch (drawable_type (GIMP_DRAWABLE(layer)))
	{
	case RGB_GIMAGE: case GRAY_GIMAGE: case INDEXED_GIMAGE:
	  if (! construct_flag)
	    initial_region (&src2PR, &src1PR, mask, NULL, layer->opacity,
			    layer->mode, visible, INITIAL_INTENSITY);
	  else
	    combine_regions (&src1PR, &src2PR, &src1PR, mask, NULL, layer->opacity,
			     layer->mode, visible, COMBINE_INTEN_A_INTEN);
	  break;

	case RGBA_GIMAGE: case GRAYA_GIMAGE: case INDEXEDA_GIMAGE:
	  if (! construct_flag)
	    initial_region (&src2PR, &src1PR, mask, NULL, layer->opacity,
			    layer->mode, visible, INITIAL_INTENSITY_ALPHA);
	  else
	    combine_regions (&src1PR, &src2PR, &src1PR, mask, NULL, layer->opacity,
			     layer->mode, visible, COMBINE_INTEN_A_INTEN_A);
	  break;

	default:
	  break;
	}

      construct_flag = 1;

      reverse_list = g_slist_next (reverse_list);
    }

  g_slist_free (reverse_list);

  return comp;
}

TempBuf *
gimp_image_composite_preview (GimpImage   *gimage, 
			      ChannelType  type,
			      int          width, 
			      int          height)
{
  int channel;

  switch (type)
    {
    case RED_CHANNEL:     channel = RED_PIX; break;
    case GREEN_CHANNEL:   channel = GREEN_PIX; break;
    case BLUE_CHANNEL:    channel = BLUE_PIX; break;
    case GRAY_CHANNEL:    channel = GRAY_PIX; break;
    case INDEXED_CHANNEL: channel = INDEXED_PIX; break;
    default: return NULL;
    }

  /*  The easy way  */
  if (gimage->comp_preview_valid[channel] &&
      gimage->comp_preview->width == width &&
      gimage->comp_preview->height == height)
    return gimage->comp_preview;
  /*  The hard way  */
  else
    {
      if (gimage->comp_preview)
	temp_buf_free (gimage->comp_preview);

      /*  Actually construct the composite preview from the layer previews!
       *  This might seem ridiculous, but it's actually the best way, given
       *  a number of unsavory alternatives.
       */
      gimage->comp_preview = gimp_image_construct_composite_preview (gimage, width, height);
      gimage->comp_preview_valid[channel] = TRUE;

      return gimage->comp_preview;
    }
}

int
gimp_image_preview_valid (GimpImage   *gimage, 
			  ChannelType  type)
{
  switch (type)
    {
    case RED_CHANNEL:     return gimage->comp_preview_valid[RED_PIX]; break;
    case GREEN_CHANNEL:   return gimage->comp_preview_valid[GREEN_PIX]; break;
    case BLUE_CHANNEL:    return gimage->comp_preview_valid[BLUE_PIX]; break;
    case GRAY_CHANNEL:    return gimage->comp_preview_valid[GRAY_PIX]; break;
    case INDEXED_CHANNEL: return gimage->comp_preview_valid[INDEXED_PIX]; break;
    default: return TRUE;
    }
}

void
gimp_image_invalidate_preview (GimpImage *gimage)
{
  Layer *layer;
  /*  Invalidate the floating sel if it exists  */
  if ((layer = gimp_image_floating_sel (gimage)))
    floating_sel_invalidate (layer);

  gimage->comp_preview_valid[0] = FALSE;
  gimage->comp_preview_valid[1] = FALSE;
  gimage->comp_preview_valid[2] = FALSE;
}

