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

/* XPM plugin version 1.2.2 */

/* 1.2.2 fixes bug that generated bad digits on images with more than 20000 colors. (thanks, yanele)
parses gtkrc (thanks, yosh)
doesn't load parameter screen on images that don't have alpha

1.2.1 fixes some minor bugs -- spaces in #XXXXXX strings, small typos in code.

1.2 compute color indexes so that we don't have to use XpmSaveXImage*

Previous...Inherited code from Ray Lehtiniemi, who inherited it from S & P.
*/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

static const char linenoise [] =
" .+@#$%&*=-;>,')!~{]^/(_:<[}|1234567890abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ`";

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


typedef struct
{
	guchar r;
	guchar g;
	guchar b;
} rgbkey;

/* whether the image is color or not.  global so I only have to pass one user value
to the GHFunc */
  int       color;
/* bytes per pixel.  global so I only have to pass one user value
to the GHFunc */
  int        cpp;

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
                          "Spencer Kimball & Peter Mattis & Ray Lehtiniemi & Nathan Summers",
                          "Spencer Kimball & Peter Mattis",
                          "1997",
                          "<Save>/Xpm",
                          "RGB*, GRAY*, INDEXED*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_xpm_load", "xpm", "<Load>/Xpm",
		  "0,string,/* XPM */");
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
	  if (gimp_drawable_has_alpha(param[2].data.d_int32))
		  if (! save_dialog ())
		    return;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 4)
	    status = STATUS_CALLING_ERROR;
	  if (status == STATUS_SUCCESS)
	    {
	      xpmvals.threshold = param[4].data.d_float;
	    }
	  if (status == STATUS_SUCCESS &&
	      (xpmvals.threshold < 0.0 || xpmvals.threshold > 1.0))
	    status = STATUS_CALLING_ERROR;

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


guint rgbhash (rgbkey *c)
{
	  return ((guint)c->r) ^ ((guint)c->g) ^ ((guint)c->b);
}

guint compare (rgbkey *c1, rgbkey *c2)
{
	return (c1->r == c2->r)&&(c1->g == c2->g)&&(c1->b == c2->b);
}
	

void set_XpmImage(XpmColor *array, guint index, char *colorstring)
{
	char *p;
	int i, charnum, indtemp;
	
	indtemp=index;
	array[index].string = p = g_new(char, cpp+1);
	
	/*convert the index number to base sizeof(linenoise)-1 */
	for(i=0; i<cpp; ++i)
	{
		charnum = indtemp%(sizeof(linenoise)-1);
		indtemp = indtemp / (sizeof (linenoise)-1);
		*p++=linenoise[charnum];
	}
	/* *p++=linenoise[indtemp]; */
	
	*p='\0'; /* C and its stupid null-terminated strings...*/
		
			
	array[index].symbolic = NULL;
	array[index].m_color = NULL;
	array[index].g4_color = NULL;
	if (color)
	{
		array[index].g_color = NULL;
		array[index].c_color = colorstring;
	} else {	
		array[index].c_color = NULL;
		array[index].g_color = colorstring;
	}
}

void create_colormap_from_hash (gpointer gkey, gpointer value, gpointer
		user_data)
{
	rgbkey *key = gkey;
	char *string = g_new(char, 8);
	sprintf(string, "#%02X%02X%02X", (int)key->r, (int)key->g, (int)key->b);
	set_XpmImage(user_data, *((int *) value), string);
}

