/*  
 *  Rotate plug-in v0.9 
 *  by Sven Neumann <sven@gimp.org> and Adam D. Moss <adam@gimp.org>
 *
 *  Any suggestions, bug-reports or patches are very welcome.
 * 
 *  A lot of changes in version 0.3 were inspired by (or even simply 
 *  copied from) the similar rotators-plug-in by Adam D. Moss. 
 *
 *  As I still prefer my version I have not stopped developing it.
 *  Probably this will result in one plug-in that includes the advantages 
 *  of both approaches.
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

/* Revision history
 *  (09/28/97)  v0.1   first development release 
 *  (09/29/97)  v0.2   nicer dialog,
 *                     changed the menu-location to Filters/Transforms
 *  (10/01/97)  v0.3   now handles layered images and undo
 *  (10/13/97)  v0.3a  small bugfix, no real changes
 *  (10/17/97)  v0.4   now handles selections
 *  (01/09/98)  v0.5   a few fixes to support portability
 *  (01/15/98)  v0.6   fixed a line that caused rotate to crash on some 
 *                     systems               
 *  (05/28/98)  v0.7   use the new gimp_message function for error output
 *  (10/09/99)  v0.8   rotate guides too
 *  (11/13/99)  v0.9   merge rotators and rotate plug-ins 
 *                     -> drop the dialog, register directly into menus instead
 */

/* TODO List
 *  - handle channels and masks
 *  - rewrite the main function to make it work on tiles rather than
 *    process the image row by row. This should result in a significant
 *    speedup (thanks to quartic for this suggestion). 
 *  - do something magical so that only one rotate can be occuring at a time!
 */  

#include <stdlib.h>
#include "config.h"
#include "libgimp/gimp.h"
#include "libgimp/stdplugins-intl.h"

/* Defines */
#define PLUG_IN_VERSION     "v0.9 (1999/11/13)"
#define PLUG_IN_IMAGE_TYPES "RGB*, INDEXED*, GRAY*"
#define PLUG_IN_AUTHOR      "Sven Neumann <sven@gimp.org>, Adam D. Moss <adam@gimp.org>"
#define PLUG_IN_COPYRIGHT   "Sven Neumann, Adam D. Moss"


typedef struct 
{
  gint angle;
  gint everything;
} RotateValues;

typedef struct 
{
  gint32 ID;
  gint32 orientation;
  gint32 position;
} GuideInfo;

static RotateValues rotvals = 
{ 
  1,        /* default to 90 degrees */
  1         /* default to whole image */
};


static void  query   (void);
static void  run     (gchar      *name,
		      gint        nparams,	
		      GParam     *param,	
		      gint       *nreturn_vals,  
		      GParam    **return_vals);
 
static void  rotate                 (void);
static void  rotate_drawable        (GDrawable  *drawable);
static void  rotate_compute_offsets (gint       *offsetx, 
				     gint       *offsety, 
				     gint       image_width, 
				     gint       image_height,
				     gint       width, 
				     gint       height);

/* Global Variables */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,   /* init_proc  */
  NULL,	  /* quit_proc  */
  query,  /* query_proc */
  run     /* run_proc   */
};

/* the image and drawable that will be used later */
GDrawable *active_drawable = NULL;
gint32    image_ID = -1;

/* Functions */

MAIN ()

