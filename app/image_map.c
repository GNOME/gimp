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
#include "appenv.h"
#include "drawable.h"
#include "gdisplay.h"
#include "gimage.h"
#include "gimage_mask.h"
#include "image_map.h"
#include "tile_manager.h"

#include "tile_manager_pvt.h"


#define WAITING 0
#define WORKING 1

#define WORK_DELAY 1

/*  Local structures  */
typedef struct _ImageMap
{
  GDisplay          *gdisp;
  GimpDrawable      *drawable;
  TileManager       *undo_tiles;
  ImageMapApplyFunc  apply_func;
  gpointer           user_data;
  PixelRegion        srcPR, destPR;
  void              *pr;
  gint               state;
  gint               idle;
} _ImageMap;


/**************************/
/*  Function definitions  */

static gint
image_map_do (gpointer data)
{
  _ImageMap *_image_map;
  GImage *gimage;
  PixelRegion shadowPR;
  int x, y, w, h;

  _image_map = (_ImageMap *) data;

  if (! (gimage = drawable_gimage ( (_image_map->drawable))))
    {
      _image_map->state = WAITING;
      return FALSE;
    }

  /*  Process the pixel regions and apply the image mapping  */
  (* _image_map->apply_func) (&_image_map->srcPR, &_image_map->destPR, _image_map->user_data);

  x = _image_map->destPR.x;
  y = _image_map->destPR.y;
  w = _image_map->destPR.w;
  h = _image_map->destPR.h;

  /*  apply the results  */
  pixel_region_init (&shadowPR, gimage->shadow, x, y, w, h, FALSE);
  gimage_apply_image (gimage, _image_map->drawable, &shadowPR,
		      FALSE, OPAQUE_OPACITY, REPLACE_MODE, NULL, x, y);

  /*  display the results  */
  if (_image_map->gdisp)
    {
      drawable_update ( (_image_map->drawable), x, y, w, h);
      gdisplay_flush_now (_image_map->gdisp);
    }

  _image_map->pr = pixel_regions_process (_image_map->pr);

  if (_image_map->pr == NULL)
    {
      _image_map->state = WAITING;
      gdisplays_flush ();
      return FALSE;
    }
  else
    return TRUE;
}

ImageMap
image_map_create (void         *gdisp_ptr,
		  GimpDrawable *drawable)
{
  _ImageMap *_image_map;

  _image_map = g_new (_ImageMap, 1);
  _image_map->gdisp = (GDisplay *) gdisp_ptr;
  _image_map->drawable = drawable;
  _image_map->undo_tiles = NULL;
  _image_map->state = WAITING;

  /* Interactive tools based on image_map disable the undo stack */
  /* to avert any unintented undo interaction through the UI     */

  gimp_image_undo_freeze(_image_map->gdisp->gimage);
  gdisplay_set_menu_sensitivity (_image_map->gdisp);
  return (ImageMap) _image_map;
}

