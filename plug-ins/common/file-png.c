/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   Portable Network Graphics (PNG) plug-in
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com) and
 *   Daniel Skarda (0rfelyus@atrey.karlin.mff.cuni.cz).
 *   and 1999-2000 Nick Lamb (njl195@zepler.org.uk)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contents:
 *
 *   main()                      - Main entry - just call gimp_main()...
 *   query()                     - Respond to a plug-in query...
 *   run()                       - Run the plug-in...
 *   load_image()                - Load a PNG image into a new image window.
 *   offsets_dialog()            - Asks the user about offsets when loading.
 *   respin_cmap()               - Re-order a Gimp colormap for PNG tRNS
 *   save_image()                - Save the specified image to a PNG file.
 *   save_compression_callback() - Update the image compression level.
 *   save_interlace_update()     - Update the interlacing option.
 *   save_dialog()               - Pop up the save dialog.
 *
 * Revision History:
 *
 *   see ChangeLog
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include <png.h>                /* PNG library definitions */

#include "libgimp/stdplugins-intl.h"


/*
 * Constants...
 */

#define LOAD_PROC              "file-png-load"
#define SAVE_PROC              "file-png-save"
#define SAVE2_PROC             "file-png-save2"
#define SAVE_DEFAULTS_PROC     "file-png-save-defaults"
#define GET_DEFAULTS_PROC      "file-png-get-defaults"
#define SET_DEFAULTS_PROC      "file-png-set-defaults"
#define PLUG_IN_BINARY         "file-png"
#define PLUG_IN_ROLE           "gimp-file-png"

#define PLUG_IN_VERSION        "1.3.4 - 03 September 2002"
#define SCALE_WIDTH            125

#define DEFAULT_GAMMA          2.20

#define PNG_DEFAULTS_PARASITE  "png-save-defaults"

/*
 * Structures...
 */

typedef struct
{
  gboolean  interlaced;
  gboolean  bkgd;
  gboolean  gama;
  gboolean  offs;
  gboolean  phys;
  gboolean  time;
  gboolean  comment;
  gboolean  save_transp_pixels;
  gint      compression_level;
}
PngSaveVals;

typedef struct
{
  gboolean   run;

  GtkWidget *interlaced;
  GtkWidget *bkgd;
  GtkWidget *gama;
  GtkWidget *offs;
  GtkWidget *phys;
  GtkWidget *time;
  GtkWidget *comment;
  GtkWidget *save_transp_pixels;
  GtkAdjustment *compression_level;
}
PngSaveGui;

/* These are not saved or restored. */
typedef struct
{
  gboolean   has_trns;
  png_bytep  trans;
  int        num_trans;
  gboolean   has_plte;
  png_colorp palette;
  int        num_palette;
}
PngGlobals;

/*
 * Local functions...
 */

static void      query                     (void);
static void      run                       (const gchar      *name,
                                            gint              nparams,
                                            const GimpParam  *param,
                                            gint             *nreturn_vals,
                                            GimpParam       **return_vals);

static gint32    load_image                (const gchar      *filename,
                                            gboolean          interactive,
                                            GError          **error);
static gboolean  save_image                (const gchar      *filename,
                                            gint32            image_ID,
                                            gint32            drawable_ID,
                                            gint32            orig_image_ID,
                                            GError          **error);

static int       respin_cmap               (png_structp       pp,
                                            png_infop         info,
                                            guchar           *remap,
                                            gint32            image_ID,
                                            GimpDrawable     *drawable);

static gboolean  save_dialog               (gint32            image_ID,
                                            gboolean          alpha);

static void      save_dialog_response      (GtkWidget        *widget,
                                            gint              response_id,
                                            gpointer          data);

static gboolean  offsets_dialog            (gint              offset_x,
                                            gint              offset_y);

static gboolean  ia_has_transparent_pixels (GimpDrawable     *drawable);

static gint      find_unused_ia_color      (GimpDrawable     *drawable,
                                            gint             *colors);

static void      load_defaults             (void);
static void      save_defaults             (void);
static void      load_gui_defaults         (PngSaveGui       *pg);

/*
 * Globals...
 */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,
  NULL,
  query,
  run
};

static const PngSaveVals defaults =
{
  FALSE,
  TRUE,
  FALSE,
  FALSE,
  TRUE,
  TRUE,
  TRUE,
  TRUE,
  9
};

static PngSaveVals pngvals;
static PngGlobals pngg;


/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN ()


/*
 * 'query()' - Respond to a plug-in query...
 */

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

#define COMMON_SAVE_ARGS \
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" }, \
    { GIMP_PDB_IMAGE,    "image",        "Input image"                  }, \
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"             }, \
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in"}, \
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in"}

#define OLD_CONFIG_ARGS \
    { GIMP_PDB_INT32, "interlace",   "Use Adam7 interlacing?"            }, \
    { GIMP_PDB_INT32, "compression", "Deflate Compression factor (0--9)" }, \
    { GIMP_PDB_INT32, "bkgd",        "Write bKGD chunk?"                 }, \
    { GIMP_PDB_INT32, "gama",        "Write gAMA chunk?"                 }, \
    { GIMP_PDB_INT32, "offs",        "Write oFFs chunk?"                 }, \
    { GIMP_PDB_INT32, "phys",        "Write pHYs chunk?"                 }, \
    { GIMP_PDB_INT32, "time",        "Write tIME chunk?"                 }

