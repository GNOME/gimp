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
#include <math.h>
#include <strings.h>
#include "gimpdrawableP.h"
#include "gimpsignal.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimpparasite.h"
#include "parasitelist.h"

#include "libgimp/parasite.h"
#include "libgimp/gimpintl.h"

enum {
  INVALIDATE_PREVIEW,
  LAST_SIGNAL
};


static void gimp_drawable_class_init (GimpDrawableClass *klass);
static void gimp_drawable_init	     (GimpDrawable      *drawable);
static void gimp_drawable_destroy    (GtkObject		*object);

static guint gimp_drawable_signals[LAST_SIGNAL] = { 0 };

static GimpDrawableClass *parent_class = NULL;


GtkType
gimp_drawable_get_type ()
{
  static GtkType type;
  GIMP_TYPE_INIT(type,
		 GimpDrawable,
		 GimpDrawableClass,
		 gimp_drawable_init,
		 gimp_drawable_class_init,
		 GIMP_TYPE_OBJECT);
  return type;
}

static void
gimp_drawable_class_init (GimpDrawableClass *class)
{
  GtkObjectClass *object_class;
  GtkType type=GIMP_TYPE_DRAWABLE;
  object_class = GTK_OBJECT_CLASS(class);
  parent_class = gtk_type_class (GIMP_TYPE_OBJECT);
  
  gimp_drawable_signals[INVALIDATE_PREVIEW] =
	  gimp_signal_new ("invalidate_pr", GTK_RUN_LAST, type,
			   GTK_SIGNAL_OFFSET(GimpDrawableClass,
					     invalidate_preview),
			   gimp_sigtype_void);

  gtk_object_class_add_signals (object_class, gimp_drawable_signals, LAST_SIGNAL);

  object_class->destroy = gimp_drawable_destroy;
}


/*
 *  Static variables
 */
int global_drawable_ID = 1;
static GHashTable *gimp_drawable_table = NULL;

/**************************/
/*  Function definitions  */

GimpDrawable*
gimp_drawable_get_ID (int drawable_id)
{
  if (gimp_drawable_table == NULL)
    return NULL;

  return (GimpDrawable*) g_hash_table_lookup (gimp_drawable_table, 
					      (gpointer) drawable_id);
}


void
gimp_drawable_merge_shadow (GimpDrawable *drawable, int undo)
{
  GImage *gimage;
  PixelRegion shadowPR;
  int x1, y1, x2, y2;

  if (! drawable) 
    return;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return;

  if (! gimage->shadow)
    return;

  /*  A useful optimization here is to limit the update to the
   *  extents of the selection mask, as it cannot extend beyond
   *  them.
   */
  gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);
  pixel_region_init (&shadowPR, gimage->shadow, x1, y1,
		     (x2 - x1), (y2 - y1), FALSE);
  gimage_apply_image (gimage, drawable, &shadowPR, undo, OPAQUE_OPACITY,
		      REPLACE_MODE, NULL, x1, y1);
}


void
gimp_drawable_fill (GimpDrawable *drawable, guchar r, guchar g,
		    guchar b, guchar a)
{
  GImage *gimage;
  PixelRegion destPR;
  guchar c[MAX_CHANNELS];
  guchar i;
  

  g_assert(GIMP_IS_DRAWABLE(drawable));
  gimage=gimp_drawable_gimage(drawable);
  g_assert(gimage);

  switch (gimp_drawable_type (drawable))
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      c[RED_PIX] = r;
      c[GREEN_PIX] = g;
      c[BLUE_PIX] = b;
      if (gimp_drawable_type (drawable) == RGBA_GIMAGE)
	c[ALPHA_PIX] = a;
      break;
    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      c[GRAY_PIX] = r;
      if (gimp_drawable_type (drawable) == GRAYA_GIMAGE)
	c[ALPHA_G_PIX] = a;
      break;
    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      c[RED_PIX] = r;
      c[GREEN_PIX] = g;
      c[BLUE_PIX] = b;
      gimage_transform_color (gimage, drawable, c, &i, RGB);
      c[INDEXED_PIX] = i;
      if (gimp_drawable_type (drawable) == INDEXEDA_GIMAGE)
	  c[ALPHA_I_PIX] = a;
      break;
    default:
      g_message (_("Can't fill unknown image type."));
      break;
    }

  pixel_region_init (&destPR,
		     gimp_drawable_data (drawable),
		     0, 0,
		     gimp_drawable_width (drawable),
		     gimp_drawable_height (drawable),
		     TRUE);
  color_region (&destPR, c);

}


