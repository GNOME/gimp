/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Compose plug-in (C) 1997,1999 Peter Kirchgessner
 * e-mail: peter@kirchgessner.net, WWW: http://www.kirchgessner.net
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
 * This plug-in composes RGB-images from several types of channels
 */

/* Event history:
 * V 1.00, PK, 29-Jul-97, Creation
 * V 1.01, nn, 20-Dec-97, Add default case in switch for hsv_to_rgb ()
 * V 1.02, PK, 18-Sep-98, Change variable names in Parameter definition.
 *         Otherwise script-fu merges parameters (reported by Patrick Valsecchi)
 *         Check images for same width/height (interactive mode)
 *         Use drawables in interactive menu
 *         Use g_message in interactive mode
 *         Check sensitivity of menues (thanks to Kevin Turner,
 *           kevint@poboxes.com)
 * V1.03, PK, 17-Mar-99, Update for GIMP 1.1.3
 *         Allow image ID 0
 *         Prepare for localization
 */
static char ident[] = "@(#) GIMP Compose plug-in v1.03 17-Mar-99";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

/* Declare local functions
 */
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      show_message (const char *message);

static gint32    compose (char *compose_type,
                          gint32 *compose_ID,
                          int compose_by_drawable);

static gint32    create_new_image (char *filename, guint width, guint height,
                   GDrawableType gdtype, gint32 *layer_ID, GDrawable **drawable,
                   GPixelRgn *pixel_rgn);

static int       cmp_icase (char *s1, char *s2);

static void      compose_rgb  (unsigned char **src, int *incr, int numpix,
                               unsigned char *dst);
static void      compose_rgba (unsigned char **src, int *incr, int numpix,
                               unsigned char *dst);
static void      compose_hsv  (unsigned char **src, int *incr, int numpix,
                               unsigned char *dst);
static void      compose_cmy  (unsigned char **src, int *incr, int numpix,
                               unsigned char *dst);
static void      compose_cmyk (unsigned char **src, int *incr, int numpix,
                               unsigned char *dst);

static gint      compose_dialog (char *compose_type,
                                 gint32 drawable_ID);

static gint      check_gray (gint32 image_id,
                             gint32 drawable_id,
                             gpointer data);

static void      image_menu_callback (gint32     id,
                                      gpointer   data);

static void      compose_ok_callback         (GtkWidget *widget,
                                              gpointer   data);
static void      compose_type_toggle_update  (GtkWidget *widget,
                                              gpointer   data);

/* Maximum number of images to compose */
#define MAX_COMPOSE_IMAGES 4


/* Description of a composition */
typedef struct {
  char *compose_type;             /* Type of composition ("RGB", "RGBA",...) */
  int num_images;                 /* Number of input images needed */
  char *channel_name[MAX_COMPOSE_IMAGES];  /* channel names for dialog */
  char *filename;                 /* Name of new image */
                                  /* Compose functon */
  void (*compose_fun)(unsigned char **src, int *incr_src, int numpix,
                      unsigned char *dst);
} COMPOSE_DSC;

/* Array of available compositions. */
#define CHNL_NA "-"

static COMPOSE_DSC compose_dsc[] = {
 { N_("RGB"),  3, { N_("Red:"),   N_("Green:"), N_("Blue:"),   CHNL_NA },
              N_("rgb-compose"),  compose_rgb },
 { N_("RGBA"), 4, { N_("Red:"),   N_("Green:"), N_("Blue:"),N_("Alpha:") },
              N_("rgba-compose"),  compose_rgba },
 { N_("HSV"),  3, { N_("Hue:"),   N_("Saturation:"), N_("Value:"),  CHNL_NA },
              N_("hsv-compose"),  compose_hsv },
 { N_("CMY"),  3, { N_("Cyan:"),  N_("Magenta:"), N_("Yellow:"), CHNL_NA },
              N_("cmy-compose"),  compose_cmy },
 { N_("CMYK"), 4, { N_("Cyan:"),  N_("Magenta:"),N_("Yellow:"),N_("Black:")},
              N_("cmyk-compose"), compose_cmyk }
};