#define FULL_CONFIG_ARGS \
    OLD_CONFIG_ARGS,                                                        \
    { GIMP_PDB_INT32, "comment", "Write comment?"                        }, \
    { GIMP_PDB_INT32, "svtrans", "Preserve color of transparent pixels?" }

  static const GimpParamDef save_args[] =
  {
    COMMON_SAVE_ARGS,
    OLD_CONFIG_ARGS
  };

  static const GimpParamDef save_args2[] =
  {
    COMMON_SAVE_ARGS,
    FULL_CONFIG_ARGS
  };

  static const GimpParamDef save_args_defaults[] =
  {
    COMMON_SAVE_ARGS
  };

  static const GimpParamDef save_get_defaults_return_vals[] =
  {
    FULL_CONFIG_ARGS
  };

  static const GimpParamDef save_args_set_defaults[] =
  {
    FULL_CONFIG_ARGS
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in PNG file format",
                          "This plug-in loads Portable Network Graphics "
                          "(PNG) files.",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, "
                          "Nick Lamb <njl195@zepler.org.uk>",
                          PLUG_IN_VERSION,
                          N_("PNG image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/png");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "png", "", "0,string,\211PNG\r\n\032\n");

  gimp_install_procedure (SAVE_PROC,
                          "Saves files in PNG file format",
                          "This plug-in saves Portable Network Graphics "
                          "(PNG) files.",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, "
                          "Nick Lamb <njl195@zepler.org.uk>",
                          PLUG_IN_VERSION,
                          N_("PNG image"),
                          "RGB*,GRAY*,INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_install_procedure (SAVE2_PROC,
                          "Saves files in PNG file format",
                          "This plug-in saves Portable Network Graphics "
                          "(PNG) files. "
                          "This procedure adds 2 extra parameters to "
                          "file-png-save that allows to control whether "
                          "image comments are saved and whether transparent "
                          "pixels are saved or nullified.",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, "
                          "Nick Lamb <njl195@zepler.org.uk>",
                          PLUG_IN_VERSION,
                          N_("PNG image"),
                          "RGB*,GRAY*,INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args2), 0,
                          save_args2, NULL);

  gimp_install_procedure (SAVE_DEFAULTS_PROC,
                          "Saves files in PNG file format",
                          "This plug-in saves Portable Network Graphics (PNG) "
                          "files, using the default settings stored as "
                          "a parasite.",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, "
                          "Nick Lamb <njl195@zepler.org.uk>",
                          PLUG_IN_VERSION,
                          N_("PNG image"),
                          "RGB*,GRAY*,INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args_defaults), 0,
                          save_args_defaults, NULL);

  gimp_register_file_handler_mime (SAVE_DEFAULTS_PROC, "image/png");
  gimp_register_save_handler (SAVE_DEFAULTS_PROC, "png", "");

  gimp_install_procedure (GET_DEFAULTS_PROC,
                          "Get the current set of defaults used by the "
                          "PNG file save plug-in",
                          "This procedure returns the current set of "
                          "defaults stored as a parasite for the PNG "
                          "save plug-in. "
                          "These defaults are used to seed the UI, by the "
                          "file_png_save_defaults procedure, and by "
                          "gimp_file_save when it detects to use PNG.",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, "
                          "Nick Lamb <njl195@zepler.org.uk>",
                          PLUG_IN_VERSION,
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          0, G_N_ELEMENTS (save_get_defaults_return_vals),
                          NULL, save_get_defaults_return_vals);

  gimp_install_procedure (SET_DEFAULTS_PROC,
                          "Set the current set of defaults used by the "
                          "PNG file save plug-in",
                          "This procedure set the current set of defaults "
                          "stored as a parasite for the PNG save plug-in. "
                          "These defaults are used to seed the UI, by the "
                          "file_png_save_defaults procedure, and by "
                          "gimp_file_save when it detects to use PNG.",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>",
                          "Michael Sweet <mike@easysw.com>, "
                          "Daniel Skarda <0rfelyus@atrey.karlin.mff.cuni.cz>, "
                          "Nick Lamb <njl195@zepler.org.uk>",
                          PLUG_IN_VERSION,
                          NULL,
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args_set_defaults), 0,
                          save_args_set_defaults, NULL);
}


