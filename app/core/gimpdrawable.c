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
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "apptypes.h"

#include "paint-funcs/paint-funcs.h"

#include "gimage_mask.h"
#include "gimpchannel.h"
#include "gimpdrawable.h"
#include "gimpdrawable-preview.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplist.h"
#include "gimppreviewcache.h"
#include "gimpparasite.h"
#include "parasitelist.h"
#include "pixel_region.h"
#include "tile.h"
#include "tile_manager.h"
#include "undo.h"

#include "libgimp/gimpparasite.h"

#include "libgimp/gimpintl.h"


enum
{
  VISIBILITY_CHANGED,
  REMOVED,
  LAST_SIGNAL
};


static void   gimp_drawable_class_init         (GimpDrawableClass *klass);
static void   gimp_drawable_init               (GimpDrawable      *drawable);
static void   gimp_drawable_destroy            (GtkObject         *object);
static void   gimp_drawable_name_changed       (GimpObject        *drawable);
static void   gimp_drawable_invalidate_preview (GimpViewable      *viewable);


/*  private variables  */

static guint gimp_drawable_signals[LAST_SIGNAL] = { 0 };

static GimpViewableClass *parent_class = NULL;

static gint        global_drawable_ID  = 1;
static GHashTable *gimp_drawable_table = NULL;


GtkType
gimp_drawable_get_type (void)
{
  static GtkType drawable_type = 0;

  if (! drawable_type)
    {
      GtkTypeInfo drawable_info =
      {
	"GimpDrawable",
	sizeof (GimpDrawable),
	sizeof (GimpDrawableClass),
	(GtkClassInitFunc) gimp_drawable_class_init,
	(GtkObjectInitFunc) gimp_drawable_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      drawable_type = gtk_type_unique (GIMP_TYPE_VIEWABLE, &drawable_info);
    }

  return drawable_type;
}

static void
gimp_drawable_class_init (GimpDrawableClass *klass)
{
  GtkObjectClass    *object_class;
  GimpObjectClass   *gimp_object_class;
  GimpViewableClass *viewable_class;

  object_class      = (GtkObjectClass *) klass;
  gimp_object_class = (GimpObjectClass *) klass;
  viewable_class    = (GimpViewableClass *) klass;

  parent_class = gtk_type_class (GIMP_TYPE_VIEWABLE);

  gimp_drawable_signals[VISIBILITY_CHANGED] =
    gtk_signal_new ("visibility_changed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpDrawableClass,
                                       visibility_changed),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  gimp_drawable_signals[REMOVED] =
    gtk_signal_new ("removed",
                    GTK_RUN_FIRST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GimpDrawableClass,
                                       removed),
                    gtk_signal_default_marshaller,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_drawable_signals,
				LAST_SIGNAL);

  object_class->destroy = gimp_drawable_destroy;

  gimp_object_class->name_changed = gimp_drawable_name_changed;

  viewable_class->invalidate_preview = gimp_drawable_invalidate_preview;
  viewable_class->get_preview        = gimp_drawable_get_preview;

  klass->removed = NULL;
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
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
		       GINT_TO_POINTER (drawable->ID),
		       (gpointer) drawable);
}

