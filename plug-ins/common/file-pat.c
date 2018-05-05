/*
 * pat plug-in version 1.01
 * Loads/exports version 1 GIMP .pat files, by Tim Newsome <drz@frody.bloke.com>
 *
 * GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "app/core/gimppattern-header.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-pat-load"
#define SAVE_PROC      "file-pat-save"
#define PLUG_IN_BINARY "file-pat"
#define PLUG_IN_ROLE   "gimp-file-pat"


/*  local function prototypes  */

static void       query          (void);
static void       run            (const gchar      *name,
                                  gint              nparams,
                                  const GimpParam  *param,
                                  gint             *nreturn_vals,
                                  GimpParam       **return_vals);
static gint32     load_image     (GFile            *file,
                                  GError          **error);
static gboolean   save_image     (GFile            *file,
                                  gint32            image_ID,
                                  gint32            drawable_ID,
                                  GError          **error);

static gboolean   save_dialog    (void);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/*  private variables  */

static gchar description[256] = "GIMP Pattern";


MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "uri",      "The URI of the file to load" },
    { GIMP_PDB_STRING, "raw-uri",  "The URI of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
    { GIMP_PDB_IMAGE,    "image",       "Input image"                      },
    { GIMP_PDB_DRAWABLE, "drawable",    "Drawable to export"                 },
    { GIMP_PDB_STRING,   "uri",         "The URI of the file to export the image in" },
    { GIMP_PDB_STRING,   "raw-uri",     "The URI of the file to export the image in" },
    { GIMP_PDB_STRING,   "description", "Short description of the pattern" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads Gimp's .PAT pattern files",
                          "The images in the pattern dialog can be loaded "
                          "directly with this plug-in",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          N_("GIMP pattern"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_plugin_icon_register (LOAD_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_PATTERN);
  gimp_register_file_handler_mime (LOAD_PROC, "image/x-gimp-pat");
  gimp_register_file_handler_uri (LOAD_PROC);
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "pat",
                                    "",
                                    "20,string,GPAT");

  gimp_install_procedure (SAVE_PROC,
                          "Exports Gimp pattern file (.PAT)",
                          "New Gimp patterns can be created by exporting them "
                          "in the appropriate place with this plug-in.",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          N_("GIMP pattern"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_plugin_icon_register (SAVE_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_PATTERN);
  gimp_register_file_handler_mime (SAVE_PROC, "image/x-gimp-pat");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "pat", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;
  GimpExportReturn   export = GIMP_EXPORT_CANCEL;
  GError            *error  = NULL;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (g_file_new_for_uri (param[1].data.d_string),
                             &error);

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
  else if (strcmp (name, SAVE_PROC) == 0)
    {
      GFile        *file;
      GimpParasite *parasite;
      gint32        orig_image_ID;

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;
      file        = g_file_new_for_uri (param[3].data.d_string);

      orig_image_ID = image_ID;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);

          export = gimp_export_image (&image_ID, &drawable_ID, "PAT",
                                      GIMP_EXPORT_CAN_HANDLE_GRAY    |
                                      GIMP_EXPORT_CAN_HANDLE_RGB     |
                                      GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);

          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }

          /*  Possibly retrieve data  */
          gimp_get_data (SAVE_PROC, description);

          parasite = gimp_image_get_parasite (orig_image_ID,
                                              "gimp-pattern-name");
          if (parasite)
            {
              strncpy (description,
                       gimp_parasite_data (parasite),
                       MIN (sizeof (description),
                            gimp_parasite_data_size (parasite)));
              description[sizeof (description) - 1] = '\0';

              gimp_parasite_free (parasite);
            }
          else
            {
              gchar *name = g_path_get_basename (gimp_file_get_utf8_name (file));

              if (g_str_has_suffix (name, ".pat"))
                name[strlen (name) - 4] = '\0';

              if (strlen (name))
                {
                  strncpy (description, name, sizeof (description));
                  description[sizeof (description) - 1] = '\0';
                }

              g_free (name);
            }
          break;

        default:
          break;
        }

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          if (! save_dialog ())
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          if (nparams != 6)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              strncpy (description, param[5].data.d_string,
                       sizeof (description));
              description[sizeof (description) - 1] = '\0';
            }
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (save_image (file, image_ID, drawable_ID, &error))
            {
              gimp_set_data (SAVE_PROC, description, sizeof (description));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);

      if (strlen (description))
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-pattern-name",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (description) + 1,
                                        description);
          gimp_image_attach_parasite (orig_image_ID, parasite);
          gimp_parasite_free (parasite);
        }
      else
        {
          gimp_image_detach_parasite (orig_image_ID, "gimp-pattern-name");
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

static gint32
load_image (GFile   *file,
            GError **error)
{
  GInputStream     *input;
  PatternHeader     ph;
  gchar            *name;
  gchar            *temp;
  guchar           *buf;
  gint32            image_ID;
  gint32            layer_ID;
  GimpParasite     *parasite;
  GeglBuffer       *buffer;
  const Babl       *file_format;
  gint              line;
  GimpImageBaseType base_type;
  GimpImageType     image_type;
  gsize             bytes_read;

  gimp_progress_init_printf (_("Opening '%s'"),
                             g_file_get_parse_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));
  if (! input)
    return -1;

  if (! g_input_stream_read_all (input, &ph, sizeof (PatternHeader),
                                 &bytes_read, NULL, error) ||
      bytes_read != sizeof (PatternHeader))
    {
      g_object_unref (input);
      return -1;
    }

  /*  rearrange the bytes in each unsigned int  */
  ph.header_size  = g_ntohl (ph.header_size);
  ph.version      = g_ntohl (ph.version);
  ph.width        = g_ntohl (ph.width);
  ph.height       = g_ntohl (ph.height);
  ph.bytes        = g_ntohl (ph.bytes);
  ph.magic_number = g_ntohl (ph.magic_number);

  if (ph.magic_number != GPATTERN_MAGIC ||
      ph.version      != 1 ||
      ph.header_size  <= sizeof (PatternHeader))
    {
      g_object_unref (input);
      return -1;
    }

  temp = g_new (gchar, ph.header_size - sizeof (PatternHeader));

  if (! g_input_stream_read_all (input,
                                 temp, ph.header_size - sizeof (PatternHeader),
                                 &bytes_read, NULL, error) ||
      bytes_read != ph.header_size - sizeof (PatternHeader))
    {
      g_free (temp);
      g_object_unref (input);
      return -1;
    }

  name = gimp_any_to_utf8 (temp, ph.header_size - sizeof (PatternHeader) - 1,
                           _("Invalid UTF-8 string in pattern file '%s'."),
                           g_file_get_parse_name (file));
  g_free (temp);

  /* Now there's just raw data left. */

  /*
   * Create a new image of the proper size and associate the filename with it.
   */

  switch (ph.bytes)
    {
    case 1:
      base_type = GIMP_GRAY;
      image_type = GIMP_GRAY_IMAGE;
      file_format = babl_format ("Y' u8");
      break;
    case 2:
      base_type = GIMP_GRAY;
      image_type = GIMP_GRAYA_IMAGE;
      file_format = babl_format ("Y'A u8");
      break;
    case 3:
      base_type = GIMP_RGB;
      image_type = GIMP_RGB_IMAGE;
      file_format = babl_format ("R'G'B' u8");
      break;
    case 4:
      base_type = GIMP_RGB;
      image_type = GIMP_RGBA_IMAGE;
      file_format = babl_format ("R'G'B'A u8");
      break;
    default:
      g_message ("Unsupported pattern depth: %d\n"
                 "GIMP Patterns must be GRAY or RGB", ph.bytes);
      g_object_unref (input);
      return -1;
    }

  /* Sanitize input dimensions and guard against overflows. */
  if ((ph.width == 0) || (ph.width > GIMP_MAX_IMAGE_SIZE) ||
      (ph.height == 0) || (ph.height > GIMP_MAX_IMAGE_SIZE) ||
      (G_MAXSIZE / ph.width / ph.bytes < 1))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Invalid header data in '%s': width=%lu, height=%lu, "
                     "bytes=%lu"), g_file_get_parse_name (file),
                   (unsigned long int)ph.width, (unsigned long int)ph.height,
                   (unsigned long int)ph.bytes);
      g_object_unref (input);
      return -1;
    }

  image_ID = gimp_image_new (ph.width, ph.height, base_type);
  gimp_image_set_filename (image_ID, g_file_get_uri (file));

  parasite = gimp_parasite_new ("gimp-pattern-name",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (name) + 1, name);
  gimp_image_attach_parasite (image_ID, parasite);
  gimp_parasite_free (parasite);

  layer_ID = gimp_layer_new (image_ID, name, ph.width, ph.height,
                             image_type,
                             100,
                             gimp_image_get_default_new_layer_mode (image_ID));
  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  g_free (name);

  buffer = gimp_drawable_get_buffer (layer_ID);

  /* this can't overflow because ph.width is <= GIMP_MAX_IMAGE_SIZE */
  buf = g_malloc (ph.width * ph.bytes);

  for (line = 0; line < ph.height; line++)
    {
      if (! g_input_stream_read_all (input, buf, ph.width * ph.bytes,
                                     &bytes_read, NULL, error) ||
          bytes_read != ph.width * ph.bytes)
        {
          if (line == 0)
            {
              g_free (buf);
              g_object_unref (buffer);
              g_object_unref (input);
              return -1;
            }
          else
            {
              g_message ("GIMP Pattern file is truncated "
                         "(%d of %d lines recovered).",
                         line - 1, ph.height);
              break;
            }
        }

      gegl_buffer_set (buffer, GEGL_RECTANGLE (0, line, ph.width, 1), 0,
                       file_format, buf, GEGL_AUTO_ROWSTRIDE);

      gimp_progress_update ((gdouble) line / (gdouble) ph.height);
    }

  g_free (buf);
  g_object_unref (buffer);
  g_object_unref (input);

  gimp_progress_update (1.0);

  return image_ID;
}

