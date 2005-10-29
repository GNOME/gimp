/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2000 Michael Natterer and Sven Neumann
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <libgimpbase/gimpwin32-io.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "config/gimpconfig-file.h"
#include "config/gimprc.h"

#include "core/gimp-templates.h"

#include "widgets/gimpwidgets-utils.h"

#include "user-install-dialog.h"

#include "gimp-intl.h"


#define PAGE_STYLE(widget)  gtk_widget_modify_style (widget, page_style)
#define TITLE_STYLE(widget) gtk_widget_modify_style (widget, title_style)

enum
{
  GPL_PAGE,
  MIGRATION_PAGE,
  TREE_PAGE,
  LOG_PAGE,
  TUNING_PAGE,
  NUM_PAGES
};

enum
{
  DIRENT_COLUMN,
  PIXBUF_COLUMN,
  DESC_COLUMN,
  NUM_COLUMNS
};


static void      user_install_response (GtkWidget   *widget,
                                        gint         response_id,
                                        GimpRc      *gimprc);

static gboolean  user_install_run      (const gchar *oldgimp);
static void      user_install_tuning   (GimpRc      *gimprc);


/*  private stuff  */

static GtkWidget  *user_install_dialog = NULL;

static GtkWidget  *notebook        = NULL;

static GtkWidget  *title_image     = NULL;

static GtkWidget  *title_label     = NULL;
static GtkWidget  *footer_label    = NULL;

static GtkWidget  *log_page        = NULL;
static GtkWidget  *tuning_page     = NULL;

static GtkRcStyle *title_style     = NULL;
static GtkRcStyle *page_style      = NULL;

static GdkColor    black_color;
static GdkColor    white_color;
static GdkColor    title_color;

static gchar      *oldgimp         = NULL;
static gint        oldgimp_major   = 0;
static gint        oldgimp_minor   = 0;
static gboolean    migrate         = FALSE;


typedef enum
{
  TREE_ITEM_DO_NOTHING,        /* Don't pre-create            */
  TREE_ITEM_MKDIR,             /* Create the directory        */
  TREE_ITEM_FROM_SYSCONF_DIR   /* Copy from sysconf directory */
} TreeItemType;