#define MAX_COMPOSE_TYPES (sizeof (compose_dsc) / sizeof (compose_dsc[0]))


typedef struct {
  gint32 compose_ID[MAX_COMPOSE_IMAGES];  /* Image IDs of input images */
  char compose_type[32];                  /* type of composition */
} ComposeVals;

/* Dialog structure */
typedef struct {
  int width, height;                     /* Size of selected image */

  GtkWidget *channel_label[MAX_COMPOSE_IMAGES]; /* The labels to change */
  GtkWidget *channel_menu[MAX_COMPOSE_IMAGES];  /* The menues */

  gint32 select_ID[MAX_COMPOSE_IMAGES];  /* Image Ids selected by menu */
  gint compose_flag[MAX_COMPOSE_TYPES];  /* toggle data of compose type */
  gint run;
} ComposeInterface;

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static ComposeVals composevals =
{
  { 0 },  /* Image IDs of images to compose */
  "rgb"   /* Type of composition */
};

static ComposeInterface composeint =
{
  0, 0,     /* width, height */
  { NULL }, /* Label Widgets */
  { NULL }, /* Menu Widgets */
  { 0 },    /* Image IDs from menues */
  { 0 },    /* Compose type toggle flags */
  FALSE     /* run */
};

static GRunModeType run_mode;


MAIN ()

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image1", "First input image" },
    { PARAM_DRAWABLE, "drawable", "Input drawable (not used)" },
    { PARAM_IMAGE, "image2", "Second input image" },
    { PARAM_IMAGE, "image3", "Third input image" },
    { PARAM_IMAGE, "image4", "Fourth input image" },
    { PARAM_STRING, "compose_type", "What to compose: RGB, RGBA, HSV,\
 CMY, CMYK" }
  };
  static GParamDef return_vals[] =
  {
    { PARAM_IMAGE, "new_image", "Output image" }
  };
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

  static GParamDef drw_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image1", "First input image (not used)" },
    { PARAM_DRAWABLE, "drawable1", "First input drawable" },
    { PARAM_DRAWABLE, "drawable2", "Second input drawable" },
    { PARAM_DRAWABLE, "drawable3", "Third input drawable" },
    { PARAM_DRAWABLE, "drawable4", "Fourth input drawable" },
    { PARAM_STRING, "compose_type", "What to compose: RGB, RGBA, HSV,\
 CMY, CMYK" }
  };
  static GParamDef drw_return_vals[] =
  {
    { PARAM_IMAGE, "new_image", "Output image" }
  };
  static int drw_nargs = sizeof (args) / sizeof (args[0]);
  static int drw_nreturn_vals = sizeof (return_vals) / sizeof (return_vals[0]);

  INIT_I18N ();

  gimp_install_procedure ("plug_in_compose",
			  "Compose an image from multiple gray images",
			  "This function creates a new image from "
			  "multiple gray images",
			  "Peter Kirchgessner",
			  "Peter Kirchgessner (peter@kirchgessner.net)",
			  "1997",
			  N_("<Image>/Image/Mode/Compose..."),
			  "GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);

  gimp_install_procedure ("plug_in_drawable_compose",
			  "Compose an image from multiple drawables of gray images",
			  "This function creates a new image from "
			  "multiple drawables of gray images",
			  "Peter Kirchgessner",
			  "Peter Kirchgessner (peter@kirchgessner.net)",
			  "1998",
			  NULL,   /* It is not available in interactive mode */
			  "GRAY*",
			  PROC_PLUG_IN,
			  drw_nargs, drw_nreturn_vals,
			  drw_args, drw_return_vals);
}


static void show_message (const char *message)

{
 if (run_mode == RUN_INTERACTIVE)
   gimp_message (message);
 else
   printf ("%s\n", message);
}