/*
 * 'run()' - Run the plug-in...
 */

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[10];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID;
  gint32            orig_image_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;
  GError           *error  = NULL;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals = values;

  gegl_init (NULL, NULL);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      run_mode = param[0].data.d_int32;

      image_ID = load_image (param[1].data.d_string,
                             run_mode == GIMP_RUN_INTERACTIVE, &error);

      if (image_ID != -1)
        {
          *nreturn_vals = 2;
          values[1].type = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else if (strcmp (name, SAVE_PROC)  == 0 ||
           strcmp (name, SAVE2_PROC) == 0 ||
           strcmp (name, SAVE_DEFAULTS_PROC) == 0)
    {
      gboolean alpha;

      run_mode    = param[0].data.d_int32;
      image_ID    = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      load_defaults ();

      /*  eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          export = gimp_export_image (&image_ID, &drawable_ID, NULL,
                                      (GIMP_EXPORT_CAN_HANDLE_RGB |
                                       GIMP_EXPORT_CAN_HANDLE_GRAY |
                                       GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                       GIMP_EXPORT_CAN_HANDLE_ALPHA));

          if (export == GIMP_EXPORT_CANCEL)
            {
              *nreturn_vals = 1;
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;
        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          /*
           * Possibly retrieve data...
           */
          gimp_get_data (SAVE_PROC, &pngvals);

          alpha = gimp_drawable_has_alpha (drawable_ID);

          /*
           * If the image has no transparency, then there is usually
           * no need to save a bKGD chunk.  For more information, see:
           * http://bugzilla.gnome.org/show_bug.cgi?id=92395
           */
          if (! alpha)
            pngvals.bkgd = FALSE;

          /*
           * Then acquire information with a dialog...
           */
          if (! save_dialog (orig_image_ID, alpha))
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*
           * Make sure all the arguments are there!
           */
          if (nparams != 5)
            {
              if (nparams != 12 && nparams != 14)
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }
              else
                {
                  pngvals.interlaced        = param[5].data.d_int32;
                  pngvals.compression_level = param[6].data.d_int32;
                  pngvals.bkgd              = param[7].data.d_int32;
                  pngvals.gama              = param[8].data.d_int32;
                  pngvals.offs              = param[9].data.d_int32;
                  pngvals.phys              = param[10].data.d_int32;
                  pngvals.time              = param[11].data.d_int32;

                  if (nparams == 14)
                    {
                      pngvals.comment            = param[12].data.d_int32;
                      pngvals.save_transp_pixels = param[13].data.d_int32;
                    }
                  else
                    {
                      pngvals.comment            = TRUE;
                      pngvals.save_transp_pixels = TRUE;
                    }

                  if (pngvals.compression_level < 0 ||
                      pngvals.compression_level > 9)
                    {
                      status = GIMP_PDB_CALLING_ERROR;
                    }
                }
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          /*
           * Possibly retrieve data...
           */
          gimp_get_data (SAVE_PROC, &pngvals);
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (save_image (param[3].data.d_string,
                          image_ID, drawable_ID, orig_image_ID, &error))
            {
              gimp_set_data (SAVE_PROC, &pngvals, sizeof (pngvals));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }
  else if (strcmp (name, GET_DEFAULTS_PROC) == 0)
    {
      load_defaults ();

      *nreturn_vals = 10;

#define SET_VALUE(index, field)        G_STMT_START { \
 values[(index)].type = GIMP_PDB_INT32;        \
 values[(index)].data.d_int32 = pngvals.field; \
} G_STMT_END

      SET_VALUE (1, interlaced);
      SET_VALUE (2, compression_level);
      SET_VALUE (3, bkgd);
      SET_VALUE (4, gama);
      SET_VALUE (5, offs);
      SET_VALUE (6, phys);
      SET_VALUE (7, time);
      SET_VALUE (8, comment);
      SET_VALUE (9, save_transp_pixels);

#undef SET_VALUE
    }
  else if (strcmp (name, SET_DEFAULTS_PROC) == 0)
    {
      if (nparams == 9)
        {
          pngvals.interlaced          = param[0].data.d_int32;
          pngvals.compression_level   = param[1].data.d_int32;
          pngvals.bkgd                = param[2].data.d_int32;
          pngvals.gama                = param[3].data.d_int32;
          pngvals.offs                = param[4].data.d_int32;
          pngvals.phys                = param[5].data.d_int32;
          pngvals.time                = param[6].data.d_int32;
          pngvals.comment             = param[7].data.d_int32;
          pngvals.save_transp_pixels  = param[8].data.d_int32;

          save_defaults ();
        }
      else
        {
          status = GIMP_PDB_CALLING_ERROR;
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


struct read_error_data
{
  guchar       *pixel;           /* Pixel data */
  GeglBuffer   *buffer;          /* GEGL buffer for layer */
  const Babl   *file_format;
  guint32       width;           /* png_infop->width */
  guint32       height;          /* png_infop->height */
  gint          bpp;             /* Bytes per pixel */
  gint          tile_height;     /* Height of tile in GIMP */
  gint          begin;           /* Beginning tile row */
  gint          end;             /* Ending tile row */
  gint          num;             /* Number of rows to load */
};

static void
on_read_error (png_structp png_ptr, png_const_charp error_msg)
{
  struct read_error_data *error_data = png_get_error_ptr (png_ptr);
  gint                    begin;
  gint                    end;
  gint                    num;

  g_warning (_("Error loading PNG file: %s"), error_msg);

  /* Flush the current half-read row of tiles */

  gegl_buffer_set (error_data->buffer,
                   GEGL_RECTANGLE (0, error_data->begin,
                                   gegl_buffer_get_width (error_data->buffer),
                                   error_data->num),
                   0,
                   error_data->file_format,
                   error_data->pixel,
                   GEGL_AUTO_ROWSTRIDE);

  /* Fill the rest of the rows of tiles with 0s */

  memset (error_data->pixel, 0,
          error_data->tile_height * error_data->width * error_data->bpp);

  for (begin = error_data->begin + error_data->tile_height,
        end = error_data->end + error_data->tile_height;
        begin < error_data->height;
        begin += error_data->tile_height, end += error_data->tile_height)
    {
      if (end > error_data->height)
        end = error_data->height;

      num = end - begin;

      gegl_buffer_set (error_data->buffer,
                       GEGL_RECTANGLE (0, begin,
                                       gegl_buffer_get_width (error_data->buffer),
                                       num),
                       0,
                       error_data->file_format,
                       error_data->pixel,
                       GEGL_AUTO_ROWSTRIDE);
    }

  longjmp (png_jmpbuf (png_ptr), 1);
}

static int
get_bit_depth_for_palette (int num_palette)
{
  if (num_palette <= 2)
    return 1;
  else if (num_palette <= 4)
    return 2;
  else if (num_palette <= 16)
    return 4;
  else
    return 8;
}

/*
 * 'load_image()' - Load a PNG image into a new image window.
 */
static gint32
load_image (const gchar  *filename,
            gboolean      interactive,
            GError      **error)
{
  int i,                        /* Looping var */
    trns,                       /* Transparency present */
    bpp,                        /* Bytes per pixel */
    have_u16 = 0,               /* 16bit values? */
    image_type,                 /* Type of image */
    layer_type,                 /* Type of drawable/layer */
    empty,                      /* Number of fully transparent indices */
    num_passes,                 /* Number of interlace passes in file */
    pass,                       /* Current pass in file */
    tile_height,                /* Height of tile in GIMP */
    begin,                      /* Beginning tile row */
    end,                        /* Ending tile row */
    num;                        /* Number of rows to load */
  FILE *fp;                     /* File pointer */
  volatile gint32 image = -1;   /* Image -- preserved against setjmp() */
  gint32 layer;                 /* Layer */
  GeglBuffer *buffer;           /* GEGL buffer for layer */
  const Babl *file_format;      /* BABL format for layer */
  png_structp pp;               /* PNG read pointer */
  png_infop info;               /* PNG info pointers */
  guchar **pixels,              /* Pixel rows */
   *pixel;                      /* Pixel data */
  guchar alpha[256],            /* Index -> Alpha */
   *alpha_ptr;                  /* Temporary pointer */
  struct read_error_data
   error_data;

  png_textp  text;
  gint       num_texts;

  pp = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!pp)
    {
      /* this could happen if the compile time and run-time libpng
         versions do not match. */

      g_set_error (error, 0, 0,
                   _("Error creating PNG read struct while saving '%s'."),
                   gimp_filename_to_utf8 (filename));
      return -1;
    }

  info = png_create_info_struct (pp);

  if (setjmp (png_jmpbuf (pp)))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error while reading '%s'. File corrupted?"),
                   gimp_filename_to_utf8 (filename));
      return image;
    }

  /*
   * Open the file and initialize the PNG read "engine"...
   */

  fp = g_fopen (filename, "rb");

  if (fp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  png_init_io (pp, fp);

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  /*
   * Get the image dimensions and create the image...
   */

  png_read_info (pp, info);

  /*
   * Latest attempt, this should be my best yet :)
   */

  if (png_get_bit_depth (pp, info) == 16)
    {
      have_u16 = 1;
    }

  if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    png_set_swap (pp);

  if (png_get_color_type (pp, info) == PNG_COLOR_TYPE_GRAY &&
      png_get_bit_depth (pp, info) < 8)
    {
      png_set_expand (pp);
    }

  if (png_get_color_type (pp, info) == PNG_COLOR_TYPE_PALETTE &&
      png_get_bit_depth (pp, info) < 8)
    {
      png_set_packing (pp);
    }

  /*
   * Expand G+tRNS to GA, RGB+tRNS to RGBA
   */

  if (png_get_color_type (pp, info) != PNG_COLOR_TYPE_PALETTE &&
      png_get_valid (pp, info, PNG_INFO_tRNS))
    {
      png_set_expand (pp);
    }

  /*
   * Turn on interlace handling... libpng returns just 1 (ie single pass)
   * if the image is not interlaced
   */

  num_passes = png_set_interlace_handling (pp);

  /*
   * Special handling for INDEXED + tRNS (transparency palette)
   */

  if (png_get_valid (pp, info, PNG_INFO_tRNS) &&
      png_get_color_type (pp, info) == PNG_COLOR_TYPE_PALETTE)
    {
      png_get_tRNS (pp, info, &alpha_ptr, &num, NULL);
      /* Copy the existing alpha values from the tRNS chunk */
      for (i = 0; i < num; ++i)
        alpha[i] = alpha_ptr[i];
      /* And set any others to fully opaque (255)  */
      for (i = num; i < 256; ++i)
        alpha[i] = 255;
      trns = 1;
    }
  else
    {
      trns = 0;
    }

  /*
   * Update the info structures after the transformations take effect
   */

  png_read_update_info (pp, info);

  switch (png_get_color_type (pp, info))
    {
    case PNG_COLOR_TYPE_RGB:           /* RGB */
      image_type = GIMP_RGB;
      layer_type = GIMP_RGB_IMAGE;
      bpp = have_u16 ? 6 : 3;
      file_format = babl_format (have_u16 ? "R'G'B' u16" : "R'G'B' u8");
      break;

    case PNG_COLOR_TYPE_RGB_ALPHA:     /* RGBA */
      image_type = GIMP_RGB;
      layer_type = GIMP_RGBA_IMAGE;
      bpp = have_u16 ? 8 : 4;
      file_format = babl_format (have_u16 ? "R'G'B'A u16" : "R'G'B'A u8");
      break;

    case PNG_COLOR_TYPE_GRAY:          /* Grayscale */
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAY_IMAGE;
      bpp = have_u16 ? 2 : 1;
      file_format = babl_format (have_u16 ? "Y' u16" : "Y' u8");
      break;

    case PNG_COLOR_TYPE_GRAY_ALPHA:    /* Grayscale + alpha */
      image_type = GIMP_GRAY;
      layer_type = GIMP_GRAYA_IMAGE;
      bpp = have_u16 ? 4 : 2;
      file_format = babl_format (have_u16 ? "Y'A u16" : "Y'A u8");
      break;

#warning port PNG indexed format
#if 0
    case PNG_COLOR_TYPE_PALETTE:       /* Indexed */
      bpp = 1;
      image_type = GIMP_INDEXED;
      layer_type = GIMP_INDEXED_IMAGE;
      break;
#endif

    default:                           /* Aie! Unknown type */
      g_set_error (error, 0, 0,
                   _("Unknown color model in PNG file '%s'."),
                   gimp_filename_to_utf8 (filename));
      return -1;
    }

  image = gimp_image_new_with_precision (png_get_image_width (pp, info),
                                         png_get_image_height (pp, info),
                                         image_type,
                                         have_u16 ?
                                           GIMP_PRECISION_U16 :
                                           GIMP_PRECISION_U8);
  if (image == -1)
    {
      g_set_error (error, 0, 0,
                   _("Could not create new image for '%s': %s"),
                   gimp_filename_to_utf8 (filename), gimp_get_pdb_error ());
      return -1;
    }

  /*
   * Create the "background" layer to hold the image...
   */

  layer = gimp_layer_new (image, _("Background"),
                          png_get_image_width (pp, info),
                          png_get_image_height (pp, info),
                          layer_type, 100, GIMP_NORMAL_MODE);
  gimp_image_insert_layer (image, layer, -1, 0);

  /*
   * Find out everything we can about the image resolution
   * This is only practical with the new 1.0 APIs, I'm afraid
   * due to a bug in libpng-1.0.6, see png-implement for details
   */

  if (png_get_valid (pp, info, PNG_INFO_gAMA))
    {
      GimpParasite *parasite;
      gchar         buf[G_ASCII_DTOSTR_BUF_SIZE];
      gdouble       gamma;

      png_get_gAMA (pp, info, &gamma);

      g_ascii_dtostr (buf, sizeof (buf), gamma);

      parasite = gimp_parasite_new ("gamma",
                                    GIMP_PARASITE_PERSISTENT,
                                    strlen (buf) + 1, buf);
      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }

  if (png_get_valid (pp, info, PNG_INFO_oFFs))
    {
      gint offset_x = png_get_x_offset_pixels (pp, info);
      gint offset_y = png_get_y_offset_pixels (pp, info);

      if (! interactive)
        {
          gimp_layer_set_offsets (layer, offset_x, offset_y);
        }
      else if (offsets_dialog (offset_x, offset_y))
        {
          gimp_layer_set_offsets (layer, offset_x, offset_y);

          if ((abs (offset_x) > png_get_image_width (pp, info)) ||
              (abs (offset_y) > png_get_image_height (pp, info)))
            {
              g_message (_("The PNG file specifies an offset that caused "
                           "the layer to be positioned outside the image."));
            }
        }
    }

  if (png_get_valid (pp, info, PNG_INFO_pHYs))
    {
      png_uint_32  xres;
      png_uint_32  yres;
      gint         unit_type;

      if (png_get_pHYs (pp, info,
                        &xres, &yres, &unit_type) && xres > 0 && yres > 0)
        {
          switch (unit_type)
            {
            case PNG_RESOLUTION_UNKNOWN:
              {
                gdouble image_xres, image_yres;

                gimp_image_get_resolution (image, &image_xres, &image_yres);

                if (xres > yres)
                  image_xres = image_yres * (gdouble) xres / (gdouble) yres;
                else
                  image_yres = image_xres * (gdouble) yres / (gdouble) xres;

                gimp_image_set_resolution (image, image_xres, image_yres);
              }
              break;

            case PNG_RESOLUTION_METER:
              gimp_image_set_resolution (image,
                                         (gdouble) xres * 0.0254,
                                         (gdouble) yres * 0.0254);
              break;

            default:
              break;
            }
        }

    }

  gimp_image_set_filename (image, filename);

  /*
   * Load the colormap as necessary...
   */

  empty = 0; /* by default assume no full transparent palette entries */

  if (png_get_color_type (pp, info) & PNG_COLOR_MASK_PALETTE)
    {
      png_colorp palette;
      int num_palette;

      png_get_PLTE (pp, info, &palette, &num_palette);
      if (png_get_valid (pp, info, PNG_INFO_tRNS))
        {
          for (empty = 0; empty < 256 && alpha[empty] == 0; ++empty)
            /* Calculates number of fully transparent "empty" entries */;

          /*  keep at least one entry  */
          empty = MIN (empty, num_palette - 1);

          gimp_image_set_colormap (image, (guchar *) (palette + empty),
                                   num_palette - empty);
        }
      else
        {
          gimp_image_set_colormap (image, (guchar *) palette,
                                   num_palette);
        }
    }

  buffer = gimp_drawable_get_buffer (layer);

  /*
   * Temporary buffer...
   */

  tile_height = gimp_tile_height ();
  pixel = g_new0 (guchar, tile_height * png_get_image_width (pp, info) * bpp);
  pixels = g_new (guchar *, tile_height);

  for (i = 0; i < tile_height; i++)
    pixels[i] = pixel + png_get_image_width (pp, info) * bpp * i;

  /* Install our own error handler to handle incomplete PNG files better */
  error_data.buffer      = buffer;
  error_data.pixel       = pixel;
  error_data.file_format = file_format;
  error_data.tile_height = tile_height;
  error_data.width       = png_get_image_width (pp, info);
  error_data.height      = png_get_image_height (pp, info);
  error_data.bpp         = bpp;

  png_set_error_fn (pp, &error_data, on_read_error, NULL);

  for (pass = 0; pass < num_passes; pass++)
    {
      /*
       * This works if you are only reading one row at a time...
       */

      for (begin = 0, end = tile_height;
           begin < png_get_image_height (pp, info);
           begin += tile_height, end += tile_height)
        {
          if (end > png_get_image_height (pp, info))
            end = png_get_image_height (pp, info);

          num = end - begin;

          if (pass != 0)        /* to handle interlaced PiNGs */
            gegl_buffer_get (buffer,
                             GEGL_RECTANGLE (0, begin,
                                             png_get_image_width (pp, info),
                                             num),
                             1.0,
                             file_format,
                             pixel,
                             GEGL_AUTO_ROWSTRIDE,
                             GEGL_ABYSS_NONE);

          error_data.begin = begin;
          error_data.end   = end;
          error_data.num   = num;

          png_read_rows (pp, pixels, NULL, num);

          gegl_buffer_set (buffer,
                           GEGL_RECTANGLE (0, begin,
                                           png_get_image_width (pp, info),
                                           num),
                           0,
                           file_format,
                           pixel,
                           GEGL_AUTO_ROWSTRIDE);

          gimp_progress_update
            (((gdouble) pass +
              (gdouble) end / (gdouble) png_get_image_height (pp, info)) /
             (gdouble) num_passes);
        }
    }

  /* Switch back to default error handler */
  png_set_error_fn (pp, NULL, NULL, NULL);

  png_read_end (pp, info);

  if (png_get_text (pp, info, &text, &num_texts))
    {
      gchar *comment = NULL;

      for (i = 0; i < num_texts && !comment; i++, text++)
        {
          if (text->key == NULL || strcmp (text->key, "Comment"))
            continue;

          if (text->text_length > 0)   /*  tEXt  */
            {
              comment = g_convert (text->text, -1,
                                   "UTF-8", "ISO-8859-1",
                                   NULL, NULL, NULL);
            }
          else if (g_utf8_validate (text->text, -1, NULL))
            {                          /*  iTXt  */
              comment = g_strdup (text->text);
            }
        }

      if (comment && *comment)
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-comment",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (comment) + 1, comment);
          gimp_image_attach_parasite (image, parasite);
          gimp_parasite_free (parasite);
        }

      g_free (comment);
    }

#if defined(PNG_iCCP_SUPPORTED)
  /*
   * Get the iCCP (colour profile) chunk, if any, and attach it as
   * a parasite
   */

  {
    png_uint_32 proflen;
    png_charp   profname, profile;
    int         profcomp;

    if (png_get_iCCP (pp, info, &profname, &profcomp, &profile, &proflen))
      {
        GimpParasite *parasite;

        parasite = gimp_parasite_new ("icc-profile",
                                      GIMP_PARASITE_PERSISTENT |
                                      GIMP_PARASITE_UNDOABLE,
                                      proflen, profile);

        gimp_image_attach_parasite (image, parasite);
        gimp_parasite_free (parasite);

        if (profname)
          {
            gchar *tmp = g_convert (profname, strlen (profname),
                                    "ISO-8859-1", "UTF-8", NULL, NULL, NULL);

            if (tmp)
              {
                parasite = gimp_parasite_new ("icc-profile-name",
                                              GIMP_PARASITE_PERSISTENT |
                                              GIMP_PARASITE_UNDOABLE,
                                              strlen (tmp), tmp);
                gimp_image_attach_parasite (image, parasite);
                gimp_parasite_free (parasite);

                g_free (tmp);
              }
          }
      }
  }
#endif

  /*
   * Done with the file...
   */

  png_destroy_read_struct (&pp, &info, NULL);

  g_free (pixel);
  g_free (pixels);
  g_object_unref (buffer);
  free (pp);
  free (info);

  fclose (fp);

#if 0
  if (trns)
    {
      gimp_layer_add_alpha (layer);
      drawable = gimp_drawable_get (layer);
      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
                           drawable->height, TRUE, FALSE);

      pixel = g_new (guchar, tile_height * drawable->width * 2); /* bpp == 1 */

      for (begin = 0, end = tile_height;
           begin < drawable->height; begin += tile_height, end += tile_height)
        {
          if (end > drawable->height)
            end = drawable->height;
          num = end - begin;

          gimp_pixel_rgn_get_rect (&pixel_rgn, pixel, 0, begin,
                                   drawable->width, num);

          for (i = 0; i < tile_height * drawable->width; ++i)
            {
              pixel[i * 2 + 1] = alpha[pixel[i * 2]];
              pixel[i * 2] -= empty;
            }

          gimp_pixel_rgn_set_rect (&pixel_rgn, pixel, 0, begin,
                                   drawable->width, num);
        }

      g_free (pixel);
    }
