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

#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "appenv.h"
#include "gdisplay_ops.h"
#include "gimprc.h"
#include "gimpui.h"
#include "unitrc.h"
#include "user_install.h"

#include "libgimp/gimpenv.h"
#include "libgimp/gimplimits.h"
#include "libgimp/gimpmath.h"

#include "libgimp/gimpintl.h"

#include "pixmaps/eek.xpm"
#include "pixmaps/folder.xpm"
#include "pixmaps/new.xpm"
#include "pixmaps/wilber.xpm"

#ifndef G_OS_WIN32
#  ifndef __EMX__
#  define USER_INSTALL "user_install"
#  else
#  include <process.h>
#  define USER_INSTALL "user_install.cmd"
#  endif
#else
#  define STRICT
#  include <windows.h>
#  define USER_INSTALL "user_install.bat"
#endif

#define NUM_PAGES    6
#define EEK_PAGE     (NUM_PAGES - 1)
#define WILBER_WIDTH 62

#define PAGE_STYLE(widget) gtk_widget_set_style (widget, page_style)
#define TITLE_STYLE(widget) gtk_widget_set_style (widget, title_style)

static void     install_dialog_create     (InstallCallback);
static void     install_continue_callback (GtkWidget *widget, gpointer data);
static void     install_cancel_callback   (GtkWidget *widget, gpointer data);

static gboolean install_run               (void);
static void     install_tuning            (void);
static void     install_tuning_done       (void);
static void     install_resolution        (void);
static void     install_resolution_done   (void);


void
install_verify (InstallCallback install_callback)
{
  gboolean properly_installed = TRUE;
  gchar *filename;
  struct stat stat_buf;

  /* gimp_directory now always returns something */
  filename = gimp_directory ();

  if (stat (filename, &stat_buf) != 0)
    properly_installed = FALSE;

  /*  If there is already a proper installation, invoke the callback  */
  if (properly_installed)
    {
      (* install_callback) ();
    }
  /*  Otherwise, prepare for installation  */
  else if (no_interface)
    {
      g_print (_("The GIMP is not properly installed for the current user\n"));
      g_print (_("User installation was skipped because the '--nointerface' flag was encountered\n"));
      g_print (_("To perform user installation, run the GIMP without the '--nointerface' flag\n"));

      (* install_callback) ();
    }
  else
    {
      install_dialog_create (install_callback);
    }
}

/*  private stuff  */

static GtkWidget *install_dialog = NULL;

static GtkWidget *notebook       = NULL;

static GtkWidget *title_pixmap = NULL;

static GtkWidget *title_label  = NULL;
static GtkWidget *footer_label = NULL;

static GtkWidget *log_page        = NULL;
static GtkWidget *tuning_page     = NULL;
static GtkWidget *resolution_page = NULL;

static GtkWidget *continue_button = NULL;

static GtkStyle *title_style = NULL;

static GtkStyle    *page_style = NULL;
static GdkColormap *colormap   = NULL;

static GdkGC *white_gc = NULL;

static GdkColor black_color;
static GdkColor white_color;
static GdkColor title_color;

