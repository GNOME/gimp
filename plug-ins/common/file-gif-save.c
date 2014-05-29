/* GIF saving file filter for GIMP
 *
 *    Copyright
 *    - Adam D. Moss
 *    - Peter Mattis
 *    - Spencer Kimball
 *
 *      Based around original GIF code by David Koblas.
 *
 *
 * Version 4.1.0 - 2003-06-16
 *                        Adam D. Moss - <adam@gimp.org> <adam@foxbox.org>
 */
/*
 * This filter uses code taken from the "giftopnm" and "ppmtogif" programs
 *    which are part of the "netpbm" package.
 */
/*
 *  "The Graphics Interchange Format(c) is the Copyright property of
 *  CompuServe Incorporated.  GIF(sm) is a Service Mark property of
 *  CompuServe Incorporated."
 */
/* Copyright notice for GIF code from which this plugin was long ago     */
/* derived (David Koblas has granted permission to relicense):           */
/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993, David Koblas.  (koblas@extra.com)     | */
/* +-------------------------------------------------------------------+ */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-gif-save"
#define SAVE2_PROC     "file-gif-save2"
#define PLUG_IN_BINARY "file-gif-save"
#define PLUG_IN_ROLE   "gimp-file-gif-save"


/* uncomment the line below for a little debugging info */
/* #define GIFDEBUG yesplease */


enum
{
  DISPOSE_STORE_VALUE_COLUMN,
  DISPOSE_STORE_LABEL_COLUMN
};

enum
{
  DISPOSE_UNSPECIFIED,
  DISPOSE_COMBINE,
  DISPOSE_REPLACE
};

typedef struct
{
  gint     interlace;
  gint     save_comment;
  gint     loop;
  gint     default_delay;
  gint     default_dispose;
  gboolean always_use_default_delay;
  gboolean always_use_default_dispose;
  gboolean as_animation;
} GIFSaveVals;


/* Declare some local functions.
 */
static void     query                  (void);
static void     run                    (const gchar      *name,
                                        gint              nparams,
                                        const GimpParam  *param,
                                        gint             *nreturn_vals,
                                        GimpParam       **return_vals);

static gboolean  save_image            (const gchar      *filename,
                                        gint32            image_ID,
                                        gint32            drawable_ID,
                                        gint32            orig_image_ID,
                                        GError          **error);

static GimpPDBStatusType sanity_check  (const gchar      *filename,
                                        gint32           *image_ID,
                                        GError          **error);
static gboolean bad_bounds_dialog      (void);

static gboolean save_dialog            (gint32            image_ID);
static void     comment_entry_callback (GtkTextBuffer    *buffer);


static GimpRunMode   run_mode;
static GimpParasite *comment_parasite   = NULL;
static gboolean      comment_was_edited = FALSE;
static gchar        *globalcomment      = NULL;
static gint          Interlace; /* For compression code */


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static GIFSaveVals gsvals =
{
  FALSE,   /* interlace                            */
  TRUE,    /* save comment                         */
  TRUE,    /* loop infinitely                      */
  100,     /* default_delay between frames (100ms) */
  0,       /* default_dispose = "don't care"       */
  FALSE,   /* don't always use default_delay       */
  FALSE,   /* don't always use default_dispose     */
  FALSE    /* as_animation                         */
};


MAIN ()

#define COMMON_SAVE_ARGS \
    { GIMP_PDB_INT32,    "run-mode",        "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" }, \
    { GIMP_PDB_IMAGE,    "image",           "Image to save" }, \
    { GIMP_PDB_DRAWABLE, "drawable",        "Drawable to save" }, \
    { GIMP_PDB_STRING,   "filename",        "The name of the file to save the image in" }, \
    { GIMP_PDB_STRING,   "raw-filename",    "The name entered" }, \
    { GIMP_PDB_INT32,    "interlace",       "Try to save as interlaced" }, \
    { GIMP_PDB_INT32,    "loop",            "(animated gif) loop infinitely" }, \
    { GIMP_PDB_INT32,    "default-delay",   "(animated gif) Default delay between framese in milliseconds" }, \
    { GIMP_PDB_INT32,    "default-dispose", "(animated gif) Default disposal type (0=`don't care`, 1=combine, 2=replace)" }

static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    COMMON_SAVE_ARGS
  };

  static const GimpParamDef save2_args[] =
  {
    COMMON_SAVE_ARGS,
    { GIMP_PDB_INT32,    "as-animation", "Save GIF as animation?" },
    { GIMP_PDB_INT32,    "force-delay", "(animated gif) Use specified delay for all frames?" },
    { GIMP_PDB_INT32,    "force-dispose", "(animated gif) Use specified disposal for all frames?" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "saves files in Compuserve GIF file format",
                          "Save a file in Compuserve GIF format, with "
                          "possible animation, transparency, and comment.  "
                          "To save an animation, operate on a multi-layer "
                          "file.  The plug-in will interpret <50% alpha as "
                          "transparent.  When run non-interactively, the "
                          "value for the comment is taken from the "
                          "'gimp-comment' parasite.  ",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "1995-1997",
                          N_("GIF image"),
                          "INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_install_procedure (SAVE2_PROC,
                          "saves files in Compuserve GIF file format",
                          "Save a file in Compuserve GIF format, with "
                          "possible animation, transparency, and comment.  "
                          "To save an animation, operate on a multi-layer "
                          "file and give the 'as-animation' parameter "
                          "as TRUE.  The plug-in will interpret <50% "
                          "alpha as transparent.  When run "
                          "non-interactively, the value for the comment "
                          "is taken from the 'gimp-comment' parasite.  ",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "Spencer Kimball, Peter Mattis, Adam Moss, David Koblas",
                          "1995-1997",
                          N_("GIF image"),
                          "INDEXED*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save2_args), 0,
                          save2_args, NULL);

  gimp_register_file_handler_mime (SAVE_PROC, "image/gif");
  gimp_register_save_handler (SAVE_PROC, "gif", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;
  GError           *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, SAVE_PROC) == 0 || strcmp (name, SAVE2_PROC) == 0)
    {
      const gchar *filename;
      gint32       image_ID, sanitized_image_ID = 0;
      gint32       drawable_ID;
      gint32       orig_image_ID;

      image_ID    = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;
      filename    = param[3].data.d_string;

      if (run_mode == GIMP_RUN_INTERACTIVE ||
          run_mode == GIMP_RUN_WITH_LAST_VALS)
        gimp_ui_init (PLUG_IN_BINARY, FALSE);

      status = sanity_check (filename, &image_ID, &error);

      /* Get the export options */
      if (status == GIMP_PDB_SUCCESS)
        {
          /* If the sanity check succeeded, the image_ID will point to
           * a duplicate image to delete later. */
          sanitized_image_ID = image_ID;

          switch (run_mode)
            {
            case GIMP_RUN_INTERACTIVE:
              /*  Possibly retrieve data  */
              gimp_get_data (SAVE_PROC, &gsvals);

              /*  First acquire information with a dialog  */
              if (! save_dialog (image_ID))
                status = GIMP_PDB_CANCEL;
              break;

            case GIMP_RUN_NONINTERACTIVE:
              /*  Make sure all the arguments are there!  */
              if (nparams != 9 && nparams != 12)
                {
                  status = GIMP_PDB_CALLING_ERROR;
                }
              else
                {
                  gsvals.interlace       = (param[5].data.d_int32) ? TRUE : FALSE;
                  gsvals.save_comment    = TRUE;  /*  no way to to specify that through the PDB  */
                  gsvals.loop            = (param[6].data.d_int32) ? TRUE : FALSE;
                  gsvals.default_delay   = param[7].data.d_int32;
                  gsvals.default_dispose = param[8].data.d_int32;
                  if (nparams == 12)
                    {
                      gsvals.as_animation = (param[9].data.d_int32) ? TRUE : FALSE;
                      gsvals.always_use_default_delay = (param[10].data.d_int32) ? TRUE : FALSE;
                      gsvals.always_use_default_dispose = (param[11].data.d_int32) ? TRUE : FALSE;
                    }
                }
              break;

            case GIMP_RUN_WITH_LAST_VALS:
              /*  Possibly retrieve data  */
              gimp_get_data (SAVE_PROC, &gsvals);
              break;

            default:
              break;
            }
        }

      /* Create an exportable image based on the export options */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          {
            GimpExportCapabilities capabilities =
              GIMP_EXPORT_CAN_HANDLE_INDEXED |
              GIMP_EXPORT_CAN_HANDLE_GRAY |
              GIMP_EXPORT_CAN_HANDLE_ALPHA;

            if (gsvals.as_animation)
              capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

            export = gimp_export_image (&image_ID, &drawable_ID, "GIF",
                                        capabilities);

            if (export == GIMP_EXPORT_CANCEL)
              {
                values[0].data.d_status = GIMP_PDB_CANCEL;
                if (sanitized_image_ID)
                  gimp_image_delete (sanitized_image_ID);
                return;
              }
          }
          break;
        default:
          break;
        }

      /* Write the image to file */
      if (status == GIMP_PDB_SUCCESS)
        {
          if (save_image (param[3].data.d_string,
                          image_ID, drawable_ID, orig_image_ID,
                          &error))
            {
              /*  Store psvals data  */
              gimp_set_data (SAVE_PROC, &gsvals, sizeof (GIFSaveVals));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }

          gimp_image_delete (sanitized_image_ID);
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}


