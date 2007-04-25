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

/*
 * REVISION HISTORY
 *
 * 2003-06-16
 * 4.01.00 - Attempt to use the palette colour closest to that of the
 *           GIMP's current brush background colour for the GIF file's
 *           background index hint for non-transparency-aware image
 *           viewers.  NOTE that this is merely a hint and may be
 *           ignored by this plugin for various (rare) reasons that
 *           would usually entail writing a somewhat larger image file.
 *           + major version bump to indicate 1.3/1.4 branch.
 *
 * 2002/04/24 - Cameron Gregory, http://www.flamingtext.com/
 *           Added no compress option
 *           Added rlecompress().  Should not be covered by lzw patent,
 *           but this is not legal advice.
 *
 * 99/04/25
 * 3.00.02 - Save the comment back onto the image as a persistent
 *           parasite if the comment was edited.
 *
 * 99/03/30
 * 3.00.01 - Round image timing to nearest 10ms instead of
 *           truncating.  Insert a mandatory 10ms minimum delay
 *           for the frames of looping animated GIFs, to avoid
 *           generating an evil CPU-sucking animation that 'other'
 *           GIF-animators sometimes like to save.
 *
 * 99/03/20
 * 3.00.00 - GIF-loading code moved to separate plugin.
 *
 * 99/02/22
 * 2.01.02 - Don't show a progress bar when loading non-interactively
 *
 * 99/01/23
 * 2.01.01 - Use a text-box to permit multi-line comments.  Don't
 *           try to write comment blocks which are longer than
 *           permitted.
 *
 * 98/10/09
 * 2.01.00 - Added support for persistent GIF Comments through
 *           the GIMP 1.1 GimpParasite mechanism where available.
 *           Did some user-interface tweaks.
 *           Fixed a bug when trying to save a GIF smaller
 *           than five pixels high as interlaced.
 *
 * 98/09/28
 * 2.00.05 - Fixed TigerT's Infinite GIF Bug.  Icky one.
 *
 * 98/09/15
 * 2.00.04 - The facility to specify the background colour of
 *           a transparent/animated GIF for non-transparent
 *           viewers now works very much more consistantly.
 *
 *           The only situations in which it will fail to work
 *           as expected now are those where file size can be reduced
 *           (abeit not by much, as the plugin is sometimes more pessimistic
 *           than it need be) by re-using an existing unused colour
 *           index rather than using another bit per pixel in the
 *           encoded file.  That will never be an issue with an image
 *           which was freshly converted from RGB to INDEXED with the
 *           Quantize option, as that option removes any unused colours
 *           from the image.
 *
 *           Let me know if you find the consistancy/size tradeoff more
 *           annoying than helpful and I can adjust it.  IMHO it is too
 *           arcane a feature to present to any user as a runtime option.
 *
 * 98/05/18
 * 2.00.03 - If we did manage to decode at least one frame of a
 *           gif, be sure to display the resulting image even if
 *           it terminated abruptly.
 *
 * 98/04/28
 * 2.00.02 - Fixed a bug with (ms) tag parsing.
 *
 * 98/03/16
 * 2.00.01 - Fixed a long-standing bug when loading GIFs which layer
 *           opaque frames onto transparent ones.
 *
 * 98/03/15
 * 2.00.00 - No longer beta.  Uses the current GIMP brush background
 *           colour as the transparent-index colour for viewers that
 *           don't know about transparency, instead of magenta.  Note
 *           that this is by no means likely to actually work, but
 *           is more likely to do so if your image has been freshly
 *           to-index'd before saving.
 *
 *           Also added some analysis to gif-reading to prevent the
 *           number of encoded bits being pumped up inadvertantly for
 *           successive load/saves of the same image.  [Adam]
 *
 * 97/12/11
 * 1.99.18 - Bleh.  Disposals should specify how the next frame will
 *           be composed with this frame, NOT how this frame will
 *           be composed with the previous frame.  Fixed.  [Adam]
 *
 * 97/11/30
 * 1.99.17 - No more bogus transparency indices in animated GIFs,
 *           hopefully.  Saved files are better-behaved, sometimes
 *           smaller.  [Adam]
 *
 * 97/09/29
 * 1.99.16 - Added a dialog for the user to choose what to do if
 *           one of the layers of the image extends beyond the image
 *           borders - crop or cancel.  Added code behind it.
 *
 *           Corrected the number of bits for the base image to be
 *           on the generous side.  Hopefully we can no longer generate
 *           GIFs which make XV barf.
 *
 *           Now a lot more careful about whether we choose to encode
 *           as a GIF87a or a GIF89a.  Hopefully does everything by the
 *           book.  It should now be nigh-on impossible to torture the
 *           plugin into generating a GIF which isn't extremely well
 *           behaved with respect to the GIF89a specification.
 *
 *           Fixed(?) a lot of dialog callback details, should now be
 *           happy with window deletion (GTK+970925).  Fixed the
 *           cancellation of a save.  [Adam]
 *
 * 97/09/16
 * 1.99.15 - Hey!  We can now cope with loading images which change
 *           colourmap between frames.  This plugin will never save
 *           such abominations of nature while I still live, though.
 *           There should be no noncorrupt GIF in the universe which
 *           GIMP can't load and play now.  [Adam]
 *
 * 97/09/14
 * 1.99.14 - Added a check for layers whose extents don't lay
 *           within the image boundaries, which would make it a
 *           lot harder to generate badly-behaved GIFs.  Doesn't
 *           do anything about it yet, but it should crop all layers
 *           to the image boundaries.  Also, there's now a (separate)
 *           animation-preview plugin!  [Adam]
 *
 * 97/08/29
 * 1.99.13 - Basic ability to embed GIF comments within saved images.
 *           Fixed a bug with encoding the number of loops in a GIF file -
 *           would have been important, but we're not using that feature
 *           yet anyway.  ;)
 *           Subtly improved dialog layout a little. [Adam]
 *
 * 97/07/25
 * 1.99.12 - Fixed attempts to load GIFs which don't exist.  Made a
 *           related cosmetic adjustment. [Adam]
 *
 * 97/07/10
 * 1.99.11 - Fixed a bug with loading and saving GIFs where the bottom
 *           layer wasn't the same size as the image. [Adam]
 *
 * 97/07/06
 * 1.99.10 - New 'save' dialog, now most of the default behaviour of
 *           animated GIF saving is user-settable (looping, default
 *           time between frames, etc.)
 *           PDB entry for saving is no longer compatible.  Fortunately
 *           I don't think that anyone is using file_gif_save in
 *           scripts.  [Adam]
 *
 * 97/07/05
 * 1.99.9  - More animated GIF work: now loads and saves frame disposal
 *           information.  This is neat and will also allow some delta
 *           stuff in the future.
 *           The disposal-method is kept in the layer name, like the timing
 *           info.
 *           (replace) - this frame replaces whatever else has been shown
 *                       so far.
 *           (combine) - this frame builds apon the previous frame.
 *           If a disposal method is not specified, it is assumed to mean
 *           "don't care."  [Adam]
 *
 * 97/07/04
 * 1.99.8  - Can save per-frame timing information too, now.  The time
 *           for which a frame is visible is specified within the layer name
 *           as i,e. (250ms).  If a frame doesn't have this timing value
 *           it defaults to lasting 100ms. [Adam]
 *
 * 97/07/02
 * 1.99.7  - For animated GIFs, fixed the saving of timing information for
 *           frames which couldn't be made transparent.
 *           Added the loading of timing information into the layer
 *           names.  Adjusted GIMP's GIF magic number very slightly. [Adam]
 *
 * 97/06/30
 * 1.99.6  - Now saves GRAY and GRAYA images, albeit not always
 *           optimally (yet). [Adam]
 *
 * 97/06/25
 * 1.99.5  - Good, the transparancy-on-big-architectures bug is
 *           fixed.  Cleaned up some stuff.
 *           (Adam D. Moss, adam@foxbox.org)
 *
 * 97/06/23
 * 1.99.4  - Trying to fix some endianness/word-size problems with
 *           transparent gif-saving on some architectures... does
 *           this help?  (Adam D. Moss, adam@foxbox.org)
 *
 * 97/05/18
 * 1.99.3  - Fixed the problem with GIFs getting loop extensions even
 *           if they only had one frame (thanks to Zach for noticing -
 *           git!  :) )  (Adam D. Moss, adam@foxbox.org)
 *
 * 97/05/17
 * 1.99.2  - Can now save animated GIFs.  Correctly handles saving of
 *           image offsets.  Uses N*tscape extentions to loop infinitely.
 *           Some notable shortcomings - see TODO list below.
 *           (Adam D. Moss, adam@foxbox.org)
 *
 * 97/05/16
 * 1.99.1  - Implemented image offsets in animated GIF loading.  Requires
 *           a fix to gimp_layer_translate in libgimp/gimplayer.c if used
 *           with GIMP versions <= 0.99.10.  Started work on saving animated
 *           GIFs.  Started TODO list.  (Adam D. Moss, adam@foxbox.org)
 *
 * 97/05/15
 * 1.99.0  - Started revision log.  GIF plugin now loads/saves INDEXED
 *           and INDEXEDA images with correct transparency where possible.
 *           Loads multi-image (animated) GIFs as a framestack implemented
 *           in GIMP layers.  Some bug fixes to original code, some new bugs
 *           cheerfully added.  (Adam D. Moss, adam@foxbox.org)
 *
 * Previous versions - load/save INDEXED images.
 *           (Peter Mattis & Spencer Kimball, gimp@scam.xcf.berkeley.edu)
 */