static gint
save_image (char   *filename,
            gint32  image_ID,
            gint32  drawable_ID)
{
  GDrawable *drawable;
    
  int       width;
  int       height;
  int       alpha;
  int 	    ncolors=1;
  int	    *indexno;
  int 	    indexed;
  
  XpmColor  *colormap;
  XpmImage  *image;
    
  guint   *ibuff   = NULL;
  /*guint   *mbuff   = NULL;*/
  GPixelRgn  pixel_rgn;
  guchar    *buffer;
  guchar    *data;

  GHashTable *hash = NULL;
  
  int        i, j, k;
  int        threshold = 255 * xpmvals.threshold;

  gint      rc = FALSE;

  /* get some basic stats about the image */
  switch (gimp_drawable_type (drawable_ID)) {
  case RGBA_IMAGE:
  case INDEXEDA_IMAGE:
  case GRAYA_IMAGE:
    alpha = 1;
    break;
  case RGB_IMAGE:
  case INDEXED_IMAGE:
  case GRAY_IMAGE:
    alpha = 0;
    break;
  default:
    return FALSE;
  }

  switch (gimp_drawable_type (drawable_ID)) {
  case GRAYA_IMAGE:
  case GRAY_IMAGE:
    color = 0;
    break;
  case RGBA_IMAGE:
  case RGB_IMAGE:
  case INDEXED_IMAGE:
  case INDEXEDA_IMAGE:	  	  
    color = 1;
    break;
  default:
    return FALSE;
  }

  switch (gimp_drawable_type (drawable_ID)) {
  case GRAYA_IMAGE:
  case GRAY_IMAGE:
  case RGBA_IMAGE:
  case RGB_IMAGE:
    indexed = 0;
    break;
  case INDEXED_IMAGE:
  case INDEXEDA_IMAGE:	  	  
    indexed = 1;
    break;
  default:
    return FALSE;
  }

  drawable = gimp_drawable_get (drawable_ID);
  width    = drawable->width;
  height   = drawable->height;

  /* allocate buffers making the assumption that ibuff and mbuff
     are 32 bit aligned... */
  if ((ibuff = g_new(guint, width*height)) == NULL)
    goto cleanup;

  /*if ((mbuff = g_new(guint, width*height)) == NULL)
    goto cleanup;*/
  
  if ((hash = g_hash_table_new((GHashFunc)rgbhash, (GCompareFunc) compare)) == NULL)
    goto cleanup;
  
  /* put up a progress bar */
  {
    char *name = g_new (char, strlen (filename) + 12);
    if (!name)
      gimp_quit();
    sprintf (name, "Saving %s:", filename);
    gimp_progress_init (name);
    free (name);
  }

  
  /* allocate a pixel region to work with */
  if ((buffer = g_new(guchar, gimp_tile_height()*width*drawable->bpp)) == NULL)
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
          guint *idata = ibuff + (i+j) * width;
          /*guint *mdata = mbuff + (i+j) * width;*/

          /* do each pixel in the row */
          for (k=0; k<width; k++)
            {
	      rgbkey *key = g_new(rgbkey, 1);
	      guchar a;
  
              /* get pixel data */
              key->r = *(data++);
	      key->g = color && !indexed ? *(data++) : key->r;
	      key->b = color && !indexed ? *(data++) : key->r;
	      a = alpha ? *(data++) : 255;
	      
	      if (a < threshold) 
		      *(idata++) = 0;
	      else
		      if (indexed)
			      *(idata++) = (key->r)+1;
		      else {
		      	indexno = g_hash_table_lookup(hash, key);
			      if (!indexno) {
				      indexno = g_new(int, 1);
				      *indexno = ncolors++;
				      g_hash_table_insert(hash, key, indexno);
			      	key = g_new(rgbkey, 1);
		      		}
		      *(idata++) = *indexno;
		  }
            }
      
          /* kick the progress bar */
          gimp_progress_update ((double) (i+j) / (double) height);
        }
    
  } 
  g_free(buffer);

  if (indexed)
  {
	  guchar *cmap;
	  cmap = gimp_image_get_cmap(image_ID, &ncolors);
	  ncolors++; /* for transparency */
  	  colormap = g_new(XpmColor, ncolors);
	  cpp=(double)1.0+(double)log(ncolors)/(double)log(sizeof(linenoise)-1.0);
	  set_XpmImage(colormap, 0, "None");
	  for (i=0; i<ncolors-1; i++)
	  {
		  char *string;
		  guchar r, g, b;
		  r = *(cmap++);
		  g = *(cmap++);
		  b = *(cmap++);
		  string = g_new(char, 8);
	          sprintf(string, "#%02X%02X%02X", (int)r, (int)g, (int)b);
		  set_XpmImage(colormap, i+1, string);
	  } 
  }else
  {
	  colormap = g_new(XpmColor, ncolors);
	  
	  cpp=(double)1.0+(double)log(ncolors)/(double)log(sizeof(linenoise)-1.0);
	  set_XpmImage(colormap, 0, "None");

	  g_hash_table_foreach (hash, create_colormap_from_hash, colormap);
  }
    
  image = g_new(XpmImage, 1);
  
  image->width=width;
  image->height=height;
  image->ncolors=ncolors;
  image->cpp=cpp;
  image->colorTable=colormap;	    
  image->data = ibuff;
  
      /* do the save */
      XpmWriteFileFromXpmImage(filename, image, NULL);
      rc = TRUE;

  
 cleanup:

  /* clean up resources */  
  gimp_drawable_detach (drawable);
  
  
  if (ibuff) g_free(ibuff);
  /*if (mbuff) g_free(mbuff);*/
  if (hash)  g_hash_table_destroy(hash);
  
  return rc;
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
  gtk_rc_parse(gimp_gtkrc());

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
