/*
 * GFLI 1.3
 *
 * A gimp plug-in to read and write FLI and FLC movies.
 *
 * Copyright (C) 1998 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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
 *
 * This is a first loader for FLI and FLC movies. It uses as the same method as
 * the gif plug-in to store the animation (i.e. 1 layer/frame).
 * 
 * Current disadvantages:
 * - Generates A LOT OF warnings.
 * - Consumes a lot of memory (See wish-list: use the original data or 
 *   compression).
 * - doesn't support palette changes between two frames.
 *
 * Wish-List:
 * - I'd like to have a different format for storing animations, so I can use
 *   Layers and Alpha-Channels for effects. An older version of 
 *   this plug-in created one image per frame, and went real soon out of
 *   memory.
 * - I'd like a method that requests unmodified frames from the original
 *   image, and stores modified without destroying the original file.
 * - I'd like a way to store additional information about a image to it, for
 *   example copyright stuff or a timecode.
 * - I've thought about a small utility to mix MIDI events as custom chunks
 *   between the FLI chunks. Anyone interested in implementing this ?
 */

/*
 * History:
 * 1.0 first release
 * 1.1 first support for FLI saving (BRUN and LC chunks)
 * 1.2 support for load/save ranges, fixed SGI & SUN problems (I hope...), fixed FLC
 * 1.3 made saving actually work, alpha channel is silently ignored; 
       loading was broken too, fixed it  --Sven
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "fli.h"

#include "libgimp/stdplugins-intl.h"


static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

/* return the image-ID of the new image, or -1 in case of an error */
static gint32 load_image      (gchar  *filename,
			       gint32  from_frame,
			       gint32  to_frame);
static gint   load_dialog     (gchar  *name);

/* return TRUE or FALSE */
static gint   save_image      (gchar  *filename,
			       gint32  image_id,
			       gint32  from_frame,
			       gint32  to_frame);
static gint   save_dialog     (gint32  image_id);

/* return TRUE or FALSE */
static gint   get_info        (gchar  *filename,
			       gint32 *width,
			       gint32 *height,
			       gint32 *frames);

/*
 * GIMP interface
 */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

