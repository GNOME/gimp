/* The GIMP -- an image manipulation program
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

/*  saves and loads gimp icon files (For toolbox, etc)...
 */

#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include <netinet/in.h>


typedef struct
{
  gchar icon_name[256];
} GIconSaveVals;

typedef struct
{
  gint  run;
} GIconSaveInterface;

/* Declare some local functions.
 */
static void   query       (void);
static void   run         (char    *name,
			   int      nparams,
			   GParam  *param,
			   int     *nreturn_vals,
			   GParam **return_vals);
static gint32 load_image  (char   *filename);
static gint   save_image  (char   *filename,
			   gint32  image_ID,
			   gint32  drawable_ID);

static gint   save_dialog    (void);

static void   close_callback (GtkWidget *widget,
			      gpointer   data);
static void   ok_callback    (GtkWidget *widget,
			      gpointer   data);
static void   entry_callback (GtkWidget *widget,
			      gpointer   data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

static GIconSaveVals givals =
{
  "gicon" /*  icon_name  */
};

static GIconSaveInterface giint =
{
  FALSE   /*  run  */
};

MAIN ()

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static int nload_return_vals = sizeof (load_return_vals) / sizeof (load_return_vals[0]);

  static GParamDef save_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE, "image", "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING, "filename", "The name of the file to save the image in" },
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" },
    { PARAM_STRING, "icon_name", "The name of the icon" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_gicon_load",
                          "loads files of the .ico file format",
                          "FIXME: write help",
			  "Spencer Kimball",
			  "Spencer Kimball",
                          "1997",
                          "<Load>/GIcon",
                          NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_gicon_save",
                          "saves files in the .ico file format",
                          "FIXME: write help",
			  "Spencer Kimball",
			  "Spencer Kimball",
                          "1997",
                          "<Save>/GIcon",
                          "GRAY*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_load_handler ("file_gicon_load", "ico", "");
  gimp_register_save_handler ("file_gicon_save", "ico", "");
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  gint32 image_ID;
	GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *return_vals = values;
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;
  values[1].type = PARAM_IMAGE;
  values[1].data.d_image = -1;

  if (strcmp (name, "file_gicon_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string);

      if (image_ID != -1)
	{
	  values[0].data.d_status = STATUS_SUCCESS;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
      *nreturn_vals = 2;
    }
  else if (strcmp (name, "file_gicon_save") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_gicon_save", &givals);

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    return;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 6)
	    status = STATUS_CALLING_ERROR;
	  else
	    strncpy (givals.icon_name, param[5].data.d_string, 256);
	  break;

	case RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_gicon_save", &givals);
	  break;

	default:
	  break;
	}

      if (save_image (param[3].data.d_string,
		      param[1].data.d_int32,
		      param[2].data.d_int32))
	{
	  /*  Store persistent data  */
	  gimp_set_data ("file_gicon_save", &givals, sizeof (GIconSaveVals));

	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
      *nreturn_vals = 1;
    }
}

static gint32
load_image (char *filename)
{
  GDrawable *drawable;
  GPixelRgn pixel_rgn;
  FILE * fp;
  gint32 image_ID;
  gint32 layer_ID;
  char name_buf[256];
  char * data_buf;
  unsigned char *dest;
  int val;
  int width, height;
  int i, j;

  /*  Open the requested file  */
  if (! (fp = fopen (filename, "r")))
    {
      fprintf (stderr, "gicon: can't open \"%s\"\n", filename);
      return -1;
    }

  /*  Check the identifier string  */
  fscanf (fp, "/*  %s icon image format -- S. Kimball, P. Mattis  */\n", name_buf);
  if (strcmp ("GIMP", name_buf))
    {
      fprintf (stderr, "Not a GIcon file: %s!\n", filename);
      return -1;
    }

  /*  Parse the icon name  */
  fscanf (fp, "/*  Image name: %s  */\n", name_buf);

  /*  Get the width and height  */
  fscanf (fp, "#define %s %d\n", name_buf, &width);
  fscanf (fp, "#define %s %d\n", name_buf, &height);
  fscanf (fp, "static char *%s [] = \n{\n", name_buf);

  /*  Get a new image structure  */
  image_ID = gimp_image_new (width, height, GRAY);
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID, "Background", width, height,
			     GRAYA_IMAGE, 100, NORMAL_MODE);
  gimp_image_add_layer (image_ID, layer_ID, 0);

  drawable = gimp_drawable_get (layer_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, TRUE, FALSE);

  data_buf = g_new (char, width);
  dest     = g_new (guchar, width * 2);

  for (i = 0; i < height; i++)
    {
      fscanf (fp, "  \"%s\",\n", data_buf);
      for (j = 0; j < width; j++)
	{
	  val = data_buf[j];
	  if (val == '.')
	    {
	      dest[j*2+0] = 0;
	      dest[j*2+1] = 0;  /*  set alpha channel to transparent  */
	    }
	  else
	    {
	      dest[j*2+0] = (255 * (val - 'a')) / 7;
	      dest[j*2+1] = 255;  /*  set alpha channel to opaque  */
	    }
	}

      gimp_pixel_rgn_set_row (&pixel_rgn, dest, 0, i, width);
    }

  /*  Clean up  */
  fclose (fp);

  gimp_drawable_flush(drawable);

  g_free (data_buf);
  g_free (dest);

  return image_ID;
}

static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID)
{
  GDrawable *drawable;
  GPixelRgn  pixel_rgn;
  FILE * fp;
  int i, j;
  int w, h;
  int has_alpha;
  int val;
  char ch;
  unsigned char *src_buf, *src;

  drawable = gimp_drawable_get (drawable_ID);
  gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0, drawable->width,
		       drawable->height, FALSE, FALSE);

  w = drawable->width;
  h = drawable->height;
  has_alpha = gimp_drawable_has_alpha (drawable_ID);

  /*  open the file for writing  */
  if ((fp = fopen (filename, "w")))
    {
      fprintf (fp, "/*  GIMP icon image format -- S. Kimball, P. Mattis  */\n");
      fprintf (fp, "/*  Image name: %s  */\n", givals.icon_name);
      fprintf (fp, "\n\n");
      fprintf (fp, "#define %s_width %d\n", givals.icon_name, w);
      fprintf (fp, "#define %s_height %d\n", givals.icon_name, h);
      fprintf (fp, "static char *%s_bits [] = \n{\n", givals.icon_name);

      /*  write the brush data to the file  */
      src_buf = g_new (guchar, w * drawable->bpp);
      for (i = 0; i < h; i++)
	{
	  gimp_pixel_rgn_get_row (&pixel_rgn, src_buf, 0, i, w);
	  src = src_buf;

	  fprintf (fp, "  \"");
	  for (j = 0; j < w; j++)
	    {
	      if (has_alpha && !src[1])
		{
		  ch = '.';
		}
	      else
		{
		  val = (int) ((double) src[0] * 7.0 / 255.0 + 0.5);
		  ch = 'a' + val;
		}
	      fputc (ch, fp);

	      src += drawable->bpp;
	    }

	  fprintf (fp, (i == (h-1)) ? "\"\n};\n" : "\",\n");
	}

      /*  Clean up  */
      g_free (src_buf);

      fclose (fp);
    }

  return TRUE;
}