/* ppmtogif.c - read a portable pixmap and produce a GIF file
**
** Based on GIFENCOD by David Rowley <mgardi@watdscu.waterloo.edu>. A
** Lempel-Ziv compression based on "compress".
**
** Modified by Marcel Wijkstra <wijkstra@fwi.uva.nl>
**
**
** Copyright (C) 1989 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
**
** The Graphics Interchange Format(c) is the Copyright property of
** CompuServe Incorporated.  GIF(sm) is a Service Mark property of
** CompuServe Incorporated.
*/

#define MAXCOLORS 256

/*
 * Pointer to function returning an int
 */
typedef int (*ifunptr) (int, int);


static gint find_unused_ia_color   (const guchar *pixels,
                                     gint          numpixels,
                                     gint          num_indices,
                                     gint         *colors);

static void special_flatten_indexed_alpha (guchar *pixels,
                                           gint    transparent,
                                           gint    numpixels);
static gint colors_to_bpp  (int);
static gint bpp_to_colors  (int);
static gint get_pixel      (int, int);
static gint gif_next_pixel (ifunptr);
static void bump_pixel     (void);

static void gif_encode_header              (FILE *, gboolean, int, int, int, int,
                                            int *, int *, int *, ifunptr);
static void gif_encode_graphic_control_ext (FILE *, int, int, int, int,
                                            int, int, int, ifunptr);
static void gif_encode_image_data          (FILE *, int, int, int, int,
                                            ifunptr, gint, gint);
static void gif_encode_close               (FILE *);
static void gif_encode_loop_ext            (FILE *, guint);
static void gif_encode_comment_ext         (FILE *, const gchar *comment);

static gint     rowstride;
static guchar  *pixels;
static gint     cur_progress;
static gint     max_progress;

static void compress        (int      init_bits,
                             FILE    *outfile,
                             ifunptr  ReadValue);
static void no_compress     (int      init_bits,
                             FILE    *outfile,
                             ifunptr  ReadValue);
static void rle_compress    (int      init_bits,
                             FILE    *outfile,
                             ifunptr  ReadValue);
static void normal_compress (int      init_bits,
                             FILE    *outfile,
                             ifunptr  ReadValue);

static void put_word        (int, FILE *);
static void output          (gint);
static void cl_block        (void);
static void cl_hash         (glong);
static void char_init       (void);
static void char_out        (int);
static void flush_char      (void);


static gint
find_unused_ia_color (const guchar *pixels,
                       gint          numpixels,
                       gint          num_indices,
                       gint         *colors)
{
  gboolean ix_used[256];
  gint     i;

#ifdef GIFDEBUG
  g_printerr ("GIF: fuiac: Image claims to use %d/%d indices - finding free "
              "index...\n", *colors, num_indices);
#endif

  for (i = 0; i < 256; i++)
    ix_used[i] = FALSE;

  for (i = 0; i < numpixels; i++)
    {
      if (pixels[i * 2 + 1])
        ix_used[pixels[i * 2]] = TRUE;
    }

  for (i = num_indices - 1; i >= 0; i--)
    {
      if (! ix_used[i])
        {
#ifdef GIFDEBUG
          g_printerr ("GIF: Found unused color index %d.\n", (int) i);
#endif
          return i;
        }
    }

  /* Couldn't find an unused color index within the number of
     bits per pixel we wanted.  Will have to increment the number
     of colors in the image and assign a transparent pixel there. */
  if (*colors < 256)
    {
      (*colors)++;

      g_printerr ("GIF: 2nd pass "
                  "- Increasing bounds and using color index %d.\n",
                  *colors - 1);
      return ((*colors) - 1);
    }

  g_message (_("Couldn't simply reduce colors further. Saving as opaque."));

  return -1;
}


static void
special_flatten_indexed_alpha (guchar *pixels,
                               gint    transparent,
                               gint    numpixels)
{
  guint32 i;

  /* Each transparent pixel in the image is mapped to a uniform value for
     encoding, if image already has <=255 colors */

  if (transparent == -1) /* tough, no indices left for the trans. index */
    {
      for (i = 0; i < numpixels; i++)
        pixels[i] = pixels[i * 2];
    }
  else  /* make transparent */
    {
      for (i = 0; i < numpixels; i++)
        {
          if (! (pixels[i * 2 + 1] & 128))
            {
              pixels[i] = (guchar) transparent;
            }
          else
            {
              pixels[i] = pixels[i * 2];
            }
        }
    }
}


