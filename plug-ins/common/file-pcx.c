/*
 * pcx.c GIMP plug-in for loading & exporting PCX files
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

/* This code is based in parts on code by Francisco Bustamante, but the
   largest portion of the code has been rewritten and is now maintained
   occasionally by Nick Lamb njl195@zepler.org.uk */

#include "config.h"

#include <errno.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-pcx-load"
#define LOAD_PROC_DCX  "file-dcx-load"
#define EXPORT_PROC    "file-pcx-export"
#define PLUG_IN_BINARY "file-pcx"
#define PLUG_IN_ROLE   "gimp-file-pcx"


typedef struct _Pcx      Pcx;
typedef struct _PcxClass PcxClass;

struct _Pcx
{
  GimpPlugIn      parent_instance;
};

struct _PcxClass
{
  GimpPlugInClass parent_class;
};


#define PCX_TYPE  (pcx_get_type ())
#define PCX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PCX_TYPE, Pcx))

GType                   pcx_get_type         (void) G_GNUC_CONST;

static GList          * pcx_query_procedures (GimpPlugIn            *plug_in);
static GimpProcedure  * pcx_create_procedure (GimpPlugIn            *plug_in,
                                              const gchar           *name);

static GimpValueArray * pcx_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * dcx_load             (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GFile                 *file,
                                              GimpMetadata          *metadata,
                                              GimpMetadataLoadFlags *flags,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);
static GimpValueArray * pcx_export           (GimpProcedure         *procedure,
                                              GimpRunMode            run_mode,
                                              GimpImage             *image,
                                              GFile                 *file,
                                              GimpExportOptions     *options,
                                              GimpMetadata          *metadata,
                                              GimpProcedureConfig   *config,
                                              gpointer               run_data);

static GimpImage      * load_single          (GimpProcedure         *procedure,
                                              GFile                 *file,
                                              GObject               *config,
                                              gint                   run_mode,
                                              GError               **error);
static GimpImage      * load_multi           (GimpProcedure         *procedure,
                                              GFile                 *file,
                                              GObject               *config,
                                              gint                   run_mode,
                                              GError               **error);

static GimpImage      * load_image           (GimpProcedure         *procedure,
                                              FILE                  *fd,
                                              const gchar           *filename,
                                              GObject               *config,
                                              gint                   run_mode,
                                              gint                   image_num,
                                              GError               **error);
static gboolean         pcx_load_dialog      (GimpProcedure         *procedure,
                                              GObject               *config);

static void             load_1               (FILE                  *fp,
                                              gint                   width,
                                              gint                   height,
                                              guchar                *buf,
                                              guint16                bytes);
static void             load_4               (FILE                  *fp,
                                              gint                   width,
                                              gint                   height,
                                              guchar                *buf,
                                              guint16                bytes);
static void             load_sub_8           (FILE                  *fp,
                                              gint                   width,
                                              gint                   height,
                                              gint                   bpp,
                                              gint                   plane,
                                              guchar                *buf,
                                              guint16                bytes);
static void             load_8               (FILE                  *fp,
                                              gint                   width,
                                              gint                   height,
                                              guchar                *buf,
                                              guint16                bytes);
static void             load_24              (FILE                  *fp,
                                              gint                   width,
                                              gint                   height,
                                              guchar                *buf,
                                              guint16                bytes,
                                              guint8                 planes);
static void             readline             (FILE                  *fp,
                                              guchar                *buf,
                                              gint                   bytes);

static gboolean         export_image         (GFile                 *file,
                                              GimpImage             *image,
                                              GimpDrawable          *drawable,
                                              GError               **error);
static void             save_less_than_8     (FILE                  *fp,
                                              gint                   width,
                                              gint                   height,
                                              const gint             bpp,
                                              const guchar          *buf,
                                              gboolean               padding);
static void             save_8               (FILE                  *fp,
                                              gint                   width,
                                              gint                   height,
                                              const guchar          *buf,
                                              gboolean               padding);
static void             save_24              (FILE                  *fp,
                                              gint                   width,
                                              gint                   height,
                                              const guchar          *buf,
                                              gboolean               padding);
static void             writeline            (FILE                  *fp,
                                              const guchar          *buf,
                                              gint                   bytes);


