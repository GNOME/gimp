/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Film plug-in (C) 1997 Peter Kirchgessner
 * e-mail: pkirchg@aol.com, WWW: http://members.aol.com/pkirchg
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
 * This plug-in generates a film roll with several images
 */

/* Event history:
 * V 1.00, PK, 01-Jul-97, Creation
 * V 1.01, PK, 24-Jul-97, Fix problem with previews on Irix
 * V 1.02, PK, 24-Sep-97, Try different font sizes when inquire failed.
 *                        Fit film height to images
 * V 1.03, nn, 20-Dec-97, Initialize layers in film()
 * V 1.04, PK, 08-Oct-99, Fix problem with image id zero
 *                        Internationalization
 */
static char ident[] = "@(#) GIMP Film plug-in v1.04 1999-10-08";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Maximum number of pictures per film */
#define MAX_FILM_PICTURES   64
#define COLOR_BUTTON_WIDTH  50
#define COLOR_BUTTON_HEIGHT 20

/* Define how the plug-in works. Values marked (r) are with regard */
/* to film_height (i.e. it should be a value from 0.0 to 1.0) */
typedef struct
{
  gint     film_height;           /* height of the film */
  guchar   film_color[3];         /* color of film */
  gdouble  picture_height;        /* height of picture (r) */
  gdouble  picture_space;         /* space between pictures (r) */
  gdouble  hole_offset;           /* distance from hole to edge of film (r) */
  gdouble  hole_width;            /* width of hole (r) */
  gdouble  hole_height;           /* height of holes (r) */
  gdouble  hole_space;            /* distance of holes (r) */
  gdouble  number_height;         /* height of picture numbering (r) */
  gint     number_start;          /* number for first picture */
  guchar   number_color[3];       /* color of number */
  gchar    number_fontf[256];     /* font family to use for numbering */
  gint     number_pos[2];         /* flags where to draw numbers (top/bottom) */
  gint     keep_height;           /* flag if to keep max. image height */
  gint     num_images;            /* number of images */
  gint32   image[MAX_FILM_PICTURES]; /* list of image IDs */
} FilmVals;

/* Data to use for the dialog */
typedef struct
{
  GtkWidget *font_entry;
  GtkObject *advanced_adj[7];
  GtkWidget *image_list_all;
  GtkWidget *image_list_film;
  gint       run;
} FilmInterface;


/* Declare local functions
 */
static void      query  (void);
static void      run    (gchar     *name,
			 gint       nparams,
			 GParam    *param,
			 gint      *nreturn_vals,
			 GParam   **return_vals);


static gint32    create_new_image   (gchar          *filename,
				     guint           width,
				     guint           height,
				     GDrawableType   gdtype,
				     gint32         *layer_ID,
				     GDrawable     **drawable,
				     GPixelRgn      *pixel_rgn);

static gchar   * compose_image_name (gint32          image_ID);

static gint32    film               (void);

static gint      check_filmvals     (void);

static void      convert_to_rgb     (GDrawable      *srcdrawable,
				     gint            numpix,
				     guchar         *src,
				     guchar         *dst);

static void      set_pixels         (gint            numpix,
				     guchar         *dst,
				     guchar         *rgb);

static gint      scale_layer        (gint32          src_layer,
				     gint            src_x0,
				     gint            src_y0,
				     gint            src_width,
				     gint            src_height,
				     gint32          dst_layer,
				     gint            dst_x0,
				     gint            dst_y0,
				     gint            dst_width,
				     gint            dst_height);

static guchar  * create_hole_rgb    (gint            width,
				     gint            height);

static void      draw_hole_rgb      (GDrawable      *drw,
				     gint            x,
				     gint            y,
				     gint            width,
				     gint            height,
				     guchar         *hole);

static void      draw_number        (gint32          layer_ID,
				     gint            num,
				     gint            x,
				     gint            y,
				     gint            height);


static void        add_list_item_callback (GtkWidget *widget,
					   GtkWidget *list);
static void        del_list_item_callback (GtkWidget *widget,
					   GtkWidget *list);

static GtkWidget * add_image_list         (gint       add_box_flag,
					   gint       n,
					   gint32    *image_id,
					   GtkWidget *hbox);

static gint        film_dialog            (gint32     image_ID);
static void        film_ok_callback       (GtkWidget *widget,
					   gpointer   data);