static gint
parse_ms_tag (const gchar *str)
{
  gint sum    = 0;
  gint offset = 0;
  gint length;

  length = strlen (str);

find_another_bra:

  while ((offset < length) && (str[offset] != '('))
    offset++;

  if (offset >= length)
    return(-1);

  if (! g_ascii_isdigit (str[++offset]))
    goto find_another_bra;

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset < length) && (g_ascii_isdigit (str[offset])));

  if (length - offset <= 2)
    return(-3);

  if ((g_ascii_toupper (str[offset]) != 'M')
      || (g_ascii_toupper (str[offset+1]) != 'S'))
    return -4;

  return sum;
}


static gint
parse_disposal_tag (const gchar *str)
{
  gint offset = 0;
  gint length;

  length = strlen(str);

  while ((offset + 9) <= length)
    {
      if (strncmp(&str[offset], "(combine)", 9) == 0)
        return(0x01);
      if (strncmp(&str[offset], "(replace)", 9) == 0)
        return(0x02);
      offset++;
    }

  return (gsvals.default_dispose);
}


static GimpPDBStatusType
sanity_check (const gchar  *filename,
              gint32       *image_ID,
              GError      **error)
{
  gint32 *layers;
  gint    nlayers;
  gint    image_width;
  gint    image_height;
  gint    i;

  image_width  = gimp_image_width (*image_ID);
  image_height = gimp_image_height (*image_ID);

  if (image_width > G_MAXUSHORT || image_height > G_MAXUSHORT)
    {
      g_set_error (error, 0, 0,
                   _("Unable to save '%s'.  "
                   "The GIF file format does not support images that are "
                   "more than %d pixels wide or tall."),
                   gimp_filename_to_utf8 (filename), G_MAXUSHORT);

      return GIMP_PDB_EXECUTION_ERROR;
    }

  /*** Iterate through the layers to make sure they're all ***/
  /*** within the bounds of the image                      ***/

  *image_ID = gimp_image_duplicate (*image_ID);
  layers = gimp_image_get_layers (*image_ID, &nlayers);

  for (i = 0; i < nlayers; i++)
    {
      gint offset_x;
      gint offset_y;

      gimp_drawable_offsets (layers[i], &offset_x, &offset_y);

      if (offset_x < 0 ||
          offset_y < 0 ||
          offset_x + gimp_drawable_width (layers[i]) > image_width ||
          offset_y + gimp_drawable_height (layers[i]) > image_height)
        {
          g_free (layers);

          /* Image has illegal bounds - ask the user what it wants to do */

          /* Do the crop if we can't talk to the user, or if we asked
           * the user and they said yes.
           */
          if ((run_mode == GIMP_RUN_NONINTERACTIVE) || bad_bounds_dialog ())
            {
              gimp_image_crop (*image_ID, image_width, image_height, 0, 0);
              return GIMP_PDB_SUCCESS;
            }
          else
            {
              gimp_image_delete (*image_ID);
              return GIMP_PDB_CANCEL;
            }
        }
    }

  g_free (layers);

  return GIMP_PDB_SUCCESS;
}


