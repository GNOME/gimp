/*
 * GFLI 1.3
 *
 * A gimp plug-in to read and write FLI and FLC movies.
 *
 * Copyright (C) 1998 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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
#define PLUG_IN_ROLE   "gimp-file-fli"


typedef struct _Fli      Fli;
typedef struct _FliClass FliClass;

struct _Fli
{
  GimpPlugIn      parent_instance;
};

struct _FliClass
{
  GimpPlugInClass parent_class;
};


#define FLI_TYPE  (fli_get_type ())
#define FLI (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLI_TYPE, Fli))

GType                   fli_get_type         (void) G_GNUC_CONST;

static GList          * fli_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * fli_create_procedure (GimpPlugIn           *plug_in,
                                              const gchar          *name);

static GimpValueArray * fli_load             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * fli_save             (GimpProcedure        *procedure,
                                              GimpRunMode           run_mode,
                                              GimpImage            *image,
                                              GimpDrawable         *drawable,
                                              GFile                *file,
                                              const GimpValueArray *args,
                                              gpointer              run_data);
static GimpValueArray * fli_info             (GimpProcedure        *procedure,
                                              const GimpValueArray *args,
                                              gpointer              run_data);

static GimpImage      * load_image           (GFile                *file,
                                              gint32                from_frame,
                                              gint32                to_frame,
                                              GError              **error);
static gboolean         load_dialog          (GFile                *file);

static gboolean         save_image           (GFile                *file,
                                              GimpImage            *image,
                                              gint32                from_frame,
                                              gint32                to_frame,
                                              GError              **error);
static gboolean         save_dialog          (GimpImage            *image);

static gboolean         get_info             (GFile                *file,
                                              gint32               *width,
                                              gint32               *height,
                                              gint32               *frames,
                                              GError              **error);


G_DEFINE_TYPE (Fli, fli, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (FLI_TYPE)


static gint32 from_frame;
static gint32 to_frame;


static void
fli_class_init (FliClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = fli_query_procedures;
  plug_in_class->create_procedure = fli_create_procedure;
}

static void
fli_init (Fli *fli)
{
}

static GList *
fli_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));
  list = g_list_append (list, g_strdup (INFO_PROC));

  return list;
}

static GimpProcedure *
fli_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           fli_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, N_("AutoDesk FLIC animation"));

      gimp_procedure_set_documentation (procedure,
                                        "Load FLI-movies",
                                        "This is an experimental plug-in to "
                                        "handle FLI movies",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Jens Ch. Restemeier",
                                      "Jens Ch. Restemeier",
                                      "1997");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-flic");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "fli,flc");

      GIMP_PROC_ARG_INT (procedure, "from-frame",
                         "From frame",
                         "Load beginning from this frame",
                         -1, G_MAXINT, -1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "to-frame",
                         "To frame",
                         "End loading with this frame",
                         -1, G_MAXINT, -1,
                         G_PARAM_READWRITE);
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = gimp_save_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           fli_save, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED, GRAY");

      gimp_procedure_set_menu_label (procedure, N_("AutoDesk FLIC animation"));

      gimp_procedure_set_documentation (procedure,
                                        "Export FLI-movies",
                                        "This is an experimental plug-in to "
                                        "handle FLI movies",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Jens Ch. Restemeier",
                                      "Jens Ch. Restemeier",
                                      "1997");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-flic");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "fli,flc");

      GIMP_PROC_ARG_INT (procedure, "from-frame",
                         "From frame",
                         "Export beginning from this frame",
                         -1, G_MAXINT, -1,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "to-frame",
                         "To frame",
                         "End exporting with this frame",
                         -1, G_MAXINT, -1,
                         G_PARAM_READWRITE);
    }
  else if (! strcmp (name, INFO_PROC))
    {
      procedure = gimp_procedure_new (plug_in, name,
                                      GIMP_PDB_PROC_TYPE_PLUGIN,
                                      fli_info, NULL, NULL);

      gimp_procedure_set_documentation (procedure,
                                        "Get information about a Fli movie",
                                        "This is an experimental plug-in to "
                                        "handle FLI movies",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Jens Ch. Restemeier",
                                      "Jens Ch. Restemeier",
                                      "1997");

      GIMP_PROC_ARG_STRING (procedure, "uri",
                            "URI",
                            "The local file:// URI of the file to get info",
                            NULL,
                            G_PARAM_READWRITE);

      GIMP_PROC_VAL_INT (procedure, "width",
                         "Width",
                         "Width of one frame",
                         0, GIMP_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_VAL_INT (procedure, "height",
                         "Height",
                         "Height of one frame",
                         0, GIMP_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_VAL_INT (procedure, "frames",
                         "Frames",
                         "Number of frames",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
fli_load (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      from_frame = GIMP_VALUES_GET_INT (args, 0);
      to_frame   = GIMP_VALUES_GET_INT (args, 1);
      break;

    case GIMP_RUN_INTERACTIVE:
      if (! load_dialog (file))
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
      break;
    }

  image = load_image (file, from_frame, to_frame,
                      &error);

  if (! image)
    return gimp_procedure_new_return_values (procedure,
                                             GIMP_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static GimpValueArray *
fli_save (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          GFile                *file,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case GIMP_RUN_NONINTERACTIVE:
      from_frame = GIMP_VALUES_GET_INT (args, 0);
      to_frame   = GIMP_VALUES_GET_INT (args, 1);

      if (from_frame > to_frame)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CALLING_ERROR,
                                                 NULL);
      break;

    case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init (PLUG_IN_BINARY);

      export = gimp_export_image (&image, &drawable, "FLI",
                                  GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                  GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                  GIMP_EXPORT_CAN_HANDLE_ALPHA   |
                                  GIMP_EXPORT_CAN_HANDLE_LAYERS);

      if (export == GIMP_EXPORT_CANCEL)
        return gimp_procedure_new_return_values (procedure,
                                                 GIMP_PDB_CANCEL,
                                                 NULL);

      if (! save_dialog (image))
        status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if (! save_image (file, image, from_frame, to_frame,
                        &error))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  return gimp_procedure_new_return_values (procedure, status, error);
}

static GimpValueArray *
fli_info (GimpProcedure        *procedure,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpValueArray *return_vals;
  GFile          *file;
  gint32          width;
  gint32          height;
  gint32          frames;
  GError         *error = NULL;

  file = g_file_new_for_uri (GIMP_VALUES_GET_STRING (args, 0));

  if (! get_info (file, &width, &height, &frames,
                  &error))
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               error);
    }

  return_vals = gimp_procedure_new_return_values (procedure,
                                                  GIMP_PDB_SUCCESS,
                                                  NULL);

  GIMP_VALUES_SET_INT (return_vals, 1, width);
  GIMP_VALUES_SET_INT (return_vals, 2, height);
  GIMP_VALUES_SET_INT (return_vals, 3, frames);

  return return_vals;
}

/*
 * Open FLI animation and return header-info
 */
