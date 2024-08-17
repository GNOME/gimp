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
#define MAIL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAIL_TYPE, Mail))

GType                   mail_get_type         (void) G_GNUC_CONST;

static GList          * mail_init_procedures  (GimpPlugIn           *plug_in);
static GimpProcedure  * mail_create_procedure (GimpPlugIn           *plug_in,
                                               const gchar          *name);

static GimpValueArray * mail_run              (GimpProcedure        *procedure,
                                               GimpRunMode           run_mode,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               GimpProcedureConfig  *config,
                                               gpointer              run_data);

static GimpPDBStatusType  send_image          (GObject              *config,
                                               GimpImage            *image,
                                               gint                  n_drawables,
                                               GimpDrawable        **drawables,
                                               gint32                run_mode);

static gboolean           send_dialog             (GimpProcedure    *procedure,
                                                   GObject          *config);
static gboolean           valid_file              (GFile            *file);
static gchar            * find_extension          (const gchar      *filename);

#ifdef SENDMAIL
static gchar            * sendmail_content_type   (const gchar      *filename);
static void               sendmail_create_headers (FILE             *mailpipe,
                                                   GObject          *config);
static gboolean           sendmail_to64           (const gchar      *filename,
                                                   FILE             *outfile,
                                                   GError          **error);
static FILE             * sendmail_pipe           (gchar           **cmd,
                                                   GPid             *pid);
#endif


