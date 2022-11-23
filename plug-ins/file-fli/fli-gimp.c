/*
 * GFLI 1.3
 *
 * A ligma plug-in to read and write FLI and FLC movies.
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "fli.h"

#include "libligma/stdplugins-intl.h"


#define LOAD_PROC      "file-fli-load"
#define SAVE_PROC      "file-fli-save"
#define INFO_PROC      "file-fli-info"
#define PLUG_IN_BINARY "file-fli"
#define PLUG_IN_ROLE   "ligma-file-fli"


typedef struct _Fli      Fli;
typedef struct _FliClass FliClass;

struct _Fli
{
  LigmaPlugIn      parent_instance;
};

struct _FliClass
{
  LigmaPlugInClass parent_class;
};


#define FLI_TYPE  (fli_get_type ())
#define FLI (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), FLI_TYPE, Fli))

GType                   fli_get_type         (void) G_GNUC_CONST;

static GList          * fli_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * fli_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * fli_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * fli_save             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * fli_info             (LigmaProcedure        *procedure,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static LigmaImage      * load_image           (GFile                *file,
                                              GObject              *config,
                                              GError              **error);
static gboolean         load_dialog          (GFile                *file,
                                              LigmaProcedure        *procedure,
                                              GObject              *config);

static gboolean         save_image           (GFile                *file,
                                              LigmaImage            *image,
                                              GObject              *config,
                                              GError              **error);
static gboolean         save_dialog          (LigmaImage            *image,
                                              LigmaProcedure        *procedure,
                                              GObject              *config);

static gboolean         get_info             (GFile                *file,
                                              gint32               *width,
                                              gint32               *height,
                                              gint32               *frames,
                                              GError              **error);


G_DEFINE_TYPE (Fli, fli, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (FLI_TYPE)
DEFINE_STD_SET_I18N


static void
fli_class_init (FliClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = fli_query_procedures;
  plug_in_class->create_procedure = fli_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
fli_init (Fli *fli)
{
}

static GList *
fli_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (SAVE_PROC));
  list = g_list_append (list, g_strdup (INFO_PROC));

  return list;
}

static LigmaProcedure *
fli_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           fli_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("AutoDesk FLIC animation"));

      ligma_procedure_set_documentation (procedure,
                                        "Load FLI-movies",
                                        "This is an experimental plug-in to "
                                        "handle FLI movies",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Jens Ch. Restemeier",
                                      "Jens Ch. Restemeier",
                                      "1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-flic");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "fli,flc");

      LIGMA_PROC_ARG_INT (procedure, "from-frame",
                         "From frame",
                         "Load beginning from this frame",
                         -1, G_MAXINT, -1,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "to-frame",
                         "To frame",
                         "End loading with this frame",
                         -1, G_MAXINT, -1,
                         G_PARAM_READWRITE);
    }
  else if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           fli_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "INDEXED, GRAY");

      ligma_procedure_set_menu_label (procedure, _("AutoDesk FLIC animation"));

      ligma_procedure_set_documentation (procedure,
                                        "Export FLI-movies",
                                        "This is an experimental plug-in to "
                                        "handle FLI movies",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Jens Ch. Restemeier",
                                      "Jens Ch. Restemeier",
                                      "1997");

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-flic");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "fli,flc");

      LIGMA_PROC_ARG_INT (procedure, "from-frame",
                         "_From:",
                         "Export beginning from this frame",
                         -1, G_MAXINT, -1,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "to-frame",
                         "_To:",
                         "End exporting with this frame "
                         "(or -1 for all frames)",
                         -1, G_MAXINT, -1,
                         G_PARAM_READWRITE);
    }
  else if (! strcmp (name, INFO_PROC))
    {
      procedure = ligma_procedure_new (plug_in, name,
                                      LIGMA_PDB_PROC_TYPE_PLUGIN,
                                      fli_info, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Get information about a Fli movie",
                                        "This is an experimental plug-in to "
                                        "handle FLI movies",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Jens Ch. Restemeier",
                                      "Jens Ch. Restemeier",
                                      "1997");

      LIGMA_PROC_ARG_FILE (procedure, "file",
                          "File",
                          "The local file to get info about",
                          G_PARAM_READWRITE);

      LIGMA_PROC_VAL_INT (procedure, "width",
                         "Width",
                         "Width of one frame",
                         0, LIGMA_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_VAL_INT (procedure, "height",
                         "Height",
                         "Height of one frame",
                         0, LIGMA_MAX_IMAGE_SIZE, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_VAL_INT (procedure, "frames",
                         "Frames",
                         "Number of frames",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
fli_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaProcedureConfig *config;
  LigmaValueArray      *return_vals;
  LigmaImage           *image;
  GError              *error = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, NULL, run_mode, args);

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! load_dialog (file, procedure, G_OBJECT (config)))
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
    }

  image = load_image (file, G_OBJECT (config),
                      &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  ligma_procedure_config_end_run (config, LIGMA_PDB_SUCCESS);
  g_object_unref (config);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
fli_save (LigmaProcedure        *procedure,
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
  GError              *error  = NULL;

  gegl_init (NULL, NULL);

  config = ligma_procedure_create_config (procedure);
  ligma_procedure_config_begin_run (config, image, run_mode, args);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "FLI",
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED |
                                  LIGMA_EXPORT_CAN_HANDLE_GRAY    |
                                  LIGMA_EXPORT_CAN_HANDLE_ALPHA   |
                                  LIGMA_EXPORT_CAN_HANDLE_LAYERS);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      if (! save_dialog (image, procedure, G_OBJECT (config)))
        status = LIGMA_PDB_CANCEL;
    }

  if (status == LIGMA_PDB_SUCCESS)
    {
      if (! save_image (file, image, G_OBJECT (config),
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

static LigmaValueArray *
fli_info (LigmaProcedure        *procedure,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray *return_vals;
  GFile          *file;
  gint32          width;
  gint32          height;
  gint32          frames;
  GError         *error = NULL;

  file = LIGMA_VALUES_GET_FILE (args, 0);

  if (! get_info (file, &width, &height, &frames,
                  &error))
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               error);
    }

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_INT (return_vals, 1, width);
  LIGMA_VALUES_SET_INT (return_vals, 2, height);
  LIGMA_VALUES_SET_INT (return_vals, 3, frames);

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
  FILE         *fp;
  s_fli_header  fli_header;

  *width = 0; *height = 0; *frames = 0;

  fp = g_fopen (g_file_peek_path (file),"rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  if (! fli_read_header (fp, &fli_header, error))
    {
      fclose (fp);
      return FALSE;
    }
  fclose (fp);

  *width  = fli_header.width;
  *height = fli_header.height;
  *frames = fli_header.frames;

  return TRUE;
}

/*
 * load fli animation and store as framestack
 */
