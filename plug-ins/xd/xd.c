/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Daniel Risacher
 * Copyright (C) 1997 Josh MacDonald
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

/* XD.C -- Xdelta versioned image plugin.
 * Author: Josh MacDonald */

/* xd.c loosely based on gz.c by Daniel Risacher */
/* gz.c loosley based on url.c by Josh MacDonald */
/* gz.c very loosely on hrz.c by Albert Cahalan <acahalan at cs.uml.edu> */

#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "xdelta.h"
#include "X11/xpm.h"

#ifdef XDELTA_OLD_PREFIX
#include <zlib.h>
#endif

#define PREVIEW_MAX_DIM 64

static void   query      (void);
static void   run        (char    *name,
                          int      nparams,
                          GParam  *param,
                          int     *nreturn_vals,
                          GParam **return_vals);
static gint32 load_image (char *filename, gint32 run_mode, gboolean interactive);

static gint save_image (char   *filename,
			gint32  image_ID,
			gint32  drawable_ID,
			gint32 run_mode,
			gboolean interactive);

static int valid_file (char* filename) ;
static gint get_a_version (XdFile* xd, gchar* ext);

const gchar* program_name = "xd";

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

MAIN ();

static void
query ()
{
  static GParamDef load_args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "filename", "The name of the file to load" },
    { PARAM_STRING, "raw_filename", "The name entered" },
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
    { PARAM_STRING, "raw_filename", "The name of the file to save the image in" }
  };
  static int nsave_args = sizeof (save_args) / sizeof (save_args[0]);

  gimp_install_procedure ("file_xd_load",
                          "extract a file from a XDELTA version file",
                          "",
                          "Josh MacDonald",
                          "Josh MacDonald",
                          "1997",
                          "<Load>/xd",
			  NULL,
                          PROC_PLUG_IN,
                          nload_args, nload_return_vals,
                          load_args, load_return_vals);

  gimp_install_procedure ("file_xd_save",
                          "store a file into an XDELTA version file",
                          "",
                          "Josh MacDonald",
                          "Josh MacDonald",
                          "1997",
                          "<Save>/xd",
			  "RGB*, GRAY*, INDEXED*",
                          PROC_PLUG_IN,
                          nsave_args, 0,
                          save_args, NULL);

  gimp_register_save_handler ("file_xd_save", "xd", "");
  gimp_register_load_handler ("file_xd_load", "xd", "");
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
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "file_xd_load") == 0)
    {
      image_ID = load_image (param[1].data.d_string,
			     param[0].data.d_int32,
			     run_mode == RUN_INTERACTIVE);
      if (image_ID != -1)
	{
	  *nreturn_vals = 2;
	  values[0].data.d_status = STATUS_SUCCESS;
	  values[1].type = PARAM_IMAGE;
	  values[1].data.d_image = image_ID;
	}
      else
	{
	  values[0].data.d_status = STATUS_EXECUTION_ERROR;
	}
    }
  else if (strcmp (name, "file_xd_save") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  break;
	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 4)
	    status = STATUS_CALLING_ERROR;

	case RUN_WITH_LAST_VALS:
	  break;

	default:
	  break;
	}

      *nreturn_vals = 1;
      if (save_image (param[3].data.d_string,
		      param[1].data.d_int32,
		      param[2].data.d_int32,
		      param[0].data.d_int32,
		      run_mode == RUN_INTERACTIVE))
	{
	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
  else
    g_assert (FALSE);
}