static gboolean
save_image (const gchar *filename,
            gint32       image_ID,
            gint32       drawable_ID,
            gint32       orig_image_ID,
            GError     **error)
{
  GeglBuffer    *buffer;
  GimpImageType  drawable_type;
  const Babl    *format = NULL;
  FILE          *outfile;
  gint           Red[MAXCOLORS];
  gint           Green[MAXCOLORS];
  gint           Blue[MAXCOLORS];
  guchar        *cmap;
  guint          rows, cols;
  gint           BitsPerPixel, liberalBPP = 0, useBPP = 0;
  gint           colors;
  gint           i;
  gint           transparent;
  gint           offset_x, offset_y;

  gint32        *layers;
  gint           nlayers;

  gboolean       is_gif89 = FALSE;

  gint           Delay89;
  gint           Disposal;
  gchar         *layer_name;

  GimpRGB        background;
  guchar         bgred, bggreen, bgblue;
  guchar         bgindex = 0;
  guint          best_error = 0xFFFFFFFF;

  /* Save the comment back to the ImageID, if appropriate */
  if (globalcomment != NULL && comment_was_edited)
    {
      comment_parasite = gimp_parasite_new ("gimp-comment",
                                            GIMP_PARASITE_PERSISTENT,
                                            strlen (globalcomment) + 1,
                                            (void*) globalcomment);
      gimp_image_attach_parasite (orig_image_ID, comment_parasite);
      gimp_parasite_free (comment_parasite);
      comment_parasite = NULL;
    }

  /* The GIF spec says 7bit ASCII for the comment block. */
  if (gsvals.save_comment && globalcomment)
    {
      const gchar *c   = globalcomment;
      gint         len;

      for (len = strlen (c); len; c++, len--)
        {
          if ((guchar) *c > 127)
            {
              g_message (_("The GIF format only supports comments in "
                           "7bit ASCII encoding. No comment is saved."));

              g_free (globalcomment);
              globalcomment = NULL;

              break;
            }
        }
    }

  /* get a list of layers for this image_ID */
  layers = gimp_image_get_layers (image_ID, &nlayers);

  drawable_type = gimp_drawable_type (layers[0]);

  /* If the image has multiple layers (i.e. will be animated), a comment,
     or transparency, then it must be encoded as a GIF89a file, not a vanilla
     GIF87a. */
  if (nlayers > 1)
    is_gif89 = TRUE;

  if (gsvals.save_comment)
    is_gif89 = TRUE;

  switch (drawable_type)
    {
    case GIMP_INDEXEDA_IMAGE:
      is_gif89 = TRUE;
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_image_get_colormap (image_ID, &colors);

      gimp_context_get_background (&background);
      gimp_rgb_get_uchar (&background, &bgred, &bggreen, &bgblue);

      for (i = 0; i < colors; i++)
        {
          Red[i]   = *cmap++;
          Green[i] = *cmap++;
          Blue[i]  = *cmap++;
        }
      for ( ; i < 256; i++)
        {
          Red[i]   = bgred;
          Green[i] = bggreen;
          Blue[i]  = bgblue;
        }
      break;
    case GIMP_GRAYA_IMAGE:
      is_gif89 = TRUE;
    case GIMP_GRAY_IMAGE:
      colors = 256;                   /* FIXME: Not ideal. */
      for ( i = 0;  i < 256; i++)
        {
          Red[i] = Green[i] = Blue[i] = i;
        }

      if (drawable_type == GIMP_GRAYA_IMAGE)
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
      break;

    default:
      g_message (_("Cannot save RGB color images. Convert to "
                   "indexed color or grayscale first."));
      return FALSE;
    }


  /* find earliest index in palette which is closest to the background
     color, and ATTEMPT to use that as the GIF's default background color. */
  for (i = 255; i >= 0; --i)
    {
      guint local_error = 0;

      local_error += (Red[i] - bgred)     * (Red[i] - bgred);
      local_error += (Green[i] - bggreen) * (Green[i] - bggreen);
      local_error += (Blue[i] - bgblue)   * (Blue[i] - bgblue);

      if (local_error <= best_error)
        {
          bgindex = i;
          best_error = local_error;
        }
    }


  /* open the destination file for writing */
  outfile = g_fopen (filename, "wb");
  if (!outfile)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }


  /* init the progress meter */
  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));


  /* write the GIFheader */

  if (colors < 256)
    {
      /* we keep track of how many bits we promised to have in liberalBPP,
         so that we don't accidentally come under this when doing
         clever transparency stuff where we can re-use wasted indices. */
      liberalBPP = BitsPerPixel =
        colors_to_bpp (colors + ((drawable_type==GIMP_INDEXEDA_IMAGE) ? 1 : 0));
    }
  else
    {
      liberalBPP = BitsPerPixel =
        colors_to_bpp (256);

      if (drawable_type == GIMP_INDEXEDA_IMAGE)
        {
          g_printerr ("GIF: Too many colors?\n");
        }
    }

  cols = gimp_image_width (image_ID);
  rows = gimp_image_height (image_ID);
  Interlace = gsvals.interlace;
  gif_encode_header (outfile, is_gif89, cols, rows, bgindex,
                     BitsPerPixel, Red, Green, Blue, get_pixel);


  /* If the image has multiple layers it'll be made into an
     animated GIF, so write out the infinite-looping extension */
  if ((nlayers > 1) && (gsvals.loop))
    gif_encode_loop_ext (outfile, 0);

  /* Write comment extension - mustn't be written before the looping ext. */
  if (gsvals.save_comment && globalcomment)
    {
      gif_encode_comment_ext (outfile, globalcomment);
    }


  /*** Now for each layer in the image, save an image in a compound GIF ***/
  /************************************************************************/

  cur_progress = 0;
  max_progress = nlayers * rows;

  for (i = nlayers - 1; i >= 0; i--, cur_progress = (nlayers - i) * rows)
    {
      drawable_type = gimp_drawable_type (layers[i]);
      buffer = gimp_drawable_get_buffer (layers[i]);
      gimp_drawable_offsets (layers[i], &offset_x, &offset_y);
      cols = gimp_drawable_width (layers[i]);
      rows = gimp_drawable_height (layers[i]);
      rowstride = cols;

      pixels = g_new (guchar, (cols * rows *
                               (((drawable_type == GIMP_INDEXEDA_IMAGE) ||
                                 (drawable_type == GIMP_GRAYA_IMAGE)) ? 2 : 1)));

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, cols, rows), 1.0,
                       format, pixels,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      /* sort out whether we need to do transparency jiggery-pokery */
      if ((drawable_type == GIMP_INDEXEDA_IMAGE) ||
          (drawable_type == GIMP_GRAYA_IMAGE))
        {
          /* Try to find an entry which isn't actually used in the
             image, for a transparency index. */

          transparent =
            find_unused_ia_color (pixels,
                                   cols * rows,
                                   bpp_to_colors (colors_to_bpp (colors)),
                                   &colors);

          special_flatten_indexed_alpha (pixels,
                                         transparent,
                                         cols * rows);
        }
      else
        {
          transparent = -1;
        }

      BitsPerPixel = colors_to_bpp (colors);

      if (BitsPerPixel != liberalBPP)
        {
          /* We were able to re-use an index within the existing bitspace,
             whereas the estimate in the header was pessimistic but still
             needs to be upheld... */
#ifdef GIFDEBUG
          static gboolean onceonly = FALSE;

          if (! onceonly)
            {
              g_warning ("Promised %d bpp, pondered writing chunk with %d bpp!",
                         liberalBPP, BitsPerPixel);
              onceonly = TRUE;
            }
#endif
        }

      useBPP = (BitsPerPixel > liberalBPP) ? BitsPerPixel : liberalBPP;

      if (is_gif89)
        {
          if (i > 0 && ! gsvals.always_use_default_dispose)
            {
              layer_name = gimp_item_get_name (layers[i - 1]);
              Disposal = parse_disposal_tag (layer_name);
              g_free (layer_name);
            }
          else
            {
              Disposal = gsvals.default_dispose;
            }

          layer_name = gimp_item_get_name (layers[i]);
          Delay89 = parse_ms_tag (layer_name);
          g_free (layer_name);

          if (Delay89 < 0 || gsvals.always_use_default_delay)
            Delay89 = (gsvals.default_delay + 5) / 10;
          else
            Delay89 = (Delay89 + 5) / 10;

          /* don't allow a CPU-sucking completely 0-delay looping anim */
          if ((nlayers > 1) && gsvals.loop && (Delay89 == 0))
            {
              static gboolean onceonly = FALSE;

              if (!onceonly)
                {
                  g_message (_("Delay inserted to prevent evil "
                               "CPU-sucking animation."));
                  onceonly = TRUE;
                }
              Delay89 = 1;
            }

          gif_encode_graphic_control_ext (outfile, Disposal, Delay89, nlayers,
                                          cols, rows,
                                          transparent,
                                          useBPP,
                                          get_pixel);
        }

      gif_encode_image_data (outfile, cols, rows,
                             (rows > 4) ? gsvals.interlace : 0,
                             useBPP,
                             get_pixel,
                             offset_x, offset_y);
      gimp_progress_update (1.0);

      g_object_unref (buffer);

      g_free (pixels);
    }

  g_free(layers);

  gif_encode_close (outfile);

  return TRUE;
}

static gboolean
bad_bounds_dialog (void)
{
  GtkWidget *dialog;
  gboolean   crop;

  dialog = gtk_message_dialog_new (NULL, 0,
                                   GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                                   _("The image you are trying to save as a "
                                     "GIF contains layers which extend beyond "
                                     "the actual borders of the image."));

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL,     GTK_RESPONSE_CANCEL,
                          GIMP_STOCK_TOOL_CROP, GTK_RESPONSE_OK,
                          NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            _("The GIF file format does not "
                                              "allow this.  You may choose "
                                              "whether to crop all of the "
                                              "layers to the image borders, "
                                              "or cancel this save."));

  gtk_widget_show (dialog);

  crop = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return crop;
}

static GtkWidget *
file_gif_toggle_button_init (GtkBuilder  *builder,
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

static GtkWidget *
file_gif_spin_button_int_init (GtkBuilder  *builder,
                               const gchar *name,
                               int          initial_value,
                               int         *value_pointer)
{
  GtkWidget     *spin_button = NULL;
  GtkAdjustment *adjustment  = NULL;

  spin_button = GTK_WIDGET (gtk_builder_get_object (builder, name));

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (spin_button));
  gtk_adjustment_set_value (adjustment, initial_value);
  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &gsvals.default_delay);

  return spin_button;
}

