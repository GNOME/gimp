/*
 *  Guillotine plug-in v0.9 by Adam D. Moss, adam@foxbox.org.  1998/09/01
 */

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

/*
 * HISTORY:
 *     0.9 : 1998/09/01
 *           Initial release.
 */



#include <stdio.h>
#include <stdlib.h>
#include "libgimp/gimp.h"


/* Declare local functions.
 */
static void      query  (void);
static void      guillotine (gint32 image_ID);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);



GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable (unused)" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_guillotine",
			  "Slice up the image into subimages, cutting along the image's Guides.  Fooey to you and your broccoli, Pokey.",
			  "This function takes an image and blah blah.  Hooray!",
			  "Adam D. Moss (adam@foxbox.org)",
			  "Adam D. Moss (adam@foxbox.org)",
			  "1998",
			  "<Image>/Image/Transforms/Guillotine",
			  "RGB*, INDEXED*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}



static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  gint32 image_ID;
  GStatusType status = STATUS_SUCCESS;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;


  /*  Get the specified drawable  */
  image_ID = param[1].data.d_image;

  if (status == STATUS_SUCCESS)
    {
      gimp_progress_init ("Guillotine...");
      guillotine (image_ID);
      gimp_displays_flush ();
    }

  values[0].data.d_status = status;
}


int unexciting (const void *a, const void *b)
{
  gint j = *(gint*)a;
  gint k = *(gint*)b;

  return (j-k);
}


static void
guillotine(gint32 image_ID)
{
  gint num_vguides;
  gint num_hguides;
  gint guide_num;
  gint* hguides;
  gint* vguides;
  gchar filename[1024];
  gint i,x,y;
  GParam *return_vals;
  gint nreturn_vals;

  num_vguides = 0;
  num_hguides = 0;
  guide_num = gimp_image_findnext_guide(image_ID, 0);

  /* Count the guides so we can allocate appropriate memory */
  if (guide_num>0)
    {
      do
	{
	  switch (gimp_image_get_guide_orientation(image_ID, guide_num))
	    {
	    case ORIENTATION_VERTICAL:
	      num_vguides++; break;
	    case ORIENTATION_HORIZONTAL:
	      num_hguides++; break;
	    default:
	      printf("Aie!  Aie!  Aie!\n");
	      gimp_quit();
	    }
	  guide_num = gimp_image_findnext_guide(image_ID, guide_num);
	}
      while (guide_num>0);
    }

  if (num_vguides+num_hguides)
    {
      printf("Yay... found %d horizontal guides and %d vertical guides.\n",
	     num_hguides, num_vguides);
    }
  else
    {
      printf("Poopy, no guides.\n");
      return;
    }


  /* Allocate memory for the arrays of guide offsets, build arrays */
  vguides = g_malloc ((num_vguides+2) * sizeof(gint));
  hguides = g_malloc ((num_hguides+2) * sizeof(gint));
  num_vguides = 0;
  num_hguides = 0;
  vguides[num_vguides++] = 0;
  hguides[num_hguides++] = 0;
  guide_num = gimp_image_findnext_guide(image_ID, 0);
  if (guide_num>0)
    {
      do
	{
	  switch (gimp_image_get_guide_orientation(image_ID, guide_num))
	    {
	    case ORIENTATION_VERTICAL:
	      vguides[num_vguides++] =
		gimp_image_get_guide_position(image_ID, guide_num); break;
	    case ORIENTATION_HORIZONTAL:
	      hguides[num_hguides++] =
		gimp_image_get_guide_position(image_ID, guide_num); break;
	    default:
	      printf("Aie!  Aie!  Aie!  Too!\n");
	      gimp_quit();
	    }
	  guide_num = gimp_image_findnext_guide(image_ID, guide_num);
	}
      while (guide_num>0);
    }
  vguides[num_vguides++] = gimp_image_width(image_ID);
  hguides[num_hguides++] = gimp_image_height(image_ID);

  qsort(hguides, num_hguides, sizeof(gint), &unexciting);
  qsort(vguides, num_vguides, sizeof(gint), &unexciting);

  for (i=0;i<num_vguides;i++)
    printf("%d,",vguides[i]);
  printf("\n");
  for (i=0;i<num_hguides;i++)
    printf("%d,",hguides[i]);
  printf("\n");


  /* Do the actual dup'ing and cropping... this isn't a too naive a
     way to do this since we got copy-on-write tiles, either. */
  for (y=0; y<num_hguides-1; y++)
    {
      for (x=0; x<num_vguides-1; x++)
	{
	  gint32 new_image;

	  return_vals =
	    gimp_run_procedure("gimp_channel_ops_duplicate", &nreturn_vals,
			       PARAM_IMAGE, image_ID,
			       PARAM_END);

	  if (return_vals[0].data.d_status == STATUS_SUCCESS)
	    new_image = return_vals[1].data.d_int32;
	  else
	    {
	      printf("Aie3!\n");
	      return;
	    }

	  gimp_image_disable_undo (new_image);

	  gimp_run_procedure ("gimp_undo_push_group_start", &nreturn_vals,
			      PARAM_IMAGE, new_image,
			      PARAM_END);

/* 	  printf("(%dx%d:%d,%d:%d,%d)\n", */
/* 		 (vguides[x+1]-vguides[x]), */
/* 		 (hguides[y+1]-hguides[y]), */
/* 		 vguides[x], hguides[y],x, y);  */


	  gimp_run_procedure("gimp_crop", &nreturn_vals,
                       PARAM_IMAGE, new_image,
                       PARAM_INT32, (vguides[x+1]-vguides[x]),
                       PARAM_INT32, (hguides[y+1]-hguides[y]),
                       PARAM_INT32, vguides[x],
                       PARAM_INT32, hguides[y],
                       PARAM_END);

	  gimp_run_procedure ("gimp_undo_push_group_end", &nreturn_vals,
			      PARAM_IMAGE, new_image,
			      PARAM_END);

	  gimp_image_enable_undo (new_image);

	  /* show the rough coordinates of the image in the title */
  	  sprintf(filename, "%s-(%i,%i)", gimp_image_get_filename (image_ID), 
		     x, y);
	  gimp_image_set_filename(new_image, filename);

	  gimp_display_new (new_image);
	}
    }
}