static LigmaImage *
load_image (GFile    *file,
            GObject  *config,
            GError  **error)
{
  FILE         *fp;
  GeglBuffer   *buffer;
  LigmaImage    *image;
  LigmaLayer    *layer;
  guchar       *fb, *ofb, *fb_x;
  guchar        cm[768], ocm[768];
  s_fli_header  fli_header;
  gint          cnt;
  gint          from_frame;
  gint          to_frame;

  g_object_get (config,
                "from-frame", &from_frame,
                "to-frame",   &to_frame,
                NULL);

  ligma_progress_init_printf (_("Opening '%s'"),
                             ligma_file_get_utf8_name (file));

  fp = g_fopen (g_file_peek_path (file) ,"rb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  if (! fli_read_header (fp, &fli_header, error))
    {
      fclose (fp);
      return NULL;
    }

  fseek (fp, 128, SEEK_SET);

  /*
   * Fix parameters
   */
  if ((from_frame == -1) && (to_frame == -1))
    {
      /* to make scripting easier: */
      from_frame = 1;
      to_frame   = fli_header.frames;
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

  if (from_frame > fli_header.frames)
    {
      /* nothing to do ... */
      fclose (fp);
      return NULL;
    }

  if (to_frame > fli_header.frames)
    {
      to_frame = fli_header.frames;
    }

  image = ligma_image_new (fli_header.width, fli_header.height, LIGMA_INDEXED);
  ligma_image_set_file (image, file);

  fb  = g_malloc (fli_header.width * fli_header.height);
  ofb = g_malloc (fli_header.width * fli_header.height);

  /*
   * Skip to the beginning of requested frames:
   */
  for (cnt = 1; cnt < from_frame; cnt++)
    {
      if (! fli_read_frame (fp, &fli_header, ofb, ocm, fb, cm, error))
        {
          fclose (fp);
          g_free (fb);
          g_free (ofb);
          return FALSE;
        }
      memcpy (ocm, cm, 768);
      fb_x = fb; fb = ofb; ofb = fb_x;
    }
  /*
   * Load range
   */
  for (cnt = from_frame; cnt <= to_frame; cnt++)
    {
      gchar *name_buf = g_strdup_printf (_("Frame %d (%ums)"), cnt, fli_header.speed);

      g_debug ("Loading frame %d", cnt);

      layer = ligma_layer_new (image, name_buf,
                              fli_header.width, fli_header.height,
                              LIGMA_INDEXED_IMAGE,
                              100,
                              ligma_image_get_default_new_layer_mode (image));
      g_free (name_buf);

      if (! fli_read_frame (fp, &fli_header, ofb, ocm, fb, cm, error))
        {
          /* Since some of the frames could have been read, let's not make
           * this fatal, unless it's the first frame. */
          if (error && *error)
            {
              ligma_item_delete (LIGMA_ITEM(layer));
              if (cnt > from_frame)
                {
                  g_warning ("Failed to read frame %d. Possibly corrupt animation.\n%s",
                              cnt, (*error)->message);
                  g_clear_error (error);
                }
              else
                {
                  ligma_image_delete (image);
                  g_prefix_error (error, _("Failed to read frame %d. Possibly corrupt animation.\n"), cnt);
                  fclose (fp);
                  g_free (fb);
                  g_free (ofb);
                  return FALSE;
                }
            }

          break;
        }

      buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (layer));

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0,
                                               fli_header.width,
                                               fli_header.height), 0,
                       NULL, fb, GEGL_AUTO_ROWSTRIDE);

      g_object_unref (buffer);

      if (cnt > 0)
        ligma_layer_add_alpha (layer);

      ligma_image_insert_layer (image, layer, NULL, 0);

      if (cnt < to_frame)
        {
          memcpy (ocm, cm, 768);
          fb_x = fb; fb = ofb; ofb = fb_x;
        }

      if (to_frame > from_frame)
        ligma_progress_update ((double) cnt + 1 / (double)(to_frame - from_frame));
    }

  ligma_image_set_colormap (image, cm, 256);

  fclose (fp);

  g_free (fb);
  g_free (ofb);

  ligma_progress_update (1.0);

  return image;
}