static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID, drawable_ID;
  int compose_by_drawable;
  gchar *msg;

  INIT_I18N_UI ();

  run_mode = param[0].data.d_int32;
  compose_by_drawable = (strcmp (name, "plug_in_drawable_compose") == 0);

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
      gimp_get_data (name , &composevals);

      /* The dialog is now drawable based. Get a drawable-ID of the image */
      if (strcmp (name, "plug_in_compose") == 0)
      {gint32 *layer_list;
       gint nlayers;

        layer_list = gimp_image_get_layers (param[1].data.d_int32, &nlayers);
        if ((layer_list == NULL) || (nlayers <= 0))
        {
	  msg = g_strdup_printf (_("compose: Could not get layers for image %d"),
				 (int)param[1].data.d_int32);
          show_message (msg);
	  g_free (msg);
          return;
        }
        drawable_ID = layer_list[0];
        g_free (layer_list);
      }
      else
        drawable_ID = param[2].data.d_int32;

      compose_by_drawable = 1;

      /*  First acquire information with a dialog  */
      if (! compose_dialog (composevals.compose_type, drawable_ID))
	return;

      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 7)
	status = STATUS_CALLING_ERROR;

      if (status == STATUS_SUCCESS)
      {
        composevals.compose_ID[0] =
          compose_by_drawable ? param[2].data.d_int32 : param[1].data.d_int32;
        composevals.compose_ID[1] = param[3].data.d_int32;
        composevals.compose_ID[2] = param[4].data.d_int32;
        composevals.compose_ID[3] = param[5].data.d_int32;
        strncpy (composevals.compose_type, param[6].data.d_string,
                 sizeof (composevals.compose_type));
        composevals.compose_type[sizeof (composevals.compose_type)-1] = '\0';
      }
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data (name, &composevals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      if (run_mode != RUN_NONINTERACTIVE)
        gimp_progress_init (_("Composing..."));

      image_ID = compose (composevals.compose_type, composevals.compose_ID,
                          compose_by_drawable);

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
        gimp_set_data (name, &composevals, sizeof (ComposeVals));
    }

  values[0].data.d_status = status;
}


