/* Plug-in to save .gpb (GIMP Pixmap Brush) files.
 * This is probably only of temporary interest, the format for
 * pixmap brushes is likely to change.
 *
 * Copyright (C) 1999 Tor Lillqvist
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>

#include "app/brush_header.h"
#include "app/pattern_header.h"

/* Declare local data types
 */

typedef struct {
  guint spacing;
  guchar *description;
} t_info;

t_info info = {   /* Initialize to this, change if non-interactive later */
  10,
  "GIMP Pixmap Brush"
};

static int run_flag = 0;

/* Declare some local functions.
 */
static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ()

static void
query ()
{
  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_INT32, "spacing", "Spacing of the brush" },
    { PARAM_STRING, "description", "Short description of the brush" },
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_gpb_save",
                          "saves a layer in the GIMP pixmap brush format",
                          "The layer must have an alpha channel",
                          "Tor Lillqvist",
                          "Tor Lillqvist",
                          "1999",
                          "<Save>/GPB",
			  "RGBA",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_save_handler ("file_gpb_save", "gpb", "");
}

static void
entry_callback(GtkWidget *widget,
	       gpointer   data)
{
  if (data == &info.description)
    info.description = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
  else if (data == &info.spacing)
    info.spacing = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
close_callback (GtkWidget *widget,
		gpointer   data)
{
  gtk_main_quit();
}

static void
ok_callback (GtkWidget *widget,
	     gpointer    data)
{
  run_flag = 1;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static gint
save_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *table;
  gchar **argv;
  gint argc;
  gchar buffer[12];

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("gpb_save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW(dlg), "Save As Pixmap Brush");
  gtk_window_position (GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT(dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT(button), "clicked",
		      (GtkSignalFunc) ok_callback, dlg);
  gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT(button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  /* The main table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  label = gtk_label_new ("Spacing:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0,
		    0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
		   GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 200, 0);
  sprintf (buffer, "%i", info.spacing);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_callback, &info.spacing);
  gtk_widget_show (entry);

  label = gtk_label_new ("Description:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0,
		   0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2, GTK_EXPAND | GTK_FILL,
		   GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_usize (entry, 200, 0);
  gtk_entry_set_text (GTK_ENTRY (entry), info.description);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entry_callback, &info.description);
  gtk_widget_show (entry);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}

static gboolean
try_fwrite (gpointer buffer,
	    guint   size,
	    guint   nitems,
	    FILE   *file)
{
  if (fwrite (buffer, size, nitems, file) < nitems)
    {
      g_message ("GPB: write error");
      fclose (file);
      return FALSE;
    }
  return TRUE;
}

static gboolean
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  BrushHeader brushheader;
  PatternHeader patternheader;
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  FILE *file;
  gint y, x;
  guchar *buffer;
  gchar *temp;

  g_assert (gimp_drawable_has_alpha (drawable_ID));

  drawable = gimp_drawable_get (drawable_ID);
  gimp_tile_cache_size (gimp_tile_height () * drawable->width * 4);

  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width, drawable->height, FALSE, FALSE);

  temp = g_strdup_printf ("Saving %s:", filename);
  gimp_progress_init (temp);
  g_free (temp);

  file = fopen (filename, "wb");

  if (file == NULL)
    {
      g_message ("GPB: can't open \"%s\"", filename);
      return FALSE;
    }

  brushheader.header_size = g_htonl (sizeof (brushheader) + strlen (info.description));
  brushheader.version = g_htonl (2);
  brushheader.width = g_htonl (drawable->width);
  brushheader.height = g_htonl (drawable->height);
  brushheader.bytes = g_htonl (1);
  brushheader.magic_number = g_htonl (GBRUSH_MAGIC);
  brushheader.spacing = g_htonl (info.spacing);

  if (!try_fwrite (&brushheader, sizeof (brushheader), 1, file))
    return FALSE;
  if (!try_fwrite (info.description, strlen (info.description), 1, file))
    return FALSE;

  buffer = g_malloc (drawable->width * drawable->bpp);
  for (y = 0; y < drawable->height; y++)
    {
      gimp_pixel_rgn_get_row (&pixel_rgn, buffer, 0, y, drawable->width);
      for (x = 0; x < drawable->width; x++)
	if (!try_fwrite (buffer + x*4 + 3, 1, 1, file))
	  return FALSE;
      gimp_progress_update (0.5 * y / drawable->height);
    }
  g_free (buffer);

  gimp_progress_update (0.5);

  patternheader.header_size = g_htonl (sizeof (patternheader) + strlen ("foo"));
  patternheader.version = g_htonl (1);
  patternheader.width = g_htonl (drawable->width);
  patternheader.height = g_htonl (drawable->height);
  patternheader.bytes = g_htonl (3);
  patternheader.magic_number = g_htonl (GPATTERN_MAGIC);

  if (!try_fwrite (&patternheader, sizeof (patternheader), 1, file))
    return FALSE;
  if (!try_fwrite ("foo", strlen ("foo"), 1, file))
    return FALSE;

  buffer = g_malloc (drawable->width * drawable->bpp);

  for (y = 0; y < drawable->height; y++)
    {
      gimp_pixel_rgn_get_row (&pixel_rgn, buffer, 0, y, drawable->width);
      for (x = 0; x < drawable->width; x++)
	if (!try_fwrite (buffer + x*4, 3, 1, file))
	  return FALSE;
      gimp_progress_update (0.5 + 0.5 * y / drawable->height);
    }
  g_free (buffer);

  gimp_progress_update (1.0);

  fclose (file);

  return TRUE;
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *return_vals = values;
  *nreturn_vals = 1;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_gpb_save") == 0)
    {
      switch (run_mode) {
      case RUN_INTERACTIVE:
	/*  Possibly retrieve data  */
	gimp_get_data ("file_gpb_save", &info);
	if (!save_dialog ())
	  return;
	break;

      case RUN_NONINTERACTIVE:  /* FIXME - need a real RUN_NONINTERACTIVE */
	if (nparams != 7)
	  status = STATUS_CALLING_ERROR;
	if (status == STATUS_SUCCESS)
	  {
	    info.spacing = param[5].data.d_int32;
	    info.description = g_strdup (param[6].data.d_string);
	  }
	break;

      case RUN_WITH_LAST_VALS:
	gimp_get_data ("file_gpb_save", &info);
	break;
      }

      if (save_image (param[3].data.d_string, param[1].data.d_int32, param[2].data.d_int32))
	{
	  gimp_set_data ("file_gpb_save", &info, sizeof (info));
	  status = STATUS_SUCCESS;
	}
      else
	status = STATUS_EXECUTION_ERROR;
    }
  values[0].data.d_status = status;
}