static struct
{
  gboolean      directory;
  gchar        *name;
  gchar        *description;
  TreeItemType  type;
}
tree_items[] =
{
  {
    FALSE, "gimprc",
    N_("The gimprc is used to store personal preferences "
       "that affect GIMP's default behavior.  "
       "Paths to search for brushes, palettes, gradients, "
       "patterns, plug-ins and modules can also configured "
       "here."),
    TREE_ITEM_DO_NOTHING
  },
  {
    FALSE, "gtkrc",
    N_("GIMP uses an additional gtkrc file so you can "
       "configure it to look differently than other GTK apps."),
    TREE_ITEM_FROM_SYSCONF_DIR
  },
  {
    FALSE, "pluginrc",
    N_("Plug-ins and extensions are external programs which "
       "provide additional functionality to GIMP.  These "
       "programs are searched for at run-time and information "
       "about their functionality is cached in this file.  "
       "This file is intended to be written to by GIMP only, "
       "and should not be edited."),
    TREE_ITEM_DO_NOTHING
  },
  {
    FALSE, "menurc",
    N_("Key shortcuts can be dynamically redefined. "
       "The menurc is a dump of your configuration so it can "
       "be remembered for the next session.  You may edit this "
       "file if you wish, but it is much easier to define the "
       "keys from within GIMP.  Deleting this file will "
       "restore the default shortcuts."),
    TREE_ITEM_DO_NOTHING
  },
  {
    FALSE, "sessionrc",
    N_("The sessionrc is used to store what dialog windows were "
       "open the last time you quit GIMP.  You can configure "
       "GIMP to reopen these dialogs at the saved position."),
    TREE_ITEM_DO_NOTHING
  },
  {
    FALSE, "templaterc",
    N_("This file holds a collection of standard media sizes that "
       "serve as image templates."),
    TREE_ITEM_DO_NOTHING
  },
  {
    FALSE, "unitrc",
    N_("The unitrc is used to store your user units database.  "
       "You can define additional units and use them just "
       "like you use the built-in units inches, millimeters, "
       "points and picas.  This file is overwritten each time "
       "you quit the GIMP."),
    TREE_ITEM_DO_NOTHING
  },
  {
    TRUE, "brushes",
    N_("This folder is used to store user defined brushes. "
       "GIMP checks this folder in addition to the system-wide "
       "brushes installation."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "fonts",
    N_("This folder is used to store fonts you only want to be "
       "visible in GIMP. GIMP checks this folder in addition to "
       "the system-wide fonts installation. Use this only if you really "
       "want to have fonts available in GIMP only, otherwise put them "
       "in your global font directory."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "gradients",
    N_("This folder is used to store user defined gradients.  "
       "GIMP checks this folder in addition to the system-wide "
       "gradients installation."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "palettes",
    N_("This folder is used to store user defined palettes.  "
       "GIMP checks this folder in addition to the system-wide "
       "palettes installation."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "patterns",
    N_("This folder is used to store user defined patterns.  "
       "GIMP checks this folder in addition to the system-wide "
       "patterns installation when searching for patterns."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "plug-ins",
    N_("This folder is used to store user created, temporary, "
       "or otherwise non-system-supported plug-ins.  GIMP "
       "checks this folder in addition to the system-wide "
       "plug-in folder."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "modules",
    N_("This folder is used to store user created, temporary, "
       "or otherwise non-system-supported DLL modules.  GIMP "
       "checks this folder in addition to the system-wide "
       "module folder."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "interpreters",
    N_("This folder is used to store configuration for user "
       "created, temporary, or otherwise non-system-supported "
       "plug-in interpreters.  GIMP checks this folder in "
       "addition to the system-wide GIMP interpreters folder "
       "when searching for plug-in interpreter configuration "
       "files."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "environ",
    N_("This folder is used to store user created, temporary, "
       "or otherwise non-system-supported additions to the "
       "plug-in environment.  GIMP checks this folder in "
       "addition to the system-wide GIMP environment folder "
       "when searching for plug-in environment modification "
       "files."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "scripts",
    N_("This folder is used to store user created and installed "
       "scripts.  GIMP checks this folder in addition to "
       "the systemwide scripts folder."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "templates",
    N_("This folder is searched for image templates."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "themes",
    N_("This folder is searched for user-installed themes."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "tmp",
    N_("This folder is used for temporary files."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "tool-options",
    N_("This folder is used to store tool options."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "curves",
    N_("This folder is used to store parameter files for the Curves tool."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "levels",
    N_("This folder is used to store parameter files for the Levels tool."),
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "fractalexplorer",
    NULL,
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "gfig",
    NULL,
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "gflare",
    NULL,
    TREE_ITEM_MKDIR
  },
  {
    TRUE, "gimpressionist",
    NULL,
    TREE_ITEM_MKDIR
  }
};


static void
user_install_notebook_set_page (GtkNotebook *notebook,
                                gint         index)
{
  GtkWidget *page;
  gchar     *title;
  gchar     *footer;

  page = gtk_notebook_get_nth_page (notebook, index);

  title  = g_object_get_data (G_OBJECT (page), "title");
  footer = g_object_get_data (G_OBJECT (page), "footer");

  gtk_label_set_text (GTK_LABEL (title_label), title);
  gtk_label_set_text (GTK_LABEL (footer_label), footer);

  gtk_notebook_set_current_page (notebook, index);
}

static void
user_install_response (GtkWidget *widget,
                       gint       response_id,
                       GimpRc    *gimprc)
{
  static gint index = GPL_PAGE;

  if (response_id != GTK_RESPONSE_OK)
    {
      exit (EXIT_SUCCESS);
    }

  switch (index)
    {
    case GPL_PAGE:
      if (oldgimp)
        index = MIGRATION_PAGE;
      else
        index = TREE_PAGE;

      user_install_notebook_set_page (GTK_NOTEBOOK (notebook), index);
      break;

    case MIGRATION_PAGE:
      if (migrate)
        {
          index = TREE_PAGE;
          /* fallthrough */
        }
      else
        {
          user_install_notebook_set_page (GTK_NOTEBOOK (notebook), ++index);
          break;
        }

    case TREE_PAGE:
      user_install_notebook_set_page (GTK_NOTEBOOK (notebook), ++index);

      /*  Creating the directories can take some time on NFS, so inform
       *  the user and set the buttons insensitive
       */
      gtk_dialog_set_response_sensitive (GTK_DIALOG (widget),
                                         GTK_RESPONSE_CANCEL, FALSE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (widget),
                                         GTK_RESPONSE_OK, TRUE);

      if (user_install_run (migrate ? oldgimp : NULL))
        {
          gtk_dialog_set_response_sensitive (GTK_DIALOG (widget),
                                             GTK_RESPONSE_OK, TRUE);
          gtk_label_set_text (GTK_LABEL (footer_label),
                              _("Installation successful.  "
                                "Click \"Continue\" to proceed."));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (footer_label),
                              _("Installation failed.  "
                                "Contact system administrator."));

          index = TUNING_PAGE; /* skip to last page */
        }

      gtk_dialog_set_response_sensitive (GTK_DIALOG (widget),
                                         GTK_RESPONSE_CANCEL, TRUE);
      break;

    case LOG_PAGE:
      if (! migrate)
        {
          user_install_notebook_set_page (GTK_NOTEBOOK (notebook), ++index);
          user_install_tuning (gimprc);
          break;
        }
      /* else fallthrough */

    case TUNING_PAGE:
      if (! migrate)
        gimp_rc_save (gimprc);

      g_object_unref (title_style);
      g_object_unref (page_style);

      gtk_widget_destroy (user_install_dialog);

      gtk_main_quit ();
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static gboolean
user_install_corner_expose (GtkWidget      *widget,
                            GdkEventExpose *eevent,
                            gpointer        data)
{
  GtkCornerType corner;

  /* call default handler explicitly, then draw the corners */
  if (GTK_WIDGET_GET_CLASS (widget)->expose_event)
    GTK_WIDGET_GET_CLASS (widget)->expose_event (widget, eevent);

  corner = GPOINTER_TO_INT (data);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      switch (corner)
        {
        case GTK_CORNER_TOP_LEFT:     corner = GTK_CORNER_TOP_RIGHT;    break;
        case GTK_CORNER_BOTTOM_LEFT:  corner = GTK_CORNER_BOTTOM_RIGHT; break;
        case GTK_CORNER_TOP_RIGHT:    corner = GTK_CORNER_TOP_LEFT;     break;
        case GTK_CORNER_BOTTOM_RIGHT: corner = GTK_CORNER_BOTTOM_LEFT;  break;
        }
    }

  switch (corner)
    {
    case GTK_CORNER_TOP_LEFT:
      gdk_draw_arc (widget->window,
                    widget->style->white_gc,
                    TRUE,
                    0, 0,
                    widget->allocation.width * 2,
                    widget->allocation.height * 2,
                    90 * 64,
                    180 * 64);
      break;

    case GTK_CORNER_BOTTOM_LEFT:
      gdk_draw_arc (widget->window,
                    widget->style->white_gc,
                    TRUE,
                    0, -widget->allocation.height,
                    widget->allocation.width * 2,
                    widget->allocation.height * 2,
                    180 * 64,
                    270 * 64);
      break;

    case GTK_CORNER_TOP_RIGHT:
      gdk_draw_arc (widget->window,
                    widget->style->white_gc,
                    TRUE,
                    -widget->allocation.width, 0,
                    widget->allocation.width * 2,
                    widget->allocation.height * 2,
                    0 * 64,
                    90 * 64);
      break;

    case GTK_CORNER_BOTTOM_RIGHT:
      gdk_draw_arc (widget->window,
                    widget->style->white_gc,
                    TRUE,
                    -widget->allocation.width, -widget->allocation.height,
                    widget->allocation.width * 2,
                    widget->allocation.height * 2,
                    270 * 64,
                    360 * 64);
      break;

    default:
      break;
    }

  return TRUE;
}

static GtkWidget *
user_install_notebook_append_page (GtkNotebook *notebook,
                                   gchar       *title,
                                   gchar       *footer,
                                   gint         vbox_spacing)
{
  GtkWidget *page;

  page = gtk_vbox_new (FALSE, vbox_spacing);
  g_object_set_data (G_OBJECT (page), "title", title);
  g_object_set_data (G_OBJECT (page), "footer", footer);
  gtk_notebook_append_page (notebook, page, NULL);
  gtk_widget_show (page);

  return page;
}

static void
add_label (GtkBox   *box,
           gchar    *text)
{
  GtkWidget *label;

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), text);
  PAGE_STYLE (label);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
  gtk_box_pack_start (box, label, FALSE, FALSE, 0);

  gtk_widget_show (label);
}

static void
user_install_sel_changed (GtkTreeSelection *sel,
                          gpointer          data)
{
  GtkNotebook  *notebook = GTK_NOTEBOOK (data);
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint          index = 0;

  if (gtk_tree_selection_get_selected (sel, &model, &iter))
    gtk_tree_model_get (model, &iter, DESC_COLUMN, &index, -1);

  gtk_notebook_set_current_page (notebook, index);
}

static void
user_install_tv_fix_size_request (GtkWidget     *widget,
                                  GtkAllocation *allocation)
{
  gtk_widget_set_size_request (widget, allocation->width, allocation->height);
  g_signal_handlers_disconnect_by_func (widget,
                                        user_install_tv_fix_size_request,
                                        NULL);
}

void
user_install_dialog_run (const gchar *alternate_system_gimprc,
                         const gchar *alternate_gimprc,
                         gboolean     verbose)
{
  GimpRc    *gimprc;
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *ebox;
  GtkWidget *table;
  GtkWidget *darea;
  GtkWidget *page;
  GtkWidget *sep;
  GtkWidget *eek_box;
  GdkPixbuf *wilber;
  gchar     *filename;
  gchar     *version;
  gchar     *title;
  gint       i;

  oldgimp = g_strdup (gimp_directory ());

  /*  FIXME  */
  version = strstr (oldgimp, "2.3");

  if (version)
    {
      version[2]    = '2';
      oldgimp_major = 2;
      oldgimp_minor = 2;
    }
  migrate = (version && g_file_test (oldgimp, G_FILE_TEST_IS_DIR));

  if (! migrate)
    {
      if (version)
        {
          version[2]    = '0';
          oldgimp_major = 2;
          oldgimp_minor = 0;
        }
      migrate = (version && g_file_test (oldgimp, G_FILE_TEST_IS_DIR));
    }

  if (! migrate)
    {
      g_free (oldgimp);
      oldgimp = NULL;
      oldgimp_major = 0;
      oldgimp_minor = 0;
    }

  gimprc = gimp_rc_new (alternate_system_gimprc, alternate_gimprc, verbose);

  dialog = user_install_dialog =
    gimp_dialog_new (_("GIMP User Installation"), "gimp-user-installation",
                     NULL, 0,
                     NULL, NULL,

                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                     _("Continue"),    GTK_RESPONSE_OK,

                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (user_install_response),
                    gimprc);

  g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, gimprc);
  eek_box = gtk_hbox_new (FALSE, 8);

  g_object_ref (GTK_DIALOG (dialog)->action_area);
  gtk_container_remove (GTK_CONTAINER (GTK_DIALOG (dialog)->action_area->parent),
                        GTK_DIALOG (dialog)->action_area);
  gtk_box_pack_end (GTK_BOX (eek_box), GTK_DIALOG (dialog)->action_area,
                    FALSE, FALSE, 0);
  g_object_unref (GTK_DIALOG (dialog)->action_area);

  gtk_box_pack_end (GTK_BOX (GTK_DIALOG (dialog)->vbox), eek_box,
                    FALSE, FALSE, 0);
  gtk_widget_show (eek_box);

  gdk_color_parse ("black",       &black_color);
  gdk_color_parse ("white",       &white_color);
  gdk_color_parse ("dark orange", &title_color);

  gtk_widget_realize (dialog);

  /*  B/W Style for the page contents  */
  page_style = gtk_widget_get_modifier_style (dialog);
  g_object_ref (page_style);

  for (i = 0; i < 5; i++)
    {
      page_style->fg[i] = black_color;
      page_style->bg[i] = white_color;

      page_style->color_flags[i] = (GTK_RC_FG | GTK_RC_BG);
    }

  page_style->text[GTK_STATE_NORMAL] = black_color;
  page_style->base[GTK_STATE_NORMAL] = white_color;

  page_style->color_flags[GTK_STATE_NORMAL] |= (GTK_RC_TEXT | GTK_RC_BASE);

  g_free (page_style->bg_pixmap_name[GTK_STATE_NORMAL]);
  page_style->bg_pixmap_name[GTK_STATE_NORMAL] = g_strdup ("<none>");

  /*  B/Colored Style for the page title  */
  title_style = gtk_rc_style_copy (page_style);

  title_style->bg[GTK_STATE_NORMAL] = title_color;

  pango_font_description_free (title_style->font_desc);
  title_style->font_desc = pango_font_description_from_string ("sans 20");

  TITLE_STYLE (dialog);

  footer_label = gtk_label_new (NULL);
  PAGE_STYLE (footer_label);
  gtk_label_set_justify (GTK_LABEL (footer_label), GTK_JUSTIFY_RIGHT);
  gtk_label_set_line_wrap (GTK_LABEL (footer_label), TRUE);
  gtk_box_pack_end (GTK_BOX (eek_box), footer_label, FALSE, FALSE,0 );
  gtk_widget_show (footer_label);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);

  ebox = gtk_event_box_new ();
  TITLE_STYLE (ebox);
  gtk_widget_set_events (ebox, GDK_EXPOSURE_MASK);
  gtk_box_pack_start (GTK_BOX (vbox), ebox, FALSE, FALSE, 0);
  gtk_widget_show (ebox);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_container_add (GTK_CONTAINER (ebox), hbox);
  gtk_widget_show (hbox);

  filename = g_build_filename (gimp_data_directory(),
                               "images", "wilber-wizard.png", NULL);
  wilber = gdk_pixbuf_new_from_file (filename, NULL);
  g_free (filename);

  if (wilber)
    {
      title_image = gtk_image_new_from_pixbuf (wilber);
      g_object_unref (wilber);

      gtk_box_pack_start (GTK_BOX (hbox), title_image, FALSE, FALSE, 8);
      gtk_widget_show (title_image);
    }

  title_label = gtk_label_new (NULL);
  TITLE_STYLE (title_label);
  gtk_label_set_justify (GTK_LABEL (title_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (title_label), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), title_label, FALSE, FALSE, 0);
  gtk_widget_show (title_label);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  ebox = gtk_event_box_new ();
  TITLE_STYLE (ebox);
  gtk_widget_set_size_request (ebox, 16, -1);
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
  gtk_widget_set_size_request (darea, 16, 16);
  g_signal_connect (darea, "expose-event",
                    G_CALLBACK (user_install_corner_expose),
                    GINT_TO_POINTER (GTK_CORNER_TOP_LEFT));
  gtk_table_attach (GTK_TABLE (table), darea, 0, 1, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (darea);

  darea = gtk_drawing_area_new ();
  TITLE_STYLE (darea);
  gtk_widget_set_size_request (darea, 16, 16);
  g_signal_connect (darea, "expose-event",
                    G_CALLBACK (user_install_corner_expose),
                    GINT_TO_POINTER (GTK_CORNER_BOTTOM_LEFT));
  gtk_table_attach (GTK_TABLE (table), darea, 0, 1, 2, 3,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (darea);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_table_attach_defaults (GTK_TABLE (table), notebook, 1, 2, 1, 2);
  gtk_widget_show (notebook);

  gtk_widget_show (vbox);

  /*  GPL_PAGE  */

  /*  version number  */
  title = g_strdup_printf (_("Welcome to\n"
                             "The GIMP %d.%d User Installation"),
                           GIMP_MAJOR_VERSION, GIMP_MINOR_VERSION);

  page = user_install_notebook_append_page (GTK_NOTEBOOK (notebook),
                                            title,
                                            _("Click \"Continue\" to enter "
                                              "the GIMP user installation."),
                                            12);

  /*  do not free title yet!  */

  add_label (GTK_BOX (page),
             _("<b>The GIMP - GNU Image Manipulation Program</b>\n"
               "Copyright (C) 1995-2005\n"
               "Spencer Kimball, Peter Mattis and the GIMP Development Team."));

  sep = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (page), sep, FALSE, FALSE, 2);
  gtk_widget_show (sep);

  add_label
    (GTK_BOX (page),
     _("This program is free software; you can redistribute it and/or modify "
       "it under the terms of the GNU General Public License as published by "
       "the Free Software Foundation; either version 2 of the License, or "
       "(at your option) any later version."));
  add_label
    (GTK_BOX (page),
     _("This program is distributed in the hope that it will be useful, "
       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
       "See the GNU General Public License for more details."));
  add_label
    (GTK_BOX (page),
     _("You should have received a copy of the GNU General Public License "
       "along with this program; if not, write to the Free Software "
       "Foundation, Inc., 59 Temple Place - Suite 330, Boston, "
       "MA 02111-1307, USA."));

  /*  MIGRATION_PAGE  */
  if (version && migrate)
  {
    GtkWidget *box;
    gchar     *title;
    gchar     *label;

    page = user_install_notebook_append_page (GTK_NOTEBOOK (notebook),
                                              _("Migrate User Settings"),
                                              _("Click \"Continue\" to proceed "
                                                "with the user installation."),
                                              12);

    title = g_strdup_printf (_("It seems you have used GIMP %s before."),
                             version);
    label = g_strdup_printf (_("_Migrate GIMP %s user settings"), version);

    box = gimp_int_radio_group_new (TRUE, title,
                                    G_CALLBACK (gimp_radio_button_update),
                                    &migrate, migrate,

                                    label,
                                    TRUE,  NULL,

                                    _("Do a _fresh user installation"),
                                    FALSE, NULL,
                                    NULL);

    g_free (label);
    g_free (title);

    gtk_box_pack_start (GTK_BOX (page), box, FALSE, FALSE, 0);
    gtk_widget_show (box);
  }

  /*  TREE_PAGE  */
  {
    GtkWidget         *hbox;
    GtkWidget         *vbox;
    GtkWidget         *scr;
    GtkWidget         *tv;
    GdkPixbuf         *file_pixbuf;
    GdkPixbuf         *folder_pixbuf;
    GtkWidget         *notebook2;
    GtkWidget         *page2;
    GtkWidget         *label;
    gchar             *str;
    GtkTreeStore      *tree;
    GtkTreeViewColumn *column;
    GtkCellRenderer   *cell;
    GtkTreeSelection  *sel;
    GtkTreeIter        iter, child;
    gint               i;

    page = user_install_notebook_append_page (GTK_NOTEBOOK (notebook),
                                              _("Personal GIMP Folder"),
                                              _("Click \"Continue\" to create "
                                                "your personal GIMP folder."),
                                              0);

    hbox = gtk_hbox_new (FALSE, 12);
    gtk_box_pack_start (GTK_BOX (page), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    tree = gtk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, GDK_TYPE_PIXBUF,
                                            G_TYPE_INT);
    tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (tree));
    g_object_unref (tree);

    column = gtk_tree_view_column_new ();

    cell = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, cell, FALSE);
    gtk_tree_view_column_set_attributes (column, cell,
                                         "pixbuf", PIXBUF_COLUMN,
                                         NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, cell, TRUE);
    gtk_tree_view_column_set_attributes (column, cell,
                                         "text", DIRENT_COLUMN,
                                         NULL);

    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), column);

    PAGE_STYLE (tv);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
    scr = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scr),
                                    GTK_POLICY_NEVER,
                                    GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (hbox), scr, FALSE, FALSE, 0);
    gtk_widget_show (scr);
    gtk_container_add (GTK_CONTAINER (scr), tv);
    gtk_widget_show (tv);

    vbox = gtk_vbox_new (FALSE, 6);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
    gtk_widget_show (vbox);

    str = g_strdup_printf (_("For a proper GIMP installation, a folder named "
                             "'<b>%s</b>' needs to be created."),
                           gimp_filename_to_utf8 (gimp_directory ()));
    add_label (GTK_BOX (vbox), str);
    g_free (str);

    add_label (GTK_BOX (vbox),
               _("This folder will contain a number of important files.  "
                 "Click on one of the files or folders in the tree "
                 "to get more information about the selected item."));

    notebook2 = gtk_notebook_new ();
    gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook2), FALSE);
    gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook2), FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), notebook2, FALSE, FALSE, 32);
    gtk_widget_show (notebook2);

    /*  empty page  */
    page2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (page2);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook2), page2, NULL);

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
    g_signal_connect (sel, "changed",
                      G_CALLBACK (user_install_sel_changed),
                      notebook2);

    file_pixbuf = gtk_widget_render_icon (tv,
                                          GTK_STOCK_NEW, GTK_ICON_SIZE_MENU,
                                          NULL);
    folder_pixbuf = gtk_widget_render_icon (tv,
                                            GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU,
                                            NULL);

    gtk_tree_store_append (tree, &iter, NULL);
    gtk_tree_store_set (tree, &iter,
                        DIRENT_COLUMN, gimp_filename_to_utf8 (gimp_directory ()),
                        PIXBUF_COLUMN, folder_pixbuf,
                        DESC_COLUMN, 0,
                        -1);

    for (i = 0; i < G_N_ELEMENTS (tree_items); i++)
      {
        gchar *foo;

        if (!tree_items[i].description)
          continue;

        gtk_tree_store_append (tree, &child, &iter);
        gtk_tree_store_set (tree, &child,
                            DIRENT_COLUMN, tree_items[i].name,
                            PIXBUF_COLUMN,
                            tree_items[i].directory ? folder_pixbuf
                                                    : file_pixbuf,
                            DESC_COLUMN, i + 1,
                            -1);

        page2 = gtk_vbox_new (FALSE, 6);

        foo = g_strdup_printf ("<b>%s</b>", tree_items[i].name);
        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), foo);
        g_free (foo);
        PAGE_STYLE (label);
        gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_box_pack_start (GTK_BOX (page2), label, FALSE, FALSE, 0);
        gtk_widget_show (label);

        label = gtk_label_new (gettext (tree_items[i].description));
        PAGE_STYLE (label);
        gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
        gtk_box_pack_start (GTK_BOX (page2), label, FALSE, FALSE, 0);
        gtk_widget_show (label);
        gtk_widget_show (page2);

        gtk_notebook_append_page (GTK_NOTEBOOK (notebook2), page2, NULL);
      }

    gtk_tree_view_expand_all (GTK_TREE_VIEW (tv));

    g_signal_connect (tv, "size-allocate",
                      G_CALLBACK (user_install_tv_fix_size_request),
                      NULL);

    g_object_unref (file_pixbuf);
    g_object_unref (folder_pixbuf);
  }

  /*  LOG_PAGE  */
  page = log_page =
    user_install_notebook_append_page (GTK_NOTEBOOK (notebook),
                                       _("User Installation Log"),
                                       _("Please wait while your personal "
                                         "GIMP folder is being created..."),
                                       0);

  /*  TUNING_PAGE  */
  page = tuning_page =
    user_install_notebook_append_page (GTK_NOTEBOOK (notebook),
                                       _("GIMP Performance Tuning"),
                                       _("Click \"Continue\" to accept the "
                                         "settings above."),
                                       24);

  add_label (GTK_BOX (page),
             _("<b>For optimal GIMP performance, some settings may have to "
               "be adjusted.</b>"));

  user_install_notebook_set_page (GTK_NOTEBOOK (notebook), 0);

  gtk_widget_show (dialog);

  gtk_main ();

  g_free (title);
  g_free (oldgimp);
}