#endif

  return image;
}

/*
 * 'offsets_dialog ()' - Asks the user about offsets when loading.
 */
static gboolean
offsets_dialog (gint offset_x,
                gint offset_y)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *label;
  gchar     *message;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Apply PNG Offset"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("Ignore PNG offset"),         GTK_RESPONSE_NO,
                            _("Apply PNG offset to layer"), GTK_RESPONSE_YES,

                            NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_YES,
                                           GTK_RESPONSE_NO,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_stock (GIMP_STOCK_QUESTION, GTK_ICON_SIZE_DIALOG);
  gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  message = g_strdup_printf (_("The PNG image you are importing specifies an "
                               "offset of %d, %d. Do you want to apply "
                               "this offset to the layer?"),
                             offset_x, offset_y);
  label = gtk_label_new (message);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_YES);

  gtk_widget_destroy (dialog);

  return run;
}

/*
 * 'save_image ()' - Save the specified image to a PNG file.
 */

static gboolean
save_image (const gchar  *filename,
            gint32        image_ID,
            gint32        drawable_ID,
            gint32        orig_image_ID,
            GError      **error)
{
  gint i, k,                    /* Looping vars */
    bpp = 0,                    /* Bytes per pixel */
    type,                       /* Type of drawable/layer */
    num_passes,                 /* Number of interlace passes in file */
    have_u16,                   /* save as 16 bit PNG */
    pass,                       /* Current pass in file */
    tile_height,                /* Height of tile in GIMP */
    width,                      /* image width */
    height,                     /* image height */
    begin,                      /* Beginning tile row */
    end,                        /* Ending tile row */
    num;                        /* Number of rows to load */
  FILE *fp;                     /* File pointer */
  GeglBuffer *buffer;           /* GEGL buffer for layer */
  const Babl *file_format;      /* BABL format of drawable */
  png_structp pp;               /* PNG read pointer */
  png_infop info;               /* PNG info pointer */
  gint offx, offy;              /* Drawable offsets from origin */
  guchar **pixels,              /* Pixel rows */
   *fixed,                      /* Fixed-up pixel data */
   *pixel;                      /* Pixel data */
  gdouble xres, yres;           /* GIMP resolution (dpi) */
  png_color_16 background;      /* Background color */
  png_time mod_time;            /* Modification time (ie NOW) */
  guchar red, green, blue;      /* Used for palette background */
  time_t cutime;                /* Time since epoch */
  struct tm *gmt;               /* GMT broken down */
  int color_type;
  int bit_depth;

  guchar remap[256];            /* Re-mapping for the palette */

  png_textp  text = NULL;

  have_u16 = (gimp_image_get_precision (image_ID) != GIMP_PRECISION_U8);

  pp = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!pp)
    {
      /* this could happen if the compile time and run-time libpng
         versions do not match. */

      g_set_error (error, 0, 0,
                   _("Error creating PNG write struct while saving '%s'."),
                   gimp_filename_to_utf8 (filename));
      return FALSE;
    }

  info = png_create_info_struct (pp);

  if (setjmp (png_jmpbuf (pp)))
    {
      g_set_error (error, 0, 0,
                   _("Error while saving '%s'. Could not save image."),
                   gimp_filename_to_utf8 (filename));
      return FALSE;
    }

  /*
   * Open the file and initialize the PNG write "engine"...
   */

  fp = g_fopen (filename, "wb");
  if (fp == NULL)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  png_init_io (pp, fp);

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  /*
   * Get the buffer for the current image...
   */

  buffer = gimp_drawable_get_buffer (drawable_ID);
  width = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);
  type = gimp_drawable_type (drawable_ID);

  /* this is a stupid workaround for some caching issues in GEGL */
  gegl_buffer_copy (buffer, NULL, buffer, NULL);

  /*
   * Initialise remap[]
   */
  for (i = 0; i < 256; i++)
    remap[i] = i;

  /*
   * Set color type and remember bytes per pixel count
   */

  bit_depth = have_u16 ? 16 : 8;

  switch (type)
    {
    case GIMP_RGB_IMAGE:
      color_type = PNG_COLOR_TYPE_RGB;
      file_format = babl_format (have_u16 ? "R'G'B' u16" : "R'G'B' u8");
      break;

    case GIMP_RGBA_IMAGE:
      color_type = PNG_COLOR_TYPE_RGB_ALPHA;
      file_format = babl_format (have_u16 ? "R'G'B'A u16" : "R'G'B'A u8");
      break;

    case GIMP_GRAY_IMAGE:
      color_type = PNG_COLOR_TYPE_GRAY;
      file_format = babl_format (have_u16 ? "Y' u16" : "Y' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
      file_format = babl_format (have_u16 ? "Y'A u16" : "Y'A u8");
      break;

#if 0   /* indexed missing */
    case GIMP_INDEXED_IMAGE:
      bpp = 1;
      color_type = PNG_COLOR_TYPE_PALETTE;
      pngg.has_plte = TRUE;
      pngg.palette = (png_colorp) gimp_image_get_colormap (image_ID,
                                                           &pngg.num_palette);
      bit_depth = get_bit_depth_for_palette (pngg.num_palette);
      break;

    case GIMP_INDEXEDA_IMAGE:
      bpp = 2;
      color_type = PNG_COLOR_TYPE_PALETTE;
      /* fix up transparency */
      bit_depth = respin_cmap (pp, info, remap, image_ID, drawable);
      break;
#endif

    default:
      g_set_error (error, 0, 0, "Image type can't be saved as PNG");
      return FALSE;
    }

  bpp = babl_format_get_bytes_per_pixel (file_format);

  /* Note: png_set_IHDR() must be called before any other png_set_*()
     functions. */
  png_set_IHDR (pp, info, width, height, bit_depth, color_type,
                pngvals.interlaced ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);

  if (pngg.has_trns)
    {
      png_set_tRNS (pp, info, pngg.trans, pngg.num_trans, NULL);
    }

  if (pngg.has_plte)
    {
      png_set_PLTE (pp, info, pngg.palette, pngg.num_palette);
    }

  /* Set the compression level */

  png_set_compression_level (pp, pngvals.compression_level);

  /* All this stuff is optional extras, if the user is aiming for smallest
     possible file size she can turn them all off */

  if (pngvals.bkgd)
    {
      GimpRGB color;

      gimp_context_get_background (&color);
      gimp_rgb_get_uchar (&color, &red, &green, &blue);

      background.index = 0;
      background.red = red;
      background.green = green;
      background.blue = blue;
      background.gray = gimp_rgb_luminance_uchar (&color);
      png_set_bKGD (pp, info, &background);
    }
  else
    {
      /* used to save_transp_pixels */
      red = green = blue = 0;
    }

  if (pngvals.gama)
    {
      GimpParasite *parasite;
      gdouble       gamma = 1.0 / DEFAULT_GAMMA;

      parasite = gimp_image_get_parasite (orig_image_ID, "gamma");
      if (parasite)
        {
          gamma = g_ascii_strtod (gimp_parasite_data (parasite), NULL);
          gimp_parasite_free (parasite);
        }

      png_set_gAMA (pp, info, gamma);
    }

  if (pngvals.offs)
    {
      gimp_drawable_offsets (drawable_ID, &offx, &offy);
      if (offx != 0 || offy != 0)
        {
          png_set_oFFs (pp, info, offx, offy, PNG_OFFSET_PIXEL);
        }
    }

  if (pngvals.phys)
    {
      gimp_image_get_resolution (orig_image_ID, &xres, &yres);
      png_set_pHYs (pp, info, RINT (xres / 0.0254), RINT (yres / 0.0254),
                    PNG_RESOLUTION_METER);
    }

  if (pngvals.time)
    {
      cutime = time (NULL);     /* time right NOW */
      gmt = gmtime (&cutime);

      mod_time.year = gmt->tm_year + 1900;
      mod_time.month = gmt->tm_mon + 1;
      mod_time.day = gmt->tm_mday;
      mod_time.hour = gmt->tm_hour;
      mod_time.minute = gmt->tm_min;
      mod_time.second = gmt->tm_sec;
      png_set_tIME (pp, info, &mod_time);
    }

#if defined(PNG_iCCP_SUPPORTED)
  {
    GimpParasite *profile_parasite;
    gchar        *profile_name = NULL;

    profile_parasite = gimp_image_get_parasite (orig_image_ID, "icc-profile");

    if (profile_parasite)
      {
        GimpParasite *parasite = gimp_image_get_parasite (orig_image_ID,
                                                          "icc-profile-name");
        if (parasite)
          profile_name = g_convert (gimp_parasite_data (parasite),
                                    gimp_parasite_data_size (parasite),
                                    "UTF-8", "ISO-8859-1", NULL, NULL, NULL);

        png_set_iCCP (pp, info,
                      profile_name ? profile_name : "ICC profile", 0,
                      (gchar *) gimp_parasite_data (profile_parasite),
                      gimp_parasite_data_size (profile_parasite));

        g_free (profile_name);
      }
  }
#endif

  if (pngvals.comment)
    {
      GimpParasite *parasite;
#ifndef PNG_iTXt_SUPPORTED
      gsize text_length = 0;
#endif /* PNG_iTXt_SUPPORTED */

      parasite = gimp_image_get_parasite (orig_image_ID, "gimp-comment");
      if (parasite)
        {
          gchar *comment = g_strndup (gimp_parasite_data (parasite),
                                      gimp_parasite_data_size (parasite));

          gimp_parasite_free (parasite);

          text = g_new0 (png_text, 1);
          text->key         = "Comment";

#ifdef PNG_iTXt_SUPPORTED

          text->compression = PNG_ITXT_COMPRESSION_NONE;
          text->text        = comment;
          text->itxt_length = strlen (comment);

#else

          text->compression = PNG_TEXT_COMPRESSION_NONE;
          text->text        = g_convert (comment, -1,
                                         "ISO-8859-1", "UTF-8",
                                         NULL, &text_length,
                                         NULL);
          text->text_length = text_length;

#endif

          if (!text->text)
            {
              g_free (text);
              text = NULL;
            }
        }
    }

  if (text)
    png_set_text (pp, info, text, 1);

  png_write_info (pp, info);
  if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
    png_set_swap (pp);

  /*
   * Turn on interlace handling...
   */

  if (pngvals.interlaced)
    num_passes = png_set_interlace_handling (pp);
  else
    num_passes = 1;

  /*
   * Convert unpacked pixels to packed if necessary
   */

  if (color_type == PNG_COLOR_TYPE_PALETTE &&
      bit_depth < 8)
    png_set_packing (pp);

  /*
   * Allocate memory for "tile_height" rows and save the image...
   */

  tile_height = gimp_tile_height ();
  pixel = g_new (guchar, tile_height * width * bpp);
  pixels = g_new (guchar *, tile_height);

  for (i = 0; i < tile_height; i++)
    pixels[i] = pixel + width * bpp * i;

  for (pass = 0; pass < num_passes; pass++)
    {
      /* This works if you are only writing one row at a time... */
      for (begin = 0, end = tile_height;
           begin < height; begin += tile_height, end += tile_height)
        {
          if (end > height)
            end = height;

          num = end - begin;

          gegl_buffer_get (buffer,
                           GEGL_RECTANGLE (0, begin, width, num),
                           1.0,
                           file_format,
                           pixel,
                           GEGL_AUTO_ROWSTRIDE,
                           GEGL_ABYSS_NONE);

          /* If we are with a RGBA image and have to pre-multiply the
             alpha channel */
          if (bpp == 4 && ! pngvals.save_transp_pixels)
            {
              for (i = 0; i < num; ++i)
                {
                  fixed = pixels[i];
                  for (k = 0; k < width; ++k)
                    {
                      gint aux = k << 2;

                      if (! fixed[aux + 3])
                        {
                          fixed[aux + 0] = red;
                          fixed[aux + 1] = green;
                          fixed[aux + 2] = blue;
                        }
                    }
                }
            }

          if (bpp == 8 && ! pngvals.save_transp_pixels)
            {
              for (i = 0; i < num; ++i)
                {
                  fixed = pixels[i];
                  for (k = 0; k < width; ++k)
                    {
                      gint aux = k << 3;

                      if (! fixed[aux + 6] && ! fixed[aux + 7])
                        {
                          fixed[aux + 0] = red;
                          fixed[aux + 1] = red;
                          fixed[aux + 2] = green;
                          fixed[aux + 3] = green;
                          fixed[aux + 4] = blue;
                          fixed[aux + 5] = blue;
                        }
                    }
                }
            }

          /* If we're dealing with a paletted image with
           * transparency set, write out the remapped palette */
          if (png_get_valid (pp, info, PNG_INFO_tRNS))
            {
              guchar inverse_remap[256];

              for (i = 0; i < 256; i++)
                inverse_remap[ remap[i] ] = i;

              for (i = 0; i < num; ++i)
                {
                  fixed = pixels[i];
                  for (k = 0; k < width; ++k)
                    {
                      fixed[k] = (fixed[k*2+1] > 127) ?
                                 inverse_remap[ fixed[k*2] ] :
                                 0;
                    }
                }
            }

          /* Otherwise if we have a paletted image and transparency
           * couldn't be set, we ignore the alpha channel */
          else if (png_get_valid (pp, info, PNG_INFO_PLTE) &&
                   bpp == 2)
            {
              for (i = 0; i < num; ++i)
                {
                  fixed = pixels[i];
                  for (k = 0; k < width; ++k)
                    {
                      fixed[k] = fixed[k * 2];
                    }
                }
            }

          png_write_rows (pp, pixels, num);

          gimp_progress_update (((double) pass + (double) end /
                                 (double) height) /
                                (double) num_passes);
        }
    }

  gimp_progress_update (1.0);

  png_write_end (pp, info);
  png_destroy_write_struct (&pp, &info);

  g_free (pixel);
  g_free (pixels);

  /*
   * Done with the file...
   */

  if (text)
    {
      g_free (text->text);
      g_free (text);
    }

  free (pp);
  free (info);

  fclose (fp);

  return TRUE;
}

