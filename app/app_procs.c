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
#include "config.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <process.h>		/* For _getpid() */
#endif
 
#include <gtk/gtk.h>

#include "libgimp/gimpfeatures.h"
#include "libgimp/gimpenv.h"

#include "appenv.h"
#include "app_procs.h"
#include "batch.h"
#include "brush_select.h"
#include "color_transfer.h"
#include "curves.h"
#include "colormaps.h"
#include "context_manager.h"
#include "devices.h"
#include "errorconsole.h"
#include "fileops.h"
#include "gdisplay.h"
#include "gdisplay_color.h"
#include "gdisplay_ops.h"
#include "gimpbrushlist.h"
#include "gimprc.h"
#include "gimpparasite.h"
#include "gimpset.h"
#include "gimpui.h"
#include "global_edit.h"
#include "gradient_select.h"
#include "gradient.h"
#include "gximage.h"
#include "hue_saturation.h"
#include "image_render.h"
#include "interface.h"
#include "internal_procs.h"
#include "lc_dialog.h"
#include "levels.h"
#include "menus.h"
#include "paint_funcs.h"
#include "palette.h"
#include "pattern_select.h"
#include "patterns.h"
#include "plug_in.h"
#include "module_db.h"
#include "procedural_db.h"
#include "session.h"
#include "temp_buf.h"
#include "tile_swap.h"
#include "tips_dialog.h"
#include "tools.h"
#include "undo.h"
#include "unitrc.h"
#include "xcf.h"
#include "errors.h"
#include "docindex.h"
#include "colormap_dialog.h"

#include "color_notebook.h"
#include "color_select.h"
#include "gimpparasite.h"

#include "libgimp/gimplimits.h"
#include "libgimp/gimpintl.h"

#define LOGO_WIDTH_MIN  300
#define LOGO_HEIGHT_MIN 110
#define AUTHORS "Spencer Kimball & Peter Mattis"

#define SHOW_NEVER 0
#define SHOW_LATER 1
#define SHOW_NOW   2

/*  Function prototype for affirmation dialog when exiting application  */
static void      really_quit_dialog                   (void);
static void      make_initialization_status_window    (void);
static void      destroy_initialization_status_window (void);
static gboolean  splash_logo_load                     (GtkWidget *window);
static gboolean  splash_logo_load_size                (GtkWidget *window);
static void      splash_logo_draw                     (GtkWidget *widget);
static void      splash_text_draw                     (GtkWidget *widget);
static void      splash_logo_expose                   (GtkWidget *widget);
static void      toast_old_temp_files                 (void);


static gint is_app_exit_finish_done = FALSE;
gint        we_are_exiting          = FALSE;

static GtkWidget *logo_area   = NULL;
static GdkPixmap *logo_pixmap = NULL;
static gint logo_width        = 0;
static gint logo_height       = 0;
static gint logo_area_width   = 0;
static gint logo_area_height  = 0;
static gint show_logo         = SHOW_NEVER;
static gint max_label_length  = MAXPATHLEN;


void
gimp_init (int    gimp_argc,
	   char **gimp_argv)
{
  /* Initialize the application */
  app_init ();

  /* Parse the rest of the command line arguments as images to load */
  if (gimp_argc > 0)
    while (gimp_argc--)
      {
	if (*gimp_argv)
	  file_open (*gimp_argv, *gimp_argv);
	gimp_argv++;
      }

  batch_init ();

  /* Handle showing dialogs with gdk_quit_adds here  */
  if (!no_interface && show_tips)
    tips_dialog_create ();
}