/* Compose an image from several gray-images */
static gint32
compose (char   *compose_type,
         gint32 *compose_ID,
         int     compose_by_drawable)
{
  int width, height, tile_height, scan_lines;
  int num_images, compose_idx, incr_src[MAX_COMPOSE_IMAGES];
  int i, j;
  gint num_layers;
  gint32 layer_ID_dst, image_ID_dst;
  guchar *src[MAX_COMPOSE_IMAGES], *dst = (guchar *)ident;
  GDrawableType gdtype_dst;
  GDrawable *drawable_src[MAX_COMPOSE_IMAGES], *drawable_dst;
  GPixelRgn pixel_rgn_src[MAX_COMPOSE_IMAGES], pixel_rgn_dst;
  
  /* Search type of composing */
  compose_idx = -1;
  for (j = 0; j < MAX_COMPOSE_TYPES; j++)
    {
      if (cmp_icase (compose_type, compose_dsc[j].compose_type) == 0)
	compose_idx = j;
    }
  if (compose_idx < 0)
    return (-1);
  
  num_images = compose_dsc[compose_idx].num_images;
  tile_height = gimp_tile_height ();
  
  /* Check image sizes */
  if (compose_by_drawable)
    {
      width = gimp_drawable_width (compose_ID[0]);
      height = gimp_drawable_height (compose_ID[0]);
      
      for (j = 1; j < num_images; j++)
	{
	  if (   (width != (int)gimp_drawable_width (compose_ID[j]))
		 || (height != (int)gimp_drawable_height (compose_ID[j])))
	    {
	      show_message (_("Compose: Drawables have different size"));
	      return -1;
	    }
	}
      for (j = 0; j < num_images; j++)
	drawable_src[j] = gimp_drawable_get (compose_ID[j]);
    }
  else    /* Compose by image ID */
    {
      width = gimp_image_width (compose_ID[0]);
      height = gimp_image_height (compose_ID[0]);
      
      for (j = 1; j < num_images; j++)
	{
	  if (   (width != (int)gimp_image_width (compose_ID[j]))
		 || (height != (int)gimp_image_height (compose_ID[j])))
	    {
	      show_message (_("Compose: Images have different size"));
	      return -1;
	    }
	}
      
      /* Get first layer/drawable for all input images */
      for (j = 0; j < num_images; j++)
	{gint32 *g32;
	
	/* Get first layer of image */
	g32 = gimp_image_get_layers (compose_ID[j], &num_layers);
	if ((g32 == NULL) || (num_layers <= 0))
	  {
	    show_message (_("Compose: Error in getting layer IDs"));
	    return (-1);
	  }
	
	/* Get drawable for layer */
	drawable_src[j] = gimp_drawable_get (g32[0]);
	g_free (g32);
	}
    }
  
  /* Get pixel region for all input drawables */
  for (j = 0; j < num_images; j++)
    {
      /* Check bytes per pixel */
      incr_src[j] = drawable_src[j]->bpp;
      if ((incr_src[j] != 1) && (incr_src[j] != 2))
	{
	  gchar *msg =
	    g_strdup_printf (_("Compose: Image is not a gray image (bpp=%d)"), 
			     incr_src[j]);
	  show_message (msg);
	  g_free (msg);
	  return (-1);
	}
      
      /* Get pixel region */
      gimp_pixel_rgn_init (&(pixel_rgn_src[j]), drawable_src[j], 0, 0,
			   width, height, FALSE, FALSE);
      
      /* Get memory for retrieving information */
      src[j] = (unsigned char *)g_malloc (tile_height * width
					  * drawable_src[j]->bpp);
    }
  
  /* Create new image */
  gdtype_dst = (compose_dsc[compose_idx].compose_fun == compose_rgba)
    ? RGBA_IMAGE : RGB_IMAGE;
  image_ID_dst = create_new_image (compose_dsc[compose_idx].filename,
				   width, height, gdtype_dst,
				   &layer_ID_dst, &drawable_dst, &pixel_rgn_dst);
  dst = (unsigned char *)g_malloc (tile_height * width * drawable_dst->bpp);
  
  /* Do the composition */
  i = 0;
  while (i < height)
    {
      scan_lines = (i+tile_height-1 < height) ? tile_height : (height-i);
      
      /* Get source pixel regions */
      for (j = 0; j < num_images; j++)
	gimp_pixel_rgn_get_rect (&(pixel_rgn_src[j]), src[j], 0, i,
				 width, scan_lines);
      
      /* Do the composition */
      compose_dsc[compose_idx].compose_fun (src,incr_src,width*tile_height,dst);
      
      /* Set destination pixel region */
      gimp_pixel_rgn_set_rect (&pixel_rgn_dst, dst, 0, i, width, scan_lines);
      
      i += scan_lines;
      
      if (run_mode != RUN_NONINTERACTIVE)
	gimp_progress_update (((double)i) / (double)height);
    }
  
  for (j = 0; j < num_images; j++)
    {
      g_free (src[j]);
      gimp_drawable_flush (drawable_src[j]);
      gimp_drawable_detach (drawable_src[j]);
    }
  g_free (dst);
  gimp_drawable_flush (drawable_dst);
  gimp_drawable_detach (drawable_dst);
  
  return image_ID_dst;
}


/* Create an image. Sets layer_ID, drawable and rgn. Returns image_ID */
static gint32
create_new_image (char           *filename,
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
  
  *drawable = gimp_drawable_get (*layer_ID);
  gimp_pixel_rgn_init (pixel_rgn, *drawable, 0, 0, (*drawable)->width,
		       (*drawable)->height, TRUE, FALSE);
  
  return (image_ID);
}


