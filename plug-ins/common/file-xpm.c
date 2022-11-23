/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

/* XPM plugin version 1.2.7 */

/*
1.2.7 fixes saving unused transparency (bug #4560)

1.2.6 fixes crash when saving indexed images (bug #109567)

1.2.5 only creates a "None" color entry if the image has alpha (bug #108034)

1.2.4 displays an error message if saving fails (bug #87588)

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

#include <string.h>

#include <glib/gstdio.h>

#include <gdk/gdk.h>          /* For GDK_WINDOWING_WIN32 */

#ifndef GDK_WINDOWING_X11
#ifndef XPM_NO_X
#define XPM_NO_X
#endif
#else
#include <X11/Xlib.h>
#endif

#include <X11/xpm.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define LOAD_PROC      "file-xpm-load"
#define SAVE_PROC      "file-xpm-save"
#define PLUG_IN_BINARY "file-xpm"
#define PLUG_IN_ROLE   "ligma-file-xpm"
#define SCALE_WIDTH    125


typedef struct
{
  guchar r;
  guchar g;
  guchar b;
} rgbkey;


typedef struct _Xpm      Xpm;
typedef struct _XpmClass XpmClass;

struct _Xpm
{
  LigmaPlugIn      parent_instance;
};

struct _XpmClass
{
  LigmaPlugInClass parent_class;
};


#define XPM_TYPE  (xpm_get_type ())
#define XPM (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), XPM_TYPE, Xpm))

GType                   xpm_get_type         (void) G_GNUC_CONST;

static GList          * xpm_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * xpm_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * xpm_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * xpm_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static LigmaImage      * load_image           (GFile               *file,
                                              GError              **error);
static guchar         * parse_colors         (XpmImage             *xpm_image);
static void             parse_image          (LigmaImage            *image,
                                              XpmImage             *xpm_image,
                                              guchar               *cmap);
static gboolean         save_image           (GFile                *file,
                                              LigmaImage            *image,
                                              LigmaDrawable         *drawable,
                                              GObject              *config,
                                              GError              **error);
static gboolean         save_dialog          (LigmaProcedure        *procedure,
                                              GObject              *config);


G_DEFINE_TYPE (Xpm, xpm, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (XPM_TYPE)
DEFINE_STD_SET_I18N


static const gchar linenoise [] =
" .+@#$%&*=-;>,')!~{]^/(_:<[}|1234567890abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ`";

/*  whether the image is color or not.  global so I only have to pass
 *  one user value to the GHFunc
 */
static gboolean   color;

/*  bytes per pixel.  global so I only have to pass one user value
 *  to the GHFunc
 */
static gint       cpp;


static void
xpm_class_init (XpmClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = xpm_query_procedures;
  plug_in_class->create_procedure = xpm_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
xpm_init (Xpm *xpm)
{
}

static GList *
xpm_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));

  return list;
}

static LigmaProcedure *
xpm_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           xpm_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("X PixMap image"));

      ligma_procedure_set_documentation (procedure,
                                        "Load files in XPM (X11 Pixmap) format.",
                                        "Load files in XPM (X11 Pixmap) format. "
                                        "XPM is a portable image format "
                                        "designed to be included in C source "
                                        "code. XLib provides utility functions "
                                        "to read this format. Newer code should "
                                        "however be using gdk-pixbuf-csource "
                                        "instead. XPM supports colored images, "
                                        "unlike the XBM format which XPM was "
                                        "designed to replace.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis & "
                                      "Ray Lehtiniemi",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-pixmap");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "xpm");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0, string,/*\\040XPM\\040*/");
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           xpm_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");

      ligma_procedure_set_menu_label (procedure, _("X PixMap image"));

      ligma_procedure_set_documentation (procedure,
                                        "Export files in XPM (X11 Pixmap) format.",
                                        "Export files in XPM (X11 Pixmap) format. "
                                        "XPM is a portable image format "
                                        "designed to be included in C source "
                                        "code. XLib provides utility functions "
                                        "to read this format. Newer code should "
                                        "however be using gdk-pixbuf-csource "
                                        "instead. XPM supports colored images, "
                                        "unlike the XBM format which XPM was "
                                        "designed to replace.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis & "
                                      "Ray Lehtiniemi & Nathan Summers",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-pixmap");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "xpm");

      LIGMA_PROC_ARG_INT (procedure, "threshold",
                         "Threshold",
                         "Alpha threshold",
                         0, 255, 127,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
xpm_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
xpm_save (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaPDBStatusType    status = LIGMA_PDB_SUCCESS;
  LigmaExportReturn     export = LIGMA_EXPORT_CANCEL;
  GError              *error = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "XPM",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB     |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY    |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("XPM format does not support multiple layers."));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (ligma_drawable_has_alpha (drawables[0]))
        if (! save_dialog (procedure, G_OBJECT (config)))
          status = LIGMA_PDB_CANCEL;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      if (! save_image (file, image, drawables[0], G_OBJECT (config),
                        &error))
        {
          status = LIGMA_PDB_EXECUTION_ERROR;
        }
    }

  ligma_procedure_config_end_run (config, status);
  g_object_unref (config);

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}