void
image_map_apply (ImageMap           image_map,
		 ImageMapApplyFunc  apply_func,
		 void              *user_data)
{
  _ImageMap *_image_map;
  gint x1, y1, x2, y2;

  _image_map = (_ImageMap *) image_map;
  _image_map->apply_func = apply_func;
  _image_map->user_data = user_data;

  /*  If we're still working, remove the timer  */
  if (_image_map->state == WORKING)
    {
      gtk_idle_remove (_image_map->idle);
      pixel_regions_process_stop (_image_map->pr);
      _image_map->pr = NULL;
    }

  /*  Make sure the drawable is still valid  */
  if (! drawable_gimage ( (_image_map->drawable)))
    return;

  /*  The application should occur only within selection bounds  */
  drawable_mask_bounds ( (_image_map->drawable), &x1, &y1, &x2, &y2);

  /*  If undo tiles don't exist, or change size, (re)allocate  */
  if (!_image_map->undo_tiles ||
      _image_map->undo_tiles->x != x1 ||
      _image_map->undo_tiles->y != y1 ||
      _image_map->undo_tiles->width != (x2 - x1) ||
      _image_map->undo_tiles->height != (y2 - y1))
    {
      /*  If either the extents changed or the tiles don't exist, allocate new  */
      if (!_image_map->undo_tiles ||
	  _image_map->undo_tiles->width != (x2 - x1) ||
	  _image_map->undo_tiles->height != (y2 - y1))
	{
	  /*  Destroy old tiles--If they exist  */
	  if (_image_map->undo_tiles != NULL)
	    tile_manager_destroy (_image_map->undo_tiles);
	  
	  /*  Allocate new tiles  */
	  _image_map->undo_tiles = tile_manager_new ((x2 - x1), (y2 - y1),
						     drawable_bytes ( (_image_map->drawable)));
	}

      /*  Copy from the image to the new tiles  */
      pixel_region_init (&_image_map->srcPR, drawable_data ( (_image_map->drawable)),
			 x1, y1, (x2 - x1), (y2 - y1), FALSE);
      pixel_region_init (&_image_map->destPR, _image_map->undo_tiles,
			 0, 0, (x2 - x1), (y2 - y1), TRUE);

      copy_region (&_image_map->srcPR, &_image_map->destPR);

      /*  Set the offsets  */
      _image_map->undo_tiles->x = x1;
      _image_map->undo_tiles->y = y1;
    }
  else /* _image_map->undo_tiles exist AND drawable dimensions have not changed... */
    {
      /* Reset to initial drawable conditions.            */
      /* Copy from the backup undo tiles to the drawable  */
      pixel_region_init (&_image_map->srcPR, _image_map->undo_tiles, 0, 0,
			 _image_map->undo_tiles->width,
			 _image_map->undo_tiles->height,
			 FALSE);
      pixel_region_init (&_image_map->destPR, drawable_data ( (_image_map->drawable)),
			 _image_map->undo_tiles->x, _image_map->undo_tiles->y,
			 _image_map->undo_tiles->width,
			 _image_map->undo_tiles->height,
			 TRUE);

      copy_region (&_image_map->srcPR, &_image_map->destPR);
    }

  /*  Configure the src from the drawable data  */
  pixel_region_init (&_image_map->srcPR, _image_map->undo_tiles,
		     0, 0, (x2 - x1), (y2 - y1), FALSE);

  /*  Configure the dest as the shadow buffer  */
  pixel_region_init (&_image_map->destPR, drawable_shadow ( (_image_map->drawable)),
		     x1, y1, (x2 - x1), (y2 - y1), TRUE);

  /*  Apply the image transformation to the pixels  */
  _image_map->pr = pixel_regions_register (2, &_image_map->srcPR, &_image_map->destPR);

  /*  Start the intermittant work procedure  */
  _image_map->state = WORKING;
  _image_map->idle = gtk_idle_add (image_map_do, image_map);
}

void
image_map_commit (ImageMap image_map)
{
  _ImageMap *_image_map;
  gint x1, y1, x2, y2;

  _image_map = (_ImageMap *) image_map;

  if (_image_map->state == WORKING)
    {
      gtk_idle_remove (_image_map->idle);

      /*  Finish the changes  */
      while (image_map_do (image_map)) ;
    }

  /*  Make sure the drawable is still valid  */
  if (! drawable_gimage ( (_image_map->drawable)))
    return;

  /* Interactive phase ends: we can commit an undo frame now */
  gimp_image_undo_thaw(_image_map->gdisp->gimage);

  /*  Register an undo step  */
  if (_image_map->undo_tiles)
    {
      x1 = _image_map->undo_tiles->x;
      y1 = _image_map->undo_tiles->y;
      x2 = _image_map->undo_tiles->x + _image_map->undo_tiles->width;
      y2 = _image_map->undo_tiles->y + _image_map->undo_tiles->height;
      drawable_apply_image ( (_image_map->drawable), x1, y1, x2, y2, _image_map->undo_tiles, FALSE);
    }

  gdisplay_set_menu_sensitivity (_image_map->gdisp);
  g_free (_image_map);
}