static void        film_reset_callback    (GtkWidget *widget,
					   gpointer   data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static gdouble advanced_defaults[] =
{
  0.695,           /* Picture height */
  0.040,           /* Picture spacing */
  0.058,           /* Hole offset to edge of film */
  0.052,           /* Hole width */
  0.081,           /* Hole height */
  0.081,           /* Hole distance */
  0.052            /* Image number height */
};

static FilmVals filmvals =
{
  256,             /* Height of film */
  { 0, 0, 0 },     /* Color of film */
  0.695,           /* Picture height */
  0.040,           /* Picture spacing */
  0.058,           /* Hole offset to edge of film */
  0.052,           /* Hole width */
  0.081,           /* Hole height */
  0.081,           /* Hole distance */
  0.052,           /* Image number height */
  1,               /* Start index of numbering */
  { 239, 159, 0 }, /* Color of number */
  "courier",       /* Font family for numbering */
  { TRUE, TRUE },  /* Numbering on top and bottom */
  0,               /* Dont keep max. image height */
  0,               /* Number of images */
  { 0 }            /* Input image list */
};


static FilmInterface filmint =
{
  NULL,       /* font entry */
  { NULL },   /* advanced adjustments */
  NULL, NULL, /* image list widgets */
  FALSE       /* run */
};


static GRunModeType run_mode;


MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image (only used as default image in interactive mode)" },
    { PARAM_DRAWABLE, "drawable", "Input drawable (not used)" },
    { PARAM_INT32, "film_height", "Height of film (0: fit to images)" },
    { PARAM_COLOR, "film_color", "Color of the film" },
    { PARAM_INT32, "number_start", "Start index for numbering" },
    { PARAM_STRING, "number_fontf", "Font family for drawing numbers" },
    { PARAM_COLOR, "number_color", "Color for numbers" },
    { PARAM_INT32, "at_top", "Flag for drawing numbers at top of film" },
    { PARAM_INT32, "at_bottom", "Flag for drawing numbers at bottom of film" },
    { PARAM_INT32, "num_images", "Number of images to be used for film" },
    { PARAM_INT32ARRAY, "image_ids", "num_images image IDs to be used for film"}
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  static GParamDef return_vals[] =
  {
    { PARAM_IMAGE, "new_image", "Output image" }
  };
  static gint nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

  gimp_install_procedure ("plug_in_film",
			  "Compose several images to a roll film",
			  "Compose several images to a roll film",
			  "Peter Kirchgessner",
			  "Peter Kirchgessner (peter@kirchgessner.net)",
			  "1997",
			  N_("<Image>/Filters/Combine/Film..."),
			  "INDEXED*, GRAY*, RGB*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;
  gint k;

  INIT_I18N_UI();

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 2;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
  values[1].type = PARAM_IMAGE;
  values[1].data.d_int32 = -1;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_film", &filmvals);

      /*  First acquire information with a dialog  */
      if (! film_dialog (param[1].data.d_int32))
	return;
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      /* Also we want to have some images to compose */
      if ((nparams != 12) || (param[10].data.d_int32 < 1))
	{
	  status = STATUS_CALLING_ERROR;
	}
      else
	{
          filmvals.keep_height       = (param[3].data.d_int32 <= 0);
          filmvals.film_height       = (filmvals.keep_height ?
					128 : param[3].data.d_int32);
          filmvals.film_color[0]     = param[4].data.d_color.red;
          filmvals.film_color[1]     = param[4].data.d_color.green;
          filmvals.film_color[2]     = param[4].data.d_color.blue;
          filmvals.number_start      = param[5].data.d_int32;
          k = sizeof (filmvals.number_fontf);
          strncpy (filmvals.number_fontf, param[6].data.d_string, k);
          filmvals.number_fontf[k-1] = '\0';
          filmvals.number_color[0]   = param[7].data.d_color.red;
          filmvals.number_color[1]   = param[7].data.d_color.green;
          filmvals.number_color[2]   = param[7].data.d_color.blue;
          filmvals.number_pos[0]     = param[8].data.d_int32;
          filmvals.number_pos[1]     = param[9].data.d_int32;
          filmvals.num_images        = param[10].data.d_int32;
          if (filmvals.num_images > MAX_FILM_PICTURES)
            filmvals.num_images = MAX_FILM_PICTURES;
          for (k = 0; k < filmvals.num_images; k++)
            filmvals.image[k] = param[11].data.d_int32array[k];
	}
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_film", &filmvals);
      break;

    default:
      break;
    }

  if (check_filmvals () < 0)
    status = STATUS_CALLING_ERROR;

  if (status == STATUS_SUCCESS)
    {
      if (run_mode != RUN_NONINTERACTIVE)
        gimp_progress_init (_("Composing Images..."));

      image_ID = film ();

      if (image_ID < 0)
	{
	  status = STATUS_EXECUTION_ERROR;
	}
      else
	{
	  values[1].data.d_int32 = image_ID;
	  gimp_image_undo_enable (image_ID);
	  gimp_image_clean_all (image_ID);
	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_display_new (image_ID);
	}

      /*  Store data  */
      if (run_mode == RUN_INTERACTIVE)
        gimp_set_data ("plug_in_film", &filmvals, sizeof (FilmVals));
    }

  values[0].data.d_status = status;
}


