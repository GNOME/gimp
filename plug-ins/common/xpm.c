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
 */

/*
   RGB and RGBA images only

   Not tested under 8-bit displays. (not likely to be either...)

   Seems to generate an AWFUL lot of colors, especially on 24 bit display
   You'll probably have better results if you convert to indexed and back
   before saving to cut down the number of colors.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"


#define SCALE_WIDTH 125

/* Structs for the save dialog */
typedef struct
{
  gdouble threshold;
} XpmSaveVals;

typedef struct
{
  gint  run;
} XpmSaveInterface;


/* Declare local functions */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);

static gint32
load_image   (char   *filename);

static void
parse_colors (XpmImage  *xpm_image,
              guchar   **cmap);

static void
parse_image  (gint32    image_ID,
              XpmImage *xpm_image,
              guchar   *cmap);

static gint
save_image   (char   *filename,
              gint32  image_ID,
              gint32  drawable_ID);

static gint
save_image_2 (GDrawable *drawable,
              XImage    *image,
              XImage    *mask,
              int        depth,
              int        alpha);

static gint
save_dialog ();

static void
save_close_callback  (GtkWidget *widget,
                      gpointer   data);

static void
save_ok_callback     (GtkWidget *widget,
                      gpointer   data);

static void
save_scale_update    (GtkAdjustment *adjustment,
                      double        *scale_val);




GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static XpmSaveVals xpmvals = 
{
  0.50  /* alpha threshold */
};

static XpmSaveInterface xpmint =
{
  FALSE   /*  run  */
};



MAIN ()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32,    "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING,   "filename", "The name of the file to save the image in" },
    { PARAM_STRING,   "raw_filename", "The name of the file to save the image in" },
    { PARAM_FLOAT,    "alpha_threshold", "Alpha cutoff threshold" }

  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_xpm_load",
                          "loads files of the xpm file format",
                          "FIXME: write help for xpm_load",
                          "Spencer Kimball & Peter Mattis & Ray Lehtiniemi",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          "<Load>/Xpm",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);
  
  gimp_install_procedure ("file_xpm_save",
                          "saves files in the xpm file format (if you're on a 16 bit display...)",
                          "FIXME: write help for xpm",
                          "Spencer Kimball & Peter Mattis & Ray Lehtiniemi",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          "<Save>/Xpm",
                          "RGB", /* , GRAY", */
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_load_handler ("file_xpm_load", "xpm", "<Load>/Xpm");
  gimp_register_save_handler ("file_xpm_save", "xpm", "<Save>/Xpm");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  gint32 image_ID;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_xpm_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[0].data.d_status = STATUS_SUCCESS;
          values[1].type = PARAM_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          values[0].data.d_status = STATUS_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, "file_xpm_save") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_xpm_save", &xpmvals);

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    return;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 5)
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      xpmvals.threshold = param[5].data.d_float;
	    }
	  if (status == STATUS_SUCCESS &&
	      (xpmvals.threshold < 0.0 || xpmvals.threshold > 1.0))
	    status = STATUS_CALLING_ERROR;
	  break;

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_xpm_save", &xpmvals);
	  break;

	default:
	  break;
	}
      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string,
                      param[1].data.d_int32,
                      param[2].data.d_int32))
	{
	  gimp_set_data ("file_xpm_save", &xpmvals, sizeof (XpmSaveVals));
	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
        values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
  else
    g_assert (FALSE);
}




static gint32
load_image (char *filename)
{
  XpmImage  xpm_image;
  guchar   *cmap;
  gint32    image_ID;
  char     *name;

  /* put up a progress bar */
  name = malloc (strlen (filename) + 12);
  if (!name)
    gimp_quit();
  sprintf (name, "Loading %s:", filename);
  gimp_progress_init (name);
  free (name);

  /* read the raw file */
  XpmReadFileToXpmImage (filename, &xpm_image, NULL);

  /* parse out the colors into a cmap */
  parse_colors (&xpm_image, &cmap);
  if (cmap == NULL)
    gimp_quit();
  
  /* create the new image */
  image_ID = gimp_image_new (xpm_image.width,
                             xpm_image.height,
                             RGB);

  /* name it */
  gimp_image_set_filename (image_ID, filename);

  /* fill it */
  parse_image(image_ID, &xpm_image, cmap);
  
  /* clean up and exit */
  g_free(cmap);
  
  return image_ID;
}