/*********************/
/*  Local functions  */

static void
print_log (GtkWidget     *view,
           GtkTextBuffer *buffer,
           GError        *error)
{
  GtkTextIter  cursor;
  GdkPixbuf   *pixbuf;

  gtk_text_buffer_insert_at_cursor (buffer, error ? "\n" : " ", -1);

  gtk_text_buffer_get_end_iter (buffer, &cursor);

  pixbuf =
    gtk_widget_render_icon (view,
                            error ? GIMP_STOCK_ERROR     : GTK_STOCK_APPLY,
                            error ? GTK_ICON_SIZE_DIALOG : GTK_ICON_SIZE_MENU,
                            NULL);
  gtk_text_buffer_insert_pixbuf (buffer, &cursor, pixbuf);
  g_object_unref (pixbuf);

  if (error)
    {
      gtk_text_buffer_insert (buffer, &cursor, "\n", -1);
      gtk_text_buffer_insert_with_tags_by_name (buffer, &cursor,
                                                error->message, -1,
                                                "bold",
                                                NULL);
    }

  gtk_text_buffer_insert (buffer, &cursor, "\n", -1);

  while (gtk_events_pending ())
    gtk_main_iteration ();
}

static gboolean
user_install_file_copy (GtkTextBuffer  *log_buffer,
                        const gchar    *source,
                        const gchar    *dest,
                        GError        **error)
{
  gchar *msg;

  msg = g_strdup_printf (_("Copying file '%s' from '%s'..."),
                         gimp_filename_to_utf8 (dest),
                         gimp_filename_to_utf8 (source));
  gtk_text_buffer_insert_at_cursor (log_buffer, msg, -1);
  g_free (msg);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  return gimp_config_file_copy (source, dest, error);
}