/* Compose a roll film image from several images */
static gint32
film (void)
{
  gint width, height, tile_height, scan_lines;
  guchar *dst, *hole;
  gint film_height, film_width;
  gint picture_width, picture_height, picture_space, picture_x0, picture_y0;
  gint hole_offset, hole_width, hole_height, hole_space, hole_x;
  gint number_height, num_images, num_pictures;
  gint i, j, k, picture_count;
  gdouble f;
  guchar f_red, f_green, f_blue;
  gint nreturn_vals, num_layers;
  gint32 *image_ID_src, image_ID_dst, layer_ID_src, layer_ID_dst;
  gint32 *layers;
  GDrawable *drawable_dst;
  GPixelRgn pixel_rgn_dst;

  /* initialize */

  layers = NULL;

  num_images = filmvals.num_images;
  image_ID_src = filmvals.image;

  if (num_images <= 0)
    return (-1);

  tile_height = gimp_tile_height ();
  /* Save foreground colour */
  gimp_palette_get_foreground (&f_red, &f_green, &f_blue);

  if (filmvals.keep_height) /* Search maximum picture height */
    {
      picture_height = 0;
      for (j = 0; j < num_images; j++)
	{
	  height = gimp_image_height (image_ID_src[j]);
	  if (height > picture_height) picture_height = height;
	}
      film_height = (int)(picture_height / filmvals.picture_height + 0.5);
      filmvals.film_height = film_height;
    }
  else
    {
      film_height = filmvals.film_height;
      picture_height = (int)(film_height * filmvals.picture_height + 0.5);
    }
  picture_space = (int)(film_height * filmvals.picture_space + 0.5);
  picture_y0 = (film_height - picture_height)/2;

  number_height = film_height * filmvals.number_height;

  /* Calculate total film width */
  film_width = 0;
  num_pictures = 0;
  for (j = 0; j < num_images; j++)
    {
      layers = gimp_image_get_layers (image_ID_src[j], &num_layers);
      /* Get scaled image size */
      width = gimp_image_width (image_ID_src[j]);
      height = gimp_image_height (image_ID_src[j]);
      f = ((double)picture_height) / (double)height;
      picture_width = width * f;

      for (k = 0; k < num_layers; k++)
	{
	  if (gimp_layer_is_floating_selection (layers[k]))
	    continue;

	  film_width += (picture_space/2);  /* Leading space */
	  film_width += picture_width;      /* Scaled image width */
	  film_width += (picture_space/2);  /* Trailing space */
	  num_pictures++;
	}

      if (layers)
	g_free (layers);
    }
#ifdef FILM_DEBUG
  printf ("film_height = %d, film_width = %d\n", film_height, film_width);
  printf ("picture_height = %d, picture_space = %d, picture_y0 = %d\n",
	  picture_height, picture_space, picture_y0);
  printf ("Number of pictures = %d\n", num_pictures);
#endif

  image_ID_dst = create_new_image (_("Untitled"),
				   (guint) film_width, (guint) film_height,
				   RGB_IMAGE, &layer_ID_dst,
				   &drawable_dst, &pixel_rgn_dst);

  dst = g_new (guchar, film_width * tile_height * 3);

  /* Fill film background */
  i = 0;
  while (i < film_height)
    {
      scan_lines =
	(i + tile_height - 1 < film_height) ? tile_height : (film_height - i);

      set_pixels (film_width * scan_lines, dst, filmvals.film_color);

      gimp_pixel_rgn_set_rect (&pixel_rgn_dst, dst, 0, i,
			       film_width, scan_lines);
      i += scan_lines;
    }
  g_free (dst);

  /* Draw all the holes */
  hole_offset = film_height * filmvals.hole_offset;
  hole_width = film_height * filmvals.hole_width;
  hole_height = film_height * filmvals.hole_height;
  hole_space = film_height * filmvals.hole_space;
  hole_x = hole_space / 2;

#ifdef FILM_DEBUG
  printf ("hole_x %d hole_offset %d hole_width %d hole_height %d hole_space %d\n",
	  hole_x, hole_offset, hole_width, hole_height, hole_space );
#endif

  hole = create_hole_rgb (hole_width, hole_height);
  if (hole)
    {
      while (hole_x < film_width)
	{
	  draw_hole_rgb (drawable_dst, hole_x,
			 hole_offset,
			 hole_width, hole_height, hole);
	  draw_hole_rgb (drawable_dst, hole_x,
			 film_height-hole_offset-hole_height,
			 hole_width, hole_height, hole);

	  hole_x += hole_width + hole_space;
	}
      g_free (hole);
    }
  gimp_drawable_detach (drawable_dst);

  /* Compose all images and layers */
  picture_x0 = 0;
  picture_count = 0;
  for (j = 0; j < num_images; j++)
    {
      width = gimp_image_width (image_ID_src[j]);
      height = gimp_image_height (image_ID_src[j]);
      f = ((gdouble) picture_height) / (gdouble) height;
      picture_width = width * f;

      layers = gimp_image_get_layers (image_ID_src[j], &num_layers);

      for (k = 0; k < num_layers; k++)
	{
	  if (gimp_layer_is_floating_selection (layers[k]))
	    continue;

	  picture_x0 += (picture_space/2);

	  layer_ID_src = layers[k];
	  /* Scale the layer and insert int new image */
	  if (scale_layer (layer_ID_src, 0, 0, width, height,
			   layer_ID_dst, picture_x0, picture_y0,
			   picture_width, picture_height) < 0)
	    {
	      printf ("film: error during scale_layer\n");
	      return -1;
	    }

	  /* Draw picture numbers */
	  if ((number_height > 0) &&
	      (filmvals.number_pos[0] || filmvals.number_pos[1]))
	    {
	      gimp_palette_set_foreground (filmvals.number_color[0],
					   filmvals.number_color[1],
					   filmvals.number_color[2]);

	      if (filmvals.number_pos[0])
		draw_number (layer_ID_dst, filmvals.number_start + picture_count,
			     picture_x0 + picture_width/2,
			     (hole_offset-number_height)/2, number_height);
	      if (filmvals.number_pos[1])
		draw_number (layer_ID_dst, filmvals.number_start + picture_count,
			     picture_x0 + picture_width/2,
			     film_height - (hole_offset + number_height)/2,
			     number_height);

	      gimp_palette_set_foreground (f_red, f_green, f_blue);
	    }

	  picture_x0 += picture_width + (picture_space/2);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_progress_update (((gdouble) (picture_count + 1)) /
				  (gdouble) num_pictures);

	  picture_count++;
	}
    }

  if (layers)
    g_free (layers);

  /* Drawing text/numbers leaves us with a floating selection. Stop it */
  gimp_run_procedure ("gimp_floating_sel_anchor", &nreturn_vals,
		      PARAM_LAYER, gimp_image_floating_selection (image_ID_dst),
		      PARAM_END);
  /* Restore foreground */
  gimp_palette_set_foreground (f_red, f_green, f_blue);

  return image_ID_dst;
}


