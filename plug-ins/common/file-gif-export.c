/* GIF exporting file filter for GIMP
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

#include <string.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define EXPORT_PROC    "file-gif-export"
#define PLUG_IN_BINARY "file-gif-export"


/* uncomment the line below for a little debugging info */
/* #define GIFDEBUG yesplease */


enum
{
  DISPOSE_UNSPECIFIED,
  DISPOSE_COMBINE,
  DISPOSE_REPLACE
};


typedef struct _Gif      Gif;
typedef struct _GifClass GifClass;

struct _Gif
{
  GimpPlugIn      parent_instance;
};

struct _GifClass
{
  GimpPlugInClass parent_class;
};


#define GIF_TYPE  (gif_get_type ())
#define GIF(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIF_TYPE, Gif))

GType                    gif_get_type         (void) G_GNUC_CONST;

static GList           * gif_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure   * gif_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray  * gif_export           (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GFile                *file,
                                               GimpExportOptions    *options,
                                               GimpMetadata         *metadata,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static gboolean          export_image         (GFile                *file,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               GimpImage            *orig_image,
                                               GObject              *config,
                                               GError              **error);

static void              export_edit_options  (GimpProcedure        *procedure,
                                               GimpProcedureConfig  *config,
                                               GimpExportOptions    *options,
                                               gpointer              create_data);

static GimpPDBStatusType sanity_check         (GFile                *file,
                                               GimpImage           **image,
                                               GimpRunMode           run_mode,
                                               GError              **error);
static gboolean          bad_bounds_dialog    (void);

static gboolean          save_dialog          (GimpImage            *image,
                                               GimpImage            *orig_image,
                                               GimpProcedure        *procedure,
                                               GObject              *config);


G_DEFINE_TYPE (Gif, gif, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (GIF_TYPE)
DEFINE_STD_SET_I18N


static gint Interlace; /* For compression code */


static void
gif_class_init (GifClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gif_query_procedures;
  plug_in_class->create_procedure = gif_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gif_init (Gif *gif)
{
}

static GList *
gif_query_procedures (GimpPlugIn *plug_in)
{
  return  g_list_append (NULL, g_strdup (EXPORT_PROC));
}

static GimpProcedure *
gif_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, gif_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED*, GRAY*");

      gimp_procedure_set_menu_label (procedure, _("GIF image"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("GIF"));

      gimp_procedure_set_documentation (procedure,
                                        _("exports files in GIF "
                                          "file format"),
                                        /* xgettext: no-c-format */
                                        _("Export a file in GIF "
                                          "format, with possible animation, "
                                          "transparency, and comment. To export "
                                          "an animation, operate on a multi-layer "
                                          "file and give the 'as-animation' "
                                          "parameter as TRUE. The plug-in will "
                                          "interpret <50% alpha as transparent. "
                                          "When run non-interactively, the value "
                                          "for the comment is taken from the "
                                          "'gimp-comment' parasite."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Spencer Kimball, Peter Mattis, "
                                      "Adam Moss, David Koblas",
                                      "Spencer Kimball, Peter Mattis, "
                                      "Adam Moss, David Koblas",
                                      "1995-1997");

      gimp_file_procedure_set_handles_remote (GIMP_FILE_PROCEDURE (procedure),
                                              TRUE);
      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/gif");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "gif");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA,
                                              export_edit_options, NULL, NULL);

      gimp_procedure_add_boolean_argument (procedure, "interlace",
                                           _("_Interlace"),
                                           _("Try to export as interlaced"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "loop",
                                           _("Loop _Forever"),
                                           _("(animated gif) Loop infinitely"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "number-of-repeats",
                                       _("_Number of repeats"),
                                       _("(animated gif) Number of repeats "
                                         "(Ignored if 'loop' is TRUE)"),
                                       0, G_MAXSHORT - 1, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "default-delay",
                                       _("_Delay between frames when unspecified"),
                                       _("(animated gif) Default delay between frames "
                                         "in milliseconds"),
                                       0, G_MAXINT, 100,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "default-dispose",
                                          _("Frame disposal _when unspecified"),
                                          _("(animated gif) Default disposal type"),
                                          gimp_choice_new_with_values ("unspecified", DISPOSE_UNSPECIFIED, _("I don't care"),                  NULL,
                                                                       "combine",     DISPOSE_COMBINE,     _("Cumulative layers (combine)"),   NULL,
                                                                       "replace",     DISPOSE_REPLACE,     _("One frame per layer (replace)"), NULL,
                                                                       NULL),
                                          "unspecified",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "as-animation",
                                           _("_As animation"),
                                           _("Export GIF as animation?"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "force-delay",
                                           _("_Use delay entered above for all frames"),
                                           _("(animated gif) Use specified delay for all frames"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "force-dispose",
                                           _("Use dis_posal entered above "
                                             "for all frames"),
                                           _("(animated gif) Use specified disposal for all frames"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "save-comment",
                                               _("Sa_ve comment"),
                                               _("Save the image comment in the GIF file"),
                                               gimp_export_comment (),
                                               G_PARAM_READWRITE);

      gimp_procedure_add_string_aux_argument (procedure, "gimp-comment",
                                              _("Commen_t"),
                                              _("Image comment"),
                                              gimp_get_default_comment (),
                                              G_PARAM_READWRITE);

      gimp_procedure_set_argument_sync (procedure, "gimp-comment",
                                        GIMP_ARGUMENT_SYNC_PARASITE);
    }

  return procedure;
}

static GimpValueArray *
gif_export (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GFile                *file,
            GimpExportOptions    *options,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpPDBStatusType  status          = GIMP_PDB_SUCCESS;
  GimpExportReturn   export          = GIMP_EXPORT_IGNORE;
  GimpImage         *orig_image;
  GimpImage         *sanitized_image = NULL;
  GimpParasite      *parasite        = NULL;
  GError            *error           = NULL;

  gegl_init (NULL, NULL);

  orig_image = image;

  if (run_mode == GIMP_RUN_INTERACTIVE ||
      run_mode == GIMP_RUN_WITH_LAST_VALS)
    gimp_ui_init (PLUG_IN_BINARY);

  status = sanity_check (file, &image, run_mode, &error);

  /* Get the export options */
  if (status == GIMP_PDB_SUCCESS)
    {
      /* If the sanity check succeeded, the image_ID will point to a
       * duplicate image to delete later.
       */
      sanitized_image = image;

      /* If imported as an animation, set the animation configurations
       * when overwriting the file */
      parasite = gimp_image_get_parasite (image, "gif/animated");
      if (parasite)
        {
          gint     num_loops;
          gchar   *parasite_data;
          guint32  parasite_size;

          parasite_data = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
          parasite_data = g_strndup (parasite_data, parasite_size);

          if (sscanf (parasite_data, "%i", &num_loops) == 1)
            {
              gboolean loop = (num_loops == 0);

              g_object_set (config,
                            "as-animation",      TRUE,
                            "loop",              loop,
                            "number-of-repeats", num_loops,
                            NULL);
            }

          gimp_image_detach_parasite (image, "gif/animated");
          g_free (parasite);
          g_free (parasite_data);
        }

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          if (! save_dialog (image, orig_image, procedure, G_OBJECT (config)))
            {
              gimp_image_delete (sanitized_image);

              return gimp_procedure_new_return_values (procedure,
                                                       GIMP_PDB_CANCEL,
                                                       NULL);
            }
        }
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      GList                  *drawables;

      export = gimp_export_options_get_image (options, &image);
      drawables = gimp_image_list_layers (image);

      if (! export_image (file, image, drawables->data, orig_image,
                          G_OBJECT (config), &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }

      gimp_image_delete (sanitized_image);
      g_list_free (drawables);
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);


  return gimp_procedure_new_return_values (procedure, status, error);
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
typedef gint (* ifunptr) (gint x,
                          gint y);


static gint find_unused_ia_color           (const guchar  *pixels,
                                            gint           numpixels,
                                            gint           num_indices,
                                            gint          *colors);

static void special_flatten_indexed_alpha  (guchar        *pixels,
                                            gint           transparent,
                                            gint           numpixels);

static gint colors_to_bpp                  (gint           colors);
static gint bpp_to_colors                  (gint           bpp);
static gint get_pixel                      (gint           x,
                                            gint           y);
static gint gif_next_pixel                 (ifunptr        getpixel);
static void bump_pixel                     (void);

static gboolean gif_encode_header              (GOutputStream  *output,
                                                gboolean        gif89,
                                                gint            width,
                                                gint            height,
                                                gint            background,
                                                gint            bpp,
                                                gint           *red,
                                                gint           *green,
                                                gint           *blue,
                                                ifunptr         get_pixel,
                                                GError        **error);
static gboolean gif_encode_graphic_control_ext (GOutputStream  *output,
                                                gint            disposal,
                                                gint            delay89,
                                                gint            n_frames,
                                                gint            width,
                                                gint            height,
                                                gint            transparent,
                                                gint            bpp,
                                                ifunptr         get_pixel,
                                                GError        **error);
static gboolean gif_encode_image_data          (GOutputStream  *output,
                                                gint            width,
                                                gint            height,
                                                gint            interlace,
                                                gint            bpp,
                                                ifunptr         get_pixel,
                                                gint            offset_x,
                                                gint            offset_y,
                                                GError        **error);
static gboolean gif_encode_close               (GOutputStream  *output,
                                                GError        **error);
static gboolean gif_encode_loop_ext            (GOutputStream  *output,
                                                guint           n_loops,
                                                GError        **error);
static gboolean gif_encode_comment_ext         (GOutputStream  *output,
                                                const gchar    *comment,
                                                GError        **error);

static gint     rowstride;
static guchar  *pixels;
static gint     cur_progress;
static gint     max_progress;

static gboolean compress        (GOutputStream *output,
                                 gint           init_bits,
                                 ifunptr        ReadValue,
                                 GError        **error);
static gboolean no_compress     (GOutputStream *output,
                                 gint           init_bits,
                                 ifunptr        ReadValue,
                                 GError        **error);
static gboolean rle_compress    (GOutputStream *output,
                                 gint           init_bits,
                                 ifunptr        ReadValue,
                                 GError        **error);
static gboolean normal_compress (GOutputStream *output,
                                 gint           init_bits,
                                 ifunptr        ReadValue,
                                 GError        **error);

static gboolean put_byte        (GOutputStream  *output,
                                 guchar          b,
                                 GError        **error);
static gboolean put_word        (GOutputStream  *output,
                                 gint            w,
                                 GError        **error);
static gboolean put_string      (GOutputStream  *output,
                                 const gchar    *s,
                                 GError        **error);
static gboolean output_code     (GOutputStream  *output,
                                 gint            code,
                                 GError        **error);
static gboolean cl_block        (GOutputStream  *output,
                                 GError        **error);
static void     cl_hash         (glong           hsize);

static void     char_init       (void);
static gboolean char_out        (GOutputStream  *output,
                                 gint            c,
                                 GError        **error);
static gboolean char_flush      (GOutputStream  *output,
                                 GError        **error);


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

  /* Couldn't find an unused color index within the number of bits per
   * pixel we wanted.  Will have to increment the number of colors in
   * the image and assign a transparent pixel there.
   */
  if (*colors < 256)
    {
      (*colors)++;

      g_printerr ("GIF: 2nd pass "
                  "- Increasing bounds and using color index %d.\n",
                  *colors - 1);
      return ((*colors) - 1);
    }

  g_message (_("Couldn't simply reduce colors further. Exporting as opaque."));

  return -1;
}


static void
special_flatten_indexed_alpha (guchar *pixels,
                               gint    transparent,
                               gint    numpixels)
{
  gint i;

  /* Each transparent pixel in the image is mapped to a uniform value
   * for encoding, if image already has <=255 colors
   */

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

  if ((g_ascii_toupper (str[offset])     != 'M') ||
      (g_ascii_toupper (str[offset + 1]) != 'S'))
    return -4;

  return sum;
}


static gint
parse_disposal_tag (const gchar *str,
                    gint         default_dispose)
{
  gint offset = 0;
  gint length;

  length = strlen (str);

  while ((offset + 9) <= length)
    {
      if (strncmp (&str[offset], "(combine)", 9) == 0)
        return 0x01;

      if (strncmp (&str[offset], "(replace)", 9) == 0)
        return 0x02 ;

      offset++;
    }

  return default_dispose;
}


static GimpPDBStatusType
sanity_check (GFile        *file,
              GimpImage   **image,
              GimpRunMode   run_mode,
              GError      **error)
{
  GList *layers;
  GList *list;
  gint   image_width;
  gint   image_height;

  image_width  = gimp_image_get_width (*image);
  image_height = gimp_image_get_height (*image);

  if (image_width > G_MAXUSHORT || image_height > G_MAXUSHORT)
    {
      g_set_error (error, 0, 0,
                   _("Unable to export '%s'.  "
                     "The GIF file format does not support images that are "
                     "more than %d pixels wide or tall."),
                   gimp_file_get_utf8_name (file), G_MAXUSHORT);

      return GIMP_PDB_EXECUTION_ERROR;
    }

  *image = gimp_image_duplicate (*image);

  /* Convert image to 8 bit precision if necessary */
  if (gimp_image_get_precision (*image) != GIMP_PRECISION_U8_NON_LINEAR &&
      gimp_image_get_precision (*image) != GIMP_PRECISION_U8_LINEAR     &&
      gimp_image_get_precision (*image) != GIMP_PRECISION_U8_PERCEPTUAL)
    {
      gboolean converted = FALSE;

      converted = gimp_image_convert_precision (*image,
                                                GIMP_PRECISION_U8_NON_LINEAR);

      if (! converted)
        {
          g_set_error (error, 0, 0,
                       _("Unable to export '%s'.  "
                         "Please convert your image to 8 bit integer "
                         "precision, as the GIF file format does not "
                         "support higher precisions."),
                       gimp_file_get_utf8_name (file));

          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  /*** Iterate through the layers to make sure they're all ***/
  /*** within the bounds of the image                      ***/
  layers = gimp_image_list_layers (*image);

  for (list = layers; list; list = g_list_next (list))
    {
      GimpDrawable *drawable = list->data;
      gint          offset_x;
      gint          offset_y;

      gimp_drawable_get_offsets (drawable, &offset_x, &offset_y);

      if (offset_x < 0 ||
          offset_y < 0 ||
          offset_x + gimp_drawable_get_width (drawable) > image_width ||
          offset_y + gimp_drawable_get_height (drawable) > image_height)
        {
          g_list_free (layers);

          /* Image has illegal bounds - ask the user what it wants to do */

          /* Do the crop if we can't talk to the user, or if we asked
           * the user and they said yes.
           */
          if ((run_mode == GIMP_RUN_NONINTERACTIVE) || bad_bounds_dialog ())
            {
              gimp_image_crop (*image, image_width, image_height, 0, 0);
              return GIMP_PDB_SUCCESS;
            }
          else
            {
              gimp_image_delete (*image);
              return GIMP_PDB_CANCEL;
            }
        }
    }

  g_list_free (layers);

  return GIMP_PDB_SUCCESS;
}

static gboolean
export_image (GFile         *file,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GimpImage     *orig_image,
              GObject       *config,
              GError       **error)
{
  GeglBuffer    *buffer;
  GimpImageType  drawable_type;
  const Babl    *format = NULL;
  GOutputStream *output;
  gint           Red[MAXCOLORS];
  gint           Green[MAXCOLORS];
  gint           Blue[MAXCOLORS];
  guchar        *cmap;
  guint          rows, cols;
  gint           BitsPerPixel;
  gint           liberalBPP = 0;
  gint           useBPP     = 0;
  gint           colors;
  gint           i;
  gint           transparent;
  gint           offset_x, offset_y;

  GList         *layers;
  GList         *list;
  gint           nlayers;

  gboolean       is_gif89 = FALSE;

  gint           Delay89;
  gint           Disposal;
  gchar         *layer_name;

  guchar         bgred   = 255;
  guchar         bggreen = 255;
  guchar         bgblue  = 255;
  guchar         bgindex = 0;
  guint          best_error = 0xFFFFFFFF;

  gboolean       config_interlace;
  gboolean       config_loop;
  gint           config_number_of_repeats;
  gint           config_default_delay;
  gint           config_default_dispose;
  gboolean       config_use_default_delay;
  gboolean       config_use_default_dispose;
  gboolean       config_as_animation;
  gboolean       config_save_comment;
  gchar         *config_comment;

  g_object_get (config,
                "interlace",         &config_interlace,
                "loop",              &config_loop,
                "number-of-repeats", &config_number_of_repeats,
                "default-delay",     &config_default_delay,
                "force-delay",       &config_use_default_delay,
                "force-dispose",     &config_use_default_dispose,
                "as-animation",      &config_as_animation,
                "save-comment",      &config_save_comment,
                "gimp-comment",      &config_comment,
                NULL);
  config_default_dispose =
    gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                         "default-dispose");

  /* The GIF spec says 7bit ASCII for the comment block. */
  if (config_save_comment && config_comment && strlen (config_comment))
    {
      const gchar *c = config_comment;
      gint         len;

      for (len = strlen (c); len; c++, len--)
        {
          if ((guchar) *c > 127)
            {
              g_message (_("The GIF format only supports comments in "
                           "7bit ASCII encoding. No comment is saved."));

              g_free (config_comment);
              config_comment = NULL;

              config_save_comment = FALSE;

              break;
            }
        }
    }
  else
    {
      config_save_comment = FALSE;
    }

  /* get a list of layers for this image */
  layers = gimp_image_list_layers (image);
  nlayers = g_list_length (layers);

  drawable_type = gimp_drawable_type (layers->data);

  /* If the image has multiple layers (i.e. will be animated), a
   * comment, or transparency, then it must be encoded as a GIF89a
   * file, not a vanilla GIF87a.
   */
  if (nlayers > 1)
    {
      is_gif89 = TRUE;

      /* Layers can be with or without alpha channel. Make sure we set
       * alpha if there is at least one layer with alpha channel. */
      if (drawable_type == GIMP_GRAY_IMAGE ||
          drawable_type == GIMP_INDEXED_IMAGE)
        {
          for (list = layers; list; list = g_list_next (list))
            {
              GimpImageType dr_type = gimp_drawable_type (list->data);

              if (dr_type == GIMP_GRAYA_IMAGE ||
                  dr_type == GIMP_INDEXEDA_IMAGE)
                {
                  drawable_type = dr_type;
                  break;
                }
            }
        }
    }

  if (config_save_comment)
    is_gif89 = TRUE;

  switch (drawable_type)
    {
    case GIMP_INDEXEDA_IMAGE:
      is_gif89 = TRUE;
    case GIMP_INDEXED_IMAGE:
        {
          GeglColor *background;
          guchar     bg[3];

          cmap = gimp_image_get_colormap (image, NULL, &colors);

          background = gimp_context_get_background ();
          gegl_color_get_pixel (background, babl_format_with_space ("R'G'B' u8", NULL), bg);
          g_object_unref (background);
          bgred   = bg[0];
          bggreen = bg[1];
          bgblue  = bg[2];

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
      g_message (_("Cannot export RGB color images. Convert to "
                   "indexed color or grayscale first."));
      return FALSE;
    }


  /* find earliest index in palette which is closest to the background
   * color, and ATTEMPT to use that as the GIF's default background
   * color.
   */
  for (i = 255; i >= 0; --i)
    {
      guint local_error = 0;

      local_error += (Red[i]   - bgred)   * (Red[i]   - bgred);
      local_error += (Green[i] - bggreen) * (Green[i] - bggreen);
      local_error += (Blue[i]  - bgblue)  * (Blue[i]  - bgblue);

      if (local_error <= best_error)
        {
          bgindex = i;
          best_error = local_error;
        }
    }


  /* init the progress meter */
  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));


  /* open the destination file for writing */
  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (output)
    {
      GDataOutputStream *data_output;

      data_output = g_data_output_stream_new (output);
      g_object_unref (output);

      g_data_output_stream_set_byte_order (data_output,
                                           G_DATA_STREAM_BYTE_ORDER_LITTLE_ENDIAN);

      output = G_OUTPUT_STREAM (data_output);
    }
  else
    {
      return FALSE;
    }


  /* write the GIFheader */

  if (colors < 256)
    {
      /* we keep track of how many bits we promised to have in
       * liberalBPP, so that we don't accidentally come under this
       * when doing clever transparency stuff where we can re-use
       * wasted indices.
       */
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

  cols = gimp_image_get_width (image);
  rows = gimp_image_get_height (image);
  Interlace = config_interlace;
  if (! gif_encode_header (output, is_gif89, cols, rows, bgindex,
                           BitsPerPixel, Red, Green, Blue, get_pixel,
                           error))
    return FALSE;


  /* If the image has multiple layers it'll be made into an animated
   * GIF, so write out the looping extension
   */
  if (nlayers > 1)
    {
      /* If 'Forever' toggle was checked, override number_of_repeats */
      if (config_loop)
        config_number_of_repeats = 0;

      if (config_loop || config_number_of_repeats > 0)
        if (! gif_encode_loop_ext (output, config_number_of_repeats, error))
          return FALSE;
    }

  /* Write comment extension - mustn't be written before the looping ext. */
  if (config_save_comment && config_comment && strlen (config_comment))
    {
      if (! gif_encode_comment_ext (output, config_comment, error))
        return FALSE;
    }


  /*** Now for each layer in the image, save an image in a compound GIF ***/
  /************************************************************************/

  cur_progress = 0;
  max_progress = nlayers * rows;

  layers = g_list_reverse (layers);

  for (list = layers, i = nlayers - 1;
       list && i >= 0;
       list = g_list_next (list), i--, cur_progress = (nlayers - i) * rows)
    {
      GimpDrawable *drawable = list->data;

      drawable_type = gimp_drawable_type (drawable);
      if (drawable_type == GIMP_GRAYA_IMAGE)
        {
          format = babl_format ("Y'A u8");
        }
      else if (drawable_type == GIMP_GRAY_IMAGE)
        {
          format = babl_format ("Y' u8");
        }

      buffer = gimp_drawable_get_buffer (drawable);
      gimp_drawable_get_offsets (drawable, &offset_x, &offset_y);
      cols = gimp_drawable_get_width (drawable);
      rows = gimp_drawable_get_height (drawable);
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
           * image, for a transparency index.
           */

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
          /* We were able to re-use an index within the existing
           * bitspace, whereas the estimate in the header was
           * pessimistic but still needs to be upheld...
           */
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
          if (i > 0 && ! config_use_default_dispose)
            {
              layer_name = gimp_item_get_name (list->next->data);
              Disposal = parse_disposal_tag (layer_name,
                                             config_default_dispose);
              g_free (layer_name);
            }
          else
            {
              Disposal = config_default_dispose;
            }

          layer_name = gimp_item_get_name (GIMP_ITEM (drawable));
          Delay89 = parse_ms_tag (layer_name);
          g_free (layer_name);

          if (Delay89 < 0 || config_use_default_delay)
            Delay89 = (config_default_delay + 5) / 10;
          else
            Delay89 = (Delay89 + 5) / 10;

          /* don't allow a CPU-sucking completely 0-delay looping anim */
          if ((nlayers > 1) && config_loop && (Delay89 == 0))
            {
              static gboolean onceonly = FALSE;

              if (! onceonly)
                {
                  g_message (_("Delay inserted to prevent evil "
                               "CPU-sucking animation."));
                  onceonly = TRUE;
                }

              Delay89 = 1;
            }

          if (! gif_encode_graphic_control_ext (output,
                                                Disposal, Delay89, nlayers,
                                                cols, rows,
                                                transparent,
                                                useBPP,
                                                get_pixel,
                                                error))
            return FALSE;
        }

      if (! gif_encode_image_data (output, cols, rows,
                                   (rows > 4) ? config_interlace : 0,
                                   useBPP,
                                   get_pixel,
                                   offset_x, offset_y,
                                   error))
        return FALSE;

      gimp_progress_update (1.0);

      g_object_unref (buffer);

      g_free (pixels);
    }

  g_list_free (layers);

  if (! gif_encode_close (output, error))
    return FALSE;

  return TRUE;
}

static void
export_edit_options (GimpProcedure        *procedure,
                     GimpProcedureConfig  *config,
                     GimpExportOptions    *options,
                     gpointer              create_data)
{
  GimpExportCapabilities capabilities;
  gboolean               as_animation;

  g_object_get (G_OBJECT (config),
                "as-animation", &as_animation,
                NULL);

  capabilities = (GIMP_EXPORT_CAN_HANDLE_INDEXED |
                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                  GIMP_EXPORT_CAN_HANDLE_ALPHA);

  if (as_animation)
    capabilities |= GIMP_EXPORT_CAN_HANDLE_LAYERS;

  g_object_set (G_OBJECT (options),
                "capabilities", capabilities,
                NULL);
}

static gboolean
bad_bounds_dialog (void)
{
  GtkWidget *dialog;
  gboolean   crop;

  dialog = gtk_message_dialog_new (NULL, 0,
                                   GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE,
                                   _("The image you are trying to export as a "
                                     "GIF contains layers which extend beyond "
                                     "the actual borders of the image."));

  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                          _("Cr_op"),   GTK_RESPONSE_OK,
                          NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            _("The GIF file format does not "
                                              "allow this.  You may choose "
                                              "whether to crop all of the "
                                              "layers to the image borders, "
                                              "or cancel this export."));

  gtk_widget_show (dialog);

  crop = (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return crop;
}

static gboolean
save_dialog (GimpImage     *image,
             GimpImage     *orig_image,
             GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget     *dialog;
  GtkWidget     *vbox;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
  gint32         n_layers;
  gboolean       animation_supported;
  gboolean       run;

  g_free (gimp_image_get_layers (image, &n_layers));

  animation_supported = n_layers > 1;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  /* Warn user that image will be converted to 8 bits if not already */
  if (gimp_image_get_precision (orig_image) != GIMP_PRECISION_U8_NON_LINEAR &&
      gimp_image_get_precision (orig_image) != GIMP_PRECISION_U8_LINEAR     &&
      gimp_image_get_precision (orig_image) != GIMP_PRECISION_U8_PERCEPTUAL)
    {
      GtkWidget *hint;

      /* Used to create vbox to store hintbox in */
      gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                       "spacer-8-bit", " ",
                                       FALSE, FALSE);

      vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                             "8-bit-warning-vbox", "spacer-8-bit",
                                             NULL);

      hint = g_object_new (GIMP_TYPE_HINT_BOX,
                           "icon-name", GIMP_ICON_DIALOG_WARNING,
                           "hint",      _("The GIF format only supports 8 bit "
                                          "integer precision.\n"
                                          "Your image will be converted on "
                                          "export, which may change the pixel "
                                          "values."),
                           NULL);
      gtk_widget_set_visible (hint, TRUE);

      gtk_box_pack_start (GTK_BOX (vbox), hint, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (vbox), hint, 0);

      gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                  "8-bit-warning-vbox", NULL);
    }

#define MAX_COMMENT 240

  text_view = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                                "gimp-comment",
                                                GTK_TYPE_TEXT_VIEW);
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  g_object_set_data (G_OBJECT (text_buffer), "max-len",
                     GINT_TO_POINTER (MAX_COMMENT));

  gimp_procedure_dialog_fill_scrolled_window (GIMP_PROCEDURE_DIALOG (dialog),
                                              "comment-scrolled",
                                              "gimp-comment");

  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "comment-frame", "save-comment", FALSE,
                                    "comment-scrolled");

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "interlace", "comment-frame", NULL);

  if (animation_supported)
    {
      GtkWidget    *grid;
      GtkWidget    *widget;

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 1);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_widget_show (grid);

      widget = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "number-of-repeats",
                                                 GIMP_TYPE_LABEL_SPIN);
      gtk_grid_attach (GTK_GRID (grid), widget, 0, 0, 1, 1);
      gtk_widget_set_visible (widget, TRUE);

      widget = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "loop", GTK_TYPE_CHECK_BUTTON);
      gtk_grid_attach (GTK_GRID (grid), widget, 1, 0, 1, 1);

      gimp_procedure_dialog_set_sensitive (GIMP_PROCEDURE_DIALOG (dialog),
                                           "number-of-repeats", TRUE,
                                           config, "loop",
                                           TRUE);

      widget = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "default-delay",
                                                 GIMP_TYPE_LABEL_SPIN);
      gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 1, 1);
      gtk_widget_set_visible (widget, TRUE);

      widget = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                                "milliseconds",
                                                _("milliseconds"),
                                                FALSE, FALSE);
      gtk_label_set_xalign (GTK_LABEL (widget), 0.1);
      gtk_grid_attach (GTK_GRID (grid), widget, 1, 1, 1, 1);
      gtk_widget_set_visible (widget, TRUE);

      widget = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "default-dispose",
                                                 G_TYPE_NONE);
      gtk_grid_attach (GTK_GRID (grid), widget, 0, 2, 2, 1);
      gtk_widget_set_visible (widget, TRUE);

      vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                             "animation-vbox", "force-delay",
                                             "force-dispose", NULL);

      gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (vbox), grid, 0);

      gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                        "ani-frame", "as-animation", FALSE,
                                        "animation-vbox");

      gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                  "ani-frame", NULL);
    }
  else
    {
      GtkWidget *hint;

      /* Used to create vbox to store hintbox in */
      gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                       "spacer", " ",
                                       FALSE, FALSE);
      gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                       "no-ani-title", _("Animated GIF"),
                                       FALSE, FALSE);

      vbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                             "no-animation-vbox", "spacer",
                                             NULL);

      hint = gimp_hint_box_new (_("You can only export as animation when the "
                                  "image has more than one layer.\n"
                                  "The image you are trying to export only "
                                  "has one layer."));
      gtk_box_pack_start (GTK_BOX (vbox), hint, FALSE, FALSE, 0);
      gtk_widget_set_margin_bottom (vbox, 12);
      gtk_widget_show (hint);
      gtk_box_reorder_child (GTK_BOX (vbox), hint, 0);

      gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                        "no-ani-frame", "no-ani-title", FALSE,
                                        "no-animation-vbox");

      gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                                  "no-ani-frame", NULL);
    }

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static int
colors_to_bpp (gint colors)
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
bpp_to_colors (gint bpp)
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