static gboolean
get_name_ver_and_xd (gchar* filename0,
		     gboolean save_or_n_load,
		     gboolean interactive,
		     gchar **ext,
		     gint* version,
		     XdFile** xd)
{
  gchar* filename = g_strdup (filename0);
  gchar* ext1 = strrchr (filename, '.');
  gchar* ext2;
  gchar* ext3;

  *version = -1;

  if (!ext1 || strcmp (ext1, ".xd") != 0)
    {
      g_warning ("xd: unrecognized extension %s\n", filename);
      goto bail;
    }

  *ext1 = 0;

  ext2 = strrchr (filename, '.');

  if (!ext2)
    {
      g_warning ("xd: unrecognized extension %s\n", filename);
      goto bail;
    }

  if (isdigit (ext2[1]))
    {
      gchar* end = NULL;

      *version = strtol (ext2+1, &end, 10);

      if (end[0] != 0 || *version < 0)
	{
	  g_warning ("xd: illegal version number %s\n", ext2+1);
	  goto bail;
	}

      *ext2 = 0;
    }

  ext3 = strrchr (filename, '.');

  if (!ext3)
    {
      g_warning ("xd: unrecognized extension %s\n", filename);
      goto bail;
    }

  *ext = g_strdup(ext3+1);

  strcat (filename, ".xd");

  if (save_or_n_load)
    {
      if (valid_file (filename))
	*xd = xd_open_write (filename);
      else
#ifdef XDELTA_OLD_PREFIX
	*xd = xd_create (filename, Z_DEFAULT_COMPRESSION);
#else
	*xd = xd_create (filename);
#endif
    }
  else
    *xd = xd_open_write (filename); /* !! it's write so it can update preview segments */

  if (!*xd)
    {
      g_warning ("xd: open failed: %s\n", gdbm_strerror (gdbm_errno));

      return FALSE;
    }

  if (*version > ((*xd)->versions + (save_or_n_load && 1)))
    {
      g_warning ("xd: version number too high\n");
      goto bail;
    }

  if (*version < 0 && !save_or_n_load)
    {
      if (interactive)
	{
	  *version = get_a_version (*xd, *ext);

	  if (*version < 0)
	    goto bail;
	}
      else
	*version = (*xd)->versions - 1;
    }

  g_free (filename);
  return TRUE;

bail:
  g_free (filename);
  return FALSE;
}

static gint
image_scale (gint image_id, guint width, guint height)
{
  GParam* params;
  gint retvals;

  params = gimp_run_procedure ("gimp_scale",
			       &retvals,
			       PARAM_IMAGE, image_id,
			       PARAM_DRAWABLE, gimp_image_get_active_layer (image_id),
			       PARAM_INT32, TRUE,
			       PARAM_FLOAT, (double) 0.0,
			       PARAM_FLOAT, (double) 0.0,
			       PARAM_FLOAT, (double) width,
			       PARAM_FLOAT, (double) height,
			       PARAM_END);

  return params[1].data.d_int32;
}

static gint
save_image (char   *filename,
	    gint32  image_ID,
	    gint32  drawable_ID,
	    gint32  run_mode,
	    gboolean interactive)
{
  GParam* params;
  gint retvals;
  gchar* ext;
  gchar* tmpname;
  gint ver;
  XdFile* xd;

  if (!get_name_ver_and_xd (filename, TRUE, interactive, &ext, &ver, &xd))
    return -1;

  params = gimp_run_procedure ("gimp_temp_name",
			       &retvals,
			       PARAM_STRING, ext,
			       PARAM_END);

  tmpname = params[1].data.d_string;

  params = gimp_run_procedure ("gimp_file_save",
			       &retvals,
 			       PARAM_INT32, run_mode,
			       PARAM_IMAGE, image_ID,
			       PARAM_DRAWABLE, drawable_ID,
			       PARAM_STRING, tmpname,
			       PARAM_STRING, tmpname,
			       PARAM_END);

  if (params[0].data.d_status == FALSE || !valid_file(tmpname))
    {
      xd_close (xd);
      unlink (tmpname);
      return -1;
    }

  if (xd_checkin (xd, tmpname) != 0)
    {
      xd_close (xd);
      unlink (tmpname);
      g_warning ("xd: checkin failed\n");
      return -1;
    }

  xd_close (xd);

  unlink (tmpname);

  return TRUE;
}

static gint32
load_image (char *filename, gint32 run_mode, gboolean interactive)
{
  GParam* params;
  gint retvals;
  gchar* ext;
  gchar* tmpname;
  gint ver;
  XdFile* xd;

  if (!get_name_ver_and_xd (filename, FALSE, interactive, &ext, &ver, &xd))
    return -1;

  params = gimp_run_procedure ("gimp_temp_name",
			       &retvals,
			       PARAM_STRING, ext,
			       PARAM_END);

  tmpname = params[1].data.d_string;

  if (xd_checkout (xd, tmpname, ver) != 0)
    {
      g_warning ("xd: checkout failed\n");
      xd_close (xd);
      return -1;
    }

  params = gimp_run_procedure ("gimp_file_load",
			       &retvals,
			       PARAM_INT32, run_mode,
			       PARAM_STRING, tmpname,
			       PARAM_STRING, tmpname,
			       PARAM_END);

  gimp_image_set_filename (params[1].data.d_int32, filename);

  xd_close (xd);

  unlink (tmpname);

  if (params[0].data.d_status == FALSE)
    return -1;
  else
    return params[1].data.d_int32;
}