G_DEFINE_TYPE (Pcx, pcx, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (PCX_TYPE)
DEFINE_STD_SET_I18N


static void
pcx_class_init (PcxClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pcx_query_procedures;
  plug_in_class->create_procedure = pcx_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pcx_init (Pcx *pcx)
{
}

static GList *
pcx_query_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC_DCX));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static GimpProcedure *
pcx_create_procedure (GimpPlugIn  *plug_in,
                      const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           pcx_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("ZSoft PCX image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads files in Zsoft PCX file format"),
                                        "FIXME: write help for pcx_load",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Francisco Bustamante & Nick Lamb",
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      "January 1997");

      gimp_procedure_add_choice_argument (procedure, "override-palette",
                                          _("_Palette Options"),
                                          _("Whether to use the built-in palette or "
                                            "a black and white palette for 1 bit images."),
                                          gimp_choice_new_with_values ("use-built-in-palette", 0, _("Use PCX image's built-in palette"), NULL,
                                                                       "use-bw-palette",       1, _("Use black and white palette"),      NULL,
                                                                       NULL),
                                          "use-built-in-palette",
                                          G_PARAM_READWRITE);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-pcx");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "pcx,pcc");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0&,byte,10,2&,byte,1,3&,byte,>0,3,byte,<9");
    }
  else if (! strcmp (name, LOAD_PROC_DCX))
    {
      procedure = gimp_load_procedure_new (plug_in, name,
                                           GIMP_PDB_PROC_TYPE_PLUGIN,
                                           dcx_load, NULL, NULL);

      gimp_procedure_set_menu_label (procedure, _("ZSoft DCX image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Loads files in Zsoft DCX file format"),
                                        "FIXME: write help for dcx_load",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Francisco Bustamante, Nick Lamb, Alex S.",
                                      "Alex S.",
                                      "2023");

      gimp_procedure_add_choice_argument (procedure, "override-palette",
                                          _("_Palette Options"),
                                          _("Whether to use the built-in palette or "
                                            "a black and white palette for 1 bit images."),
                                          gimp_choice_new_with_values ("use-built-in-palette", 0, _("Use PCX image's built-in palette"), NULL,
                                                                       "use-bw-palette",       1, _("Use black and white palette"),      NULL,
                                                                       NULL),
                                          "use-built-in-palette",
                                          G_PARAM_READWRITE);

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-dcx");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "dcx");
      gimp_file_procedure_set_magics (GIMP_FILE_PROCEDURE (procedure),
                                      "0,string,\xB1\x68\xDE\x3A");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = gimp_export_procedure_new (plug_in, name,
                                             GIMP_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, pcx_export, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "INDEXED, RGB, GRAY");

      gimp_procedure_set_menu_label (procedure, _("ZSoft PCX image"));

      gimp_procedure_set_documentation (procedure,
                                        _("Exports files in ZSoft PCX file format"),
                                        "FIXME: write help for pcx_export",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Francisco Bustamante & Nick Lamb",
                                      "Nick Lamb <njl195@zepler.org.uk>",
                                      "January 1997");

      gimp_file_procedure_set_mime_types (GIMP_FILE_PROCEDURE (procedure),
                                          "image/x-pcx");
      gimp_file_procedure_set_extensions (GIMP_FILE_PROCEDURE (procedure),
                                          "pcx,pcc");

      gimp_export_procedure_set_capabilities (GIMP_EXPORT_PROCEDURE (procedure),
                                              GIMP_EXPORT_CAN_HANDLE_RGB  |
                                              GIMP_EXPORT_CAN_HANDLE_GRAY |
                                              GIMP_EXPORT_CAN_HANDLE_INDEXED,
                                              NULL, NULL, NULL);
    }

  return procedure;
}

static GimpValueArray *
pcx_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_single (procedure, file, G_OBJECT (config), run_mode, &error);

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
dcx_load (GimpProcedure         *procedure,
          GimpRunMode            run_mode,
          GFile                 *file,
          GimpMetadata          *metadata,
          GimpMetadataLoadFlags *flags,
          GimpProcedureConfig   *config,
          gpointer               run_data)
{
  GimpValueArray *return_vals;
  GimpImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_multi (procedure, file, G_OBJECT (config), run_mode, &error);

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
pcx_export (GimpProcedure        *procedure,
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
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  export = gimp_export_options_get_image (options, &image);
  drawables = gimp_image_list_layers (image);

  if (! export_image (file,
                      image, drawables->data,
                      &error))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image);

  g_list_free (drawables);
  return gimp_procedure_new_return_values (procedure, status, error);
}