/* Check filmvals. Unreasonable values are reset to a default. */
/* If this is not possible, -1 is returned. Otherwise 0 is returned. */
static gint
check_filmvals (void)
{
  if (filmvals.film_height < 10)
    filmvals.film_height = 10;

  if (filmvals.number_start < 0)
    filmvals.number_start = 0;

  if (filmvals.number_fontf[0] == '\0')
    strcpy (filmvals.number_fontf, "courier");

  if (filmvals.num_images < 1)
    return -1;

  return 0;
}


/* Converts numpix pixels from src to RGB at dst */
static void
convert_to_rgb (GDrawable *srcdrawable,
		gint       numpix,
		guchar    *src,
		guchar    *dst)

{
 register guchar *from = src, *to = dst;
 register gint k;
 register guchar *cmap, *colour;
 gint ncols;

 switch (gimp_drawable_type (srcdrawable->id))
   {
   case RGB_IMAGE:
     memcpy ((char *)dst, (char *)src, numpix*3);
     break;

   case RGBA_IMAGE:
     from = src;
     to = dst;
     k = numpix;
     while (k-- > 0)
       {
	 *(to++) = *(from++); *(to++) = *(from++); *(to++) = *(from++);
	 from++;
       }
     break;

   case GRAY_IMAGE:
     from = src;
     to = dst;
     k = numpix;
     while (k-- > 0)
       {
	 to[0] = to[1] = to[2] = *(from++);
	 to += 3;
       }
     break;

   case GRAYA_IMAGE:
     from = src;
     to = dst;
     k = numpix;
     while (k-- > 0)
       {
	 to[0] = to[1] = to[2] = *from;
	 from += 2;
	 to += 3;
       }
     break;

   case INDEXED_IMAGE:
   case INDEXEDA_IMAGE:
     cmap = gimp_image_get_cmap (gimp_drawable_image_id (srcdrawable->id),
                                 &ncols);
     if (cmap)
       {
	 from = src;
	 to = dst;
	 k = numpix;
	 while (k-- > 0)
	   {
	     colour = cmap + 3*(int)(*from);
	     *(to++) = *(colour++);
	     *(to++) = *(colour++);
	     *(to++) = *(colour++);
	     from += srcdrawable->bpp;
	   }
       }
     break;

   default:
     printf ("convert_to_rgb: unknown image type\n");
     break;
 }
}


/* Assigns numpix pixels starting at dst with color r,g,b */
static void
set_pixels (gint    numpix,
            guchar *dst,
            guchar *rgb)
{
  register gint k;
  register guchar ur, ug, ub, *udest;

  if ((rgb[0] == rgb[1]) && (rgb[1] == rgb[2]))
    {
      memset (dst, (int)rgb[0], numpix*3);
      return;
    }

  ur = rgb[0];
  ug = rgb[1];
  ub = rgb[2];
  k = numpix;
  udest = dst;
  while (k-- > 0)
    {
      *(udest++) = ur;
      *(udest++) = ug;
      *(udest++) = ub;
    }
}