static void query (void)
{
static GParamDef args[] = {
  { PARAM_INT32,    "run_mode",   "Interactive, non-interactive"},\
  { PARAM_IMAGE,    "image",      "Input image" },\
  { PARAM_DRAWABLE, "drawable",   "Input drawable"},\
  { PARAM_INT32,    "angle",      "Angle { 90 (1), 180 (2), 270 (3) } degrees"},\
  { PARAM_INT32,    "everything", "Rotate the whole image? { TRUE, FALSE }"} 
};
static gint nargs = sizeof (args) / sizeof (args[0]);

static GParamDef menuargs[] = {
  { PARAM_INT32,    "run_mode",   "Interactive, non-interactive"},\
  { PARAM_IMAGE,    "image",      "Input image" },\
  { PARAM_DRAWABLE, "drawable",   "Input drawable"},\
};
static gint nmenuargs = sizeof (menuargs) / sizeof (menuargs[0]);

static GParamDef *return_vals = NULL;
static gint nreturn_vals      = 0;

INIT_I18N(); 

/*install the plugin */
gimp_install_procedure ("plug_in_rotate",
                        "Rotates a layer or the whole image by 90, 180 or 270 degrees",
                        "This plug-in does rotate the active layer or the whole image clockwise by multiples of 90 degrees. When the whole image is choosen, the image is resized if necessary.",
			PLUG_IN_AUTHOR,
			PLUG_IN_COPYRIGHT,
			PLUG_IN_VERSION,
			NULL,
			PLUG_IN_IMAGE_TYPES,
			PROC_PLUG_IN,		
			nargs, nreturn_vals,
			args, return_vals);

gimp_install_procedure ("plug_in_layer_rot90",
			"Rotates the given layer 90 degrees clockwise.",
			"Rotates the given layer 90 degrees clockwise.",
			PLUG_IN_AUTHOR,
			PLUG_IN_COPYRIGHT,
			PLUG_IN_VERSION,
			N_("<Image>/Layers/Rotate/90 degrees"),
			PLUG_IN_IMAGE_TYPES,
			PROC_PLUG_IN,
			nmenuargs, nreturn_vals,
			menuargs, return_vals);
gimp_install_procedure ("plug_in_layer_rot180",
			"Rotates the given layer 180 degrees.",
			"Rotates the given layer 180 degrees.",
			PLUG_IN_AUTHOR,
			PLUG_IN_COPYRIGHT,
			PLUG_IN_VERSION,
			N_("<Image>/Layers/Rotate/180 degrees"),
			PLUG_IN_IMAGE_TYPES,
			PROC_PLUG_IN,
			nmenuargs, nreturn_vals,
			menuargs, return_vals);
gimp_install_procedure ("plug_in_layer_rot270",
			"Rotates the given layer 270 degrees clockwise.",
			"Rotates the given layer 270 degrees clockwise.",
			PLUG_IN_AUTHOR,
			PLUG_IN_COPYRIGHT,
			PLUG_IN_VERSION,
			N_("<Image>/Layers/Rotate/270 degrees"),
			PLUG_IN_IMAGE_TYPES,
			PROC_PLUG_IN,
			nmenuargs, nreturn_vals,
			menuargs, return_vals);

gimp_install_procedure ("plug_in_image_rot90",
			"Rotates the given image 90 degrees clockwise.",
			"Rotates the given image 90 degrees clockwise.",
			PLUG_IN_AUTHOR,
			PLUG_IN_COPYRIGHT,
			PLUG_IN_VERSION,
			N_("<Image>/Image/Transforms/Rotate/90 degrees"),
			PLUG_IN_IMAGE_TYPES,
			PROC_PLUG_IN,
			nmenuargs, nreturn_vals,
			menuargs, return_vals);
gimp_install_procedure ("plug_in_image_rot180",
			"Rotates the given image 180 degrees.",
			"Rotates the given image 180 degrees.",
			PLUG_IN_AUTHOR,
			PLUG_IN_COPYRIGHT,
			PLUG_IN_VERSION,
			N_("<Image>/Image/Transforms/Rotate/180 degrees"),
			PLUG_IN_IMAGE_TYPES,
			PROC_PLUG_IN,
			nmenuargs, nreturn_vals,
			menuargs, return_vals);
gimp_install_procedure ("plug_in_image_rot270",
			"Rotates the given image 270 degrees clockwise.",
			"Rotates the given image 270 degrees clockwise.",
			PLUG_IN_AUTHOR,
			PLUG_IN_COPYRIGHT,
			PLUG_IN_VERSION,
			N_("<Image>/Image/Transforms/Rotate/270 degrees"),
			PLUG_IN_IMAGE_TYPES,
			PROC_PLUG_IN,
			nmenuargs, nreturn_vals,
			menuargs, return_vals);
}