static void
file_gif_combo_box_int_update_value (GtkComboBox *combo,
                                     gint        *value)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
    {
      gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)),
                          &iter,
                          DISPOSE_STORE_VALUE_COLUMN, value,
                          -1);
    }
}

static GtkWidget *
file_gif_combo_box_int_init (GtkBuilder  *builder,
                             const gchar *name,
                             int          initial_value,
                             int         *value_pointer,
                             const gchar *first_label,
                             gint         first_value,
                             ...)
{
  GtkWidget    *combo  = NULL;
  GtkListStore *store  = NULL;
  const gchar  *label  = NULL;
  gint          value  = 0;
  GtkTreeIter   iter   = { 0, };
  va_list       values;

  combo = GTK_WIDGET (gtk_builder_get_object (builder, name));
  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)));

  /* Populate */
  va_start (values, first_value);
  for (label = first_label, value = first_value;
       label;
       label = va_arg (values, const gchar *), value = va_arg (values, gint))
    {
      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          DISPOSE_STORE_VALUE_COLUMN, value,
                          DISPOSE_STORE_LABEL_COLUMN, label,
                          -1);
    }
  va_end (values);

  /* Set initial value */
  gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store),
                                 &iter,
                                 NULL,
                                 initial_value);
  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);

  /* Arrange update of value */
  g_signal_connect (combo, "changed",
                    G_CALLBACK (file_gif_combo_box_int_update_value),
                    value_pointer);

  return combo;
}

static gint
save_dialog (gint32 image_ID)
{
  GtkBuilder    *builder = NULL;
  gchar         *ui_file = NULL;
  GError        *error   = NULL;
  GtkWidget     *dialog;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
  GtkWidget     *toggle;
  GtkWidget     *frame;
  GimpParasite  *GIF2_CMNT;
  gint32         nlayers;
  gboolean       animation_supported = FALSE;
  gboolean       run;

  gimp_image_get_layers (image_ID, &nlayers);
  animation_supported = nlayers > 1;

  dialog = gimp_export_dialog_new (_("GIF"), PLUG_IN_BINARY, SAVE_PROC);

  /* GtkBuilder init */
  builder = gtk_builder_new ();
  ui_file = g_build_filename (gimp_data_directory (),
                              "ui/plug-ins/plug-in-file-gif.ui",
                              NULL);
  if (! gtk_builder_add_from_file (builder, ui_file, &error))
    g_printerr (_("Error loading UI file '%s':\n%s"),
                ui_file, error ? error->message : "???");
  g_free (ui_file);

  /* Main vbox */
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      GTK_WIDGET (gtk_builder_get_object (builder, "main-vbox")),
                      TRUE, TRUE, 0);

  /*  regular gif parameter settings  */
  file_gif_toggle_button_init (builder, "interlace",
                               gsvals.interlace, &gsvals.interlace);
  file_gif_toggle_button_init (builder, "save-comment",
                               gsvals.save_comment, &gsvals.save_comment);
  file_gif_toggle_button_init (builder, "as-animation",
                               gsvals.as_animation, &gsvals.as_animation);

  text_view   = GTK_WIDGET (gtk_builder_get_object (builder, "comment"));
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));

  if (globalcomment)
    g_free (globalcomment);

  GIF2_CMNT = gimp_image_get_parasite (image_ID, "gimp-comment");
  if (GIF2_CMNT)
    {
      globalcomment = g_strndup (gimp_parasite_data (GIF2_CMNT),
                                 gimp_parasite_data_size (GIF2_CMNT));
      gimp_parasite_free (GIF2_CMNT);
    }
  else
    {
      globalcomment = gimp_get_default_comment ();
    }

  if (globalcomment)
    gtk_text_buffer_set_text (text_buffer, globalcomment, -1);

  g_signal_connect (text_buffer, "changed",
                    G_CALLBACK (comment_entry_callback),
                    NULL);

  /*  additional animated gif parameter settings  */
  file_gif_toggle_button_init (builder, "loop-forever",
                               gsvals.loop, &gsvals.loop);

  /* default_delay entry field */
  file_gif_spin_button_int_init (builder, "delay-spin",
                                 gsvals.default_delay, &gsvals.default_delay);

  /* Disposal selector */
  file_gif_combo_box_int_init (builder, "dispose-combo",
                               gsvals.default_dispose, &gsvals.default_dispose,
                               _("I don't care"),
                               DISPOSE_UNSPECIFIED,
                               _("Cumulative layers (combine)"),
                               DISPOSE_COMBINE,
                               _("One frame per layer (replace)"),
                               DISPOSE_REPLACE,
                               NULL);

  /* The "Always use default values" toggles */
  file_gif_toggle_button_init (builder, "use-default-delay",
                               gsvals.always_use_default_delay,
                               &gsvals.always_use_default_delay);
  file_gif_toggle_button_init (builder, "use-default-dispose",
                               gsvals.always_use_default_dispose,
                               &gsvals.always_use_default_dispose);

  frame  = GTK_WIDGET (gtk_builder_get_object (builder, "animation-frame"));
  toggle = GTK_WIDGET (gtk_builder_get_object (builder, "as-animation"));
  gtk_widget_set_sensitive (toggle, animation_supported);
  if (! animation_supported)
    gimp_help_set_help_data (toggle,
                             _("You can only export as animation when the "
                               "image has more than one layer. The image "
                               "you are trying to export only has one "
                               "layer."),
                             NULL);

  g_object_bind_property (toggle, "active",
                          frame,  "sensitive",
                          G_BINDING_SYNC_CREATE);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static int
colors_to_bpp (int colors)
{
  gint bpp;

  if (colors <= 2)
    bpp = 1;
  else if (colors <= 4)
    bpp = 2;
  else if (colors <= 8)
    bpp = 3;
  else if (colors <= 16)
    bpp = 4;
  else if (colors <= 32)
    bpp = 5;
  else if (colors <= 64)
    bpp = 6;
  else if (colors <= 128)
    bpp = 7;
  else if (colors <= 256)
    bpp = 8;
  else
    {
      g_warning ("GIF: colors_to_bpp - Eep! too many colors: %d\n", colors);
      return 8;
    }

  return bpp;
}

static int
bpp_to_colors (int bpp)
{
  gint colors;

  if (bpp > 8)
    {
      g_warning ("GIF: bpp_to_colors - Eep! bpp==%d !\n", bpp);
      return 256;
    }

  colors = 1 << bpp;

  return colors;
}



static int
get_pixel (int x,
           int y)
{
  return *(pixels + (rowstride * (long) y) + (long) x);
}


/*****************************************************************************
 *
 * GIFENCODE.C    - GIF Image compression interface
 *
 * GIFEncode( FName, GHeight, GWidth, GInterlace, Background, Transparent,
 *            BitsPerPixel, Red, Green, Blue, get_pixel )
 *
 *****************************************************************************/

