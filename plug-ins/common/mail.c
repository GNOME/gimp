/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Daniel Risacher
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 *   GUMP - Gimp Useless Mail Plugin
 *          (or Gump Useless Mail Plugin if you prefer)
 *
 *   by Adrian Likins <adrian@gimp.org>
 *      MIME encapsulation by Reagan Blundell <reagan@emails.net>
 *
 * As always: The utility of this plugin is left as an exercise for
 * the reader
 *
 */

#include "config.h"

#include <string.h>

#ifdef SENDMAIL
#include <sys/types.h>
#include <sys/wait.h>
#endif

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define BUFFER_SIZE 256

#define PLUG_IN_PROC   "plug-in-mail-image"
#define PLUG_IN_BINARY "mail"
#define PLUG_IN_ROLE   "gimp-mail"


typedef struct
{
  gchar filename[BUFFER_SIZE];
  gchar receipt[BUFFER_SIZE];
  gchar from[BUFFER_SIZE];
  gchar subject[BUFFER_SIZE];
  gchar comment[BUFFER_SIZE];
} m_info;


typedef struct _Mail      Mail;
typedef struct _MailClass MailClass;

struct _Mail
{
  GimpPlugIn      parent_instance;
};

struct _MailClass
{
  GimpPlugInClass parent_class;
};


#define MAIL_TYPE  (mail_get_type ())
#define MAIL (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAIL_TYPE, Mail))

GType                   mail_get_type         (void) G_GNUC_CONST;

static GList          * mail_init_procedures  (GimpPlugIn           *plug_in);
static GimpProcedure  * mail_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * mail_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               GimpDrawable         *drawable,
                                               const GimpValueArray *args,
                                               gpointer              run_data);

static GimpPDBStatusType  send_image              (const gchar      *filename,
                                                   GimpImage        *image,
                                                   GimpDrawable     *drawable,
                                                   gint32            run_mode);

static gboolean           send_dialog             (void);
static void               mail_entry_callback     (GtkWidget        *widget,
                                                   gchar            *data);
static gboolean           valid_file              (GFile            *file);
static gchar            * find_extension          (const gchar      *filename);

#ifdef SENDMAIL
static void               mesg_body_callback      (GtkTextBuffer    *buffer,
                                                   gpointer          data);

static gchar            * sendmail_content_type   (const gchar      *filename);
static void               sendmail_create_headers (FILE             *mailpipe);
static gboolean           sendmail_to64           (const gchar      *filename,
                                                   FILE             *outfile,
                                                   GError          **error);
static FILE             * sendmail_pipe           (gchar           **cmd,
                                                   GPid             *pid);
#endif