static void 
run (gchar    *name,
     gint      nparams,
     GParam   *param,
     gint     *nreturn_vals,
     GParam **return_vals)
{
  /* Get the runmode from the in-parameters */
  GRunModeType run_mode = param[0].data.d_int32;	
  
  /* status variable, use it to check for errors in invocation usualy only 
     during non-interactive calling */	
  GStatusType status = STATUS_SUCCESS;	 
  
  /*always return at least the status to the caller. */
  static GParam values[1];
  
  /* initialize the return of the status */ 	
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  *nreturn_vals = 1;
  *return_vals = values;

  INIT_I18N(); 

  /* get image and drawable */
  image_ID = param[1].data.d_int32;
  active_drawable = gimp_drawable_get (param[2].data.d_drawable);
  
  /* Check the procedure name we were called with, to decide
     what needs to be done. */
  if (strcmp (name, "plug_in_rotate") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_NONINTERACTIVE:
	  /* check to see if invoked with the correct number of parameters */
	  if (nparams == 5)
	    {
	      rotvals.angle = (gint) param[3].data.d_int32;
	      rotvals.angle = rotvals.angle % 4;
	      rotvals.everything = (gint) param[4].data.d_int32;
	      /* Store variable states for next run */
	      gimp_set_data ("plug_in_rotate", &rotvals, sizeof (RotateValues));
	    }
	  else
	    status = STATUS_CALLING_ERROR;
	  break;
	case RUN_WITH_LAST_VALS:
	  /* Possibly retrieve data from a previous run */
	  gimp_get_data ("plug_in_rotate", &rotvals);
	  rotvals.angle = rotvals.angle % 4;
	  break;
	default:
	  break;
	}
    }
  else if (strcmp (name, "plug_in_layer_rot90") == 0)
    {
      rotvals.angle      = 1;
      rotvals.everything = FALSE;
    }
  else if (strcmp (name, "plug_in_layer_rot180") == 0)
    {
      rotvals.angle      = 2;
      rotvals.everything = FALSE;
    }
  else if (strcmp (name, "plug_in_layer_rot270") == 0)
    {
      rotvals.angle      = 3;
      rotvals.everything = FALSE;
    }
  else if (strcmp (name, "plug_in_image_rot90") == 0)
    {
      rotvals.angle      = 1;
      rotvals.everything = TRUE;
    }
  else if (strcmp (name, "plug_in_image_rot180") == 0)
    {
      rotvals.angle      = 2;
      rotvals.everything = TRUE;
    }
  else if (strcmp (name, "plug_in_image_rot270") == 0)
    {
      rotvals.angle      = 3;
      rotvals.everything = TRUE;
    }
  else 
    status = STATUS_CALLING_ERROR;

  if (status == STATUS_SUCCESS)
    {
      /* Run the main function */
      rotate ();
      
      /* If run mode is interactive, flush displays, else (script) don't 
	 do it, as the screen updates would make the scripts slow */
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush ();
    }
  values[0].data.d_status = status; 
}

static void
rotate_compute_offsets (gint *offsetx,
			gint *offsety,
			gint  image_width, 
			gint  image_height,
			gint  width, 
			gint  height)
{
  gint buffer;

  if (rotvals.everything)  /* rotate around the image center */
    {
      switch ( rotvals.angle )
	{
	case 1:   /* 90° */
	  buffer   = *offsetx;
	  *offsetx = image_height - *offsety - height;
	  *offsety = buffer;
	  break;
	case 2:   /* 180° */
	  *offsetx = image_width - *offsetx - width;
	  *offsety = image_height - *offsety - height;
	  break;
	case 3:   /* 270° */
	  buffer   = *offsetx;
	  *offsetx = *offsety;
	  *offsety = image_width - buffer - width;
	}
    }
  else  /* rotate around the drawable center */
    {
      if (rotvals.angle != 2)
	{
	  *offsetx = *offsetx + (width-height)/2 ;
	  *offsety = *offsety + (height-width)/2 ;
	}
    }
  return;
}


