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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "appenv.h"
#include "channel.h"
#include "drawable.h"
#include "errors.h"
#include "floating_sel.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gdisplay.h"
#include "layer.h"
#include "paint_funcs_area.h"
#include "palette.h"
#include "pixelarea.h"
#include "pixelrow.h"
#include "undo.h"

#include "drawable_pvt.h"
#include "canvas.h"
#include "shmbuf.h"
#include "tilebuf.h"


enum {
  INVALIDATE_PREVIEW,
  LAST_SIGNAL
};


static void gimp_drawable_class_init (GimpDrawableClass *klass);
static void gimp_drawable_init	     (GimpDrawable      *drawable);
static void gimp_drawable_destroy    (GtkObject		*object);

static gint drawable_signals[LAST_SIGNAL] = { 0 };

static GimpDrawableClass *parent_class = NULL;

Tag
drawable_tag (
              GimpDrawable *d
              )
{
  return d->tag;
}

guint
gimp_drawable_get_type ()
{
  static guint drawable_type = 0;

  if (!drawable_type)
    {
      GtkTypeInfo drawable_info =
      {
	"GimpDrawable",
	sizeof (GimpDrawable),
	sizeof (GimpDrawableClass),
	(GtkClassInitFunc) gimp_drawable_class_init,
	(GtkObjectInitFunc) gimp_drawable_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      drawable_type = gtk_type_unique (gtk_data_get_type (), &drawable_info);
    }

  return drawable_type;
}

static void
gimp_drawable_class_init (GimpDrawableClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) class;
  parent_class = gtk_type_class (gtk_data_get_type ());

  drawable_signals[INVALIDATE_PREVIEW] =
    gtk_signal_new ("invalidate_preview",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpDrawableClass, invalidate_preview),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, (guint *)drawable_signals, LAST_SIGNAL);

  object_class->destroy = gimp_drawable_destroy;
}


/*
 *  Static variables
 */
int global_drawable_ID = 1;
static GHashTable *drawable_table = NULL;

/**************************/
/*  Function definitions  */

static guint
drawable_hash (gpointer v)
{
  return (guint) v;
}

static gint
drawable_hash_compare (gpointer v1, gpointer v2)
{
  return ((guint) v1) == ((guint) v2);
}

GimpDrawable*
drawable_get_ID (int drawable_id)
{
  if (drawable_table == NULL)
    return NULL;

  return (GimpDrawable*) g_hash_table_lookup (drawable_table, 
					      (gpointer) drawable_id);
}

int
drawable_ID (GimpDrawable *drawable)
{
  return drawable->ID;
}

void
drawable_apply_image (GimpDrawable *drawable, 
		      int x1, int y1, int x2, int y2, 
                      Canvas * tiles)
{
  if (drawable)
    if (! tiles)
      undo_push_image (gimage_get_ID (drawable->gimage_ID), drawable, x1, y1, x2, y2);
    else
      undo_push_image_mod (gimage_get_ID (drawable->gimage_ID), drawable, x1, y1, x2, y2, tiles);
  
}