/*
 * TODO (more *'s means more important!)
 *
 * - PDB stuff for comments
 *
 * - 'requantize' option for INDEXEDA images which really have 256 colours
 *   in them
 *
 * - Be a bit smarter about finding unused/superfluous colour indices for
 *   lossless colour crunching of INDEXEDA images.  (Specifically, look
 *   for multiple indices which correspond to the same physical colour.)
 *
 * - Tidy up parameters for the GIFEncode routines
 *
 * - Remove unused colourmap entries for GRAYSCALE images.
 *
 * - Button to activate the animation preview plugin from the GIF-save
 *   dialog.
 *
 */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define SAVE_PROC      "file-gif-save"
#define PLUG_IN_BINARY "gif-save"


/* Define only one of these to determine which kind of gif's you would like.
 * GIF_UN means use uncompressed gifs.  These will be large, but no
 * patent problems.
 * GIF_RLE uses Run-length-encoding, which should not be covered by the
 * patent, but this is not legal advice.
 */
/* #define GIF_UN */
/* #define GIF_RLE */

/* uncomment the line below for a little debugging info */
/* #define GIFDEBUG yesplease */

/* Does the version of GIMP we're compiling for support
   data attachments to images?  ('Parasites') */
#define FACEHUGGERS aieee
/* PS: I know that technically facehuggers aren't parasites,
   the pupal-forms are.  But facehuggers are ky00te. */

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
} GIFSaveVals;