static gboolean
ia_has_transparent_pixels (GimpDrawable *drawable)
{
  GimpPixelRgn  pixel_rgn;
  gpointer      pr;
  guchar       *pixel_row;
  gint          row, col;
  guchar       *pixel;

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &pixel_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      pixel_row = pixel_rgn.data;
      for (row = 0; row < pixel_rgn.h; row++)
        {
          pixel = pixel_row;
          for (col = 0; col < pixel_rgn.w; col++)
            {
              if (pixel[1] <= 127)
                return TRUE;
              pixel += 2;
            }
          pixel_row += pixel_rgn.rowstride;
        }
    }

  return FALSE;
}

/* Try to find a color in the palette which isn't actually
 * used in the image, so that we can use it as the transparency
 * index. Taken from gif.c */
static gint
find_unused_ia_color (GimpDrawable *drawable,
                      gint         *colors)
{
  gboolean      ix_used[256];
  gboolean      trans_used = FALSE;
  GimpPixelRgn  pixel_rgn;
  gpointer      pr;
  gint          row, col;
  gint          i;

  for (i = 0; i < *colors; i++)
    ix_used[i] = FALSE;

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                       drawable->width, drawable->height, FALSE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &pixel_rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      const guchar *pixel_row = pixel_rgn.data;

      for (row = 0; row < pixel_rgn.h; row++)
        {
          const guchar *pixel = pixel_row;

          for (col = 0; col < pixel_rgn.w; col++)
            {
              if (pixel[1] > 127)
                ix_used[ pixel[0] ] = TRUE;
              else
                trans_used = TRUE;

              pixel += 2;
            }

          pixel_row += pixel_rgn.rowstride;
        }
    }

  /* If there is no transparency, ignore alpha. */
  if (trans_used == FALSE)
    return -1;

  for (i = 0; i < *colors; i++)
    {
      if (ix_used[i] == FALSE)
        return i;
    }

  /* Couldn't find an unused color index within the number of
     bits per pixel we wanted.  Will have to increment the number
     of colors in the image and assign a transparent pixel there. */
  if ((*colors) < 256)
    {
      (*colors)++;

      return (*colors) - 1;
    }

  return -1;
}


