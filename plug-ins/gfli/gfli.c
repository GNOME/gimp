/*
 * GFLI 1.2
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
 */
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>
#include <libgimp/stdplugins-intl.h>

#include "fli.h"

static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

/* return the image-ID of the new image, or -1 in case of an error */
gint32 load_image             (gchar  *filename,
			       gint32  from_frame,
			       gint32  to_frame);
gint   load_dialog            (gchar  *name);

/* return TRUE or FALSE */
gint   save_image             (gchar  *filename,
			       gint32  image_id,
			       gint32  from_frame,
			       gint32  to_frame);
gint   save_dialog            (gint32  image_id);

/* return TRUE or FALSE */
gint   get_info               (gchar  *filename,
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
  { PARAM_INT32, "to_frame", "End saveing with this frame" },
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
  gint32       image_ID;
  gint32       pc;

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
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
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
		{
		  status = STATUS_EXECUTION_ERROR;
		}
	    }
	  else
	    {
	      status = STATUS_CANCEL;
	    }
	  break;

	case RUN_WITH_LAST_VALS:
	  status = STATUS_CALLING_ERROR;
	  break;
	}
    }
  else if (strcmp (name, "file_fli_save") == 0)
    {
      INIT_I18N_UI ();

      switch (run_mode)
	{
	case RUN_NONINTERACTIVE:
	  /*
	   * check for valid parameters;
	   */
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
	  if (! save_image (param[3].data.d_string,
			    param[1].data.d_image,
			    param[5].data.d_int32,
			    param[6].data.d_int32))
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	  break;

	case RUN_INTERACTIVE:
	  if (save_dialog (param[1].data.d_image))
	    {
	      if (! save_image (param[3].data.d_string,
				param[1].data.d_image,
				from_frame, to_frame))
		{
		  status = STATUS_EXECUTION_ERROR;
		}
	    }
	  else
	    {
	      status = STATUS_CANCEL;
	    }
	  break;

	case RUN_WITH_LAST_VALS:
	  status = STATUS_CALLING_ERROR;
	  break;
	}
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
	    {
	      status = STATUS_EXECUTION_ERROR;
	    }
	}
    }
  else
    {
      status = STATUS_CALLING_ERROR;
    }

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
  FILE *f;
  s_fli_header fli_header;

  *width = 0; *height = 0; *frames = 0;

  f = fopen (filename ,"rb");

  if (!f)
    {
      g_message (_("FLI: can't open \"%s\"\n"), filename);
      return FALSE;
    }
  fli_read_header (f, &fli_header);
  fclose (f);
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
  FILE *f;
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

  f = fopen (filename ,"rb");
  if (!f)
    {
      g_message (_("FLI: can't open \"%s\"\n"), filename);
      return -1;
    }
  fli_read_header (f, &fli_header);
  if (fli_header.magic == NO_HEADER)
    return -1;
  else
    fseek (f, 128, SEEK_SET);

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
      fli_read_frame (f, &fli_header, ofb, ocm, fb, cm);
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
      gimp_image_add_layer (image_id, layer_ID, 0);
      g_free (name_buf);

      drawable = gimp_drawable_get (layer_ID);
      gimp_progress_update ((double) cnt / (double)(to_frame-from_frame));

      fli_read_frame (f, &fli_header, ofb, ocm, fb, cm);

      gimp_pixel_rgn_init (&pixel_rgn, drawable,
			   0, 0, fli_header.width, fli_header.height,
			   TRUE, FALSE);
      gimp_pixel_rgn_set_rect (&pixel_rgn, fb,
			       0, 0, fli_header.width, fli_header.height);

      gimp_drawable_flush (drawable);
      gimp_drawable_detach (drawable);
      memcpy (ocm, cm, 768);
      fb_x = fb; fb = ofb; ofb = fb_x;
    }
  gimp_image_set_cmap (image_id, cm, 256);
  fclose (f);

  g_free (fb);
  g_free (ofb);

  return image_id;
}

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
  FILE *f;
  gchar *name_buf;
  GDrawable *drawable;
  GDrawableType drawable_type;
  gint32 *framelist;
  gint nframes;
    	
  guchar *fb, *ofb;
  guchar cm[768];
  GPixelRgn pixel_rgn;
  s_fli_header fli_header;

  gint cnt;

  /*
   * prepare header and check information
   */
  framelist = gimp_image_get_layers (image_id, &nframes);
	
  /*
   * Fix parameters
   */
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

  drawable_type = gimp_drawable_type (framelist[0]);
  switch (drawable_type)
    {
    case INDEXEDA_IMAGE:
    case GRAYA_IMAGE:
      g_message (_("FLI: Sorry, can't save images with Alpha.\n"));
      return FALSE;
    case GRAY_IMAGE:
      {
	/* build grayscale palette */
	gint i;
	for (i = 0; i < 256; i++)
	  {
	    cm[i*3+0] = cm[i*3+1] = cm[i*3+2] = i;
	  }
	break;
      }
    case INDEXED_IMAGE:
      {
	gint colors, i;
	guchar *cmap;
	cmap=gimp_image_get_cmap(image_id, &colors);
	for (i = 0; i < colors; i++)
	  {
	    cm[i*3+0] = cmap[i*3+0];
	    cm[i*3+1] = cmap[i*3+1];
	    cm[i*3+2] = cmap[i*3+2];
	  }
	for (i = colors; i < 256; i++)
	  {
	    cm[i*3+0] = cm[i*3+1] = cm[i*3+2] = i;
	  }
	break;
      }
    default:
      g_message (_("FLI: Sorry, I can save only INDEXED and GRAY images.\n"));
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

  f = fopen (filename ,"wb");
  if (!f)
    {
      g_message (_("FLI: can't open \"%s\"\n"), filename);
      return FALSE;
    }
  fseek (f, 128, SEEK_SET);

  fb  = g_malloc (fli_header.width * fli_header.height);
  ofb = g_malloc (fli_header.width * fli_header.height);

  /*
   * Now write all frames
   */
  for (cnt = from_frame; cnt <= to_frame; cnt++)
    {
      gint offset_x, offset_y, xc, yc;
      guint rows, cols, rowstride;
      guchar *tmp;

      gimp_progress_update ((double) cnt / (double)(to_frame-from_frame));

      /* get layer data from GIMP */
      drawable = gimp_drawable_get (framelist[nframes-cnt]);
      gimp_drawable_offsets (framelist[cnt-1], &offset_x, &offset_y);
      cols = drawable->width;
      rows = drawable->height;
      rowstride = drawable->width;
      gimp_pixel_rgn_init (&pixel_rgn, drawable,
			   0, 0, drawable->width, drawable->height,
			   FALSE, FALSE);
      tmp = malloc(cols * rows);
      gimp_pixel_rgn_get_rect (&pixel_rgn, tmp,
			       0, 0, drawable->width, drawable->height);

      /* now paste it into the framebuffer, with the neccessary offset */
      for (yc = 0; yc < cols; yc++)
	{
	  gint yy;
	  yy = yc + offset_y;
	  if ((yy > 0) && (yy < fli_header.height))
	    {
	      for (xc = 0; xc < cols; xc++)
		{
		  gint xx;
		  xx = xc + offset_x;
		  if ((xx > 0) && (xx < fli_header.width))
		    {
		      fb[yy*fli_header.width + xx] = tmp[yc*cols+xc];
		    }
		}
	    }
	}
      free (tmp);

      /* save the frame */
      if (cnt > from_frame)
	{
	  /* save frame, allow all codecs */
	  fli_write_frame (f, &fli_header, ofb, cm, fb, cm, W_ALL);
	}
      else
	{
	  /* save first frame, no delta information, allow all codecs */
	  fli_write_frame (f, &fli_header, NULL, NULL, fb, cm, W_ALL);
	}
      memcpy (ofb, fb, fli_header.width * fli_header.height);
    }

  /*
   * finish fli
   */
  fli_write_header (f, &fli_header);
  fclose (f);

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

void
init_gtk (void)
{
  guchar *color_cube;
  gchar **argv;
  gint    argc;

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("gfli");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc());

  gdk_set_use_xshm (gimp_use_xshm ());
  gtk_preview_set_gamma (gimp_gamma ());
  gtk_preview_set_install_cmap (gimp_install_cmap ());
  color_cube = gimp_color_cube ();
  gtk_preview_set_color_cube (color_cube[0], color_cube[1],
			      color_cube[2], color_cube[3]);
  gtk_widget_set_default_visual (gtk_preview_get_visual ());
  gtk_widget_set_default_colormap (gtk_preview_get_cmap ());
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

  init_gtk ();

  dialog = gimp_dialog_new (_("GFLI 1.2 - Load framestack"), "gfli",
			    gimp_plugin_help_func, "filters/gfli.html",
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

  init_gtk ();

  dialog = gimp_dialog_new (_("GFLI 1.2 - Save framestack"), "gfli",
			    gimp_plugin_help_func, "filters/gfli.html",
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