/* Declare some local functions.
 */
static void   query                    (void);
static void   run                      (const gchar      *name,
                                        gint              nparams,
                                        const GimpParam  *param,
                                        gint             *nreturn_vals,
                                        GimpParam       **return_vals);
static gint   save_image               (const gchar      *filename,
                                        gint32            image_ID,
                                        gint32            drawable_ID,
                                        gint32            orig_image_ID);

static gboolean bounds_check           (gint32            image_ID);
static gboolean bad_bounds_dialog      (void);

static gboolean save_dialog            (gint32            image_ID);
static void     comment_entry_callback (GtkTextBuffer    *buffer);


static gboolean comment_was_edited = FALSE;

static GimpRunMode run_mode;
#ifdef FACEHUGGERS
GimpParasite * comment_parasite = NULL;
#endif

/* For compression code */
static gint Interlace;


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
  FALSE    /* don't always use default_dispose     */
};


MAIN ()

static void
query (void)
{
  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",        "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",           "Image to save" },
    { GIMP_PDB_DRAWABLE, "drawable",        "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",        "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename",    "The name entered" },
    { GIMP_PDB_INT32,    "interlace",       "Try to save as interlaced" },
    { GIMP_PDB_INT32,    "loop",            "(animated gif) loop infinitely" },
    { GIMP_PDB_INT32,    "default-delay",   "(animated gif) Default delay between framese in milliseconds" },
    { GIMP_PDB_INT32,    "default-dispose", "(animated gif) Default disposal type (0=`don't care`, 1=combine, 2=replace)" }
  };

  gimp_install_procedure (SAVE_PROC,
                          "saves files in Compuserve GIF file format",
                          "Save a file in Compuserve GIF format, with "
                          "possible animation, transparency, and comment.  "
                          "To save an animation, operate on a multi-layer "
                          "file.  The plug-in will intrepret <50% alpha as "
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
  gint32            image_ID;
  gint32            drawable_ID;
  gint32            orig_image_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, SAVE_PROC) == 0)
    {
      image_ID    = orig_image_ID = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          export = gimp_export_image (&image_ID, &drawable_ID, "GIF",
                                      (GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                       GIMP_EXPORT_CAN_HANDLE_GRAY |
                                       GIMP_EXPORT_CAN_HANDLE_ALPHA  |
                                       GIMP_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION));
          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }
          break;
        default:
          break;
        }

      if (bounds_check (image_ID))
        /* The image may or may not have had layers out of
           bounds, but the user didn't mind cropping it down. */
        {
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
              if (nparams != 9)
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
                }
              break;

            case GIMP_RUN_WITH_LAST_VALS:
              /*  Possibly retrieve data  */
              gimp_get_data (SAVE_PROC, &gsvals);
              break;

            default:
              break;
            }

          if (status == GIMP_PDB_SUCCESS)
            {
              if (save_image (param[3].data.d_string,
                              image_ID,
                              drawable_ID,
                              orig_image_ID))
                {
                  /*  Store psvals data  */
                  gimp_set_data (SAVE_PROC, &gsvals, sizeof (GIFSaveVals));
                }
              else
                {
                  status = GIMP_PDB_EXECUTION_ERROR;
                }
            }
        }
      else
        /* Some layers were out of bounds and the user wishes
          to abort.  */
        {
          status = GIMP_PDB_CANCEL;
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);
    }

  values[0].data.d_status = status;
}