static int
respin_cmap (png_structp   pp,
             png_infop     info,
             guchar       *remap,
             gint32        image_ID,
             GimpDrawable *drawable)
{
  static guchar trans[] = { 0 };

  gint          colors;
  guchar       *before;

  before = gimp_image_get_colormap (image_ID, &colors);

  /*
   * Make sure there is something in the colormap.
   */
  if (colors == 0)
    {
      before = g_new0 (guchar, 3);
      colors = 1;
    }

  /* Try to find an entry which isn't actually used in the
     image, for a transparency index. */

  if (ia_has_transparent_pixels (drawable))
    {
      gint transparent = find_unused_ia_color (drawable, &colors);

      if (transparent != -1)        /* we have a winner for a transparent
                                     * index - do like gif2png and swap
                                     * index 0 and index transparent */
        {
          static png_color palette[256];
          gint      i;

          /* Set tRNS chunk values for writing later. */
          pngg.has_trns = TRUE;
          pngg.trans = trans;
          pngg.num_trans = 1;

          /* Transform all pixels with a value = transparent to
           * 0 and vice versa to compensate for re-ordering in palette
           * due to png_set_tRNS() */

          remap[0] = transparent;
          for (i = 1; i <= transparent; i++)
            remap[i] = i - 1;

          /* Copy from index 0 to index transparent - 1 to index 1 to
           * transparent of after, then from transparent+1 to colors-1
           * unchanged, and finally from index transparent to index 0. */

          for (i = 0; i < colors; i++)
            {
              palette[i].red = before[3 * remap[i]];
              palette[i].green = before[3 * remap[i] + 1];
              palette[i].blue = before[3 * remap[i] + 2];
            }

          /* Set PLTE chunk values for writing later. */
          pngg.has_plte = TRUE;
          pngg.palette = palette;
          pngg.num_palette = colors;
        }
      else
        {
          /* Inform the user that we couldn't losslessly save the
           * transparency & just use the full palette */
          g_message (_("Couldn't losslessly save transparency, "
                       "saving opacity instead."));

          /* Set PLTE chunk values for writing later. */
          pngg.has_plte = TRUE;
          pngg.palette = (png_colorp) before;
          pngg.num_palette = colors;
        }
    }
  else
    {
      /* Set PLTE chunk values for writing later. */
      pngg.has_plte = TRUE;
      pngg.palette = (png_colorp) before;
      pngg.num_palette = colors;
    }

  return get_bit_depth_for_palette (colors);
}