/* Compare two strings ignoring case (could also be done by strcasecmp() */
/* but is it available everywhere ?) */
static int 
cmp_icase (char *s1, 
	   char *s2)     
{
  int c1, c2;
  
  c1 = toupper (*s1);  c2 = toupper (*s2);
  while (*s1 && *s2)
    {
      if (c1 != c2) return (c2 - c1);
      c1 = toupper (*(++s1));  c2 = toupper (*(++s2));
    }
  return (c2 - c1);
}


static void
compose_rgb (unsigned char **src,
             int            *incr_src,
             int             numpix,
             unsigned char  *dst)
{
  register unsigned char *red_src = src[0];
  register unsigned char *green_src = src[1];
  register unsigned char *blue_src = src[2];
  register unsigned char *rgb_dst = dst;
  register int count = numpix;
  int red_incr = incr_src[0], green_incr = incr_src[1], blue_incr = incr_src[2];
  
  if ((red_incr == 1) && (green_incr == 1) && (blue_incr == 1))
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = *(red_src++);
	  *(rgb_dst++) = *(green_src++);
	  *(rgb_dst++) = *(blue_src++);
	}
    }
  else
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = *red_src;     red_src += red_incr;
	  *(rgb_dst++) = *green_src;   green_src += green_incr;
	  *(rgb_dst++) = *blue_src;    blue_src += blue_incr;
	}
    }
}


static void
compose_rgba (unsigned char **src,
              int            *incr_src,
              int             numpix,
              unsigned char  *dst)
{
  register unsigned char *red_src = src[0];
  register unsigned char *green_src = src[1];
  register unsigned char *blue_src = src[2];
  register unsigned char *alpha_src = src[3];
  register unsigned char *rgb_dst = dst;
  register int count = numpix;
  int red_incr = incr_src[0], green_incr = incr_src[1],
    blue_incr = incr_src[2], alpha_incr = incr_src[3];
  
  if (   (red_incr == 1) && (green_incr == 1) && (blue_incr == 1)
	 && (alpha_incr == 1))
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = *(red_src++);
	  *(rgb_dst++) = *(green_src++);
	  *(rgb_dst++) = *(blue_src++);
	  *(rgb_dst++) = *(alpha_src++);
	}
    }
  else
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = *red_src;    red_src += red_incr;
	  *(rgb_dst++) = *green_src;  green_src += green_incr;
	  *(rgb_dst++) = *blue_src;   blue_src += blue_incr;
	  *(rgb_dst++) = *alpha_src;  alpha_src += alpha_incr;
	}
    }
}


static void
compose_hsv (unsigned char **src,
             int            *incr_src,
             int             numpix,
             unsigned char  *dst)
{
  register unsigned char *hue_src = src[0];
  register unsigned char *sat_src = src[1];
  register unsigned char *val_src = src[2];
  register unsigned char *rgb_dst = dst;
  register int count = numpix;
  int hue_incr = incr_src[0], sat_incr = incr_src[1], val_incr = incr_src[2];
  
  while (count-- > 0)
    {
      gimp_hsv_to_rgb4 (rgb_dst, (double) *hue_src / 255.0, 
			         (double) *sat_src / 255.0, 
			         (double) *val_src / 255.0);
      rgb_dst += 3;
      hue_src += hue_incr;
      sat_src += sat_incr;
      val_src += val_incr;
    }
}


static void
compose_cmy (unsigned char **src,
             int            *incr_src,
             int             numpix,
             unsigned char  *dst)
{
  register unsigned char *cyan_src = src[0];
  register unsigned char *magenta_src = src[1];
  register unsigned char *yellow_src = src[2];
  register unsigned char *rgb_dst = dst;
  register int count = numpix;
  int cyan_incr = incr_src[0], magenta_incr = incr_src[1],
    yellow_incr = incr_src[2];
  
  if ((cyan_incr == 1) && (magenta_incr == 1) && (yellow_incr == 1))
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = 255 - *(cyan_src++);
	  *(rgb_dst++) = 255 - *(magenta_src++);
	  *(rgb_dst++) = 255 - *(yellow_src++);
	}
    }
  else
    {
      while (count-- > 0)
	{
	  *(rgb_dst++) = 255 - *cyan_src;
	  *(rgb_dst++) = 255 - *magenta_src;
	  *(rgb_dst++) = 255 - *yellow_src;
	  cyan_src += cyan_incr;
	  magenta_src += magenta_incr;
	  yellow_src += yellow_incr;
	}
    }
}