G_DEFINE_TYPE (Mail, mail, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (MAIL_TYPE)


static m_info mail_info =
{
  "", "", "", "", ""
};

static gchar *mesg_body = NULL;


static void
mail_class_init (MailClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = mail_init_procedures;
  plug_in_class->create_procedure = mail_create_procedure;
}

static void
mail_init (Mail *mail)
{
}

static GList *
mail_init_procedures (GimpPlugIn *plug_in)
{
  GList *list = NULL;
  gchar *email_bin;

  /* Check if xdg-email or sendmail is installed.
   * TODO: allow setting the location of the executable in preferences.
   */
#ifdef SENDMAIL
  if (strlen (SENDMAIL) == 0)
    {
      email_bin = g_find_program_in_path ("sendmail");
    }
  else
    {
      /* If a directory has been set at build time, we assume that sendmail
       * can only be in this directory. */
      email_bin = g_build_filename (SENDMAIL, "sendmail", NULL);
      if (! g_file_test (email_bin, G_FILE_TEST_IS_EXECUTABLE))
        {
          g_free (email_bin);
          email_bin = NULL;
        }
    }
#else
  email_bin = g_find_program_in_path ("xdg-email");
#endif

  if (email_bin)
    list = g_list_append (list, g_strdup (PLUG_IN_PROC));

  return list;
}

static GimpProcedure *
mail_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            mail_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "*");

      gimp_procedure_set_menu_label (procedure, N_("Send by E_mail..."));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_EDIT);
      gimp_procedure_add_menu_path (procedure, "<Image>/File/Send");

      gimp_procedure_set_documentation (procedure,
                                        N_("Send the image by email"),
#ifdef SENDMAIL
                                        "Sendmail is used to send emails "
                                        "and must be properly configured.",
#else /* xdg-email */
                                        "The preferred email composer is "
                                        "used to send emails and must be "
                                        "properly configured.",
#endif
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Adrian Likins, Reagan Blundell",
                                      "Adrian Likins, Reagan Blundell, "
                                      "Daniel Risacher, "
                                      "Spencer Kimball and Peter Mattis",
                                      "1995-1997");

      GIMP_PROC_ARG_STRING (procedure, "filename",
                            "Filename",
                            "The name of the file to save the image in",
                            NULL,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "to-address",
                            "To address",
                            "The email address to send to",
                            NULL,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "from-address",
                            "From address",
                            "The email address for the From: field",
                            NULL,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "subject",
                            "Subject",
                            "The subject",
                            NULL,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_STRING (procedure, "comment",
                            "Comment",
                            "The comment",
                            NULL,
                            G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
mail_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          GimpDrawable         *drawable,
          const GimpValueArray *args,
          gpointer              run_data)
{
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  INIT_I18N ();

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      gimp_get_data (PLUG_IN_PROC, &mail_info);
      {
        gchar *filename = g_file_get_path (gimp_image_get_file (image));

        if (filename)
          {
            gchar *basename = g_filename_display_basename (filename);

            g_strlcpy (mail_info.filename, basename, BUFFER_SIZE);
            g_free (basename);
            g_free (filename);
          }
      }

      if (! send_dialog ())
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL,
                                                 NULL);
      break;

    case GIMP_RUN_NONINTERACTIVE:
      g_strlcpy (mail_info.filename, GIMP_VALUES_GET_STRING (args, 0),
                 BUFFER_SIZE);
      g_strlcpy (mail_info.receipt,  GIMP_VALUES_GET_STRING (args, 1),
                 BUFFER_SIZE);
      g_strlcpy (mail_info.from,     GIMP_VALUES_GET_STRING (args, 2),
                 BUFFER_SIZE);
      g_strlcpy (mail_info.subject,  GIMP_VALUES_GET_STRING (args, 3),
                 BUFFER_SIZE);
      g_strlcpy (mail_info.comment,  GIMP_VALUES_GET_STRING (args, 4),
                 BUFFER_SIZE);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      gimp_get_data (PLUG_IN_PROC, &mail_info);
      break;

    default:
      break;
    }

  status = send_image (mail_info.filename,
                       image,
                       drawable,
                       run_mode);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (mesg_body)
        g_strlcpy (mail_info.comment, mesg_body, BUFFER_SIZE);

      gimp_set_data (PLUG_IN_PROC, &mail_info, sizeof (m_info));
    }

  return gimp_procedure_new_return_values (procedure, status, NULL);
}

static GimpPDBStatusType
send_image (const gchar  *filename,
            GimpImage    *image,
            GimpDrawable *drawable,
            gint32        run_mode)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpItem         **drawables;
  gchar             *ext;
  GFile             *tmpfile;
  gchar             *tmpname;
#ifndef SENDMAIL /* xdg-email */
  gchar             *mailcmd[9];
  gchar             *filepath = NULL;
  GFile             *tmp_dir  = NULL;
  GFileEnumerator   *enumerator;
  gint               i;
#else /* SENDMAIL */
  gchar             *mailcmd[3];
  GPid               mailpid;
  FILE              *mailpipe = NULL;
#endif
  GError            *error = NULL;

  ext = find_extension (filename);

  if (ext == NULL)
    return GIMP_PDB_CALLING_ERROR;

  /* get a temp name with the right extension and save into it. */
  tmpfile = gimp_temp_file (ext + 1);
  tmpname = g_file_get_path (tmpfile);

  drawables = g_new (GimpItem *, 1);
  drawables[0] = (GimpItem *) drawable;
  if (! (gimp_file_save (run_mode,
                         image, 1,
                         (const GimpItem **) drawables,
                         tmpfile) &&
         valid_file (tmpfile)))
    {
      g_free (drawables);
      goto error;
    }
  g_free (drawables);