/* Scales a portion of a layer and places it in another layer. */
/* On success, 0 is returned. Otherwise -1 is returned */
static gint
scale_layer (gint32  src_layer,
	     gint    src_x0,
	     gint    src_y0,
	     gint    src_width,
	     gint    src_height,
	     gint32  dst_layer,
	     gint    dst_x0,
	     gint    dst_y0,
	     gint    dst_width,
	     gint    dst_height)
{
  gint tile_height, i, scan_lines, numpix;
  guchar *src, *tmp = (guchar *) ident; /* Just to satisfy gcc */
  gint32 tmp_image, tmp_layer;
  GDrawable *tmp_drawable, *src_drawable, *dst_drawable;
  GPixelRgn tmp_pixel_rgn, src_pixel_rgn, dst_pixel_rgn;

  tile_height = gimp_tile_height ();

  src_drawable = gimp_drawable_get (src_layer);

  /*** Get a RGB copy of the source region ***/

  tmp_image = create_new_image (_("Temporary"), src_width, src_height,
				RGB_IMAGE,
				&tmp_layer, &tmp_drawable, &tmp_pixel_rgn);

  src = g_new (guchar, src_width * tile_height * src_drawable->bpp);
  tmp = g_new (guchar, src_width * tile_height * tmp_drawable->bpp);

  gimp_pixel_rgn_init (&src_pixel_rgn, src_drawable, src_x0, src_y0,
		       src_width, src_height, FALSE, FALSE);

  i = 0;
  while (i < src_height)
    {
      scan_lines = (i+tile_height-1 < src_height) ? tile_height : (src_height-i);
      numpix = scan_lines*src_width;

      /* Get part of source image */
      gimp_pixel_rgn_get_rect (&src_pixel_rgn, src, src_x0, src_y0+i,
			       src_width, scan_lines);

      convert_to_rgb (src_drawable, numpix, src, tmp);

      /* Set part of temporary image */
      gimp_pixel_rgn_set_rect (&tmp_pixel_rgn, tmp, 0, i, src_width, scan_lines);

      i += scan_lines;
    }

  /* Now we have an image that is a copy of the */
  /* source region and has been converted to RGB. */
  /* We dont need any more access to the source image. */
  gimp_drawable_detach (src_drawable);
  g_free (src);

  /*** Resize the temporary image if necessary ***/
  if ((src_width != dst_width) || (src_height != dst_height))
    {
      gimp_drawable_detach (tmp_drawable);

      gimp_layer_scale (tmp_layer, dst_width, dst_height, 0);

      tmp_drawable = gimp_drawable_get (tmp_layer);
    }

  /*** Copy temporary image to destination image */
  gimp_pixel_rgn_init (&tmp_pixel_rgn, tmp_drawable, 0, 0,
		       dst_width, dst_height, FALSE, FALSE);
  g_free (tmp);

  tmp = g_new (guchar, dst_width * tile_height * tmp_drawable->bpp);

  dst_drawable = gimp_drawable_get (dst_layer);
  gimp_pixel_rgn_init (&dst_pixel_rgn, dst_drawable, dst_x0, dst_y0,
		       dst_width, dst_height, TRUE, FALSE);
  i = 0;
  while (i < dst_height)
    {
      scan_lines = (i+tile_height-1 < dst_height) ? tile_height : (dst_height-i);

      /* Get strip of temporary image */
      gimp_pixel_rgn_get_rect (&tmp_pixel_rgn, tmp, 0, i,
			       dst_width, scan_lines);

      /* Set strip of destination image */
      gimp_pixel_rgn_set_rect (&dst_pixel_rgn, tmp, dst_x0, dst_y0+i,
			       dst_width, scan_lines);
      i += scan_lines;
    }

  /* No more access to the temporary image */
  gimp_drawable_detach (tmp_drawable);
  g_free (tmp);
  gimp_image_delete (tmp_image);

  gimp_drawable_detach (dst_drawable);

  return 0;
}


/* Create the RGB-pixels that make up the hole */
static guchar *
create_hole_rgb (gint width,
		 gint height)
{
  guchar *hole, *top, *bottom;
  gint radius, length, k;

  hole = g_new (guchar, width * height * 3);

  /* Fill a rectangle with white */
  memset (hole, 255, width * height * 3);
  radius = height / 4;
  if (radius > width / 2)
    radius = width / 2;
  top = hole;
  bottom = hole + (height-1)*width*3;
  for (k = radius-1; k > 0; k--)  /* Rounding corners */
    {
      length = (int)(radius - sqrt ((gdouble) (radius * radius - k * k)) - 0.5);
      if (length > 0)
	{
	  set_pixels (length, top, filmvals.film_color);
	  set_pixels (length, top+(width-length)*3, filmvals.film_color);
	  set_pixels (length, bottom, filmvals.film_color);
	  set_pixels (length, bottom+(width-length)*3, filmvals.film_color);
	}
      top += width*3;
      bottom -= width*3;
    }

  return hole;
}


/* Draw the hole at the specified position */
static void
draw_hole_rgb (GDrawable *drw,
               gint       x,
               gint       y,
               gint       width,
               gint       height,
               guchar    *hole)
{
  GPixelRgn rgn;
  guchar *data;
  gint tile_height = gimp_tile_height ();
  gint i, j, scan_lines, d_width = gimp_drawable_width (drw->id);
  gint length;

  if ((width <= 0) || (height <= 0))
    return;
  if ((x+width <= 0) || (x >= d_width))
    return;
  length = width;   /* Check that we dont draw past the image */
  if ((x+length) >= d_width)
    length = d_width-x;

  data = g_new (guchar, length * tile_height * drw->bpp);

  gimp_pixel_rgn_init (&rgn, drw, x, y, length, height, TRUE, FALSE);

  i = 0;
  while (i < height)
    {
      scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i);
      if (length == width)
	{
	  memcpy (data, hole + 3*width*i, width*scan_lines*3);
	}
      else  /* We have to do some clipping */
	{
	  for (j = 0; j < scan_lines; j++)
	    memcpy (data + j*length*3, hole + (i+j)*width*3, length*3);
	}
      gimp_pixel_rgn_set_rect (&rgn, data, x, y+i, length, scan_lines);

      i += scan_lines;
    }

  g_free (data);
}