static gchar * globalcomment = NULL;



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

/*
 * a code_int must be able to hold 2**BITS values of type int, and also -1
 */
typedef int code_int;

#ifdef SIGNED_COMPARE_SLOW
typedef unsigned long int count_int;
typedef unsigned short int count_short;
#else /*SIGNED_COMPARE_SLOW */
typedef long int count_int;
#endif /*SIGNED_COMPARE_SLOW */



static gint find_unused_ia_colour   (const guchar *pixels,
                                     gint          numpixels,
                                     gint          num_indices,
                                     gint         *colors);

static void special_flatten_indexed_alpha (guchar *pixels,
                                           gint   transparent,
                                           gint    numpixels);
static int colors_to_bpp  (int);
static int bpp_to_colors  (int);
static int get_pixel      (int, int);
static int gif_next_pixel (ifunptr);
static void bump_pixel    (void);

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

#ifdef GIF_UN
static void no_compress     (int, FILE *, ifunptr);
#else
#ifdef GIF_RLE
static void rle_compress    (int, FILE *, ifunptr);
#else
static void normal_compress (int, FILE *, ifunptr);
#endif
#endif
static void put_word   (int, FILE *);
static void compress   (int, FILE *, ifunptr);
static void output     (code_int);
static void cl_block   (void);
static void cl_hash    (count_int);
static void write_err  (void);
static void char_init  (void);
static void char_out   (int);
static void flush_char (void);