static gboolean
get_info (GFile   *file,
          gint32  *width,
          gint32  *height,
          gint32  *frames,
          GError **error)
{
  gchar        *filename;
  FILE         *fp;
  s_fli_header  fli_header;

  *width = 0; *height = 0; *frames = 0;

  filename = g_file_get_path (file);
  fp = g_fopen (filename ,"rb");
  g_free (filename);

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  fli_read_header (fp, &fli_header);
  fclose (fp);

  *width  = fli_header.width;
  *height = fli_header.height;
  *frames = fli_header.frames;

  return TRUE;
}

/*
 * load fli animation and store as framestack
 */
static GimpImage *
load_image (GFile   *file,
            gint32   from_frame,
            gint32   to_frame,
            GError **error)
{
  gchar        *filename;
  FILE         *fp;
  GeglBuffer   *buffer;
  GimpImage    *image;
  GimpLayer    *layer;
  guchar       *fb, *ofb, *fb_x;
  guchar        cm[768], ocm[768];
  s_fli_header  fli_header;
  gint          cnt;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  filename = g_file_get_path (file);
  fp = g_fopen (filename ,"rb");
  g_free (file);

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  fli_read_header (fp, &fli_header);
  if (fli_header.magic == NO_HEADER)
    {
      fclose (fp);
      return NULL;
    }
  else
    {
      fseek (fp, 128, SEEK_SET);
    }

  /*
   * Fix parameters
   */
  if ((from_frame==-1) && (to_frame==-1))
    {
      /* to make scripting easier: */
      from_frame=1; to_frame=fli_header.frames;
    }
  if (to_frame < from_frame)
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
      fclose (fp);
      return NULL;
    }
  if (from_frame >= fli_header.frames)
    {
      /* nothing to do ... */
      fclose (fp);
      return NULL;
    }
  if (to_frame > fli_header.frames)
    {
      to_frame = fli_header.frames;
    }

  image = gimp_image_new (fli_header.width, fli_header.height, GIMP_INDEXED);
  gimp_image_set_file (image, file);

  fb  = g_malloc (fli_header.width * fli_header.height);
  ofb = g_malloc (fli_header.width * fli_header.height);

  /*
   * Skip to the beginning of requested frames:
   */
  for (cnt = 1; cnt < from_frame; cnt++)
    {
      fli_read_frame (fp, &fli_header, ofb, ocm, fb, cm);
      memcpy (ocm, cm, 768);
      fb_x = fb; fb = ofb; ofb = fb_x;
    }
  /*
   * Load range
   */
  for (cnt = from_frame; cnt <= to_frame; cnt++)
    {
      gchar *name_buf = g_strdup_printf (_("Frame (%i)"), cnt);

      layer = gimp_layer_new (image, name_buf,
                              fli_header.width, fli_header.height,
                              GIMP_INDEXED_IMAGE,
                              100,
                              gimp_image_get_default_new_layer_mode (image));
      g_free (name_buf);

      buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

      fli_read_frame (fp, &fli_header, ofb, ocm, fb, cm);

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0,
                                               fli_header.width,
                                               fli_header.height), 0,
                       NULL, fb, GEGL_AUTO_ROWSTRIDE);

      g_object_unref (buffer);

      if (cnt > 0)
        gimp_layer_add_alpha (layer);

      gimp_image_insert_layer (image, layer, NULL, 0);

      if (cnt < to_frame)
        {
          memcpy (ocm, cm, 768);
          fb_x = fb; fb = ofb; ofb = fb_x;
        }

      gimp_progress_update ((double) cnt + 1 / (double)(to_frame - from_frame));
    }

  gimp_image_set_colormap (image, cm, 256);

  fclose (fp);

  g_free (fb);
  g_free (ofb);

  gimp_progress_update (1.0);

  return image;
}