static gint
get_pixel (gint x,
           gint y)
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
static gint
gif_next_pixel (ifunptr getpixel)
{
  gint r;

  if (CountDown == 0)
    return EOF;

  --CountDown;

  r = (*getpixel) (curx, cury);

  bump_pixel ();

  return r;
}

/* public */

static gboolean
gif_encode_header (GOutputStream *output,
                   gboolean       gif89,
                   gint           GWidth,
                   gint           GHeight,
                   gint           Background,
                   gint           BitsPerPixel,
                   gint           Red[],
                   gint           Green[],
                   gint           Blue[],
                   ifunptr        get_pixel,
                   GError       **error)
{
  gint B;
  gint RWidth, RHeight;
  gint Resolution;
  gint ColorMapSize;
  gint i;

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
  if (! put_string (output, gif89 ? "GIF89a" : "GIF87a", error))
    return FALSE;

  /*
   * Write out the screen width and height
   */
  if (! put_word (output, RWidth,  error) ||
      ! put_word (output, RHeight, error))
    return FALSE;

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
  if (! put_byte (output, B, error))
    return FALSE;

  /*
   * Write out the Background color
   */
  if (! put_byte (output, Background, error))
    return FALSE;

  /*
   * Byte of 0's (future expansion)
   */
  if (! put_byte (output, 0, error))
    return FALSE;

  /*
   * Write out the Global Color Map
   */
  for (i = 0; i < ColorMapSize; i++)
    {
      if (! put_byte (output, Red[i],   error) ||
          ! put_byte (output, Green[i], error) ||
          ! put_byte (output, Blue[i],  error))
        return FALSE;
    }

  return TRUE;
}