static void
compose_cmyk (unsigned char **src,
              int            *incr_src,
              int             numpix,
              unsigned char  *dst)
{
  register unsigned char *cyan_src = src[0];
  register unsigned char *magenta_src = src[1];
  register unsigned char *yellow_src = src[2];
  register unsigned char *black_src = src[3];
  register unsigned char *rgb_dst = dst;
  register int count = numpix;
  int cyan, magenta, yellow, black;
  int cyan_incr = incr_src[0], magenta_incr = incr_src[1],
    yellow_incr = incr_src[2], black_incr = incr_src[3];
  
  while (count-- > 0)
    {
      black = (int)*black_src;
      if (black)
	{
	  cyan = (int)*cyan_src;
	  magenta = (int)*magenta_src;
	  yellow = (int)*yellow_src;
	  cyan += black; if (cyan > 255) cyan = 255;
	  magenta += black; if (magenta > 255) magenta = 255;
	  yellow += black; if (yellow > 255) yellow = 255;
	  *(rgb_dst++) = 255 - cyan;
	  *(rgb_dst++) = 255 - magenta;
	  *(rgb_dst++) = 255 - yellow;
	}
      else
	{
	  *(rgb_dst++) = 255 - *cyan_src;
	  *(rgb_dst++) = 255 - *magenta_src;
	  *(rgb_dst++) = 255 - *yellow_src;
	}
      cyan_src += cyan_incr;
      magenta_src += magenta_incr;
      yellow_src += yellow_incr;
      black_src += black_incr;
    }
}


