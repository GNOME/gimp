/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Multiple-image Network Graphics (MNG) plug-in
 *
 * Copyright (C) 2002 Mukund Sivaraman <muks@mukund.org>
 * Portions are copyright of the authors of the file-gif-export, file-png-export
 * and file-jpeg-export plug-ins' code. The exact ownership of these code
 * fragments has not been determined.
 *
 * This work was sponsored by Xinit Systems Limited, UK.
 * http://www.xinitsystems.com/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * --
 *
 * For now, this MNG plug-in can only export images. It cannot load images.
 * Save your working copy as .xcf and use this for "exporting" your images
 * to MNG. Supports animation the same way as animated GIFs. Supports alpha
 * transparency. Uses the libmng library (http://www.libmng.com/).
 * The MIME content-type for MNG images is video/x-mng for now. Make sure
 * your web-server is configured appropriately if you want to serve MNG
 * images.
 *
 * Since libmng cannot write PNG, JNG and delta PNG chunks at this time
 * (when this text was written), this plug-in uses libpng and jpeglib to
 * create the data for the chunks.
 *
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


/* libpng and jpeglib are currently used in this plug-in. */

#include <png.h>
#include <jpeglib.h>


/* Grrr. The grrr is because the following have to be defined by the
 * application as well for some reason, although they were enabled
 * when libmng was compiled. The authors of libmng must look into
 * this.
 */

#if !defined(MNG_SUPPORT_FULL)
#define MNG_SUPPORT_FULL 1
#endif

#if !defined(MNG_SUPPORT_READ)
#define MNG_SUPPORT_READ 1
#endif

#if !defined(MNG_SUPPORT_WRITE)
#define MNG_SUPPORT_WRITE 1
#endif

#if !defined(MNG_SUPPORT_DISPLAY)
#define MNG_SUPPORT_DISPLAY 1
#endif

#if !defined(MNG_ACCESS_CHUNKS)
#define MNG_ACCESS_CHUNKS 1
#endif

#include <libmng.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "libgimp/stdplugins-intl.h"


#define EXPORT_PROC    "file-mng-export"
#define PLUG_IN_BINARY "file-mng"
#define PLUG_IN_ROLE   "gimp-file-mng"
#define SCALE_WIDTH    125

enum
{
  CHUNKS_PNG_D,
  CHUNKS_JNG_D,
  CHUNKS_PNG,
  CHUNKS_JNG
};

enum
{
  DISPOSE_COMBINE,
  DISPOSE_REPLACE
};


/* These are not saved or restored. */

struct mng_globals_t
{
  gboolean   has_trns;
  png_bytep  trans;
  int        num_trans;
  gboolean   has_plte;
  png_colorp palette;
  int        num_palette;
};

/* The output FILE pointer which is used by libmng; passed around as
 * user data.
 */
struct mnglib_userdata_t
{
  FILE *fp;
};


typedef struct _Mng      Mng;
typedef struct _MngClass MngClass;

struct _Mng
{
  GimpPlugIn      parent_instance;
};

struct _MngClass
{
  GimpPlugInClass parent_class;
};


#define MNG_TYPE  (mng_get_type ())
#define MNG(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MNG_TYPE, Mng))

GType                   mng_get_type             (void) G_GNUC_CONST;

static GList          * mng_query_procedures     (GimpPlugIn           *plug_in);
static GimpProcedure  * mng_create_procedure     (GimpPlugIn           *plug_in,
                                                  const gchar          *name);

static GimpValueArray * mng_export               (GimpProcedure        *procedure,
                                                  GimpRunMode           run_mode,
                                                  GimpImage            *image,
                                                  GFile                *file,
                                                  GimpExportOptions    *options,
                                                  GimpMetadata         *metadata,
                                                  GimpProcedureConfig  *config,
                                                  gpointer              run_data);

static mng_ptr  MNG_DECL  myalloc       (mng_size_t  size);
static void     MNG_DECL  myfree        (mng_ptr     ptr,
                                         mng_size_t  size);
static mng_bool MNG_DECL  myopenstream  (mng_handle  handle);
static mng_bool MNG_DECL  myclosestream (mng_handle  handle);
static mng_bool MNG_DECL  mywritedata   (mng_handle  handle,
                                         mng_ptr     buf,
                                         mng_uint32  size,
                                         mng_uint32 *written_size);


static gint32    parse_chunks_type_from_layer_name   (const gchar *str,
                                                      gint         default_chunks);
static gint32    parse_disposal_type_from_layer_name (const gchar *str,
                                                      gint         default_dispose);
static gint32    parse_ms_tag_from_layer_name        (const gchar *str,
                                                      gint         default_delay);
static gint      find_unused_ia_color                (guchar      *pixels,
                                                      gint         numpixels,
                                                      gint        *colors);
static gboolean  ia_has_transparent_pixels           (guchar      *pixels,
                                                      gint         numpixels);

static gboolean  respin_cmap     (png_structp       png_ptr,
                                  png_infop         png_info_ptr,
                                  guchar           *remap,
                                  GimpImage        *image,
                                  GeglBuffer       *buffer,
                                  int              *bit_depth);

static gboolean  mng_export_image (GFile            *file,
                                   GimpImage        *image,
                                   gint              n_drawables,
                                   GList            *drawables,
                                   GObject          *config,
                                   GError          **error);
static gboolean  mng_save_dialog  (GimpImage        *image,
                                   GimpProcedure    *procedure,
                                   GObject          *config);


G_DEFINE_TYPE (Mng, mng, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (MNG_TYPE)
DEFINE_STD_SET_I18N


static struct mng_globals_t mngg;


static void
mng_class_init (MngClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = mng_query_procedures;
  plug_in_class->create_procedure = mng_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
mng_init (Mng *mng)
{
}

static GList *
mng_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (EXPORT_PROC));
}

static GimpProcedure *
mng_create_procedure (GimpPlugIn  *plug_in,
                        const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, mng_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, _("MNG animation"));
      gimp_file_procedure_set_format_name (GIMP_FILE_PROCEDURE (procedure),
                                           _("MNG"));

      gimp_procedure_set_documentation (procedure,
                                        _("Saves images in the MNG file format"),
                                        _("This plug-in saves images in the "
                                          "Multiple-image Network Graphics (MNG) "
                                          "format which can be used as a "
                                          "replacement for animated GIFs, and "
                                          "more."),
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "Mukund Sivaraman <muks@mukund.org>",
                                      "November 19, 2002");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-mng");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "mng");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB     |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                              GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                              GIMP_EXPORT_CAN_HANDLE_LAYERS,
                                              NULL, NULL, NULL);

      gimp_procedure_add_boolean_argument (procedure, "interlaced",
                                           _("_Interlace"),
                                           _("Use interlacing"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "png-compression",
                                       _("_PNG compression level"),
                                       _("PNG compression level, choose a high compression "
                                         "level for small file size"),
                                       0, 9, 9,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "jpeg-quality",
                                          _("JPEG compression _quality"),
                                          _("JPEG quality factor"),
                                          0, 1, 0.75,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "jpeg-smoothing",
                                          _("_JPEG smoothing factor"),
                                          _("JPEG smoothing factor"),
                                          0, 1, 0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "loop",
                                           _("L_oop"),
                                           _("(ANIMATED MNG) Loop infinitely"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "default-delay",
                                       _("Default fra_me delay"),
                                       _("(ANIMATED MNG) Default delay between frames in "
                                         "milliseconds"),
                                       1, G_MAXINT, 100,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "default-chunks",
                                          _("Default chunks t_ype"),
                                          _("(ANIMATED MNG) Default chunks type"),
                                          gimp_choice_new_with_values ("png-delta", CHUNKS_PNG_D, _("PNG + delta PNG"), NULL,
                                                                       "jng-delta", CHUNKS_JNG_D, _("JNG + delta PNG"), NULL,
                                                                       "all-png",   CHUNKS_PNG,   _("All PNG"),         NULL,
                                                                       "all-jng",   CHUNKS_JNG,   _("All JNG"),         NULL,
                                                                       NULL),
                                          "png-delta",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "default-dispose",
                                          _("De_fault frame disposal"),
                                          _("(ANIMATED MNG) Default dispose type"),
                                          gimp_choice_new_with_values ("combine", DISPOSE_COMBINE, _("Combine"), NULL,
                                                                       "replace", DISPOSE_REPLACE, _("Replace"), NULL,
                                                                       NULL),
                                          "combine",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "bkgd",
                                           _("Save _background color"),
                                           _("Write bKGd (background color) chunk"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "gama",
                                           _("Save _gamma"),
                                           _("Write gAMA (gamma) chunk"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "phys",
                                           _("Sa_ve resolution"),
                                           _("Write pHYs (image resolution) chunk"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "time",
                                           _("Save creation _time"),
                                           _("Write tIME (creation time) chunk"),
                                           TRUE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
mng_export (GimpProcedure        *procedure,
            GimpRunMode           run_mode,
            GimpImage            *image,
            GFile                *file,
            GimpExportOptions    *options,
            GimpMetadata         *metadata,
            GimpProcedureConfig  *config,
            gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_IGNORE;
  GList             *drawables;
  gint               n_drawables;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gimp_ui_init (PLUG_IN_BINARY);

      if (! mng_save_dialog (image, procedure, G_OBJECT (config)))
        status = GIMP_PDB_CANCEL;
    }

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);
  n_drawables = g_list_length (drawables);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! mng_export_image (file, image, n_drawables, drawables,
                              G_OBJECT (config), &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}


/*
 * Callbacks for libmng
 */

static mng_ptr MNG_DECL
myalloc (mng_size_t size)
{
  gpointer ptr;

  ptr = g_try_malloc (size);

  if (ptr != NULL)
    memset (ptr, 0, size);

  return ((mng_ptr) ptr);
}

static void MNG_DECL
myfree (mng_ptr    ptr,
        mng_size_t size)
{
  g_free (ptr);
}

static mng_bool MNG_DECL
myopenstream (mng_handle handle)
{
  return MNG_TRUE;
}

static mng_bool MNG_DECL
myclosestream (mng_handle handle)
{
  return MNG_TRUE;
}

static mng_bool MNG_DECL
mywritedata (mng_handle  handle,
             mng_ptr     buf,
             mng_uint32  size,
             mng_uint32 *written_size)
{
  struct mnglib_userdata_t *userdata =
    (struct mnglib_userdata_t *) mng_get_userdata (handle);


  *written_size = (mng_uint32) fwrite ((void *) buf, 1,
                                       (size_t) size, userdata->fp);

  return MNG_TRUE;
}


/* Parses which output chunk type to use for this layer from the layer
 * name.
 */
static gint32
parse_chunks_type_from_layer_name (const gchar *str,
                                   gint         default_chunks)
{
  guint i;

  for (i = 0; (i + 5) <= strlen (str); i++)
    {
      if (g_ascii_strncasecmp (str + i, "(png)", 5) == 0)
        return CHUNKS_PNG;
      else if (g_ascii_strncasecmp (str + i, "(jng)", 5) == 0)
        return CHUNKS_JNG;
    }

  return default_chunks;
}


/* Parses which disposal type to use for this layer from the layer
 * name.
 */

static gint32
parse_disposal_type_from_layer_name (const gchar *str,
                                     gint         default_dispose)
{
  guint i;

  for (i = 0; (i + 9) <= strlen (str); i++)
    {
      if (g_ascii_strncasecmp (str + i, "(combine)", 9) == 0)
        return DISPOSE_COMBINE;
      else if (g_ascii_strncasecmp (str + i, "(replace)", 9) == 0)
        return DISPOSE_REPLACE;
    }

  return default_dispose;
}


/* Parses the millisecond delay to use for this layer
 * from the layer name. */

static gint32
parse_ms_tag_from_layer_name (const gchar *str,
                              gint         default_delay)
{
  guint offset = 0;
  gint32 sum = 0;
  guint length = strlen (str);

  while (TRUE)
    {
      while ((offset < length) && (str[offset] != '('))
        offset++;

      if (offset >= length)
        return default_delay;

      offset++;

      if (g_ascii_isdigit (str[offset]))
        break;
    }

  do
    {
      sum *= 10;
      sum += str[offset] - '0';
      offset++;
    }
  while ((offset < length) && (g_ascii_isdigit (str[offset])));

  if ((length - offset) <= 2)
    return default_delay;

  if ((g_ascii_toupper (str[offset]) != 'M') ||
      (g_ascii_toupper (str[offset + 1]) != 'S'))
    return default_delay;

  return sum;
}


/* Try to find a color in the palette which isn't actually
 * used in the image, so that we can use it as the transparency
 * index. Taken from png.c */
static gint
find_unused_ia_color (guchar *pixels,
                       gint    numpixels,
                       gint   *colors)
{
  gint     i;
  gboolean ix_used[256];
  gboolean trans_used = FALSE;

  for (i = 0; i < *colors; i++)
    {
      ix_used[i] = FALSE;
    }

  for (i = 0; i < numpixels; i++)
    {
      /* If alpha is over a threshold, the color index in the
       * palette is taken. Otherwise, this pixel is transparent. */
      if (pixels[i * 2 + 1] > 127)
        ix_used[pixels[i * 2]] = TRUE;
      else
        trans_used = TRUE;
    }

  /* If there is no transparency, ignore alpha. */
  if (trans_used == FALSE)
    return -1;

  for (i = 0; i < *colors; i++)
    {
      if (ix_used[i] == FALSE)
        {
          return i;
        }
    }

  /* Couldn't find an unused color index within the number of
     bits per pixel we wanted.  Will have to increment the number
     of colors in the image and assign a transparent pixel there. */
  if ((*colors) < 256)
    {
      (*colors)++;
      return ((*colors) - 1);
    }

  return -1;
}


static gboolean
ia_has_transparent_pixels (guchar *pixels,
                           gint    numpixels)
{
  while (numpixels --)
    {
      if (pixels [1] <= 127)
        return TRUE;
      pixels += 2;
    }
  return FALSE;
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

/* Spins the color map (palette) putting the transparent color at
 * index 0 if there is space. If there isn't any space, warn the user
 * and forget about transparency. Returns TRUE if the colormap has
 * been changed and FALSE otherwise.
 */

static gboolean
respin_cmap (png_structp  pp,
             png_infop    info,
             guchar      *remap,
             GimpImage   *image,
             GeglBuffer  *buffer,
             int         *bit_depth)
{
  static guchar  trans[] = { 0 };
  guchar        *before;
  guchar        *pixels;
  gint           numpixels;
  gint           colors;
  gint           transparent;
  gint           cols, rows;

  before = gimp_image_get_colormap (image, NULL, &colors);

  /* Make sure there is something in the colormap */
  if (colors == 0)
    {
      before = g_newa (guchar, 3);
      memset (before, 0, sizeof (guchar) * 3);

      colors = 1;
    }

  cols      = gegl_buffer_get_width  (buffer);
  rows      = gegl_buffer_get_height (buffer);
  numpixels = cols * rows;

  pixels = (guchar *) g_malloc (numpixels * 2);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, cols, rows), 1.0,
                   NULL, pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (ia_has_transparent_pixels (pixels, numpixels))
    {
      transparent = find_unused_ia_color (pixels, numpixels, &colors);

      if (transparent != -1)
        {
          static png_color palette[256] = { {0, 0, 0} };
          gint i;

          /* Set tRNS chunk values for writing later. */
          mngg.has_trns  = TRUE;
          mngg.trans     = trans;
          mngg.num_trans = 1;

          /* Transform all pixels with a value = transparent to
           * 0 and vice versa to compensate for re-ordering in palette
           * due to png_set_tRNS().
           */

          remap[0] = transparent;
          remap[transparent] = 0;

          /* Copy from index 0 to index transparent - 1 to index 1 to
           * transparent of after, then from transparent+1 to colors-1
           * unchanged, and finally from index transparent to index 0.
           */

          for (i = 1; i < colors; i++)
            {
              palette[i].red = before[3 * remap[i]];
              palette[i].green = before[3 * remap[i] + 1];
              palette[i].blue = before[3 * remap[i] + 2];
            }

          /* Set PLTE chunk values for writing later. */
          mngg.has_plte    = TRUE;
          mngg.palette     = palette;
          mngg.num_palette = colors;

          *bit_depth = get_bit_depth_for_palette (colors);

          return TRUE;
        }
      else
        {
          g_message (_("Couldn't losslessly save transparency, "
                       "saving opacity instead."));
        }
    }

  mngg.has_plte    = TRUE;
  mngg.palette     = (png_colorp) before;
  mngg.num_palette = colors;

  *bit_depth = get_bit_depth_for_palette (colors);

  return FALSE;
}

static mng_retcode
mng_putchunk_plte_wrapper (mng_handle    handle,
                           gint          numcolors,
                           const guchar *colormap)
{
  mng_palette8 palette;

  memset (palette, 0, sizeof palette);
  if (0 < numcolors)
    memcpy (palette, colormap, numcolors * sizeof palette[0]);

  return mng_putchunk_plte (handle, numcolors, palette);
}

static mng_retcode
mng_putchunk_trns_wrapper (mng_handle    handle,
                           gint          n_alphas,
                           const guchar *buffer)
{
  const mng_bool mng_global = TRUE;
  const mng_bool mng_empty  = TRUE;
  mng_uint8arr   alphas;

  memset (alphas, 0, sizeof alphas);
  if (buffer && 0 < n_alphas)
    memcpy (alphas, buffer, n_alphas * sizeof alphas[0]);

  return mng_putchunk_trns (handle,
                            ! mng_empty,
                            ! mng_global,
                            MNG_COLORTYPE_INDEXED,
                            n_alphas,
                            alphas,
                            0, 0, 0, 0, 0, alphas);
}

static gboolean
mng_export_image (GFile         *file,
                  GimpImage     *image,
                  gint           n_drawables,
                  GList         *drawables,
                  GObject       *config,
                  GError       **error)
{
  gboolean        ret = FALSE;
  gint            rows, cols;
  volatile gint   i;
  time_t          t;
  struct tm      *gmt;

  gint            num_layers;
  GimpLayer     **layers;

  struct mnglib_userdata_t *userdata;
  mng_handle      handle;
  guint32         mng_ticks_per_second;
  guint32         mng_simplicity_profile;

  gboolean config_interlaced;
  gint     config_png_compression;
  gdouble  config_jpeg_quality;
  gdouble  config_jpeg_smoothing;
  gboolean config_loop;
  gint     config_default_delay;
  gint     config_default_chunks;
  gint     config_default_dispose;
  gboolean config_bkgd;
  gboolean config_gama;
  gboolean config_phys;
  gboolean config_time;

  g_object_get (config,
                "interlaced",      &config_interlaced,
                "png-compression", &config_png_compression,
                "jpeg-quality",    &config_jpeg_quality,
                "jpeg-smoothing",  &config_jpeg_smoothing,
                "loop",            &config_loop,
                "default-delay",   &config_default_delay,
                "bkgd",            &config_bkgd,
                "gama",            &config_gama,
                "phys",            &config_phys,
                "time",            &config_time,
                NULL);
  config_default_dispose =
    gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                         "default-dispose");
  config_default_chunks =
    gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                         "default-chunks");

  layers = gimp_image_get_layers (image, &num_layers);

  if (num_layers < 1)
    return FALSE;

  if (num_layers > 1)
    mng_ticks_per_second = 1000;
  else
    mng_ticks_per_second = 0;

  rows = gimp_image_get_height (image);
  cols = gimp_image_get_width  (image);

  mng_simplicity_profile = (MNG_SIMPLICITY_VALID |
                            MNG_SIMPLICITY_SIMPLEFEATURES |
                            MNG_SIMPLICITY_COMPLEXFEATURES);

  /* JNG and delta-PNG chunks exist */
  mng_simplicity_profile |= (MNG_SIMPLICITY_JNG |
                             MNG_SIMPLICITY_DELTAPNG);

  for (i = 0; i < num_layers; i++)
    {
      if (gimp_drawable_has_alpha (GIMP_DRAWABLE (layers[i])))
        {
          /* internal transparency exists */
          mng_simplicity_profile |= MNG_SIMPLICITY_TRANSPARENCY;

          /* validity of following flags */
          mng_simplicity_profile |= 0x00000040;

          /* semi-transparency exists */
          mng_simplicity_profile |= 0x00000100;

          /* background transparency should happen */
          mng_simplicity_profile |= 0x00000080;

          break;
        }
    }

  userdata = g_new0 (struct mnglib_userdata_t, 1);
  userdata->fp = g_fopen (g_file_peek_path (file), "wb");

  if (! userdata->fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      goto err;
    }

  handle = mng_initialize ((mng_ptr) userdata, myalloc, myfree, NULL);
  if (! handle)
    {
      g_warning ("Unable to mng_initialize() in mng_export_image()");
      goto err2;
    }

  if ((mng_setcb_openstream (handle, myopenstream) != MNG_NOERROR) ||
      (mng_setcb_closestream (handle, myclosestream) != MNG_NOERROR) ||
      (mng_setcb_writedata (handle, mywritedata) != MNG_NOERROR))
    {
      g_warning ("Unable to setup callbacks in mng_export_image()");
      goto err3;
    }

  if (mng_create (handle) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_create() image in mng_export_image()");
      goto err3;
    }

  if (mng_putchunk_mhdr (handle, cols, rows, mng_ticks_per_second, 1,
                         num_layers, config_default_delay,
                         mng_simplicity_profile) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_mhdr() in mng_export_image()");
      goto err3;
    }

  if ((num_layers > 1) && (config_loop))
    {
      gint32 ms =
        parse_ms_tag_from_layer_name (gimp_item_get_name (GIMP_ITEM (layers[0])),
                                      config_default_delay);

      if (mng_putchunk_term (handle, MNG_TERMACTION_REPEAT,
                             MNG_ITERACTION_LASTFRAME,
                             ms, 0x7fffffff) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_term() in mng_export_image()");
          goto err3;
        }
    }
  else
    {
      gint32 ms =
        parse_ms_tag_from_layer_name (gimp_item_get_name (GIMP_ITEM (layers[0])),
                                      config_default_delay);

      if (mng_putchunk_term (handle, MNG_TERMACTION_LASTFRAME,
                             MNG_ITERACTION_LASTFRAME,
                             ms, 0x7fffffff) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_term() in mng_export_image()");
          goto err3;
        }
    }


  /* For now, we hardwire a comment */

  if (mng_putchunk_text (handle,
                         strlen (MNG_TEXT_TITLE), MNG_TEXT_TITLE,
                         18, "Created using GIMP") != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_text() in mng_export_image()");
      goto err3;
    }

#if 0

  /* how do we get this to work? */
  if (config_bkgd)
    {
      GeglColor *background;
      guchar     rgb[3];
      guchar     luminance[1];

      background = gimp_context_get_background ();
      gegl_color_get_pixel (background, babl_format ("R'G'B' u8"), rgb);
      gegl_color_get_pixel (background, babl_format ("Y' u8"), luminance);
      g_object_unref (background);

      if (mng_putchunk_back (handle, rgb[0], rgb[1], rgb[2],
                             MNG_BACKGROUNDCOLOR_MANDATORY,
                             0, MNG_BACKGROUNDIMAGE_NOTILE) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_back() in mng_export_image()");
          goto err3;
        }

      if (mng_putchunk_bkgd (handle, MNG_FALSE, 2, 0,
                             luminance,
                             rgb[0], rgb[1], rgb[2])) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_bkgd() in mng_export_image()");
          goto err3;
        }
    }

#endif

  if (config_gama)
    {
      if (mng_putchunk_gama (handle, MNG_FALSE,
                             (1.0 / 2.2 * 100000)) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_gama() in mng_export_image()");
          goto err3;
        }
    }

#if 0

  /* how do we get this to work? */
  if (config_phys)
    {
      gimp_image_get_resolution (image, &xres, &yres);

      if (mng_putchunk_phyg (handle, MNG_FALSE,
                             (mng_uint32) (xres * 39.37),
                             (mng_uint32) (yres * 39.37), 1) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_phyg() in mng_export_image()");
          goto err3;
        }

      if (mng_putchunk_phys (handle, MNG_FALSE,
                             (mng_uint32) (xres * 39.37),
                             (mng_uint32) (yres * 39.37), 1) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_phys() in mng_export_image()");
          goto err3;
        }
    }

#endif

  if (config_time)
    {
      t = time (NULL);
      gmt = gmtime (&t);

      if (mng_putchunk_time (handle, gmt->tm_year + 1900, gmt->tm_mon + 1,
                             gmt->tm_mday, gmt->tm_hour, gmt->tm_min,
                             gmt->tm_sec) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_time() in mng_export_image()");
          goto err3;
        }
    }

  if (gimp_image_get_base_type (image) == GIMP_INDEXED)
    {
      guchar *palette;
      gint    numcolors;

      palette = gimp_image_get_colormap (image, NULL, &numcolors);

      if ((numcolors != 0) &&
          (mng_putchunk_plte_wrapper (handle, numcolors,
                                      palette) != MNG_NOERROR))
        {
          g_warning ("Unable to mng_putchunk_plte() in mng_export_image()");
          goto err3;
        }
    }

  for (i = (num_layers - 1); i >= 0; i--)
    {
      GimpImageType   layer_drawable_type;
      GeglBuffer     *layer_buffer;
      gint            layer_offset_x, layer_offset_y;
      gint            layer_rows, layer_cols;
      gchar          *layer_name;
      gint            layer_chunks_type;
      const Babl     *layer_format;
      volatile gint   layer_bpp;

      guint8          __attribute__((unused))layer_mng_colortype;
      guint8          __attribute__((unused))layer_mng_compression_type;
      guint8          __attribute__((unused))layer_mng_interlace_type;
      gboolean        layer_has_unique_palette;

      gchar           frame_mode;
      int             frame_delay;
      GFile          *temp_file;
      png_structp     pp;
      png_infop       info;
      FILE           *infile, *outfile;
      int             num_passes;
      int             tile_height;
      guchar        **layer_pixels, *layer_pixel;
      int             pass, j, k, begin, end, num;
      guchar         *fixed;
      guchar          layer_remap[256];
      int             color_type;
      int             bit_depth;

      layer_name          = gimp_item_get_name (GIMP_ITEM (layers[i]));
      layer_chunks_type   = parse_chunks_type_from_layer_name (layer_name,
                                                               config_default_chunks);
      layer_drawable_type = gimp_drawable_type (GIMP_DRAWABLE (layers[i]));

      layer_buffer        = gimp_drawable_get_buffer (GIMP_DRAWABLE (layers[i]));
      layer_cols          = gegl_buffer_get_width  (layer_buffer);
      layer_rows          = gegl_buffer_get_height (layer_buffer);

      gimp_drawable_get_offsets (GIMP_DRAWABLE (layers[i]),
                             &layer_offset_x, &layer_offset_y);
      layer_has_unique_palette = TRUE;

      for (j = 0; j < 256; j++)
        layer_remap[j] = j;

      switch (layer_drawable_type)
        {
        case GIMP_RGB_IMAGE:
          layer_format        = babl_format ("R'G'B' u8");
          layer_mng_colortype = MNG_COLORTYPE_RGB;
          break;
        case GIMP_RGBA_IMAGE:
          layer_format        = babl_format ("R'G'B'A u8");
          layer_mng_colortype = MNG_COLORTYPE_RGBA;
          break;
        case GIMP_GRAY_IMAGE:
          layer_format        = babl_format ("Y' u8");
          layer_mng_colortype = MNG_COLORTYPE_GRAY;
          break;
        case GIMP_GRAYA_IMAGE:
          layer_format        = babl_format ("Y'A u8");
          layer_mng_colortype = MNG_COLORTYPE_GRAYA;
          break;
        case GIMP_INDEXED_IMAGE:
          layer_format        = gegl_buffer_get_format (layer_buffer);
          layer_mng_colortype = MNG_COLORTYPE_INDEXED;
          break;
        case GIMP_INDEXEDA_IMAGE:
          layer_format        = gegl_buffer_get_format (layer_buffer);
          layer_mng_colortype = MNG_COLORTYPE_INDEXED | MNG_COLORTYPE_GRAYA;
          break;
        default:
          g_warning ("Unsupported GimpImageType in mng_export_image()");
          goto err3;
        }

      layer_bpp = babl_format_get_bytes_per_pixel (layer_format);

      /* Delta PNG chunks are not yet supported */

      /* if (i == (num_layers - 1)) */
      {
        if (layer_chunks_type == CHUNKS_JNG_D)
          layer_chunks_type = CHUNKS_JNG;
        else if (layer_chunks_type == CHUNKS_PNG_D)
          layer_chunks_type = CHUNKS_PNG;
      }

      switch (layer_chunks_type)
        {
        case CHUNKS_PNG_D:
          layer_mng_compression_type = MNG_COMPRESSION_DEFLATE;
          if (config_interlaced)
            layer_mng_interlace_type = MNG_INTERLACE_ADAM7;
          else
            layer_mng_interlace_type = MNG_INTERLACE_NONE;
          break;

        case CHUNKS_JNG_D:
          layer_mng_compression_type = MNG_COMPRESSION_DEFLATE;
          if (config_interlaced)
            layer_mng_interlace_type = MNG_INTERLACE_ADAM7;
          else
            layer_mng_interlace_type = MNG_INTERLACE_NONE;
          break;

        case CHUNKS_PNG:
          layer_mng_compression_type = MNG_COMPRESSION_DEFLATE;
          if (config_interlaced)
            layer_mng_interlace_type = MNG_INTERLACE_ADAM7;
          else
            layer_mng_interlace_type = MNG_INTERLACE_NONE;
          break;

        case CHUNKS_JNG:
          layer_mng_compression_type = MNG_COMPRESSION_BASELINEJPEG;
          if (config_interlaced)
            layer_mng_interlace_type = MNG_INTERLACE_PROGRESSIVE;
          else
            layer_mng_interlace_type = MNG_INTERLACE_SEQUENTIAL;
          break;

        default:
          g_warning ("Huh? Programmer stupidity error "
                     "with 'layer_chunks_type'");
          goto err3;
        }

      if ((i == (num_layers - 1)) ||
          (parse_disposal_type_from_layer_name (layer_name,
                                                config_default_dispose) != DISPOSE_COMBINE))
        frame_mode = MNG_FRAMINGMODE_3;
      else
        frame_mode = MNG_FRAMINGMODE_1;

      frame_delay = parse_ms_tag_from_layer_name (layer_name,
                                                  config_default_delay);

      if (mng_putchunk_fram (handle, MNG_FALSE, frame_mode, 0, NULL,
                             MNG_CHANGEDELAY_DEFAULT,
                             MNG_CHANGETIMOUT_NO,
                             MNG_CHANGECLIPPING_DEFAULT,
                             MNG_CHANGESYNCID_NO,
                             frame_delay, 0, 0,
                             layer_offset_x,
                             layer_offset_x + layer_cols,
                             layer_offset_y,
                             layer_offset_y + layer_rows,
                             0, NULL) != MNG_NOERROR)
        {
          g_warning ("Unable to mng_putchunk_fram() in mng_export_image()");
          goto err3;
        }

      if ((layer_offset_x != 0) || (layer_offset_y != 0))
        {
          if (mng_putchunk_defi (handle, 0, 0, 1, 1, layer_offset_x,
                                 layer_offset_y, 1, layer_offset_x,
                                 layer_offset_x + layer_cols, layer_offset_y,
                                 layer_offset_y + layer_rows) != MNG_NOERROR)
            {
              g_warning ("Unable to mng_putchunk_defi() in mng_export_image()");
              goto err3;
            }
        }

      if ((temp_file = gimp_temp_file ("mng")) == NULL)
        {
          g_warning ("gimp_temp_file() failed in mng_export_image()");
          goto err3;
        }

      if ((outfile = g_fopen (g_file_peek_path (temp_file), "wb")) == NULL)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not open '%s' for writing: %s"),
                       gimp_file_get_utf8_name (temp_file),
                       g_strerror (errno));
          g_file_delete (temp_file, NULL, NULL);
          goto err3;
        }

      pp = png_create_write_struct (PNG_LIBPNG_VER_STRING,
                                    NULL, NULL, NULL);
      if (! pp)
        {
          g_warning ("Unable to png_create_write_struct() in mng_export_image()");
          fclose (outfile);
          g_file_delete (temp_file, NULL, NULL);
          goto err3;
        }

      info = png_create_info_struct (pp);
      if (! info)
        {
          g_warning
            ("Unable to png_create_info_struct() in mng_export_image()");
          png_destroy_write_struct (&pp, NULL);
          fclose (outfile);
          g_file_delete (temp_file, NULL, NULL);
          goto err3;
        }

      if (setjmp (png_jmpbuf (pp)) != 0)
        {
          g_warning ("HRM saving PNG in mng_export_image()");
          png_destroy_write_struct (&pp, &info);
          fclose (outfile);
          g_file_delete (temp_file, NULL, NULL);
          goto err3;
        }

      png_init_io (pp, outfile);

      bit_depth = 8;

      switch (layer_drawable_type)
        {
        case GIMP_RGB_IMAGE:
          color_type = PNG_COLOR_TYPE_RGB;
          break;

        case GIMP_RGBA_IMAGE:
          color_type = PNG_COLOR_TYPE_RGB_ALPHA;
          break;

        case GIMP_GRAY_IMAGE:
          color_type = PNG_COLOR_TYPE_GRAY;
          break;

        case GIMP_GRAYA_IMAGE:
          color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
          break;

        case GIMP_INDEXED_IMAGE:
          color_type = PNG_COLOR_TYPE_PALETTE;

          mngg.has_plte = TRUE;
          mngg.palette  = (png_colorp)
            gimp_image_get_colormap (image, NULL, &mngg.num_palette);

          bit_depth = get_bit_depth_for_palette (mngg.num_palette);
          break;

        case GIMP_INDEXEDA_IMAGE:
          color_type = PNG_COLOR_TYPE_PALETTE;

          layer_has_unique_palette =
            respin_cmap (pp, info, layer_remap,
                         image, layer_buffer,
                         &bit_depth);
          break;

        default:
          g_warning ("This can't be!\n");
          png_destroy_write_struct (&pp, &info);
          fclose (outfile);
          g_file_delete (temp_file, NULL, NULL);
          goto err3;
        }

      /* Note: png_set_IHDR() must be called before any other
       * png_set_*() functions.
       */
      png_set_IHDR (pp, info, layer_cols, layer_rows,
                    bit_depth,
                    color_type,
                    config_interlaced ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
                    PNG_COMPRESSION_TYPE_BASE,
                    PNG_FILTER_TYPE_BASE);

      if (mngg.has_trns)
        {
          png_set_tRNS (pp, info, mngg.trans, mngg.num_trans, NULL);
        }

      if (mngg.has_plte)
        {
          png_set_PLTE (pp, info, mngg.palette, mngg.num_palette);
        }

      png_set_compression_level (pp, config_png_compression);

      png_write_info (pp, info);

      if (config_interlaced)
        num_passes = png_set_interlace_handling (pp);
      else
        num_passes = 1;

      if ((color_type == PNG_COLOR_TYPE_PALETTE) &&
          (bit_depth < 8))
        png_set_packing (pp);

      tile_height = gimp_tile_height ();
      layer_pixel = g_new (guchar, tile_height * layer_cols * layer_bpp);
      layer_pixels = g_new (guchar *, tile_height);

      for (j = 0; j < tile_height; j++)
        layer_pixels[j] = layer_pixel + (layer_cols * layer_bpp * j);

      for (pass = 0; pass < num_passes; pass++)
        {
          for (begin = 0, end = tile_height;
               begin < layer_rows;
               begin += tile_height, end += tile_height)
            {
              if (end > layer_rows)
                end = layer_rows;

              num = end - begin;

              gegl_buffer_get (layer_buffer,
                               GEGL_RECTANGLE (0, begin, layer_cols, num), 1.0,
                               layer_format, layer_pixel,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

              if (png_get_valid (pp, info, PNG_INFO_tRNS))
                {
                  for (j = 0; j < num; j++)
                    {
                      fixed = layer_pixels[j];

                      for (k = 0; k < layer_cols; k++)
                        fixed[k] = (fixed[k * 2 + 1] > 127) ?
                          layer_remap[fixed[k * 2]] : 0;
                    }
                }
              else if (png_get_valid (pp, info, PNG_INFO_PLTE)
                       && (layer_bpp == 2))
                {
                  for (j = 0; j < num; j++)
                    {
                      fixed = layer_pixels[j];

                      for (k = 0; k < layer_cols; k++)
                        fixed[k] = fixed[k * 2];
                    }
                }

              png_write_rows (pp, layer_pixels, num);
            }
        }

      g_object_unref (layer_buffer);

      png_write_end (pp, info);
      png_destroy_write_struct (&pp, &info);

      g_free (layer_pixels);
      g_free (layer_pixel);

      fclose (outfile);

      infile = g_fopen (g_file_peek_path (temp_file), "rb");

      if (! infile)
        {
          g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                       _("Could not open '%s' for reading: %s"),
                       gimp_file_get_utf8_name (temp_file),
                       g_strerror (errno));
          g_file_delete (temp_file, NULL, NULL);
          goto err3;
        }

      fseek (infile, 8L, SEEK_SET);

      while (!feof (infile))
        {
          guchar  chunksize_chars[4];
          gulong  chunksize;
          gchar   chunkname[5];
          guchar *chunkbuffer;
          glong   chunkwidth;
          glong   chunkheight;
          gchar   chunkbitdepth;
          gchar   chunkcolortype;
          gchar   chunkcompression;
          gchar   chunkfilter;
          gchar   chunkinterlaced;


          if (fread (chunksize_chars, 1, 4, infile) != 4)
            break;

          if (fread (chunkname, 1, 4, infile) != 4)
            break;

          chunkname[4] = 0;

          chunksize = ((chunksize_chars[0] << 24) |
                       (chunksize_chars[1] << 16) |
                       (chunksize_chars[2] << 8) |
                       (chunksize_chars[3]));

          chunkbuffer = NULL;

          if (chunksize > 0)
            {
              chunkbuffer = g_new (guchar, chunksize);

              if (fread (chunkbuffer, 1, chunksize, infile) != chunksize)
                break;
            }

          if (strncmp (chunkname, "IHDR", 4) == 0)
            {
              chunkwidth = ((chunkbuffer[0] << 24) |
                            (chunkbuffer[1] << 16) |
                            (chunkbuffer[2] << 8) |
                            (chunkbuffer[3]));

              chunkheight = ((chunkbuffer[4] << 24) |
                             (chunkbuffer[5] << 16) |
                             (chunkbuffer[6] << 8) |
                             (chunkbuffer[7]));

              chunkbitdepth = chunkbuffer[8];
              chunkcolortype = chunkbuffer[9];
              chunkcompression = chunkbuffer[10];
              chunkfilter = chunkbuffer[11];
              chunkinterlaced = chunkbuffer[12];

              if (mng_putchunk_ihdr (handle, chunkwidth, chunkheight,
                                     chunkbitdepth, chunkcolortype,
                                     chunkcompression, chunkfilter,
                                     chunkinterlaced) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_ihdr() "
                             "in mng_export_image()");
                  fclose (infile);
                  goto err3;
                }
            }
          else if (strncmp (chunkname, "IDAT", 4) == 0)
            {
              if (mng_putchunk_idat (handle, chunksize,
                                     chunkbuffer) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_idat() "
                             "in mng_export_image()");
                  fclose (infile);
                  goto err3;
                }
            }
          else if (strncmp (chunkname, "IEND", 4) == 0)
            {
              if (mng_putchunk_iend (handle) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_iend() "
                             "in mng_export_image()");
                  fclose (infile);
                  goto err3;
                }
            }
          else if (strncmp (chunkname, "PLTE", 4) == 0)
            {
              /* If this frame's palette is the same as the global palette,
               * write a 0-color palette chunk.
               */
              if (mng_putchunk_plte_wrapper (handle,
                                             (layer_has_unique_palette ?
                                              (chunksize / 3) : 0),
                                             chunkbuffer) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_plte() "
                             "in mng_export_image()");
                  fclose (infile);
                  goto err3;
                }
            }
          else if (strncmp (chunkname, "tRNS", 4) == 0)
            {
              if (mng_putchunk_trns_wrapper (handle,
                                             chunksize,
                                             chunkbuffer) != MNG_NOERROR)
                {
                  g_warning ("Unable to mng_putchunk_trns() "
                             "in mng_export_image()");
                  fclose (infile);
                  goto err3;
                }
            }

          if (chunksize > 0)
            g_free (chunkbuffer);

          /* read 4 bytes after the chunk */

          fread (chunkname, 1, 4, infile);
        }

      fclose (infile);
      g_file_delete (temp_file, NULL, NULL);
    }

  if (mng_putchunk_mend (handle) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_putchunk_mend() in mng_export_image()");
      goto err3;
    }

  if (mng_write (handle) != MNG_NOERROR)
    {
      g_warning ("Unable to mng_write() the image in mng_export_image()");
      goto err3;
    }

  ret = TRUE;

 err3:
  mng_cleanup (&handle);
 err2:
  fclose (userdata->fp);
 err:
  g_free (userdata);

  return ret;
}


