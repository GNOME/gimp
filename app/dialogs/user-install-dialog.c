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

#include "user-install-dialog.h"

#include "gimp-intl.h"


enum
{
  WELCOME_PAGE,
  INSTALLATION_PAGE,
  NUM_PAGES
};

enum
{
  DIRENT_COLUMN,
  PIXBUF_COLUMN,
  DESC_COLUMN,
  NUM_COLUMNS
};


static void      user_install_response (GtkWidget   *dialog,
                                        gint         response_id,
                                        GimpRc      *gimprc);

static gboolean  user_install_run      (GtkWidget   *page,
                                        const gchar *oldgimp);


/*  private stuff  */

static GtkWidget  *notebook      = NULL;

static GtkWidget  *title_label   = NULL;
static GtkWidget  *footer_label  = NULL;

static gchar      *oldgimp       = NULL;
static gint        oldgimp_major = 0;
static gint        oldgimp_minor = 0;
static gboolean    migrate       = FALSE;


typedef enum
{
  USER_INSTALL_DO_NOTHING,        /* Don't pre-create            */
  USER_INSTALL_MKDIR,             /* Create the directory        */
  USER_INSTALL_FROM_SYSCONF_DIR   /* Copy from sysconf directory */
} UserInstallAction;

static const struct
{
  const gchar       *name;
  UserInstallAction  action;
}
user_install_items[] =
{
  { "gimprc",          USER_INSTALL_DO_NOTHING       },
  { "gtkrc",           USER_INSTALL_FROM_SYSCONF_DIR },
  { "pluginrc",        USER_INSTALL_DO_NOTHING       },
  { "menurc",          USER_INSTALL_DO_NOTHING       },
  { "sessionrc",       USER_INSTALL_DO_NOTHING       },
  { "templaterc",      USER_INSTALL_DO_NOTHING       },
  { "unitrc",          USER_INSTALL_DO_NOTHING       },
  { "brushes",         USER_INSTALL_MKDIR            },
  { "fonts",           USER_INSTALL_MKDIR            },
  { "gradients",       USER_INSTALL_MKDIR            },
  { "palettes",        USER_INSTALL_MKDIR            },
  { "patterns",        USER_INSTALL_MKDIR            },
  { "plug-ins",        USER_INSTALL_MKDIR            },
  { "modules",         USER_INSTALL_MKDIR            },
  { "interpreters",    USER_INSTALL_MKDIR            },
  { "environ",         USER_INSTALL_MKDIR            },
  { "scripts",         USER_INSTALL_MKDIR            },
  { "templates",       USER_INSTALL_MKDIR            },
  { "themes",          USER_INSTALL_MKDIR            },
  { "tmp",             USER_INSTALL_MKDIR            },
  { "tool-options",    USER_INSTALL_MKDIR            },
  { "curves",          USER_INSTALL_MKDIR            },
  { "levels",          USER_INSTALL_MKDIR            },
  { "fractalexplorer", USER_INSTALL_MKDIR            },
  { "gfig",            USER_INSTALL_MKDIR            },
  { "gflare",          USER_INSTALL_MKDIR            },
  { "gimpressionist",  USER_INSTALL_MKDIR            }
};


static GtkWidget *
user_install_notebook_set_page (GtkNotebook *notebook,
                                gint         index)
{
  GtkWidget   *page;
  const gchar *title;
  const gchar *footer;

  page = gtk_notebook_get_nth_page (notebook, index);

  title  = g_object_get_data (G_OBJECT (page), "title");
  footer = g_object_get_data (G_OBJECT (page), "footer");

  gtk_label_set_text (GTK_LABEL (title_label), title);
  gtk_label_set_text (GTK_LABEL (footer_label), footer);

  gtk_notebook_set_current_page (notebook, index);

  return page;
}