static void
parse_colors (XpmImage *xpm_image, guchar **cmap)
{
  Display  *display;
  Colormap  colormap;
  int       i, j;

  
  /* open the display and get the default color map */
  display  = XOpenDisplay (NULL);
  colormap = DefaultColormap (display, DefaultScreen (display));
    
  /* alloc a buffer to hold the parsed colors */
  *cmap = g_new(guchar, sizeof (guchar) * 4 * xpm_image->ncolors);

  if ((*cmap) != NULL)
    {
      /* default to black transparent */
      memset((void*)(*cmap), 0, sizeof (guchar) * 4 * xpm_image->ncolors);
      
      /* parse each color in the file */
      for (i = 0, j = 0; i < xpm_image->ncolors; i++)
        {
          char     *colorspec = "None";
          XpmColor *xpm_color;
          XColor    xcolor;
        
          xpm_color = &(xpm_image->colorTable[i]);
        
          /* pick the best spec available */
          if (xpm_color->c_color)
            colorspec = xpm_color->c_color;
          else if (xpm_color->g_color)
            colorspec = xpm_color->g_color;
          else if (xpm_color->g4_color)
            colorspec = xpm_color->g4_color;
          else if (xpm_color->m_color)
            colorspec = xpm_color->m_color;
        
          if (strcmp(colorspec, "none") == 0)
            colorspec = "None";

          /* parse if it's not transparent.  the assumption is that
             g_new will memset the buffer to zeros */
          if (strcmp(colorspec, "None") != 0) {
            XParseColor (display, colormap, colorspec, &xcolor);
            (*cmap)[j++] = xcolor.red >> 8;
            (*cmap)[j++] = xcolor.green >> 8;
            (*cmap)[j++] = xcolor.blue >> 8;
            (*cmap)[j++] = ~0;
          } else {
            j += 4;
          }
        }
    }
    
  XCloseDisplay (display);
}



static void
parse_image(gint32 image_ID, XpmImage *xpm_image, guchar *cmap)
{
  int        tile_height;
  int        scanlines;
  int        val;  
  guchar    *buf;
  guchar    *dest;
  unsigned int *src;
  GPixelRgn  pixel_rgn;
  GDrawable *drawable;
  gint32     layer_ID;
  int        i,j;
    

  layer_ID = gimp_layer_new (image_ID,
                             "Color",
                             xpm_image->width,
                             xpm_image->height,
                             RGBA_IMAGE,
                             100,
                             NORMAL_MODE);
    
  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);
    
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0,
                       drawable->width, drawable->height,
                       TRUE, FALSE);

  tile_height = gimp_tile_height ();
    
  buf  = g_new (guchar, tile_height * xpm_image->width * 4);

  if (buf != NULL)
    {
      src  = xpm_image->data;
      for (i = 0; i < xpm_image->height; i+=tile_height)
        {
          dest = buf;
          scanlines = MIN(tile_height, xpm_image->height - i);
          j = scanlines * xpm_image->width;
          while (j--) {
            {
              val = *(src++) * 4;
              *(dest)   = cmap[val];
              *(dest+1) = cmap[val+1];
              *(dest+2) = cmap[val+2];
              *(dest+3) = cmap[val+3];
              dest += 4;
            }
          
            if ((j % 100) == 0)
              gimp_progress_update ((double) i / (double) xpm_image->height);
          }
        
          gimp_pixel_rgn_set_rect (&pixel_rgn, buf,
                                   0, i,
                                   drawable->width, scanlines);
        
        }
  
      g_free(buf);
    }
  
  gimp_drawable_detach (drawable);
}





static gint
save_image (char   *filename,
            gint32  image_ID,
            gint32  drawable_ID)
{
  Display  *display;
  int       screen;
  Visual   *visual;
  int       depth;

  GDrawable *drawable;
    
  int       width;
  int       height;
  int       alpha;

  XImage   *image   = NULL;
  XImage   *mask    = NULL;
  guchar   *ibuff   = NULL;
  guchar   *mbuff   = NULL;

  gint      rc = FALSE;


  /* get some basic stats about the image */
  switch (gimp_drawable_type (drawable_ID)) {
  case RGBA_IMAGE:
    alpha = 1;
    break;
  case RGB_IMAGE:
    alpha = 0;
    break;
  default:
    return FALSE;
  }

  drawable = gimp_drawable_get (drawable_ID);
  width    = drawable->width;
  height   = drawable->height;

  /* get some info about the display */
  display  = XOpenDisplay  (NULL);
  screen   = DefaultScreen (display);
  visual   = DefaultVisual (display, screen);
  depth    = DefaultDepth  (display, screen);
  
  /* allocate the XImages */
  if ((image = XCreateImage(display, visual, depth,
                            ZPixmap, 0, NULL,
                            width, height, 16, 0)) == NULL)
    goto cleanup;
  
  if ((mask  = XCreateImage(display, visual, 1,
                            ZPixmap, 0, NULL,
                            width, height, 8, 0)) == NULL)
    goto cleanup;


  /* allocate buffers making the assumption that ibuff and mbuff
     are 32 bit aligned... */
  if ((ibuff = g_new(guchar, image->bytes_per_line*height)) == NULL)
    goto cleanup;
  memset(ibuff, 0, image->bytes_per_line*height);
  image->data = ibuff;

  if ((mbuff = g_new(guchar, mask->bytes_per_line*height)) == NULL)
    goto cleanup;
  memset(mbuff, 0, mask->bytes_per_line*height);
  mask->data  = mbuff;

  
  /* put up a progress bar */
  {
    char *name = malloc (strlen (filename) + 12);
    if (!name)
      gimp_quit();
    sprintf (name, "Saving %s:", filename);
    gimp_progress_init (name);
    free (name);
  }

  /* do the save */
  if (save_image_2 (drawable, image, mask, depth, alpha))
    {
      XpmWriteFileFromImage(display, filename, image, mask, NULL);
      rc = TRUE;
    }      

  
 cleanup:

  /* clean up resources */  
  gimp_drawable_detach (drawable);
  
  if (image != NULL) {
    image->data = NULL;
    XDestroyImage(image);
  }
  
  if (mask != NULL) {
    mask->data = NULL;
    XDestroyImage(mask);
  }
  
  if (ibuff  != NULL) g_free(ibuff);
  if (mbuff  != NULL) g_free(mbuff);

  XCloseDisplay (display);

  return rc;
}