static gint
find_unused_ia_colour (const guchar *pixels,
                       gint          numpixels,
                       gint          num_indices,
                       gint         *colors)
{
  gboolean ix_used[256];
  gint i;

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
          g_printerr ("GIF: Found unused colour index %d.\n", (int) i);
#endif
          return i;
        }
    }

  /* Couldn't find an unused colour index within the number of
     bits per pixel we wanted.  Will have to increment the number
     of colours in the image and assign a transparent pixel there. */
  if (*colors < 256)
    {
      (*colors)++;

      g_printerr ("GIF: 2nd pass "
                  "- Increasing bounds and using colour index %d.\n",
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
     encoding, if image already has <=255 colours */

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
  gint sum = 0;
  gint offset = 0;
  gint length;

  length = strlen(str);

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


static gboolean
bounds_check (gint32 image_ID)
{
  GimpDrawable *drawable;
  gint32       *layers;
  gint          nlayers;
  gint          i;
  gint          offset_x, offset_y;

  /* get a list of layers for this image_ID */
  layers = gimp_image_get_layers (image_ID, &nlayers);


  /*** Iterate through the layers to make sure they're all ***/
  /*** within the bounds of the image                      ***/

  for (i = 0; i < nlayers; i++)
    {
      drawable = gimp_drawable_get (layers[i]);
      gimp_drawable_offsets (layers[i], &offset_x, &offset_y);

      if ((offset_x < 0)
          || (offset_y < 0)
          || (offset_x+drawable->width > gimp_image_width(image_ID))
          || (offset_y+drawable->height > gimp_image_height(image_ID)))
        {
          g_free (layers);
          gimp_drawable_detach (drawable);

          /* Image has illegal bounds - ask the user what it wants to do */

          /* Do the crop if we can't talk to the user, or if we asked
           * the user and they said yes. */
          if ((run_mode == GIMP_RUN_NONINTERACTIVE) || bad_bounds_dialog ())
            {
              gimp_image_crop (image_ID,
                               gimp_image_width (image_ID),
                               gimp_image_height (image_ID),
                               0, 0);
              return TRUE;
            }
          else
            {
              return FALSE;
            }
        }
      else
        {
          gimp_drawable_detach (drawable);
        }
    }

  g_free (layers);

  return TRUE;
}


static gint
save_image (const gchar *filename,
            gint32       image_ID,
            gint32       drawable_ID,
            gint32       orig_image_ID)
{
  GimpPixelRgn pixel_rgn;
  GimpDrawable *drawable;
  GimpImageType drawable_type;
  FILE *outfile;
  gint Red[MAXCOLORS];
  gint Green[MAXCOLORS];
  gint Blue[MAXCOLORS];
  guchar *cmap;
  guint rows, cols;
  gint BitsPerPixel, liberalBPP = 0, useBPP = 0;
  gint colors;
  gint i;
  gint transparent;
  gint offset_x, offset_y;

  gint32 *layers;
  gint    nlayers;

  gboolean is_gif89 = FALSE;

  gint   Delay89;
  gint   Disposal;
  gchar *layer_name;

  GimpRGB background;
  guchar  bgred, bggreen, bgblue;
  guchar  bgindex = 0;
  guint   best_error = 0xFFFFFFFF;


#ifdef FACEHUGGERS
  /* Save the comment back to the ImageID, if appropriate */
  if (globalcomment != NULL && comment_was_edited)
    {
      comment_parasite = gimp_parasite_new ("gimp-comment",
                                            GIMP_PARASITE_PERSISTENT,
                                            strlen (globalcomment) + 1,
                                            (void*) globalcomment);
      gimp_image_parasite_attach (orig_image_ID, comment_parasite);
      gimp_parasite_free (comment_parasite);
      comment_parasite = NULL;
    }
#endif

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
      break;

    default:
      g_message (_("Cannot save RGB color images. Convert to "
                   "indexed color or grayscale first."));
      return FALSE;
    }


  /* find earliest index in palette which is closest to the background
     colour, and ATTEMPT to use that as the GIF's default background colour. */
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
      g_message (_("Could not open '%s' for writing: %s"),
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
          g_printerr ("GIF: Too many colours?\n");
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
      drawable = gimp_drawable_get (layers[i]);
      gimp_drawable_offsets (layers[i], &offset_x, &offset_y);
      cols = drawable->width;
      rows = drawable->height;
      rowstride = drawable->width;

      gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
                           drawable->width, drawable->height, FALSE, FALSE);

      pixels = g_new (guchar, (drawable->width * drawable->height
                               * (((drawable_type == GIMP_INDEXEDA_IMAGE)
                                   || (drawable_type == GIMP_GRAYA_IMAGE)) ? 2 : 1)));

      gimp_pixel_rgn_get_rect (&pixel_rgn, pixels, 0, 0,
                               drawable->width, drawable->height);


      /* sort out whether we need to do transparency jiggery-pokery */
      if ((drawable_type == GIMP_INDEXEDA_IMAGE)
          || (drawable_type == GIMP_GRAYA_IMAGE))
        {
          /* Try to find an entry which isn't actually used in the
             image, for a transparency index. */

          transparent =
            find_unused_ia_colour (pixels,
                                   drawable->width * drawable->height,
                                   bpp_to_colors (colors_to_bpp (colors)),
                                   &colors);

          special_flatten_indexed_alpha (pixels,
                                         transparent,
                                         drawable->width * drawable->height);
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
              layer_name = gimp_drawable_get_name (layers[i - 1]);
              Disposal = parse_disposal_tag (layer_name);
              g_free (layer_name);
            }
          else
            {
              Disposal = gsvals.default_dispose;
            }

          layer_name = gimp_drawable_get_name (layers[i]);
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

     gimp_drawable_detach (drawable);

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


static gint
save_dialog (gint32 image_ID)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *toggle;
  GtkWidget     *label;
  GtkWidget     *spinbutton;
  GtkObject     *adj;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *align;
  GtkWidget     *combo;
  GtkWidget     *scrolled_window;
#ifdef FACEHUGGERS
  GimpParasite  *GIF2_CMNT;
#endif
  gint32         nlayers;
  gboolean       run;

  gimp_image_get_layers (image_ID, &nlayers);

  dialog = gimp_dialog_new (_("Save as GIF"), PLUG_IN_BINARY,
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

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  /*  regular gif parameter settings  */
  frame = gimp_frame_new (_("GIF Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("I_nterlace"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gsvals.interlace);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gsvals.interlace);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  align = gtk_alignment_new (0.0, 0.0, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);
  gtk_widget_show (align);

  toggle = gtk_check_button_new_with_mnemonic (_("_GIF comment:"));
  gtk_container_add (GTK_CONTAINER (align), toggle);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gsvals.save_comment);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gsvals.save_comment);

  /* the comment text_view in a gtk_scrolled_window */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start_defaults (GTK_BOX (hbox), scrolled_window);
  gtk_widget_show (scrolled_window);

  text_buffer = gtk_text_buffer_new (NULL);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  g_object_unref (text_buffer);

  if (globalcomment)
    g_free (globalcomment);

#ifdef FACEHUGGERS
  GIF2_CMNT = gimp_image_parasite_find (image_ID, "gimp-comment");
  if (GIF2_CMNT)
    globalcomment = g_strndup (gimp_parasite_data (GIF2_CMNT),
                               gimp_parasite_data_size (GIF2_CMNT));
  else
#endif
    globalcomment = gimp_get_default_comment ();

#ifdef FACEHUGGERS
  gimp_parasite_free (GIF2_CMNT);
#endif

  if (globalcomment)
    gtk_text_buffer_set_text (text_buffer, globalcomment, -1);

  g_signal_connect (text_buffer, "changed",
                    G_CALLBACK (comment_entry_callback),
                    NULL);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /*  additional animated gif parameter settings  */
  frame = gimp_frame_new (_("Animated GIF Options"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  toggle = gtk_check_button_new_with_mnemonic (_("_Loop forever"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), gsvals.loop);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gsvals.loop);

  /* default_delay entry field */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("_Delay between frames where unspecified:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adj, gsvals.default_delay,
                                     0, 65000, 10, 100, 0, 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &gsvals.default_delay);

  label = gtk_label_new (_("milliseconds"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  /* Disposal selector */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  label = gtk_label_new_with_mnemonic (_("_Frame disposal where unspecified:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = gimp_int_combo_box_new (_("I don't care"),
                                  DISPOSE_UNSPECIFIED,
                                  _("Cumulative layers (combine)"),
                                  DISPOSE_COMBINE,
                                  _("One frame per layer (replace)"),
                                  DISPOSE_REPLACE,
                                  NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 gsvals.default_dispose);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_int_combo_box_get_active),
                    &gsvals.default_dispose);

  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  /* The "Always use default values" toggles */
  toggle = gtk_check_button_new_with_mnemonic (_("_Use delay entered above for all frames"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                gsvals.always_use_default_delay);
  gtk_widget_show (toggle);

  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gsvals.always_use_default_delay);

  toggle = gtk_check_button_new_with_mnemonic (_("U_se disposal entered above for all frames"));
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                gsvals.always_use_default_dispose);
  gtk_widget_show (toggle);

  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &gsvals.always_use_default_dispose);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox);

  /* If the image has only one layer it can't be animated, so
     desensitize the animation options. */

  if (nlayers == 1)
    gtk_widget_set_sensitive (frame, FALSE);

  gtk_widget_show (frame);
  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}


static int
colors_to_bpp (int colors)
{
  int bpp;

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
      g_warning ("GIF: colors_to_bpp - Eep! too many colours: %d\n", colors);
      return 8;
    }

  return bpp;
}