static void
rotate_drawable (GDrawable *drawable)
{
  GPixelRgn srcPR, destPR;
  gint width, height, longside, bytes;
  gint row, col, byte;
  gint offsetx, offsety;
  gint was_preserve_transparency = FALSE;
  guchar *buffer, *src_row, *dest_row;

  /* initialize */

  row = 0;

  /* Get the size of the input drawable. */
  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  if (gimp_layer_get_preserve_transparency (drawable->id))
    {
	was_preserve_transparency = TRUE;
	gimp_layer_set_preserve_transparency (drawable->id, FALSE);
    }

  if (rotvals.angle == 2)  /* we're rotating by 180° */
    {
      gimp_tile_cache_ntiles (2 * (width / gimp_tile_width()) + 1);

      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, 
			   FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height,
			   TRUE, TRUE);
      
      src_row  = (guchar *) g_malloc (width * bytes);
      dest_row = (guchar *) g_malloc (width * bytes);
      
      for (row = 0; row < height; row++)
	{
	  gimp_pixel_rgn_get_row (&srcPR, src_row, 0, row, width); 
	  for (col = 0; col < width; col++) 
	    { 
	      for (byte = 0; byte < bytes; byte++)
		{
		  dest_row[col * bytes + byte] =  
		    src_row[(width - col - 1) * bytes + byte];
		}
	    } 	    
	  gimp_pixel_rgn_set_row (&destPR, dest_row, 0, (height - row - 1), 
				  width);
	  
	  if ((row % 5) == 0)
	    gimp_progress_update ((double) row / (double) height);
	}
 
      g_free (src_row);
      g_free (dest_row);
 
      gimp_drawable_flush (drawable); 
      gimp_drawable_merge_shadow (drawable->id, TRUE); 
      gimp_drawable_update (drawable->id, 0, 0, width, height); 

    }
  else                     /* we're rotating by 90° or 270° */
    {  
      (width > height) ? (longside = width) : (longside = height);

      gimp_layer_resize (drawable->id, longside, longside, 0, 0);
      drawable = gimp_drawable_get (drawable->id);
      gimp_drawable_flush (drawable);
      
      gimp_tile_cache_ntiles ( ((longside/gimp_tile_width())+1) + 
			       ((longside/gimp_tile_height())+1) );

      gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, longside, longside, 
			   FALSE, FALSE);
      gimp_pixel_rgn_init (&destPR, drawable, 0, 0, longside, longside,
			   TRUE, TRUE);
      
      buffer = g_malloc (longside * bytes);
      
      if (rotvals.angle == 1)     /* we're rotating by 90° */
	{
	  for (row = 0; row < height; row++)
	    {
	      gimp_pixel_rgn_get_row (&srcPR, buffer, 0, row, width); 
	      gimp_pixel_rgn_set_col (&destPR, buffer, (height - row - 1), 0,
				      width);

	      if ((row % 5) == 0)
		gimp_progress_update ((double) row / (double) height);
	    }
	}
      else                        /* we're rotating by 270° */
	{
	  for (col = 0; col < width; col++)
	    {
	      gimp_pixel_rgn_get_col (&srcPR, buffer, col, 0, height); 
	      gimp_pixel_rgn_set_row (&destPR, buffer, 0, (width - col - 1), 
				      height);

	      if ((col % 5) == 0)
		gimp_progress_update ((double) col / (double) width);
	    }
	}
 
      g_free (buffer);

      gimp_progress_update ( 1.0 );

      gimp_drawable_flush (drawable); 
      gimp_drawable_merge_shadow (drawable->id, TRUE);
      gimp_drawable_update (drawable->id, 0, 0, height, width); 

      gimp_layer_resize (drawable->id, height, width, 0, 0);
      drawable = gimp_drawable_get (drawable->id);
      gimp_drawable_flush (drawable);
      gimp_drawable_update (drawable->id, 0, 0, height, width);
    }

  gimp_drawable_offsets (drawable->id, &offsetx, &offsety);
  rotate_compute_offsets (&offsetx, &offsety, 
			  gimp_image_width (image_ID),
			  gimp_image_height (image_ID), 
			  width, height); 
  gimp_layer_set_offsets (drawable->id, offsetx, offsety);

  if (was_preserve_transparency)
    gimp_layer_set_preserve_transparency ( drawable->id, TRUE );

  return;
}


