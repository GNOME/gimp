/*
 * ZealousCrop plug-in version 1.00
 * by Adam D. Moss <adam@foxbox.org>
 * loosely based on Autocrop by Tim Newsome <drz@froody.bloke.com>
 */

/*
 * BUGS:
 *  Doesn't undo properly.
 *  Progress bar doesn't do anything yet.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Declare local functions. */
static void query (void);
static void run   (gchar    *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);
static inline gint colours_equal (guchar *col1,
				  guchar *col2,
				  gint    bytes);

static void do_zcrop (GDrawable *drawable,
		      gint32     image_id);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gint bytes;


MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure("plug_in_zealouscrop",
			 "Automagically crops unused space from the edges and middle of a picture.",
			 "",
			 "Adam D. Moss",
			 "Adam D. Moss",
			 "1997",
			 N_("<Image>/Image/Transforms/Zealous Crop"),
			 "RGB*, GRAY*, INDEXED*",
			 PROC_PLUG_IN,
			 nargs, 0,
			 args, NULL);
}

static void
run (gchar   *name,
     gint     n_params,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_id;

  INIT_I18N();

  *nreturn_vals = 1;
  *return_vals = values;

  run_mode = param[0].data.d_int32;

  if (run_mode == RUN_NONINTERACTIVE)
    {
      if (n_params != 3)
	{
	  status = STATUS_CALLING_ERROR;
	}
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Get the specified drawable  */
      drawable = gimp_drawable_get(param[2].data.d_drawable);
      image_id = param[1].data.d_image;

      /*  Make sure that the drawable is gray or RGB or indexed  */
      if (gimp_drawable_is_rgb (drawable->id) ||
	  gimp_drawable_is_gray (drawable->id) ||
	  gimp_drawable_is_indexed (drawable->id))
	{
	  gimp_progress_init (_("ZealousCropping(tm)..."));

	  gimp_tile_cache_ntiles (1 +
				  2 * (drawable->width > drawable->height ?
				       (drawable->width / gimp_tile_width()) :
				       (drawable->height / gimp_tile_height())));

	  do_zcrop(drawable, image_id);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush();

	  gimp_drawable_detach(drawable);
	}
      else
	{
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static void
do_zcrop (GDrawable *drawable,
	  gint32     image_id)
{
  GPixelRgn srcPR, destPR;
  gint    width, height, x, y;
  guchar *buffer;
  gint    nreturn_vals;
  gint8  *killrows;
  gint8  *killcols;
  gint32  livingrows, livingcols, destrow, destcol;
  gint    total_area, area;

  width = drawable->width;
  height = drawable->height;
  bytes = drawable->bpp;

  total_area = width * height * 4;
  area = 0;

  killrows = g_malloc (sizeof(gint8)*height);
  killcols = g_malloc (sizeof(gint8)*width);

  buffer = g_malloc ((width > height ? width : height) * bytes);

  /*  initialize the pixel regions  */
  gimp_pixel_rgn_init (&srcPR, drawable, 0, 0, width, height, FALSE, FALSE);
  gimp_pixel_rgn_init (&destPR, drawable, 0, 0, width, height, TRUE, TRUE);

  livingrows = 0;
  for (y=0; y<height; y++)
    {
      gimp_pixel_rgn_get_row (&srcPR, buffer, 0, y, width);

      killrows[y] = TRUE;

      for (x=0; x<width*bytes; x+=bytes)
	{
	  if (!colours_equal (buffer, &buffer[x], bytes))
	    {
	      livingrows++;
	      killrows[y] = FALSE;
	      break;
	    }
	}

      area += width;
      if (y % 20)
	gimp_progress_update ((double) area / (double) total_area);
    }


  livingcols = 0;
  for (x=0; x<width; x++)
    {
      gimp_pixel_rgn_get_col (&srcPR, buffer, x, 0, height);

      killcols[x] = TRUE;

      for (y=0; y<height*bytes; y+=bytes)
	{
	  if (!colours_equal(buffer, &buffer[y], bytes))
	    {
	      livingcols++;
	      killcols[x] = FALSE;
	      break;
	    }
	}

      area += height;
      if (x % 20)
	gimp_progress_update ((double) area / (double) total_area);
    }


  if (((livingcols==0) || (livingrows==0)) ||
      ((livingcols==width) && (livingrows==height)))
    {
      printf("ZealousCrop(tm): Nothing to be done.\n");
      return;
    }

  destrow = 0;

  for (y=0; y<height; y++)
    {
      if (!killrows[y])
	{
	  gimp_pixel_rgn_get_row(&srcPR, buffer, 0, y, width);
	  gimp_pixel_rgn_set_row(&destPR, buffer, 0, destrow, width);
	  destrow++;
	}

      area += width;
      if (y % 20)
	gimp_progress_update ((double) area / (double) total_area);
    }


  destcol = 0;
  gimp_pixel_rgn_init(&srcPR, drawable, 0, 0, width, height, FALSE, TRUE);

  for (x=0; x<width; x++)
    {
      if (!killcols[x])
	{
	  gimp_pixel_rgn_get_col(&srcPR, buffer, x, 0, height);
	  gimp_pixel_rgn_set_col(&destPR, buffer, destcol, 0, height);
	  destcol++;
	}

      area += height;
      if (x % 20)
	gimp_progress_update ((double) area / (double) total_area);
    }

/*  printf("dc: %d, lc: %d  - - - dr: %d, lr: %d\n",destcol, livingcols, destrow, livingrows);*/

    g_free(buffer);

    g_free(killrows);
    g_free(killcols);

    gimp_progress_update(1.00);

    gimp_run_procedure ("gimp_undo_push_group_start", &nreturn_vals,
			PARAM_IMAGE, image_id,
			PARAM_END);

    gimp_drawable_flush (drawable);
    gimp_drawable_merge_shadow (drawable->id, TRUE);

    gimp_run_procedure("gimp_crop", &nreturn_vals,
		       PARAM_IMAGE, image_id,
		       PARAM_INT32, (gint32)livingcols,
		       PARAM_INT32, (gint32)livingrows,
		       PARAM_INT32, 0,
		       PARAM_INT32, 0,
		       PARAM_END);

    gimp_run_procedure ("gimp_undo_push_group_end", &nreturn_vals,
			PARAM_IMAGE, image_id,
			PARAM_END);
}


static inline int colours_equal(guchar *col1, guchar *col2, int bytes)
{
  int b;

  for (b = 0; b < bytes; b++)
    {
      if (col1[b] != col2[b])
	{
	  return (FALSE);
	  break;
	}
    }

  return TRUE;
}