static gboolean
splash_logo_load_size (GtkWidget *window)
{
  gchar buf[1024];
  FILE *fp;

  if (logo_pixmap)
    return TRUE;

  g_snprintf (buf, sizeof(buf), "%s" G_DIR_SEPARATOR_S "gimp1_1_splash.ppm",
	      gimp_data_directory ());

  fp = fopen (buf, "rb");
  if (!fp)
    return FALSE;

  fgets (buf, sizeof (buf), fp);
  if (strcmp (buf, "P6\n") != 0)
    {
      fclose (fp);
      return FALSE;
    }

  fgets (buf, sizeof (buf), fp);
  fgets (buf, sizeof (buf), fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fclose (fp);

  return TRUE;
}

static gboolean
splash_logo_load (GtkWidget *window)
{
  GtkWidget *preview;
  GdkGC *gc;
  gchar   buf[1024];
  guchar *pixelrow;
  FILE *fp;
  gint count;
  gint i;

  if (logo_pixmap)
    return TRUE;

  g_snprintf (buf, sizeof(buf), "%s" G_DIR_SEPARATOR_S "gimp1_1_splash.ppm",
	      gimp_data_directory ());

  fp = fopen (buf, "rb");
  if (!fp)
    return FALSE;

  fgets (buf, sizeof (buf), fp);
  if (strcmp (buf, "P6\n") != 0)
    {
      fclose (fp);
      return FALSE;
    }

  fgets (buf, sizeof (buf), fp);
  fgets (buf, sizeof (buf), fp);
  sscanf (buf, "%d %d", &logo_width, &logo_height);

  fgets (buf, sizeof (buf), fp);
  if (strcmp (buf, "255\n") != 0)
    {
      fclose (fp);
      return FALSE;
    }

  preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (preview), logo_width, logo_height);
  pixelrow = g_new (guchar, logo_width * 3);

  for (i = 0; i < logo_height; i++)
    {
      count = fread (pixelrow, sizeof (unsigned char), logo_width * 3, fp);
      if (count != (logo_width * 3))
	{
	  gtk_widget_destroy (preview);
	  g_free (pixelrow);
	  fclose (fp);
	  return FALSE;
	}
      gtk_preview_draw_row (GTK_PREVIEW (preview), pixelrow, 0, i, logo_width);
    }

  gtk_widget_realize (window);
  logo_pixmap = gdk_pixmap_new (window->window, logo_width, logo_height,
				gtk_preview_get_visual ()->depth);
  gc = gdk_gc_new (logo_pixmap);
  gtk_preview_put (GTK_PREVIEW (preview),
		   logo_pixmap, gc,
		   0, 0, 0, 0, logo_width, logo_height);
  gdk_gc_destroy (gc);

  gtk_widget_unref (preview);
  g_free (pixelrow);

  fclose (fp);

  return TRUE;
}

static void
splash_text_draw (GtkWidget *widget)
{
  GdkFont *font = NULL;

  font = gdk_font_load (_("-*-helvetica-bold-r-normal--*-140-*-*-*-*-*-*"));
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font,  _("The GIMP"))) / 2),
		   (0.25 * logo_area_height),
		   _("The GIMP"));
  gdk_font_unref (font);

  font = gdk_fontset_load (_("-*-helvetica-bold-r-normal--*-120-*-*-*-*-*-*,*"));
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, GIMP_VERSION)) / 2),
		   (0.45 * logo_area_height),
		   GIMP_VERSION);
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, _("brought to you by"))) / 2),
		   (0.65 * logo_area_height),
		   _("brought to you by"));
  gdk_draw_string (widget->window,
		   font,
		   widget->style->fg_gc[GTK_STATE_NORMAL],
		   ((logo_area_width - gdk_string_width (font, AUTHORS)) / 2),
		   (0.80 * logo_area_height),
		   AUTHORS);
  gdk_font_unref (font);
}

static void
splash_logo_draw (GtkWidget *widget)
{
  gdk_draw_pixmap (widget->window,
		   widget->style->black_gc,
		   logo_pixmap,
		   0, 0,
		   ((logo_area_width - logo_width) / 2),
		   ((logo_area_height - logo_height) / 2),
		   logo_width, logo_height);
}

static void
splash_logo_expose (GtkWidget *widget)
{
  switch (show_logo)
    {
    case SHOW_NEVER:
    case SHOW_LATER:
      splash_text_draw (widget);
      break;
    case SHOW_NOW:
      splash_logo_draw (widget);
      break;
    }
}

static GtkWidget *win_initstatus = NULL;
static GtkWidget *label1 = NULL;
static GtkWidget *label2 = NULL;
static GtkWidget *pbar   = NULL;

static void
destroy_initialization_status_window (void)
{
  if (win_initstatus)
    {
      gtk_widget_destroy (win_initstatus);
      if (logo_pixmap != NULL)
	gdk_pixmap_unref (logo_pixmap);

      win_initstatus = label1 = label2 = pbar = logo_area = NULL;
      logo_pixmap = NULL;
    }
}