static gint
save_image_2 (GDrawable *drawable,
              XImage    *image,
              XImage    *mask,
              int        depth,
              int        alpha)
{
  GPixelRgn  pixel_rgn;
  guchar    *buffer;
  guchar    *data;
  int        i, j, k;
  int        width  = drawable->width;
  int        height = drawable->height;
  int        bpp    = drawable->bpp;
  int        threshold = 255 * xpmvals.threshold;

  
  /* allocate a pixel region to work with */
  if ((buffer = g_new(guchar, gimp_tile_height()*width*bpp)) == NULL)
    return 0;
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0,
                       width, height,
                       TRUE, FALSE);

  /* process each row of tiles */
  for (i = 0; i < height; i+=gimp_tile_height())
    {
      int scanlines;
  
      /* read the next row of tiles */
      scanlines = MIN(gimp_tile_height(), height - i);
      gimp_pixel_rgn_get_rect(&pixel_rgn, buffer, 0, i, width, scanlines);
      data = buffer;

      /* process each pixel row */
      for (j=0; j<scanlines; j++)
        {
          /* go to the start of this row in each image */
          guchar *idata = image->data + (i+j) * image->bytes_per_line;
          guchar *mdata = mask->data  + (i+j) * mask->bytes_per_line;

          /* do each pixel in the row */
          for (k=0; k<width; k++)
            {
              guchar  r, g, b, a;
              unsigned long ipixel;
  
              /* get pixel data */
              ipixel  = 0;
              r = *(data++);
              g = *(data++);
              b = *(data++);
              a = (alpha ? *(data++) : 255);


              /* pack the pixel */
              switch (depth)
                {
                case 8:
                  ipixel |= (r & 0xe0);
                  ipixel |= (g & 0xe0) >> 3;
                  ipixel |= (b & 0xc0) >> 5;
                  break;

                case 16:
                  ipixel |= (r & 0xf8) << 8;
                  ipixel |= (g & 0xfc) << 3;
                  ipixel |= (b & 0xf8) >> 3;
                  
                  *(idata++)  = ipixel & 0xff;
                  *(idata++)  = (ipixel>>8) & 0xff;
                  break;

                case 24:
                  *(idata++)  = r;
                  *(idata++)  = g;
                  *(idata++)  = b;
                  *(idata++)  = 0;
                  break;
                }

              /* mask doesn't worry about depth */
              {
                static char masks[] = {
                  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
                };
                if (a >= threshold)
                  mdata[k >> 3] |= masks[k & 0x07];
              }
            }
      
          /* kick the progress bar */
          gimp_progress_update ((double) (i+j) / (double) height);
        }
    }

  g_free(buffer);
  return 1;
}


static gint
save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *scale;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *scale_data;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("save");

  gtk_init (&argc, &argv);

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Save as Xpm");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) save_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      (GtkSignalFunc) save_ok_callback,
                      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Parameter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (1, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  label = gtk_label_new ("Alpha Threshold");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 5, 0);
  scale_data = gtk_adjustment_new (xpmvals.threshold, 0.0, 1.0, 0.01, 0.01, 0.0);
  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_widget_set_usize (scale, SCALE_WIDTH, 0);
  gtk_table_attach (GTK_TABLE (table), scale, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_scale_set_digits (GTK_SCALE (scale), 2);
  gtk_range_set_update_policy (GTK_RANGE (scale), GTK_UPDATE_DELAYED);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) save_scale_update,
		      &xpmvals.threshold);
  gtk_widget_show (label);
  gtk_widget_show (scale);

  gtk_widget_show (frame);
  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return xpmint.run;
}


/*  Save interface functions  */

static void
save_close_callback (GtkWidget *widget,
		     gpointer   data)
{
  gtk_main_quit ();
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  xpmint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
save_scale_update (GtkAdjustment *adjustment,
		   double        *scale_val)
{
  *scale_val = adjustment->value;
}