static void
user_install_response (GtkWidget *dialog,
                       gint       response_id,
                       GimpRc    *gimprc)
{
  gint index = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

  if (response_id != GTK_RESPONSE_OK)
    exit (EXIT_SUCCESS);

  switch (index)
    {
    case WELCOME_PAGE:
      {
        GtkWidget *page;

        page = user_install_notebook_set_page (GTK_NOTEBOOK (notebook),
                                               ++index);

        /*  Creating the directories can take some time on NFS, so inform
         *  the user and set the buttons insensitive
         */
        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CANCEL, FALSE);
        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK, TRUE);

        if (user_install_run (page, migrate ? oldgimp : NULL))
          {
            gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK, TRUE);

            gtk_label_set_text (GTK_LABEL (footer_label),
                                _("Installation successful.  "
                                  "Click \"Continue\" to proceed."));
          }
        else
          {
            gtk_label_set_text (GTK_LABEL (footer_label),
                                _("Installation failed!  "
                                  "Contact system administrator."));
          }

        gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CANCEL, TRUE);
      }
      break;

    case INSTALLATION_PAGE:
      if (! migrate)
        gimp_rc_save (gimprc);

      gtk_widget_destroy (dialog);

      gtk_main_quit ();
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static GtkWidget *
user_install_notebook_append_page (GtkNotebook *notebook,
                                   const gchar *title,
                                   const gchar *footer,
                                   gint         vbox_spacing)
{
  GtkWidget *page = gtk_vbox_new (FALSE, vbox_spacing);

  g_object_set_data (G_OBJECT (page), "title",  (gpointer) title);
  g_object_set_data (G_OBJECT (page), "footer", (gpointer) footer);

  gtk_notebook_append_page (notebook, page, NULL);
  gtk_widget_show (page);

  return page;
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
  GtkWidget *page;
  GtkWidget *widget;
  GdkPixbuf *wilber;
  gchar     *filename;
  gchar     *version;

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

  dialog = gimp_dialog_new (_("GIMP User Installation"),
                            "gimp-user-installation",
                            NULL, 0,
                            NULL, NULL,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            _("Continue"),    GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimprc = gimp_rc_new (alternate_system_gimprc, alternate_gimprc, verbose);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (user_install_response),
                    gimprc);

  g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, gimprc);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show (vbox);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  filename = g_build_filename (gimp_data_directory(),
                               "images", "wilber-wizard.png", NULL);
  wilber = gdk_pixbuf_new_from_file (filename, NULL);
  g_free (filename);

  if (wilber)
    {
      GtkWidget *image = gtk_image_new_from_pixbuf (wilber);

      g_object_unref (wilber);

      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_widget_show (image);
    }

  title_label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (title_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap (GTK_LABEL (title_label), TRUE);
  gimp_label_set_attributes (GTK_LABEL (title_label),
                             PANGO_ATTR_SCALE, PANGO_SCALE_X_LARGE,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), title_label, FALSE, FALSE, 0);
  gtk_widget_show (title_label);

  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  footer_label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (footer_label), 1.0, 1.0);
  gimp_label_set_attributes (GTK_LABEL (footer_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_OBLIQUE,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), footer_label, FALSE, FALSE, 0);
  gtk_widget_show (footer_label);

  /*  WELCOME_PAGE  */
  page = user_install_notebook_append_page (GTK_NOTEBOOK (notebook),
                                            _("Welcome to the GNU Image "
                                              "Manipulation Program"),
                                            NULL,
                                            12);

  if (migrate)
    {
      gchar     *title;
      gchar     *label;

      title = g_strdup_printf (_("It seems you have used GIMP %s before."),
                               version);
      label = g_strdup_printf (_("_Migrate GIMP %s user settings"), version);

      widget = gimp_int_radio_group_new (TRUE, title,
                                         G_CALLBACK (gimp_radio_button_update),
                                         &migrate, migrate,

                                         label,
                                         TRUE,  NULL,

                                         _("Do a _fresh user installation"),
                                         FALSE, NULL,

                                         NULL);

      g_free (label);
      g_free (title);
    }
  else
    {
      gchar *text;

      text = g_strdup_printf (_("It appears that you are using GIMP for the "
                                "first time.  GIMP will now create a folder "
                                "named '<b>%s</b>' and copy some files to it."),
                                gimp_filename_to_utf8 (gimp_directory ()));

      widget = g_object_new (GTK_TYPE_LABEL,
                             "use-markup", TRUE,
                             "label",      text,
                             "wrap",       TRUE,
                             "xalign",     0.0,
                             NULL);
      g_free (text);
    }

  gtk_box_pack_start (GTK_BOX (page), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /*  INSTALLATION_PAGE  */
  page = user_install_notebook_append_page (GTK_NOTEBOOK (notebook),
                                            _("User Installation Log"),
                                            _("Please wait while your "
                                              "personal GIMP folder is "
                                              "being created..."),
                                            0);

  user_install_notebook_set_page (GTK_NOTEBOOK (notebook), WELCOME_PAGE);

  gtk_widget_show (dialog);

  gtk_main ();

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

  for (i = 0; i < G_N_ELEMENTS (user_install_items); i++)
    {
      g_snprintf (dest, sizeof (dest), "%s%c%s",
                  gimp_directory (),
                  G_DIR_SEPARATOR,
                  user_install_items[i].name);

      switch (user_install_items[i].action)
        {
        case USER_INSTALL_DO_NOTHING:
          break;

        case USER_INSTALL_MKDIR:
          if (! user_install_mkdir (log_buffer, dest, &error))
              goto break_out_of_loop;
          break;

        case USER_INSTALL_FROM_SYSCONF_DIR:
          g_snprintf (source, sizeof (source), "%s%c%s",
                      gimp_sysconf_directory (), G_DIR_SEPARATOR,
                      user_install_items[i].name);

          if (! user_install_file_copy (log_buffer, source, dest, &error))
            goto break_out_of_loop;
          break;

        default:
          g_assert_not_reached ();
          break;
        }

      if (user_install_items[i].action != USER_INSTALL_DO_NOTHING)
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
user_install_run (GtkWidget   *page,
                  const gchar *oldgimp)
{
  GtkWidget     *scrolled_window;
  GtkTextBuffer *log_buffer;
  GtkWidget     *log_view;
  GError        *error = NULL;

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (page), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  log_buffer = gtk_text_buffer_new (NULL);

  gtk_text_buffer_create_tag (log_buffer, "bold",
                              "weight", PANGO_WEIGHT_BOLD,
                              NULL);

  log_view = gtk_text_view_new_with_buffer (log_buffer);
  g_object_unref (log_buffer);

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