/* Draw the number of the picture onto the film */
static void
draw_number (gint32 layer_ID,
             gint   num,
             gint   x,
             gint   y,
             gint   height)
{
  gchar buf[32];
  GDrawable *drw;
  GParam *params = NULL;
  gint nreturn_vals, k, delta, max_delta;
  gint32 image_ID, descent;
  gchar *family = filmvals.number_fontf;

  g_snprintf (buf, sizeof (buf), "%d", num);

  drw = gimp_drawable_get (layer_ID);
  image_ID = gimp_drawable_image_id (layer_ID);

  max_delta = height / 10;
  if (max_delta < 1)
    max_delta = 1;

  /* Numbers dont need the descent. Inquire it and move the text down */
  for (k = 0; k < max_delta*2 + 1; k++)
    { /* Try different font sizes if inquire of extent failed */
      delta = (k+1) / 2;
      if ((k & 1) == 0) delta = -delta;
      params = gimp_run_procedure ("gimp_text_get_extents", &nreturn_vals,
				   PARAM_STRING, buf,
				   PARAM_FLOAT, (gfloat)(height+delta),
				   PARAM_INT32, (gint32)0,  /* use pixelsize */
				   PARAM_STRING, "*",       /* foundry */
				   PARAM_STRING, family,    /* family */
				   PARAM_STRING, "*",       /* weight */
				   PARAM_STRING, "*",       /* slant */
				   PARAM_STRING, "*",       /* set_width */
				   PARAM_STRING, "*",       /* spacing */
				   PARAM_STRING, "*",
				   PARAM_STRING, "*",
				   PARAM_END);

      if (params[0].data.d_status == STATUS_SUCCESS)
	{
	  height += delta;
	  break;
	}
    }

  if (params[0].data.d_status == STATUS_SUCCESS)
    descent = params[4].data.d_int32;
  else
    descent = 0;

  params = gimp_run_procedure ("gimp_text", &nreturn_vals,
			       PARAM_IMAGE, image_ID,
			       PARAM_DRAWABLE, layer_ID,
			       PARAM_FLOAT, (gfloat)x,
			       PARAM_FLOAT, (gfloat)(y+descent/2),
			       PARAM_STRING, buf,
			       PARAM_INT32, (gint32)1,  /* border */
			       PARAM_INT32, (gint32)0,  /* antialias */
			       PARAM_FLOAT, (gfloat)height,
			       PARAM_INT32, (gint32)0,  /* use pixelsize */
			       PARAM_STRING, "*",       /* foundry */
			       PARAM_STRING, family,    /* family */
			       PARAM_STRING, "*",       /* weight */
			       PARAM_STRING, "*",       /* slant */
			       PARAM_STRING, "*",       /* set_width */
			       PARAM_STRING, "*",       /* spacing */
			       PARAM_STRING, "*",
			       PARAM_STRING, "*",
			       PARAM_END);

  if (params[0].data.d_status != STATUS_SUCCESS)
    printf ("draw_number: Error in drawing text\n");

  gimp_drawable_detach (drw);
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (gchar          *filename,
                  guint           width,
                  guint           height,
                  GDrawableType   gdtype,
                  gint32         *layer_ID,
                  GDrawable     **drawable,
                  GPixelRgn       *pixel_rgn)
{
  gint32 image_ID;
  GImageType gitype;

  if ((gdtype == GRAY_IMAGE) || (gdtype == GRAYA_IMAGE))
    gitype = GRAY;
  else if ((gdtype == INDEXED_IMAGE) || (gdtype == INDEXEDA_IMAGE))
    gitype = INDEXED;
  else
    gitype = RGB;

  image_ID = gimp_image_new (width, height, gitype);
  gimp_image_set_filename (image_ID, filename);

  *layer_ID = gimp_layer_new (image_ID, _("Background"), width, height,
			      gdtype, 100, NORMAL_MODE);
  gimp_image_add_layer (image_ID, *layer_ID, 0);

  if (drawable != NULL)
    {
      *drawable = gimp_drawable_get (*layer_ID);
      if (pixel_rgn != NULL)
	gimp_pixel_rgn_init (pixel_rgn, *drawable, 0, 0, (*drawable)->width,
			     (*drawable)->height, TRUE, FALSE);
    }

  return image_ID;
}


static gchar *
compose_image_name (gint32 image_ID)
{
  static gchar buffer[256];
  gchar *filename, *basename;

  filename = gimp_image_get_filename (image_ID);
  if (filename == NULL)
    return "";

  /* Compose a name of the basename and the image-ID */
  basename = strrchr (filename, '/');
  if (basename == NULL)
    basename = filename;
  else
    basename++;

  g_snprintf (buffer, sizeof (buffer), "%s-%ld", basename, (long)image_ID);

  g_free (filename);

  return buffer;
}


static void
add_list_item_callback (GtkWidget *widget,
                        GtkWidget *list)
{
  GList     *tmp_list;
  GtkWidget *label;
  GtkWidget *list_item;
  gint32     image_ID;

  tmp_list = GTK_LIST (list)->selection;

  while (tmp_list)
    {
      if ((label = (GtkWidget *) tmp_list->data) != NULL)
	{
	  image_ID = (gint32)gtk_object_get_user_data (GTK_OBJECT (label));
	  list_item =
	    gtk_list_item_new_with_label (compose_image_name (image_ID));

	  gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer)image_ID);
	  gtk_container_add (GTK_CONTAINER (filmint.image_list_film), list_item);
	  gtk_widget_show (list_item);
	}
      tmp_list = tmp_list->next;
    }
}