G_DEFINE_TYPE (Mail, mail, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (MAIL_TYPE)
DEFINE_STD_SET_I18N


#ifdef SENDMAIL
static gchar *mesg_body = NULL;
#endif


static void
mail_class_init (MailClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->init_procedures  = mail_init_procedures;
  plug_in_class->create_procedure = mail_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
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
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE  |
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLES |
                                           GIMP_PROCEDURE_SENSITIVE_NO_DRAWABLES);

      gimp_procedure_set_menu_label (procedure, _("Send by E_mail..."));
      gimp_procedure_set_icon_name (procedure, GIMP_ICON_EDIT);
      gimp_procedure_add_menu_path (procedure, "<Image>/File/[Send]");

      gimp_procedure_set_documentation (procedure,
                                        _("Send the image by email"),
#ifdef SENDMAIL
                                        _("Sendmail is used to send emails "
                                          "and must be properly configured."),
#else /* xdg-email */
                                        _("The preferred email composer is "
                                          "used to send emails and must be "
                                          "properly configured."),
#endif
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Adrian Likins, Reagan Blundell",
                                      "Adrian Likins, Reagan Blundell, "
                                      "Daniel Risacher, "
                                      "Spencer Kimball and Peter Mattis",
                                      "1995-1997");

      gimp_procedure_add_string_argument (procedure, "filename",
                                          _("File_name"),
                                          _("The name of the file to save the image in"),
                                          NULL,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_string_argument (procedure, "to-address",
                                          _("_To"),
                                          _("The email address to send to"),
                                          "",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_string_argument (procedure, "from-address",
                                          _("_From"),
                                          _("The email address for the From: field"),
                                          "",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_string_argument (procedure, "subject",
                                          _("Su_bject"),
                                          _("The subject"),
                                          "",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_string_argument (procedure, "comment",
                                          _("Co_mment"),
                                          _("The comment"),
                                          NULL,
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static GimpValueArray *
mail_run (GimpProcedure        *procedure,
          GimpRunMode           run_mode,
          GimpImage            *image,
          gint                  n_drawables,
          GimpDrawable        **drawables,
          GimpProcedureConfig  *config,
          gpointer              run_data)
{
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      gchar *filename = g_file_get_path (gimp_image_get_file (image));

      if (filename)
        {
          gchar *basename = g_filename_display_basename (filename);
          gchar  buffername[BUFFER_SIZE];

          g_strlcpy (buffername, basename, BUFFER_SIZE);

          g_object_set (config,
                        "filename", buffername,
                        NULL);
          g_free (basename);
          g_free (filename);
        }

      if (! send_dialog (procedure, G_OBJECT (config)))
        return gimp_procedure_new_return_values (procedure, GIMP_PDB_CANCEL, NULL);
    }

  status = send_image (G_OBJECT (config),
                       image,
                       n_drawables, drawables,
                       run_mode);

  return gimp_procedure_new_return_values (procedure, status, NULL);
}

static GimpPDBStatusType
send_image (GObject       *config,
            GimpImage     *image,
            gint           n_drawables,
            GimpDrawable **drawables,
            gint32         run_mode)
{
  GimpPDBStatusType  status  = GIMP_PDB_SUCCESS;
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
  GError            *error    = NULL;
  gchar             *filename = NULL;
  gchar             *receipt  = NULL;
  gchar             *from     = NULL;
  gchar             *subject  = NULL;
  gchar             *comment  = NULL;

  mailcmd[0] = NULL;
  g_object_get (config,
                "filename",     &filename,
                "to-address",   &receipt,
                "from-address", &from,
                "subject",      &subject,
                "comment",      &comment,
                NULL);

  ext = find_extension (filename);

  if (ext == NULL)
    return GIMP_PDB_CALLING_ERROR;

  /* get a temp name with the right extension and save into it. */
  tmpfile = gimp_temp_file (ext + 1);
  tmpname = g_file_get_path (tmpfile);

  if (! (gimp_file_save (run_mode, image, tmpfile, NULL) &&
         valid_file (tmpfile)))
    {
      goto error;
    }

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
          if (g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_REGULAR)
            {
              GFile *file = g_file_enumerator_get_child (enumerator, info);
              g_file_delete (file, NULL, NULL);
              g_object_unref (file);
            }

          g_object_unref (info);
        }

      g_object_unref (enumerator);
    }

  filepath = g_build_filename (gimp_file_get_utf8_name (tmp_dir), filename,
                               NULL);
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
  if (subject != NULL && strlen (subject) > 0)
    {
      mailcmd[i++] = "--subject";
      mailcmd[i++] = subject;
    }
  if (comment != NULL && strlen (comment) > 0)
    {
      mailcmd[i++] = "--body";
      mailcmd[i++] = comment;
    }
  if (receipt != NULL && strlen (receipt) > 0)
    {
      mailcmd[i++] = receipt;
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

  mailcmd[1] = receipt;
  mailcmd[2] = NULL;

  /* create a pipe to sendmail */
  mailpipe = sendmail_pipe (mailcmd, &mailpid);

  if (mailpipe == NULL)
    return GIMP_PDB_EXECUTION_ERROR;

  sendmail_create_headers (mailpipe, config);

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

  g_free (filename);
  g_free (receipt);
  g_free (from);
  g_free (subject);
  g_free (comment);

  g_free (mailcmd[0]);
  g_free (tmpname);
  g_object_unref (tmpfile);

  return status;
}


static gboolean
send_dialog (GimpProcedure *procedure,
             GObject       *config)
{
  GtkWidget     *dlg;
  GtkWidget     *main_vbox;
  GtkWidget     *entry;
  GtkWidget     *real_entry;
  GtkWidget     *grid;
  GtkWidget     *button;
#ifdef SENDMAIL
  GtkWidget     *scrolled_window;
  GtkWidget     *text_view;
#endif
  gchar         *gump_from;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY);

  /* check gimprc for a preferred "From:" address */
  gump_from = gimp_gimprc_query ("gump-from");

  if (gump_from)
    {
      g_object_set (config,
                    "from-address", gump_from,
                    NULL);
      g_free (gump_from);
    }

  dlg = gimp_procedure_dialog_new (procedure,
                                   GIMP_PROCEDURE_CONFIG (config),
                                   _("Send by Email"));
  /* Change "Ok" button to "Send" */
  button = gtk_dialog_get_widget_for_response (GTK_DIALOG (dlg),
                                               GTK_RESPONSE_OK);
  gtk_button_set_label (GTK_BUTTON (button), _("Send"));


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
  entry = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                            "filename", GIMP_TYPE_LABEL_ENTRY);
  real_entry = gimp_label_entry_get_entry (GIMP_LABEL_ENTRY (entry));
  gtk_widget_set_size_request (real_entry, 200, -1);
  gtk_entry_set_activates_default (GTK_ENTRY (real_entry), TRUE);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg),
                              "filename",
                              NULL);
  gtk_entry_set_max_length (GTK_ENTRY (real_entry),
                            BUFFER_SIZE - 1);

#ifdef SENDMAIL
  /* To entry */
  entry = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                            "to-address",
                                            GIMP_TYPE_LABEL_ENTRY);
  real_entry = gimp_label_entry_get_entry (GIMP_LABEL_ENTRY (entry));
  gtk_widget_set_size_request (real_entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (real_entry), BUFFER_SIZE - 1);
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg),
                              "to-address",
                              NULL);
  gtk_entry_set_max_length (GTK_ENTRY (real_entry),
                            BUFFER_SIZE - 1);

  gtk_widget_grab_focus (real_entry);

  /* From entry */
  entry = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                            "from-address",
                                            GIMP_TYPE_LABEL_ENTRY);
  real_entry = gimp_label_entry_get_entry (GIMP_LABEL_ENTRY (entry));
  gtk_widget_set_size_request (real_entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (real_entry), BUFFER_SIZE - 1);
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg),
                              "from-address",
                              NULL);
  gtk_entry_set_max_length (GTK_ENTRY (real_entry),
                            BUFFER_SIZE - 1);

  /* Subject entry */
  entry = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                            "subject",
                                            GIMP_TYPE_LABEL_ENTRY);
  real_entry = gimp_label_entry_get_entry (GIMP_LABEL_ENTRY (entry));
  gtk_widget_set_size_request (real_entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (real_entry), BUFFER_SIZE - 1);
  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg),
                              "subject",
                              NULL);
  gtk_entry_set_max_length (GTK_ENTRY (real_entry),
                            BUFFER_SIZE - 1);

  /* Body  */
  text_view = gimp_procedure_dialog_get_widget (GIMP_PROCEDURE_DIALOG (dlg),
                                                "comment",
                                                GTK_TYPE_TEXT_VIEW);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);

  scrolled_window =
    gimp_procedure_dialog_fill_scrolled_window (GIMP_PROCEDURE_DIALOG (dlg),
                                                "comment-scrolled",
                                                "comment");
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  gimp_procedure_dialog_fill (GIMP_PROCEDURE_DIALOG (dlg),
                              "comment-scrolled",
                              NULL);
