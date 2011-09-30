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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

#include <sys/types.h>
#include <sys/wait.h>

#include <glib/gstdio.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* GdkPixbuf RGBA C-Source image dump 1-byte-run-length-encoded */

#ifdef __SUNPRO_C
#pragma align 4 (mail_icon)
#endif
#ifdef __GNUC__
static const guint8 mail_icon[] __attribute__ ((__aligned__ (4))) =
#else
static const guint8 mail_icon[] =
#endif
{ ""
  /* Pixbuf magic (0x47646b50) */
  "GdkP"
  /* length: header (24) + pixel_data (584) */
  "\0\0\2`"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (64) */
  "\0\0\0@"
  /* width (16) */
  "\0\0\0\20"
  /* height (16) */
  "\0\0\0\20"
  /* pixel_data: */
  "\245\0\0\0\0\1\210\212\2050\205\210\212\205\377\1\210\212\2050\210\0"
  "\0\0\0\2\210\212\2050\210\212\205\377\205\377\377\377\377\2\210\212\205"
  "\377\210\212\2050\206\0\0\0\0\13\210\212\2050\210\212\205\377\377\377"
  "\377\377\331\326\320\377\311\307\301\377\301\277\272\377\257\254\251"
  "\377\241\240\232\377\377\377\377\377\210\212\205\377\210\212\2050\204"
  "\0\0\0\0\15\210\212\2050\210\212\205\377\377\377\377\377\331\326\320"
  "\377\250\247\241\377\276\275\271\377\316\315\312\377\317\316\314\377"
  "\306\305\305\377\275\275\275\377\377\377\377\377\210\212\205\377\210"
  "\212\2050\203\0\0\0\0\7\210\212\205\377\344\344\343\377\271\270\263\377"
  "\225\224\216\377\333\332\331\377\354\354\353\377\365\365\365\377\202"
  "\377\377\377\377\4\305\305\305\377\261\256\252\377\352\352\351\377\210"
  "\212\205\377\203\0\0\0\0\4\210\212\205\377\367\367\367\377\257\256\254"
  "\377\365\365\365\377\204\377\377\377\377\5\353\353\353\377\330\330\330"
  "\377\315\314\311\377\367\367\367\377\210\212\205\377\203\0\0\0\0\4\210"
  "\212\205\377\377\377\377\377\346\345\342\377\252\251\247\377\202\353"
  "\353\353\377\7\341\341\341\377\330\330\330\377\317\317\317\377\246\245"
  "\241\377\363\363\362\377\377\377\377\377\210\212\205\377\203\0\0\0\0"
  "\5\210\212\205\377\377\377\377\377\373\373\370\377\342\342\337\377\222"
  "\221\221\377\202\266\264\263\377\6\274\273\267\377\241\240\240\377\363"
  "\363\362\377\367\365\363\377\377\377\377\377\210\212\205\377\203\0\0"
  "\0\0\5\210\212\205\377\377\377\377\377\373\373\370\377\360\357\355\377"
  "\245\242\235\377\203\373\373\370\377\5\220\215\207\377\371\367\363\377"
  "\362\360\354\377\377\377\377\377\210\212\205\377\203\0\0\0\0\4\210\212"
  "\205\377\377\377\377\377\373\372\367\377\262\261\256\377\203\373\373"
  "\370\377\202\371\370\364\377\4\274\273\267\377\351\347\342\377\377\377"
  "\377\377\210\212\205\377\203\0\0\0\0\15\210\212\205\377\377\377\377\377"
  "\311\307\304\377\373\373\370\377\367\365\361\377\372\371\366\377\370"
  "\366\362\377\372\370\365\377\367\365\362\377\353\350\343\377\317\316"
  "\312\377\377\377\377\377\210\212\205\377\203\0\0\0\0\2\210\212\205\377"
  "\360\360\360\377\211\377\377\377\377\2\360\360\360\377\210\212\205\377"
  "\203\0\0\0\0\1\210\212\205\217\213\210\212\205\377\1\210\212\205\217"
  "\221\0\0\0\0"
};


#ifndef SENDMAIL
#define SENDMAIL "/usr/lib/sendmail"
#endif

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