static void
del_list_item_callback (GtkWidget *widget,
                        GtkWidget *list)
{
  GList *tmp_list;
  GList *clear_list;

  tmp_list = GTK_LIST (list)->selection;
  clear_list = NULL;

  while (tmp_list)
    {
      clear_list = g_list_prepend (clear_list, tmp_list->data);
      tmp_list = tmp_list->next;
    }

  clear_list = g_list_reverse (clear_list);

  gtk_list_remove_items (GTK_LIST (list), clear_list);

  g_list_free (clear_list);
}


static GtkWidget *
add_image_list (gint       add_box_flag,
                gint       n,
                gint32    *image_id,
                GtkWidget *hbox)

{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *scrolled_win;
  GtkWidget *list;
  GtkWidget *list_item;
  GtkWidget *button;
  gint       i;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (add_box_flag ? _("Available Images:")
                                      : _("On Film:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win),
					 list);
  gtk_widget_show (list);

  for (i = 0; i < n; i++)
    {
      list_item =
	gtk_list_item_new_with_label (compose_image_name (image_id[i]));

      gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer) image_id[i]);
      gtk_container_add (GTK_CONTAINER (list), list_item);
      gtk_widget_show (list_item);
    }

  button = gtk_button_new_with_label (add_box_flag ? _("Add >>"):_("Remove"));
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      add_box_flag ? (GtkSignalFunc) add_list_item_callback
                                   : (GtkSignalFunc) del_list_item_callback,
                      list);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  return list;
}


static gint
film_dialog (gint32 image_ID)