static struct
{
  gboolean  directory;
  gchar    *text;
  gchar    *description;
}
tree_items[] =
{
  {
    FALSE, "gimprc",
    N_("The gimprc is used to store personal preferences\n"
       "that affect GIMP's default behavior.\n"
       "Paths to search for brushes, palettes, gradients,\n"
       "patterns, plug-ins and modules can also configured\n"
       "here.")
  },
  {
    FALSE, "gtkrc",
    N_("GIMP uses an additional gtkrc file so you can\n"
       "configure it to look differently than other GTK apps.")
  },
  {
    FALSE, "pluginrc",
    N_("Plug-ins and extensions are external programs run\n"
       "by the GIMP which provide additional functionality.\n"
       "These programs are searched for at run-time and\n"
       "information about their functionality and mod-times\n"
       "is cached in this file.  This file is intended to\n"
       "be GIMP-readable only, and should not be edited.")
  },
  {
    FALSE, "menurc",
    N_("Key shortcuts can be dynamically redefined in The GIMP.\n"
       "The menurc is a dump of your configuration so it can.\n"
       "be remembered for the next session.  You may edit this\n"
       "file if you wish, but it is much easier to define the\n"
       "keys from within The GIMP.  Deleting this file will\n"
       "restore the default shortcuts.")
  },
  {
    FALSE, "sessionrc",
    N_("The sessionrc is used to store what dialog windows were\n"
       "open the last time you quit The GIMP.  You can configure\n"
       "The GIMP to reopen these dialogs at the saved position.")
  },
  {
    FALSE, "unitrc",
    N_("The unitrc is used to store your user units database.\n"
       "You can define additional units and use them just\n"
       "like you use the built-in units inches, millimeters,\n"
       "points and picas.  This file is overwritten each time\n"
       "you quit the GIMP.")
  },
  {
    TRUE, "brushes",
    N_("This is a subdirectory which can be used to store\n"
       "user defined brushes.  The default gimprc file\n"
       "checks this subdirectory in addition to the system-\n"
       "wide GIMP brushes installation when searching for\n"
       "brushes.")
  },
  {
    TRUE, "generated_brushes",
    N_("This is a subdirectory which is used to store brushes\n"
       "that are created with the brush editor.  The default\n"
       "gimprc file checks this subdirectory when searching\n"
       "for generated brushes.")
  },
  {
    TRUE, "gradients",
    N_("This is a subdirectory which can be used to store\n"
       "user defined gradients.  The default gimprc file\n"
       "checks this subdirectory in addition to the system-\n"
       "wide GIMP gradients installation when searching\n"
       "for gradients.")
  },
  {
    TRUE, "palettes",
    N_("This is a subdirectory which can be used to store\n"
       "user defined palettes.  The default gimprc file\n"
       "checks only this subdirectory (not the system-wide\n"
       "installation) when searching for palettes.  During\n"
       "installation, the system palettes will be copied\n"
       "here.  This is done to allow modifications made to\n"
       "palettes during GIMP execution to persist across\n"
       "sessions.")
  },
  {
    TRUE, "patterns",
    N_("This is a subdirectory which can be used to store\n"
       "user defined patterns.  The default gimprc file\n"
       "checks this subdirectory in addition to the system-\n"
       "wide GIMP patterns installation when searching for\n"
       "patterns.")
  },
  {
    TRUE, "plug-ins",
    N_("This is a subdirectory which can be used to store\n"
       "user created, temporary, or otherwise non-system-\n"
       "supported plug-ins.  The default gimprc file checks\n"
       "this subdirectory in addition to the systemwide\n"
       "GIMP plug-in directories when searching for plug-ins.")
  },
  {
    TRUE, "modules",
    N_("This subdirectory can be used to store user created,\n"
       "temporary, or otherwise non-system-supported DLL\n"
       "modules.  The default gimprc file checks this subdirectory\n"
       "in addition to the system-wide GIMP module directory\n"
       "when searching for modules to load when initializing.")
  },
  {
    TRUE, "scripts",
    N_("This subdirectory is used by the GIMP to store user\n"
       "created and installed scripts.  The default gimprc file\n"
       "checks this subdirectory in addition to the systemwide\n"
       "GIMP scripts subdirectory when searching for scripts")
  },
  {
    TRUE, "tmp",
    N_("This subdirectory is used by the GIMP to temporarily\n"
       "store undo buffers to reduce memory usage.  If GIMP is\n"
       "unceremoniously killed, files may persist in this directory\n"
       "of the form: gimp<#>.<#>.  These files are useless across\n"
       "GIMP sessions and can be destroyed with impunity.")
  },
  {
    TRUE, "curves",
    N_("This subdirectory is used to store parameter files for\n"
       "the Curves tool.")
  },
  {
    TRUE, "levels",
    N_("This subdirectory is used to store parameter files for\n"
       "the Levels tool.")
  },
  {
    TRUE, "fractalexplorer",
    N_("This is a subdirectory which can be used to store\n"
       "user defined fractals to be used by the FractalExplorer\n"
       "plug-in.  The default gimprc file checks this subdirectory\n"
       "in addition to the systemwide GIMP FractalExplorer\n" 
       "installation when searching for fractals.")
  },  
  {
    TRUE, "gfig",
    N_("This is a subdirectory which can be used to store\n"
       "user defined figures to be used by the GFig plug-in.\n"
       "The default gimprc file checks this subdirectory in\n"
       "addition to the systemwide GIMP GFig installation\n"
       "when searching for gfig figures.")
  },
  {
    TRUE, "gflare",
    N_("This is a subdirectory which can be used to store\n"
       "user defined gflares to be used by the GFlare plug-in.\n"
       "The default gimprc file checks this subdirectory in\n"
       "addition to the systemwide GIMP GFlares installation\n"
       "when searching for gflares.")
  },
  {
    TRUE, "gimpressionist",
    N_("This is a subdirectory which can be used to store\n"
       "user defined data to be used by the Gimpressionist\n"
       "plug-in.  The default gimprc file checks this subdirectory\n"
       "in addition to the systemwide GIMP Gimpressionist\n"
       "installation when searching for data.")    
  }  
};
static gint num_tree_items = sizeof (tree_items) / sizeof (tree_items[0]);

static void
install_notebook_set_page (GtkNotebook *notebook,
			   gint         index)
{
  gchar *title;
  gchar *footer;
  GtkWidget *page;
  
  page = gtk_notebook_get_nth_page (notebook, index);
  
  title  = gtk_object_get_data (GTK_OBJECT (page), "title");
  footer = gtk_object_get_data (GTK_OBJECT (page), "footer");

  gtk_label_set_text (GTK_LABEL (title_label), title);
  gtk_label_set_text (GTK_LABEL (footer_label), footer);

  if (index == EEK_PAGE)
    {
      gtk_widget_set_usize (title_pixmap, 
			    title_pixmap->allocation.width,
			    title_pixmap->allocation.height);
      gimp_pixmap_set (GIMP_PIXMAP (title_pixmap), eek_xpm);
    }
  
  gtk_notebook_set_page (notebook, index);
}

static void
install_continue_callback (GtkWidget *widget,
			   gpointer   data)
{
  static gint notebook_index = 0;
  InstallCallback callback;

  callback = (InstallCallback) data;

  switch (notebook_index)
    {
    case 0:
      break;
      
    case 1:
      if (!install_run ())
	gtk_widget_set_sensitive (continue_button, FALSE);
      break;

    case 2:
#ifdef G_OS_WIN32
      FreeConsole ();
#endif
      parse_buffers_init ();
      parse_unitrc ();
      parse_gimprc ();
      install_tuning ();
      break;

    case 3:
      install_tuning_done ();
      install_resolution ();
      break;

    case 4:
      install_resolution_done ();

      gtk_widget_destroy (install_dialog);
      gdk_gc_unref (white_gc);
      gtk_style_unref (title_style);
      gtk_style_unref (page_style);

      (* callback) ();
      return;
      break;

    case EEK_PAGE:
    default:
      g_assert_not_reached ();
    }

  if (notebook_index < NUM_PAGES - 1)
    install_notebook_set_page (GTK_NOTEBOOK (notebook), ++notebook_index);
}


static void
install_cancel_callback (GtkWidget *widget,
			 gpointer   data)
{
  static gint timeout = 0;

  if (timeout)
    gtk_exit (0);

  gtk_widget_destroy (continue_button);
  install_notebook_set_page (GTK_NOTEBOOK (notebook), EEK_PAGE);
  timeout = gtk_timeout_add (1024, (GtkFunction)gtk_exit, (gpointer)0);
}