static gint
compose_dialog (char   *compose_type,
                gint32  drawable_ID)
{
  GtkWidget *dlg;
  GtkWidget *toggle;
  GtkWidget *left_frame, *right_frame;
  GtkWidget *left_vbox, *right_vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *image_option_menu, *image_menu;
  GSList *group;
  gchar **argv;
  gint argc;
  int j, compose_idx, sensitive;

  /* Check default compose type */
  compose_idx = -1;
  for (j = 0; j < MAX_COMPOSE_TYPES; j++)
  {
    if (cmp_icase (compose_type, compose_dsc[j].compose_type) == 0)
      compose_idx = j;
  }
  if (compose_idx < 0) compose_idx = 0;

  /* Save original image width/height */
  composeint.width = gimp_drawable_width (drawable_ID);
  composeint.height = gimp_drawable_height (drawable_ID);

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("compose");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Compose"), "compose",
			 gimp_plugin_help_func, "filters/compose.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), compose_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* The left frame keeps the compose type toggles */
  left_frame = gtk_frame_new (_("Compose Channels"));
  gtk_frame_set_shadow_type (GTK_FRAME (left_frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), left_frame, FALSE, FALSE, 0);

  left_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (left_vbox), 4);
  gtk_container_add (GTK_CONTAINER (left_frame), left_vbox);

  /* The right frame keeps the selection menues for images. */
  /* Because the labels within this frame will change when a toggle */
  /* in the left frame is changed, fill in the right part first. */
  /* Otherwise it can occur, that a non-existing label might be changed. */

  right_frame = gtk_frame_new (_("Channel Representations"));
  gtk_frame_set_shadow_type (GTK_FRAME (right_frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (hbox), right_frame, TRUE, TRUE, 0);

  right_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (right_vbox), 4);
  gtk_container_add (GTK_CONTAINER (right_frame), right_vbox);

  table = gtk_table_new (MAX_COMPOSE_IMAGES, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (right_vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  /* Channel names */
  for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
    {
      composeint.channel_label[j] = label =
	gtk_label_new (gettext (compose_dsc[compose_idx].channel_name[j]));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, j, j+1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }
  /* Set sensitivity of last label */
  sensitive = (strcmp (compose_dsc[compose_idx].channel_name[3],
                       CHNL_NA) != 0);
  gtk_widget_set_sensitive (composeint.channel_label[3], sensitive);

  /* Menues to select images */
  for (j = 0; j <  MAX_COMPOSE_IMAGES; j++)
    {
      composeint.select_ID[j] = drawable_ID;
      composeint.channel_menu[j] = image_option_menu = gtk_option_menu_new ();
      image_menu = gimp_drawable_menu_new (check_gray, image_menu_callback,
					   &(composeint.select_ID[j]),
					   composeint.select_ID[j]);
      gtk_table_attach (GTK_TABLE (table), image_option_menu, 1, 2, j, j+1,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

      gtk_widget_show (image_option_menu);
      gtk_option_menu_set_menu (GTK_OPTION_MENU (image_option_menu), image_menu);
    }
  gtk_widget_set_sensitive (composeint.channel_menu[3], sensitive);

  /* Compose types */
  group = NULL;
  for (j = 0; j < MAX_COMPOSE_TYPES; j++)
    {
      toggle = gtk_radio_button_new_with_label (group,
						gettext(compose_dsc[j].compose_type));
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
      gtk_box_pack_start (GTK_BOX (left_vbox), toggle, TRUE, TRUE, 0);
      composeint.compose_flag[j] = (j == compose_idx);
      gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
			  (GtkSignalFunc) compose_type_toggle_update,
			  &(composeint.compose_flag[j]));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				    composeint.compose_flag[j]);
      gtk_widget_show (toggle);
    }

  gtk_widget_show (left_vbox);
  gtk_widget_show (right_vbox);
  gtk_widget_show (left_frame);
  gtk_widget_show (right_frame);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return composeint.run;
}

/*  Compose interface functions  */

static gint
check_gray (gint32   image_id,
            gint32   drawable_id,
            gpointer data)

{
  return ((gimp_image_base_type (image_id) == GRAY)
	  && (gimp_image_width (image_id) == composeint.width)
	  && (gimp_image_height (image_id) == composeint.height));
}


static void
image_menu_callback (gint32   id,
                     gpointer data)
{
  *(gint32 *)data = id;
}


static void
compose_ok_callback (GtkWidget *widget,
                     gpointer   data)
{
  int j;

  composeint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));

  for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
    composevals.compose_ID[j] = composeint.select_ID[j];

  for (j = 0; j < MAX_COMPOSE_TYPES; j++)
    {
      if (composeint.compose_flag[j])
	{
	  strcpy (composevals.compose_type, compose_dsc[j].compose_type);
	  break;
	}
    }
}


static void
compose_type_toggle_update (GtkWidget *widget,
                            gpointer   data)
{
  gint *toggle_val;
  gint compose_idx, j;
  int sensitive;

  toggle_val = (gint *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      *toggle_val = TRUE;
      compose_idx = toggle_val - &(composeint.compose_flag[0]);
      for (j = 0; j < MAX_COMPOSE_IMAGES; j++)
	gtk_label_set_text (GTK_LABEL (composeint.channel_label[j]),
			    compose_dsc[compose_idx].channel_name[j]);
      
      /* Set sensitivity of last label */
      sensitive = (strcmp (compose_dsc[compose_idx].channel_name[3],
			   CHNL_NA) != 0);
      gtk_widget_set_sensitive (composeint.channel_label[3], sensitive);
      gtk_widget_set_sensitive (composeint.channel_menu[3], sensitive);
    }
  else
    *toggle_val = FALSE;
}