static void
make_initialization_status_window (void)
{
  if (!no_interface && !no_splash)
    {
      GtkWidget *vbox;
      GtkWidget *logo_hbox;
      GtkStyle  *style;

      win_initstatus = gtk_window_new (GTK_WINDOW_DIALOG);

      gtk_window_set_title (GTK_WINDOW (win_initstatus),                      
			    _("GIMP Startup"));
      gtk_window_set_wmclass (GTK_WINDOW (win_initstatus),
			      "gimp_startup", "Gimp");
      gtk_window_set_position (GTK_WINDOW (win_initstatus), GTK_WIN_POS_CENTER);
      gtk_window_set_policy (GTK_WINDOW (win_initstatus), FALSE, FALSE, FALSE);

      gimp_dialog_set_icon (GTK_WINDOW (win_initstatus));

      if (no_splash_image == FALSE &&
	  splash_logo_load_size (win_initstatus))
	{
	  show_logo = SHOW_LATER;
	}

      vbox = gtk_vbox_new (FALSE, 4);
      gtk_container_add (GTK_CONTAINER (win_initstatus), vbox);

      logo_hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX(vbox), logo_hbox, FALSE, TRUE, 0);

      logo_area = gtk_drawing_area_new ();

      gtk_signal_connect (GTK_OBJECT (logo_area), "expose_event",
			  GTK_SIGNAL_FUNC (splash_logo_expose),
			  NULL);

      logo_area_width  = MAX (logo_width, LOGO_WIDTH_MIN);
      logo_area_height = MAX (logo_height, LOGO_HEIGHT_MIN);

      gtk_drawing_area_size (GTK_DRAWING_AREA (logo_area),
			     logo_area_width, logo_area_height);
      gtk_box_pack_start (GTK_BOX(logo_hbox), logo_area, TRUE, FALSE, 0);

      label1 = gtk_label_new ("");
      gtk_box_pack_start_defaults (GTK_BOX (vbox), label1);
      label2 = gtk_label_new ("");
      gtk_box_pack_start_defaults (GTK_BOX (vbox), label2);

      pbar = gtk_progress_bar_new ();
      gtk_box_pack_start_defaults (GTK_BOX (vbox), pbar);

      gtk_widget_show (vbox);
      gtk_widget_show (logo_hbox);
      gtk_widget_show (logo_area);
      gtk_widget_show (label1);
      gtk_widget_show (label2);
      gtk_widget_show (pbar);

      gtk_widget_show (win_initstatus);

      /*  This is a hack: we try to compute a good guess for the maximum 
       *  number of charcters that will fit into the splash-screen using 
       *  the default_font
       */
      style = gtk_widget_get_style (win_initstatus);
      max_label_length =
	0.8 * (float)strlen (AUTHORS) *
	((float)logo_area_width /
	 (float)gdk_string_width (style->font, AUTHORS));
    }
}

void
app_init_update_status (gchar *label1val,
		        gchar *label2val,
		        float  pct_progress)
{
  gchar *temp;

  if (!no_interface && !no_splash && win_initstatus)
    {
      if (label1val && strcmp (label1val, GTK_LABEL (label1)->label))
	gtk_label_set (GTK_LABEL (label1), label1val);

      if (label2val && strcmp (label2val, GTK_LABEL (label2)->label))
	{
	  while (strlen (label2val) > max_label_length)
	    {
	      temp = strchr (label2val, G_DIR_SEPARATOR);
	      if (temp == NULL)  /* for sanity */
		break;
	      temp++;
	      label2val = temp;
	    }

	  gtk_label_set_text (GTK_LABEL (label2), label2val);
	}

      if (pct_progress >= 0.0 && pct_progress <= 1.0 && 
	  gtk_progress_get_current_percentage (&(GTK_PROGRESS_BAR (pbar)->progress)) != pct_progress)
	/*
	 GTK_PROGRESS_BAR(pbar)->percentage != pct_progress)
	*/
	{
	  gtk_progress_bar_update (GTK_PROGRESS_BAR (pbar), pct_progress);
	}

      while (gtk_events_pending ())
	gtk_main_iteration ();

      /* We sync here to make sure things get drawn before continuing,
       * is the improved look worth the time? I'm not sure...
       */
      gdk_flush ();
    }
}

/* #define RESET_BAR() app_init_update_status("", "", 0) */
#define RESET_BAR()