static gboolean
user_install_mkdir (GtkTextBuffer  *log_buffer,
                    const gchar    *dirname,
                    GError        **error)
{
  gchar *msg;

  msg = g_strdup_printf (_("Creating folder '%s'..."),
                         gimp_filename_to_utf8 (dirname));
  gtk_text_buffer_insert_at_cursor (log_buffer, msg, -1);
  g_free (msg);

  while (gtk_events_pending ())
    gtk_main_iteration ();

  if (g_mkdir (dirname,
               S_IRUSR | S_IWUSR | S_IXUSR |
               S_IRGRP | S_IXGRP |
               S_IROTH | S_IXOTH) == -1)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Cannot create folder '%s': %s"),
                   gimp_filename_to_utf8 (dirname), g_strerror (errno));
      return FALSE;
    }

  return TRUE;
}

static gboolean
user_install_dir_copy (GtkWidget      *log_view,
                       GtkTextBuffer  *log_buffer,
                       const gchar    *source,
                       const gchar    *base,
                       GError        **error)
{
  GDir  *source_dir = NULL;
  GDir  *dest_dir   = NULL;
  gchar  dest[1024];
  gchar *basename;
  gchar *dirname;

  basename = g_path_get_basename (source);
  dirname = g_build_filename (base, basename, NULL);
  g_free (basename);

  if (! user_install_mkdir (log_buffer, dirname, error))
    {
      g_free (dirname);
      return FALSE;
    }

  print_log (log_view, log_buffer, NULL);

  dest_dir = g_dir_open (dirname, 0, error);
  if (dest_dir)
    {
      source_dir = g_dir_open (source, 0, error);
      if (source_dir)
        {
          const gchar *basename;
          gchar       *name;

          while ((basename = g_dir_read_name (source_dir)) != NULL)
            {
              name = g_build_filename (source, basename, NULL);

              if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
                {
                  g_snprintf (dest, sizeof (dest), "%s%c%s",
                              dirname, G_DIR_SEPARATOR, basename);

                  if (! user_install_file_copy (log_buffer, name, dest, error))
                    {
                      g_free (name);
                      goto break_out_of_loop;
                    }

                  print_log (log_view, log_buffer, NULL);
                }

              g_free (name);
            }
        }
    }

 break_out_of_loop:
  g_free (dirname);

  if (source_dir)
    g_dir_close (source_dir);
  if (dest_dir)
    g_dir_close (dest_dir);

  return (*error == NULL);
}