#define MAXDIFF 195075    /*  3 * SQR (255) + 1  */

/*
 * get framestack and store as fli animation
 * (some code was taken from the GIF plugin.)
 */
static gboolean
save_image (GFile      *file,
            GimpImage  *image,
            gint32      from_frame,
            gint32      to_frame,
            GError    **error)
{
  gchar        *filename;
  FILE         *fp;
  GList        *framelist;
  GList        *iter;
  gint          nframes;
  gint          colors, i;
  guchar       *cmap;
  guchar        bg;
  guchar        red, green, blue;
  gint          diff, sum, max;
  gint          offset_x, offset_y, xc, yc, xx, yy;
  guint         rows, cols, bytes;
  guchar       *src_row;
  guchar       *fb, *ofb;
  guchar        cm[768];
  GimpRGB       background;
  s_fli_header  fli_header;
  gint          cnt;

  framelist = gimp_image_list_layers (image);
  framelist = g_list_reverse (framelist);
  nframes = g_list_length (framelist);

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

  switch (gimp_image_base_type (image))
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
      cmap = gimp_image_get_colormap (image, &colors);
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
      g_message (_("Sorry, I can export only INDEXED and GRAY images."));
      return FALSE;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  /*
   * First build the fli header.
   */
  fli_header.filesize = 0;  /* will be fixed when writing the header */
  fli_header.frames   = 0;  /* will be fixed during the write */
  fli_header.width    = gimp_image_width (image);
  fli_header.height   = gimp_image_height (image);

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
  fli_header.created  = 0;  /* program ID. not necessary... */
  fli_header.updated  = 0;  /* date in MS-DOS format. ignore...*/
  fli_header.aspect_x = 1;  /* aspect ratio. Will be added as soon.. */
  fli_header.aspect_y = 1;  /* ... as GIMP supports it. */
  fli_header.oframe1  = fli_header.oframe2 = 0; /* will be fixed during the write */

  filename = g_file_get_path (file);
  fp = g_fopen (filename , "wb");
  g_free (filename);

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }
  fseek (fp, 128, SEEK_SET);

  fb = g_malloc (fli_header.width * fli_header.height);
  ofb = g_malloc (fli_header.width * fli_header.height);

  /* initialize with bg color */
  memset (fb, bg, fli_header.width * fli_header.height);

  /*
   * Now write all frames
   */
  for (iter = g_list_nth (framelist, from_frame - 1), cnt = from_frame;
       iter && cnt <= to_frame;
       iter = g_list_next (iter), cnt++)
    {
      GimpDrawable *drawable = iter->data;
      GeglBuffer   *buffer;
      const Babl   *format = NULL;

      buffer = gimp_drawable_get_buffer (drawable);

      if (gimp_drawable_is_gray (drawable))
        {
          if (gimp_drawable_has_alpha (drawable))
            format = babl_format ("Y' u8");
          else
            format = babl_format ("Y'A u8");
        }
      else
        {
          format = gegl_buffer_get_format (buffer);
        }

      cols = gegl_buffer_get_width  (buffer);
      rows = gegl_buffer_get_height (buffer);

      gimp_drawable_offsets (drawable, &offset_x, &offset_y);

      bytes = babl_format_get_bytes_per_pixel (format);

      src_row = g_malloc (cols * bytes);

      /* now paste it into the framebuffer, with the necessary offset */
      for (yc = 0, yy = offset_y; yc < rows; yc++, yy++)
        {
          if (yy >= 0 && yy < fli_header.height)
            {
              gegl_buffer_get (buffer, GEGL_RECTANGLE (0, yc, cols, 1), 1.0,
                               format, src_row,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

              for (xc = 0, xx = offset_x; xc < cols; xc++, xx++)
                {
                  if (xx >= 0 && xx < fli_header.width)
                    fb[yy * fli_header.width + xx] = src_row[xc * bytes];
                }
            }
        }

      g_free (src_row);
      g_object_unref (buffer);

      /* save the frame */
      if (cnt > from_frame)
        {
          /* save frame, allow all codecs */
          fli_write_frame (fp, &fli_header, ofb, cm, fb, cm, W_ALL);
        }
      else
        {
          /* save first frame, no delta information, allow all codecs */
          fli_write_frame (fp, &fli_header, NULL, NULL, fb, cm, W_ALL);
        }

      if (cnt < to_frame)
        memcpy (ofb, fb, fli_header.width * fli_header.height);

      gimp_progress_update ((double) cnt + 1 / (double)(to_frame - from_frame));
    }

  /*
   * finish fli
   */
  fli_write_header (fp, &fli_header);
  fclose (fp);

  g_free (fb);
  g_free (ofb);
  g_free (framelist);

  gimp_progress_update (1.0);

  return TRUE;
}

