/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Face saver reading and writing code Copyright 1998 by John Kodis
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * fs.c -- a facesaver/faceserver plugin for the gimp.  Developed by
 * John Kodis, Dec 1997.  Bug reports are welcomed at kodis@jagunet.com
 *
 * Based on HRZ code by Albert Cahalan <acahalan at cs.uml.edu>, 
 * which was in turn based on PNM code by Erik Nygren (nygren@mit.edu).
 */

#include <string.h>

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

static int save_okay = FALSE;

static void query()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32,  "run_mode",     "Interactive, non-interactive" },
    { PARAM_STRING, "filename",     "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name of the file to load" },
  };
  static int nload_args = sizeof (load_args) / sizeof (load_args[0]);

  static GParamDef load_return_vals[] =
  {
    { PARAM_IMAGE, "image", "Output image" },
  };
  static int nload_return_vals = 
    sizeof(load_return_vals) / sizeof(*load_return_vals);

  static GParamDef save_args[] =
  {
    { PARAM_INT32,    "run_mode", "Interactive, non-interactive" },
    { PARAM_IMAGE,    "image",    "Input image" },
    { PARAM_DRAWABLE, "drawable", "Drawable to save" },
    { PARAM_STRING,   "filename", "Name of the file to save the image in" },
    { PARAM_STRING,   "raw_filename", "Name of the file to save the image in" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure(
      "file_fs_load",
      "loads files in face saver file format",
      "FIXME: write help for file_fs_load",
      "John Kodis", "John Kodis", "1997",
      "<Load>/FS",
      NULL, PROC_PLUG_IN,
      nload_args, nload_return_vals,
      load_args, load_return_vals);

  gimp_install_procedure(
      "file_fs_save",
      "saves files in face saver file format",
      "saves a grayscale image in facesaver format",
      "John Kodis", "John Kodis", "1997",
      "<Save>/FS",
      "GRAY*", PROC_PLUG_IN,
      nsave_args, 0,
      save_args, NULL);

  gimp_register_magic_load_handler(
      "file_fs_load", "fs", "", "");
  gimp_register_save_handler(
      "file_fs_save", "fs", "");
}

typedef struct 
{
  int ix, iy, iz;
  int px, py, pz;
  char *emsg;
  unsigned char *pixmap;
}
Fs_info;

static Fs_info *fs_alloc(void)
{
  Fs_info *fs = g_malloc(sizeof(*fs));
  fs->emsg = g_malloc(FILENAME_MAX + 100);
  *fs->emsg = '\0';
  fs->pixmap = NULL;
  fs->ix = fs->iy = fs->iz = fs->px = fs->py = fs->pz = 0;
  return fs;
}

static void fs_free(Fs_info *fs)
{
  if (fs)
    {
      if (fs->emsg) g_free(fs->emsg);
      if (fs->pixmap) g_free(fs->pixmap);
      g_free(fs);
    }
}

static int fs_ingest(char *fn, Fs_info *fs)
{
  FILE *ifd = fopen(fn, "r");
  if (ifd == NULL)
    sprintf(fs->emsg, "can't open file \"%s\"", fn);
  else
    {
      int body = FALSE;
      char buf[1000];

      while (!body && fgets(buf, sizeof(buf), ifd))
	if (strncasecmp(buf, "picdata:", 8) == 0)
	  sscanf(buf+8, "%d %d %d", &fs->px, &fs->py, &fs->pz);
	else if (strncasecmp(buf, "image:", 6) == 0)
	  sscanf(buf+6, "%d %d %d", &fs->ix, &fs->iy, &fs->iz);
	else if (buf[strspn(buf, " \t\n")] == '\0')
	  body = TRUE;

      if (!body)
	sprintf(fs->emsg, "%s: missing body", fn);
      else if (!fs->pz)
	sprintf(fs->emsg, "%s: bad or missing picdata line", fn);
      #ifdef REQUIRE_IMAGE_LINE
      else if (!fs->iz)
	sprintf(fs->emsg, "%s: bad or missing image line", fn);
      #endif
      else
	{
	  char *pix_x0;
	  int x, y, pixval, pixno = 0, pixcnt = fs->px * fs->py;
	  fs->pixmap = g_malloc(pixcnt + 1);
	  pix_x0 = fs->pixmap + pixcnt;
	  for (y=0; y<fs->py; y++)
	    {
	      char *pix = (pix_x0 -= fs->px);
	      for (x=0; x<fs->px; x++)
		if (pixno++ < pixcnt && fscanf(ifd, "%2x", &pixval) == 1)
		  *pix++ = pixval & 0xff;
		else
		  {
		    sprintf(fs->emsg, "%s: eof at %d of %d (%dx%d) pixels",
			    fn, pixno, pixcnt, fs->px, fs->py);
		    return FALSE;
		  }
	    }
	}
    }
  return !*fs->emsg;
}

static gint32 load_image(char *filename)
{
  char *loading;
  gint32 image_id, layer_id;
  GDrawable *drawable;
  GPixelRgn pixel_rgn;
  Fs_info *fs = fs_alloc();

  loading = g_malloc(strlen(filename) + 11);
  sprintf(loading, "Loading %s:", filename);
  gimp_progress_init(loading);
  g_free(loading);

  gimp_progress_update(0.25);
  if (!fs_ingest(filename, fs))
    {
      fprintf(stderr, "fs_load: %s\n", fs->emsg);
      fs_free(fs);
      return -1;
    }

  /* Create new image of proper size and associate filename; create
     and add a new layer to the image, and get a drawable; finally,
     initialize the drawing region. */
  gimp_progress_update(0.50);
  image_id = gimp_image_new(fs->px, fs->py, GRAY);
  gimp_image_set_filename(image_id, filename);
  layer_id = gimp_layer_new(image_id, "Background", 
    fs->px, fs->py, GRAY_IMAGE, 100, NORMAL_MODE);
  gimp_image_add_layer(image_id, layer_id, 0);
  drawable = gimp_drawable_get(layer_id);
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, 
    drawable->width, drawable->height, TRUE, FALSE);

  /* Transfer the pixel buffer into the image layer. */
  gimp_progress_update(0.75);
  gimp_pixel_rgn_set_rect(&pixel_rgn, fs->pixmap, 0,0, fs->px, fs->py);

  /* Lastly, display the image and return. */
  gimp_progress_update(1.00);
  gimp_drawable_flush(drawable);
  return image_id;
}