static gboolean
user_install_create_files (GtkWidget     *log_view,
                           GtkTextBuffer *log_buffer)
{
  gchar   dest[1024];
  gchar   source[1024];
  gint    i;
  GError *error = NULL;

  for (i = 0; i < G_N_ELEMENTS (tree_items); i++)
    {
      g_snprintf (dest, sizeof (dest), "%s%c%s",
                  gimp_directory (), G_DIR_SEPARATOR, tree_items[i].name);

      switch (tree_items[i].type)
        {
        case TREE_ITEM_DO_NOTHING:
          break;

        case TREE_ITEM_MKDIR:
          if (! user_install_mkdir (log_buffer, dest, &error))
              goto break_out_of_loop;
          break;

        case TREE_ITEM_FROM_SYSCONF_DIR:
          g_snprintf (source, sizeof (source), "%s%c%s",
                      gimp_sysconf_directory (), G_DIR_SEPARATOR,
                      tree_items[i].name);

          g_assert (! tree_items[i].directory);

          if (! user_install_file_copy (log_buffer, source, dest, &error))
            goto break_out_of_loop;
          break;

        default:
          g_assert_not_reached ();
          break;
        }

      if (tree_items[i].type != TREE_ITEM_DO_NOTHING)
        print_log (log_view, log_buffer, NULL);
    }

 break_out_of_loop:
  if (error)
    {
      print_log (log_view, log_buffer, error);
      g_clear_error (&error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
user_install_migrate_files (const gchar   *oldgimp,
                            gint           oldgimp_major,
                            gint           oldgimp_minor,
                            GtkWidget     *log_view,
                            GtkTextBuffer *log_buffer)
{
  GDir   *dir;
  GError *error  = NULL;

  dir = g_dir_open (oldgimp, 0, &error);
  if (dir)
    {
      const gchar *basename;
      gchar       *source = NULL;
      gchar        dest[1024];

      while ((basename = g_dir_read_name (dir)) != NULL)
        {
          source = g_build_filename (oldgimp, basename, NULL);

          if (g_file_test (source, G_FILE_TEST_IS_REGULAR))
            {
              /*  skip these for all old versions  */
              if ((strncmp (basename, "gimpswap.", 9) == 0) ||
                  (strncmp (basename, "pluginrc",  8) == 0) ||
                  (strncmp (basename, "themerc",   7) == 0))
                {
                  goto next_file;
                }

              /*  skip menurc for gimp 2.0 since the format has changed  */
              if (oldgimp_minor == 0 &&
                  (strncmp (basename, "menurc", 6) == 0))
                {
                  goto next_file;
                }

              g_snprintf (dest, sizeof (dest), "%s%c%s",
                          gimp_directory (), G_DIR_SEPARATOR, basename);

              user_install_file_copy (log_buffer, source, dest, &error);

              print_log (log_view, log_buffer, error);
              g_clear_error (&error);
            }
          else if (g_file_test (source, G_FILE_TEST_IS_DIR) &&
                   strcmp (basename, "tmp") != 0)
            {
              if (! user_install_dir_copy (log_view, log_buffer,
                                           source, gimp_directory (), &error))
                {
                  print_log (log_view, log_buffer, error);
                  g_clear_error (&error);
                }
            }

            next_file:

          g_free (source);
          source = NULL;
        }

      /*  create the tmp directory that was explicitely not copied  */

      g_snprintf (dest, sizeof (dest), "%s%c%s",
                  gimp_directory (), G_DIR_SEPARATOR, "tmp");

      if (user_install_mkdir (log_buffer, dest, &error))
        print_log (log_view, log_buffer, NULL);

      g_dir_close (dir);
    }

  if (error)
    {
      print_log (log_view, log_buffer, error);
      g_clear_error (&error);

      return FALSE;
    }

  gimp_templates_migrate (oldgimp);

  return TRUE;
}

static gboolean
user_install_run (const gchar *oldgimp)
{
  GtkWidget     *scrolled_window;
  GtkTextBuffer *log_buffer;
  GtkWidget     *log_view;
  GError        *error = NULL;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (log_page), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  log_buffer = gtk_text_buffer_new (NULL);

  gtk_text_buffer_create_tag (log_buffer, "bold",
                              "weight", PANGO_WEIGHT_BOLD,
                              NULL);

  log_view = gtk_text_view_new_with_buffer (log_buffer);
  g_object_unref (log_buffer);

  PAGE_STYLE (log_view);
  gtk_text_view_set_editable (GTK_TEXT_VIEW (log_view), FALSE);

  gtk_container_add (GTK_CONTAINER (scrolled_window), log_view);
  gtk_widget_show (log_view);

  if (! user_install_mkdir (log_buffer, gimp_directory (), &error))
    {
      print_log (log_view, log_buffer, error);
      g_clear_error (&error);

      return FALSE;
    }

  print_log (log_view, log_buffer, NULL);

  if (oldgimp)
    return user_install_migrate_files (oldgimp, oldgimp_major, oldgimp_minor,
                                       log_view, log_buffer);
  else
    return user_install_create_files (log_view, log_buffer);
}

static void
user_install_tuning (GimpRc *gimprc)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;

  /*  tile cache size  */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (tuning_page), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  add_label (GTK_BOX (vbox),
             _("GIMP uses a limited amount of memory to store image data, "
               "the so-called \"Tile Cache\".  You should adjust its size "
               "to fit into memory.  Consider the amount of memory used by "
               "other running processes."));

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_prop_memsize_entry_new (G_OBJECT (gimprc), "tile-cache-size");
  gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  label = gtk_label_new (_("Tile cache size:"));
  PAGE_STYLE (label);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);


  /*  swap file location  */
  vbox = gtk_vbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (tuning_page), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  add_label (GTK_BOX (vbox),
             _("All image and undo data which doesn't fit into the Tile Cache "
               "will be written to a swap file.  This file should be located "
               "on a local filesystem with enough free space (several hundred "
               "MB). On a UNIX system, you may want to use the system-wide "
               "temp-dir (\"/tmp\" or \"/var/tmp\")."));

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  entry = gimp_prop_file_entry_new (G_OBJECT (gimprc), "swap-path",
                                    _("Select Swap Dir"),
                                    TRUE, TRUE);
  gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  label = gtk_label_new (_("Swap folder:"));
  PAGE_STYLE (label);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
}