static void               query                (void);
static void               run                  (const gchar      *name,
                                                gint              nparams,
                                                const GimpParam  *param,
                                                gint             *nreturn_vals,
                                                GimpParam       **return_vals);

static GimpPDBStatusType  save_image           (const gchar      *filename,
                                                gint32            image_ID,
                                                gint32            drawable_ID,
                                                gint32            run_mode);

static gboolean           save_dialog          (void);
static void               mail_entry_callback  (GtkWidget        *widget,
                                                gchar            *data);
static void               mesg_body_callback   (GtkTextBuffer    *buffer,
                                                gpointer          data);

static gboolean           valid_file           (const gchar      *filename);
static void               create_headers       (FILE             *mailpipe);
static gchar            * find_extension       (const gchar      *filename);
static gboolean           to64                 (const gchar      *filename,
                                                FILE             *outfile,
                                                GError          **error);
static FILE             * sendmail_pipe        (gchar           **cmd,
                                                GPid             *pid);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static m_info mail_info =
{
  "", "", "", "", ""
};

static gchar *mesg_body = NULL;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",      "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }" },
    { GIMP_PDB_IMAGE,    "image",         "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",      "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",      "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "to-address",    "The email address to send to" },
    { GIMP_PDB_STRING,   "from-address",  "The email address for the From: field" },
    { GIMP_PDB_STRING,   "subject",       "The subject" },
    { GIMP_PDB_STRING,   "comment",       "The Comment" },
    { GIMP_PDB_INT32,    "encapsulation", "ignored" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
                          N_("Send the image by email"),
                          "You need to have sendmail installed",
                          "Adrian Likins, Reagan Blundell",
                          "Adrian Likins, Reagan Blundell, Daniel Risacher, "
                          "Spencer Kimball and Peter Mattis",
                          "1995-1997",
                          N_("Send by E_mail..."),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/File/Send");
  gimp_plugin_icon_register (PLUG_IN_PROC, GIMP_ICON_TYPE_INLINE_PIXBUF,
                             mail_icon);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[2];
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gint32             image_ID;
  gint32             drawable_ID;

  INIT_I18N ();

  run_mode    = param[0].data.d_int32;
  image_ID    = param[1].data.d_image;
  drawable_ID = param[2].data.d_drawable;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, PLUG_IN_PROC) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          gimp_get_data (PLUG_IN_PROC, &mail_info);
          {
            gchar *filename = gimp_image_get_filename (image_ID);

            if (filename)
              {
                gchar *basename = g_filename_display_basename (filename);

                g_strlcpy (mail_info.filename, basename, BUFFER_SIZE);
                g_free (basename);
                g_free (filename);
              }
          }

          if (! save_dialog ())
            status = GIMP_PDB_CANCEL;
          break;

        case GIMP_RUN_NONINTERACTIVE:
          /*  Make sure all the arguments are there!  */
          if (nparams < 8)
            {
              status = GIMP_PDB_CALLING_ERROR;
            }
          else
            {
              g_strlcpy (mail_info.filename,
                         param[3].data.d_string, BUFFER_SIZE);
              g_strlcpy (mail_info.receipt,
                         param[4].data.d_string, BUFFER_SIZE);
              g_strlcpy (mail_info.from,
                         param[5].data.d_string, BUFFER_SIZE);
              g_strlcpy (mail_info.subject,
                         param[6].data.d_string, BUFFER_SIZE);
              g_strlcpy (mail_info.comment,
                         param[7].data.d_string, BUFFER_SIZE);
            }
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          gimp_get_data (PLUG_IN_PROC, &mail_info);
          break;

        default:
          break;
        }

      if (status == GIMP_PDB_SUCCESS)
        {
          status = save_image (mail_info.filename,
                               image_ID,
                               drawable_ID,
                               run_mode);

          if (status == GIMP_PDB_SUCCESS)
            {
              if (mesg_body)
                g_strlcpy (mail_info.comment, mesg_body, BUFFER_SIZE);

              gimp_set_data (PLUG_IN_PROC, &mail_info, sizeof (m_info));
            }
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static GimpPDBStatusType
save_image (const gchar *filename,
            gint32       image_ID,
            gint32       drawable_ID,
            gint32       run_mode)
{
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  gchar             *ext;
  gchar             *tmpname;
  gchar             *mailcmd[3];
  GPid               mailpid;
  FILE              *mailpipe;
  GError            *error = NULL;

  ext = find_extension (filename);

  if (ext == NULL)
    return GIMP_PDB_CALLING_ERROR;

  /* get a temp name with the right extension and save into it. */
  tmpname = gimp_temp_name (ext + 1);

  /* construct the "sendmail user@location" line */
  mailcmd[0] = SENDMAIL;
  mailcmd[1] = mail_info.receipt;
  mailcmd[2] = NULL;

  /* create a pipe to sendmail */
  mailpipe = sendmail_pipe (mailcmd, &mailpid);

  if (mailpipe == NULL)
    return GIMP_PDB_EXECUTION_ERROR;

  create_headers (mailpipe);

  fflush (mailpipe);

  if (! (gimp_file_save (run_mode,
                         image_ID,
                         drawable_ID,
                         tmpname,
                         tmpname) && valid_file (tmpname)))
    {
      goto error;
    }

  if (! to64 (tmpname, mailpipe, &error))
    {
      g_message ("%s", error->message);
      g_error_free (error);
      goto error;
    }

  fprintf (mailpipe, "\n--GUMP-MIME-boundary--\n");

  goto cleanup;

error:
  /* stop sendmail from doing anything */
  kill (mailpid, SIGINT);
  status = GIMP_PDB_EXECUTION_ERROR;

cleanup:
  /* close out the sendmail process */
  fclose (mailpipe);
  waitpid (mailpid, NULL, 0);
  g_spawn_close_pid (mailpid);

  /* delete the tmpfile that was generated */
  g_unlink (tmpname);
  g_free (tmpname);

  return status;
}


static gboolean
save_dialog (void)
{
  GtkWidget     *dlg;
  GtkWidget     *main_vbox;
  GtkWidget     *entry;
  GtkWidget     *table;
  GtkWidget     *scrolled_window;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
  gchar         *gump_from;
  gint           row = 0;
  gboolean       run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

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

                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                         _("_Send"),       GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dlg));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* table */
  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  /* Filename entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.filename);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Filename:"), 0.0, 0.5,
                             entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    mail_info.filename);

  /* To entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.receipt);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             C_("email-address", "_To:"), 0.0, 0.5,
                             entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    mail_info.receipt);

  gtk_widget_grab_focus (entry);

  /* From entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.from);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             C_("email-address", "_From:"), 0.0, 0.5,
                             entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    mail_info.from);

  /* Subject entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.subject);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("S_ubject:"), 0.0, 0.5,
                             entry, 1, FALSE);
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

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

static gboolean
valid_file (const gchar *filename)
{
  struct stat buf;

  return g_stat (filename, &buf) == 0 && buf.st_size > 0;
}

static gchar *
find_content_type (const gchar *filename)
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
          /* we found somehting, loop back, and look again */
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

static void
create_headers (FILE *mailpipe)
{
  /* create all the mail header stuff. Feel free to add your own */
  /* It is advisable to leave the X-Mailer header though, as     */
  /* there is a possibilty of a Gimp mail scanner/reader in the  */
  /* future. It will probabaly need that header.                 */

  fprintf (mailpipe, "To: %s \n", mail_info.receipt);
  fprintf (mailpipe, "Subject: %s \n", mail_info.subject);
  if (strlen (mail_info.from) > 0)
    fprintf (mailpipe, "From: %s \n", mail_info.from);

  fprintf (mailpipe, "X-Mailer: GIMP Useless Mail Plug-In %s\n", GIMP_VERSION);

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
    gchar *content = find_content_type (mail_info.filename);

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
to64 (const gchar  *filename,
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

  if (! g_spawn_async_with_pipes (NULL, cmd, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
                                  NULL, NULL, pid, &fd, NULL, NULL, &err))
    {
      g_message (_("Could not start sendmail (%s)"), err->message);
      g_error_free (err);

      *pid = -1;
      return NULL;
    }

  return fdopen (fd, "wb");
}