static void
gimp_drawable_destroy (GtkObject *object)
{
  GimpDrawable *drawable;

  g_return_if_fail (GIMP_IS_DRAWABLE (object));

  drawable = GIMP_DRAWABLE (object);

  g_hash_table_remove (gimp_drawable_table, GINT_TO_POINTER (drawable->ID));

  if (drawable->tiles)
    tile_manager_destroy (drawable->tiles);

  if (drawable->preview_cache)
    gimp_preview_cache_invalidate (&drawable->preview_cache);

  if (drawable->parasites)
    gtk_object_unref (GTK_OBJECT (drawable->parasites));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_drawable_name_changed (GimpObject *object)
{
  GimpDrawable *drawable;
  GimpDrawable *drawable2;
  GList        *list, *list2, *base_list;
  gint          unique_ext = 0;
  gchar        *ext;
  gchar        *new_name = NULL;

  g_return_if_fail (GIMP_IS_DRAWABLE (object));

  drawable = GIMP_DRAWABLE (object);

  /*  if no other layers to check name against  */
  if (drawable->gimage == NULL || 
      gimp_image_is_empty (drawable->gimage))
    return;

  if (GIMP_IS_LAYER (drawable))
    base_list = GIMP_LIST (drawable->gimage->layers)->list;
  else if (GIMP_IS_CHANNEL (drawable))
    base_list = GIMP_LIST (drawable->gimage->channels)->list;
  else
    base_list = NULL;

  for (list = base_list; 
       list; 
       list = g_list_next (list))
    {
      drawable2 = GIMP_DRAWABLE (list->data);

      if (drawable != drawable2 &&
	  strcmp (gimp_object_get_name (GIMP_OBJECT (drawable)),
		  gimp_object_get_name (GIMP_OBJECT (drawable2))) == 0)
	{
          ext = strrchr (GIMP_OBJECT (drawable)->name, '#');

          if (ext)
            {
	      gchar *ext_str;

	      unique_ext = atoi (ext + 1);

	      ext_str = g_strdup_printf ("%d", unique_ext);

	      /*  check if the extension really is of the form "#<n>"  */
	      if (! strcmp (ext_str, ext + 1))
		{
		  *ext = '\0';
		}
	      else
                {
                  unique_ext = 0;
                }

              g_free (ext_str);
            }
          else
            {
              unique_ext = 0;
            }

	  do
	    {
	      unique_ext++;

	      g_free (new_name);

	      new_name = g_strdup_printf ("%s#%d",
					  GIMP_OBJECT (drawable)->name,
					  unique_ext);

              for (list2 = base_list; list2; list2 = g_list_next (list2))
                {
		  drawable2 = GIMP_DRAWABLE (list2->data);

		  if (drawable == drawable2)
		    continue;

                  if (! strcmp (GIMP_OBJECT (drawable2)->name, new_name))
                    {
                      break;
                    }
                }
            }
          while (list2);

          g_free (GIMP_OBJECT (drawable)->name);

          GIMP_OBJECT (drawable)->name = new_name;

          break;
        }
    }
}

static void
gimp_drawable_invalidate_preview (GimpViewable *viewable)
{
  GimpDrawable *drawable;
  GimpImage    *gimage;

  drawable = GIMP_DRAWABLE (viewable);

  drawable->preview_valid = FALSE;

  gimage = gimp_drawable_gimage (drawable);

  if (gimage)
    gimage->comp_preview_valid = FALSE;
}

void
gimp_drawable_configure (GimpDrawable  *drawable,
			 GimpImage     *gimage,
			 gint           width,
			 gint           height, 
			 GimpImageType  type,
			 const gchar   *name)
{
  gint     bpp;
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

  gimp_drawable_set_visible (drawable, TRUE);

  if (gimage)
    gimp_drawable_set_gimage (drawable, gimage);

  gimp_object_set_name (GIMP_OBJECT (drawable), name);

  /*  preview variables  */
  drawable->preview_cache = NULL;
  drawable->preview_valid = FALSE;
}

gint
gimp_drawable_get_ID (GimpDrawable *drawable)
{
  g_return_val_if_fail (drawable != NULL, -1);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->ID;
}

GimpDrawable *
gimp_drawable_get_by_ID (gint drawable_id)
{
  if (gimp_drawable_table == NULL)
    return NULL;

  return (GimpDrawable *) g_hash_table_lookup (gimp_drawable_table, 
					       GINT_TO_POINTER (drawable_id));
}

GimpImage *
gimp_drawable_gimage (const GimpDrawable *drawable)
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

void
gimp_drawable_merge_shadow (GimpDrawable *drawable,
			    gint          undo)
{
  GimpImage   *gimage;
  PixelRegion  shadowPR;
  gint         x1, y1, x2, y2;

  g_return_if_fail (drawable != NULL);
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
  gimp_image_apply_image (gimage, drawable, &shadowPR, undo, OPAQUE_OPACITY,
			  REPLACE_MODE, NULL, x1, y1);
}

void
gimp_drawable_fill (GimpDrawable  *drawable,
		    const GimpRGB *color)
{
  GimpImage   *gimage;
  PixelRegion  destPR;
  guchar       c[MAX_CHANNELS];
  guchar       i;

  g_return_if_fail (drawable != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gimage = gimp_drawable_gimage (drawable);

  g_return_if_fail (gimage != NULL);

  switch (gimp_drawable_type (drawable))
    {
    case RGB_GIMAGE: case RGBA_GIMAGE:
      gimp_rgba_get_uchar (color,
			   &c[RED_PIX],
			   &c[GREEN_PIX],
			   &c[BLUE_PIX],
			   &c[ALPHA_PIX]);
      if (gimp_drawable_type (drawable) != RGBA_GIMAGE)
	c[ALPHA_PIX] = 255;
      break;

    case GRAY_GIMAGE: case GRAYA_GIMAGE:
      gimp_rgba_get_uchar (color,
			   &c[GRAY_PIX],
			   NULL,
			   NULL,
			   &c[ALPHA_G_PIX]);
      if (gimp_drawable_type (drawable) != GRAYA_GIMAGE)
	c[ALPHA_G_PIX] = 255;
      break;

    case INDEXED_GIMAGE: case INDEXEDA_GIMAGE:
      gimp_rgb_get_uchar (color,
			  &c[RED_PIX],
			  &c[GREEN_PIX],
			  &c[BLUE_PIX]);
      gimp_image_transform_color (gimage, drawable, c, &i, RGB);
      c[INDEXED_PIX] = i;
      if (gimp_drawable_type (drawable) == INDEXEDA_GIMAGE)
	gimp_rgba_get_uchar (color,
			     NULL,
			     NULL,
			     NULL,
			     &c[ALPHA_I_PIX]);
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
  gint       off_x, off_y;

  g_return_val_if_fail (drawable != NULL, FALSE);
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

/* The removed signal is sent out when the layer is no longer
 * associcated with an image.  It's needed because layers aren't
 * destroyed immediately, but kept around for undo purposes.  Connect
 * to the removed signal to update bits of UI that are tied to a
 * particular layer.
 */
void
gimp_drawable_removed (GimpDrawable *drawable)
{
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  gtk_signal_emit (GTK_OBJECT (drawable), gimp_drawable_signals[REMOVED]);
}

gboolean
gimp_drawable_has_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return drawable->has_alpha;
}

GimpImageType
gimp_drawable_type (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->type;
}

GimpImageType
gimp_drawable_type_with_alpha (const GimpDrawable *drawable)
{
  GimpImageType type;
  gboolean      has_alpha;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  type = gimp_drawable_type (drawable);
  has_alpha = gimp_drawable_has_alpha (drawable);

  if (has_alpha)
    {
      return type;
    }
  else
    {
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
    }

  return 0;
}

gboolean
gimp_drawable_get_visible (const GimpDrawable *drawable)
{
  g_return_val_if_fail (drawable != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return drawable->visible;
}

void
gimp_drawable_set_visible (GimpDrawable *drawable,
                           gboolean      visible)
{
  g_return_if_fail (drawable != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  visible = visible ? TRUE : FALSE;

  if (drawable->visible != visible)
    {
      drawable->visible = visible;

      gtk_signal_emit (GTK_OBJECT (drawable),
                       gimp_drawable_signals[VISIBILITY_CHANGED]);
    }
}

guchar *
gimp_drawable_get_color_at (GimpDrawable *drawable,
			    gint          x,
			    gint          y)
{
  Tile   *tile;
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

  if (GIMP_IMAGE_TYPE_HAS_ALPHA (gimp_drawable_type (drawable)))
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
gimp_drawable_parasite_list_foreach_func (gchar          *key,
					  GimpParasite   *p,
					  gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (key);
}

gchar **
gimp_drawable_parasite_list (const GimpDrawable *drawable,
			     gint               *count)
{
  gchar **list;
  gchar **cur;

  g_return_val_if_fail (drawable != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (count != NULL, NULL);

  *count = parasite_list_length (drawable->parasites);
  cur = list = g_new (gchar *, *count);

  parasite_list_foreach (drawable->parasites,
			 (GHFunc) gimp_drawable_parasite_list_foreach_func,
			 &cur);

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
	   ! gimp_parasite_compare (parasite,
				    gimp_drawable_parasite_find
				    (drawable, gimp_parasite_name (parasite))))
    {
      undo_push_cantundo (drawable->gimage, _("parasite attach to drawable"));
    }

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

  if (! (p = parasite_list_find (drawable->parasites, parasite)))
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
gimp_drawable_set_tattoo (GimpDrawable *drawable,
			  Tattoo        val)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  drawable->tattoo = val;
}

gboolean
gimp_drawable_is_rgb (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if (gimp_drawable_type (drawable) == RGBA_GIMAGE ||
      gimp_drawable_type (drawable) == RGB_GIMAGE)
    return TRUE;
  else
    return FALSE;
}

gboolean
gimp_drawable_is_gray (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if (gimp_drawable_type (drawable) == GRAYA_GIMAGE ||
      gimp_drawable_type (drawable) == GRAY_GIMAGE)
    return TRUE;
  else
    return FALSE;
}

gboolean
gimp_drawable_is_indexed (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  if (gimp_drawable_type (drawable) == INDEXEDA_GIMAGE ||
      gimp_drawable_type (drawable) == INDEXED_GIMAGE)
    return TRUE;
  else
    return FALSE;
}

TileManager *
gimp_drawable_data (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->tiles;
}

TileManager *
gimp_drawable_shadow (GimpDrawable *drawable)
{
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return NULL;

  return gimp_image_shadow (gimage, drawable->width, drawable->height, 
			    drawable->bytes);
}

int
gimp_drawable_bytes (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->bytes;
}

gint
gimp_drawable_width (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->width;
}

gint
gimp_drawable_height (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  return drawable->height;
}

void
gimp_drawable_offsets (const GimpDrawable *drawable,
		       gint               *off_x,
		       gint               *off_y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  *off_x = drawable->offset_x;
  *off_y = drawable->offset_y;
}

guchar *
gimp_drawable_cmap (const GimpDrawable *drawable)
{
  GimpImage *gimage;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (! (gimage = gimp_drawable_gimage (drawable)))
    return NULL;

  return gimage->cmap;
}