static gint  Width, Height;
static gint  curx, cury;
static glong CountDown;
static gint  Pass = 0;

/*
 * Bump the 'curx' and 'cury' to point to the next pixel
 */
static void
bump_pixel (void)
{
  /*
   * Bump the current X position
   */
  curx++;

  /*
   * If we are at the end of a scan line, set curx back to the beginning
   * If we are interlaced, bump the cury to the appropriate spot,
   * otherwise, just increment it.
   */
  if (curx == Width)
    {
      cur_progress++;

      if ((cur_progress % 20) == 0)
        gimp_progress_update ((gdouble) cur_progress / (gdouble) max_progress);

      curx = 0;

      if (! Interlace)
        ++cury;
      else
        {
          switch (Pass)
            {

            case 0:
              cury += 8;
              if (cury >= Height)
                {
                  Pass++;
                  cury = 4;
                }
              break;

            case 1:
              cury += 8;
              if (cury >= Height)
                {
                  Pass++;
                  cury = 2;
                }
              break;

            case 2:
              cury += 4;
              if (cury >= Height)
                {
                  Pass++;
                  cury = 1;
                }
              break;

            case 3:
              cury += 2;
              break;
            }
        }
    }
}

/*
 * Return the next pixel from the image
 */
static int
gif_next_pixel (ifunptr getpixel)
{
  int r;

  if (CountDown == 0)
    return EOF;

  --CountDown;

  r = (*getpixel) (curx, cury);

  bump_pixel ();

  return r;
}

/* public */

static void
gif_encode_header (FILE     *fp,
                   gboolean  gif89,
                   int       GWidth,
                   int       GHeight,
                   int       Background,
                   int       BitsPerPixel,
                   int       Red[],
                   int       Green[],
                   int       Blue[],
                   ifunptr   get_pixel)
{
  int B;
  int RWidth, RHeight;
  int Resolution;
  int ColorMapSize;
  int i;

  ColorMapSize = 1 << BitsPerPixel;

  RWidth = Width = GWidth;
  RHeight = Height = GHeight;

  Resolution = BitsPerPixel;

  /*
   * Calculate number of bits we are expecting
   */
  CountDown = (long) Width *(long) Height;

  /*
   * Indicate which pass we are on (if interlace)
   */
  Pass = 0;

  /*
   * Set up the current x and y position
   */
  curx = cury = 0;

  /*
   * Write the Magic header
   */
  fwrite (gif89 ? "GIF89a" : "GIF87a", 1, 6, fp);

  /*
   * Write out the screen width and height
   */
  put_word (RWidth, fp);
  put_word (RHeight, fp);

  /*
   * Indicate that there is a global color map
   */
  B = 0x80;                        /* Yes, there is a color map */

  /*
   * OR in the resolution
   */
  B |= (Resolution - 1) << 5;

  /*
   * OR in the Bits per Pixel
   */
  B |= (BitsPerPixel - 1);

  /*
   * Write it out
   */
  fputc (B, fp);

  /*
   * Write out the Background color
   */
  fputc (Background, fp);

  /*
   * Byte of 0's (future expansion)
   */
  fputc (0, fp);

  /*
   * Write out the Global Color Map
   */
  for (i = 0; i < ColorMapSize; i++)
    {
      fputc (Red[i], fp);
      fputc (Green[i], fp);
      fputc (Blue[i], fp);
    }
}


static void
gif_encode_graphic_control_ext (FILE    *fp,
                                int      Disposal,
                                int      Delay89,
                                int      NumFramesInImage,
                                int      GWidth,
                                int      GHeight,
                                int      Transparent,
                                int      BitsPerPixel,
                                ifunptr  get_pixel)
{
  Width = GWidth;
  Height = GHeight;

  /*
   * Calculate number of bits we are expecting
   */
  CountDown = (long) Width *(long) Height;

  /*
   * Indicate which pass we are on (if interlace)
   */
  Pass = 0;

  /*
   * Set up the current x and y position
   */
  curx = cury = 0;

  /*
   * Write out extension for transparent color index, if necessary.
   */
  if ( (Transparent >= 0) || (NumFramesInImage > 1) )
    {
      /* Extension Introducer - fixed. */
      fputc ('!', fp);
      /* Graphic Control Label - fixed. */
      fputc (0xf9, fp);
      /* Block Size - fixed. */
      fputc (4, fp);

      /* Packed Fields - XXXdddut (d=disposal, u=userInput, t=transFlag) */
      /*                    s8421                                        */
      fputc ( ((Transparent >= 0) ? 0x01 : 0x00) /* TRANSPARENCY */

              /* DISPOSAL */
              | ((NumFramesInImage > 1) ? (Disposal << 2) : 0x00 ),
              /* 0x03 or 0x01 build frames cumulatively */
              /* 0x02 clears frame before drawing */
              /* 0x00 'don't care' */

              fp);

      fputc (Delay89 & 255, fp);
      fputc ((Delay89 >> 8) & 255, fp);

      fputc (Transparent, fp);
      fputc (0, fp);
    }
}


static void
gif_encode_image_data (FILE    *fp,
                       int      GWidth,
                       int      GHeight,
                       int      GInterlace,
                       int      BitsPerPixel,
                       ifunptr  get_pixel,
                       gint     offset_x,
                       gint     offset_y)
{
  int LeftOfs, TopOfs;
  int InitCodeSize;

  Interlace = GInterlace;

  Width = GWidth;
  Height = GHeight;
  LeftOfs = (int) offset_x;
  TopOfs = (int) offset_y;

  /*
   * Calculate number of bits we are expecting
   */
  CountDown = (long) Width * (long) Height;

  /*
   * Indicate which pass we are on (if interlace)
   */
  Pass = 0;

  /*
   * The initial code size
   */
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;

  /*
   * Set up the current x and y position
   */
  curx = cury = 0;

  /*
   * Write an Image separator
   */
  fputc (',', fp);

  /*
   * Write the Image header
   */

  put_word (LeftOfs, fp);
  put_word (TopOfs, fp);
  put_word (Width, fp);
  put_word (Height, fp);

  /*
   * Write out whether or not the image is interlaced
   */
  if (Interlace)
    fputc (0x40, fp);
  else
    fputc (0x00, fp);

  /*
   * Write out the initial code size
   */
  fputc (InitCodeSize, fp);

  /*
   * Go and actually compress the data
   */
  compress (InitCodeSize + 1, fp, get_pixel);

  /*
   * Write out a Zero-length packet (to end the series)
   */
  fputc (0, fp);

#if 0
  /***************************/
  Interlace = GInterlace;
  Width = GWidth;
  Height = GHeight;
  LeftOfs = TopOfs = 0;

  CountDown = (long) Width *(long) Height;
  Pass = 0;
  /*
   * The initial code size
   */
  if (BitsPerPixel <= 1)
    InitCodeSize = 2;
  else
    InitCodeSize = BitsPerPixel;
  /*
   * Set up the current x and y position
   */
  curx = cury = 0;
#endif
}