static gboolean
save_image (GFile   *file,
            gint32   image_ID,
            gint32   drawable_ID,
            GError **error)
{
  GOutputStream *output;
  PatternHeader  ph;
  GeglBuffer    *buffer;
  const Babl    *file_format;
  guchar        *buf;
  gint           width;
  gint           height;
  gint           line_size;
  gint           line;

  switch (gimp_drawable_type (drawable_ID))
    {
    case GIMP_GRAY_IMAGE:
      file_format = babl_format ("Y' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      file_format = babl_format ("Y'A u8");
      break;

    case GIMP_RGB_IMAGE:
    case GIMP_INDEXED_IMAGE:
      file_format = babl_format ("R'G'B' u8");
      break;

    case GIMP_RGBA_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      file_format = babl_format ("R'G'B'A u8");
      break;

    default:
      g_message ("Unsupported image type: %d\n"
                 "GIMP Patterns must be GRAY or RGB",
                 gimp_drawable_type (drawable_ID));
      return FALSE;
    }

  gimp_progress_init_printf (_("Exporting '%s'"),
                             g_file_get_parse_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    return FALSE;

  buffer = gimp_drawable_get_buffer (drawable_ID);

  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  ph.header_size  = g_htonl (sizeof (PatternHeader) + strlen (description) + 1);
  ph.version      = g_htonl (1);
  ph.width        = g_htonl (width);
  ph.height       = g_htonl (height);
  ph.bytes        = g_htonl (babl_format_get_bytes_per_pixel (file_format));
  ph.magic_number = g_htonl (GPATTERN_MAGIC);

  if (! g_output_stream_write_all (output, &ph, sizeof (PatternHeader),
                                   NULL, NULL, error))
    {
      g_object_unref (output);
      return FALSE;
    }

  if (! g_output_stream_write_all (output,
                                   description, strlen (description) + 1,
                                   NULL, NULL, error))
    {
      g_object_unref (output);
      return FALSE;
    }

  line_size = width * babl_format_get_bytes_per_pixel (file_format);

  /* this can't overflow because drawable->width is <= GIMP_MAX_IMAGE_SIZE */
  buf = g_alloca (line_size);

  for (line = 0; line < height; line++)
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, line, width, 1), 1.0,
                       file_format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (! g_output_stream_write_all (output, buf, line_size,
                                       NULL, NULL, error))
        {
          g_object_unref (buffer);
          g_object_unref (output);
          return FALSE;
        }

      gimp_progress_update ((gdouble) line / (gdouble) height);
    }

  g_object_unref (buffer);
  g_object_unref (output);

  gimp_progress_update (1.0);

  return TRUE;
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *grid;
  GtkWidget *entry;
  gboolean   run;

  dialog = gimp_export_dialog_new (_("Pattern"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (entry), description);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Description:"), 1.0, 0.5,
                            entry, 1);
  gtk_widget_show (entry);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      strncpy (description, gtk_entry_get_text (GTK_ENTRY (entry)),
               sizeof (description));
      description[sizeof (description) - 1] = '\0';
    }

  gtk_widget_destroy (dialog);

  return run;
}