static struct
{
  guint8  manufacturer;
  guint8  version;
  guint8  compression;
  guint8  bpp;
  guint16 x1, y1;
  guint16 x2, y2;
  guint16 hdpi;
  guint16 vdpi;
  guint8  colormap[48];
  guint8  reserved;
  guint8  planes;
  guint16 bytesperline;
  guint16 color;
  guint8  filler[58];
} pcx_header;

static struct {
  size_t   size;
  gpointer address;
} const pcx_header_buf_xlate[] = {
  { 1,  &pcx_header.manufacturer },
  { 1,  &pcx_header.version      },
  { 1,  &pcx_header.compression  },
  { 1,  &pcx_header.bpp          },
  { 2,  &pcx_header.x1           },
  { 2,  &pcx_header.y1           },
  { 2,  &pcx_header.x2           },
  { 2,  &pcx_header.y2           },
  { 2,  &pcx_header.hdpi         },
  { 2,  &pcx_header.vdpi         },
  { 48, &pcx_header.colormap     },
  { 1,  &pcx_header.reserved     },
  { 1,  &pcx_header.planes       },
  { 2,  &pcx_header.bytesperline },
  { 2,  &pcx_header.color        },
  { 58, &pcx_header.filler       },
  { 0,  NULL }
};

static void
pcx_header_from_buffer (guint8 *buf)
{
  gint i;
  gint buf_offset = 0;

  for (i = 0; pcx_header_buf_xlate[i].size != 0; i++)
    {
      memmove (pcx_header_buf_xlate[i].address, buf + buf_offset,
               pcx_header_buf_xlate[i].size);
      buf_offset += pcx_header_buf_xlate[i].size;
    }
}

static void
pcx_header_to_buffer (guint8 *buf)
{
  gint i;
  gint buf_offset = 0;

  for (i = 0; pcx_header_buf_xlate[i].size != 0; i++)
    {
      memmove (buf + buf_offset, pcx_header_buf_xlate[i].address,
               pcx_header_buf_xlate[i].size);
      buf_offset += pcx_header_buf_xlate[i].size;
    }
}

static GimpImage *
load_single (GimpProcedure  *procedure,
             GFile          *file,
             GObject        *config,
             gint            run_mode,
             GError        **error)
{
  FILE      *fd;
  GimpImage *image;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  fd = g_fopen (g_file_peek_path (file), "rb");

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  image = load_image (procedure, fd, gimp_file_get_utf8_name (file),
                      G_OBJECT (config), run_mode, 0, error);
  fclose (fd);

  if (! image)
    {
      g_prefix_error (error,
                      _("Could not load PCX image: "));
      return NULL;
    }

  gimp_progress_update (1.0);

  return image;
}

