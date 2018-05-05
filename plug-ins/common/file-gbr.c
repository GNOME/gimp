/* GIMP - The GNU Image Manipulation Program
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

/*
 * gbr plug-in version 1.00
 * Loads/exports version 2 GIMP .gbr files, by Tim Newsome <drz@frody.bloke.com>
 * Some bits stolen from the .99.7 source tree.
 *
 * Added in GBR version 1 support after learning that there wasn't a
 * tool to read them.
 * July 6, 1998 by Seth Burgess <sjburges@gimp.org>
 *
 * Dec 17, 2000
 * Load and save GIMP brushes in GRAY or RGBA.  jtl + neo
 *
 *
 * TODO: Give some better error reporting on not opening files/bad headers
 *       etc.
 */

#include "config.h"

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "app/core/gimpbrush-header.h"
#include "app/core/gimppattern-header.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-gbr-load"
#define SAVE_PROC      "file-gbr-save"
#define PLUG_IN_BINARY "file-gbr"
#define PLUG_IN_ROLE   "gimp-file-gbr"


typedef struct
{
  gchar description[256];
  gint  spacing;
} BrushInfo;


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
static void       entry_callback (GtkWidget        *widget,
                                  gpointer          data);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


/*  private variables  */

static BrushInfo info =
{
  "GIMP Brush",
  10
};


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
    { GIMP_PDB_IMAGE,  "image",        "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",    "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",       "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",    "Drawable to export" },
    { GIMP_PDB_STRING,   "uri",         "The URI of the file to export the image in" },
    { GIMP_PDB_STRING,   "raw-uri",     "The URI of the file to export the image in" },
    { GIMP_PDB_INT32,    "spacing",     "Spacing of the brush" },
    { GIMP_PDB_STRING,   "description", "Short description of the brush" }
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads GIMP brushes",
                          "Loads GIMP brushes (1 or 4 bpp and old .gpb format)",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "1997-2005",
                          N_("GIMP brush"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_plugin_icon_register (LOAD_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_BRUSH);
  gimp_register_file_handler_mime (LOAD_PROC, "image/x-gimp-gbr");
  gimp_register_file_handler_uri (LOAD_PROC);
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "gbr, gpb",
                                    "",
                                    "20, string, GIMP");

  gimp_install_procedure (SAVE_PROC,
                          "Exports files in the GIMP brush file format",
                          "Exports files in the GIMP brush file format",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "1997-2000",
                          N_("GIMP brush"),
                          "RGB*, GRAY*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_plugin_icon_register (SAVE_PROC, GIMP_ICON_TYPE_ICON_NAME,
                             (const guint8 *) GIMP_ICON_BRUSH);
  gimp_register_file_handler_mime (SAVE_PROC, "image/x-gimp-gbr");
  gimp_register_file_handler_uri (SAVE_PROC);
  gimp_register_save_handler (SAVE_PROC, "gbr", "");
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

          export = gimp_export_image (&image_ID, &drawable_ID, "GBR",
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
          gimp_get_data (SAVE_PROC, &info);

          parasite = gimp_image_get_parasite (orig_image_ID,
                                              "gimp-brush-name");
          if (parasite)
            {
              strncpy (info.description,
                       gimp_parasite_data (parasite),
                       MIN (sizeof (info.description),
                            gimp_parasite_data_size (parasite)));
              info.description[sizeof (info.description) - 1] = '\0';

              gimp_parasite_free (parasite);
            }
          else
            {
              gchar *name = g_path_get_basename (gimp_file_get_utf8_name (file));

              if (g_str_has_suffix (name, ".gbr"))
                name[strlen (name) - 4] = '\0';

              if (strlen (name))
                {
                  strncpy (info.description, name, sizeof (info.description));
                  info.description[sizeof (info.description) - 1] = '\0';
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
          if (nparams != 7)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              info.spacing = (param[5].data.d_int32);
              strncpy (info.description, param[6].data.d_string,
                       sizeof (info.description));
              info.description[sizeof (info.description) - 1] = '\0';
            }
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          if (save_image (file, image_ID, drawable_ID, &error))
            {
              gimp_set_data (SAVE_PROC, &info, sizeof (info));
            }
          else
            {
              status = GIMP_PDB_EXECUTION_ERROR;
            }
        }

      if (export == GIMP_EXPORT_EXPORT)
        gimp_image_delete (image_ID);

      if (strlen (info.description))
        {
          GimpParasite *parasite;

          parasite = gimp_parasite_new ("gimp-brush-name",
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (info.description) + 1,
                                        info.description);
          gimp_image_attach_parasite (orig_image_ID, parasite);
          gimp_parasite_free (parasite);
        }
      else
        {
          gimp_image_detach_parasite (orig_image_ID, "gimp-brush-name");
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
  GInputStream      *input;
  gchar             *name;
  BrushHeader        bh;
  guchar            *brush_buf = NULL;
  gint32             image_ID;
  gint32             layer_ID;
  GimpParasite      *parasite;
  GeglBuffer        *buffer;
  const Babl        *format;
  GimpImageBaseType  base_type;
  GimpImageType      image_type;
  gsize              bytes_read;
  gsize              size;
  gint               i;

  gimp_progress_init_printf (_("Opening '%s'"),
                             g_file_get_parse_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, error));
  if (! input)
    return -1;

  size = G_STRUCT_OFFSET (BrushHeader, magic_number);

  if (! g_input_stream_read_all (input, &bh, size,
                                 &bytes_read, NULL, error) ||
      bytes_read != size)
    {
      g_object_unref (input);
      return -1;
    }

  /*  rearrange the bytes in each unsigned int  */
  bh.header_size  = g_ntohl (bh.header_size);
  bh.version      = g_ntohl (bh.version);
  bh.width        = g_ntohl (bh.width);
  bh.height       = g_ntohl (bh.height);
  bh.bytes        = g_ntohl (bh.bytes);

  /* Sanitize values */
  if ((bh.width  == 0) || (bh.width  > GIMP_MAX_IMAGE_SIZE) ||
      (bh.height == 0) || (bh.height > GIMP_MAX_IMAGE_SIZE) ||
      ((bh.bytes != 1) && (bh.bytes != 2) && (bh.bytes != 4) &&
       (bh.bytes != 18)) ||
      (G_MAXSIZE / bh.width / bh.height / MAX (4, bh.bytes) < 1))
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Invalid header data in '%s': width=%lu, height=%lu, "
                     "bytes=%lu"), g_file_get_parse_name (file),
                   (unsigned long int)bh.width, (unsigned long int)bh.height,
                   (unsigned long int)bh.bytes);
      return -1;
    }

  switch (bh.version)
    {
    case 1:
      /* Version 1 didn't have a magic number and had no spacing  */
      bh.spacing = 25;
      bh.header_size += 8;
      if (bh.header_size < sizeof (BrushHeader))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Unsupported brush format"));
          g_object_unref (input);
          return -1;
        }
      break;

    case 2:
    case 3: /*  cinepaint brush  */
      size = sizeof (bh.magic_number) + sizeof (bh.spacing);

      if (! g_input_stream_read_all (input,
                                     (guchar *) &bh +
                                     G_STRUCT_OFFSET (BrushHeader,
                                                      magic_number), size,
                                     &bytes_read, NULL, error) ||
          bytes_read != size)
        {
          g_object_unref (input);
          return -1;
        }

      bh.magic_number = g_ntohl (bh.magic_number);
      bh.spacing      = g_ntohl (bh.spacing);

      if (bh.version == 3)
        {
          if (bh.bytes == 18 /* FLOAT16_GRAY_GIMAGE */)
            {
              bh.bytes = 2;
            }
          else
            {
              g_message (_("Unsupported brush format"));
              g_object_unref (input);
              return -1;
            }
        }

      if (bh.magic_number == GBRUSH_MAGIC &&
          bh.header_size  >  sizeof (BrushHeader))
        break;

    default:
      g_message (_("Unsupported brush format"));
      g_object_unref (input);
      return -1;
    }

  if ((size = (bh.header_size - sizeof (BrushHeader))) > 0)
    {
      gchar *temp = g_new (gchar, size);

      if (! g_input_stream_read_all (input, temp, size,
                                     &bytes_read, NULL, error) ||
          bytes_read != size)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error in GIMP brush file '%s'"),
                       g_file_get_parse_name (file));
          g_object_unref (input);
          g_free (temp);
          return -1;
        }

      name = gimp_any_to_utf8 (temp, size - 1,
                               _("Invalid UTF-8 string in brush file '%s'."),
                               g_file_get_parse_name (file));
      g_free (temp);
    }
  else
    {
      name = g_strdup (_("Unnamed"));
    }

  /* Now there's just raw data left. */

  size = (gsize) bh.width * bh.height * bh.bytes;
  brush_buf = g_malloc (size);

  if (! g_input_stream_read_all (input, brush_buf, size,
                                 &bytes_read, NULL, error) ||
      bytes_read != size)
    {
      g_object_unref (input);
      g_free (brush_buf);
      g_free (name);
      return -1;
    }

  switch (bh.bytes)
    {
    case 1:
      {
        PatternHeader ph;

        /* For backwards-compatibility, check if a pattern follows.
         * The obsolete .gpb format did it this way.
         */

        if (g_input_stream_read_all (input, &ph, sizeof (PatternHeader),
                                     &bytes_read, NULL, NULL) &&
            bytes_read == sizeof (PatternHeader))
          {
            /*  rearrange the bytes in each unsigned int  */
            ph.header_size  = g_ntohl (ph.header_size);
            ph.version      = g_ntohl (ph.version);
            ph.width        = g_ntohl (ph.width);
            ph.height       = g_ntohl (ph.height);
            ph.bytes        = g_ntohl (ph.bytes);
            ph.magic_number = g_ntohl (ph.magic_number);

            if (ph.magic_number == GPATTERN_MAGIC        &&
                ph.version      == 1                     &&
                ph.header_size  > sizeof (PatternHeader) &&
                ph.bytes        == 3                     &&
                ph.width        == bh.width              &&
                ph.height       == bh.height             &&
                g_input_stream_skip (input,
                                     ph.header_size - sizeof (PatternHeader),
                                     NULL, NULL) ==
                ph.header_size - sizeof (PatternHeader))
              {
                guchar *plain_brush = brush_buf;
                gint    i;

                bh.bytes = 4;
                brush_buf = g_malloc ((gsize) bh.width * bh.height * 4);

                for (i = 0; i < ph.width * ph.height; i++)
                  {
                    if (! g_input_stream_read_all (input,
                                                   brush_buf + i * 4, 3,
                                                   &bytes_read, NULL, error) ||
                        bytes_read != 3)
                      {
                        g_object_unref (input);
                        g_free (name);
                        g_free (plain_brush);
                        g_free (brush_buf);
                        return -1;
                      }

                    brush_buf[i * 4 + 3] = plain_brush[i];
                  }

                g_free (plain_brush);
              }
          }
      }
      break;

    case 2:
      {
        guint16 *buf = (guint16 *) brush_buf;

        for (i = 0; i < bh.width * bh.height; i++, buf++)
          {
            union
            {
              guint16 u[2];
              gfloat  f;
            } short_float;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
            short_float.u[0] = 0;
            short_float.u[1] = GUINT16_FROM_BE (*buf);
#else
            short_float.u[0] = GUINT16_FROM_BE (*buf);
            short_float.u[1] = 0;
#endif

            brush_buf[i] = (guchar) (short_float.f * 255.0 + 0.5);
          }

        bh.bytes = 1;
      }
      break;

    default:
      break;
    }

  /*
   * Create a new image of the proper size and
   * associate the filename with it.
   */

  switch (bh.bytes)
    {
    case 1:
      base_type = GIMP_GRAY;
      image_type = GIMP_GRAY_IMAGE;
      format = babl_format ("Y' u8");
      break;

    case 4:
      base_type = GIMP_RGB;
      image_type = GIMP_RGBA_IMAGE;
      format = babl_format ("R'G'B'A u8");
      break;

    default:
      g_message ("Unsupported brush depth: %d\n"
                 "GIMP Brushes must be GRAY or RGBA\n",
                 bh.bytes);
      g_free (name);
      return -1;
    }

  image_ID = gimp_image_new (bh.width, bh.height, base_type);
  gimp_image_set_filename (image_ID, g_file_get_uri (file));

  parasite = gimp_parasite_new ("gimp-brush-name",
                                GIMP_PARASITE_PERSISTENT,
                                strlen (name) + 1, name);
  gimp_image_attach_parasite (image_ID, parasite);
  gimp_parasite_free (parasite);

  layer_ID = gimp_layer_new (image_ID, name, bh.width, bh.height,
                             image_type,
                             100,
                             gimp_image_get_default_new_layer_mode (image_ID));
  gimp_image_insert_layer (image_ID, layer_ID, -1, 0);

  g_free (name);

  buffer = gimp_drawable_get_buffer (layer_ID);

  /*  invert  */
  if (image_type == GIMP_GRAY_IMAGE)
    for (i = 0; i < bh.width * bh.height; i++)
      brush_buf[i] = 255 - brush_buf[i];

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, bh.width, bh.height), 0,
                   format, brush_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (brush_buf);
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
  BrushHeader    bh;
  guchar        *brush_buf;
  GeglBuffer    *buffer;
  const Babl    *format;
  gint           line;
  gint           x;
  gint           bpp;
  gint           file_bpp;
  gint           width;
  gint           height;
  GimpRGB        gray, white;

  gimp_rgba_set_uchar (&white, 255, 255, 255, 255);

  switch (gimp_drawable_type (drawable_ID))
    {
    case GIMP_GRAY_IMAGE:
      file_bpp = 1;
      format = babl_format ("Y' u8");
      break;

    case GIMP_GRAYA_IMAGE:
      file_bpp = 1;
      format = babl_format ("Y'A u8");
      break;

    default:
      file_bpp = 4;
      format = babl_format ("R'G'B'A u8");
      break;
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             g_file_get_parse_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (! output)
    return FALSE;

  buffer = gimp_drawable_get_buffer (drawable_ID);

  width  = gimp_drawable_width  (drawable_ID);
  height = gimp_drawable_height (drawable_ID);

  bh.header_size  = g_htonl (sizeof (BrushHeader) +
                             strlen (info.description) + 1);
  bh.version      = g_htonl (2);
  bh.width        = g_htonl (width);
  bh.height       = g_htonl (height);
  bh.bytes        = g_htonl (file_bpp);
  bh.magic_number = g_htonl (GBRUSH_MAGIC);
  bh.spacing      = g_htonl (info.spacing);

  if (! g_output_stream_write_all (output, &bh, sizeof (BrushHeader),
                                   NULL, NULL, error))
    {
      g_object_unref (output);
      return FALSE;
    }

  if (! g_output_stream_write_all (output,
                                   info.description,
                                   strlen (info.description) + 1,
                                   NULL, NULL, error))
    {
      g_object_unref (output);
      return FALSE;
    }

  brush_buf = g_new (guchar, width * bpp);

  for (line = 0; line < height; line++)
    {
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, line, width, 1), 1.0,
                       format, brush_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      switch (bpp)
        {
        case 1:
          /*  invert  */
          for (x = 0; x < width; x++)
            brush_buf[x] = 255 - brush_buf[x];
          break;

        case 2:
          for (x = 0; x < width; x++)
            {
              /*  apply alpha channel  */
              gimp_rgba_set_uchar (&gray,
                                   brush_buf[2 * x],
                                   brush_buf[2 * x],
                                   brush_buf[2 * x],
                                   brush_buf[2 * x + 1]);
              gimp_rgb_composite (&gray, &white, GIMP_RGB_COMPOSITE_BEHIND);
              gimp_rgba_get_uchar (&gray, &brush_buf[x], NULL, NULL, NULL);
              /* invert */
              brush_buf[x] = 255 - brush_buf[x];
            }
          break;
        }

      if (! g_output_stream_write_all (output, brush_buf, width * file_bpp,
                                       NULL, NULL, error))
        {
          g_free (brush_buf);
          g_object_unref (output);
          return FALSE;
        }

      gimp_progress_update ((gdouble) line / (gdouble) height);
    }

  g_free (brush_buf);
  g_object_unref (buffer);
  g_object_unref (output);

  gimp_progress_update (1.0);

  return TRUE;
}

static gboolean
save_dialog (void)
{
  GtkWidget     *dialog;
  GtkWidget     *grid;
  GtkWidget     *entry;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  gboolean       run;

  dialog = gimp_export_dialog_new (_("Brush"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main grid */
  grid = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Description:"), 1.0, 0.5,
                            entry, 1);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    info.description);

  adj = (GtkAdjustment *) gtk_adjustment_new (info.spacing, 1, 1000, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY (spinbutton), TRUE);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Spacing:"), 1.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &info.spacing);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
entry_callback (GtkWidget *widget,
                gpointer   data)
{
  strncpy (info.description, gtk_entry_get_text (GTK_ENTRY (widget)),
           sizeof (info.description));
  info.description[sizeof (info.description) - 1] = '\0';
}