static gint
install_corner_expose (GtkWidget      *widget,
		       GdkEventExpose *eevent,
		       gpointer        data)
{
  switch ((GtkCornerType) data)
    {
    case GTK_CORNER_TOP_LEFT:
      gdk_draw_arc (widget->window,
		    white_gc,
		    TRUE,
		    0, 0,
		    widget->allocation.width * 2,
		    widget->allocation.height * 2,
		    90 * 64,
		    180 * 64);
      break;
      
    case GTK_CORNER_BOTTOM_LEFT:
      gdk_draw_arc (widget->window,
		    white_gc,
		    TRUE,
		    0, -widget->allocation.height,
		    widget->allocation.width * 2,
		    widget->allocation.height * 2,
		    180 * 64,
		    270 * 64);
      break;
      
    case GTK_CORNER_TOP_RIGHT:
      gdk_draw_arc (widget->window,
		    white_gc,
		    TRUE,
		    -widget->allocation.width, 0,
		    widget->allocation.width * 2,
		    widget->allocation.height * 2,
		    0 * 64,
		    90 * 64);
      break;
      
    case GTK_CORNER_BOTTOM_RIGHT:
      gdk_draw_arc (widget->window,
		    white_gc,
		    TRUE,
		    -widget->allocation.width, -widget->allocation.height,
		    widget->allocation.width * 2,
		    widget->allocation.height * 2,
		    270 * 64,
		    360 * 64);
      break;
      
    default:
      return FALSE;
    }

  return TRUE;
}

static GtkWidget *
install_notebook_append_page (GtkNotebook *notebook,
			      gchar       *title,
			      gchar       *footer)
{
  GtkWidget *page;
  
  page = gtk_vbox_new (FALSE, 6);
  gtk_object_set_data (GTK_OBJECT (page), "title", title);
  gtk_object_set_data (GTK_OBJECT (page), "footer", footer);
  gtk_notebook_append_page (notebook, page, NULL);
  gtk_widget_show (page);

  return page;
}

static void
add_label (GtkBox   *box,
	   gchar    *text)
{
  GtkWidget *label;

  label = gtk_label_new (text);
  PAGE_STYLE (label);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (box, label, FALSE, FALSE, 0);
  
  gtk_widget_show (label);
}

static void
install_ctree_select_row (GtkWidget      *widget,
			  gint            row, 
			  gint            column, 
			  GdkEventButton *bevent,
			  gpointer        data)
{
  GtkNotebook *notebook;

  notebook = (GtkNotebook*) data;

  gtk_notebook_set_page (notebook, row);
}