static gboolean
gif_encode_graphic_control_ext (GOutputStream *output,
                                int            Disposal,
                                int            Delay89,
                                int            NumFramesInImage,
                                int            GWidth,
                                int            GHeight,
                                int            Transparent,
                                int            BitsPerPixel,
                                ifunptr        get_pixel,
                                GError       **error)
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
      if (! put_byte (output, '!', error))
        return FALSE;

      /* Graphic Control Label - fixed. */
      if (! put_byte (output, 0xf9, error))
        return FALSE;

      /* Block Size - fixed. */
      if (! put_byte (output, 4, error))
        return FALSE;

      /* Packed Fields - XXXdddut (d=disposal, u=userInput, t=transFlag) */
      /*                    s8421                                        */
      if (! put_byte (output,
                      ((Transparent >= 0) ? 0x01 : 0x00) /* TRANSPARENCY */

                      /* DISPOSAL */
                      | ((NumFramesInImage > 1) ? (Disposal << 2) : 0x00 ),
                      /* 0x03 or 0x01 build frames cumulatively */
                      /* 0x02 clears frame before drawing */
                      /* 0x00 'don't care' */

                      error))
        return FALSE;

      if (! put_word (output, Delay89, error))
        return FALSE;

      if (! put_byte (output, Transparent, error) ||
          ! put_byte (output, 0, error))
        return FALSE;
    }

  return TRUE;
}


