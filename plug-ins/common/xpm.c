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

/* XPM plugin version 1.2.3 */

/*
1.2.3 fixes bug when running in noninteractive mode
changes alpha_threshold range from [0, 1] to [0,255] for consistency with
the threshold_alpha plugin

1.2.2 fixes bug that generated bad digits on images with more than 20000
colors. (thanks, yanele)
parses gtkrc (thanks, yosh)
doesn't load parameter screen on images that don't have alpha

1.2.1 fixes some minor bugs -- spaces in #XXXXXX strings, small typos in code.

1.2 compute color indexes so that we don't have to use XpmSaveXImage*

Previous...Inherited code from Ray Lehtiniemi, who inherited it from S & P.
*/

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/xpm.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


static const gchar linenoise [] =
" .+@#$%&*=-;>,')!~{]^/(_:<[}|1234567890abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ`";

#define SCALE_WIDTH 125

/* Structs for the save dialog */
typedef struct
{
  gint threshold;
} XpmSaveVals;

typedef struct
{
  gint run;
} XpmSaveInterface;


typedef struct
{
  guchar r;
  guchar g;
  guchar b;
} rgbkey;

/*  whether the image is color or not.  global so I only have to pass
 *  one user value to the GHFunc
 */
gboolean   color;

/*  bytes per pixel.  global so I only have to pass one user value
 *  to the GHFunc
 */
gint       cpp;

/* Declare local functions */
static void     query               (void);
static void     run                 (gchar         *name,
				     gint           nparams,
				     GParam        *param,
				     gint          *nreturn_vals,
				     GParam       **return_vals);

static gint32   load_image          (gchar         *filename);
static void     parse_colors        (XpmImage      *xpm_image,
				     guchar       **cmap);
static void     parse_image         (gint32         image_ID,
				     XpmImage      *xpm_image,
				     guchar        *cmap);
static gboolean save_image          (gchar         *filename,
				     gint32         image_ID,
				     gint32         drawable_ID);

static gint     save_dialog         (void);
static void     save_ok_callback    (GtkWidget     *widget,
				     gpointer       data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static XpmSaveVals xpmvals = 
{
  127  /* alpha threshold */
};

static XpmSaveInterface xpmint =
{
  FALSE   /*  run  */
};


MAIN ()

static void
query (void)
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32,     "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING,    "filename",     "The name of the file to load" },
    { PARAM_STRING,    "raw_filename", "The name entered" }
  };
  static gint nload_args        = sizeof (load_args) / sizeof (load_args[0]);

  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE,    "image",         "Output image" }
  };
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GParamDef save_args[] =
  {
    { PARAM_INT32,    "run_mode",      "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",         "Input image" },
    { PARAM_DRAWABLE, "drawable",      "Drawable to save" },
    { PARAM_STRING,   "filename",      "The name of the file to save the image in" },
    { PARAM_STRING,   "raw_filename",  "The name of the file to save the image in" },
    { PARAM_INT32,    "threshold",     "Alpha threshold (0-255)" }
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

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

  gimp_register_magic_load_handler ("file_xpm_load",
				    "xpm",
				    "<Load>/Xpm",
				    "0, string,/*\\040XPM\\040*/");
  gimp_register_save_handler ("file_xpm_save",
			      "xpm",
			      "<Save>/Xpm");
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType  run_mode;
  GStatusType   status = STATUS_SUCCESS;
  gint32        image_ID;
  gint32        drawable_ID;
  GimpExportReturnType export = EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  if (strcmp (name, "file_xpm_load") == 0)
    {
      INIT_I18N();

      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[1].type         = PARAM_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = STATUS_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, "file_xpm_save") == 0)
    {
      INIT_I18N_UI();

      gimp_ui_init ("xpm", FALSE);

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  export = gimp_export_image (&image_ID, &drawable_ID, "XPM", 
				      (CAN_HANDLE_RGB |
				       CAN_HANDLE_GRAY |
				       CAN_HANDLE_INDEXED |
				       CAN_HANDLE_ALPHA));
	  if (export == EXPORT_CANCEL)
	    {
	      values[0].data.d_status = STATUS_CANCEL;
	      return;
	    }
	  break;
	default:
	  break;
	}

      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_xpm_save", &xpmvals);

	  /*  First acquire information with a dialog  */
	  if (gimp_drawable_has_alpha (drawable_ID))
	    if (! save_dialog ())
	      status = STATUS_CANCEL;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 6)
	    {
	      status = STATUS_CALLING_ERROR;
	    }
	  else
	    {
	      xpmvals.threshold = param[5].data.d_int32;

	      if (xpmvals.threshold < 0 ||
		  xpmvals.threshold > 255)
		status = STATUS_CALLING_ERROR;
	    }
	  break;

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_xpm_save", &xpmvals);
	  break;

	default:
	  break;
	}

      if (status == STATUS_SUCCESS)
	{
	  if (save_image (param[3].data.d_string,
			  image_ID,
			  drawable_ID))
	    {
	      gimp_set_data ("file_xpm_save", &xpmvals, sizeof (XpmSaveVals));
	    }
	  else
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}

      if (export == EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = STATUS_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}