void
install_dialog_create (InstallCallback callback)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *ebox;
  GtkWidget *table;
  GtkWidget *darea;
  GtkWidget *page;
  GtkWidget *sep;
  GdkFont   *large_font;

  dialog = install_dialog =
    gimp_dialog_new (_("GIMP User Installation"), "user_installation",
		     NULL, NULL,
		     GTK_WIN_POS_CENTER,
		     FALSE, FALSE, FALSE,

		     _("Continue"), install_continue_callback,
		     callback, NULL, &continue_button, TRUE, FALSE,
		     _("Cancel"), install_cancel_callback,
		     callback, 1, NULL, FALSE, TRUE,

		     NULL);

  gimp_dialog_set_icon (GTK_WINDOW (dialog));

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 8);

  /*  hide the separator between the dialog's vbox and the action area  */
  gtk_widget_destroy (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_BIN (dialog)->child)), 0)));

  gtk_widget_realize (dialog);

  /*  B/W Style for the page contents  */
  page_style = gtk_style_copy (gtk_widget_get_default_style ());
  colormap = gtk_widget_get_colormap (dialog);

  gdk_color_black (colormap, &black_color);
  gdk_color_white (colormap, &white_color);

  page_style->fg[GTK_STATE_NORMAL]   = black_color;
  page_style->text[GTK_STATE_NORMAL] = black_color;
  page_style->bg[GTK_STATE_NORMAL]   = white_color;

  gdk_font_unref (page_style->font);
  page_style->font = dialog->style->font;
  gdk_font_ref (page_style->font);

  /*  B/Colored Style for the page title  */
  title_style = gtk_style_copy (page_style);

  if (gdk_color_parse ("dark orange", &title_color) &&
      gdk_colormap_alloc_color (colormap, &title_color, FALSE, TRUE))
    {
      title_style->bg[GTK_STATE_NORMAL] = title_color;
    }

  /*  this is a fontset, e.g. multiple comma-separated font definitions  */
  large_font = gdk_fontset_load (_("-*-helvetica-bold-r-normal-*-*-240-*-*-*-*-*-*,*"));

  if (large_font)
    {
      gdk_font_unref (title_style->font);
      title_style->font = large_font;
      gdk_font_ref (title_style->font);
    }
  
  /*  W/W GC for the corner  */
  white_gc = gdk_gc_new (dialog->window);
  gdk_gc_set_foreground (white_gc, &white_color);

  TITLE_STYLE (dialog);

  footer_label = gtk_label_new (NULL);
  PAGE_STYLE (footer_label);
  gtk_label_set_justify (GTK_LABEL (footer_label), GTK_JUSTIFY_RIGHT);
  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->action_area), footer_label,
		    FALSE, FALSE, 8);
  gtk_widget_show (footer_label);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);

  ebox = gtk_event_box_new ();
  TITLE_STYLE (ebox);
  gtk_widget_set_events (ebox, GDK_EXPOSURE_MASK);
  gtk_widget_set_usize (ebox, WILBER_WIDTH + 16, -1);
  gtk_box_pack_start (GTK_BOX (vbox), ebox, FALSE, FALSE, 0);
  gtk_widget_show (ebox);
  
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_container_add (GTK_CONTAINER (ebox), hbox);
  gtk_widget_show (hbox);

  title_pixmap = gimp_pixmap_new (wilber_xpm);
  gtk_box_pack_start (GTK_BOX (hbox), title_pixmap, FALSE, FALSE, 8);
  gtk_widget_show (title_pixmap);

  title_label = gtk_label_new (NULL);
  TITLE_STYLE (title_label);  
  gtk_label_set_justify (GTK_LABEL (title_label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (hbox), title_label, FALSE, FALSE, 0);
  gtk_widget_show (title_label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  ebox = gtk_event_box_new ();
  TITLE_STYLE (ebox);  
  gtk_widget_set_usize (ebox, 16, -1);
  gtk_box_pack_start (GTK_BOX (hbox), ebox, FALSE, FALSE, 0);
  gtk_widget_show (ebox);

  ebox = gtk_event_box_new ();
  PAGE_STYLE (ebox);  
  gtk_box_pack_start (GTK_BOX (hbox), ebox, TRUE, TRUE, 0);
  gtk_widget_show (ebox);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 8);
  gtk_container_add (GTK_CONTAINER (ebox), table);
  gtk_widget_show (table);

  darea = gtk_drawing_area_new ();
  TITLE_STYLE (darea);  
  gtk_drawing_area_size (GTK_DRAWING_AREA (darea), 16, 16);
  gtk_signal_connect_after (GTK_OBJECT (darea), "expose_event",
			    GTK_SIGNAL_FUNC (install_corner_expose),
			    (gpointer) GTK_CORNER_TOP_LEFT);
  gtk_table_attach (GTK_TABLE (table), darea, 0, 1, 0, 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (darea);

  darea = gtk_drawing_area_new ();
  TITLE_STYLE (darea);  
  gtk_drawing_area_size (GTK_DRAWING_AREA (darea), 16, 16);
  gtk_signal_connect_after (GTK_OBJECT (darea), "expose_event",
			    GTK_SIGNAL_FUNC (install_corner_expose),
			    (gpointer) GTK_CORNER_BOTTOM_LEFT);
  gtk_table_attach (GTK_TABLE (table), darea, 0, 1, 2, 3,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (darea);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), notebook, 1, 2, 1, 2);
  gtk_widget_show (notebook);

  gtk_widget_show (vbox);

  /*  Page 1  */
  page = install_notebook_append_page (GTK_NOTEBOOK (notebook),
				       _("Welcome to\n"
					 "The GIMP User Installation"),
				       _("Click \"Continue\" to enter the GIMP user installation."));

  add_label (GTK_BOX (page),
	     _("The GIMP - GNU Image Manipulation Program\n"
	       "Copyright (C) 1995-2000\n"
	       "Spencer Kimball, Peter Mattis and the GIMP Development Team."));

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (page), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  add_label (GTK_BOX (page),
	     _("This program is free software; you can redistribute it and/or modify\n"
	       "it under the terms of the GNU General Public License as published by\n"
	       "the Free Software Foundation; either version 2 of the License, or\n"
	       "(at your option) any later version."));
  add_label (GTK_BOX (page),
	     _("This program is distributed in the hope that it will be useful,\n"
	       "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
	       "See the GNU General Public License for more details."));
  add_label (GTK_BOX (page),
	     _("You should have received a copy of the GNU General Public License\n"
	       "along with this program; if not, write to the Free Software\n"
	       "Foundation, Inc., 59 Temple Place - Suite 330, Boston,\n"
	       "MA 02111-1307, USA."));

  /*  Page 2  */
  {
    GtkWidget *hbox;
    GtkWidget *vbox;
    GtkWidget *ctree;
    GtkWidget *notebook2;
    GtkWidget *page2;
    GtkWidget *label;
    GtkCTreeNode *main_node = NULL;
    GtkCTreeNode *sub_node = NULL;
    GdkPixmap *file_pixmap;
    GdkBitmap *file_mask;
    GdkPixmap *folder_pixmap;
    GdkBitmap *folder_mask;
    gchar     *str;

    gint i;

    gchar *node[1];
    
    page = install_notebook_append_page (GTK_NOTEBOOK (notebook),
					 _("Personal GIMP Directory"),
					 _("Click \"Continue\" to create your personal GIMP directory."));

    hbox = gtk_hbox_new (FALSE, 8);
    gtk_box_pack_start (GTK_BOX (page), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    ctree = gtk_ctree_new (1, 0);
    PAGE_STYLE (ctree);
    gtk_ctree_set_indent (GTK_CTREE (ctree), 12);
    gtk_clist_set_shadow_type (GTK_CLIST (ctree), GTK_SHADOW_NONE);
    gtk_box_pack_start (GTK_BOX (hbox), ctree, FALSE, FALSE, 0);
    gtk_widget_show (ctree);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);

    str = g_strdup_printf (_("For a proper GIMP installation, a subdirectory named\n"
			     "%s needs to be created."), gimp_directory ());
    add_label (GTK_BOX (vbox), str);
    g_free (str);

    add_label (GTK_BOX (vbox),
	       _("This subdirectory will contain a number of important files.\n"
		 "Click on one of the files or subdirectories in the tree\n"
		 "to get more information about the selected item."));

    notebook2 = gtk_notebook_new ();
    gtk_container_set_border_width (GTK_CONTAINER (notebook2), 8);
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook2), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook2), FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), notebook2, TRUE, TRUE, 0);
    gtk_widget_show (notebook2);

    /*  empty page  */
    page2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (page2);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook2), page2, NULL);

    node[0] = gimp_directory ();

    main_node = gtk_ctree_insert_node (GTK_CTREE (ctree), NULL, NULL,
				       node, 4,
				       NULL, NULL, NULL, NULL,
				       FALSE, TRUE);	  

    gtk_signal_connect (GTK_OBJECT (ctree), "select_row",
			GTK_SIGNAL_FUNC (install_ctree_select_row),
			notebook2);

    file_pixmap = gdk_pixmap_create_from_xpm_d (dialog->window,
						&file_mask,
						&page_style->bg[GTK_STATE_NORMAL],
						new_xpm);
    folder_pixmap = gdk_pixmap_create_from_xpm_d (dialog->window,
						  &folder_mask,
						  &page_style->bg[GTK_STATE_NORMAL],
						  folder_xpm);

    for (i = 0; i < num_tree_items; i++)
      {
	node[0] = tree_items[i].text;

	if (tree_items[i].directory)
	  {
	    sub_node = gtk_ctree_insert_node (GTK_CTREE (ctree), main_node, NULL,
					      node, 4,
					      folder_pixmap, folder_mask,
					      folder_pixmap, folder_mask,
					      FALSE, TRUE);	  
	  }
	else
	  {
	    sub_node = gtk_ctree_insert_node (GTK_CTREE (ctree), main_node, NULL,
					      node, 4,
					      file_pixmap, file_mask,
					      file_pixmap, file_mask,
					      FALSE, TRUE);
	  }

	page2 = gtk_vbox_new (FALSE, 0);
	label = gtk_label_new (gettext (tree_items[i].description));
	PAGE_STYLE (label);
	PAGE_STYLE (label);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (page2), label, TRUE, TRUE, 0);
	gtk_widget_show (label);
	gtk_widget_show (page2);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook2), page2, NULL);
      }

    gtk_clist_set_column_width (GTK_CLIST (ctree), 0,
				gtk_clist_optimal_column_width (GTK_CLIST (ctree), 0));

    gtk_widget_set_usize (ctree, -1, ctree->requisition.height);

    gdk_pixmap_unref (file_pixmap);
    gdk_bitmap_unref (file_mask);
    gdk_pixmap_unref (folder_pixmap);
    gdk_bitmap_unref (folder_mask);
  }
  
  /*  Page 3  */
  page = log_page =
    install_notebook_append_page (GTK_NOTEBOOK (notebook),
				  _("User Installation Log"),
				  NULL);

  /*  Page 4  */
  page = tuning_page = 
    install_notebook_append_page (GTK_NOTEBOOK (notebook),
				  _("GIMP Performance Tuning"),
				  _("Click \"Continue\" to accept the settings above."));
  
  add_label (GTK_BOX (page),
	     _("For optimal GIMP performance, some settings may have to be adjusted."));

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (page), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  /*  Page 5  */
  page = resolution_page = 
    install_notebook_append_page (GTK_NOTEBOOK (notebook),
				  _("Monitor Resolution"),
				  _("Click \"Continue\" to start The GIMP."));
  
  add_label (GTK_BOX (resolution_page),
	     _("To display images in their natural size, "
	       "GIMP needs to know your monitor resolution."));

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (page), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  /*  EEK page  */
  page = install_notebook_append_page (GTK_NOTEBOOK (notebook),
				       _("Aborting Installation..."),
				       NULL);

  install_notebook_set_page (GTK_NOTEBOOK (notebook), 0);

  gtk_widget_show (dialog);
}