static gboolean
gif_encode_image_data (GOutputStream *output,
                       int            GWidth,
                       int            GHeight,
                       int            GInterlace,
                       int            BitsPerPixel,
                       ifunptr        get_pixel,
                       gint           offset_x,
                       gint           offset_y,
                       GError       **error)
{
  gint LeftOfs, TopOfs;
  gint InitCodeSize;

  Interlace = GInterlace;

  Width   = GWidth;
  Height  = GHeight;
  LeftOfs = (gint) offset_x;
  TopOfs  = (gint) offset_y;

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
  if (! put_byte (output, ',', error))
    return FALSE;

  /*
   * Write the Image header
   */

  if (! put_word (output, LeftOfs, error) ||
      ! put_word (output, TopOfs,  error) ||
      ! put_word (output, Width,   error) ||
      ! put_word (output, Height,  error))
    return FALSE;

  /*
   * Write out whether or not the image is interlaced
   */
  if (Interlace)
    {
      if (! put_byte (output, 0x40, error))
        return FALSE;
    }
  else
    {
      if (! put_byte (output, 0x00, error))
        return FALSE;
    }

  /*
   * Write out the initial code size
   */
  if (! put_byte (output, InitCodeSize, error))
    return FALSE;

  /*
   * Go and actually compress the data
   */
  if (! compress (output, InitCodeSize + 1, get_pixel, error))
    return FALSE;

  /*
   * Write out a Zero-length packet (to end the series)
   */
  if (! put_byte (output, 0, error))
    return FALSE;

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

  return TRUE;
}