static gint
valid_file (char* filename)
{
  int stat_res;
  struct stat buf;

  stat_res = stat(filename, &buf);

  if ((0 == stat_res) && (buf.st_size > 0))
    return 1;
  else
    return 0;
}

static gint    preview_selected_version = -1;
static XdFile *preview_xd;
static datum      *xpm_buffers;
static GdkPixmap **xpm_pixmaps;

static void
select_callback (GtkWidget *data)
{
  gtk_main_quit ();
}

static void
cancel_callback (GtkWidget *widget)
{
  preview_selected_version = -1;
  gtk_main_quit ();
}

static void
destroy_callback (GtkWidget *widget)
{
  gtk_main_quit ();
}

static gint
preview_event (GtkWidget *widget,
	       GdkEvent  *event)
{
  gint image_no = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));
  gint h, w;

  switch (event->type)
    {
    case GDK_EXPOSE:
      gdk_window_get_size (xpm_pixmaps[image_no], &w, &h);

      gdk_draw_pixmap (GTK_DRAWING_AREA (widget)->widget.window,
		       GTK_DRAWING_AREA (widget)->widget.style->white_gc,
		       xpm_pixmaps[image_no],
		       0,
		       0,
		       0,
		       0,
		       w,
		       h);
      break;
    default:
      break;
    }

  return 0;
}

static gint
select_event (GtkWidget *widget,
	      GdkEvent  *event)
{
  preview_selected_version = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));
  g_print ("%d selected\n", preview_selected_version);
  return 0;
}

static datum
get_buffer (XdFile* xd, gchar* ext, gint ver)
{
  char req[64];
  datum dat;
  datum key;
  datum err;

  err.dptr = NULL;

  sprintf (req, "xdelta-xpm-%d-%d", PREVIEW_MAX_DIM, ver);

  key.dptr = req;
  key.dsize = strlen (req);

  dat = gdbm_fetch (xd->dbf, key);

  if (!dat.dptr)
    {
      GParam* params;
      gchar* tmpname;
      gint retvals;
      gint image_id, drawable_id;
      guint neww, newh;
      MappedFile* map;

      params = gimp_run_procedure ("gimp_temp_name",
				   &retvals,
				   PARAM_STRING, ext,
				   PARAM_END);

      tmpname = params[1].data.d_string;

      if (xd_checkout (xd, tmpname, ver) != 0)
	{
	  g_warning ("xd: checkout failed\n");
	  return err;
	}

      params = gimp_run_procedure ("gimp_file_load",
				   &retvals,
				   PARAM_INT32, RUN_NONINTERACTIVE,
				   PARAM_STRING, tmpname,
				   PARAM_STRING, tmpname,
				   PARAM_END);

      if (params[0].data.d_status == FALSE)
	goto bail;

      image_id = params[1].data.d_int32;

      gimp_image_flatten (image_id);

      if (gimp_image_width (image_id) > gimp_image_height (image_id))
	{
	  neww = PREVIEW_MAX_DIM;
	  newh = (gimp_image_height (image_id) * PREVIEW_MAX_DIM) / gimp_image_width (image_id);
	}
      else
	{
	  newh = PREVIEW_MAX_DIM;
	  neww = (gimp_image_width (image_id) * PREVIEW_MAX_DIM) / gimp_image_height (image_id);
	}

      drawable_id = image_scale (image_id, neww, newh);

      params = gimp_run_procedure ("gimp_temp_name",
				   &retvals,
				   PARAM_STRING, "xpm",
				   PARAM_END);

      tmpname = params[1].data.d_string;

      params = gimp_run_procedure ("gimp_file_save",
				   &retvals,
				   PARAM_INT32, RUN_NONINTERACTIVE,
				   PARAM_IMAGE, image_id,
				   PARAM_DRAWABLE, drawable_id,
				   PARAM_STRING, tmpname,
				   PARAM_STRING, tmpname,
				   PARAM_END);

      if (params[0].data.d_status == FALSE || !valid_file(tmpname))
	goto bail;

      map = map_file (tmpname);

      if (!map)
	goto bail;

      dat.dptr = map->seg;
      dat.dsize = map->len;

      if (gdbm_store (xd->dbf, key, dat, GDBM_REPLACE) != 0)
	goto bail;
    }

  return dat;

bail:

  g_warning ("xd: save failed in preview generation\n");

  return err;
}