static void save_close_callback(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

static void save_ok_callback(GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(data));
  save_okay = TRUE;
}

static gint save_image(char *filename, gint32 image_id, gint32 drawable_id)
{
  GPixelRgn pixel_rgn;
  GDrawable *drawable;
  FILE *fd;
  int x, y, w, h;
  char *saving, *pixbuf, *pix_x0;

  drawable = gimp_drawable_get(drawable_id);
  w = drawable->width;
  h = drawable->height;
  gimp_pixel_rgn_init(&pixel_rgn, drawable, 0, 0, w, h, FALSE, FALSE);

  if (gimp_drawable_has_alpha(drawable_id))
    {
      fprintf(stderr, "fs: can't save images with alpha channels.");
      return FALSE;
    }
  if (gimp_drawable_type(drawable_id) != GRAY_IMAGE)
    {
      fprintf(stderr, "fs: image must be grayscale for facesaver format\n");
      return FALSE;
    }
  if ((fd = fopen(filename, "w")) == NULL)
    {
      fprintf(stderr, "fs: can't open \"%s\"\n", filename);
      return FALSE;
    }

  saving = g_malloc(strlen(filename) + 11);
  sprintf(saving, "Saving %s:", filename);
  gimp_progress_init(saving);
  g_free(saving);
  fprintf(fd, "Picdata: %d %d %d\n", w, h, drawable->bpp * 8);
  fprintf(fd, "Image:   %d %d %d\n", w, h, drawable->bpp * 8);

  gimp_progress_update(0.25);
  pixbuf = g_malloc(w * h);
  gimp_pixel_rgn_get_rect(&pixel_rgn, pixbuf, 0, 0, w, h);
  gimp_progress_update(0.50);

  pix_x0 = pixbuf + w*h;
  for (y=0; y<h; y++)
    {
      unsigned char *pix = (pix_x0 -= w);
      for (x=0; x<w; x++)
	{
	  if ((x%27) == 0) fprintf(fd, "\n");
	  fprintf(fd, "%02x", *pix++);
	}
    }
  fprintf(fd, "\n");

  fclose(fd);
  g_free(pixbuf);
  gimp_drawable_detach(drawable);
  return TRUE;
}

static gint save_dialog(void)
{
  GtkWidget *dlg;
  GtkWidget *button;
  gint argc = 1;
  gchar **argv = g_new(gchar*, 1);
  argv[0] = g_strdup("save");

  gtk_init(&argc, &argv);
  gtk_rc_parse(gimp_gtkrc());
  dlg = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dlg), "Save as facesaver file");
  gtk_window_position(GTK_WINDOW(dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect(GTK_OBJECT(dlg), "destroy",
    (GtkSignalFunc)save_close_callback, NULL);

  button = gtk_button_new_with_label("Okay");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
    (GtkSignalFunc)save_ok_callback, dlg);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), 
    button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);

  button = gtk_button_new_with_label("Cancel");
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
    (GtkSignalFunc)gtk_widget_destroy, GTK_OBJECT(dlg));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->action_area), 
    button, TRUE, TRUE, 0);
  gtk_widget_show(button);

  gtk_widget_show(dlg);
  gtk_main();
  gdk_flush();

  return save_okay; /* Initially false; true after clicking on "Okay" */
}

static void 
run(char *name, int params, GParam *param, int *rvals, GParam **rval)
{
  static GParam values[2];
  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_SUCCESS;

  *rvals = 1;
  *rval = values;

  if (strcmp(name, "file_fs_load") == 0)
    {
      gint32 image_id = load_image(param[1].data.d_string);
      if (image_id == -1)
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
      else
	{
	  *rvals = 2;
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image_id;
	}
    }
  else if (strcmp(name, "file_fs_save") == 0)
    {
      GRunModeType mode = param[0].data.d_int32;
      if (mode == RUN_NONINTERACTIVE && params != 4)
	values[0].data.d_status = STATUS_CALLING_ERROR;
      else if ((mode == RUN_WITH_LAST_VALS) ||
	       (mode == RUN_INTERACTIVE && save_dialog()) )
	{
	  char *filename     = param[3].data.d_string;
	  gint32 image_id    = param[1].data.d_int32;
	  gint32 drawable_id = param[2].data.d_int32;
	  if (!save_image(filename, image_id, drawable_id))
	    values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
  else
    values[0].data.d_status = STATUS_CALLING_ERROR;
}

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN()
