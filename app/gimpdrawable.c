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
#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "gimpdrawableP.h"
#include "gimpsignal.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "gimpparasite.h"
#include "parasitelist.h"
#include "undo.h"

#include "libgimp/gimpmath.h"
#include "libgimp/gimpparasite.h"

#include "libgimp/gimpintl.h"


enum
{
  INVALIDATE_PREVIEW,
  LAST_SIGNAL
};

static void gimp_drawable_class_init (GimpDrawableClass *klass);
static void gimp_drawable_init	     (GimpDrawable      *drawable);
static void gimp_drawable_destroy    (GtkObject		*object);

static guint gimp_drawable_signals[LAST_SIGNAL] = { 0 };

static GimpDrawableClass *parent_class = NULL;


GtkType
gimp_drawable_get_type (void)
{
  static GtkType type;
  GIMP_TYPE_INIT (type,
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
  GtkType type = GIMP_TYPE_DRAWABLE;

  object_class = GTK_OBJECT_CLASS (class);
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
static gint        global_drawable_ID  = 1;
static GHashTable *gimp_drawable_table = NULL;

/**************************/
/*  Function definitions  */

GimpDrawable*
gimp_drawable_get_ID (gint drawable_id)
{
  if (gimp_drawable_table == NULL)
    return NULL;

  return (GimpDrawable*) g_hash_table_lookup (gimp_drawable_table, 
					      (gpointer) drawable_id);
}

void
gimp_drawable_merge_shadow (GimpDrawable *drawable,
			    gint          undo)
{
  GImage *gimage;
  PixelRegion shadowPR;
  int x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_drawable_gimage (drawable);
  g_return_if_fail (gimage != NULL);
  g_return_if_fail (gimage->shadow != NULL);

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
gimp_drawable_fill (GimpDrawable *drawable,
		    guchar        r,
		    guchar        g,
		    guchar        b,
		    guchar        a)
{
  GImage *gimage;
  PixelRegion destPR;
  guchar c[MAX_CHANNELS];
  guchar i;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_drawable_gimage (drawable);
  g_return_if_fail (gimage != NULL);

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
		     gimp_drawable_width  (drawable),
		     gimp_drawable_height (drawable),
		     TRUE);
  color_region (&destPR, c);

}

gboolean
gimp_drawable_mask_bounds (GimpDrawable *drawable, 
			   gint         *x1,
			   gint         *y1,
			   gint         *x2,
			   gint         *y2)
{
  GimpImage *gimage;
  gint off_x, off_y;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  gimage = gimp_drawable_gimage (drawable);
  g_return_val_if_fail (gimage != NULL, FALSE);

  if (gimage_mask_bounds (gimage, x1, y1, x2, y2))
    {
      gimp_drawable_offsets (drawable, &off_x, &off_y);
      *x1 = CLAMP (*x1 - off_x, 0, gimp_drawable_width  (drawable));
      *y1 = CLAMP (*y1 - off_y, 0, gimp_drawable_height (drawable));
      *x2 = CLAMP (*x2 - off_x, 0, gimp_drawable_width  (drawable));
      *y2 = CLAMP (*y2 - off_y, 0, gimp_drawable_height (drawable));
      return TRUE;
    }
  else
    {
      *x2 = gimp_drawable_width  (drawable);
      *y2 = gimp_drawable_height (drawable);
      return FALSE;
    }
}

void
gimp_drawable_invalidate_preview (GimpDrawable *drawable,
				  gboolean      emit_signal)
{
  GimpImage *gimage;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  drawable->preview_valid = FALSE;

  if (emit_signal)
    gtk_signal_emit (GTK_OBJECT (drawable),
		     gimp_drawable_signals[INVALIDATE_PREVIEW]);

  gimage = gimp_drawable_gimage (drawable);
  if (gimage)
    {
      gimage->comp_preview_valid[0] = FALSE;
      gimage->comp_preview_valid[1] = FALSE;
      gimage->comp_preview_valid[2] = FALSE;
    }
}


GimpImage *
gimp_drawable_gimage (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->gimage;
}

void
gimp_drawable_set_gimage (GimpDrawable *drawable,
			  GimpImage    *gimage)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (gimage == NULL)
    drawable->tattoo = 0;
  else if (drawable->tattoo == 0 || drawable->gimage != gimage )
    drawable->tattoo = gimp_image_get_new_tattoo (gimage);

  drawable->gimage = gimage;
}

gboolean
gimp_drawable_has_alpha (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return drawable->has_alpha;
}

GimpImageType
gimp_drawable_type (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->type;
}

GimpImageType
gimp_drawable_type_with_alpha (GimpDrawable *drawable)
{
  GimpImageType type;
  gboolean has_alpha;
  
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  type = gimp_drawable_type (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);

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
      default:
	  g_assert_not_reached ();
	  break;
      }
  return 0;
}