void
app_init (void)
{
  gchar *filename;
  gchar *path;

  filename = gimp_gtkrc ();

  if (be_verbose || no_splash)
    g_print (_("parsing \"%s\"\n"), filename);

  gtk_rc_parse (filename);

  if (!no_interface)
    get_standard_colormaps ();

  make_initialization_status_window ();

  if (!no_interface && !no_splash && win_initstatus)
    splash_text_draw (logo_area);

  /* Create the context of all existing images */
  image_context = gimp_set_new (GIMP_TYPE_IMAGE, TRUE);

  /*  Initialize the context system before loading any data  */
  context_manager_init ();

  /*  Initialize the procedural database
   *    We need to do this first because any of the init
   *    procedures might install or query it as needed.
   */
  procedural_db_init ();
  RESET_BAR();
  internal_procs_init ();

  color_display_init ();

  RESET_BAR();
  parse_buffers_init ();
  parse_unitrc ();         /*  this needs to be done before gimprc loading  */
  parse_gimprc ();         /*  parse the local GIMP configuration file      */

  if (always_restore_session)
    restore_session = TRUE;

  /* make sure the monitor resolution is valid */
  if (monitor_xres < GIMP_MIN_RESOLUTION ||
      monitor_yres < GIMP_MIN_RESOLUTION)
    {
      gdisplay_xserver_resolution (&monitor_xres, &monitor_yres);
      using_xserver_resolution = TRUE;
    }

  /* Now we are ready to draw the splash-screen-image to the start-up window */
  if (no_interface == FALSE)
    {
      if (no_splash_image == FALSE && show_logo &&
	  splash_logo_load (win_initstatus))
	{
	  show_logo = SHOW_NOW;
	  splash_logo_draw (logo_area);
        }
    }

  RESET_BAR();
  file_ops_pre_init ();    /*  pre-initialize the file types  */
  RESET_BAR();
  xcf_init ();             /*  initialize the xcf file format routines */

  app_init_update_status (_("Looking for data files"), _("Parasites"), 0.00);
  gimp_init_parasites ();          /*  initialize  the global parasite table  */
  app_init_update_status (NULL, _("Brushes"), 0.20);
  brushes_init (no_data);          /*  initialize the list of gimp brushes    */
  app_init_update_status (NULL, _("Patterns"), 0.40);
  patterns_init (no_data);         /*  initialize the list of gimp patterns   */
  app_init_update_status (NULL, _("Palettes"), 0.60);
  palettes_init (no_data);         /*  initialize the list of gimp palettes   */
  app_init_update_status (NULL, _("Gradients"), 0.80);
  gradients_init (no_data);        /*  initialize the list of gimp gradients  */
  app_init_update_status (NULL, NULL, 1.00);

  plug_in_init ();           /*  initialize the plug in structures  */
  module_db_init ();         /*  load any modules we need           */
  RESET_BAR();
  file_ops_post_init ();     /*  post-initialize the file types     */

  menus_reorder_plugins ();  /*  beautify some menus                */

  /* Add the swap file  */
  if (swap_path == NULL)
    swap_path = g_get_tmp_dir ();

  toast_old_temp_files ();
  path = g_strdup_printf ("%s" G_DIR_SEPARATOR_S "gimpswap.%lu",
			  swap_path, (unsigned long) getpid ());
  tile_swap_add (path, NULL, NULL);
  g_free (path);

  destroy_initialization_status_window ();

  /*  Things to do only if there is an interface  */
  if (no_interface == FALSE)
    {
      devices_init ();
      session_init ();
      create_toolbox ();

      /*  Fill the "last opened" menu items with the first last_opened_size
       *  elements of the docindex
       */
      {
	FILE *fp;
	gchar **filenames = g_new (gchar *, last_opened_size);
	gint dummy, i;

	if ((fp = idea_manager_parse_init (&dummy, &dummy, &dummy, &dummy)))
	  {
	    /*  read the filenames...  */
	    for (i = 0; i < last_opened_size; i++)
	      if ((filenames[i] = idea_manager_parse_line (fp)) == NULL)
		break;

	    /*  ...and add them in reverse order  */
	    for (--i; i >= 0; i--)
	      {
		menus_last_opened_add (filenames[i]);
		g_free (filenames[i]);
	      }

	    fclose (fp);
	  }
	g_free (filenames);
      }

      gximage_init ();
      render_setup (transparency_type, transparency_size);
      tool_options_dialog_new ();

      /*  EEK: force signal emission  */
      if (gimp_context_get_tool (gimp_context_get_user ()) == RECT_SELECT)
	{
	  gtk_signal_emit_by_name (GTK_OBJECT (gimp_context_get_user ()),
				   "tool_changed", RECT_SELECT);
	}
      else
	{
	  gimp_context_set_tool (gimp_context_get_user (), RECT_SELECT);
	}

      /*  FIXME: This needs to go in preferences  */
      message_handler = MESSAGE_BOX;
    }

  color_transfer_init ();
  paint_funcs_setup ();

  /* register internal color selectors */
  color_select_init ();

  if (no_interface == FALSE)
    {
      devices_restore ();
      session_restore ();
    }
}