static LigmaImage *
load_image (GFile   *file,
            GError  **error)
{
  XpmImage   xpm_image;
  guchar    *cmap;
  LigmaImage *image;

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  /* read the raw file */
  switch (XpmReadFileToXpmImage (g_file_peek_path (file), &xpm_image, NULL))
    {
    case XpmSuccess:
      break;

    case XpmOpenFailed:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error opening file '%s'"),
                   ligma_file_get_utf8_name (file));
      return NULL;

    case XpmFileInvalid:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", _("XPM file invalid"));
      return NULL;

    default:
      return NULL;
    }

  cmap = parse_colors (&xpm_image);

  image = ligma_image_new (xpm_image.width,
                          xpm_image.height,
                          LIGMA_RGB);

  ligma_image_set_file (image, file);

  /* fill it */
  parse_image (image, &xpm_image, cmap);

  g_free (cmap);

  return image;
}

static guchar *
parse_colors (XpmImage *xpm_image)
{
#ifndef XPM_NO_X
  Display  *display;
  Colormap  colormap;
#endif
  gint      i, j;
  guchar   *cmap;

#ifndef XPM_NO_X
  /* open the display and get the default color map */
  display  = XOpenDisplay (NULL);
  if (display == NULL)
    g_printerr ("Could not open display\n");

  colormap = DefaultColormap (display, DefaultScreen (display));
#endif

  /* alloc a buffer to hold the parsed colors */
  cmap = g_new0 (guchar, 4 * xpm_image->ncolors);

  /* parse each color in the file */
  for (i = 0, j = 0; i < xpm_image->ncolors; i++)
    {
      gchar     *colorspec = "None";
      XpmColor *xpm_color;
#ifndef XPM_NO_X
      XColor    xcolor;
#else
      GdkColor  xcolor;
#endif

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

      /* parse if it's not transparent */
      if (strcmp (colorspec, "None") != 0)
        {
#ifndef XPM_NO_X
          XParseColor (display, colormap, colorspec, &xcolor);
#else
          gdk_color_parse (colorspec, &xcolor);
#endif
          cmap[j++] = xcolor.red >> 8;
          cmap[j++] = xcolor.green >> 8;
          cmap[j++] = xcolor.blue >> 8;
          cmap[j++] = ~0;
        }
      else
        {
          j += 4;
        }
    }
#ifndef XPM_NO_X
  XCloseDisplay (display);
#endif
  return cmap;
}