#ifndef SENDMAIL /* xdg-email */
  /* From xdg-email doc:
   * "Some e-mail applications require the file to remain present
   * after xdg-email returns."
   * As a consequence, the file cannot be removed at the end of the
   * function. We actually have no way to ever know *when* the file can
   * be removed since the caller could leave the email window opened for
   * hours. Yet we still want to clean sometimes and not have temporary
   * images piling up.
   * So I use a known directory that we control under $GIMP_DIRECTORY/tmp/,
   * and clean it out each time the plugin runs. This means that *if* you
   * are in the above case (your email client requires the file to stay
   * alive), you cannot run twice the plugin at the same time.
   */
  tmp_dir = gimp_directory_file ("tmp", PLUG_IN_PROC, NULL);

  if (g_mkdir_with_parents (gimp_file_get_utf8_name (tmp_dir),
                            S_IRUSR | S_IWUSR | S_IXUSR) == -1)
    {
      g_message ("Temporary directory %s could not be created.",
                 gimp_file_get_utf8_name (tmp_dir));
      g_error_free (error);
      goto error;
    }

  enumerator = g_file_enumerate_children (tmp_dir,
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                          G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                          NULL, NULL);
  if (enumerator)
    {
      GFileInfo *info;

      while ((info = g_file_enumerator_next_file (enumerator,
                                                  NULL, NULL)))
        {
          if (g_file_info_get_file_type (info) == G_FILE_TYPE_REGULAR)
            {
              GFile *file = g_file_enumerator_get_child (enumerator, info);
              g_file_delete (file, NULL, NULL);
              g_object_unref (file);
            }

          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }

  filepath = g_build_filename (gimp_file_get_utf8_name (tmp_dir),
                               mail_info.filename, NULL);
  if (g_rename (tmpname, filepath) == -1)
    {
      /* But on some system, I got an 'Invalid cross-device link' errno
       * with g_rename().
       * On the other hand, g_file_move() seems to be more robust.
       */
      GFile *source = tmpfile;
      GFile *target = g_file_new_for_path (filepath);

      if (! g_file_move (source, target, G_FILE_COPY_NONE, NULL, NULL, NULL, &error))
        {
          g_message ("%s", error->message);
          g_clear_error (&error);
          g_object_unref (target);
          goto error;
        }

      g_object_unref (target);
    }

  mailcmd[0] = g_strdup ("xdg-email");
  mailcmd[1] = "--attach";
  mailcmd[2] = filepath;
  i = 3;
  if (strlen (mail_info.subject) > 0)
    {
      mailcmd[i++] = "--subject";
      mailcmd[i++] = mail_info.subject;
    }
  if (strlen (mail_info.comment) > 0)
    {
      mailcmd[i++] = "--body";
      mailcmd[i++] = mail_info.comment;
    }
  if (strlen (mail_info.receipt) > 0)
    {
      mailcmd[i++] = mail_info.receipt;
    }
  mailcmd[i] = NULL;

  if (! g_spawn_async (NULL, mailcmd, NULL,
                       G_SPAWN_SEARCH_PATH,
                       NULL, NULL, NULL, &error))
    {
      g_message ("%s", error->message);
      g_error_free (error);
      goto error;
    }

#else /* SENDMAIL */
  /* construct the "sendmail user@location" line */
  if (strlen (SENDMAIL) == 0)
    mailcmd[0] = g_strdup ("sendmail");
  else
    mailcmd[0] = g_build_filename (SENDMAIL, "sendmail", NULL);

  mailcmd[1] = mail_info.receipt;
  mailcmd[2] = NULL;

  /* create a pipe to sendmail */
  mailpipe = sendmail_pipe (mailcmd, &mailpid);

  if (mailpipe == NULL)
    return GIMP_PDB_EXECUTION_ERROR;

  sendmail_create_headers (mailpipe);

  fflush (mailpipe);

  if (! sendmail_to64 (tmpname, mailpipe, &error))
    {
      g_message ("%s", error->message);
      g_error_free (error);
      goto error;
    }

  fprintf (mailpipe, "\n--GUMP-MIME-boundary--\n");
#endif

  goto cleanup;

error:
  /* stop sendmail from doing anything */
#ifdef SENDMAIL
  kill (mailpid, SIGINT);
#endif
  status = GIMP_PDB_EXECUTION_ERROR;

cleanup:
  /* close out the sendmail process */
#ifdef SENDMAIL
  if (mailpipe)
    {
      fclose (mailpipe);
      waitpid (mailpid, NULL, 0);
      g_spawn_close_pid (mailpid);
    }

  /* delete the tmpfile that was generated */
  g_unlink (tmpname);
#else
  if (tmp_dir)
    g_object_unref (tmp_dir);
  if (filepath)
    g_free (filepath);
#endif

  g_free (mailcmd[0]);
  g_free (tmpname);
  g_object_unref (tmpfile);

  return status;
}