static gint
save_dialog()
{
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *table;
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new(gchar *, 1);
  argv[0] = g_strdup("gicon");

  gtk_init(&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Save As GIcon");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
		     (GtkSignalFunc) close_callback, NULL);

  /*  Action area  */
  button = gtk_button_new_with_label("OK");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) ok_callback,
		     dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
			    (GtkSignalFunc) gtk_widget_destroy,
			    GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  /* The main table */
  /* Set its size (y, x) */
  table = gtk_table_new(1, 2, FALSE);
  gtk_container_border_width(GTK_CONTAINER(table), 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show(table);

  gtk_table_set_row_spacings(GTK_TABLE(table), 10);
  gtk_table_set_col_spacings(GTK_TABLE(table), 10);

  /**********************
   * label
   **********************/
  label = gtk_label_new("Icon Name:");
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);

  /************************
   * The entry
   ************************/
  entry = gtk_entry_new();
  gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1, GTK_EXPAND | GTK_FILL,
		   GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_set_usize(entry, 200, 0);
  gtk_entry_set_text(GTK_ENTRY(entry), givals.icon_name);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc) entry_callback, givals.icon_name);
  gtk_widget_show(entry);


  gtk_widget_show(dlg);

  gtk_main();
  gdk_flush();

  return giint.run;
}

static void
close_callback (GtkWidget *widget,
		gpointer   data)
{
  gtk_main_quit();
}

static void
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  giint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
entry_callback (GtkWidget *widget,
		gpointer   data)
{
  strncpy(givals.icon_name, gtk_entry_get_text (GTK_ENTRY (widget)), 256);
}