int
gimp_drawable_mask_bounds (GimpDrawable *drawable, 
		      int *x1, int *y1, int *x2, int *y2)
{
  GImage *gimage;
  int off_x, off_y;

  if (! drawable)
    return FALSE;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return FALSE;

  if (gimage_mask_bounds (gimage, x1, y1, x2, y2))
    {
      gimp_drawable_offsets (drawable, &off_x, &off_y);
      *x1 = CLAMP (*x1 - off_x, 0, gimp_drawable_width (drawable));
      *y1 = CLAMP (*y1 - off_y, 0, gimp_drawable_height (drawable));
      *x2 = CLAMP (*x2 - off_x, 0, gimp_drawable_width (drawable));
      *y2 = CLAMP (*y2 - off_y, 0, gimp_drawable_height (drawable));
      return TRUE;
    }
  else
    {
      *x2 = gimp_drawable_width (drawable);
      *y2 = gimp_drawable_height (drawable);
      return FALSE;
    }
}


void
gimp_drawable_invalidate_preview (GimpDrawable *drawable)
{
  GImage *gimage;

  if (! drawable)
    return;

  drawable->preview_valid = FALSE;
#if 0
  gtk_signal_emit (GTK_OBJECT(drawable), gimp_drawable_signals[INVALIDATE_PREVIEW]);
#endif
  gimage = gimp_drawable_gimage (drawable);
  if (gimage)
    {
      gimage->comp_preview_valid[0] = FALSE;
      gimage->comp_preview_valid[1] = FALSE;
      gimage->comp_preview_valid[2] = FALSE;
    }
}


int
gimp_drawable_dirty (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->dirty = (drawable->dirty < 0) ? 2 : drawable->dirty + 1;
  else
    return 0;
}


int
gimp_drawable_clean (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->dirty = (drawable->dirty <= 0) ? 0 : drawable->dirty - 1;
  else
    return 0;
}


GimpImage *
gimp_drawable_gimage (GimpDrawable *drawable)
{
  g_assert(GIMP_IS_DRAWABLE(drawable));
	
  return drawable->gimage;
}


void
gimp_drawable_set_gimage (GimpDrawable *drawable, GimpImage *gimage)
{
  g_assert(GIMP_IS_DRAWABLE(drawable));
  if (gimage == NULL)
    drawable->tattoo = 0;
  else if (drawable->tattoo == 0 || drawable->gimage != gimage )
    drawable->tattoo = gimp_image_get_new_tattoo(gimage);
  drawable->gimage = gimage;
}


int
gimp_drawable_type (GimpDrawable *drawable)
{
  if (drawable)
    return drawable->type;
  else
    return -1;
}

int
gimp_drawable_has_alpha (GimpDrawable *drawable)
{
  if (drawable)
    return drawable->has_alpha;
  else
    return FALSE;
}


int 
gimp_drawable_visible (GimpDrawable *drawable)
{
  return drawable->visible;
}

char *
gimp_drawable_get_name (GimpDrawable *drawable)
{
  return drawable->name;
}

