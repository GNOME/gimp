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

#include "config.h"

#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


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
static void   query         (void);
static void   run           (gchar   *name,
			     gint     nparams,
			     GimpParam  *param,
			     gint    *nreturn_vals,
			     GimpParam **return_vals);
static gint32 load_image    (gchar   *filename);
static gint   save_image    (gchar   *filename,
			     gint32   image_ID,
			     gint32   drawable_ID);

static gint   save_dialog    (void);

static void   ok_callback    (GtkWidget *widget,
			      gpointer   data);
static void   entry_callback (GtkWidget *widget,
			      gpointer   data);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
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
query (void)
{
  static GimpParamDef load_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_STRING, "filename", "The name of the file to load" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to load" }
  };
  static GimpParamDef load_return_vals[] =
  {
    { GIMP_PDB_IMAGE, "image", "Output image" },
  };
  static gint nload_args = sizeof (load_args) / sizeof (load_args[0]);
  static gint nload_return_vals = (sizeof (load_return_vals) /
				   sizeof (load_return_vals[0]));

  static GimpParamDef save_args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save" },
    { GIMP_PDB_STRING, "filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING, "raw_filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING, "icon_name", "The name of the icon" }
  };
  static gint nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_gicon_load",
                          "loads files of the .ico file format",
                          "FIXME: write help",
			  "Spencer Kimball",
			  "Spencer Kimball",
                          "1997",
                          "<Load>/GIcon",
                          NULL,
                          GIMP_PLUGIN,
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
                          GIMP_PLUGIN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_magic_load_handler ("file_gicon_load",
				    "ico",
				    "",
				    "0,string,"
				    "/*\\040\\040GIMP\\040icon\\040image");
  gimp_register_save_handler ("file_gicon_save",
			      "ico",
			      "");
}

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpRunModeType  run_mode;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  gint32        image_ID;
  gint32        drawable_ID;
  GimpExportReturnType export = GIMP_EXPORT_CANCEL;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "file_gicon_load") == 0)
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
  else if (strcmp (name, "file_gicon_save") == 0)
    {
      image_ID    = param[1].data.d_int32;
      drawable_ID = param[2].data.d_int32;

      /*  eventually export the image */ 
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	case GIMP_RUN_WITH_LAST_VALS:
	  INIT_I18N_UI();
	  gimp_ui_init ("gicon", FALSE);
	  export = gimp_export_image (&image_ID, &drawable_ID, "GIcon", 
				      (GIMP_EXPORT_CAN_HANDLE_GRAY |
				       GIMP_EXPORT_CAN_HANDLE_ALPHA ));
	  if (export == GIMP_EXPORT_CANCEL)
	    {
	      values[0].data.d_status = GIMP_PDB_CANCEL;
	      return;
	    }
	  break;
	default:
	  INIT_I18N();
	  break;
	}

      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_gicon_save", &givals);

	  /*  First acquire information with a dialog  */
	  if (! save_dialog ())
	    status = GIMP_PDB_CANCEL;
	  break;

	case GIMP_RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 6)
	    {
	      status = GIMP_PDB_CALLING_ERROR;
	    }
	  else
	    {
	      strncpy (givals.icon_name, param[5].data.d_string, 256);
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  /*  Possibly retrieve data  */
	  gimp_get_data ("file_gicon_save", &givals);
	  break;

	default:
	  break;
	}

      if (status == GIMP_PDB_SUCCESS)
	{
	  if (save_image (param[3].data.d_string, image_ID, drawable_ID))
	    {
	      /*  Store persistent data  */
	      gimp_set_data ("file_gicon_save", &givals, sizeof (GIconSaveVals));
	    }
	  else
	    {
	      status = GIMP_PDB_EXECUTION_ERROR;
	    }
	}

      if (export == GIMP_EXPORT_EXPORT)
	gimp_image_delete (image_ID);
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static gint32
load_image (gchar *filename)
{
  GimpDrawable *drawable;
  GimpPixelRgn pixel_rgn;
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
  image_ID = gimp_image_new (width, height, GIMP_GRAY);
  gimp_image_set_filename (image_ID, filename);

  layer_ID = gimp_layer_new (image_ID, "Background", width, height,
			     GIMP_GRAYA_IMAGE, 100, GIMP_NORMAL_MODE);
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
  GimpDrawable *drawable;
  GimpPixelRgn  pixel_rgn;
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
save_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *table;
  GtkWidget *entry;

  dlg = gimp_dialog_new (_("Save as GIcon"), "gicon",
			 gimp_standard_help_func, "filters/gicon.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* The main table */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);


  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Icon Name:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry), givals.icon_name);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (entry_callback),
		      givals.icon_name);
  gtk_widget_show (entry);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return giint.run;
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
  strncpy (givals.icon_name, gtk_entry_get_text (GTK_ENTRY (widget)), 256);
}