static void
gif_encode_close (FILE *fp)
{
  /*
   * Write the GIF file terminator
   */
  fputc (';', fp);

  /*
   * And close the file
   */
  fclose (fp);
}


static void
gif_encode_loop_ext (FILE  *fp,
                     guint  num_loops)
{
  fputc(0x21, fp);
  fputc(0xff, fp);
  fputc(0x0b, fp);
  fputs("NETSCAPE2.0", fp);
  fputc(0x03, fp);
  fputc(0x01, fp);
  put_word(num_loops, fp);
  fputc(0x00, fp);

  /* NOTE: num_loops == 0 means 'loop infinitely' */
}


static void
gif_encode_comment_ext (FILE        *fp,
                        const gchar *comment)
{
  if (!comment || !*comment)
    return;

  if (strlen (comment) > 240)
    {
      g_printerr ("GIF: warning:"
                  "comment too large - comment block not written.\n");
      return;
    }

  fputc (0x21, fp);
  fputc (0xfe, fp);
  fputc (strlen (comment), fp);
  fputs (comment, fp);
  fputc (0x00, fp);
}



/*
 * Write out a word to the GIF file
 */
static void
put_word (int   w,
          FILE *fp)
{
  fputc (w & 0xff, fp);
  fputc ((w / 256) & 0xff, fp);
}


/***************************************************************************
 *
 *  GIFCOMPR.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu)
 *
 ***************************************************************************/

/*
 * General DEFINEs
 */

#define GIF_BITS    12

#define HSIZE  5003                /* 80% occupancy */

/*
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */

static int n_bits;                /* number of bits/code */
static int maxbits = GIF_BITS;        /* user settable max # bits/code */
static gint maxcode;        /* maximum code, given n_bits */
static gint maxmaxcode = (gint) 1 << GIF_BITS;        /* should NEVER generate this code */
#ifdef COMPATIBLE                /* But wrong! */
#define MAXCODE(Mn_bits)        ((gint) 1 << (Mn_bits) - 1)
#else /*COMPATIBLE */
#define MAXCODE(Mn_bits)        (((gint) 1 << (Mn_bits)) - 1)
#endif /*COMPATIBLE */

static glong htab[HSIZE];
static unsigned short codetab[HSIZE];
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

static const gint hsize = HSIZE; /* the original reason for this being
                                    variable was "for dynamic table sizing",
                                    but since it was never actually changed
                                    I made it const   --Adam. */

static gint free_ent = 0;        /* first unused entry */

/*
 * block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

static int offset;
static long int in_count = 1;        /* length of input */
static long int out_count = 0;        /* # of codes output (for debugging) */

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static gint  g_init_bits;
static FILE *g_outfile;

static int ClearCode;
static int EOFCode;


static gulong cur_accum;
static gint   cur_bits;

static gulong masks[] =
{
  0x0000, 0x0001, 0x0003, 0x0007,
  0x000F, 0x001F, 0x003F, 0x007F,
  0x00FF, 0x01FF, 0x03FF, 0x07FF,
  0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF,
  0xFFFF
};


static void
compress (int      init_bits,
          FILE    *outfile,
          ifunptr  ReadValue)
{
  if (FALSE)
    no_compress (init_bits, outfile, ReadValue);
  else if (FALSE)
    rle_compress (init_bits, outfile, ReadValue);
  else
    normal_compress (init_bits, outfile, ReadValue);
}

static void
no_compress (int      init_bits,
             FILE    *outfile,
             ifunptr  ReadValue)
{
  long fcode;
  gint i /* = 0 */ ;
  int c;
  gint ent;
  gint hsize_reg;
  int hshift;


  /*
   * Set up the globals:  g_init_bits - initial number of bits
   *                      g_outfile   - pointer to output file
   */
  g_init_bits = init_bits;
  g_outfile = outfile;

  cur_bits = 0;
  cur_accum = 0;

  /*
   * Set up the necessary values
   */
  offset = 0;
  out_count = 0;
  clear_flg = 0;
  in_count = 1;

  ClearCode = (1 << (init_bits - 1));
  EOFCode = ClearCode + 1;
  free_ent = ClearCode + 2;


  /* Had some problems here... should be okay now.  --Adam */
  n_bits = g_init_bits;
  maxcode = MAXCODE (n_bits);


  char_init ();

  ent = gif_next_pixel (ReadValue);

  hshift = 0;
  for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
    ++hshift;
  hshift = 8 - hshift;                /* set hash code range bound */

  hsize_reg = hsize;
  cl_hash ((glong) hsize_reg);        /* clear hash table */

  output ((gint) ClearCode);


  while ((c = gif_next_pixel (ReadValue)) != EOF)
    {
      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((gint) c << hshift) ^ ent);        /* xor hashing */

      output ((gint) ent);
      ++out_count;
      ent = c;

      if (free_ent < maxmaxcode)
        {
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        cl_block ();
    }

  /*
   * Put out the final code.
   */
  output ((gint) ent);
  ++out_count;
  output ((gint) EOFCode);
}

static void
rle_compress (int      init_bits,
              FILE    *outfile,
              ifunptr  ReadValue)
{
  long fcode;
  gint i /* = 0 */ ;
  int c, last;
  gint ent;
  gint disp;
  gint hsize_reg;
  int hshift;


  /*
   * Set up the globals:  g_init_bits - initial number of bits
   *                      g_outfile   - pointer to output file
   */
  g_init_bits = init_bits;
  g_outfile = outfile;

  cur_bits = 0;
  cur_accum = 0;

  /*
   * Set up the necessary values
   */
  offset = 0;
  out_count = 0;
  clear_flg = 0;
  in_count = 1;

  ClearCode = (1 << (init_bits - 1));
  EOFCode = ClearCode + 1;
  free_ent = ClearCode + 2;


  /* Had some problems here... should be okay now.  --Adam */
  n_bits = g_init_bits;
  maxcode = MAXCODE (n_bits);


  char_init ();

  last = ent = gif_next_pixel (ReadValue);

  hshift = 0;
  for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
    ++hshift;
  hshift = 8 - hshift;                /* set hash code range bound */

  hsize_reg = hsize;
  cl_hash ((glong) hsize_reg);        /* clear hash table */

  output ((gint) ClearCode);



  while ((c = gif_next_pixel (ReadValue)) != EOF)
    {
      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((gint) c << hshift) ^ ent);        /* xor hashing */


      if (last == c) {
        if (HashTabOf (i) == fcode)
          {
            ent = CodeTabOf (i);
            continue;
          }
        else if ((long) HashTabOf (i) < 0)        /* empty slot */
          goto nomatch;
        disp = hsize_reg - i;        /* secondary hash (after G. Knott) */
        if (i == 0)
          disp = 1;
      probe:
        if ((i -= disp) < 0)
          i += hsize_reg;

        if (HashTabOf (i) == fcode)
          {
            ent = CodeTabOf (i);
            continue;
          }
        if ((long) HashTabOf (i) > 0)
          goto probe;
        }
    nomatch:
      output ((gint) ent);
      ++out_count;
      last = ent = c;
      if (free_ent < maxmaxcode)
        {
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        cl_block ();
    }

  /*
   * Put out the final code.
   */
  output ((gint) ent);
  ++out_count;
  output ((gint) EOFCode);
}