/* The main rotate function */
static void 
rotate (void)
{
  GDrawable *drawable;
  gint32 *layers;
  gint i;
  gint nlayers;
  gint32 guide_ID;
  GuideInfo *guide;
  GList *guides = NULL;
  GList *list;

  if (rotvals.angle == 0) return;  

  /* if there's a selection and we try to rotate the whole image */
  /* create an error message and exit                            */     
  if ( rotvals.everything )
    {
      if ( !gimp_selection_is_empty (image_ID) ) 
	{
	  gimp_message (_("You can not rotate the whole image if there's a selection."));
	  gimp_drawable_detach (active_drawable);
	  return;
	}
      if ( gimp_layer_is_floating_selection (active_drawable->id) ) 
	{
	  gimp_message (_("You can not rotate the whole image if there's a floating selection."));
	  gimp_drawable_detach (active_drawable);
	  return;
	}
    }
  else
    /* if we are trying to rotate a chennel or a mask, create an error message and exit */
    {
      if ( !gimp_drawable_is_layer (active_drawable->id) )
	{
	  gimp_message (_("Sorry, channels and masks can not be rotated."));
	  gimp_drawable_detach (active_drawable);
	  return;
	}
    } 
  
  gimp_progress_init (_("Rotating..."));

  gimp_undo_push_group_start (image_ID);

  if (rotvals.everything)  /* rotate the whole image */ 
    {
      gint32 width = gimp_image_width (image_ID);
      gint32 height = gimp_image_height (image_ID);

      gimp_drawable_detach (active_drawable);
      layers = gimp_image_get_layers (image_ID, &nlayers);
      for ( i=0; i<nlayers; i++ )
	{
	  drawable = gimp_drawable_get (layers[i]);
	  rotate_drawable (drawable);
	  gimp_drawable_detach (drawable);
	}
      g_free(layers);	  

      /* build a list of all guides and remove them */
      guide_ID = 0;
      while ((guide_ID = gimp_image_find_next_guide (image_ID, guide_ID)) != 0)
	{
	  guide = g_new (GuideInfo, 1);
	  guide->ID = guide_ID;
	  guide->orientation = gimp_image_get_guide_orientation (image_ID, guide_ID);
	  guide->position = gimp_image_get_guide_position (image_ID, guide_ID);
	  guides = g_list_prepend (guides, guide);
	}
      for (list = guides; list; list = list->next)  
	{
	  guide = (GuideInfo *)list->data;
	  gimp_image_delete_guide (image_ID, guide->ID);
	}

      /* if rotation is not 180 degrees, resize the image */
      /*    Do it now after the guides are removed, since */ 
      /*    gimp_image_resize() moves the guides.         */ 
      if (rotvals.angle != 2)
	gimp_image_resize (image_ID, height, width, 0, 0);

      /* add the guides back to the image */
      if (guides)
	{
	  switch (rotvals.angle)
	    {
	    case 1:
	      for (list = guides; list; list = list->next)  
		{
		  guide = (GuideInfo *)list->data;
		  if (guide->orientation == ORIENTATION_HORIZONTAL)
		    gimp_image_add_vguide (image_ID, height - guide->position);
		  else
		    gimp_image_add_hguide (image_ID, guide->position);
		  g_free (guide);
		}     
	      break;
	    case 2:
	      for (list = guides; list; list = list->next)  
		{
		  guide = (GuideInfo *)list->data;
		  if (guide->orientation == ORIENTATION_HORIZONTAL)
		    gimp_image_add_hguide (image_ID, height - guide->position);
		  else
		    gimp_image_add_vguide (image_ID, width - guide->position);
		  g_free (guide);
		}     
	      break;
	    case 3:
	      for (list = guides; list; list = list->next)  
		{
		  guide = (GuideInfo *)list->data;
		  if (guide->orientation == ORIENTATION_HORIZONTAL)
		    gimp_image_add_vguide (image_ID, guide->position);
		  else
		    gimp_image_add_hguide (image_ID, width - guide->position);
		  g_free (guide);
		}     
	      break;
	    default: 
	      break;
	    }
	  g_list_free (guides);
	}
    }
  else  /* rotate only the active layer */
    {

      /* check for active selection and float it */
      if ( !gimp_selection_is_empty (image_ID) &&
	   !gimp_layer_is_floating_selection (active_drawable->id) )
	active_drawable = 
	  gimp_drawable_get (gimp_selection_float (image_ID, 
						   active_drawable->id,  
						   0, 0));
			    
      rotate_drawable (active_drawable);
      gimp_drawable_detach (active_drawable);
    }

  gimp_undo_push_group_end (image_ID);

  return;
}