void
image_map_clear (ImageMap image_map)
{
  _ImageMap *_image_map;
  PixelRegion srcPR, destPR;

  _image_map = (_ImageMap *) image_map;

  if (_image_map->state == WORKING)
    {
      gtk_idle_remove (_image_map->idle);
      pixel_regions_process_stop (_image_map->pr);
      _image_map->pr = NULL;
    }

  _image_map->state = WAITING;
  /*  Make sure the drawable is still valid  */
  if (! drawable_gimage ( (_image_map->drawable)))
    return;

  /*  restore the original image  */
  if (_image_map->undo_tiles)
    {
      /*  Copy from the drawable to the tiles  */
      pixel_region_init (&srcPR, _image_map->undo_tiles, 0, 0,
			 _image_map->undo_tiles->width,
			 _image_map->undo_tiles->height,
			 FALSE);
      pixel_region_init (&destPR, drawable_data ( (_image_map->drawable)),
			 _image_map->undo_tiles->x, _image_map->undo_tiles->y,
			 _image_map->undo_tiles->width,
			 _image_map->undo_tiles->height,
			 TRUE);

      /* if the user has changed the image depth get out quickly */
      if (destPR.bytes != srcPR.bytes) 
	{
	  g_message ("image depth change, unable to restore original image");
	  tile_manager_destroy (_image_map->undo_tiles);
          gimp_image_undo_thaw(_image_map->gdisp->gimage);
          gdisplay_set_menu_sensitivity (_image_map->gdisp);
	  g_free (_image_map);
	  return;
	}
      
      copy_region (&srcPR, &destPR);

      /*  Update the area  */
      drawable_update ( (_image_map->drawable),
		       _image_map->undo_tiles->x, _image_map->undo_tiles->y,
		       _image_map->undo_tiles->width,
		       _image_map->undo_tiles->height);

      /*  Free the undo_tiles tile manager  */
      tile_manager_destroy (_image_map->undo_tiles);
      _image_map->undo_tiles = NULL;
    }
}

void
image_map_abort (ImageMap image_map)
{
  _ImageMap *_image_map = (_ImageMap *) image_map;

  image_map_clear(image_map);
  gimp_image_undo_thaw(_image_map->gdisp->gimage);
  gdisplay_set_menu_sensitivity (_image_map->gdisp);
  g_free (image_map);
}

unsigned char *
image_map_get_color_at (ImageMap image_map, 
			gint     x, 
			gint     y)
{
  Tile *tile;
  unsigned char *src;
  unsigned char *dest;
  _ImageMap *_image_map;

  if(!image_map)
    return NULL;

  _image_map = (_ImageMap *) image_map;

  if (x >= 0 && x < gimp_drawable_width (_image_map->drawable) && 
      y >= 0 && y < gimp_drawable_height (_image_map->drawable))
    {
      /* Check if done damage to original image */
      if (!_image_map->undo_tiles)
	return (gimp_drawable_get_color_at (_image_map->drawable, x, y));

      if (!image_map ||
	  (!gimp_drawable_gimage(_image_map->drawable) && 
	   gimp_drawable_is_indexed(_image_map->drawable)) ||
	  x < 0 || y < 0 ||
	  x >= _image_map->undo_tiles->width ||
	  y >= _image_map->undo_tiles->height)
	{
	  return NULL;
	}

      dest = g_new(unsigned char, 5);

      tile = tile_manager_get_tile (_image_map->undo_tiles, x, y,
				    TRUE, FALSE);

      src = tile_data_pointer (tile, x % TILE_WIDTH, y % TILE_HEIGHT);

      gimp_image_get_color (gimp_drawable_gimage(_image_map->drawable),
			    gimp_drawable_type (_image_map->drawable), dest, src);

      if(TYPE_HAS_ALPHA(gimp_drawable_type (_image_map->drawable)))
	dest[3] = src[gimp_drawable_bytes (_image_map->drawable) - 1];
      else
	dest[3] = 255;
      if (gimp_drawable_is_indexed(_image_map->drawable))
	dest[4] = src[0];
      else
	dest[4] = 0;
      tile_release (tile, FALSE);
      return dest;
    }
  else /* out of bounds error */
    {
      return NULL;
    }
}