GParamDef load_args[] =
{
  { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
  { PARAM_STRING, "filename", "The name of the file to load" },
  { PARAM_STRING, "raw_filename", "The name entered" },
  { PARAM_INT32, "from_frame", "Load beginning from this frame" },
  { PARAM_INT32, "to_frame", "End loading with this frame" },
};
GParamDef load_return_vals[] =
{
  { PARAM_IMAGE, "image", "Output image" },
};
gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
gint nload_return_vals = (sizeof (load_return_vals) /
			  sizeof (load_return_vals[0]));

GParamDef save_args[] =
{
  { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
  { PARAM_IMAGE, "image", "Input image" },
  { PARAM_DRAWABLE, "drawable", "Input drawable (unused)" },
  { PARAM_STRING, "filename", "The name of the file to save" },
  { PARAM_STRING, "raw_filename", "The name entered" },
  { PARAM_INT32, "from_frame", "Save beginning from this frame" },
  { PARAM_INT32, "to_frame", "End saving with this frame" },
};
gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

GParamDef info_args[] =
{
  { PARAM_STRING, "filename", "The name of the file to get info" },
};

GParamDef info_return_vals[] =
{
  { PARAM_INT32, "width", "Width of one frame" },
  { PARAM_INT32, "height", "Height of one frame" },
  { PARAM_INT32, "frames", "Number of Frames" },
};
gint ninfo_args = sizeof (info_args) / sizeof (info_args[0]);
gint ninfo_return_vals = (sizeof (info_return_vals) /
			  sizeof (info_return_vals[0]));


static gint32 from_frame;
static gint32 to_frame;

MAIN ()

static void
query (void)
{
  /*
   * Load/save procedures
   */
  gimp_install_procedure ("file_fli_load_frames",
			  "load FLI-movies",
			  "This is an experimantal plug-in to handle FLI movies",
			  "Jens Ch. Restemeier",
			  "Jens Ch. Restemeier",
			  "1997",
			  "<Load>/FLI",
			  NULL,
			  PROC_PLUG_IN,
			  nload_args, nload_return_vals,
			  load_args, load_return_vals);

  gimp_install_procedure ("file_fli_load",
			  "load FLI-movies",
			  "This is an experimantal plug-in to handle FLI movies",
			  "Jens Ch. Restemeier",
			  "Jens Ch. Restemeier",
			  "1997",
			  "<Load>/FLI",
			  NULL,
			  PROC_PLUG_IN,
			  nload_args - 2, nload_return_vals,
			  load_args, load_return_vals);

  gimp_register_magic_load_handler ("file_fli_load",
				    "fli",
				    "",
				    "");
  
  gimp_install_procedure ("file_fli_save",
			  "save FLI-movies",
			  "This is an experimantal plug-in to handle FLI movies",
			  "Jens Ch. Restemeier",
			  "Jens Ch. Restemeier",
			  "1997",
			  "<Save>/FLI",
			  "INDEXED,GRAY",
			  PROC_PLUG_IN,
			  nsave_args, 0,
			  save_args, NULL);

  gimp_register_save_handler ("file_fli_save",
			      "fli",
			      "");


  /*
   * Utility functions:
   */
  gimp_install_procedure ("file_fli_info",
			  "Get info about a Fli movie",
			  "This is a experimantal plug-in to handle FLI movies",
			  "Jens Ch. Restemeier",
			  "Jens Ch. Restemeier",
			  "1997",
			  NULL,
			  NULL,
			  PROC_EXTENSION,
			  ninfo_args, ninfo_return_vals,
			  info_args, info_return_vals);
}

GParam values[5];

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  GStatusType  status = STATUS_SUCCESS;
  GRunModeType run_mode;
  gint32       pc;
  gint32       image_ID;
  gint32       drawable_ID;
  gint32       orig_image_ID;
  GimpExportReturnType export = EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_EXECUTION_ERROR;

  if (strncmp (name, "file_fli_load", strlen("file_fli_load")) == 0)
    {
      INIT_I18N_UI ();

      switch (run_mode)
	{
	case RUN_NONINTERACTIVE:
	  /*
	   * check for valid parameters: 
	   * (Or can I trust GIMP ?)
	   */
	  if ((nparams < nload_args - 2) || (nload_args < nparams))
	    {
	      status = STATUS_CALLING_ERROR;
	      break;
	    }
	  for (pc = 0; pc < nload_args - 2; pc++)
	    {
	      if (load_args[pc].type != param[pc].type)
		{
		  status = STATUS_CALLING_ERROR;
		  break;
		}
	    }
	  for (pc = nload_args - 2; pc < nparams; pc++)
	    {
	      if (load_args[pc].type != param[pc].type)
		{
		  status = STATUS_CALLING_ERROR;
		  break;
		}
	    }

	  to_frame   = (nparams < nload_args - 1) ? 1 : param[3].data.d_int32;
	  from_frame = (nparams < nload_args) ? -1 : param[4].data.d_int32;
	  image_ID = load_image (param[1].data.d_string,
				 from_frame, to_frame);

	  if (image_ID != -1)
	    {
	      *nreturn_vals = 2;
	      values[1].type         = PARAM_IMAGE;
	      values[1].data.d_image = image_ID;
	    }
	  else
	    status = STATUS_EXECUTION_ERROR;
	  break;

	case RUN_INTERACTIVE:
	  if (load_dialog (param[1].data.d_string))
	    {
	      image_ID = load_image (param[1].data.d_string,
				     from_frame, to_frame);

	      if (image_ID != -1)
		{
		  *nreturn_vals = 2;
		  values[1].type         = PARAM_IMAGE;
		  values[1].data.d_image = image_ID;
		}
	      else
		status = STATUS_EXECUTION_ERROR;
	    }
	  else
	    status = STATUS_CANCEL;
	  break;

	case RUN_WITH_LAST_VALS:
	  status = STATUS_CALLING_ERROR;
	  break;
	}
    }  
  else if (strcmp (name, "file_fli_save") == 0)
    {
      INIT_I18N_UI ();

      image_ID    = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      switch (run_mode)
	{
	case RUN_NONINTERACTIVE:
	  if (nparams!=nsave_args)
	    {
	      status = STATUS_CALLING_ERROR;
	      break;
	    }
	  for (pc = 0; pc < nsave_args; pc++)
	    {
	      if (save_args[pc].type!=param[pc].type)
		{
		  status = STATUS_CALLING_ERROR;
		  break;
		}
	    }
	  if (! save_image (param[3].data.d_string, image_ID,
			    param[5].data.d_int32,
			    param[6].data.d_int32))
	    status = STATUS_EXECUTION_ERROR;
	  break;

	case RUN_INTERACTIVE:
	case RUN_WITH_LAST_VALS:
	  gimp_ui_init ("gfli", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "FLI", 
				      (CAN_HANDLE_INDEXED |
				       CAN_HANDLE_GRAY | 
				       CAN_HANDLE_ALPHA |
				       CAN_HANDLE_LAYERS));
	  if (export == EXPORT_CANCEL)
	    {
	      values[0].data.d_status = STATUS_CANCEL;
	      return;
	    }

	  if (save_dialog (param[1].data.d_image))
	    {
	      if (! save_image (param[3].data.d_string, image_ID, from_frame, to_frame))
		status = STATUS_EXECUTION_ERROR;
	    }
	  else
	    status = STATUS_CANCEL;
	  break;
	}

      if (export == EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else if (strcmp (name, "file_fli_info") == 0)
    {
      gint32 width, height, frames;

      INIT_I18N_UI ();

      /*
       * check for valid parameters;
       */
      if (nparams != ninfo_args)
	status = STATUS_CALLING_ERROR;

      if (status == STATUS_SUCCESS)
	{
	  for (pc = 0; pc < nsave_args; pc++)
	    {
	      if (info_args[pc].type != param[pc].type)
		{
		  status = STATUS_CALLING_ERROR;
		  break;
		}
	    }
	}

      if (status == STATUS_SUCCESS)
	{
	  if (get_info (param[0].data.d_string, &width, &height, &frames))
	    {
	      *nreturn_vals = 4;
	      values[1].type = PARAM_INT32;
	      values[1].data.d_int32 = width;
	      values[2].type = PARAM_INT32;
	      values[2].data.d_int32 = height;
	      values[3].type = PARAM_INT32;
	      values[3].data.d_int32 = frames;
	    }
	  else
	    status = STATUS_EXECUTION_ERROR;
	}
    }
  else
    status = STATUS_CALLING_ERROR;

  values[0].data.d_status = status;
}

/*
 * Open FLI animation and return header-info
 */
gint
get_info (gchar  *filename,
	  gint32 *width,
	  gint32 *height,
	  gint32 *frames)
{
  FILE *file;
  s_fli_header fli_header;

  *width = 0; *height = 0; *frames = 0;

  file = fopen (filename ,"rb");

  if (!file)
    {
      g_message (_("FLI: Can't open \"%s\""), filename);
      return FALSE;
    }
  fli_read_header (file, &fli_header);
  fclose (file);
  *width = fli_header.width;
  *height = fli_header.height;
  *frames = fli_header.frames;

  return TRUE;
}

/*
 * load fli animation and store as framestack
 */
gint32
load_image (gchar  *filename,
	    gint32  from_frame,
	    gint32  to_frame)
{
  FILE *file;
  gchar *name_buf;
  GDrawable *drawable;
  gint32 image_id, layer_ID;
  	
  guchar *fb, *ofb, *fb_x;
  guchar cm[768], ocm[768];
  GPixelRgn pixel_rgn;
  s_fli_header fli_header;

  gint cnt;

  name_buf = g_strdup_printf (_("Loading %s:"), filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  file = fopen (filename ,"rb");
  if (!file)
    {
      g_message (_("FLI: Can't open \"%s\""), filename);
      return -1;
    }
  fli_read_header (file, &fli_header);
  if (fli_header.magic == NO_HEADER)
    return -1;
  else
    fseek (file, 128, SEEK_SET);

  /*
   * Fix parameters
   */
  if ((from_frame==-1) && (to_frame==-1))
    {
      /* to make scripting easier: */
      from_frame=1; to_frame=fli_header.frames;
    }
  if (to_frame<from_frame)
    {
      to_frame = fli_header.frames;
    }
  if (from_frame < 1)
    {
      from_frame = 1;
    }
  if (to_frame < 1)
    {
      /* nothing to do ... */
      return -1;
    }
  if (from_frame >= fli_header.frames)
    {
      /* nothing to do ... */
      return -1;
    }
  if (to_frame>fli_header.frames)
    {
      to_frame = fli_header.frames;
    }

  image_id = gimp_image_new (fli_header.width, fli_header.height, INDEXED);
  gimp_image_set_filename (image_id, filename);

  fb = g_malloc (fli_header.width * fli_header.height);
  ofb = g_malloc (fli_header.width * fli_header.height);

  /*
   * Skip to the beginning of requested frames: 
   */
  for (cnt = 1; cnt < from_frame; cnt++)
    {
      fli_read_frame (file, &fli_header, ofb, ocm, fb, cm);
      memcpy (ocm, cm, 768);
      fb_x = fb; fb = ofb; ofb = fb_x;
    }
  /*
   * Load range
   */
  for (cnt = from_frame; cnt <= to_frame; cnt++)
    {
      name_buf = g_strdup_printf (_("Frame (%i)"), cnt); 
      layer_ID = gimp_layer_new (image_id, name_buf,
				 fli_header.width, fli_header.height,
				 INDEXED_IMAGE, 100, NORMAL_MODE);
      g_free (name_buf);

      drawable = gimp_drawable_get (layer_ID);

      fli_read_frame (file, &fli_header, ofb, ocm, fb, cm);

      gimp_pixel_rgn_init (&pixel_rgn, drawable,
			   0, 0, fli_header.width, fli_header.height,
			   TRUE, FALSE);
      gimp_pixel_rgn_set_rect (&pixel_rgn, fb,
			       0, 0, fli_header.width, fli_header.height);

      gimp_drawable_flush (drawable);
      gimp_drawable_detach (drawable);

      if (cnt > 0)
	gimp_layer_add_alpha (layer_ID);

      gimp_image_add_layer (image_id, layer_ID, 0);

      if (cnt < to_frame)
	{
	  memcpy (ocm, cm, 768);
	  fb_x = fb; fb = ofb; ofb = fb_x;
	}
      
      gimp_progress_update ((double) cnt + 1 / (double)(to_frame - from_frame));
    }

  gimp_image_set_cmap (image_id, cm, 256);

  fclose (file);

  g_free (fb);
  g_free (ofb);

  return image_id;
}


#define MAXDIFF 195075    /*  3 * SQR (255) + 1  */

/*
 * get framestack and store as fli animation
 * (some code was taken from the GIF plugin.)
 */ 
gint
save_image (gchar  *filename,
	    gint32  image_id,
	    gint32  from_frame,
	    gint32  to_frame)
{
  FILE *file;
  gchar *name_buf;
  GDrawable *drawable;
  gint32 *framelist;
  gint nframes;
  gint colors, i;
  guchar *cmap;
  guchar bg;
  guchar red, green, blue;
  gint diff, sum, max;
  gint offset_x, offset_y, xc, yc, xx, yy;
  guint rows, cols, bytes;
  guchar *src_row;
  guchar *fb, *ofb;
  guchar cm[768];
  GPixelRgn pixel_rgn;
  s_fli_header fli_header;

  gint cnt;

  framelist = gimp_image_get_layers (image_id, &nframes);
	
  if ((from_frame == -1) && (to_frame == -1))
    {
      /* to make scripting easier: */
      from_frame = 0; to_frame = nframes;
    }
  if (to_frame < from_frame)
    {
      to_frame = nframes;
    }
  if (from_frame < 1)
    {
      from_frame = 1;
    }
  if (to_frame < 1)
    {
      /* nothing to do ... */
      return FALSE;
    }
  if (from_frame > nframes)
    {
      /* nothing to do ... */
      return FALSE;
    }
  if (to_frame > nframes)
    {
      to_frame = nframes;
    }

  gimp_palette_get_background (&red, &green, &blue);

  switch (gimp_image_base_type (image_id))
    {
    case GIMP_GRAY:
      /* build grayscale palette */
      for (i = 0; i < 256; i++)
	{
	  cm[i*3+0] = cm[i*3+1] = cm[i*3+2] = i;
	}
      bg = INTENSITY (red, green, blue);
      break;

    case GIMP_INDEXED:
      max = MAXDIFF;
      bg = 0;
      cmap = gimp_image_get_cmap (image_id, &colors);
      for (i = 0; i < MIN (colors, 256); i++)
	{
	  cm[i*3+0] = cmap[i*3+0];
	  cm[i*3+1] = cmap[i*3+1];
	  cm[i*3+2] = cmap[i*3+2];
	  
	  diff = red - cm[i*3+0];
	  sum = SQR (diff);
	  diff = green - cm[i*3+1];
	  sum +=  SQR (diff);
	  diff = blue - cm[i*3+2];
	  sum += SQR (diff);

	  if (sum < max)
	    {
	      bg = i;
	      max = sum;
	    }
	}
      for (i = colors; i < 256; i++)
	{
	  cm[i*3+0] = cm[i*3+1] = cm[i*3+2] = i;
	}
      break;
      
    default:
      g_message (_("FLI: Sorry, I can save only INDEXED and GRAY images."));
      return FALSE;
    }

  name_buf = g_strdup_printf (_("Saving %s:"), filename);
  gimp_progress_init (name_buf);
  g_free (name_buf);

  /*
   * First build the fli header.
   */
  fli_header.filesize = 0;	/* will be fixed when writing the header */
  fli_header.frames   = 0; 	/* will be fixed during the write */
  fli_header.width    = gimp_image_width (image_id);
  fli_header.height   = gimp_image_height (image_id);

  if ((fli_header.width == 320) && (fli_header.height == 200))
    {
      fli_header.magic = HEADER_FLI;
    }
  else
    {
      fli_header.magic = HEADER_FLC;
    }
  fli_header.depth    = 8;  /* I've never seen a depth != 8 */
  fli_header.flags    = 3;
  fli_header.speed    = 1000 / 25;
  fli_header.created  = 0;  /* program ID. not neccessary... */
  fli_header.updated  = 0;  /* date in MS-DOS format. ignore...*/
  fli_header.aspect_x = 1;  /* aspect ratio. Will be added as soon.. */
  fli_header.aspect_y = 1;  /* ... as GIMP supports it. */
  fli_header.oframe1  = fli_header.oframe2 = 0; /* will be fixed during the write */

  file = fopen (filename ,"wb");
  if (!file)
    {
      g_message (_("FLI: Can't open \"%s\""), filename);
      return FALSE;
    }
  fseek (file, 128, SEEK_SET);

  fb = g_malloc (fli_header.width * fli_header.height);
  ofb = g_malloc (fli_header.width * fli_header.height);

  /* initialize with bg color */
  memset (fb, bg, fli_header.width * fli_header.height);

  /*
   * Now write all frames
   */
  for (cnt = from_frame; cnt <= to_frame; cnt++)
    {
     /* get layer data from GIMP */
      drawable = gimp_drawable_get (framelist[nframes-cnt]);
      gimp_drawable_offsets (framelist[nframes-cnt], &offset_x, &offset_y);
      cols = drawable->width;
      rows = drawable->height;
      bytes = drawable->bpp;
      gimp_pixel_rgn_init (&pixel_rgn, drawable,
			   0, 0, cols, rows,
			   FALSE, FALSE);
      src_row = g_malloc (cols * bytes);

      /* now paste it into the framebuffer, with the neccessary offset */
      for (yc = 0, yy = offset_y; yc < rows; yc++, yy++)
	{
	  if (yy >= 0 && yy < fli_header.height)
	    {
	      gimp_pixel_rgn_get_row (&pixel_rgn, src_row, 0, yc, cols);
	      
	      for (xc = 0, xx = offset_x; xc < cols; xc++, xx++)
		{
		  if (xx >= 0 && xx < fli_header.width)
		    fb[yy * fli_header.width + xx] = src_row[xc * bytes];
		}
	    }
	}

      g_free (src_row);

      /* save the frame */
      if (cnt > from_frame)
	{
	  /* save frame, allow all codecs */
	  fli_write_frame (file, &fli_header, ofb, cm, fb, cm, W_ALL);
	}
      else
	{
	  /* save first frame, no delta information, allow all codecs */
	  fli_write_frame (file, &fli_header, NULL, NULL, fb, cm, W_ALL);
	}

      if (cnt < to_frame)
	memcpy (ofb, fb, fli_header.width * fli_header.height);
 
      gimp_progress_update ((double) cnt + 1 / (double)(to_frame - from_frame));
     }

  /*
   * finish fli
   */
  fli_write_header (file, &fli_header);
  fclose (file);

  g_free (fb);
  g_free (ofb);
  g_free (framelist);

  return TRUE;
}

/*
 * Dialogs for interactive usage
 */
gint result = FALSE;

static void
cb_ok (GtkWidget *widget,
       gpointer   data)
{
  result = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

gint
load_dialog (gchar *name)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;

  gint32 width, height, nframes;
  get_info (name, &width, &height, &nframes);

  from_frame = 1;
  to_frame   = nframes;

  gimp_ui_init ("gfli", FALSE);

  dialog = gimp_dialog_new (_("GFLI 1.3 - Load framestack"), "gfli",
			    gimp_standard_help_func, "filters/gfli.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), cb_ok,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table,
		     FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Maybe I add on-the-fly RGB conversion, to keep palettechanges...
   * But for now you can set a start- and a end-frame:
   */
  spinbutton = gimp_spin_button_new (&adj,
				     from_frame, 1, nframes, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("From:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &from_frame);

  spinbutton = gimp_spin_button_new (&adj,
				     to_frame, 1, nframes, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("To:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &to_frame);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  return result;
}

gint
save_dialog (gint32 image_id)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;

  gint nframes;

  g_free (gimp_image_get_layers (image_id, &nframes));

  from_frame = 1;
  to_frame   = nframes;

  dialog = gimp_dialog_new (_("GFLI 1.3 - Save framestack"), "gfli",
			    gimp_standard_help_func, "filters/gfli.html",
			    GTK_WIN_POS_MOUSE,
			    FALSE, TRUE, FALSE,

			    _("OK"), cb_ok,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), table,
		     FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Maybe I add on-the-fly RGB conversion, to keep palettechanges...
   * But for now you can set a start- and a end-frame:
   */
  spinbutton = gimp_spin_button_new (&adj,
				     from_frame, 1, nframes, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("From:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &from_frame);

  spinbutton = gimp_spin_button_new (&adj,
				     to_frame, 1, nframes, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("To:"), 1.0, 0.5,
			     spinbutton, 1, TRUE);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
		      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
		      &to_frame);

  gtk_widget_show (dialog);

  gtk_main ();
  gdk_flush ();

  return result;
}