gboolean
gimp_drawable_visible (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return drawable->visible;
}

gchar *
gimp_drawable_get_name (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->name;
}

void
gimp_drawable_set_name (GimpDrawable *drawable,
			gchar        *name)
{
  GSList *list, *listb, *base_list;
  GimpDrawable *drawableb;
  gint number = 1;
  gchar *newname;
  gchar *ext;
  gchar numberbuf[20];

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (name != NULL);

  if (drawable->name)
    {
      g_free (drawable->name);
      drawable->name = NULL;
    }

  if (drawable->gimage == 0 || drawable->gimage->layers == 0)
    {
      /* no other layers to check name against */
      drawable->name = g_strdup (name);
      return;
    }

  if (GIMP_IS_LAYER (drawable))
    base_list = drawable->gimage->layers;
  else if (GIMP_IS_CHANNEL (drawable))
    base_list = drawable->gimage->channels;
  else
    base_list = NULL;

  list = base_list;
  while (list)
    {
      drawableb = GIMP_DRAWABLE (list->data);
      if (drawable != drawableb &&
	  strcmp (name, gimp_drawable_get_name (drawableb)) == 0)
	{ /* names conflict */
	  newname = g_malloc (strlen (name) + 10); /* if this aint enough 
						      yer screwed */
	  strcpy (newname, name);
	  if ((ext = strrchr (newname, '#')))
	    {
	      number = atoi(ext+1);
	      /* Check if there really was the number we think after the # */
	      sprintf (numberbuf, "#%d", number);
	      if (strcmp (ext, numberbuf) != 0)
		{
		  /* No, so just ignore the # */
		  number = 1;
		  ext = &newname[strlen (newname)];
		}
	    }
	  else
	    {
	      number = 1;
	      ext = &newname[strlen (newname)];
	    }
	  sprintf (ext, "#%d", number+1);
	  listb = base_list;
	  while (listb) /* make sure the new name is unique */
	    {
	      drawableb = GIMP_DRAWABLE (listb->data);
	      if (drawable !=
		  drawableb && strcmp (newname,
				       gimp_drawable_get_name(drawableb)) == 0)
		{
		  number++;
		  sprintf (ext, "#%d", number+1);
		  /* Rescan from beginning */
		  listb = base_list;
		  continue;
		}
	      listb = listb->next;
	    }
	  drawable->name = g_strdup (newname);
	  g_free (newname);
	  return;
	}
      list = list->next;
    }
  drawable->name = g_strdup (name);
}

guchar *
gimp_drawable_get_color_at (GimpDrawable *drawable,
			    gint          x,
			    gint          y)
{
  Tile *tile;
  guchar *src;
  guchar *dest;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_drawable_gimage (drawable) ||
			!gimp_drawable_is_indexed (drawable), NULL);

  /* do not make this a g_return_if_fail() */
  if ( !(x >= 0 && x < drawable->width && y >= 0 && y < drawable->height))
    return NULL;

  dest = g_new (guchar, 5);

  tile = tile_manager_get_tile (gimp_drawable_data (drawable), x, y,
				TRUE, FALSE);
  src = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);

  gimp_image_get_color (gimp_drawable_gimage (drawable),
			gimp_drawable_type (drawable), dest, src);

  if (TYPE_HAS_ALPHA (gimp_drawable_type (drawable)))
    dest[3] = src[gimp_drawable_bytes (drawable) - 1];
  else
    dest[3] = 255;

  if (gimp_drawable_is_indexed (drawable))
    dest[4] = src[0];
  else
    dest[4] = 0;

  tile_release (tile, FALSE);

  return dest;
}