static void
parse_image (LigmaImage *image,
             XpmImage  *xpm_image,
             guchar    *cmap)
{
  GeglBuffer *buffer;
  gint        tile_height;
  gint        scanlines;
  gint        val;
  guchar     *buf;
  guchar     *dest;
  guint      *src;
  LigmaLayer  *layer;
  gint        i;

  layer = ligma_layer_new (image,
                          _("Color"),
                          xpm_image->width,
                          xpm_image->height,
                          LIGMA_RGBA_IMAGE,
                          100,
                          ligma_image_get_default_new_layer_mode (image));

  ligma_image_insert_layer (image, layer, NULL, 0);

  buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

  tile_height = ligma_tile_height ();

  buf  = g_new (guchar, tile_height * xpm_image->width * 4);

  src  = xpm_image->data;
  for (i = 0; i < xpm_image->height; i += tile_height)
    {
      gint j;

      dest = buf;
      scanlines = MIN (tile_height, xpm_image->height - i);
      j = scanlines * xpm_image->width;
      while (j--)
        {
          {
            val = *(src++) * 4;
            *(dest)   = cmap[val];
            *(dest+1) = cmap[val+1];
            *(dest+2) = cmap[val+2];
            *(dest+3) = cmap[val+3];
            dest += 4;
          }

          if ((j % 100) == 0)
            ligma_progress_update ((double) i / (double) xpm_image->height);
        }

      gegl_buffer_set (buffer,
                       GEGL_RECTANGLE (0, i, xpm_image->width, scanlines), 0,
                       NULL, buf, GEGL_AUTO_ROWSTRIDE);
    }

  g_free (buf);
  g_object_unref (buffer);

  ligma_progress_update (1.0);
}

static guint
rgbhash (rgbkey *c)
{
  return ((guint)c->r) ^ ((guint)c->g) ^ ((guint)c->b);
}

static guint
compare (rgbkey *c1,
         rgbkey *c2)
{
  return (c1->r == c2->r) && (c1->g == c2->g) && (c1->b == c2->b);
}

static void
set_XpmImage (XpmColor *array,
              guint     index,
              gchar    *colorstring)
{
  gchar *p;
  gint   i, charnum, indtemp;

  indtemp=index;
  array[index].string = p = g_new (gchar, cpp+1);

  /*convert the index number to base sizeof(linenoise)-1 */
  for (i = 0; i < cpp; ++i)
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
    }
  else
    {
      array[index].c_color = NULL;
      array[index].g_color = colorstring;
    }
}

static void
create_colormap_from_hash (gpointer gkey,
                           gpointer value,
                           gpointer user_data)
{
  rgbkey *key    = gkey;
  gchar  *string = g_new (gchar, 8);

  sprintf (string, "#%02X%02X%02X", (int)key->r, (int)key->g, (int)key->b);
  set_XpmImage (user_data, *((int *) value), string);
}

static void
decrement_hash_values (gpointer gkey,
                       gpointer value,
                       gpointer user_data)
{
  --(*((guint*) value));
}

