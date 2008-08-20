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

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "fli.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-fli-load"
#define SAVE_PROC      "file-fli-save"
#define INFO_PROC      "file-fli-info"
#define PLUG_IN_BINARY "file-fli"


static void      query       (void);
static void      run         (const gchar      *name,
                              gint              nparams,
                              const GimpParam  *param,
                              gint             *nreturn_vals,
                              GimpParam       **return_vals);

/* return the image-ID of the new image, or -1 in case of an error */
static gint32    load_image  (const  gchar  *filename,
                              gint32         from_frame,
                              gint32         to_frame,
                              GError       **error);
static gboolean  load_dialog (const gchar   *filename);

static gboolean  save_image  (const gchar   *filename,
                              gint32         image_id,
                              gint32         from_frame,
                              gint32         to_frame,
                              GError       **error);
static gboolean  save_dialog (gint32         image_id);

static gboolean  get_info    (const gchar   *filename,
                              gint32        *width,
                              gint32        *height,
                              gint32        *frames,
                              GError       **error);

/*
 * GIMP interface
 */
const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static const GimpParamDef load_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive"   },
  { GIMP_PDB_STRING, "filename",     "The name of the file to load"   },
  { GIMP_PDB_STRING, "raw-filename", "The name entered"               },
  { GIMP_PDB_INT32,  "from-frame",   "Load beginning from this frame" },
  { GIMP_PDB_INT32,  "to-frame",     "End loading with this frame"    }
};

static const GimpParamDef load_return_vals[] =
{
  { GIMP_PDB_IMAGE, "image", "Output image" },
};