static gboolean
gif_encode_close (GOutputStream  *output,
                  GError        **error)
{
  /*
   * Write the GIF file terminator
   */
  if (! put_byte (output, ';', error))
    return FALSE;

  /*
   * And close the file
   */
  return g_output_stream_close (output, NULL, error);
}


static gboolean
gif_encode_loop_ext (GOutputStream  *output,
                     guint           num_loops,
                     GError        **error)
{
  return (put_byte   (output, 0x21, error) &&
          put_byte   (output, 0xff, error) &&
          put_byte   (output, 0x0b, error) &&
          put_string (output, "NETSCAPE2.0", error) &&
          put_byte   (output, 0x03, error) &&
          put_byte   (output, 0x01, error) &&
          put_word   (output, num_loops, error) &&
          put_byte   (output, 0x00, error));

  /* NOTE: num_loops == 0 means 'loop infinitely' */
}


static gboolean
gif_encode_comment_ext (GOutputStream  *output,
                        const gchar    *comment,
                        GError        **error)
{
  if (!comment || !*comment)
    return TRUE;

  if (strlen (comment) > 240)
    {
      g_printerr ("GIF: warning:"
                  "comment too large - comment block not written.\n");
      return TRUE;
    }

  return (put_byte   (output, 0x21, error) &&
          put_byte   (output, 0xfe, error) &&
          put_byte   (output, strlen (comment), error) &&
          put_string (output, comment, error) &&
          put_byte   (output, 0x00, error));
}