{
  GtkWidget *dlg;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *toggle;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *sep;
  gint32 *image_id_list;
  gint    nimages, j, row;

  gimp_ui_init ("film", TRUE);

  dlg = gimp_dialog_new (_("Film"), "film",
			 gimp_standard_help_func, "filters/film.html",
			 GTK_WIN_POS_NONE,
			 FALSE, TRUE, FALSE,

			 _("OK"), film_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
                      GTK_SIGNAL_FUNC (gtk_main_quit),
                      NULL);

  main_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), main_vbox,
		      TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /*** The frames on the left keep film options ***/

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_show (vbox2);

  /* Film height/colour */
  frame = gtk_frame_new (_("Film"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Keep maximum image height */
  toggle = gtk_check_button_new_with_label (_("Fit Height to Images"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
                      GTK_SIGNAL_FUNC (gimp_toggle_button_update),
                      &filmvals.keep_height);
  gtk_widget_show (toggle);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Film height */
  spinbutton = gimp_spin_button_new (&adj, filmvals.film_height, 10,
				     GIMP_MAX_IMAGE_SIZE, 1, 10, 0, 1, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &filmvals.film_height);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Height:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  gtk_object_set_data (GTK_OBJECT (toggle), "inverse_sensitive", spinbutton);
  gtk_object_set_data
    (GTK_OBJECT (spinbutton), "inverse_sensitive",
     /* FIXME: eeeeeek */
     g_list_nth_data (gtk_container_children (GTK_CONTAINER (table)), 1));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				filmvals.keep_height);

  /* Film color */
  button = gimp_color_button_new (_("Select Film Color"),
				  COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT,
				  filmvals.film_color, 3);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Color:"), 1.0, 0.5,
			     button, 1, TRUE);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Film numbering: Startindex/Font/colour */
  frame = gtk_frame_new (_("Numbering"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* Startindex */
  spinbutton = gimp_spin_button_new (&adj, filmvals.number_start, 0,
				     G_MAXINT, 1, 10, 0, 1, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &filmvals.number_start);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Start Index:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);

  /* Fontfamily for numbering */
  filmint.font_entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 60, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), filmvals.number_fontf);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Font:"), 1.0, 0.5,
			     entry, 1, FALSE);

  /* Numbering color */
  button = gimp_color_button_new (_("Select Number Color"),
				  COLOR_BUTTON_WIDTH, COLOR_BUTTON_HEIGHT,
				  filmvals.number_color, 3);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Color:"), 1.0, 0.5,
			     button, 1, TRUE);

  for (j = 0; j < 2; j++)
    {
      toggle =
	gtk_check_button_new_with_label (j ? _("At Bottom") : _("At Top"));
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			  GTK_SIGNAL_FUNC (gimp_toggle_button_update),
			  &(filmvals.number_pos[j]));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				    filmvals.number_pos[j]);
      gtk_widget_show (toggle);
    }

  gtk_widget_show (vbox);
  gtk_widget_show (frame);
  gtk_widget_show (vbox2);

  /*** The right frame keeps the image selection ***/
  frame = gtk_frame_new (_("Image Selection"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (TRUE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  /* Get a list of all image names */
  image_id_list = gimp_query_images (&nimages);
  filmint.image_list_all = add_image_list (1, nimages, image_id_list, hbox);

  /* Get a list of the images used for the film */
  filmint.image_list_film = add_image_list (0, 1, &image_ID, hbox);

  gtk_widget_show (frame);
  gtk_widget_show (hbox);

  frame = gtk_frame_new (_("Advanced Settings (All Values are Fractions "
			   "of the Film Height)"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (11, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  row = 0;

  filmint.advanced_adj[0] = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			  _("Image Height:"), 1.0, 0.5,
			  filmvals.picture_height,
			  0.0, 1.0, 0.001, 0.01, 3,
			  TRUE, 0, 0,
			  NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &filmvals.picture_height);

  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 3);

  filmint.advanced_adj[1] = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			  _("Image Spacing:"), 1.0, 0.5,
			  filmvals.picture_space,
			  0.0, 1.0, 0.001, 0.01, 3,
			  TRUE, 0, 0,
			  NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &filmvals.picture_space);

  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 3);

  sep = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (table), sep, 0, 3, row, row + 1,
		    GTK_FILL, 0, 0, 2);
  gtk_widget_show (sep);

  row++;

  filmint.advanced_adj[2] = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			  _("Hole Offset:"), 1.0, 0.5,
			  filmvals.hole_offset,
			  0.0, 1.0, 0.001, 0.01, 3,
			  TRUE, 0, 0,
			  NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &filmvals.hole_offset);

  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 3);

  filmint.advanced_adj[3] = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			  _("Hole Width:"), 1.0, 0.5,
			  filmvals.hole_width,
			  0.0, 1.0, 0.001, 0.01, 3,
			  TRUE, 0, 0,
			  NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &filmvals.hole_width);

  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 3);

  filmint.advanced_adj[4] = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			  _("Hole Height:"), 1.0, 0.5,
			  filmvals.hole_height,
			  0.0, 1.0, 0.001, 0.01, 3,
			  TRUE, 0, 0,
			  NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &filmvals.hole_height);

  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 3);

  filmint.advanced_adj[5] = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			  _("Hole Spacing:"), 1.0, 0.5,
			  filmvals.hole_space,
			  0.0, 1.0, 0.001, 0.01, 3,
			  TRUE, 0, 0,
			  NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &filmvals.hole_space);

  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 3);

  sep = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (table), sep, 0, 3, row, row + 1,
		    GTK_FILL, 0, 0, 2);
  gtk_widget_show (sep);

  row++;

  filmint.advanced_adj[6] = adj =
    gimp_scale_entry_new (GTK_TABLE (table), 0, row++,
			  _("Number Height:"), 1.0, 0.5,
			  filmvals.number_height,
			  0.0, 1.0, 0.001, 0.01, 3,
			  TRUE, 0, 0,
			  NULL, NULL);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_double_adjustment_update),
		      &filmvals.number_height);

  gtk_spin_button_configure (GIMP_SCALE_ENTRY_SPINBUTTON (adj),
			     GIMP_SCALE_ENTRY_SPINBUTTON_ADJ (adj), 0.01, 3);

  sep = gtk_hseparator_new ();
  gtk_table_attach (GTK_TABLE (table), sep, 0, 3, row, row + 1,
		    GTK_FILL, 0, 0, 2);
  gtk_widget_show (sep);

  row++;

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 0, 3, row, row + 1);
  gtk_widget_show (hbox);

  row++;

  button = gtk_button_new_with_label (_("Reset to Defaults"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 4, 0);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (film_reset_callback),
		      NULL);
  gtk_widget_show (button);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return filmint.run;
}


static void
film_ok_callback (GtkWidget *widget,
                  gpointer   data)
{
  gint       num_images;
  gchar     *s;
  GtkWidget *label;
  GList     *tmp_list;
  gint32     image_ID;

  /* Read font family */
  s = gtk_entry_get_text (GTK_ENTRY (filmint.font_entry));
  if (strlen (s) > 0)
    {
      strncpy (filmvals.number_fontf, s, sizeof (filmvals.number_fontf));
      filmvals.number_fontf[sizeof (filmvals.number_fontf)-1] = '\0';
    }

  /* Read image list */
  num_images = 0;
  if (filmint.image_list_film != NULL)
    {
      
      for (tmp_list = GTK_LIST (filmint.image_list_film)->children;
	   tmp_list;
	   tmp_list = g_list_next (tmp_list))
	{
	  if ((label = (GtkWidget *) tmp_list->data) != NULL)
	    {
	      image_ID = (gint32) gtk_object_get_user_data (GTK_OBJECT (label));
	      if ((image_ID >= 0) && (num_images < MAX_FILM_PICTURES))
		filmvals.image[num_images++] = image_ID;
	    }
	}
      filmvals.num_images = num_images;
    }

  filmint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
film_reset_callback (GtkWidget *widget,
		     gpointer   data)
{
  gint i, num;

  num = sizeof (advanced_defaults) / sizeof (advanced_defaults[0]);

  for (i = 0; i < num; i++)
    gtk_adjustment_set_value (GTK_ADJUSTMENT (filmint.advanced_adj[i]),
			      advanced_defaults[i]);
}