static gint32
load_image (gchar *filename)
{
  XpmImage  xpm_image;
  guchar   *cmap;
  gint32    image_ID;
  gchar    *name;

  /* put up a progress bar */
  name = g_strdup_printf (_("Loading %s:"), filename);
  gimp_progress_init (name);
  g_free (name);

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
parse_colors (XpmImage  *xpm_image, 
	      guchar   **cmap)
{
  Display  *display;
  Colormap  colormap;
  gint      i, j;

  /* open the display and get the default color map */
  display  = XOpenDisplay (NULL);
  colormap = DefaultColormap (display, DefaultScreen (display));
    
  /* alloc a buffer to hold the parsed colors */
  *cmap = g_new (guchar, sizeof (guchar) * 4 * xpm_image->ncolors);

  if ((*cmap) != NULL)
    {
      /* default to black transparent */
      memset((void*)(*cmap), 0, sizeof (guchar) * 4 * xpm_image->ncolors);
      
      /* parse each color in the file */
      for (i = 0, j = 0; i < xpm_image->ncolors; i++)
        {
          gchar     *colorspec = "None";
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
          if (strcmp (colorspec, "None") != 0)
	    {
	      XParseColor (display, colormap, colorspec, &xcolor);
	      (*cmap)[j++] = xcolor.red >> 8;
	      (*cmap)[j++] = xcolor.green >> 8;
	      (*cmap)[j++] = xcolor.blue >> 8;
	      (*cmap)[j++] = ~0;
	    }
	  else
	    {
	      j += 4;
	    }
        }
    }
    
  XCloseDisplay (display);
}

static void
parse_image (gint32    image_ID, 
	     XpmImage *xpm_image, 
	     guchar   *cmap)
{
  gint       tile_height;
  gint       scanlines;
  gint       val;  
  guchar    *buf;
  guchar    *dest;
  guint     *src;
  GPixelRgn  pixel_rgn;
  GDrawable *drawable;
  gint32     layer_ID;
  gint       i, j;
    
  layer_ID = gimp_layer_new (image_ID,
                             _("Color"),
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

guint
rgbhash (rgbkey *c)
{
  return ((guint)c->r) ^ ((guint)c->g) ^ ((guint)c->b);
}

guint
compare (rgbkey *c1, 
	 rgbkey *c2)
{
  return (c1->r == c2->r) && (c1->g == c2->g) && (c1->b == c2->b);
}
	
void
set_XpmImage (XpmColor *array, 
	      guint     index, 
	      gchar    *colorstring)
{
  gchar *p;
  gint i, charnum, indtemp;
  
  indtemp=index;
  array[index].string = p = g_new (gchar, cpp+1);
  
  /*convert the index number to base sizeof(linenoise)-1 */
  for (i=0; i<cpp; ++i)
    {
      charnum = indtemp % (sizeof (linenoise) - 1);
      indtemp = indtemp / (sizeof (linenoise) - 1);
      *p++ = linenoise[charnum];
    }
  /* *p++=linenoise[indtemp]; */
  
  *p = '\0'; /* C and its stupid null-terminated strings... */

  array[index].symbolic = NULL;
  array[index].m_color  = NULL;
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

void
create_colormap_from_hash (gpointer gkey, 
			   gpointer value, 
			   gpointer user_data)
{
  rgbkey *key = gkey;
  gchar *string = g_new(char, 8);

  sprintf (string, "#%02X%02X%02X", (int)key->r, (int)key->g, (int)key->b);
  set_XpmImage (user_data, *((int *) value), string);
}

static gboolean
save_image (gchar  *filename,
            gint32  image_ID,
            gint32  drawable_ID)
{
  GDrawable *drawable;    

  gint       width;
  gint       height;
  gint 	     ncolors = 1;
  gint	    *indexno;
  gboolean   indexed;
  gboolean   alpha;

  XpmColor  *colormap;
  XpmImage  *image;

  guint     *ibuff   = NULL;
  /*guint   *mbuff   = NULL;*/
  GPixelRgn  pixel_rgn;
  guchar    *buffer;
  guchar    *data;

  GHashTable *hash = NULL;

  gint       i, j, k;
  gint       threshold = xpmvals.threshold;

  gboolean   rc = FALSE;

  /* get some basic stats about the image */
  switch (gimp_drawable_type (drawable_ID)) 
    {
    case RGBA_IMAGE:
    case INDEXEDA_IMAGE:
    case GRAYA_IMAGE:
      alpha = TRUE;
      break;
    case RGB_IMAGE:
    case INDEXED_IMAGE:
    case GRAY_IMAGE:
      alpha = FALSE;
      break;
    default:
      return FALSE;
    }

  switch (gimp_drawable_type (drawable_ID)) 
    {
    case GRAYA_IMAGE:
    case GRAY_IMAGE:
      color = FALSE;
      break;
    case RGBA_IMAGE:
    case RGB_IMAGE:
    case INDEXED_IMAGE:
    case INDEXEDA_IMAGE:	  	  
    color = TRUE;
    break;
    default:
      return FALSE;
    }
  
  switch (gimp_drawable_type (drawable_ID)) 
    {
    case GRAYA_IMAGE:
    case GRAY_IMAGE:
    case RGBA_IMAGE:
    case RGB_IMAGE:
      indexed = FALSE;
      break;
    case INDEXED_IMAGE:
    case INDEXEDA_IMAGE:	  	  
      indexed = TRUE;
      break;
    default:
      return FALSE;
    }
  
  drawable = gimp_drawable_get (drawable_ID);
  width    = drawable->width;
  height   = drawable->height;
  
  /* allocate buffers making the assumption that ibuff and mbuff
     are 32 bit aligned... */
  if ((ibuff = g_new (guint, width * height)) == NULL)
    goto cleanup;

  /*if ((mbuff = g_new(guint, width*height)) == NULL)
    goto cleanup;*/
  
  if ((hash = g_hash_table_new ((GHashFunc)rgbhash, 
				(GCompareFunc) compare)) == NULL)
    goto cleanup;
  
  /* put up a progress bar */
  {
    gchar *name;

    name = g_strdup_printf (_("Saving %s:"), filename);
    gimp_progress_init (name);
    g_free (name);
  }

  
  /* allocate a pixel region to work with */
  if ((buffer = g_new (guchar, 
		       gimp_tile_height()*width*drawable->bpp)) == NULL)
    return 0;

  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0,
                       width, height,
                       TRUE, FALSE);

  /* process each row of tiles */
  for (i = 0; i < height; i+=gimp_tile_height())
    {
      gint scanlines;
  
      /* read the next row of tiles */
      scanlines = MIN (gimp_tile_height(), height - i);
      gimp_pixel_rgn_get_rect (&pixel_rgn, buffer, 0, i, width, scanlines);
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
	      rgbkey *key = g_new (rgbkey, 1);
	      guchar a;
  
              /* get pixel data */
              key->r = *(data++);
	      key->g = color && !indexed ? *(data++) : key->r;
	      key->b = color && !indexed ? *(data++) : key->r;
	      a = alpha ? *(data++) : 255;
	      
	      if (a < threshold) 
		{
		  *(idata++) = 0;
		}
	      else
		{
		  if (indexed)
		    {
		      *(idata++) = (key->r)+1;
		    }
		  else
		    {
		      indexno = g_hash_table_lookup (hash, key);
		      if (!indexno)
			{
			  indexno = g_new (gint, 1);
			  *indexno = ncolors++;
			  g_hash_table_insert (hash, key, indexno);
			  key = g_new (rgbkey, 1);
			}
		      *(idata++) = *indexno;
		    }
		}
            }

          /* kick the progress bar */
          gimp_progress_update ((gdouble) (i+j) / (gdouble) height);
        }    
    } 
  g_free (buffer);

  if (indexed)
    {
      guchar *cmap;
      cmap = gimp_image_get_cmap(image_ID, &ncolors);
      ncolors++; /* for transparency */
      colormap = g_new (XpmColor, ncolors);
      cpp = (gdouble) 1.0 + 
	(gdouble) log (ncolors) / (double) log (sizeof (linenoise)-1.0);
      set_XpmImage (colormap, 0, "None");
      for (i=0; i<ncolors-1; i++)
	{
	  gchar *string;
	  guchar r, g, b;
	  r = *(cmap++);
	  g = *(cmap++);
	  b = *(cmap++);
	  string = g_new (gchar, 8);
	  sprintf (string, "#%02X%02X%02X", (int)r, (int)g, (int)b);
	  set_XpmImage (colormap, i+1, string);
	} 
    }
  else
    {
      colormap = g_new (XpmColor, ncolors);
      
      cpp = (gdouble) 1.0 + 
	(gdouble) log (ncolors) / (gdouble) log (sizeof (linenoise) - 1.0);
      set_XpmImage (colormap, 0, "None");
      
      g_hash_table_foreach (hash, create_colormap_from_hash, colormap);
    }

  image = g_new (XpmImage, 1);
  
  image->width=width;
  image->height=height;
  image->ncolors=ncolors;
  image->cpp=cpp;
  image->colorTable=colormap;	    
  image->data = ibuff;
  
  /* do the save */
  XpmWriteFileFromXpmImage (filename, image, NULL);
  rc = TRUE;

 cleanup:
  /* clean up resources */  
  gimp_drawable_detach (drawable);
  
  if (ibuff) 
    g_free (ibuff);
  /*if (mbuff) g_free(mbuff);*/
  if (hash)  
    g_hash_table_destroy (hash);
  
  return rc;
}

static gint
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkObject *scale_data;

  dlg = gimp_dialog_new (_("Save as XPM"), "xpm",
			 gimp_standard_help_func, "filters/xpm.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), save_ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /*  parameter settings  */
  frame = gtk_frame_new (_("Parameter Settings"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 4);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  scale_data = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
				     _("Alpha Threshold:"), SCALE_WIDTH, 0,
				     xpmvals.threshold, 0, 255, 1, 8, 0,
				     TRUE, 0, 0,
				     NULL, NULL);

  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &xpmvals.threshold);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return xpmint.run;
}

static void
save_ok_callback (GtkWidget *widget,
		  gpointer   data)
{
  xpmint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