static GtkWidget *
toggle_button_init (GtkBuilder  *builder,
                    const gchar *name,
                    gboolean     initial_value,
                    gboolean    *value_pointer)
{
  GtkWidget *toggle = NULL;

  toggle = GTK_WIDGET (gtk_builder_get_object (builder, name));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), initial_value);
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    value_pointer);

  return toggle;
}

static gboolean
save_dialog (gint32    image_ID,
             gboolean  alpha)
{
  PngSaveGui    pg;
  GtkWidget    *dialog;
  GtkBuilder   *builder;
  gchar        *ui_file;
  GimpParasite *parasite;
  GError       *error = NULL;

  /* Dialog init */
  dialog = gimp_export_dialog_new (_("PNG"), PLUG_IN_BINARY, SAVE_PROC);
  g_signal_connect (dialog, "response",
                    G_CALLBACK (save_dialog_response),
                    &pg);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  /* GtkBuilder init */
  builder = gtk_builder_new ();
  ui_file = g_build_filename (gimp_data_directory (),
                              "ui/plug-ins/plug-in-file-png.ui",
                              NULL);
  if (! gtk_builder_add_from_file (builder, ui_file, &error))
    {
      gchar *display_name = g_filename_display_name (ui_file);

      g_printerr (_("Error loading UI file '%s': %s"),
                  display_name, error ? error->message : _("Unknown error"));

      g_free (display_name);
    }

  g_free (ui_file);

  /* Table */
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      GTK_WIDGET (gtk_builder_get_object (builder, "table")),
                      TRUE, TRUE, 0);

  /* Toggles */
  pg.interlaced = toggle_button_init (builder, "interlace",
                                      pngvals.interlaced,
                                      &pngvals.interlaced);
  pg.bkgd = toggle_button_init (builder, "save-background-color",
                                pngvals.bkgd,
                                &pngvals.bkgd);
  pg.gama = toggle_button_init (builder, "save-gamma",
                                pngvals.gama,
                                &pngvals.gama);
  pg.offs = toggle_button_init (builder, "save-layer-offset",
                                pngvals.offs,
                                &pngvals.offs);
  pg.phys = toggle_button_init (builder, "save-resolution",
                                pngvals.phys,
                                &pngvals.phys);
  pg.time = toggle_button_init (builder, "save-creation-time",
                                pngvals.time,
                                &pngvals.time);

  /* Comment toggle */
  parasite = gimp_image_get_parasite (image_ID, "gimp-comment");
  pg.comment =
    toggle_button_init (builder, "save-comment",
                        pngvals.comment && parasite != NULL,
                        &pngvals.comment);
  gtk_widget_set_sensitive (pg.comment, parasite != NULL);
  gimp_parasite_free (parasite);

  /* Transparent pixels toggle */
  pg.save_transp_pixels =
    toggle_button_init (builder,
                        "save-transparent-pixels",
                        alpha && pngvals.save_transp_pixels,
                        &pngvals.save_transp_pixels);
  gtk_widget_set_sensitive (pg.save_transp_pixels, alpha);

  /* Compression level scale */
  pg.compression_level =
    GTK_ADJUSTMENT (gtk_builder_get_object (builder, "compression-level"));
  gtk_adjustment_set_value (pg.compression_level, pngvals.compression_level);
  g_signal_connect (pg.compression_level, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &pngvals.compression_level);

  /* Load/save defaults buttons */
  g_signal_connect_swapped (gtk_builder_get_object (builder, "load-defaults"),
                            "clicked",
                            G_CALLBACK (load_gui_defaults),
                            &pg);

  g_signal_connect_swapped (gtk_builder_get_object (builder, "save-defaults"),
                            "clicked",
                            G_CALLBACK (save_defaults),
                            &pg);

  /* Show dialog and run */
  gtk_widget_show (dialog);

  pg.run = FALSE;

  gtk_main ();

  return pg.run;
}

