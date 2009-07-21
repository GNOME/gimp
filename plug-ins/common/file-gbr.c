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
 * Loads/saves version 2 GIMP .gbr files, by Tim Newsome <drz@frody.bloke.com>
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

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>

#include <glib.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "app/core/gimpbrush-header.h"
#include "app/core/gimppattern-header.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-gbr-load"
#define SAVE_PROC      "file-gbr-save"
#define PLUG_IN_BINARY "file-gbr"


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

static gint32     load_image     (const gchar      *filename,
                                  GError          **error);
static gboolean   save_image     (const gchar      *filename,
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
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE,  "image",        "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",        "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
    { GIMP_PDB_INT32,    "spacing",      "Spacing of the brush" },
    { GIMP_PDB_STRING,   "description",  "Short description of the brush" }
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

  gimp_plugin_icon_register (LOAD_PROC, GIMP_ICON_TYPE_STOCK_ID,
                             (const guint8 *) GIMP_STOCK_BRUSH);
  gimp_register_file_handler_mime (LOAD_PROC, "image/x-gimp-gbr");
  gimp_register_magic_load_handler (LOAD_PROC,
                                    "gbr, gpb",
                                    "",
                                    "20, string, GIMP");

  gimp_install_procedure (SAVE_PROC,
                          "Saves files in the GIMP brush file format",
                          "Saves files in the GIMP brush file format",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "Tim Newsome, Jens Lautenbacher, Sven Neumann",
                          "1997-2000",
                          N_("GIMP brush"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_plugin_icon_register (SAVE_PROC, GIMP_ICON_TYPE_STOCK_ID,
                             (const guint8 *) GIMP_STOCK_BRUSH);
  gimp_register_file_handler_mime (SAVE_PROC, "image/x-gimp-gbr");
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

  run_mode = param[0].data.d_int32;

  INIT_I18N ();

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string, &error);

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
      GimpParasite *parasite;
      gint32        orig_image_ID;

      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      orig_image_ID = image_ID;

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          export = gimp_export_image (&image_ID, &drawable_ID, NULL,
                                      GIMP_EXPORT_CAN_HANDLE_GRAY  |
                                      GIMP_EXPORT_CAN_HANDLE_RGB   |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);
          if (export == GIMP_EXPORT_CANCEL)
            {
              values[0].data.d_status = GIMP_PDB_CANCEL;
              return;
            }

          /*  Possibly retrieve data  */
          gimp_get_data (SAVE_PROC, &info);
          break;

        default:
          break;
        }

      parasite = gimp_image_parasite_find (orig_image_ID, "gimp-brush-name");
      if (parasite)
        {
          gchar *name = g_strndup (gimp_parasite_data (parasite),
                                   gimp_parasite_data_size (parasite));
          gimp_parasite_free (parasite);

          strncpy (info.description, name, sizeof (info.description));
          info.description[sizeof (info.description) - 1] = '\0';
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
          if (save_image (param[3].data.d_string, image_ID, drawable_ID,
                          &error))
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

      if (info.description && strlen (info.description))
        {
          gimp_image_attach_new_parasite (orig_image_ID, "gimp-brush-name",
                                          GIMP_PARASITE_PERSISTENT,
                                          strlen (info.description) + 1,
                                          info.description);
        }
      else
        {
          gimp_image_parasite_detach (orig_image_ID, "gimp-brush-name");
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
load_image (const gchar  *filename,
            GError      **error)
{
  gchar             *name;
  gint               fd;
  BrushHeader        bh;
  guchar            *brush_buf = NULL;
  gint32             image_ID;
  gint32             layer_ID;
  GimpDrawable      *drawable;
  GimpPixelRgn       pixel_rgn;
  gint               bn_size;
  GimpImageBaseType  base_type;
  GimpImageType      image_type;
  gssize             size;

  fd = g_open (filename, O_RDONLY | _O_BINARY, 0);

  if (fd == -1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (read (fd, &bh, sizeof (BrushHeader)) != sizeof (BrushHeader))
    {
      close (fd);
      return -1;
    }

  /*  rearrange the bytes in each unsigned int  */
  bh.header_size  = g_ntohl (bh.header_size);
  bh.version      = g_ntohl (bh.version);
  bh.width        = g_ntohl (bh.width);
  bh.height       = g_ntohl (bh.height);
  bh.bytes        = g_ntohl (bh.bytes);
  bh.magic_number = g_ntohl (bh.magic_number);
  bh.spacing      = g_ntohl (bh.spacing);

  switch (bh.version)
    {
    case 1:
      /* Version 1 didn't have a magic number and had no spacing  */
      bh.spacing = 25;
      /* And we need to rewind the handle, 4 due spacing and 4 due magic */
      lseek (fd, -8, SEEK_CUR);
      bh.header_size += 8;
      break;

    case 3: /*  cinepaint brush  */
      if (bh.bytes == 18 /* FLOAT16_GRAY_GIMAGE */)
        {
          bh.bytes = 2;
        }
      else
        {
          g_message (_("Unsupported brush format"));
          close (fd);
          return -1;
        }
      /*  fallthrough  */

    case 2:
      if (bh.magic_number == GBRUSH_MAGIC &&
          bh.header_size  >  sizeof (BrushHeader))
        break;

    default:
      g_message (_("Unsupported brush format"));
      close (fd);
      return -1;
    }

  if ((bn_size = (bh.header_size - sizeof (BrushHeader))) > 0)
    {
      gchar *temp = g_new (gchar, bn_size);

      if ((read (fd, temp, bn_size)) < bn_size)
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Error in GIMP brush file '%s'"),
                       gimp_filename_to_utf8 (filename));
          close (fd);
          g_free (temp);
          return -1;
        }

      name = gimp_any_to_utf8 (temp, -1,
                               _("Invalid UTF-8 string in brush file '%s'."),
                               gimp_filename_to_utf8 (filename));
      g_free (temp);
    }
  else
    {
      name = g_strdup (_("Unnamed"));
    }

  /* Now there's just raw data left. */

  size = bh.width * bh.height * bh.bytes;
  brush_buf = g_malloc (size);

  if (read (fd, brush_buf, size) != size)
    {
      close (fd);
      g_free (brush_buf);
      g_free (name);
      return -1;
    }

  switch (bh.bytes)
    {
    case 1:
      {
        PatternHeader ph;

        /*  For backwards-compatibility, check if a pattern follows.
            The obsolete .gpb format did it this way.  */

        if (read (fd, &ph, sizeof (PatternHeader)) == sizeof(PatternHeader))
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
                lseek (fd, ph.header_size - sizeof (PatternHeader),
                       SEEK_CUR) > 0)
              {
                guchar *plain_brush = brush_buf;
                gint    i;

                bh.bytes = 4;
                brush_buf = g_malloc (4 * bh.width * bh.height);

                for (i = 0; i < ph.width * ph.height; i++)
                  {
                    if (read (fd, brush_buf + i * 4, 3) != 3)
                      {
                        close (fd);
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
        gint     i;

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
      break;

    case 4:
      base_type = GIMP_RGB;
      image_type = GIMP_RGBA_IMAGE;
      break;

    default:
      g_message ("Unsupported brush depth: %d\n"
                 "GIMP Brushes must be GRAY or RGBA\n",
                 bh.bytes);
      return -1;
    }

  image_ID = gimp_image_new (bh.width, bh.height, base_type);
  gimp_image_set_filename (image_ID, filename);

  gimp_image_attach_new_parasite (image_ID, "gimp-brush-name",
                                  GIMP_PARASITE_PERSISTENT,
                                  strlen (name) + 1, name);

  layer_ID = gimp_layer_new (image_ID, name, bh.width, bh.height,
                             image_type, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);

  g_free (name);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0, drawable->width, drawable->height,
                       TRUE, FALSE);

  gimp_pixel_rgn_set_rect (&pixel_rgn, brush_buf,
                           0, 0, bh.width, bh.height);
  g_free (brush_buf);

  if (image_type == GIMP_GRAY_IMAGE)
    gimp_invert (layer_ID);

  close (fd);

  gimp_drawable_flush (drawable);
  gimp_progress_update (1.0);

  return image_ID;
}

static gboolean
save_image (const gchar  *filename,
            gint32        image_ID,
            gint32        drawable_ID,
            GError      **error)
{
  gint          fd;
  BrushHeader   bh;
  guchar       *buffer;
  GimpDrawable *drawable;
  gint          line;
  gint          x;
  gint          bpp;
  GimpPixelRgn  pixel_rgn;

  switch (gimp_drawable_type (drawable_ID))
    {
    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      bpp = 4;
      break;

    case GIMP_GRAY_IMAGE:
    case GIMP_GRAYA_IMAGE:
      bpp = 1;
      break;

    default:
      {
        g_message (_("GIMP brushes are either GRAYSCALE or RGBA"));
        return FALSE;
      }
    }

  fd = g_open (filename, O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0666);

  if (fd == -1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Could not open '%s' for writing: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  drawable = gimp_drawable_get (drawable_ID);

  bh.header_size  = g_htonl (sizeof (BrushHeader) +
                             strlen (info.description) + 1);
  bh.version      = g_htonl (2);
  bh.width        = g_htonl (drawable->width);
  bh.height       = g_htonl (drawable->height);
  bh.bytes        = g_htonl (bpp);
  bh.magic_number = g_htonl (GBRUSH_MAGIC);
  bh.spacing      = g_htonl (info.spacing);

  if (write (fd, &bh, sizeof (BrushHeader)) != sizeof (BrushHeader))
    {
      close (fd);
      return FALSE;
    }

  if (write (fd, info.description, strlen (info.description) + 1) !=
      strlen (info.description) + 1)
    {
      close (fd);
      return FALSE;
    }

  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0, drawable->width, drawable->height,
                       FALSE, FALSE);

  buffer = g_new (guchar, drawable->width * bpp);

  for (line = 0; line < drawable->height; line++)
    {
      gimp_pixel_rgn_get_row (&pixel_rgn, buffer, 0, line, drawable->width);

      switch (drawable->bpp)
        {
        case 1:
          /*  invert  */
          for (x = 0; x < drawable->width; x++)
            buffer[x] = 255 - buffer[x];
          break;

        case 2:
          /*  invert and drop alpha channel  */
          for (x = 0; x < drawable->width; x++)
            buffer[x] = 255 - buffer[2 * x];
          break;

        case 3:
          /*  add alpha channel  */
          for (x = drawable->width - 1; x >= 0; x--)
            {
              buffer[x * 4 + 3] = 0xFF;
              buffer[x * 4 + 2] = buffer[x * 3 + 2];
              buffer[x * 4 + 1] = buffer[x * 3 + 1];
              buffer[x * 4 + 0] = buffer[x * 3 + 0];
            }
          break;
        }

      if (write (fd, buffer, drawable->width * bpp) != drawable->width * bpp)
        {
          g_free (buffer);
          close (fd);
          return FALSE;
        }

      gimp_progress_update ((gdouble) line / (gdouble) drawable->height);
    }

  g_free (buffer);

  gimp_drawable_detach (drawable);

  close (fd);

  return TRUE;
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *entry;
  GtkWidget *spinbutton;
  GtkObject *adj;
  gboolean   run;

  dialog = gimp_export_dialog_new (_("Brush"), PLUG_IN_BINARY, SAVE_PROC);

  /* The main table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (gimp_export_dialog_get_content_area (dialog)),
                      table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  spinbutton = gimp_spin_button_new (&adj,
                                     info.spacing, 1, 1000, 1, 10, 0, 1, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Spacing:"), 1.0, 0.5,
                             spinbutton, 1, TRUE);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &info.spacing);

  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Description:"), 1.0, 0.5,
                             entry, 1, FALSE);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (entry_callback),
                    info.description);

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
