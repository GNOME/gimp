/*
 * Rotate plug-in version 1.00 - 97.09.28
 * by Adam D. Moss <adam@gimp.org>
 *
 * This plugin will rotate a single layer or a whole image
 * by 90 degrees clockwise or anticlockwise.  (So I guess it's
 * actually four plugins in one.  Bonus!)
 *
 * This is part of the GIMP package and falls under the GPL.
 */

/*
 * TODO:
 *
 * Work somewhat better with channels & masks
 * Test PDB interface
 * Progress bar
 * 
 * Do something magical so that only one rotate can be occuring
 *   at a time!
 */

#include <stdlib.h>
#include <stdio.h>
#include "libgimp/gimp.h"

/* Declare local functions. */
static void query(void);
static void run(char *name,
		int nparams,
		GParam * param,
		int *nreturn_vals,
		GParam ** return_vals);

static void do_layerrot(GDrawable*, gint32, gboolean, gboolean);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc */
  NULL,  /* quit_proc */
  query, /* query_proc */
  run,   /* run_proc */
};


MAIN()

static void query()
{
  static GParamDef args[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image"},
    {PARAM_DRAWABLE, "drawable", "Input drawable"},
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof(args) / sizeof(args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure("plug_in_layer_rot90",
			 "Rotates the given layer 90 degrees clockwise.",
			 "",
			 "Adam D. Moss (adam@gimp.org)",
			 "Adam D. Moss (adam@gimp.org)",
			 "1997",
			 "<Image>/Image/Transforms/Layer/Rotate 90",
			 "RGB*, GRAY*, INDEXED*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);

  gimp_install_procedure("plug_in_layer_rot270",
			 "Rotates the given layer 90 degrees anticlockwise.",
			 "",
			 "Adam D. Moss (adam@gimp.org)",
			 "Adam D. Moss (adam@gimp.org)",
			 "1997",
			 "<Image>/Image/Transforms/Layer/Rotate 270",
			 "RGB*, GRAY*, INDEXED*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);

  gimp_install_procedure("plug_in_image_rot90",
			 "Rotates the given image 90 degrees clockwise.",
			 "",
			 "Adam D. Moss (adam@gimp.org)",
			 "Adam D. Moss (adam@gimp.org)",
			 "1997",
			 "<Image>/Image/Transforms/Image/Rotate 90",
			 "RGB*, GRAY*, INDEXED*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);

  gimp_install_procedure("plug_in_image_rot270",
			 "Rotates the current image 90 degrees anticlockwise.",
			 "",
			 "Adam D. Moss (adam@gimp.org)",
			 "Adam D. Moss (adam@gimp.org)",
			 "1997",
			 "<Image>/Image/Transforms/Image/Rotate 270",
			 "RGB*, GRAY*, INDEXED*",
			 PROC_PLUG_IN,
			 nargs, nreturn_vals,
			 args, return_vals);
}

static void run(char *name, int n_params, GParam * param, int *nreturn_vals,
		GParam ** return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_id;
  gboolean clockwise;
  gboolean alllayers;
  gint32 *layers;
  int nlayers,i;
  int proc_returnvals;


  /* Check the procedure name we were called with, to decide
     what needs to be done. */
  if (
      (strcmp(name,"plug_in_layer_rot90")==0)
      ||
      (strcmp(name,"plug_in_image_rot90")==0)
      )      
    clockwise = TRUE;
  else
    clockwise = FALSE;

  if (
      (strcmp(name,"plug_in_image_rot270")==0)
      ||
      (strcmp(name,"plug_in_image_rot90")==0)
      )      
    alllayers = TRUE;
  else
    alllayers = FALSE;


  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  if (run_mode == RUN_NONINTERACTIVE) {
    if (n_params != 3) {
      status = STATUS_CALLING_ERROR;
    }
  }

  if (status == STATUS_SUCCESS) {
    /*  Get the specified drawable  */
    drawable = gimp_drawable_get(param[2].data.d_drawable);
    image_id = param[1].data.d_image;

    /*  Make sure that the drawable is gray or RGB or indexed  */
    if (gimp_drawable_color(drawable->id) || gimp_drawable_gray(drawable->id) || gimp_drawable_indexed(drawable->id)) {


      gimp_tile_cache_ntiles(7 + 2*(
				    drawable->width > drawable->height ?
				    ( drawable->width / gimp_tile_width() ) :
				    ( drawable->height / gimp_tile_height() )
				    )
			     );


      /******************* DO THE WORK **************/
      
      gimp_run_procedure ("gimp_undo_push_group_start", &proc_returnvals,
			  PARAM_IMAGE, image_id,
			  PARAM_END);

      if (alllayers) /* rotate whole image, including all layers */
	{
	  gimp_drawable_detach(drawable);
	  
	  /* get a list of layers for this image_id */
	  layers = gimp_image_get_layers (image_id, &nlayers);

	  for (i=0;i<nlayers;i++)
	    {
	      drawable = gimp_drawable_get (layers[i]);
	      do_layerrot(drawable, image_id, clockwise, alllayers);
	      gimp_drawable_detach(drawable);
	    }
	  g_free(layers);
	  gimp_image_resize(image_id,
			    gimp_image_height(image_id),
			    gimp_image_width(image_id),
			    0, 0);
	}
      else /* just rotate a single layer */
	{
	  do_layerrot(drawable, image_id, clockwise, alllayers);
	  gimp_drawable_detach(drawable);
	}
      
      gimp_run_procedure ("gimp_undo_push_group_end", &proc_returnvals,
			  PARAM_IMAGE, image_id,
			  PARAM_END);

      /******************* WORK FINISHED **************/
      

      if (run_mode != RUN_NONINTERACTIVE)
	gimp_displays_flush();

    } else {
      status = STATUS_EXECUTION_ERROR;
    }
  }

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}



static void do_layerrot(GDrawable *drawable,
			gint32 image_id,
			gboolean clockwise,
			gboolean alllayers)
{
  GPixelRgn rowPR, colPR;
  gint width, height, x, y;
  guchar *buffer;
  gboolean x_is_longer;
  gint xoff,yoff, newxoff,newyoff;
  gint was_preserve_transparency = FALSE;
  gint bytes;


  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  x_is_longer = (width > height);

  if (gimp_drawable_layer(drawable->id))
    {
      gimp_layer_resize(drawable->id,
			x_is_longer? width : height,
			x_is_longer? width : height,
			0, 0);
      gimp_drawable_flush (drawable);
      drawable = gimp_drawable_get(drawable->id);
    }
  else /* not a layer... probably a channel... abort operation */
    return;


  /* If 'preserve transparency' was on, we need to temporatily
     turn it off while rotating... */
  if (gimp_layer_get_preserve_transparency(drawable->id))
    {
	was_preserve_transparency = TRUE;
	gimp_layer_set_preserve_transparency(drawable->id,
					     FALSE);
    }
      

  buffer = g_malloc ((x_is_longer? width: height) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init(&rowPR, drawable, 0, 0,
		      x_is_longer? width:height,
		      x_is_longer? width:height,
		      FALSE, FALSE);
  gimp_pixel_rgn_init(&colPR, drawable, 0, 0,
		      x_is_longer? width:height,
		      x_is_longer? width:height,
		      TRUE, TRUE);

  if (clockwise)
    {
      /* CLOCKWISE */
      for (y=0; y<height; y++)
	{
	  x=(height-y)-1;
	  
	  gimp_pixel_rgn_get_row(&rowPR, buffer, 0, y, width);
	  gimp_pixel_rgn_set_col(&colPR, buffer, x, 0, width);
	}
    }
  else
    {
      /* ANTICLOCKWISE */
      for (x=0; x<width; x++)
	{
	  y=(width-x)-1;
	  
	  gimp_pixel_rgn_get_col(&rowPR, buffer, x, 0, height);
	  gimp_pixel_rgn_set_row(&colPR, buffer, 0, y, height);
	}
    }

  g_free(buffer);
  
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, 0, 0, height, width);

  gimp_layer_resize(drawable->id,
		    height,
		    width,
		    0, 0);
  drawable = gimp_drawable_get(drawable->id);

  gimp_drawable_flush (drawable);
  gimp_drawable_update (drawable->id, 0, 0, height, width);

  if (!alllayers)
    {
      /* Offset the layer so that it remains centered on the same spot */
      gimp_drawable_offsets (drawable->id,
			     &xoff,
			     &yoff);
      newxoff = xoff+(width-height)/2;
      newyoff = yoff+(height-width)/2;
      gimp_layer_set_offsets(drawable->id,
			     newxoff,
			     newyoff);
    }
  else /* alllayers */
    {
      /* Iterating through all layers... so rotate around the image
	 centre, not the layer centre. */
      gimp_drawable_offsets (drawable->id,
			     &xoff,
			     &yoff);
      if (clockwise)
	{
	  newyoff = xoff;
	  newxoff = gimp_image_height(image_id)-yoff-height;
	}
      else /* anticlockwise */
	{
	  newxoff = yoff;
	  newyoff = gimp_image_width(image_id)-xoff-width;
	}

      gimp_layer_set_offsets(drawable->id,
			     newxoff,
			     newyoff);      
    }

  /* set 'preserve transparency' if it was set when we started... */
  if (was_preserve_transparency)
    {
      gimp_layer_set_preserve_transparency(drawable->id,
					   TRUE);
    }
}