static gboolean
send_dialog (void)
{
  GtkWidget     *dlg;
  GtkWidget     *main_vbox;
  GtkWidget     *entry;
  GtkWidget     *grid;
#ifdef SENDMAIL
  GtkWidget     *scrolled_window;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
#endif
  gchar         *gump_from;
  gint           row = 0;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  /* check gimprc for a preferred "From:" address */
  gump_from = gimp_gimprc_query ("gump-from");

  if (gump_from)
    {
      g_strlcpy (mail_info.from, gump_from, BUFFER_SIZE);
      g_free (gump_from);
    }

  dlg = gimp_dialog_new (_("Send by Email"), PLUG_IN_ROLE,
                         NULL, 0,
                         gimp_standard_help_func, PLUG_IN_PROC,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_Send"),   GTK_RESPONSE_OK,

                         NULL);

  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* grid */
  grid = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);

  /* Filename entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.filename);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("_Filename:"), 0.0, 0.5,
                            entry, 1);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    mail_info.filename);
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);

#ifdef SENDMAIL
  /* To entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.receipt);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            C_("email-address", "_To:"), 0.0, 0.5,
                            entry, 1);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    mail_info.receipt);

  gtk_widget_grab_focus (entry);

  /* From entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.from);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            C_("email-address", "_From:"), 0.0, 0.5,
                            entry, 1);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    mail_info.from);

  /* Subject entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.subject);
  gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("S_ubject:"), 0.0, 0.5,
                            entry, 1);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    mail_info.subject);

  /* Body  */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (main_vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  text_buffer = gtk_text_buffer_new (NULL);

  g_signal_connect (text_buffer, "changed",
                    G_CALLBACK (mesg_body_callback),
                    NULL);

  gtk_text_buffer_set_text (text_buffer, mail_info.comment, -1);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  g_object_unref (text_buffer);

  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);
#endif

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

static gboolean
valid_file (GFile *file)
{
  gchar    *filename;
  GStatBuf  buf;
  gboolean  valid;

  filename = g_file_get_path (file);
  valid = g_stat (filename, &buf) == 0 && buf.st_size > 0;
  g_free (filename);

  return valid;
}

static gchar *
find_extension (const gchar *filename)
{
  gchar *filename_copy;
  gchar *ext;

  /* we never free this copy - aren't we evil! */
  filename_copy = g_strdup (filename);

  /* find the extension, boy! */
  ext = strrchr (filename_copy, '.');

  while (TRUE)
    {
      if (!ext || ext[1] == '\0' || strchr (ext, G_DIR_SEPARATOR))
        {
          g_message (_("some sort of error with the file extension "
                       "or lack thereof"));

          return NULL;
        }

      if (0 != g_ascii_strcasecmp (ext, ".gz") &&
          0 != g_ascii_strcasecmp (ext, ".bz2"))
        {
          return ext;
        }
      else
        {
          /* we found something, loop back, and look again */
          *ext = 0;
          ext = strrchr (filename_copy, '.');
        }
    }

  g_free (filename_copy);

  return ext;
}

static void
mail_entry_callback (GtkWidget *widget,
                     gchar     *data)
{
  g_strlcpy (data, gtk_entry_get_text (GTK_ENTRY (widget)), BUFFER_SIZE);
}

#ifdef SENDMAIL
static void
mesg_body_callback (GtkTextBuffer *buffer,
                    gpointer       data)
{
  GtkTextIter start_iter;
  GtkTextIter end_iter;

  gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);

  g_free (mesg_body);
  mesg_body = gtk_text_buffer_get_text (buffer, &start_iter, &end_iter, FALSE);
}