/*
 * Dialogs for interactive usage
 */
static gboolean
load_dialog (GFile *file)
{
  GtkWidget     *dialog;
  GtkWidget     *grid;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  gint32         width, height, nframes;
  gboolean       run;

  get_info (file, &width, &height, &nframes, NULL);

  from_frame = 1;
  to_frame   = nframes;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_dialog_new (_("GFLI 1.3 - Load framestack"), PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Open"),   GTK_RESPONSE_OK,

                            NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /*
   * Maybe I add on-the-fly RGB conversion, to keep palettechanges...
   * But for now you can set a start- and a end-frame:
   */
  adj = gtk_adjustment_new (from_frame, 1, nframes, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            C_("frame-range", "_From:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &from_frame);

  adj = gtk_adjustment_new (to_frame, 1, nframes, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            C_("frame-range", "_To:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &to_frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
save_dialog (GimpImage *image)
{
  GtkWidget     *dialog;
  GtkWidget     *grid;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  gint           nframes;
  gboolean       run;

  g_free (gimp_image_get_layers (image, &nframes));

  from_frame = 1;
  to_frame   = nframes;

  dialog = gimp_export_dialog_new (_("GFLI 1.3"), PLUG_IN_BINARY, SAVE_PROC);

  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /*
   * Maybe I add on-the-fly RGB conversion, to keep palettechanges...
   * But for now you can set a start- and a end-frame:
   */
  adj = gtk_adjustment_new (from_frame, 1, nframes, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            C_("frame-range", "_From:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &from_frame);

  adj = gtk_adjustment_new (to_frame, 1, nframes, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adj, 1, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            C_("frame-range", "_To:"), 0.0, 0.5,
                            spinbutton, 1);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &to_frame);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}