/*
 * Write stuff to the GIF file
 */
static gboolean
put_byte (GOutputStream  *output,
          guchar          b,
          GError        **error)
{
  return g_data_output_stream_put_byte (G_DATA_OUTPUT_STREAM (output),
                                        b, NULL, error);
}

static gboolean
put_word (GOutputStream  *output,
          gint            w,
          GError        **error)
{
  return g_data_output_stream_put_int16 (G_DATA_OUTPUT_STREAM (output),
                                         w, NULL, error);
}

static gboolean
put_string (GOutputStream  *output,
            const gchar    *s,
            GError        **error)
{
  return g_data_output_stream_put_string (G_DATA_OUTPUT_STREAM (output),
                                          s, NULL, error);
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

#define GIF_BITS   12

#define HSIZE    5003                /* 80% occupancy */

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

static gint n_bits;                /* number of bits/code */
static gint maxbits = GIF_BITS;    /* user settable max # bits/code */
static gint maxcode;               /* maximum code, given n_bits */
static gint maxmaxcode = (gint) 1 << GIF_BITS;        /* should NEVER generate this code */
#ifdef COMPATIBLE                /* But wrong! */
#define MAXCODE(Mn_bits)        ((gint) 1 << (Mn_bits) - 1)
#else /*COMPATIBLE */
#define MAXCODE(Mn_bits)        (((gint) 1 << (Mn_bits)) - 1)
#endif /*COMPATIBLE */

static glong   htab[HSIZE];
static gushort codetab[HSIZE];
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
static gint  clear_flg = 0;

static gint  offset;
static glong in_count  = 1;        /* length of input */
static glong out_count = 0;        /* # of codes output (for debugging) */

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

static gint g_init_bits;

static gint ClearCode;
static gint EOFCode;

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


static gboolean
compress (GOutputStream  *output,
          gint            init_bits,
          ifunptr         ReadValue,
          GError        **error)
{
  if (FALSE)
    return no_compress (output, init_bits, ReadValue, error);
  else if (FALSE)
    return rle_compress (output, init_bits, ReadValue, error);
  else
    return normal_compress (output, init_bits, ReadValue, error);
}

static gboolean
no_compress (GOutputStream  *output,
             gint            init_bits,
             ifunptr         ReadValue,
             GError        **error)
{
  glong fcode;
  gint  i /* = 0 */ ;
  gint  c;
  gint  ent;
  gint  hsize_reg;
  gint  hshift;

  /*
   * Set up the globals:  g_init_bits - initial number of bits
   */
  g_init_bits = init_bits;

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


  char_init();

  ent = gif_next_pixel (ReadValue);

  hshift = 0;
  for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
    ++hshift;
  hshift = 8 - hshift;                /* set hash code range bound */

  hsize_reg = hsize;
  cl_hash ((glong) hsize_reg);        /* clear hash table */

  if (! output_code (output, (gint) ClearCode, error))
    return FALSE;

  while ((c = gif_next_pixel (ReadValue)) != EOF)
    {
      ++in_count;

      fcode = (long) (((long) c << maxbits) + ent);
      i = (((gint) c << hshift) ^ ent);        /* xor hashing */

      if (! output_code (output, (gint) ent, error))
        return FALSE;

      ++out_count;
      ent = c;

      if (free_ent < maxmaxcode)
        {
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        {
          if (! cl_block (output, error))
            return FALSE;
        }
    }

  /*
   * Put out the final code.
   */
  if (! output_code (output, (gint) ent, error))
    return FALSE;

  ++out_count;

  if (! output_code (output, (gint) EOFCode, error))
    return FALSE;

  return TRUE;
}

static gboolean
rle_compress (GOutputStream  *output,
              gint            init_bits,
              ifunptr         ReadValue,
              GError        **error)
{
  glong fcode;
  gint  i /* = 0 */ ;
  gint  c, last;
  gint  ent;
  gint  disp;
  gint  hsize_reg;
  gint  hshift;

  /*
   * Set up the globals:  g_init_bits - initial number of bits
   */
  g_init_bits = init_bits;

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

  if (! output_code (output, (gint) ClearCode, error))
    return FALSE;


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
      if (! output_code (output, (gint) ent, error))
        return FALSE;

      ++out_count;
      last = ent = c;
      if (free_ent < maxmaxcode)
        {
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        {
          if (! cl_block (output, error))
            return FALSE;
        }
    }

  /*
   * Put out the final code.
   */
  if (! output_code (output, (gint) ent, error))
    return FALSE;

  ++out_count;

  if (! output_code (output, (gint) EOFCode, error))
    return FALSE;

  return TRUE;
}

static gboolean
normal_compress (GOutputStream  *output,
                 gint            init_bits,
                 ifunptr         ReadValue,
                 GError        **error)
{
  glong fcode;
  gint  i /* = 0 */ ;
  gint  c;
  gint  ent;
  gint  disp;
  gint  hsize_reg;
  gint  hshift;

  /*
   * Set up the globals:  g_init_bits - initial number of bits
   */
  g_init_bits = init_bits;

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


  char_init();

  ent = gif_next_pixel (ReadValue);

  hshift = 0;
  for (fcode = (long) hsize; fcode < 65536L; fcode *= 2L)
    ++hshift;
  hshift = 8 - hshift;                /* set hash code range bound */

  hsize_reg = hsize;
  cl_hash ((glong) hsize_reg);        /* clear hash table */

  if (! output_code (output, (gint) ClearCode, error))
    return FALSE;


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
      if (! output_code (output, (gint) ent, error))
        return FALSE;

      ++out_count;
      ent = c;
      if (free_ent < maxmaxcode)
        {
          CodeTabOf (i) = free_ent++;        /* code -> hashtable */
          HashTabOf (i) = fcode;
        }
      else
        {
          if (! cl_block (output, error))
            return FALSE;
        }
    }

  /*
   * Put out the final code.
   */
  if (! output_code (output, (gint) ent, error))
    return FALSE;

  ++out_count;

  if (! output_code (output, (gint) EOFCode, error))
    return FALSE;

  return TRUE;
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

static gboolean
output_code (GOutputStream  *output,
             gint            code,
             GError        **error)
{
  cur_accum &= masks[cur_bits];

  if (cur_bits > 0)
    cur_accum |= ((long) code << cur_bits);
  else
    cur_accum = code;

  cur_bits += n_bits;

  while (cur_bits >= 8)
    {
      if (! char_out (output, (guchar) (cur_accum & 0xff), error))
        return FALSE;

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
          if (! char_out (output, (guchar) (cur_accum & 0xff), error))
            return FALSE;

          cur_accum >>= 8;
          cur_bits -= 8;
        }

      if (! char_flush (output, error))
        return FALSE;
    }

  return TRUE;
}

/*
 * Clear out the hash table
 */
static gboolean
cl_block (GOutputStream  *output,
          GError        **error) /* table clear for block compress */
{
  cl_hash ((glong) hsize);
  free_ent = ClearCode + 2;
  clear_flg = 1;

  return output_code (output, (gint) ClearCode, error);
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
 * GIF Specific routines
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
static gboolean
char_out (GOutputStream  *output,
          gint            c,
          GError        **error)
{
  accum[a_count++] = c;

  if (a_count >= 254)
    return char_flush (output, error);

  return TRUE;
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static gboolean
char_flush (GOutputStream  *output,
            GError        **error)
{
  if (a_count > 0)
    {
      if (! put_byte (output, a_count, error))
        return FALSE;

      if (! g_output_stream_write_all (output, accum, a_count,
                                       NULL, NULL, error))
        return FALSE;

      a_count = 0;
    }

  return TRUE;
}