static int
bpp_to_colors (int bpp)
{
  int colors;

  if (bpp>8)
    {
      g_warning ("GIF: bpp_to_colors - Eep! bpp==%d !\n", bpp);
      return 256;
    }

  colors = 1 << bpp;

  return (colors);
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

      if ((cur_progress % 16) == 0)
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
gif_encode_header (FILE    *fp,
                  gboolean gif89,
                  int      GWidth,
                  int      GHeight,
                  int      Background,
                  int      BitsPerPixel,
                  int      Red[],
                  int      Green[],
                  int      Blue[],
                  ifunptr  get_pixel)
{
  int B;
  int RWidth, RHeight;
  int LeftOfs, TopOfs;
  int Resolution;
  int ColorMapSize;
  int InitCodeSize;
  int i;

  ColorMapSize = 1 << BitsPerPixel;

  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = TopOfs = 0;

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
   * Write the Magic header
   */
  fwrite (gif89 ? "GIF89a" : "GIF87a", 1, 6, fp);

  /*
   * Write out the screen width and height
   */
  put_word (RWidth, fp);
  put_word (RHeight, fp);

  /*
   * Indicate that there is a global colour map
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
   * Write out the Background colour
   */
  fputc (Background, fp);

  /*
   * Byte of 0's (future expansion)
   */
  fputc (0, fp);

  /*
   * Write out the Global Colour Map
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
  int RWidth, RHeight;
  int LeftOfs, TopOfs;
  int Resolution;
  int ColorMapSize;
  int InitCodeSize;

  ColorMapSize = 1 << BitsPerPixel;

  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = TopOfs = 0;

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
   * Write out extension for transparent colour index, if necessary.
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
  int RWidth, RHeight;
  int LeftOfs, TopOfs;
  int Resolution;
  int ColorMapSize;
  int InitCodeSize;

  Interlace = GInterlace;

  ColorMapSize = 1 << BitsPerPixel;

  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = (int) offset_x;
  TopOfs = (int) offset_y;

  Resolution = BitsPerPixel;

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
  ColorMapSize = 1 << BitsPerPixel;
  RWidth = Width = GWidth;
  RHeight = Height = GHeight;
  LeftOfs = TopOfs = 0;
  Resolution = BitsPerPixel;

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
gif_encode_close (FILE    *fp)
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
gif_encode_loop_ext (FILE    *fp,
                     guint    num_loops)
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

#ifdef NO_UCHAR
typedef char char_type;
#else /*NO_UCHAR */
typedef unsigned char char_type;
#endif /*NO_UCHAR */

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
static code_int maxcode;        /* maximum code, given n_bits */
static code_int maxmaxcode = (code_int) 1 << GIF_BITS;        /* should NEVER generate this code */
#ifdef COMPATIBLE                /* But wrong! */
#define MAXCODE(Mn_bits)        ((code_int) 1 << (Mn_bits) - 1)
#else /*COMPATIBLE */
#define MAXCODE(Mn_bits)        (((code_int) 1 << (Mn_bits)) - 1)
#endif /*COMPATIBLE */

static count_int htab[HSIZE];
static unsigned short codetab[HSIZE];
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

static const code_int hsize = HSIZE; /* the original reason for this being
                                        variable was "for dynamic table sizing",
                                        but since it was never actually changed
                                        I made it const   --Adam. */

static code_int free_ent = 0;        /* first unused entry */

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

static int g_init_bits;
static FILE *g_outfile;

static int ClearCode;
static int EOFCode;


static unsigned long cur_accum;
static int cur_bits;

static unsigned long masks[] =
{0x0000, 0x0001, 0x0003, 0x0007,
 0x000F, 0x001F, 0x003F, 0x007F,
 0x00FF, 0x01FF, 0x03FF, 0x07FF,
 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF,
 0xFFFF};


static void
compress (int      init_bits,
          FILE    *outfile,
          ifunptr  ReadValue)
{
#ifdef GIF_UN
        no_compress(init_bits, outfile, ReadValue);
#else
#ifdef GIF_RLE
        rle_compress(init_bits, outfile, ReadValue);
#else
        normal_compress(init_bits, outfile, ReadValue);
#endif
#endif
}

#ifdef GIF_UN
static void
no_compress (int      init_bits,
             FILE    *outfile,
             ifunptr  ReadValue)
{
  register long fcode;
  register code_int i /* = 0 */ ;
  register int c;
  register code_int ent;
  register code_int hsize_reg;
  register int hshift;


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
  cl_hash ((count_int) hsize_reg);        /* clear hash table */

  output ((code_int) ClearCode);


#ifdef SIGNED_COMPARE_SLOW
  while ((c = gif_next_pixel (ReadValue)) != (unsigned) EOF)
    {
#else /*SIGNED_COMPARE_SLOW */
  while ((c = gif_next_pixel (ReadValue)) != EOF)
    {                                /* } */
#endif /*SIGNED_COMPARE_SLOW */

      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((code_int) c << hshift) ^ ent);        /* xor hashing */

      output ((code_int) ent);
      ++out_count;
      ent = c;
#ifdef SIGNED_COMPARE_SLOW
      if ((unsigned) free_ent < (unsigned) maxmaxcode)
        {
#else /*SIGNED_COMPARE_SLOW */
      if (free_ent < maxmaxcode)
        {                        /* } */
#endif /*SIGNED_COMPARE_SLOW */
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        cl_block ();
    }

  /*
   * Put out the final code.
   */
  output ((code_int) ent);
  ++out_count;
  output ((code_int) EOFCode);
}
#else
#ifdef GIF_RLE

static void
rle_compress (int      init_bits,
              FILE    *outfile,
              ifunptr  ReadValue)
{
  register long fcode;
  register code_int i /* = 0 */ ;
  register int c, last;
  register code_int ent;
  register code_int disp;
  register code_int hsize_reg;
  register int hshift;


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
  cl_hash ((count_int) hsize_reg);        /* clear hash table */

  output ((code_int) ClearCode);



#ifdef SIGNED_COMPARE_SLOW
  while ((c = gif_next_pixel (ReadValue)) != (unsigned) EOF)
    {
#else /*SIGNED_COMPARE_SLOW */
  while ((c = gif_next_pixel (ReadValue)) != EOF)
    {                                /* } */
#endif /*SIGNED_COMPARE_SLOW */

      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((code_int) c << hshift) ^ ent);        /* xor hashing */


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
      output ((code_int) ent);
      ++out_count;
      last = ent = c;
#ifdef SIGNED_COMPARE_SLOW
      if ((unsigned) free_ent < (unsigned) maxmaxcode)
        {
#else /*SIGNED_COMPARE_SLOW */
      if (free_ent < maxmaxcode)
        {                        /* } */
#endif /*SIGNED_COMPARE_SLOW */
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        cl_block ();
    }

  /*
   * Put out the final code.
   */
  output ((code_int) ent);
  ++out_count;
  output ((code_int) EOFCode);
}

#else

static void
normal_compress (int      init_bits,
                 FILE    *outfile,
                 ifunptr  ReadValue)
{
  register long fcode;
  register code_int i /* = 0 */ ;
  register int c;
  register code_int ent;
  register code_int disp;
  register code_int hsize_reg;
  register int hshift;


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
  cl_hash ((count_int) hsize_reg);        /* clear hash table */

  output ((code_int) ClearCode);



#ifdef SIGNED_COMPARE_SLOW
  while ((c = gif_next_pixel (ReadValue)) != (unsigned) EOF)
    {
#else /*SIGNED_COMPARE_SLOW */
  while ((c = gif_next_pixel (ReadValue)) != EOF)
    {                                /* } */
#endif /*SIGNED_COMPARE_SLOW */

      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((code_int) c << hshift) ^ ent);        /* xor hashing */

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
      output ((code_int) ent);
      ++out_count;
      ent = c;
#ifdef SIGNED_COMPARE_SLOW
      if ((unsigned) free_ent < (unsigned) maxmaxcode)
        {
#else /*SIGNED_COMPARE_SLOW */
      if (free_ent < maxmaxcode)
        {                        /* } */
#endif /*SIGNED_COMPARE_SLOW */
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        cl_block ();
    }

  /*
   * Put out the final code.
   */
  output ((code_int) ent);
  ++out_count;
  output ((code_int) EOFCode);
}
#endif
#endif



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
output (code_int code)
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
        write_err ();
    }
}

/*
 * Clear out the hash table
 */
static void
cl_block (void)                        /* table clear for block compress */
{
  cl_hash ((count_int) hsize);
  free_ent = ClearCode + 2;
  clear_flg = 1;

  output ((code_int) ClearCode);
}

static void
cl_hash (count_int hsize)        /* reset code table */
{

  register count_int *htab_p = htab + hsize;

  register long i;
  register long m1 = -1;

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

static void
write_err (void)
{
  g_message (_("Error writing output file."));
  return;
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

  if (strlen (text) > 240)
    {
      g_message (_("The default comment is limited to %d characters."), 240);

      gtk_text_buffer_get_iter_at_offset (buffer, &start_iter, 240 - 1);
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