static GimpImage *
load_multi (GimpProcedure  *procedure,
            GFile          *file,
            GObject        *config,
            gint            run_mode,
            GError        **error)
{
  FILE      *fd;
  GimpImage *image       = NULL;
  GimpImage *temp_image  = NULL;
  gint       offset;
  gint       next_id     = 8;
  gint       valid_offset;

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_file_get_utf8_name (file));

  fd = g_fopen (g_file_peek_path (file), "rb");

  if (! fd)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return NULL;
    }

  /* Skip header */
  fread (&offset, 1, 4, fd);

  /* Read the first offset */
  fread (&offset, 1, 4, fd);
  valid_offset = fseek (fd, offset, SEEK_SET);

  if (valid_offset != 0)
    {
      fclose (fd);
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("DCX image offset exceeds the file size"));
      return NULL;
    }

  image = load_image (procedure, fd, gimp_file_get_utf8_name (file),
                      G_OBJECT (config), run_mode, 0, error);

  if (! image)
    {
      fclose (fd);
      g_prefix_error (error,
                      _("Could not load DCX image: "));
      return NULL;
    }

  fseek (fd, next_id, SEEK_SET);

  /* DCX can hold a maximum of 1023 images,
   * plus a terminal value of 0 */
  for (gint i = 1; i < 1023; i++)
    {
      GimpLayer **layers;
      GimpLayer  *new_layer;
      gint        n_layers;

      fread (&offset, 1, 4, fd);

      if (offset == 0)
        break;

      valid_offset = fseek (fd, offset, SEEK_SET);
      if (valid_offset != 0)
        {
          fclose (fd);
          g_message (_("%s: DCX image offset exceeds the file size: %s\n"),
                     G_STRFUNC, (*error)->message);
          g_clear_error (error);
          return image;
        }

      temp_image = load_image (procedure, fd, gimp_file_get_utf8_name (file),
                               G_OBJECT (config), run_mode, i, error);

      if (temp_image)
        {
          layers = gimp_image_get_layers (temp_image, &n_layers);
          new_layer = gimp_layer_new_from_drawable (GIMP_DRAWABLE (layers[0]),
                                                    image);
          gimp_item_set_name (GIMP_ITEM (new_layer),
                              g_file_get_basename (file));
          if (! gimp_image_insert_layer (image, new_layer, NULL, 0))
            g_message (_("Mixed-mode DCX image not loaded"));

          g_free (layers);
        }
      else
        {
          fclose (fd);
          g_message (_("%s: Could not load all DCX images: %s\n"),
                     G_STRFUNC, (*error)->message);
          g_clear_error (error);
          return image;
        }

      next_id += 4;
      fseek (fd, next_id, SEEK_SET);
    }

  if (image)
    fclose (fd);

  gimp_progress_update (1.0);

  return image;
}