void
gimp_drawable_set_name (GimpDrawable *drawable, char *name)
{
  GSList *list, *listb, *base_list;
  GimpDrawable *drawableb;
  int number = 1;
  char *newname;
  char *ext;
  char numberbuf[20];

  g_return_if_fail(GIMP_IS_DRAWABLE(drawable));
  if (drawable->name)
    g_free(drawable->name);
  drawable->name = NULL;
  if (drawable->gimage == 0 || drawable->gimage->layers == 0)
  {
    /* no other layers to check name against */
    drawable->name = g_strdup(name);
    return;
  }
  if (GIMP_IS_LAYER(drawable))
    base_list = drawable->gimage->layers;
  else if (GIMP_IS_CHANNEL(drawable))
    base_list = drawable->gimage->channels;
  else
    base_list = NULL;

  list = base_list;
  while (list)
  {
    drawableb = GIMP_DRAWABLE(list->data);
    if (drawable != drawableb &&
	strcmp(name, gimp_drawable_get_name(drawableb)) == 0)
    { /* names conflict */
      newname = g_malloc(strlen(name)+10); /* if this aint enough 
						 yer screwed */
      strcpy (newname, name);
      if ((ext = strrchr(newname, '#')))
      {
	number = atoi(ext+1);
	/* Check if there really was the number we think after the # */
	sprintf (numberbuf, "#%d", number);
	if (strcmp (ext, numberbuf) != 0)
	{
	  /* No, so just ignore the # */
	  number = 1;
	  ext = &newname[strlen(newname)];
	}
      }
      else
      {
	number = 1;
	ext = &newname[strlen(newname)];
      }
      sprintf(ext, "#%d", number+1);
      listb = base_list;
      while (listb) /* make sure the new name is unique */
      {
	drawableb = GIMP_DRAWABLE(listb->data);
	if (drawable != drawableb && strcmp(newname,
				      gimp_drawable_get_name(drawableb)) == 0)
	{
	  number++;
	  sprintf(ext, "#%d", number+1);
	  /* Rescan from beginning */
	  listb = base_list;
	  continue;
	}
	listb = listb->next;
      }
      drawable->name = g_strdup(newname);
      g_free(newname);
      return;
    }
    list = list->next;
  }
  drawable->name = g_strdup(name);
}


Parasite *
gimp_drawable_find_parasite (const GimpDrawable *drawable, const char *name)
{
  return parasite_list_find(drawable->parasites, name);
}

static void list_func(char *key, Parasite *p, char ***cur)
{
  *(*cur)++ = (char *) g_strdup (key);
}

char **
gimp_drawable_parasite_list (GimpDrawable *drawable, gint *count)
{
  char **list, **cur;

  *count = parasite_list_length (drawable->parasites);
  cur = list = (char **) g_malloc (sizeof (char *) * *count);

  parasite_list_foreach (drawable->parasites, (GHFunc)list_func, &cur);
  
  return list;
}

void
gimp_drawable_attach_parasite (GimpDrawable *drawable, Parasite *parasite)
{
  parasite_list_add(drawable->parasites, parasite);
  if (parasite_is_persistent(parasite)) /* make sure we can be saved */
    gimp_image_dirty(drawable->gimage);
  if (parasite_has_flag(parasite, PARASITE_ATTACH_PARENT))
  {
    parasite_shift_parent(parasite);
    gimp_image_attach_parasite(drawable->gimage, parasite);
  }
  else if (parasite_has_flag(parasite, PARASITE_ATTACH_GRANDPARENT))
  {
    parasite_shift_parent(parasite);
    parasite_shift_parent(parasite);
    gimp_attach_parasite(parasite);
  }
}

void
gimp_drawable_detach_parasite (GimpDrawable *drawable, const char *parasite)
{
  Parasite *p;
  if (!(p = parasite_list_find(drawable->parasites, parasite)))
    return;
  if (parasite_is_persistent(p))
    gimp_image_dirty(drawable->gimage);
  parasite_list_remove(drawable->parasites, parasite);
}

Tattoo
gimp_drawable_get_tattoo(const GimpDrawable *drawable)
{
  return drawable->tattoo;
}