/*********************/
/*  Local functions  */

#ifdef G_OS_WIN32

char *
quote_spaces (char *string)
{
  int nspaces = 0;
  char *p = string, *q, *new;

  while (*p)
    {
      if (*p == ' ')
	nspaces++;
      p++;
    }

  if (nspaces == 0)
    return g_strdup (string);

  new = g_malloc (strlen (string) + nspaces*2 + 1);

  p = string;
  q = new;
  while (*p)
    {
      if (*p == ' ')
	{
	  *q++ = '"';
	  *q++ = ' ';
	  *q++ = '"';
	}
      else
	*q++ = *p;
      p++;
    }
  *q = '\0';

  return new;
}

#endif

static gboolean
install_run (void)
{
  FILE *pfp;
  gchar buffer[2048];
  struct stat stat_buf;
  gint err;
  gboolean executable = TRUE;

  /*  Generate output  */
  g_snprintf (buffer, sizeof (buffer), "%s" G_DIR_SEPARATOR_S USER_INSTALL,
	      gimp_data_directory ());
  if ((err = stat (buffer, &stat_buf)) != 0)
    {
      gchar *str;
      
      str = g_strdup_printf ("%s\n%s", buffer,
			     _("does not exist.  Cannot install."));
      add_label (GTK_BOX (log_page), str);
      g_free (str);

      executable = FALSE;
    }
#ifdef S_IXUSR
  else if (! (S_IXUSR & stat_buf.st_mode) || ! (S_IRUSR & stat_buf.st_mode))
    {
      gchar *str;
      
      str = g_strdup_printf ("%s\n%s", buffer,
			     _("has invalid permissions.  Cannot install."));
      add_label (GTK_BOX (log_page), str);
      g_free (str);

      executable = FALSE;
    }
#endif

  if (executable)
    {
#ifdef G_OS_WIN32
      char *quoted_data_dir, *quoted_user_dir;

      /* On Windows, it is common for the GIMP data directory
       * to have spaces in it ("c:\Program Files\GIMP"). Put spaces in quotes.
       */
      quoted_data_dir = quote_spaces (gimp_data_directory ());
      quoted_user_dir = quote_spaces (gimp_directory ());

      /* The Microsoft _popen doesn't work in Windows applications, sigh.
       * Do the installation by calling system(). The user_install.bat
       * ends with a pause command, so the user has to press enter in
       * the console window to continue, and thus has a chance to read
       * at the window contents.
       */

      AllocConsole ();
      g_snprintf (buffer, sizeof(buffer), "%s" G_DIR_SEPARATOR_S USER_INSTALL " %s %s",
		  quoted_data_dir, quoted_data_dir,
		  quoted_user_dir);

      if (system (buffer) == -1)
	executable = FALSE;
      g_free (quoted_data_dir);
      g_free (quoted_user_dir);

      if (executable)
	add_label (GTK_BOX (log_page),
		   _("Did you notice any error messages in the console window?\n"
		     "If not, installation was successful!\n"
		     "Otherwise, quit and investigate the possible reason..."));
#else
#ifndef __EMX__
      g_snprintf (buffer, sizeof(buffer), "%s" G_DIR_SEPARATOR_S USER_INSTALL " %s %s %s",
		  gimp_data_directory (),
		  "2>&1",
		  gimp_data_directory(), gimp_directory ());
#else
      g_snprintf (buffer, sizeof(buffer), "cmd.exe /c %s" G_DIR_SEPARATOR_S USER_INSTALL " %s %s",
		  gimp_data_directory (), gimp_data_directory(),
		  gimp_directory ());
      {
	char *s = buffer + 10;
	while (*s)
	  {
	    if (*s == '/') *s = '\\';
	    s++;
	  }
      }
#endif

      /* urk - should really use something better than popen(), since
       * we can't tell if the installation script failed --austin */
      if ((pfp = popen (buffer, "r")) != NULL)
	{
	  GtkWidget *table;
	  GtkWidget *log_text;
	  GtkWidget *vsb;
	  GtkAdjustment *vadj;

	  table = gtk_table_new (1, 2, FALSE);
	  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
	  gtk_box_pack_start (GTK_BOX (log_page), table, TRUE, TRUE, 0);

	  vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
	  vsb  = gtk_vscrollbar_new (vadj);
	  log_text = gtk_text_new (NULL, vadj);

	  gtk_table_attach (GTK_TABLE (table), vsb, 1, 2, 0, 1,
			    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
	  gtk_table_attach (GTK_TABLE (table), log_text, 0, 1, 0, 1,
			    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			    0, 0);

	  gtk_widget_show (vsb);
	  gtk_widget_show (log_text);
	  gtk_widget_show (table);

	  while (fgets (buffer, sizeof (buffer), pfp))
	    gtk_text_insert (GTK_TEXT (log_text), NULL, NULL, NULL, buffer, -1);
	  pclose (pfp);

	  add_label (GTK_BOX (log_page),
		     _("Did you notice any error messages in the lines above?\n"
		       "If not, installation was successful!\n"
		       "Otherwise, quit and investigate the possible reason..."));
	}
      else
	executable = FALSE;
#endif /* !G_OS_WIN32 */
    }

  if (executable)
    {
      gtk_object_set_data (GTK_OBJECT (log_page), "footer",
           _("Click \"Continue\" to complete GIMP installation."));
    }
  else
    {
      add_label (GTK_BOX (log_page),
		 _("Installation failed.  Contact system administrator."));
    }

  return executable;
}

static GtkObject *tile_cache_adj    = NULL;
static GtkWidget *swap_path_filesel = NULL;
static GtkWidget *xserver_toggle    = NULL;
static GtkWidget *resolution_entry  = NULL;

static void
install_tuning (void)
{
  GtkWidget *hbox;
  GtkWidget *sep;
  GtkWidget *label;
  GtkWidget *memsize;

  /*  tile cache size  */
  add_label (GTK_BOX (tuning_page),
	     _("GIMP uses a limited amount of memory to store image data, the so-called\n"
	       "\"Tile Cache\". You should adjust it's size to fit into memory. Consider\n"
	       "the amount of memory used by other running processes."));

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (tuning_page), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  tile_cache_adj = gtk_adjustment_new (tile_cache_size, 
				       0, (4069.0 * 1024 * 1024), 1.0, 1.0, 0.0);
  memsize = gimp_mem_size_entry_new (GTK_ADJUSTMENT (tile_cache_adj));
  gtk_box_pack_end (GTK_BOX (hbox), memsize, FALSE, FALSE, 0);
  gtk_widget_show (memsize);

  label = gtk_label_new (_("Tile Cache Size:"));
  PAGE_STYLE (label);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  
  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (tuning_page), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  /*  swap file location  */
  add_label (GTK_BOX (tuning_page),
	     _("All image and undo data which doesn't fit into the Tile Cache will be\n"
	       "written to a swap file. This file should be located on a local filesystem\n"
	       "with enough free space (several hundred MB). On a UNIX system, you\n"
	       "may want to use the system-wide temp-dir (\"/tmp\" or \"/var/tmp\")."));

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (tuning_page), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  swap_path_filesel = gimp_file_selection_new (_("Select Swap Dir"), swap_path,
					       TRUE, TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), swap_path_filesel, FALSE, FALSE, 0);
  gtk_widget_show (swap_path_filesel);

  label = gtk_label_new (_("Swap Directory:"));
  PAGE_STYLE (label);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
}