GimpParasite *
gimp_drawable_parasite_find (const GimpDrawable *drawable,
			     const gchar        *name)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return parasite_list_find (drawable->parasites, name);
}

static void
list_func (gchar          *key,
	   GimpParasite   *p,
	   gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (key);
}

gchar **
gimp_drawable_parasite_list (GimpDrawable *drawable,
			     gint         *count)
{
  gchar **list;
  gchar **cur;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  *count = parasite_list_length (drawable->parasites);
  cur = list = g_new (gchar *, *count);

  parasite_list_foreach (drawable->parasites, (GHFunc) list_func, &cur);
  
  return list;
}

void
gimp_drawable_parasite_attach (GimpDrawable *drawable,
			       GimpParasite *parasite)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  /* only set the dirty bit manually if we can be saved and the new
     parasite differs from the current one and we arn't undoable */
  if (gimp_parasite_is_undoable (parasite))
    {
      /* do a group in case we have attach_parent set */
      undo_push_group_start (drawable->gimage, PARASITE_ATTACH_UNDO);

      undo_push_drawable_parasite (drawable->gimage, drawable, parasite);
    }
  else if (gimp_parasite_is_persistent (parasite) &&
	   !gimp_parasite_compare (parasite,
				   gimp_drawable_parasite_find
				   (drawable, gimp_parasite_name (parasite))))
    undo_push_cantundo (drawable->gimage, _("parasite attach to drawable"));

  parasite_list_add (drawable->parasites, parasite);
  if (gimp_parasite_has_flag (parasite, GIMP_PARASITE_ATTACH_PARENT))
    {
      parasite_shift_parent (parasite);
      gimp_image_parasite_attach (drawable->gimage, parasite);
    }
  else if (gimp_parasite_has_flag (parasite, GIMP_PARASITE_ATTACH_GRANDPARENT))
    {
      parasite_shift_parent (parasite);
      parasite_shift_parent (parasite);
      gimp_parasite_attach (parasite);
    }
  if (gimp_parasite_is_undoable (parasite))
    {
      undo_push_group_end (drawable->gimage);
    }
}

void
gimp_drawable_parasite_detach (GimpDrawable *drawable,
			       const gchar  *parasite)
{
  GimpParasite *p;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (!(p = parasite_list_find (drawable->parasites, parasite)))
    return;

  if (gimp_parasite_is_undoable (p))
    undo_push_drawable_parasite_remove (drawable->gimage, drawable,
					gimp_parasite_name (p));
  else if (gimp_parasite_is_persistent (p))
    undo_push_cantundo (drawable->gimage, _("detach parasite from drawable"));

  parasite_list_remove (drawable->parasites, parasite);
}

Tattoo
gimp_drawable_get_tattoo (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), 0); 

  return drawable->tattoo;
}

void
gimp_drawable_set_tattoo (GimpDrawable *drawable, Tattoo val)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  drawable->tattoo = val;
}

gboolean
gimp_drawable_is_rgb (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if (gimp_drawable_type (drawable) == RGBA_GIMAGE ||
      gimp_drawable_type (drawable) == RGB_GIMAGE)
    return TRUE;
  else
    return FALSE;
}

gboolean
gimp_drawable_is_gray (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if (gimp_drawable_type (drawable) == GRAYA_GIMAGE ||
      gimp_drawable_type (drawable) == GRAY_GIMAGE)
    return TRUE;
  else
    return FALSE;
}

gboolean
gimp_drawable_is_indexed (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if (gimp_drawable_type (drawable) == INDEXEDA_GIMAGE ||
      gimp_drawable_type (drawable) == INDEXED_GIMAGE)
    return TRUE;
  else
    return FALSE;
}