static GimpImage *
load_image (GimpProcedure  *procedure,
            FILE           *fd,
            const gchar    *filename,
            GObject        *config,
            gint            run_mode,
            gint            image_num,
            GError        **error)
{
  GeglBuffer   *buffer;
  guint16       offset_x, offset_y, bytesperline;
  gint32        width, height;
  guint16       resolution_x, resolution_y;
  GimpImage    *image;
  GimpLayer    *layer;
  guchar       *dest, cmap[768];
  guint8        header_buf[128];
  gboolean      override_palette = FALSE;

  if (fread (header_buf, 128, 1, fd) == 0)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not read header from '%s'"),
                   filename);
      return NULL;
    }

  pcx_header_from_buffer (header_buf);

  if (pcx_header.manufacturer != 10)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("'%s' is not a PCX file"),
                   filename);
      return NULL;
    }

  offset_x     = GUINT16_FROM_LE (pcx_header.x1);
  offset_y     = GUINT16_FROM_LE (pcx_header.y1);
  width        = GUINT16_FROM_LE (pcx_header.x2) - offset_x + 1;
  height       = GUINT16_FROM_LE (pcx_header.y2) - offset_y + 1;
  bytesperline = GUINT16_FROM_LE (pcx_header.bytesperline);
  resolution_x = GUINT16_FROM_LE (pcx_header.hdpi);
  resolution_y = GUINT16_FROM_LE (pcx_header.vdpi);

  if ((width <= 0) || (width > GIMP_MAX_IMAGE_SIZE))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Unsupported or invalid image width: %d"),
                   width);
      return NULL;
    }
  if ((height <= 0) || (height > GIMP_MAX_IMAGE_SIZE))
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Unsupported or invalid image height: %d"),
                   height);
      return NULL;
    }
  if ((bytesperline + 1) < ((width * pcx_header.bpp + 7) / 8) ||
      bytesperline == 0)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Invalid number of bytes per line in PCX header"));
      return NULL;
    }
  if ((resolution_x < 1) || (resolution_x > GIMP_MAX_RESOLUTION) ||
      (resolution_y < 1) || (resolution_y > GIMP_MAX_RESOLUTION))
    {
      g_message (_("Resolution out of bounds in XCX header, using 72x72"));
      resolution_x = 72;
      resolution_y = 72;
    }

  /* Shield against potential buffer overflows in load_*() functions. */
  if (G_MAXSIZE / width / height < 3)
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Image dimensions too large: width %d x height %d"),
                   width, height);
      return NULL;
    }

  if (pcx_header.planes == 3 && pcx_header.bpp == 8)
    {
      image = gimp_image_new (width, height, GIMP_RGB);
      layer = gimp_layer_new (image, _("Background"), width, height,
                              GIMP_RGB_IMAGE,
                              100,
                              gimp_image_get_default_new_layer_mode (image));
    }
  else if (pcx_header.planes == 4 && pcx_header.bpp == 8)
    {
      image = gimp_image_new (width, height, GIMP_RGB);
      layer = gimp_layer_new (image, _("Background"), width, height,
                              GIMP_RGBA_IMAGE,
                              100,
                              gimp_image_get_default_new_layer_mode (image));
    }
  else
    {
      image = gimp_image_new (width, height, GIMP_INDEXED);
      layer = gimp_layer_new (image, _("Background"), width, height,
                              GIMP_INDEXED_IMAGE,
                              100,
                              gimp_image_get_default_new_layer_mode (image));
    }

  gimp_image_set_resolution (image, resolution_x, resolution_y);

  gimp_image_insert_layer (image, layer, NULL, 0);
  gimp_layer_set_offsets (layer, offset_x, offset_y);

  buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (layer));

  if (pcx_header.planes == 1 && pcx_header.bpp == 1)
    {
      const guint8 *colormap = pcx_header.colormap;
      dest = g_new (guchar, ((gsize) width) * height);
      load_1 (fd, width, height, dest, bytesperline);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          override_palette = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                                  "override-palette");

          /* Only show dialogue once for DCX import */
          if (image_num == 0 && pcx_load_dialog (procedure, config))
            override_palette = gimp_procedure_config_get_choice_id (GIMP_PROCEDURE_CONFIG (config),
                                                                    "override-palette");
        }
      /* Monochrome does not mean necessarily B&W. Therefore we still
       * want to check the header palette, even for just 2 colors.
       * Hopefully the header palette will always be filled with
       * meaningful colors and the creator software did not just assume
       * B&W by being monochrome.
       * Until now test samples showed that even when B&W the header
       * palette was correctly filled with these 2 colors and we didn't
       * find counter-examples.
       * See bug 159947, comment 21 and 23.
       */
      /* ... Actually, there *are* files out there with a zeroed 1-bit palette,
       * which are supposed to be displayed as B&W (see issue #2997.)  These
       * files *might* be in the wrong (who knows...) but the fact is that
       * other software, including older versions of GIMP, do display them
       * "correctly", so let's follow suit: if the two palette colors are
       * equal (or if the user chooses the option in the load dialog),
       * use a B&W palette instead.
       */
      if (! memcmp (colormap, colormap + 3, 3) || override_palette)
        {
          static const guint8 bw_colormap[6] = {  0,   0,   0,
                                                255, 255, 255};
          colormap = bw_colormap;
        }
      gimp_image_set_colormap (image, colormap, 2);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 2)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 1, 2, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 4);
    }
  else if (pcx_header.bpp == 2 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 2, 1, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 4);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 3)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 1, 3, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 8);
    }
  else if (pcx_header.bpp == 1 && pcx_header.planes == 4)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_4 (fd, width, height, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 16);
    }
  else if (pcx_header.bpp == 4 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_sub_8 (fd, width, height, 4, 1, dest, bytesperline);
      gimp_image_set_colormap (image, pcx_header.colormap, 16);
    }
  else if (pcx_header.bpp == 8 && pcx_header.planes == 1)
    {
      dest = g_new (guchar, ((gsize) width) * height);
      load_8 (fd, width, height, dest, bytesperline);
      fseek (fd, -768L, SEEK_END);
      fread (cmap, 768, 1, fd);
      gimp_image_set_colormap (image, cmap, 256);
    }
  else if (pcx_header.bpp == 8 && (pcx_header.planes == 3 || pcx_header.planes == 4))
    {
      dest = g_new (guchar, ((gsize) width) * height * pcx_header.planes);
      load_24 (fd, width, height, dest, bytesperline, pcx_header.planes);
    }
  else
    {
      g_set_error (error, GIMP_PLUG_IN_ERROR, 0,
                   _("Unusual PCX flavour, giving up"));
      g_object_unref (buffer);
      return NULL;
    }

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, width, height), 0,
                   NULL, dest, GEGL_AUTO_ROWSTRIDE);

  g_free (dest);
  g_object_unref (buffer);

  return image;
}