static const GimpParamDef save_args[] =
{
  { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive" },
  { GIMP_PDB_IMAGE,    "image",        "Input image" },
  { GIMP_PDB_DRAWABLE, "drawable",     "Input drawable (unused)" },
  { GIMP_PDB_STRING,   "filename",     "The name of the file to save" },
  { GIMP_PDB_STRING,   "raw-filename", "The name entered" },
  { GIMP_PDB_INT32,    "from-frame",   "Save beginning from this frame" },
  { GIMP_PDB_INT32,    "to-frame",     "End saving with this frame" },
};

static const GimpParamDef info_args[] =
{
  { GIMP_PDB_STRING, "filename", "The name of the file to get info" },
};
static const GimpParamDef info_return_vals[] =
{
  { GIMP_PDB_INT32, "width",  "Width of one frame" },
  { GIMP_PDB_INT32, "height", "Height of one frame" },
  { GIMP_PDB_INT32, "frames", "Number of Frames" },
};


static gint32 from_frame;
static gint32 to_frame;

MAIN ()

static void
query (void)
{
  /*
   * Load/save procedures
   */
  gimp_install_procedure (LOAD_PROC,
			  "load FLI-movies",
			  "This is an experimantal plug-in to handle FLI movies",
			  "Jens Ch. Restemeier",
			  "Jens Ch. Restemeier",
			  "1997",
			  N_("AutoDesk FLIC animation"),
			  NULL,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (load_args) - 2,
                          G_N_ELEMENTS (load_return_vals),
			  load_args,
                          load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/x-flic");
  gimp_register_magic_load_handler (LOAD_PROC,
				    "fli,flc",
				    "",
				    "");

  gimp_install_procedure (SAVE_PROC,
			  "save FLI-movies",
			  "This is an experimantal plug-in to handle FLI movies",
			  "Jens Ch. Restemeier",
			  "Jens Ch. Restemeier",
			  "1997",
			  N_("AutoDesk FLIC animation"),
			  "INDEXED,GRAY",
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (save_args), 0,
			  save_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/x-flic");
  gimp_register_save_handler (SAVE_PROC,
			      "fli,flc",
			      "");

  /*
   * Utility functions:
   */
  gimp_install_procedure (INFO_PROC,
			  "Get information about a Fli movie",
			  "This is a experimantal plug-in to handle FLI movies",
			  "Jens Ch. Restemeier",
			  "Jens Ch. Restemeier",
			  "1997",
			  NULL,
			  NULL,
			  GIMP_PLUGIN,
			  G_N_ELEMENTS (info_args),
                          G_N_ELEMENTS (info_return_vals),
			  info_args,
                          info_return_vals);
}

GimpParam values[5];

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;
  gint32             pc;
  gint32             image_ID;
  gint32             drawable_ID;
  gint32             orig_image_ID;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error  = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      switch (run_mode)
	{
	case GIMP_RUN_NONINTERACTIVE:
	  /*
	   * check for valid parameters:
	   * (Or can I trust GIMP ?)
	   */
	  if ((nparams < G_N_ELEMENTS (load_args) - 2) ||
              (G_N_ELEMENTS (load_args) < nparams))
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	      break;
	    }
	  for (pc = 0; pc < G_N_ELEMENTS (load_args) - 2; pc++)
	    {
	      if (load_args[pc].type != param[pc].type)
		{
		  status = GIMP_PDB_CALLING_ERROR;
		  break;
		}
	    }
	  for (pc = G_N_ELEMENTS (load_args) - 2; pc < nparams; pc++)
	    {
	      if (load_args[pc].type != param[pc].type)
		{
		  status = GIMP_PDB_CALLING_ERROR;
		  break;
		}
	    }

	  to_frame   = ((nparams < G_N_ELEMENTS (load_args) - 1) ?
                        1 : param[3].data.d_int32);
	  from_frame = ((nparams < G_N_ELEMENTS (load_args)) ?
                        -1 : param[4].data.d_int32);

          image_ID = load_image (param[1].data.d_string,
                                 from_frame, to_frame, &error);

	  if (image_ID != -1)
	    {
	      *nreturn_vals = 2;
	      values[1].type         = GIMP_PDB_IMAGE;
	      values[1].data.d_image = image_ID;
	    }
	  else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
	  break;

	case GIMP_RUN_INTERACTIVE:
	  if (load_dialog (param[1].data.d_string))
	    {
	      image_ID = load_image (param[1].data.d_string,
                                     from_frame, to_frame, &error);

	      if (image_ID != -1)
		{
		  *nreturn_vals = 2;
		  values[1].type         = GIMP_PDB_IMAGE;
		  values[1].data.d_image = image_ID;
		}
	      else
                {
                  status = GIMP_PDB_EXECUTION_ERROR;
                }
            }
	  else
            {
              status = GIMP_PDB_CANCEL;
            }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  status = GIMP_PDB_CALLING_ERROR;
	  break;
	}
    }
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      image_ID    = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      switch (run_mode)
	{
	case GIMP_RUN_NONINTERACTIVE:
	  if (nparams != G_N_ELEMENTS (save_args))
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	      break;
	    }
	  for (pc = 0; pc < G_N_ELEMENTS (save_args); pc++)
	    {
	      if (save_args[pc].type!=param[pc].type)
		{
		  status = GIMP_PDB_CALLING_ERROR;
		  break;
		}
	    }
	  if (! save_image (param[3].data.d_string, image_ID,
			    param[5].data.d_int32,
			    param[6].data.d_int32, &error))
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
	  break;

	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_ui_init (PLUG_IN_BINARY, FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "FLI",
				      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                      GIMP_EXPORT_CAN_HANDLE_LAYERS);
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }

	  if (save_dialog (param[1].data.d_image))
	    {
	      if (! save_image (param[3].data.d_string,
                                image_ID, from_frame, to_frame, &error))
                {
                  status = GIMP_PDB_EXECUTION_ERROR;
                }
            }
	  else
            {
              status = GIMP_PDB_CANCEL;
            }
	  break;
	}

      if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else if (strcmp (name, INFO_PROC) == 0)
    {
      gint32 width, height, frames;

      /*
       * check for valid parameters;
       */
      if (nparams != G_N_ELEMENTS (info_args))
	status = GIMP_PDB_CALLING_ERROR;

      if (status == GIMP_PDB_SUCCESS)
	{
	  for (pc = 0; pc < G_N_ELEMENTS (save_args); pc++)
	    {
	      if (info_args[pc].type != param[pc].type)
		{
		  status = GIMP_PDB_CALLING_ERROR;
		  break;
		}
	    }
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (get_info (param[0].data.d_string,
                        &width, &height, &frames, &error))
	    {
	      *nreturn_vals = 4;
	      values[1].type = GIMP_PDB_INT32;
	      values[1].data.d_int32 = width;
	      values[2].type = GIMP_PDB_INT32;
	      values[2].data.d_int32 = height;
	      values[3].type = GIMP_PDB_INT32;
	      values[3].data.d_int32 = frames;
	    }
	  else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}

/*
 * Open FLI animation and return header-info
 */
static gboolean
get_info (const gchar  *filename,
	  gint32       *width,
	  gint32       *height,
	  gint32       *frames,
          GError      **error)
{
  FILE *file;
  s_fli_header fli_header;

  *width = 0; *height = 0; *frames = 0;

  file = g_fopen (filename ,"rb");

  if (!file)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  fli_read_header (file, &fli_header);
  fclose (file);

  *width  = fli_header.width;
  *height = fli_header.height;
  *frames = fli_header.frames;

  return TRUE;
}

/*
 * load fli animation and store as framestack
 */
static gint32
load_image (const gchar  *filename,
	    gint32        from_frame,
	    gint32        to_frame,
            GError      **error)
{
  FILE         *file;
  GimpDrawable *drawable;
  gint32        image_id, layer_ID;
  guchar       *fb, *ofb, *fb_x;
  guchar        cm[768], ocm[768];
  GimpPixelRgn  pixel_rgn;
  s_fli_header  fli_header;
  gint          cnt;

  file = g_fopen (filename ,"rb");
  if (!file)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

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

  image_id = gimp_image_new (fli_header.width, fli_header.height, GIMP_INDEXED);
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
      gchar *name_buf = g_strdup_printf (_("Frame (%i)"), cnt);

      layer_ID = gimp_layer_new (image_id, name_buf,
				 fli_header.width, fli_header.height,
				 GIMP_INDEXED_IMAGE, 100, GIMP_NORMAL_MODE);
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

  gimp_image_set_colormap (image_id, cm, 256);

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
static gboolean
save_image (const gchar  *filename,
	    gint32        image_id,
	    gint32        from_frame,
	    gint32        to_frame,
            GError      **error)
{
  FILE *file;
  GimpDrawable *drawable;
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
  GimpPixelRgn pixel_rgn;
  GimpRGB      background;
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

  gimp_context_get_background (&background);
  gimp_rgb_get_uchar (&background, &red, &green, &blue);

  switch (gimp_image_base_type (image_id))
    {
    case GIMP_GRAY:
      /* build grayscale palette */
      for (i = 0; i < 256; i++)
	{
	  cm[i*3+0] = cm[i*3+1] = cm[i*3+2] = i;
	}
      bg = GIMP_RGB_LUMINANCE (red, green, blue) + 0.5;
      break;

    case GIMP_INDEXED:
      max = MAXDIFF;
      bg = 0;
      cmap = gimp_image_get_colormap (image_id, &colors);
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
      g_message (_("Sorry, I can save only INDEXED and GRAY images."));
      return FALSE;
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

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

  file = g_fopen (filename ,"wb");
  if (!file)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
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
static gboolean
load_dialog (const gchar *filename)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  gint32     width, height, nframes;
  gboolean   run;

  get_info (filename, &width, &height, &nframes, NULL);

  from_frame = 1;
  to_frame   = nframes;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("GFLI 1.3 - Load framestack"), PLUG_IN_BINARY,
                            NULL, 0,
			    gimp_standard_help_func, LOAD_PROC,

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_OPEN,   GTK_RESPONSE_OK,

			    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
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
			     _("From:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &from_frame);

  spinbutton = gimp_spin_button_new (&adj,
				     to_frame, 1, nframes, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("To:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &to_frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
save_dialog (gint32 image_id)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkObject *adj;
  gint       nframes;
  gboolean   run;

  g_free (gimp_image_get_layers (image_id, &nframes));

  from_frame = 1;
  to_frame   = nframes;

  dialog = gimp_dialog_new (_("GFLI 1.3 - Save framestack"), PLUG_IN_BINARY,
                            NULL, 0,
			    gimp_standard_help_func, SAVE_PROC,

			    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			    GTK_STOCK_SAVE,   GTK_RESPONSE_OK,

			    NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG (dialog)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*
   * Maybe I add on-the-fly RGB conversion, to keep palettechanges...
   * But for now you can set a start- and a end-frame:
   */
  spinbutton = gimp_spin_button_new (&adj,
				     from_frame, 1, nframes, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("From:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &from_frame);

  spinbutton = gimp_spin_button_new (&adj,
				     to_frame, 1, nframes, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("To:"), 0.0, 0.5,
			     spinbutton, 1, TRUE);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &to_frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