TileManager *
gimp_drawable_data (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->tiles;
}

TileManager *
gimp_drawable_shadow (GimpDrawable *drawable)
{
  GImage *gimage;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return NULL;

  return gimage_shadow (gimage, drawable->width, drawable->height, 
			drawable->bytes);
}

int
gimp_drawable_bytes (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->bytes;
}

gint
gimp_drawable_width (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->width;
}

gint
gimp_drawable_height (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->height;
}

void
gimp_drawable_offsets (GimpDrawable *drawable,
		       gint         *off_x,
		       gint         *off_y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  *off_x = drawable->offset_x;
  *off_y = drawable->offset_y;
}

guchar *
gimp_drawable_cmap (GimpDrawable *drawable)
{
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  gimage = gimp_drawable_gimage (drawable);
  g_return_val_if_fail (gimage != NULL, NULL);

  return gimage->cmap;
}

void
gimp_drawable_deallocate (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (drawable->tiles)
    tile_manager_destroy (drawable->tiles);
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
  drawable->name          = NULL;
  drawable->tiles         = NULL;
  drawable->visible       = FALSE;
  drawable->width         = 0;
  drawable->height        = 0;
  drawable->offset_x      = 0;
  drawable->offset_y      = 0;
  drawable->bytes         = 0;
  drawable->ID            = global_drawable_ID++;
  drawable->tattoo        = 0;
  drawable->gimage        = NULL;
  drawable->type          = -1;
  drawable->has_alpha     = FALSE;
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;
  drawable->parasites     = parasite_list_new ();
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;

  if (gimp_drawable_table == NULL)
    gimp_drawable_table = g_hash_table_new (g_direct_hash, NULL);

  g_hash_table_insert (gimp_drawable_table,
		       (gpointer) drawable->ID,
		       (gpointer) drawable);
}

static void
gimp_drawable_destroy (GtkObject *object)
{
  GimpDrawable *drawable;

  g_return_if_fail (GIMP_IS_DRAWABLE (object));

  drawable = GIMP_DRAWABLE (object);

  g_hash_table_remove (gimp_drawable_table, (gpointer) drawable->ID);
  
  if (drawable->name)
    g_free (drawable->name);

  if (drawable->tiles)
    tile_manager_destroy (drawable->tiles);

  if (drawable->preview_cache)
    gimp_preview_cache_invalidate (&drawable->preview_cache);

  if (drawable->parasites)
    gtk_object_unref (GTK_OBJECT (drawable->parasites));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void
gimp_drawable_configure (GimpDrawable  *drawable,
			 GimpImage     *gimage,
			 gint           width,
			 gint           height, 
			 GimpImageType  type,
			 gchar         *name)
{
  gint bpp;
  gboolean alpha;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (!name)
    name = _("unnamed");

  switch (type)
    {
    case RGB_GIMAGE:
      bpp = 3; alpha = FALSE; break;
    case GRAY_GIMAGE:
      bpp = 1; alpha = FALSE; break;
    case RGBA_GIMAGE:
      bpp = 4; alpha = TRUE; break;
    case GRAYA_GIMAGE:
      bpp = 2; alpha = TRUE; break;
    case INDEXED_GIMAGE:
      bpp = 1; alpha = FALSE; break;
    case INDEXEDA_GIMAGE:
      bpp = 2; alpha = TRUE; break;
    default:
      g_message (_("Layer type %d not supported."), type);
      return;
    }

  drawable->name      = NULL;
  drawable->width     = width;
  drawable->height    = height;
  drawable->bytes     = bpp;
  drawable->type      = type;
  drawable->has_alpha = alpha;
  drawable->offset_x  = 0;
  drawable->offset_y  = 0;

  if (drawable->tiles)
    tile_manager_destroy (drawable->tiles);
  drawable->tiles = tile_manager_new (width, height, bpp);
  drawable->visible = TRUE;

  if (gimage)
    gimp_drawable_set_gimage (drawable, gimage);

  gimp_drawable_set_name (drawable, name);

  /*  preview variables  */
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;
}
