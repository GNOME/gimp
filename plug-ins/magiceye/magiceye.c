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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ---------------------------------------------------------------------------------------
 *
 * Magic Eye version 0.2
 *
 * This is a plugin for the gimp.
 * Copyright (C) 1997 Alexander Schulz
 * New versions: http://www.uni-karlsruhe.de/~Alexander.Schulz/english/gimp/gimp.html
 * Mail: Alexander.Schulz@stud.uni-karlsruhe.de
 *
 * Some parts that deal with the interaction with the Gimp
 * are modified from other plugins.
 * Thanks to the other programmers.
 */

#include <libgimp/gimp.h>
#include <libgimp/gimpmenu.h>
#include "magiceye.h"

/* Declare local functions.
 */

/* static void text_callback (int, void *, void *); */


gdouble strip_width;
gdouble depth;
gint from_left, up_down;
GDrawable *map_drawable;
GStatusType status = STATUS_SUCCESS;


GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ()

void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (unused)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" },
    { PARAM_INT32, "mapimage", "Map Image" },
  };
  static GParamDef return_vals[] =
  {
    { PARAM_IMAGE, "new_image", "Output image" },
    { PARAM_IMAGE, "new_layer", "Output layer" },
  };
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

  gimp_install_procedure ("plug_in_magic_eye",
			  "Create a stereogram",
			  "Create a stereogram",
			  "Alexander Schulz",
			  "Alexander Schulz",
			  "1997",
			  "<Image>/Filters/Misc/Magic Eye",
			  "GRAY",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[3];
  GRunModeType run_mode;
  gint32 new_layer;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 3;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  values[1].type = PARAM_IMAGE;
  values[1].type = PARAM_LAYER;

  /*  Get the specified drawable  */
  map_drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      /* gimp_get_data ("plug_in_tile", &tvals); */

      /*  First acquire information with a dialog  */
      if (! magic_dialog())
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      /*if (nparams != 5)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  tvals.new_width = param[3].data.d_int32;
	  tvals.new_height = param[4].data.d_int32;
	}
      if (tvals.new_width < 0 || tvals.new_height < 0)
	status = STATUS_CALLING_ERROR;*/
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      /* gimp_get_data ("plug_in_tile", &tvals); */
      break;

    default:
      break;
    }

  /*  Make sure that the drawable is gray or RGB color  */
  if (status == STATUS_SUCCESS)
    {
      values[1].data.d_image = magiceye (map_drawable, &new_layer);
      values[2].data.d_layer = new_layer;
      gimp_display_new (values[1].data.d_image);
      return;
    }
}