void
drawable_merge_shadow (GimpDrawable *drawable, int undo)
{
  GImage *gimage;
  PixelArea shadow_area;
  int x1, y1, x2, y2;

  if (! drawable)
    return;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  /*  A useful optimization here is to limit the update to the
   *  extents of the selection mask, as it cannot extend beyond
   *  them.
   */
  drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
  pixelarea_init (&shadow_area, gimage->shadow, 
                  x1, y1, (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_painthit (gimage, drawable, NULL, &shadow_area, undo, 1.0,
                         REPLACE_MODE,  x1, y1);
}


void
drawable_fill (GimpDrawable *drawable, int fill_type)
{
  GImage *gimage;

  if (! drawable)
    return;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  {
    PixelArea dest_area;
    COLOR16_NEW (paint, drawable_tag(drawable));

    /* init the fill color */
    COLOR16_INIT (paint);
    switch (fill_type)
      {
      case BACKGROUND_FILL:
        color16_background (&paint);
        break;
      case FOREGROUND_FILL:
        color16_foreground (&paint);
        break;
      case WHITE_FILL:
        color16_white (&paint);
        break;
      case TRANSPARENT_FILL:
        color16_transparent (&paint);
        break;
      case NO_FILL:
      default:
        return;
      }

    /* init the area to be filled */
    pixelarea_init (&dest_area, drawable_data(drawable),
                    0, 0, 0, 0, TRUE);

    /* do the fill */
    color_area (&dest_area, &paint);
  }
  
  /*  Update the filled area  */
  drawable_update (drawable, 0, 0,
		   drawable_width (drawable),
		   drawable_height (drawable));
}


void
drawable_update (GimpDrawable *drawable, int x, int y, int w, int h)
{
  GImage *gimage;
  int offset_x, offset_y;

  if (! drawable)
    return;

  if (! (gimage = drawable_gimage (drawable)))
    return;

  drawable_offsets (drawable, &offset_x, &offset_y);
  x += offset_x;
  y += offset_y;
  gdisplays_update_area (gimage->ID, x, y, w, h);

  /*  invalidate the preview  */
  drawable_invalidate_preview (drawable);
}


int
drawable_mask_bounds (GimpDrawable *drawable, 
		      int *x1, int *y1, int *x2, int *y2)
{
  GImage *gimage;
  int off_x, off_y;

  if (! drawable)
    return FALSE;

  if (! (gimage = drawable_gimage (drawable)))
    return FALSE;

  if (gimage_mask_bounds (gimage, x1, y1, x2, y2))
    {
      drawable_offsets (drawable, &off_x, &off_y);
      *x1 = BOUNDS (*x1 - off_x, 0, drawable_width (drawable));
      *y1 = BOUNDS (*y1 - off_y, 0, drawable_height (drawable));
      *x2 = BOUNDS (*x2 - off_x, 0, drawable_width (drawable));
      *y2 = BOUNDS (*y2 - off_y, 0, drawable_height (drawable));
      return TRUE;
    }
  else
    {
      *x2 = drawable_width (drawable);
      *y2 = drawable_height (drawable);
      return FALSE;
    }
}


void
drawable_invalidate_preview (GimpDrawable *drawable)
{
  GImage *gimage;

  if (! drawable)
    return;

  drawable->preview_valid = FALSE;

  gtk_signal_emit (GTK_OBJECT(drawable), drawable_signals[INVALIDATE_PREVIEW]);

  gimage = drawable_gimage (drawable);
  if (gimage)
    {
      gimage->comp_preview_valid[0] = FALSE;
      gimage->comp_preview_valid[1] = FALSE;
      gimage->comp_preview_valid[2] = FALSE;
    }
}


int
drawable_dirty (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->dirty = (drawable->dirty < 0) ? 2 : drawable->dirty + 1;
  else
    return 0;
}


int
drawable_clean (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->dirty = (drawable->dirty <= 0) ? 0 : drawable->dirty - 1;
  else
    return 0;
}


GImage *
drawable_gimage (GimpDrawable *drawable)
{
  if (drawable)
    return gimage_get_ID (drawable->gimage_ID);
  else
    return NULL;
}


int
drawable_type (GimpDrawable *drawable)
{
  if (drawable)
    return drawable->type;
  else
    return -1;
}

int
drawable_has_alpha (GimpDrawable *drawable)
{
  if (drawable)
    return drawable->has_alpha;
  else
    return FALSE;
}


int 
drawable_visible (GimpDrawable *drawable)
{
  return drawable->visible;
}

char *
drawable_name (GimpDrawable *drawable)
{
  return drawable->name;
}

int
drawable_type_with_alpha (GimpDrawable *drawable)
{
  int type = drawable_type (drawable);
  int has_alpha = drawable_has_alpha (drawable);

  if (has_alpha)
    return type;
  else
    switch (type)
      {
      case RGB_GIMAGE:
	return RGBA_GIMAGE; break;
      case GRAY_GIMAGE:
	return GRAYA_GIMAGE; break;
      case INDEXED_GIMAGE:
	return INDEXEDA_GIMAGE; break;
      }
  return 0;
}


int
drawable_color (GimpDrawable *drawable)
{
  if (drawable_type (drawable) == RGBA_GIMAGE ||
      drawable_type (drawable) == RGB_GIMAGE)
    return 1;
  else
    return 0;
}


int
drawable_gray (GimpDrawable *drawable)
{
  if (drawable_type (drawable) == GRAYA_GIMAGE ||
      drawable_type (drawable) == GRAY_GIMAGE)
    return 1;
  else
    return 0;
}


int
drawable_indexed (GimpDrawable *drawable)
{
  if (drawable_type (drawable) == INDEXEDA_GIMAGE ||
      drawable_type (drawable) == INDEXED_GIMAGE)
    return 1;
  else
    return 0;
}


Canvas *
drawable_data (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->tiles;
  else
    return NULL;
}

Canvas *
drawable_shadow (GimpDrawable *drawable)
{
  GImage *gimage;

  if (! (gimage = drawable_gimage (drawable)))
    return NULL;

  if (drawable) 
    return gimage_shadow (gimage, drawable->width, drawable->height, 
			  drawable->tag);
  else
    return NULL;
}

int
drawable_bytes (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->bytes;
  else
    return -1;
}


int
drawable_width (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->width;
  else
    return -1;
}


int
drawable_height (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->height;
  else
    return -1;
}


void
drawable_offsets (GimpDrawable *drawable, int *off_x, int *off_y)
{
  if (drawable) 
    {
      *off_x = drawable->offset_x;
      *off_y = drawable->offset_y;
    }
  else
    *off_x = *off_y = 0;
}

#define FIXME /* indexed color handling in general... */
unsigned char *
drawable_cmap (GimpDrawable *drawable)
{
  GImage *gimage;

  if ((gimage = drawable_gimage (drawable)))
    return gimage->cmap;
  else
    return NULL;
}


Layer *
drawable_layer (GimpDrawable *drawable)
{
  if (drawable && GIMP_IS_LAYER(drawable))
    return GIMP_LAYER (drawable);
  else
    return NULL;
}


LayerMask *
drawable_layer_mask (GimpDrawable *drawable)
{
  if (drawable && GIMP_IS_LAYER_MASK(drawable))
    return GIMP_LAYER_MASK(drawable);
  else
    return NULL;
}


Channel *
drawable_channel    (GimpDrawable *drawable)
{
  if (drawable && GIMP_IS_CHANNEL(drawable))
    return GIMP_CHANNEL(drawable);
  else
    return NULL;
}

void
drawable_deallocate (GimpDrawable *drawable)
{
  if (drawable->tiles)
    canvas_delete (drawable->tiles);
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
  drawable->name = NULL;
  drawable->tiles = NULL;
  drawable->tiles = NULL;
  drawable->visible = FALSE;
  drawable->width = drawable->height = 0;
  drawable->offset_x = drawable->offset_y = 0;
  drawable->bytes = 0;
  drawable->dirty = FALSE;
  drawable->gimage_ID = -1;
  drawable->type = -1;
  drawable->has_alpha = FALSE;
  drawable->preview = NULL;
  drawable->preview_valid = FALSE;

  drawable->ID = global_drawable_ID++;
  if (drawable_table == NULL)
    drawable_table = g_hash_table_new (drawable_hash,
				       drawable_hash_compare);
  g_hash_table_insert (drawable_table, (gpointer) drawable->ID,
		       (gpointer) drawable);
}

static void
gimp_drawable_destroy (GtkObject *object)
{
  GimpDrawable *drawable;
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (object));

  drawable = GIMP_DRAWABLE (object);

  g_hash_table_remove (drawable_table, (gpointer) drawable->ID);
  
  if (drawable->name)
    g_free (drawable->name);

  if (drawable->tiles)
    canvas_delete (drawable->tiles);

  if (drawable->preview)
    canvas_delete (drawable->preview);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

  
void 
gimp_drawable_configure  (
                          GimpDrawable * drawable,
                          int gimage_ID,
                          int width,
                          int height,
                          Tag tag,
                          Storage storage,
                          char * name
                          )
{
  if (!name)
    name = "unnamed";
  
  drawable->tag = tag;
  
  if (drawable->name) 
    g_free (drawable->name);
  drawable->name = g_strdup(name);
  drawable->width = width;
  drawable->height = height;
  drawable->bytes = tag_bytes (drawable->tag);

  switch (tag_format (tag))
    {
    case FORMAT_RGB:
      drawable->type = (tag_alpha (tag) == ALPHA_YES) ? RGBA_GIMAGE: RGB_GIMAGE;
      break;
    case FORMAT_GRAY:
      drawable->type = (tag_alpha (tag) == ALPHA_YES) ? GRAYA_GIMAGE: GRAY_GIMAGE;
      break;
    case FORMAT_INDEXED:
      drawable->type = (tag_alpha (tag) == ALPHA_YES) ? INDEXEDA_GIMAGE: INDEXED_GIMAGE;
      break;
    default:
      break;
    }
  drawable->has_alpha = (tag_alpha (drawable->tag) == ALPHA_YES) ? TRUE : FALSE;
  drawable->offset_x = 0;
  drawable->offset_y = 0;

  if (drawable->tiles)
    canvas_delete (drawable->tiles);
  drawable->tiles = canvas_new (drawable->tag, width, height, storage);

  drawable->dirty = FALSE;
  drawable->visible = TRUE;

  drawable->gimage_ID = gimage_ID;

  /*  preview variables  */
  drawable->preview = NULL;
  drawable->preview_valid = FALSE;
}
  