static gchar *
sendmail_content_type (const gchar *filename)
{
  /* This function returns a MIME Content-type: value based on the
     filename it is given.  */
  const gchar *type_mappings[20] =
  {
    "gif" , "image/gif",
    "jpg" , "image/jpeg",
    "jpeg", "image/jpeg",
    "tif" , "image/tiff",
    "tiff", "image/tiff",
    "png" , "image/png",
    "g3"  , "image/g3fax",
    "ps"  , "application/postscript",
    "eps" , "application/postscript",
    NULL, NULL
  };

  gchar *ext;
  gint   i;

  ext = find_extension (filename);

  if (!ext)
    {
      return g_strdup ("application/octet-stream");
    }

  i = 0;
  ext += 1;

  while (type_mappings[i])
    {
      if (g_ascii_strcasecmp (ext, type_mappings[i]) == 0)
        {
          return g_strdup (type_mappings[i + 1]);
        }

      i += 2;
    }

  return g_strdup_printf ("image/x-%s", ext);
}

static void
sendmail_create_headers (FILE *mailpipe)
{
  /* create all the mail header stuff. Feel free to add your own */
  /* It is advisable to leave the X-Mailer header though, as     */
  /* there is a possibility of a Gimp mail scanner/reader in the  */
  /* future. It will probably need that header.                 */

  fprintf (mailpipe, "To: %s \n", mail_info.receipt);
  fprintf (mailpipe, "Subject: %s \n", mail_info.subject);
  if (strlen (mail_info.from) > 0)
    fprintf (mailpipe, "From: %s \n", mail_info.from);

  fprintf (mailpipe, "X-Mailer: GIMP Useless Mail plug-in %s\n", GIMP_VERSION);

  fprintf (mailpipe, "MIME-Version: 1.0\n");
  fprintf (mailpipe, "Content-type: multipart/mixed; "
                     "boundary=GUMP-MIME-boundary\n");

  fprintf (mailpipe, "\n\n");

  fprintf (mailpipe, "--GUMP-MIME-boundary\n");
  fprintf (mailpipe, "Content-type: text/plain; charset=UTF-8\n\n");

  if (mesg_body)
    fprintf (mailpipe, "%s", mesg_body);

  fprintf (mailpipe, "\n\n");

  {
    gchar *content = sendmail_content_type (mail_info.filename);

    fprintf (mailpipe, "--GUMP-MIME-boundary\n");
    fprintf (mailpipe, "Content-type: %s\n", content);
    fprintf (mailpipe, "Content-transfer-encoding: base64\n");
    fprintf (mailpipe, "Content-disposition: attachment; filename=\"%s\"\n",
             mail_info.filename);
    fprintf (mailpipe, "Content-description: %s\n\n", mail_info.filename);

    g_free (content);
  }
}

static gboolean
sendmail_to64 (const gchar  *filename,
               FILE         *outfile,
               GError      **error)
{
  GMappedFile  *infile;
  const guchar *in;
  gchar         out[2048];
  gint          state = 0;
  gint          save  = 0;
  gsize         len;
  gsize         bytes;
  gsize         c;

  infile = g_mapped_file_new (filename, FALSE, error);
  if (! infile)
    return FALSE;

  in = (const guchar *) g_mapped_file_get_contents (infile);
  len = g_mapped_file_get_length (infile);

  for (c = 0; c < len;)
    {
      gsize step = MIN (1024, len - c);

      bytes = g_base64_encode_step (in + c, step, TRUE, out, &state, &save);
      fwrite (out, 1, bytes, outfile);

      c += step;
    }

  bytes = g_base64_encode_close (TRUE, out, &state, &save);
  fwrite (out, 1, bytes, outfile);

  g_mapped_file_unref (infile);

  return TRUE;
}

static FILE *
sendmail_pipe (gchar **cmd,
               GPid   *pid)
{
  gint    fd;
  GError *err = NULL;

  if (! g_spawn_async_with_pipes (NULL, cmd, NULL,
                                  G_SPAWN_DO_NOT_REAP_CHILD |
                                  G_SPAWN_SEARCH_PATH,
                                  NULL, NULL, pid, &fd, NULL, NULL, &err))
    {
      g_message (_("Could not start sendmail (%s)"), err->message);
      g_error_free (err);

      *pid = -1;
      return NULL;
    }

  return fdopen (fd, "wb");
}
#endif