static void
install_tuning_done (void)
{
}

static GtkWidget *calibrate_entry = NULL;
static gdouble    calibrate_xres  = 1.0;
static gdouble    calibrate_yres  = 1.0;
static gint       ruler_width     = 1;
static gint       ruler_height    = 1;

static void
install_resolution_calibrate_ok (GtkWidget *button,
				 gpointer   data)
{
  gdouble x, y;

  x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (calibrate_entry), 0);
  y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (calibrate_entry), 1);

  calibrate_xres = (gdouble)ruler_width * calibrate_xres / x;
  calibrate_yres = (gdouble)ruler_height * calibrate_yres / y;
  
  if (ABS (x -y) > GIMP_MIN_RESOLUTION)
    gimp_chain_button_set_active (GIMP_COORDINATES_CHAINBUTTON (resolution_entry), FALSE);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (resolution_entry), 0, calibrate_xres);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (resolution_entry), 1, calibrate_yres);

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
install_resolution_calibrate (GtkWidget *button,
			      gpointer   data)
{
  GtkWidget *dialog;
  GtkWidget *table;
  GtkWidget *ebox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *darea;
  GtkWidget *ruler;
  GList     *list;

  gtk_widget_hide (install_dialog);

  dialog = gimp_dialog_new (_("Calibrate Monitor Resolution"),
			    "calibrate_resolution",
			    NULL, NULL,
			    GTK_WIN_POS_CENTER,
			    FALSE, FALSE, FALSE,

			    _("OK"), install_resolution_calibrate_ok,
			    NULL, NULL, NULL, TRUE, FALSE,
			    _("Cancel"), gtk_widget_destroy,
			    NULL, 1, NULL, FALSE, TRUE,

			    NULL);

  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);
  
  TITLE_STYLE (dialog);
  
  gimp_dialog_set_icon (GTK_WINDOW (dialog));

  gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area), 8);

  /*  hide the separator between the dialog's vbox and the action area  */
  /*  gtk_widget_destroy (GTK_WIDGET (g_list_nth_data (gtk_container_children (GTK_CONTAINER (GTK_BIN (dialog)->child)), 0)));*/

  ruler_width  = gdk_screen_width ();
  ruler_height = gdk_screen_height ();

  ruler_width  = ruler_width - 300 - (ruler_width % 100);
  ruler_height = ruler_height - 300 - (ruler_height % 100);

  table = gtk_table_new (4, 4, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 16);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), table);
  gtk_widget_show (table);
  
  ruler = gtk_hruler_new ();
  PAGE_STYLE (ruler);
  gtk_widget_set_usize (ruler, ruler_width, 32);
  gtk_ruler_set_range (GTK_RULER (ruler), 0, ruler_width, 0, ruler_width);
  gtk_table_attach (GTK_TABLE (table), ruler, 1, 3, 0, 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (ruler);

  ruler = gtk_vruler_new ();
  PAGE_STYLE (ruler);
  gtk_widget_set_usize (ruler, 32, ruler_height);
  gtk_ruler_set_range (GTK_RULER (ruler), 0, ruler_height, 0, ruler_height);
  gtk_table_attach (GTK_TABLE (table), ruler, 0, 1, 1, 3,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (ruler);

  ebox = gtk_event_box_new ();
  PAGE_STYLE (ebox);  
  gtk_table_attach (GTK_TABLE (table), ebox, 1, 2, 1, 2,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (ebox);

  table = gtk_table_new (3, 3, FALSE);
  gtk_container_add (GTK_CONTAINER (ebox), table);
  gtk_widget_show (table);

  darea = gtk_drawing_area_new ();
  TITLE_STYLE (darea);  
  gtk_drawing_area_size (GTK_DRAWING_AREA (darea), 16, 16);
  gtk_signal_connect_after (GTK_OBJECT (darea), "expose_event",
			    GTK_SIGNAL_FUNC (install_corner_expose),
			    (gpointer) GTK_CORNER_TOP_LEFT);
  gtk_table_attach (GTK_TABLE (table), darea, 0, 1, 0, 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (darea);

  darea = gtk_drawing_area_new ();
  TITLE_STYLE (darea);  
  gtk_drawing_area_size (GTK_DRAWING_AREA (darea), 16, 16);
  gtk_signal_connect_after (GTK_OBJECT (darea), "expose_event",
			    GTK_SIGNAL_FUNC (install_corner_expose),
			    (gpointer) GTK_CORNER_BOTTOM_LEFT);
  gtk_table_attach (GTK_TABLE (table), darea, 0, 1, 2, 3,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (darea);

  darea = gtk_drawing_area_new ();
  TITLE_STYLE (darea);  
  gtk_drawing_area_size (GTK_DRAWING_AREA (darea), 16, 16);
  gtk_signal_connect_after (GTK_OBJECT (darea), "expose_event",
			    GTK_SIGNAL_FUNC (install_corner_expose),
			    (gpointer) GTK_CORNER_TOP_RIGHT);
  gtk_table_attach (GTK_TABLE (table), darea, 2, 3, 0, 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (darea);

  darea = gtk_drawing_area_new ();
  TITLE_STYLE (darea);  
  gtk_drawing_area_size (GTK_DRAWING_AREA (darea), 16, 16);
  gtk_signal_connect_after (GTK_OBJECT (darea), "expose_event",
			    GTK_SIGNAL_FUNC (install_corner_expose),
			    (gpointer) GTK_CORNER_BOTTOM_RIGHT);
  gtk_table_attach (GTK_TABLE (table), darea, 2, 3, 2, 3,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (darea);

  vbox = gtk_vbox_new (FALSE, 16);
  gtk_table_attach_defaults (GTK_TABLE (table), vbox, 1, 2, 1, 2);
  gtk_widget_show (vbox);
  
  add_label (GTK_BOX (vbox),
	     _("Measure the rulers and enter their lengths below."));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  calibrate_xres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (resolution_entry), 0);
  calibrate_yres = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (resolution_entry), 1);

  calibrate_entry =
    gimp_coordinates_new  (GIMP_UNIT_INCH, "%p",
			   FALSE, FALSE, 75,
			   GIMP_SIZE_ENTRY_UPDATE_SIZE,
			   FALSE,
			   FALSE,
			   _("Horizontal:"),
			   ruler_width,
			   calibrate_xres,
			   1, GIMP_MAX_IMAGE_SIZE,
			   0, 0,
			   _("Vertical:"),
			   ruler_height,
			   calibrate_yres,
			   1, GIMP_MAX_IMAGE_SIZE,
			   0, 0);
  gtk_widget_hide (GTK_WIDGET (GIMP_COORDINATES_CHAINBUTTON (calibrate_entry)));
  
  for (list = GTK_TABLE (calibrate_entry)->children;
       list;
       list = g_list_next (list))
    {
      GtkTableChild *child = (GtkTableChild *) list->data;

      if (child && GTK_IS_LABEL (child->widget))
	PAGE_STYLE (GTK_WIDGET (child->widget));
    }

  gtk_box_pack_end (GTK_BOX (hbox), calibrate_entry, FALSE, FALSE, 0);
  gtk_widget_show (calibrate_entry);
  
  gtk_widget_show (dialog);

  gtk_main ();
  gtk_widget_show (install_dialog);
}

static void
install_resolution (void)
{
  GtkWidget *hbox;
  GtkWidget *sep;
  GimpChainButton *chain;
  GtkWidget *button;
  GList     *list; 
  gchar     *pixels_per_unit;
  gdouble    xres, yres;
  gchar     *str;

  gdisplay_xserver_resolution (&xres, &yres);

  add_label (GTK_BOX (resolution_page),
	     _("GIMP can obtain this information from your X-server.\n"
	       "However, most X-servers do not return useful values."));
  
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (resolution_page), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  str = g_strdup_printf (_("Get Resolution from X-server (Currently %d x %d dpi)"),
			 ROUND (xres), ROUND (yres));
  xserver_toggle = gtk_check_button_new_with_label (str);
  g_free (str);

  PAGE_STYLE (GTK_BIN (xserver_toggle)->child);
  gtk_box_pack_end (GTK_BOX (hbox), xserver_toggle, FALSE, FALSE, 0);
  gtk_widget_show (xserver_toggle);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (resolution_page), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  add_label (GTK_BOX (resolution_page),
	     _("Alternatively, you can set the monitor resolution manually."));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (resolution_page), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  pixels_per_unit = g_strconcat (_("Pixels"), "/%s", NULL);
  resolution_entry =
    gimp_coordinates_new (GIMP_UNIT_INCH, pixels_per_unit,
			  FALSE, FALSE, 75,
			  GIMP_SIZE_ENTRY_UPDATE_RESOLUTION,
			  abs (monitor_xres - monitor_yres) < GIMP_MIN_RESOLUTION,
			  FALSE,
			  _("Monitor Resolution X:"),
			  monitor_xres,
			  1.0,
			  GIMP_MIN_RESOLUTION,
			  GIMP_MAX_RESOLUTION,
			  0, 0,
			  _("Y:"),
			  monitor_yres,
			  1.0,
			  GIMP_MIN_RESOLUTION,
			  GIMP_MAX_RESOLUTION,
			  0, 0);
  g_free (pixels_per_unit);

  chain = GIMP_COORDINATES_CHAINBUTTON (resolution_entry);
  PAGE_STYLE (GTK_WIDGET (chain->line1));
  PAGE_STYLE (GTK_WIDGET (chain->line2));

  for (list = GTK_TABLE (resolution_entry)->children;
       list;
       list = g_list_next (list))
    {
      GtkTableChild *child = (GtkTableChild *) list->data;

      if (child && GTK_IS_LABEL (child->widget))
	PAGE_STYLE (GTK_WIDGET (child->widget));
    }

  gtk_box_pack_end (GTK_BOX (hbox), resolution_entry, FALSE, FALSE, 0);
  gtk_widget_show (resolution_entry);

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (resolution_page), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  add_label (GTK_BOX (resolution_page),
	     _("You can also press the \"Calibrate\" button to open a window\n"
	       "which lets you determine your monitor resolution interactively."));

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (resolution_page), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("Calibrate"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 4, 0);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (install_resolution_calibrate),
		      NULL);
  gtk_widget_show (button);
  
  gtk_object_set_data (GTK_OBJECT (xserver_toggle), "inverse_sensitive",
		       resolution_entry);
  gtk_object_set_data (GTK_OBJECT (resolution_entry), "inverse_sensitive",
		       button);
  gtk_signal_connect (GTK_OBJECT (xserver_toggle), "toggled",
		      GTK_SIGNAL_FUNC (gimp_toggle_button_sensitive_update),
		      NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (xserver_toggle),
				using_xserver_resolution);
}

static void
install_resolution_done (void)
{
  GList *update = NULL;
  GList *remove = NULL;

  gint      new_tile_cache_size;
  gchar    *new_swap_path;
  gboolean  new_using_xserver_resolution;
  gdouble   new_monitor_xres;
  gdouble   new_monitor_yres;
  
  new_tile_cache_size = GTK_ADJUSTMENT (tile_cache_adj)->value;
  new_swap_path =
    gimp_file_selection_get_filename (GIMP_FILE_SELECTION (swap_path_filesel));
  new_using_xserver_resolution =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (xserver_toggle));
  new_monitor_xres =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (resolution_entry), 0);
  new_monitor_yres =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (resolution_entry), 1);

  if (tile_cache_size != new_tile_cache_size)
    {
      tile_cache_size = new_tile_cache_size;
      update = g_list_append (update, "tile-cache-size");
    }
  if (strcmp (swap_path, new_swap_path))
    {
      g_free (swap_path);
      swap_path = new_swap_path;
      update = g_list_append (update, "swap-path");
    }
  if (using_xserver_resolution != new_using_xserver_resolution ||
      ABS (monitor_xres - new_monitor_xres) > GIMP_MIN_RESOLUTION)
    {
      monitor_xres = new_monitor_xres;
      update = g_list_append (update, "monitor-xresolution");
    }
  if (using_xserver_resolution != new_using_xserver_resolution ||
      ABS (monitor_yres - new_monitor_yres) > GIMP_MIN_RESOLUTION)
    {
      monitor_yres = new_monitor_yres;
      update = g_list_append (update, "monitor-yresolution");
    }

  using_xserver_resolution = new_using_xserver_resolution;

  if (using_xserver_resolution)
    {
      /* special value of 0 for either x or y res in the gimprc file
       * means use the xserver's current resolution */
      monitor_xres = 0.0;
      monitor_yres = 0.0;
    }

  save_gimprc (&update, &remove);

  if (using_xserver_resolution)
    gdisplay_xserver_resolution (&monitor_xres, &monitor_yres);

  g_list_free (update);
  g_list_free (remove);

}