static GdkPixmap*
gdk_pixmap_create_from_xpm_b (GdkWindow  *window,
			      GdkBitmap **mask,
			      GdkColor   *transparent_color,
			      guint8 *segment,
			      gint len)
{
  gchar* foo = "/tmp/foo.xpm";
  FILE* f;

  g_warning ("gdk_pixmap_create_from_xpm: this proceedure is slow because Peter is a wuss.  it should be rewritten.");

  f = fopen (foo, "w");

  fwrite (segment, len, 1, f);

  return gdk_pixmap_create_from_xpm (window, mask, transparent_color, foo);
}


static gint
get_a_version (XdFile* xd, gchar* ext)
{
  /* Come up with a dialog presenting all previews, a field for pixel
   * size, and a button to regenerate previews with the pixel size.
   * Clicking on an image selects the version. */
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *listbox;
  GtkWidget *plist;
  GtkWidget *hbox;
  GtkWidget *vbox;
  gchar **argv;
  gint argc, i;

  if (xd->versions == 0)
    return -1;

  xpm_buffers = g_new0 (datum, xd->versions);
  xpm_pixmaps = g_new0 (GdkPixmap*, xd->versions);

  preview_xd = xd;

  for (i=0; i<xd->versions;i+=1)
    {
      xpm_buffers[i] = get_buffer (xd, ext, i);

      if (!xpm_buffers[i].dptr)
	return -1;
    }

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("save");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /* Dialog */
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), "Load from Xdelta File");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy", (GtkSignalFunc) destroy_callback, NULL);

  /* Button box */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), vbox, FALSE, FALSE, 1);

  /* Preview list */
  listbox = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_usize (listbox, /*W*/ (PREVIEW_MAX_DIM + 4) * 2, /*H*/ (PREVIEW_MAX_DIM + 4) * 4);
  gtk_box_pack_start (GTK_BOX (vbox), listbox, TRUE, TRUE, 2);

  /* Button box */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);

  /* Generate button */
  button = gtk_button_new_with_label ("Select");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked", (GtkSignalFunc) select_callback, GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  /* Cancel button */
  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked", (GtkSignalFunc) cancel_callback, GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  plist = gtk_list_new ();
  gtk_container_add (GTK_CONTAINER (listbox), plist);
  gtk_list_set_selection_mode (GTK_LIST (plist), GTK_SELECTION_BROWSE);

  gtk_widget_show (plist);
  gtk_widget_show (listbox);
  gtk_widget_show (hbox);
  gtk_widget_show (vbox);
  gtk_widget_show (dlg);

  for (i=0;i<xd->versions;i+=1)
    {
      GtkWidget *list_item;
      GtkWidget *lhbox;
      GtkWidget *drawing_area;
      GtkWidget *label;
      GString *text;

      /* List Item */
      list_item = gtk_list_item_new ();

      gtk_object_set_user_data (GTK_OBJECT (list_item), (gpointer) i);

      /* with a hbox */
      lhbox = gtk_hbox_new (FALSE, 1);
      gtk_container_add (GTK_CONTAINER (list_item), lhbox);

      /* and a drawing area. */
      drawing_area = gtk_drawing_area_new ();
      gtk_drawing_area_size (GTK_DRAWING_AREA (drawing_area), PREVIEW_MAX_DIM + 4, PREVIEW_MAX_DIM + 4);
      gtk_object_set_user_data (GTK_OBJECT (drawing_area), (gpointer) i);
      gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
      gtk_box_pack_start (GTK_BOX (lhbox), drawing_area, TRUE, TRUE, 0);
      gtk_signal_connect (GTK_OBJECT (drawing_area), "event", (GtkSignalFunc) preview_event, drawing_area);

      /* and a label */
      text = g_string_new (NULL);
      g_string_sprintf (text, "version %d\n", i);
      label = gtk_label_new (text->str);
      gtk_box_pack_start (GTK_BOX (lhbox), label, TRUE, TRUE, 0);
      g_string_free (text, TRUE);

      gtk_list_append_items (GTK_LIST (plist), g_list_prepend (NULL, list_item));

      gtk_signal_connect (GTK_OBJECT (list_item), "select", (GtkSignalFunc) select_event, list_item);

      gtk_widget_show (drawing_area);
      gtk_widget_show (label);
      gtk_widget_show (lhbox);
      gtk_widget_show (list_item);

      xpm_pixmaps[i] = gdk_pixmap_create_from_xpm_b (drawing_area->window,
						     NULL,
						     &drawing_area->style->bg[GTK_STATE_SELECTED],
						     xpm_buffers[i].dptr,
						     xpm_buffers[i].dsize);
    }

  gtk_main ();
  gdk_flush ();

  return preview_selected_version;
}