int
gimp_drawable_type_with_alpha (GimpDrawable *drawable)
{
  int type = gimp_drawable_type (drawable);
  int has_alpha = gimp_drawable_has_alpha (drawable);

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
gimp_drawable_color (GimpDrawable *drawable)
{
  if (gimp_drawable_type (drawable) == RGBA_GIMAGE ||
      gimp_drawable_type (drawable) == RGB_GIMAGE)
    return 1;
  else
    return 0;
}


int
gimp_drawable_gray (GimpDrawable *drawable)
{
  if (gimp_drawable_type (drawable) == GRAYA_GIMAGE ||
      gimp_drawable_type (drawable) == GRAY_GIMAGE)
    return 1;
  else
    return 0;
}


int
gimp_drawable_indexed (GimpDrawable *drawable)
{
  if (gimp_drawable_type (drawable) == INDEXEDA_GIMAGE ||
      gimp_drawable_type (drawable) == INDEXED_GIMAGE)
    return 1;
  else
    return 0;
}


TileManager *
gimp_drawable_data (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->tiles;
  else
    return NULL;
}


TileManager *
gimp_drawable_shadow (GimpDrawable *drawable)
{
  GImage *gimage;

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return NULL;

  if (drawable) 
    return gimage_shadow (gimage, drawable->width, drawable->height, 
			  drawable->bytes);
  else
    return NULL;
}


int
gimp_drawable_bytes (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->bytes;
  else
    return -1;
}


int
gimp_drawable_width (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->width;
  else
    return -1;
}


int
gimp_drawable_height (GimpDrawable *drawable)
{
  if (drawable) 
    return drawable->height;
  else
    return -1;
}


void
gimp_drawable_offsets (GimpDrawable *drawable, int *off_x, int *off_y)
{
  if (drawable) 
    {
      *off_x = drawable->offset_x;
      *off_y = drawable->offset_y;
    }
  else
    *off_x = *off_y = 0;
}

unsigned char *
gimp_drawable_cmap (GimpDrawable *drawable)
{
  GImage *gimage;

  if ((gimage = gimp_drawable_gimage (drawable)))
    return gimage->cmap;
  else
    return NULL;
}


void
gimp_drawable_deallocate (GimpDrawable *drawable)
{
  if (drawable->tiles)
    tile_manager_destroy (drawable->tiles);
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
  drawable->name = NULL;
  drawable->tiles = NULL;
  drawable->visible = FALSE;
  drawable->width = drawable->height = 0;
  drawable->offset_x = drawable->offset_y = 0;
  drawable->bytes = 0;
  drawable->dirty = FALSE;
  drawable->gimage = NULL;
  drawable->type = -1;
  drawable->has_alpha = FALSE;
  drawable->preview = NULL;
  drawable->preview_valid = FALSE;
  drawable->parasites = parasite_list_new();
  drawable->tattoo = 0;
  gimp_matrix_identity(drawable->transform);

  drawable->ID = global_drawable_ID++;
  if (gimp_drawable_table == NULL)
    gimp_drawable_table = g_hash_table_new (g_direct_hash, NULL);
  g_hash_table_insert (gimp_drawable_table, (gpointer) drawable->ID,
		       (gpointer) drawable);
}

static void
gimp_drawable_destroy (GtkObject *object)
{
  GimpDrawable *drawable;
  g_return_if_fail (object != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (object));

  drawable = GIMP_DRAWABLE (object);

  g_hash_table_remove (gimp_drawable_table, (gpointer) drawable->ID);
  
  if (drawable->name)
    g_free (drawable->name);

  if (drawable->tiles)
    tile_manager_destroy (drawable->tiles);

  if (drawable->preview)
    temp_buf_free (drawable->preview);

  if (drawable->parasites)
    gtk_object_unref(GTK_OBJECT(drawable->parasites));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void
gimp_drawable_configure (GimpDrawable *drawable,
			 GimpImage* gimage, int width, int height, 
			 int type, char *name)
{
  int bpp;
  int alpha;

  if (!name)
    name = _("unnamed");

  switch (type)
    {
    case RGB_GIMAGE:
      bpp = 3; alpha = 0; break;
    case GRAY_GIMAGE:
      bpp = 1; alpha = 0; break;
    case RGBA_GIMAGE:
      bpp = 4; alpha = 1; break;
    case GRAYA_GIMAGE:
      bpp = 2; alpha = 1; break;
    case INDEXED_GIMAGE:
      bpp = 1; alpha = 0; break;
    case INDEXEDA_GIMAGE:
      bpp = 2; alpha = 1; break;
    default:
      g_message (_("Layer type %d not supported."), type);
      return;
    }

  drawable->name = NULL;
  drawable->width = width;
  drawable->height = height;
  drawable->bytes = bpp;
  drawable->type = type;
  drawable->has_alpha = alpha;
  drawable->offset_x = 0;
  drawable->offset_y = 0;

  if (drawable->tiles)
    tile_manager_destroy (drawable->tiles);
  drawable->tiles = tile_manager_new (width, height, bpp);
  drawable->dirty = FALSE;
  drawable->visible = TRUE;

  if (gimage)
    gimp_drawable_set_gimage(drawable, gimage);

  gimp_drawable_set_name(drawable, name);

  /*  preview variables  */
  drawable->preview = NULL;
  drawable->preview_valid = FALSE;
}
  