static gboolean
save_image (GFile         *file,
            LigmaImage     *image,
            LigmaDrawable  *drawable,
            GObject       *config,
            GError       **error)
{
  GeglBuffer *buffer;
  const Babl *format;
  gint        width;
  gint        height;
  gint        ncolors = 1;
  gint       *indexno;
  gboolean    indexed;
  gboolean    alpha;
  gboolean    alpha_used = FALSE;
  XpmColor   *colormap;
  XpmImage   *xpm_image;
  guint      *ibuff   = NULL;
  guchar     *buf;
  guchar     *data;
  GHashTable *hash = NULL;
  gint        i, j, k;
  gint        threshold;
  gboolean    success = FALSE;

  g_object_get (config,
                "threshold", &threshold,
                NULL);

  buffer = ligma_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  alpha   = ligma_drawable_has_alpha (drawable);
  color   = ! ligma_drawable_is_gray (drawable);
  indexed = ligma_drawable_is_indexed (drawable);

  switch (ligma_drawable_type (drawable))
    {
    case LIGMA_RGB_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case LIGMA_RGBA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    case LIGMA_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case LIGMA_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case LIGMA_INDEXED_IMAGE:
    case LIGMA_INDEXEDA_IMAGE:
      format = gegl_buffer_get_format (buffer);
      break;

    default:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported drawable type"));
      g_object_unref (buffer);
      return FALSE;
    }

  /* allocate buffer making the assumption that ibuff is 32 bit aligned... */
  ibuff = g_new (guint, width * height);

  hash = g_hash_table_new ((GHashFunc) rgbhash, (GCompareFunc) compare);

  ligma_progress_init_printf (_("Exporting '%s'"),
                             ligma_file_get_utf8_name (file));

  ncolors = alpha ? 1 : 0;

  /* allocate a pixel region to work with */
  buf = g_new (guchar,
               ligma_tile_height () * width *
               babl_format_get_bytes_per_pixel (format));

  /* process each row of tiles */
  for (i = 0; i < height; i += ligma_tile_height ())
    {
      gint scanlines;

      /* read the next row of tiles */
      scanlines = MIN (ligma_tile_height(), height - i);

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, i, width, scanlines), 1.0,
                       format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      data = buf;

      /* process each pixel row */
      for (j = 0; j < scanlines; j++)
        {
          /* go to the start of this row in each image */
          guint *idata = ibuff + (i+j) * width;

          /* do each pixel in the row */
          for (k = 0; k < width; k++)
            {
              rgbkey *key = g_new (rgbkey, 1);
              guchar  a;

              /* get pixel data */
              key->r = *(data++);
              key->g = color && !indexed ? *(data++) : key->r;
              key->b = color && !indexed ? *(data++) : key->r;
              a = alpha ? *(data++) : 255;

              if (a < threshold)
                {
                  *(idata++) = 0;
                  alpha_used = TRUE;
                }
              else
                {
                  if (indexed)
                    {
                      *(idata++) = (key->r) + (alpha ? 1 : 0);
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
          ligma_progress_update ((gdouble) (i+j) / (gdouble) height);
        }
    }

  g_free (buf);

  /* remove alpha if not actually used */
  if (alpha && !alpha_used)
    {
      gint i;
      --ncolors;
      for (i = 0; i < width * height; ++i)
        --ibuff[i];

      g_hash_table_foreach (hash, decrement_hash_values, NULL);
    }

  if (indexed)
    {
      guchar *cmap = ligma_image_get_colormap (image, &ncolors);
      guchar *c;

      c = cmap;

      if (alpha_used)
        ncolors++;

      colormap = g_new (XpmColor, ncolors);
      cpp =
        1 + (gdouble) log (ncolors) / (gdouble) log (sizeof (linenoise) - 1.0);

      if (alpha_used)
        set_XpmImage (colormap, 0, "None");

      for (i = alpha_used ? 1 : 0; i < ncolors; i++)
        {
          gchar *string;
          guchar r, g, b;

          r = *c++;
          g = *c++;
          b = *c++;

          string = g_new (gchar, 8);
          sprintf (string, "#%02X%02X%02X", (int)r, (int)g, (int)b);
          set_XpmImage (colormap, i, string);
        }

      g_free (cmap);
    }
  else
    {
      colormap = g_new (XpmColor, ncolors);
      cpp =
        1 + (gdouble) log (ncolors) / (gdouble) log (sizeof (linenoise) - 1.0);

      if (alpha_used)
        set_XpmImage (colormap, 0, "None");

      g_hash_table_foreach (hash, create_colormap_from_hash, colormap);
    }

  xpm_image = g_new (XpmImage, 1);

  xpm_image->width      = width;
  xpm_image->height     = height;
  xpm_image->ncolors    = ncolors;
  xpm_image->cpp        = cpp;
  xpm_image->colorTable = colormap;
  xpm_image->data       = ibuff;

  /* do the save */
  switch (XpmWriteFileFromXpmImage (g_file_peek_path (file), xpm_image, NULL))
    {
    case XpmSuccess:
      success = TRUE;
      break;

    case XpmOpenFailed:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Error opening file '%s'"),
                   ligma_file_get_utf8_name (file));
      break;

    case XpmFileInvalid:
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "%s", _("XPM file invalid"));
      break;

    default:
      break;
    }

  g_object_unref (buffer);
  g_free (ibuff);

  if (hash)
    g_hash_table_destroy (hash);

  ligma_progress_update (1.0);

  return success;
}

static gboolean
save_dialog (LigmaProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *scale;
  gboolean   run;

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Export Image as XPM"));

  scale = ligma_prop_scale_entry_new (config, "threshold", NULL, 1.0, FALSE, 0, 0);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      scale, TRUE, TRUE, 6);
  gtk_widget_show (scale);
  gtk_widget_show (dialog);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
