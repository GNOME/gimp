/*
 * pat plug-in version 1.01
 * Loads/saves version 1 GIMP .pat files, by Tim Newsome <drz@frody.bloke.com>
 *
 * GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "app/core/gimppattern-header.h"

#include "libgimp/stdplugins-intl.h"


#define LOAD_PROC      "file-pat-load"
#define SAVE_PROC      "file-pat-save"
#define PLUG_IN_BINARY "pat"


/*  local function prototypes  */

static void       query          (void);
static void       run            (const gchar      *name,
				  gint              nparams,
				  const GimpParam  *param,
				  gint             *nreturn_vals,
				  GimpParam       **return_vals);
static gint32     load_image     (const gchar      *filename);
static gboolean   save_image     (const gchar      *filename,
				  gint32            image_ID,
				  gint32            drawable_ID);

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
    { GIMP_PDB_INT32,  "run-mode",     "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename",     "The name of the file to load" },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load" }
  };
  static const GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" }
  };

  static const GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",     "Interactive, non-interactive"     },
    { GIMP_PDB_IMAGE,    "image",        "Input image"                      },
    { GIMP_PDB_DRAWABLE, "drawable",     "Drawable to save"                 },
    { GIMP_PDB_STRING,   "filename",     "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "raw-filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "description",  "Short description of the pattern" }
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

  gimp_plugin_icon_register (LOAD_PROC, GIMP_ICON_TYPE_STOCK_ID,
                             (const guint8 *) GIMP_STOCK_PATTERN);
  gimp_register_file_handler_mime (LOAD_PROC, "image/x-gimp-pat");
  gimp_register_magic_load_handler (LOAD_PROC,
				    "pat",
				    "",
				    "20,string,GPAT");

  gimp_install_procedure (SAVE_PROC,
                          "Saves Gimp pattern file (.PAT)",
                          "New Gimp patterns can be created by saving them "
			  "in the appropriate place with this plug-in.",
                          "Tim Newsome",
                          "Tim Newsome",
                          "1997",
                          N_("GIMP pattern"),
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (save_args), 0,
                          save_args, NULL);

  gimp_plugin_icon_register (SAVE_PROC, GIMP_ICON_TYPE_STOCK_ID,
                             (const guint8 *) GIMP_STOCK_PATTERN);
  gimp_register_file_handler_mime (SAVE_PROC, "image/x-gimp-pat");
  gimp_register_save_handler (SAVE_PROC, "pat", "");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam  values[2];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  gint32            drawable_ID;
  GimpExportReturn  export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  INIT_I18N ();

  if (strcmp (name, LOAD_PROC) == 0)
    {
      image_ID = load_image (param[1].data.d_string);

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
	  export = gimp_export_image (&image_ID, &drawable_ID, "PAT",
				      GIMP_EXPORT_CAN_HANDLE_GRAY |
				      GIMP_EXPORT_CAN_HANDLE_RGB |
                                      GIMP_EXPORT_CAN_HANDLE_ALPHA);
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }

	  /*  Possibly retrieve data  */
	  gimp_get_data (SAVE_PROC, description);
	  break;

	default:
	  break;
	}

      parasite = gimp_image_parasite_find (orig_image_ID, "gimp-pattern-name");
      if (parasite)
        {
          gchar *name = g_strndup (gimp_parasite_data (parasite),
                                   gimp_parasite_data_size (parasite));
          gimp_parasite_free (parasite);

          strncpy (description, name, sizeof (description));
          description[sizeof (description) - 1] = '\0';
        }

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  if (!save_dialog ())
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
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID))
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
          gimp_image_attach_new_parasite (orig_image_ID, "gimp-pattern-name",
                                          GIMP_PARASITE_PERSISTENT,
                                          strlen (description) + 1,
                                          description);
        }
      else
        {
          gimp_image_parasite_detach (orig_image_ID, "gimp-pattern-name");
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (const gchar *filename)
{
  gint              fd;
  PatternHeader     ph;
  gchar            *name;
  gchar            *temp;
  guchar           *buffer;
  gint32            image_ID;
  gint32            layer_ID;
  GimpDrawable     *drawable;
  gint              line;
  GimpPixelRgn      pixel_rgn;
  GimpImageBaseType base_type;
  GimpImageType     image_type;

  fd = g_open (filename, O_RDONLY | _O_BINARY, 0);

  if (fd == -1)
    {
      g_message (_("Could not open '%s' for reading: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return -1;
    }

  gimp_progress_init_printf (_("Opening '%s'"),
                             gimp_filename_to_utf8 (filename));

  if (read (fd, &ph, sizeof (PatternHeader)) != sizeof (PatternHeader))
    {
      close (fd);
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
      close (fd);
      return -1;
    }

  temp = g_new (gchar, ph.header_size - sizeof (PatternHeader));

  if (read (fd, temp, ph.header_size - sizeof (PatternHeader)) !=
      ph.header_size - sizeof (PatternHeader))
    {
      g_free (temp);
      close (fd);
      return -1;
    }

  name = gimp_any_to_utf8 (temp, -1,
                           _("Invalid UTF-8 string in pattern file '%s'."),
                           gimp_filename_to_utf8 (filename));
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
      break;
    case 2:
      base_type = GIMP_GRAY;
      image_type = GIMP_GRAYA_IMAGE;
      break;
    case 3:
      base_type = GIMP_RGB;
      image_type = GIMP_RGB_IMAGE;
      break;
    case 4:
      base_type = GIMP_RGB;
      image_type = GIMP_RGBA_IMAGE;
      break;
    default:
      g_message ("Unsupported pattern depth: %d\n"
                 "GIMP Patterns must be GRAY or RGB", ph.bytes);
      return -1;
    }

  image_ID = gimp_image_new (ph.width, ph.height, base_type);
  gimp_image_set_filename (image_ID, filename);

  gimp_image_attach_new_parasite (image_ID, "gimp-pattern-name",
                                  GIMP_PARASITE_PERSISTENT,
                                  strlen (name) + 1, name);

  layer_ID = gimp_layer_new (image_ID, name, ph.width, ph.height,
                             image_type, 100, GIMP_NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);

  g_free (name);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable,
                       0, 0, drawable->width, drawable->height,
                       TRUE, FALSE);

  buffer = g_malloc (ph.width * ph.bytes);

  for (line = 0; line < ph.height; line++)
    {
      if (read (fd, buffer, ph.width * ph.bytes) != ph.width * ph.bytes)
	{
	  close (fd);
	  g_free (buffer);
	  return -1;
	}

      gimp_pixel_rgn_set_row (&pixel_rgn, buffer, 0, line, ph.width);

      gimp_progress_update ((gdouble) line / (gdouble) ph.height);
    }

  gimp_drawable_flush (drawable);

  return image_ID;
}

static gboolean
save_image (const gchar *filename,
	    gint32       image_ID,
	    gint32       drawable_ID)
{
  gint          fd;
  PatternHeader ph;
  guchar       *buffer;
  GimpDrawable *drawable;
  gint          line;
  GimpPixelRgn  pixel_rgn;

  fd = g_open (filename, O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0644);

  if (fd == -1)
    {
      g_message (_("Could not open '%s' for writing: %s"),
                 gimp_filename_to_utf8 (filename), g_strerror (errno));
      return FALSE;
    }

  gimp_progress_init_printf (_("Saving '%s'"),
                             gimp_filename_to_utf8 (filename));

  drawable = gimp_drawable_get (drawable_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, FALSE, FALSE);

  ph.header_size  = g_htonl (sizeof (PatternHeader) + strlen (description) + 1);
  ph.version      = g_htonl (1);
  ph.width        = g_htonl (drawable->width);
  ph.height       = g_htonl (drawable->height);
  ph.bytes        = g_htonl (drawable->bpp);
  ph.magic_number = g_htonl (GPATTERN_MAGIC);

  if (write (fd, &ph, sizeof (PatternHeader)) != sizeof (PatternHeader))
    {
      close (fd);
      return FALSE;
    }

  if (write (fd, description, strlen (description) + 1) != strlen (description) + 1)
    {
      close (fd);
      return FALSE;
    }

  buffer = g_malloc (drawable->width * drawable->bpp);
  if (buffer == NULL)
    {
      close (fd);
      return FALSE;
    }

  for (line = 0; line < drawable->height; line++)
    {
      gimp_pixel_rgn_get_row (&pixel_rgn, buffer, 0, line, drawable->width);

      if (write (fd, buffer, drawable->width * drawable->bpp) !=
	  drawable->width * drawable->bpp)
	{
	  close (fd);
	  return FALSE;
	}

      gimp_progress_update ((gdouble) line / (gdouble) drawable->height);
    }

  g_free (buffer);
  close (fd);

  return TRUE;
}

static gboolean
save_dialog (void)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *entry;
  gboolean   run;

  dialog = gimp_dialog_new (_("Save as Pattern"), PLUG_IN_BINARY,
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

  /* The main table */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), description);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Description:"), 1.0, 0.5,
			     entry, 1, FALSE);
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