#endif

  gtk_widget_show (dlg);

  run = gimp_procedure_dialog_run (GIMP_PROCEDURE_DIALOG (dlg));

  gtk_widget_destroy (dlg);

  return run;
}

static gboolean
valid_file (GFile *file)
{
  GStatBuf  buf;
  gboolean  valid;

  valid = g_stat (g_file_peek_path (file), &buf) == 0 && buf.st_size > 0;

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

#ifdef SENDMAIL
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
sendmail_create_headers (FILE    *mailpipe,
                         GObject *config)
{
  gchar *filename = NULL;
  gchar *receipt  = NULL;
  gchar *from     = NULL;
  gchar *subject  = NULL;
  gchar *comment  = NULL;

  g_object_get (config,
                "filename",     &filename,
                "to-address",   &receipt,
                "from-address", &from,
                "subject",      &subject,
                "comment",      &comment,
                NULL);

  /* create all the mail header stuff. Feel free to add your own */
  /* It is advisable to leave the X-Mailer header though, as     */
  /* there is a possibility of a Gimp mail scanner/reader in the  */
  /* future. It will probably need that header.                 */

  fprintf (mailpipe, "To: %s \n", receipt);
  fprintf (mailpipe, "Subject: %s \n", subject);
  if (from != NULL && strlen (from) > 0)
    fprintf (mailpipe, "From: %s \n", from);

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
    gchar *content = sendmail_content_type (filename);

    fprintf (mailpipe, "--GUMP-MIME-boundary\n");
    fprintf (mailpipe, "Content-type: %s\n", content);
    fprintf (mailpipe, "Content-transfer-encoding: base64\n");
    fprintf (mailpipe, "Content-disposition: attachment; filename=\"%s\"\n",
             filename);
    fprintf (mailpipe, "Content-description: %s\n\n", filename);

    g_free (content);
  }

  g_free (filename);
  g_free (receipt);
  g_free (from);
  g_free (subject);
  g_free (comment);
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