#define MAXDIFF 195075    /*  3 * SQR (255) + 1  */

/*
 * get framestack and store as fli animation
 * (some code was taken from the GIF plugin.)
 */
static gboolean
save_image (GFile      *file,
            LigmaImage  *image,
            GObject    *config,
            GError    **error)
{
  FILE         *fp;
  GList        *framelist;
  GList        *iter;
  gint          n_frames;
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
  LigmaRGB       background;
  s_fli_header  fli_header;
  gint          cnt;
  gint          from_frame;
  gint          to_frame;
  gboolean      write_ok = FALSE;

  g_object_get (config,
                "from-frame", &from_frame,
                "to-frame",   &to_frame,
                NULL);

  framelist = ligma_image_list_layers (image);
  framelist = g_list_reverse (framelist);
  n_frames  = g_list_length (framelist);

  if ((from_frame == -1) && (to_frame == -1))
    {
      /* to make scripting easier: */
      from_frame = 1;
      to_frame   = n_frames;
    }
  if (to_frame < from_frame)
    {
      to_frame = n_frames;
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
  if (from_frame > n_frames)
    {
      /* nothing to do ... */
      return FALSE;
    }
  if (to_frame > n_frames)
    {
      to_frame = n_frames;
    }

  ligma_context_get_background (&background);
  ligma_rgb_get_uchar (&background, &red, &green, &blue);

  switch (ligma_image_get_base_type (image))
    {
    case LIGMA_GRAY:
      /* build grayscale palette */
      for (i = 0; i < 256; i++)
        {
          cm[i*3+0] = cm[i*3+1] = cm[i*3+2] = i;
        }
      bg = LIGMA_RGB_LUMINANCE (red, green, blue) + 0.5;
      break;

    case LIGMA_INDEXED:
      max = MAXDIFF;
      bg = 0;
      cmap = ligma_image_get_colormap (image, &colors);
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
      /* Not translating this, since we should never get this error, unless
       * someone messed up setting supported image types. */
      g_set_error (error, LIGMA_PLUG_IN_ERROR, 0,
                   "Exporting of RGB images is not supported!");
      return FALSE;
    }

  ligma_progress_init_printf (_("Exporting '%s'"),
                             ligma_file_get_utf8_name (file));

  /*
   * First build the fli header.
   */
  fli_header.filesize = 0;  /* will be fixed when writing the header */
  fli_header.frames   = 0;  /* will be fixed during the write */
  fli_header.width    = ligma_image_get_width (image);
  fli_header.height   = ligma_image_get_height (image);

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
  fli_header.aspect_y = 1;  /* ... as LIGMA supports it. */
  fli_header.oframe1  = fli_header.oframe2 = 0; /* will be fixed during the write */

  fp = g_fopen (g_file_peek_path (file) , "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   ligma_file_get_utf8_name (file), g_strerror (errno));
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
      LigmaDrawable *drawable = iter->data;
      GeglBuffer   *buffer;
      const Babl   *format = NULL;

      buffer = ligma_drawable_get_buffer (drawable);

      g_debug ("Writing frame: %d", cnt);

      if (ligma_drawable_is_gray (drawable))
        {
          if (ligma_drawable_has_alpha (drawable))
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

      ligma_drawable_get_offsets (drawable, &offset_x, &offset_y);

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
          write_ok = fli_write_frame (fp, &fli_header, ofb, cm, fb, cm, W_ALL, error);
        }
      else
        {
          /* save first frame, no delta information, allow all codecs */
          write_ok = fli_write_frame (fp, &fli_header, NULL, NULL, fb, cm, W_ALL, error);
        }
      if (! write_ok)
        break;

      if (cnt < to_frame)
        memcpy (ofb, fb, fli_header.width * fli_header.height);

      ligma_progress_update ((double) cnt + 1 / (double)(to_frame - from_frame));
    }

  /*
   * finish fli
   */
  if (write_ok)
    write_ok = fli_write_header (fp, &fli_header, error);
  fclose (fp);

  g_free (fb);
  g_free (ofb);
  g_list_free (framelist);

  ligma_progress_update (1.0);

  return write_ok;
}

/*
 * Dialogs for interactive usage
 */
static gboolean
load_dialog (GFile         *file,
             LigmaProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *grid;
  GtkWidget *spinbutton;
  gint       width, height, n_frames;
  gboolean   run;

  get_info (file, &width, &height, &n_frames, NULL);

  g_object_set (config,
                "from-frame", 1,
                "to-frame",   n_frames,
                NULL);

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Open FLIC Animation"));

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

  spinbutton = ligma_prop_spin_button_new (config, "from-frame",
                                          1, 10, 0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            C_("frame-range", "_From:"), 0.0, 0.5,
                            spinbutton, 1);

  spinbutton = ligma_prop_spin_button_new (config, "to-frame",
                                          1, 10, 0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            C_("frame-range", "_To:"), 0.0, 0.5,
                            spinbutton, 1);

  gtk_widget_show (dialog);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static gboolean
save_dialog (LigmaImage     *image,
             LigmaProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  gint       n_frames;
  gboolean   run;

  g_free (ligma_image_get_layers (image, &n_frames));

  g_object_set (config,
                "from-frame", 1,
                "to-frame",   n_frames,
                NULL);

  dialog = ligma_procedure_dialog_new (procedure,
                                      LIGMA_PROCEDURE_CONFIG (config),
                                      _("Export Image as FLI Animation"));
  /*
   * Maybe I add on-the-fly RGB conversion, to keep palettechanges...
   * But for now you can set a start- and a end-frame:
   */

  ligma_procedure_dialog_fill (LIGMA_PROCEDURE_DIALOG (dialog), NULL);

  gtk_widget_show (dialog);

  run = ligma_procedure_dialog_run (LIGMA_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