int
app_exit_finish_done (void)
{
  return is_app_exit_finish_done;
}

void
app_exit_finish (void)
{
  if (app_exit_finish_done ())
    return;
  is_app_exit_finish_done = TRUE;

  message_handler = CONSOLE;
  we_are_exiting = TRUE;

  /*  do this here before brushes and patterns are freed  */
  if (!no_interface)
    device_status_free ();

  module_db_free ();
  lc_dialog_free ();
  gdisplays_delete ();
  global_edit_free ();
  named_buffers_free ();
  swapping_free ();
  brush_dialog_free ();
  brushes_free ();
  pattern_dialog_free ();
  patterns_free ();
  palette_dialog_free ();
  palettes_free ();
  gradient_dialog_free ();
  gradients_free ();
  context_manager_free ();
  hue_saturation_free ();
  curves_free ();
  levels_free ();
  paint_funcs_free ();
  plug_in_kill ();
  procedural_db_free ();
  error_console_free ();
  menus_quit ();
  tile_swap_exit ();
  save_unitrc ();
  gimp_parasiterc_save ();

  /*  Things to do only if there is an interface  */
  if (!no_interface)
    {
      toolbox_free ();
      close_idea_window ();
      gximage_free ();
      render_free ();
      tool_options_dialog_free ();
      save_sessionrc ();
    }

  /* gtk_exit (0); */
  gtk_main_quit();
}

void
app_exit (gboolean kill_it)
{
  /*  If it's the user's perogative, and there are dirty images  */
  if (!kill_it && gdisplays_dirty () && !no_interface)
    really_quit_dialog ();
  else
    app_exit_finish ();
}

/*************************************************
 *   Routines to query exiting the application   *
 *************************************************/

static void
really_quit_callback (GtkWidget *button,
		      gboolean   quit,
		      gpointer   data)
{
  if (quit)
    {
      app_exit_finish ();
    }
  else
    {
      menus_set_sensitive ("<Toolbox>/File/Quit", TRUE);
      menus_set_sensitive ("<Image>/File/Quit", TRUE);
    }
}

static void
really_quit_dialog (void)
{
  GtkWidget *dialog;

  menus_set_sensitive ("<Toolbox>/File/Quit", FALSE);
  menus_set_sensitive ("<Image>/File/Quit", FALSE);

  dialog = gimp_query_boolean_box (_("Really Quit?"),
				   gimp_standard_help_func,
				   "dialogs/really_quit.html",
				   TRUE,
				   _("Some files unsaved.\n\nQuit the GIMP?"),
				   _("Quit"), _("Cancel"),
				   NULL, NULL,
				   really_quit_callback,
				   NULL);

  gtk_widget_show (dialog);
}

static void
toast_old_temp_files (void)
{
  DIR *dir;
  struct dirent *entry;
  GString *filename = g_string_new ("");

  dir = opendir (swap_path);

  if (!dir)
    return;

  while ((entry = readdir (dir)) != NULL)
    if (!strncmp (entry->d_name, "gimpswap.", 9))
      {
        /* don't try to kill swap files of running processes
         * yes, I know they might not all be gimp processes, and when you
         * unlink, it's refcounted, but lets not confuse the user by
         * "where did my disk space go?" cause the filename is gone
         * if the kill succeeds, and there running process isn't gimp
         * we'll probably get it the next time around
         */

	gint pid = atoi (entry->d_name + 9);
#ifndef G_OS_WIN32
	if (kill (pid, 0))
#endif
	  {
	    /*  On Windows, you can't remove open files anyhow,
	     *  so no harm trying.
	     */
	    g_string_sprintf (filename, "%s" G_DIR_SEPARATOR_S "%s",
			      swap_path, entry->d_name);
	    unlink (filename->str);
	  }
      }

  closedir (dir);
  
  g_string_free (filename, TRUE);
}