static void
normal_compress (int      init_bits,
                 FILE    *outfile,
                 ifunptr  ReadValue)
{
  long fcode;
  gint i /* = 0 */ ;
  int c;
  gint ent;
  gint disp;
  gint hsize_reg;
  int hshift;


  /*
   * Set up the globals:  g_init_bits - initial number of bits
   *                      g_outfile   - pointer to output file
   */
  g_init_bits = init_bits;
  g_outfile = outfile;

  cur_bits = 0;
  cur_accum = 0;

  /*
   * Set up the necessary values
   */
  offset = 0;
  out_count = 0;
  clear_flg = 0;
  in_count = 1;

  ClearCode = (1 << (init_bits - 1));
  EOFCode = ClearCode + 1;
  free_ent = ClearCode + 2;


  /* Had some problems here... should be okay now.  --Adam */
  n_bits = g_init_bits;
  maxcode = MAXCODE (n_bits);


  char_init ();

  ent = gif_next_pixel (ReadValue);

  hshift = 0;
  for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
    ++hshift;
  hshift = 8 - hshift;                /* set hash code range bound */

  hsize_reg = hsize;
  cl_hash ((glong) hsize_reg);        /* clear hash table */

  output ((gint) ClearCode);


  while ((c = gif_next_pixel (ReadValue)) != EOF)
    {
      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((gint) c << hshift) ^ ent);        /* xor hashing */

      if (HashTabOf (i) == fcode)
        {
          ent = CodeTabOf (i);
          continue;
        }
      else if ((long) HashTabOf (i) < 0)        /* empty slot */
        goto nomatch;
      disp = hsize_reg - i;        /* secondary hash (after G. Knott) */
      if (i == 0)
        disp = 1;
    probe:
      if ((i -= disp) < 0)
        i += hsize_reg;

      if (HashTabOf (i) == fcode)
        {
          ent = CodeTabOf (i);
          continue;
        }
      if ((long) HashTabOf (i) > 0)
        goto probe;
    nomatch:
      output ((gint) ent);
      ++out_count;
      ent = c;
      if (free_ent < maxmaxcode)
        {
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        cl_block ();
    }

  /*
   * Put out the final code.
   */
  output ((gint) ent);
  ++out_count;
  output ((gint) EOFCode);
}


/*****************************************************************
 * TAG( output )
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< (long)wordsize - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a GIF_BITS character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */

static void
output (gint code)
{
  cur_accum &= masks[cur_bits];

  if (cur_bits > 0)
    cur_accum |= ((long) code << cur_bits);
  else
    cur_accum = code;

  cur_bits += n_bits;

  while (cur_bits >= 8)
    {
      char_out ((unsigned int) (cur_accum & 0xff));
      cur_accum >>= 8;
      cur_bits -= 8;
    }

  /*
   * If the next entry is going to be too big for the code size,
   * then increase it, if possible.
   */
  if (free_ent > maxcode || clear_flg)
    {
      if (clear_flg)
        {

          maxcode = MAXCODE (n_bits = g_init_bits);
          clear_flg = 0;

        }
      else
        {

          ++n_bits;
          if (n_bits == maxbits)
            maxcode = maxmaxcode;
          else
            maxcode = MAXCODE (n_bits);
        }
    }

  if (code == EOFCode)
    {
      /*
       * At EOF, write the rest of the buffer.
       */
      while (cur_bits > 0)
        {
          char_out ((unsigned int) (cur_accum & 0xff));
          cur_accum >>= 8;
          cur_bits -= 8;
        }

      flush_char ();

      fflush (g_outfile);

      if (ferror (g_outfile))
        g_message (_("Error writing output file."));
    }
}

/*
 * Clear out the hash table
 */
static void
cl_block (void)                        /* table clear for block compress */
{
  cl_hash ((glong) hsize);
  free_ent = ClearCode + 2;
  clear_flg = 1;

  output ((gint) ClearCode);
}

static void
cl_hash (glong hsize)        /* reset code table */
{

  glong *htab_p = htab + hsize;

  long i;
  long m1 = -1;

  i = hsize - 16;
  do
    {                                /* might use Sys V memset(3) here */
      *(htab_p - 16) = m1;
      *(htab_p - 15) = m1;
      *(htab_p - 14) = m1;
      *(htab_p - 13) = m1;
      *(htab_p - 12) = m1;
      *(htab_p - 11) = m1;
      *(htab_p - 10) = m1;
      *(htab_p - 9) = m1;
      *(htab_p - 8) = m1;
      *(htab_p - 7) = m1;
      *(htab_p - 6) = m1;
      *(htab_p - 5) = m1;
      *(htab_p - 4) = m1;
      *(htab_p - 3) = m1;
      *(htab_p - 2) = m1;
      *(htab_p - 1) = m1;
      htab_p -= 16;
    }
  while ((i -= 16) >= 0);

  for (i += 16; i > 0; --i)
    *--htab_p = m1;
}


/******************************************************************************
 *
 * GIF Specific routines
 *
 ******************************************************************************/

/*
 * Number of characters so far in this 'packet'
 */
static int a_count;

/*
 * Set up the 'byte output' routine
 */
static void
char_init (void)
{
  a_count = 0;
}

/*
 * Define the storage for the packet accumulator
 */
static char accum[256];

/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void
char_out (int c)
{
  accum[a_count++] = c;
  if (a_count >= 254)
    flush_char ();
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static void
flush_char (void)
{
  if (a_count > 0)
    {
      fputc (a_count, g_outfile);
      fwrite (accum, 1, a_count, g_outfile);
      a_count = 0;
    }
}


/*  Save interface functions  */

static void
comment_entry_callback (GtkTextBuffer *buffer)
{
  GtkTextIter   start_iter;
  GtkTextIter   end_iter;
  gchar        *text;

  gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
  text = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);

#define MAX_COMMENT 240

  if (strlen (text) > MAX_COMMENT)
    {
      /* translators: the %d is *always* 240 here */
      g_message (_("The default comment is limited to %d characters."),
                 MAX_COMMENT);

      gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, MAX_COMMENT - 1);
      gtk_text_buffer_get_end_iter (buffer, &end_iter);

      /*  this calls us recursivaly, but in the else branch
       */
      gtk_text_buffer_delete (buffer, &start_iter, &end_iter);
    }
  else
    {
      g_free (globalcomment);
      globalcomment = g_strdup (text);
      comment_was_edited = TRUE;
    }

  g_free (text);
}