gint
magiceye (GDrawable *mapimage,gint32 *layer)
{
  GDrawableType dest_type;
  unsigned char *src1p;
  unsigned char *src2p;
  unsigned char *destp;
  unsigned char *cmap;
  unsigned char *savep;
  unsigned char *temp_buf;
  long src1_width, src1_height;
  long src2_width, src2_height;
  long src2_channels;
  long dest_channels;

  GPixelRgn streifen_rgn;
  GDrawable *streifen;
  GPixelRgn dest_rgn;
  GDrawable *destination;
  GPixelRgn map_rgn;

  gint32 dest, image_ID;
  int i, j, hoehe, x, _up_down;
  gint number;

  streifen = gimp_drawable_get(magiceye_ID);
  image_ID=gimp_drawable_image_id(magiceye_ID);
  gimp_pixel_rgn_init(&streifen_rgn, streifen, 0, 0, streifen->width, streifen->height, FALSE, FALSE);
  gimp_pixel_rgn_init(&map_rgn, mapimage, 0, 0, mapimage->width, mapimage->height, FALSE, FALSE);

  dest_type = gimp_drawable_type(magiceye_ID);

  src2_channels = streifen->bpp;

  src1p = g_malloc(mapimage->width*mapimage->height);
  src2p = g_malloc(streifen->width*streifen->height*src2_channels);

  gimp_pixel_rgn_get_rect(&map_rgn, src1p, 0, 0, mapimage->width, mapimage->height);
  gimp_pixel_rgn_get_rect(&streifen_rgn, src2p, 0, 0, streifen->width, streifen->height);

  src2_width = streifen->width;
  src2_height = streifen->height;
  src1_width = mapimage->width;
  src1_height = mapimage->height;

  dest_channels = src2_channels;
  

  switch (dest_type){
    case RGB_IMAGE:
      dest= gimp_image_new (src1_width, src1_height, RGB);
      break;
    case GRAY_IMAGE:
      dest= gimp_image_new (src1_width, src1_height, GRAY);
      break;
    case INDEXED_IMAGE:
      dest= gimp_image_new (src1_width, src1_height, INDEXED);
      break;
    default:
      printf("Magic Eye: cannot operate on unknown image types or alpha images");
      /* gimp_quit (); */
      return -1;
      break;
    }

  *layer = gimp_layer_new (dest, "Background", src1_width, src1_height, dest_type, 100, NORMAL_MODE);
  
  
  gimp_image_add_layer(dest,*layer,0);
  destination = gimp_drawable_get(*layer);

  destp = g_malloc(mapimage->width*mapimage->height*dest_channels);
  savep = destp;

  if (dest_type == INDEXED_IMAGE)
    {
      cmap = gimp_image_get_cmap (image_ID, &number);
#ifdef DEBUG
      printf("ColorMap: %i %i \n",magiceye_ID, image_ID);
      for (i=0;i<number;i++) { printf("%4i: %x %x %x\n",i,cmap[i],cmap[i+1],cmap[i+2]); }
#endif
      gimp_image_set_cmap(dest, cmap, number);
      g_free(cmap);
    } 


#ifdef DEBUG
    printf("%i %i %i %i %i %i %i %i %i %i\n",magiceye_ID,src2_width,src2_height,src1_width,src1_height,dest_channels,(int)strip_width,dest,number,cmap);
#endif

  temp_buf = g_malloc (20);
  sprintf (temp_buf, "Creating image");
  gimp_progress_init (temp_buf);
  g_free (temp_buf);
    
  
  /* Here ist the point where the work is done, not too much actually */
  /* First copy the Background to the new image */
  for (i = 0; i < src1_height; i++)
    {
      for (j = 0; j < src1_width*dest_channels; j++)
	{
	  *destp++ = *(src2p+((j%((int)strip_width*dest_channels))+src2_width*(i%src2_height)*dest_channels));
	}
      if ((i % 5) == 0)
	gimp_progress_update ((double) i / (double) src1_height);
    }
    
  destp = savep;
  gimp_progress_update(0);
  
  if (up_down) _up_down=-1; else _up_down=1;

  if (from_left) {
  /* Then add the map-image */
  /* Beginning from the left */
  for (j = strip_width; j < src1_width; j++)
    {
      for (i = 0; i < src1_height; i++)
	{
	  hoehe=src1p[i*src1_width*1+j*1] * depth / 255;
	  for (x=0; x<dest_channels; x++)
	    {
	       destp[(i*src1_width+j-hoehe*_up_down)*dest_channels+x] =
	         destp[(i*src1_width+j-(int)strip_width)*dest_channels+x];
	    }
	}
      if ((j % 5) == 0)
	gimp_progress_update ((double) j / (double) src1_width);
    }
  } else {
  /* Or add the map-image */
  /* Beginning from the middle */
  for (j = (src1_width / 2 / strip_width) * strip_width; j < src1_width; j++)
    {
      for (i = 0; i < src1_height; i++)
	{
	  hoehe=src1p[i*src1_width*1+j*1] * depth / 255;
	  for (x=0; x<dest_channels; x++)
	    {
	       destp[(i*src1_width+j-hoehe*_up_down)*dest_channels+x] =
	         destp[(i*src1_width+j-(int)strip_width)*dest_channels+x];
	    }
	}
      if ((j % 5) == 0)
	gimp_progress_update ((double) (j - (src1_width / 2 / strip_width) * strip_width ) / (double) src1_width);
    }
  for (j = (src1_width / 2 / strip_width) * strip_width; j > strip_width; j--)
    {
      for (i = 0; i < src1_height; i++)
	{
	  hoehe=src1p[i*src1_width*1+j*1] * depth / 255;
	  for (x=0; x<dest_channels; x++)
	    {
	       destp[(i*src1_width+j+hoehe*_up_down-(int)strip_width)*dest_channels+x] =
	         destp[(i*src1_width+j)*dest_channels+x];
	    }
	}
      if ((j % 5) == 0)
	gimp_progress_update ((double) (src1_width - j) / (double) src1_width);
    }
    gimp_progress_update(1);
  }

  gimp_progress_update(0);

  /* end of work */

  gimp_pixel_rgn_init(&dest_rgn, destination, 0, 0, destination->width, destination->height, TRUE, FALSE);
  gimp_pixel_rgn_set_rect(&dest_rgn, savep, 0, 0, destination->width, destination->height);
  gimp_drawable_detach(destination);
  g_free(src2p);
  g_free(src1p);
  g_free(savep);
  return dest;
}