static gboolean
pcx_load_dialog (GimpProcedure *procedure,
                 GObject       *config)
{
  GtkWidget    *dialog;
  gboolean      run;

  gimp_ui_init (PLUG_IN_BINARY);

  dialog = gimp_procedure_dialog_new (procedure,
                                      GIMP_PROCEDURE_CONFIG (config),
                                      _("Import from PCX"));

  gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dialog),
                                    "override-palette", GIMP_TYPE_INT_RADIO_FRAME);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dialog),
                              NULL);

  gtk_widget_show (dialog);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
load_8 (FILE    *fp,
        gint     width,
        gint     height,
        guchar  *buf,
        guint16  bytes)
{
  gint    row;
  guchar *line = g_new (guchar, bytes);

  for (row = 0; row < height; buf += width, ++row)
    {
      readline (fp, line, bytes);
      memcpy (buf, line, width);
      gimp_progress_update ((double) row / (double) height);
    }

  g_free (line);
}

static void
load_24 (FILE    *fp,
         gint     width,
         gint     height,
         guchar  *buf,
         guint16  bytes,
         guint8   planes)
{
  gint    x, y, c;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buf += width * planes, ++y)
    {
      for (c = 0; c < planes; ++c)
        {
          readline (fp, line, bytes);
          for (x = 0; x < width; ++x)
            {
              buf[x * planes + c] = line[x];
            }
        }
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
load_1 (FILE    *fp,
        gint     width,
        gint     height,
        guchar  *buf,
        guint16  bytes)
{
  gint    x, y;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buf += width, ++y)
    {
      readline (fp, line, bytes);
      for (x = 0; x < width; ++x)
        {
          if (line[x / 8] & (128 >> (x % 8)))
            buf[x] = 1;
          else
            buf[x] = 0;
        }
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
load_4 (FILE    *fp,
        gint     width,
        gint     height,
        guchar  *buf,
        guint16  bytes)
{
  gint    x, y, c;
  guchar *line = g_new (guchar, bytes);

  for (y = 0; y < height; buf += width, ++y)
    {
      for (x = 0; x < width; ++x)
        buf[x] = 0;
      for (c = 0; c < 4; ++c)
        {
          readline(fp, line, bytes);
          for (x = 0; x < width; ++x)
            {
              if (line[x / 8] & (128 >> (x % 8)))
                buf[x] += (1 << c);
            }
        }
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
load_sub_8 (FILE    *fp,
            gint     width,
            gint     height,
            gint     bpp,
            gint     plane,
            guchar  *buf,
            guint16  bytes)
{
  gint    x, y, c, b;
  guchar *line = g_new (guchar, bytes);
  gint    real_bpp = bpp - 1;
  gint    current_bit = 0;

  for (y = 0; y < height; buf += width, ++y)
    {
      for (x = 0; x < width; ++x)
        buf[x] = 0;
      for (c = 0; c < plane; ++c)
        {
          readline (fp, line, bytes);
          for (x = 0; x < width; ++x)
            {
              for (b = 0; b < bpp; b++)
                {
                  current_bit = bpp * x + b;
                  if (line[current_bit / 8] & (128 >> (current_bit % 8)))
                    buf[x] += (1 << (real_bpp - b + c));
                }
            }
        }
      gimp_progress_update ((double) y / (double) height);
    }

  g_free (line);
}

static void
readline (FILE   *fp,
          guchar *buf,
          gint    bytes)
{
  static guchar count = 0, value = 0;

  if (pcx_header.compression)
    {
      while (bytes--)
        {
          if (count == 0)
            {
              value = fgetc (fp);
              if (value < 0xc0)
                {
                  count = 1;
                }
              else
                {
                  count = value - 0xc0;
                  value = fgetc (fp);
                }
            }
          count--;
          *(buf++) = value;
        }
    }
  else
    {
      fread (buf, bytes, 1, fp);
    }
}

static gboolean
export_image (GFile         *file,
              GimpImage     *image,
              GimpDrawable  *drawable,
              GError       **error)
{
  FILE          *fp;
  GeglBuffer    *buffer;
  const Babl    *format;
  GimpImageType  drawable_type;
  guchar        *cmap= NULL;
  guchar        *pixels;
  gint           offset_x, offset_y;
  guint          width, height;
  gdouble        resolution_x, resolution_y;
  gint           colors, i;
  guint8         header_buf[128];
  gboolean       padding = FALSE;

  drawable_type = gimp_drawable_type (drawable);
  gimp_drawable_get_offsets (drawable, &offset_x, &offset_y);

  buffer = gimp_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  gimp_progress_init_printf (_("Exporting '%s'"),
                             gimp_file_get_utf8_name (file));

  pcx_header.manufacturer = 0x0a;
  pcx_header.version      = 5;
  pcx_header.compression  = 1;

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      cmap = gimp_image_get_colormap (image, NULL, &colors);

      if (colors > 16)
        {
          pcx_header.bpp            = 8;
          pcx_header.planes         = 1;
          pcx_header.bytesperline   = width;
        }
      else if (colors > 2)
        {
          pcx_header.bpp            = 4;
          pcx_header.planes         = 1;
          pcx_header.bytesperline   = (width + 1) / 2;
        }
      else
       {
          pcx_header.bpp            = 1;
          pcx_header.planes         = 1;
          pcx_header.bytesperline   = (width + 7) / 8;
        }
      pcx_header.color        = GUINT16_TO_LE (1);
      format                  = NULL;

      /* Some references explain that 2bpp/1plane and 4bpp/1plane files
       * would use the palette at EOF (not the one from the header) if
       * we are in version 5 of PCX. Other sources affirm that even in
       * version 5, EOF palette must be used only when there are more
       * than 16 colors. We go with this second assumption.
       * See bug 159947, comment 21 and 23.
       */
      if (colors <= 16)
        {
          for (i = 0; i < (colors * 3); i++)
            {
              pcx_header.colormap[i] = cmap[i];
            }
        }

      break;

    case GIMP_RGB_IMAGE:
      pcx_header.bpp          = 8;
      pcx_header.planes       = 3;
      pcx_header.color        = GUINT16_TO_LE (1);
      pcx_header.bytesperline = width;
      format                  = babl_format ("R'G'B' u8");
      break;

    case GIMP_GRAY_IMAGE:
      pcx_header.bpp          = 8;
      pcx_header.planes       = 1;
      pcx_header.color        = GUINT16_TO_LE (2);
      pcx_header.bytesperline = width;
      format                  = babl_format ("Y' u8");
      break;

    default:
      g_message (_("Cannot export images with alpha channel."));
      return FALSE;
  }

  /* Bytes per Line must be an even number, according to spec */
  if (pcx_header.bytesperline % 2 != 0)
    {
      pcx_header.bytesperline++;
      padding = TRUE;
    }
  pcx_header.bytesperline = GUINT16_TO_LE (pcx_header.bytesperline);

  pixels = (guchar *) g_malloc (width * height * pcx_header.planes);

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, width, height), 1.0,
                   format, pixels,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if ((offset_x < 0) || (offset_x > (1<<16)))
    {
      g_message (_("Invalid X offset: %d"), offset_x);
      return FALSE;
    }

  if ((offset_y < 0) || (offset_y > (1<<16)))
    {
      g_message (_("Invalid Y offset: %d"), offset_y);
      return FALSE;
    }

  if (offset_x + width - 1 > (1<<16))
    {
      g_message (_("Right border out of bounds (must be < %d): %d"), (1<<16),
                 offset_x + width - 1);
      return FALSE;
    }

  if (offset_y + height - 1 > (1<<16))
    {
      g_message (_("Bottom border out of bounds (must be < %d): %d"), (1<<16),
                 offset_y + height - 1);
      return FALSE;
    }

  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  pcx_header.x1 = GUINT16_TO_LE ((guint16)offset_x);
  pcx_header.y1 = GUINT16_TO_LE ((guint16)offset_y);
  pcx_header.x2 = GUINT16_TO_LE ((guint16)(offset_x + width - 1));
  pcx_header.y2 = GUINT16_TO_LE ((guint16)(offset_y + height - 1));

  gimp_image_get_resolution (image, &resolution_x, &resolution_y);

  pcx_header.hdpi = GUINT16_TO_LE (RINT (MAX (resolution_x, 1.0)));
  pcx_header.vdpi = GUINT16_TO_LE (RINT (MAX (resolution_y, 1.0)));
  pcx_header.reserved = 0;

  pcx_header_to_buffer (header_buf);

  fwrite (header_buf, 128, 1, fp);

  switch (drawable_type)
    {
    case GIMP_INDEXED_IMAGE:
      if (colors > 16)
        {
          save_8 (fp, width, height, pixels, padding);
          fputc (0x0c, fp);
          fwrite (cmap, colors, 3, fp);
          for (i = colors; i < 256; i++)
            {
              fputc (0, fp);
              fputc (0, fp);
              fputc (0, fp);
            }
        }
      else /* Covers 1 and 4 bpp */
        {
          save_less_than_8 (fp, width, height, pcx_header.bpp, pixels, padding);
        }
      break;

    case GIMP_RGB_IMAGE:
      save_24 (fp, width, height, pixels, padding);
      break;

    case GIMP_GRAY_IMAGE:
      save_8 (fp, width, height, pixels, padding);
      fputc (0x0c, fp);
      for (i = 0; i < 256; i++)
        {
          fputc ((guchar) i, fp);
          fputc ((guchar) i, fp);
          fputc ((guchar) i, fp);
        }
      break;

    default:
      return FALSE;
    }

  g_object_unref (buffer);
  g_free (pixels);

  if (fclose (fp) != 0)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Writing to file '%s' failed: %s"),
                   gimp_file_get_utf8_name (file), g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

static void
save_less_than_8 (FILE         *fp,
                  gint          width,
                  gint          height,
                  const gint    bpp,
                  const guchar *buf,
                  gboolean      padding)
{
  const gint  bit_limit     = (8 - bpp);
  const gint  buf_size      = width * height;
  const gint  line_end      = width - 1;
  gint        j             = bit_limit;
  gint        count         = 0;
  guchar      byte_to_write = 0x00;
  guchar     *line;
  gint        x;

  line = (guchar *) g_malloc (((width + 7) / 8) * bpp);

  for (x = 0; x < buf_size; x++)
    {
      byte_to_write |= (buf[x] << j);
      j -= bpp;

      if (j < 0 || (x % width == line_end))
        {
          line[count] = byte_to_write;
          count++;
          byte_to_write = 0x00;
          j = bit_limit;

          if ((x % width == line_end))
            {
              writeline (fp, line, count);
              count = 0;
              if (padding)
                fputc ('\0', fp);
              gimp_progress_update ((double) x / (double) buf_size);
            }
        }
    }
  g_free (line);
}

static void
save_8 (FILE         *fp,
        gint          width,
        gint          height,
        const guchar *buf,
        gboolean      padding)
{
  int row;

  for (row = 0; row < height; ++row)
    {
      writeline (fp, buf, width);
      buf += width;
      if (padding)
        fputc ('\0', fp);
      gimp_progress_update ((double) row / (double) height);
    }
}

static void
save_24 (FILE         *fp,
         gint          width,
         gint          height,
         const guchar *buf,
         gboolean      padding)
{
  int     x, y, c;
  guchar *line;

  line = (guchar *) g_malloc (width);

  for (y = 0; y < height; ++y)
    {
      for (c = 0; c < 3; ++c)
        {
          for (x = 0; x < width; ++x)
            {
              line[x] = buf[(3*x) + c];
            }
          writeline (fp, line, width);
          if (padding)
            fputc ('\0', fp);
        }
      buf += width * 3;
      gimp_progress_update ((double) y / (double) height);
    }
  g_free (line);
}

static void
writeline (FILE         *fp,
           const guchar *buf,
           gint          bytes)
{
  const guchar *finish = buf + bytes;
  guchar        value;
  guchar        count;

  while (buf < finish)
    {
      value = *(buf++);
      count = 1;

      while (buf < finish && count < 63 && *buf == value)
        {
          count++; buf++;
        }

      if (value < 0xc0 && count == 1)
        {
          fputc (value, fp);
        }
      else
        {
          fputc (0xc0 + count, fp);
          fputc (value, fp);
        }
    }
}