/* The interactive dialog. */

static gboolean
mng_save_dialog (GimpImage     *image,
                 GimpProcedure *procedure,
                 GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *combo;
  GtkWidget *label;
  GtkWidget *scale;
  gint       num_layers;
  gboolean   run;

  dialog = gimp_export_procedure_dialog_new (GIMP_EXPORT_PROCEDURE (procedure),
                                             GIMP_PROCEDURE_CONFIG (config),
                                             image);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "options-vbox", "interlaced",
                                  "bkgd", "gama", "phys", "time",
                                  NULL);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "options-label", _("MNG Options"),
                                   FALSE, FALSE);
  gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                    "options-frame", "options-label",
                                    FALSE, "options-vbox");

  g_free (gimp_image_get_layers (image, &num_layers));

  if (num_layers == 1)
    {
      GimpParamSpecChoice *cspec;

      cspec =
        GIMP_PARAM_SPEC_CHOICE (g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                              "default-chunks"));

      gimp_choice_set_sensitive (cspec->choice, "all-png", FALSE);
      gimp_choice_set_sensitive (cspec->choice, "all-jng", FALSE);
    }

  combo = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                            "default-chunks",
                                            G_TYPE_NONE);
  gtk_widget_set_margin_bottom (combo, 6);

  combo = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                            "default-dispose",
                                            G_TYPE_NONE);
  gtk_widget_set_margin_bottom (combo, 6);

  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "png-compression", 1.0);
  gtk_widget_set_margin_bottom (scale, 6);

  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "jpeg-quality", 1.0);
  gtk_widget_set_margin_bottom (scale, 6);

  scale = gimp_procedure_dialog_get_scale_entry (GIMP_PROCEDURE_DIALOG (dialog),
                                                 "jpeg-smoothing", 1.0);
  gtk_widget_set_margin_bottom (scale, 6);

  /* MNG Animation Options */
  label = gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                           "milliseconds-label",
                                           _("milliseconds"), FALSE, FALSE);
  gtk_widget_set_margin_start (label, 6);
  hbox = gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                         "animation-box", "default-delay",
                                         "milliseconds-label", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);

  gimp_procedure_dialog_fill_box (GIMP_PROCEDURE_DIALOG (dialog),
                                  "mng-animation-box", "loop",
                                  "animation-box", NULL);
  gimp_procedure_dialog_get_label (GIMP_PROCEDURE_DIALOG (dialog),
                                   "mng-animation-label",
                                   _("Animated MNG Options"), FALSE, FALSE);
  frame = gimp_procedure_dialog_fill_frame (GIMP_PROCEDURE_DIALOG (dialog),
                                            "mng-animation-frame",
                                            "mng-animation-label", FALSE,
                                            "mng-animation-box");

  if (num_layers <= 1)
    {
      gtk_widget_set_sensitive (frame, FALSE);
      gimp_help_set_help_data (frame,
                               _("These options are only available when "
                                 "the exported image has more than one "
                                 "layer. The image you are exporting only has "
                                 "one layer."),
                               NULL);
    }

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              "options-frame", "default-chunks",
                              "default-dispose", "png-compression",
                              "jpeg-quality", "jpeg-smoothing",
                              "mng-animation-frame", NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