static void
save_dialog_response (GtkWidget *widget,
                      gint       response_id,
                      gpointer   data)
{
  PngSaveGui *pg = data;

  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      pg->run = TRUE;

    default:
      gtk_widget_destroy (widget);
      break;
    }
}

static void
load_defaults (void)
{
  GimpParasite *parasite;

  parasite = gimp_get_parasite (PNG_DEFAULTS_PARASITE);

  if (parasite)
    {
      gchar        *def_str;
      PngSaveVals   tmpvals;
      gint          num_fields;

      def_str = g_strndup (gimp_parasite_data (parasite),
                           gimp_parasite_data_size (parasite));

      gimp_parasite_free (parasite);

      num_fields = sscanf (def_str, "%d %d %d %d %d %d %d %d %d",
                           &tmpvals.interlaced,
                           &tmpvals.bkgd,
                           &tmpvals.gama,
                           &tmpvals.offs,
                           &tmpvals.phys,
                           &tmpvals.time,
                           &tmpvals.comment,
                           &tmpvals.save_transp_pixels,
                           &tmpvals.compression_level);

      g_free (def_str);

      if (num_fields == 9)
        {
          memcpy (&pngvals, &tmpvals, sizeof (tmpvals));
          return;
        }
    }

  memcpy (&pngvals, &defaults, sizeof (defaults));
}

static void
save_defaults (void)
{
  GimpParasite *parasite;
  gchar        *def_str;

  def_str = g_strdup_printf ("%d %d %d %d %d %d %d %d %d",
                             pngvals.interlaced,
                             pngvals.bkgd,
                             pngvals.gama,
                             pngvals.offs,
                             pngvals.phys,
                             pngvals.time,
                             pngvals.comment,
                             pngvals.save_transp_pixels,
                             pngvals.compression_level);

  parasite = gimp_parasite_new (PNG_DEFAULTS_PARASITE,
                                GIMP_PARASITE_PERSISTENT,
                                strlen (def_str), def_str);

  gimp_attach_parasite (parasite);

  gimp_parasite_free (parasite);
  g_free (def_str);
}

static void
load_gui_defaults (PngSaveGui *pg)
{
  load_defaults ();

#define SET_ACTIVE(field) \
  if (gtk_widget_is_sensitive (pg->field)) \
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pg->field), pngvals.field)

  SET_ACTIVE (interlaced);
  SET_ACTIVE (bkgd);
  SET_ACTIVE (gama);
  SET_ACTIVE (offs);
  SET_ACTIVE (phys);
  SET_ACTIVE (time);
  SET_ACTIVE (comment);
  SET_ACTIVE (save_transp_pixels);

#undef SET_ACTIVE

  gtk_adjustment_set_value (pg->compression_level,
                            pngvals.compression_level);
}
