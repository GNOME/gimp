/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Daniel Risacher
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

/*
 *   GUMP - Gimp Useless Mail Plugin (or Gump Useless Mail Plugin if
 *   you prefer) version about .85 I would say... give or take a few
 *   decimal points
 *
 *
 *   by Adrian Likins <adrian@gimp.org>
 *      MIME encapsulation by Reagan Blundell <reagan@emails.net>
 *
 *
 *
 *   Based heavily on gz.c by Daniel Risacher
 *
 *     Lets you choose to send a image to the mail from the file save
 *     as dialog.  images are piped to uuencode and then to mail...
 *
 *
 *   This works fine for .99.10. I havent actually tried it in
 *   combination with the gz plugin, but it works with all other file
 *   types. I will eventually get around to making sure it works with
 *   gz.
 *
 *  To use: 1) image->File->mail image
 *          2) when the mail dialog popups up, fill it out. Only to:
 *             and filename are required note: the filename needs to a
 *             type that the image can be saved as. otherwise you will
 *             just send an empty message.
 *          3) click ok and it should be on its way
 *
 *
 * NOTE: You probabaly need sendmail installed. If your sendmail is in
 *       an odd spot you can change the #define below. If you use
 *       qmail or other MTA's, and this works after changing the
 *       MAILER, let me know how well or what changes were needed.
 *
 * NOTE: Uuencode is needed. If it is in the path, it should work fine
 *       as is. Other- wise just change the UUENCODE.
 *
 *
 * TODO: 1) the aforementioned abilty to specify the
 *           uuencode filename                         *done*
 *       2) someway to do this without tmp files
 *              * wont happen anytime soon*
 *       3) MIME? *done*
 *       4) a pointlessly snazzier dialog
 *       5) make sure it works with gz
 *               * works for .xcfgz but not .xcf.gz *
 *       6) add an option to choose if mail get
 *          uuencode or not (or MIME'ed for that matter)
 *       7) realtime preview
 *       8) better entry for comments    *done*
 *       9) list of frequently used addreses
 *      10) openGL compliance
 *      11) better handling of filesave errors
 *
 *
 * As always: The utility of this plugin is left as an exercise for
 * the reader
 *
 */

#include "config.h"

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

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

#ifndef UUENCODE
#define UUENCODE "uuencode"
#endif

enum
{
  ENCAPSULATION_UUENCODE,
  ENCAPSULATION_MIME
};

#define BUFFER_SIZE 256

#define PLUG_IN_PROC   "plug-in-mail-image"
#define PLUG_IN_BINARY "mail"

typedef struct
{
  gchar receipt[BUFFER_SIZE];
  gchar subject[BUFFER_SIZE];
  gchar comment[BUFFER_SIZE];
  gchar from[BUFFER_SIZE];
  gchar filename[BUFFER_SIZE];
  gint  encapsulation;
} m_info;


static void   query (void);
static void   run   (const gchar      *name,
		     gint              nparams,
		     const GimpParam  *param,
		     gint             *nreturn_vals,
		     GimpParam       **return_vals);

static GimpPDBStatusType save_image   (const gchar *filename,
                                       gint32       image_ID,
                                       gint32       drawable_ID,
                                       gint32       run_mode);

static gboolean  save_dialog          (void);
static void      mail_entry_callback  (GtkWidget     *widget,
                                       gchar         *data);
static void      mesg_body_callback   (GtkTextBuffer *buffer,
                                       gpointer       data);

static gboolean  valid_file     (const gchar  *filename);
static void      create_headers (FILE         *mailpipe);
static gchar    *find_extension (const gchar  *filename);
static gboolean  to64           (const gchar  *filename,
                                 FILE         *outfile,
                                 GError      **error);
static gint      sane_dup2      (gint          fd1,
                                 gint          fd2);
static FILE     *sendmail_pipe  (gchar       **cmd,
                                 gint         *pid);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static m_info mail_info =
{
  "", "", "", "", "",
  ENCAPSULATION_MIME,  /* Change this to ENCAPSULATION_UUENCODE
			  if you prefer that as the default */
};

static gchar * mesg_body = NULL;


MAIN ()

static void
query (void)
{
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",      "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,    "image",         "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable",      "Drawable to save" },
    { GIMP_PDB_STRING,   "filename",      "The name of the file to save the image in" },
    { GIMP_PDB_STRING,   "receipt",       "The email address to send to" },
    { GIMP_PDB_STRING,   "from",          "The email address for the From: field" },
    { GIMP_PDB_STRING,   "subject",       "The subject" },
    { GIMP_PDB_STRING,   "comment",       "The Comment" },
    { GIMP_PDB_INT32,    "encapsulation", "Uuencode, MIME" }
  };

  gimp_install_procedure (PLUG_IN_PROC,
			  N_("Email the image"),
			  "You need to have uuencode and mail installed",
			  "Adrian Likins, Reagan Blundell",
			  "Adrian Likins, Reagan Blundell, Daniel Risacher, "
                          "Spencer Kimball and Peter Mattis",
			  "1995-1997",
			  N_("_Mail Image..."),
			  "RGB*, GRAY*, INDEXED*",
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
          if (! strlen (mail_info.filename))
            {
              gchar *filename = gimp_image_get_filename (image_ID);

              if (filename)
                {
                  gchar *basename = g_path_get_basename (filename);

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
	  if (nparams != 9)
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
	      mail_info.encapsulation = param[8].data.d_int32;
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
	    gimp_set_data (PLUG_IN_PROC, &mail_info, sizeof (m_info));
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
  pid_t              uupid;
  gint               wpid;
  gint               process_status;
  FILE              *mailpipe;

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

  /* This is necessary to make the comments and headers work
   * correctly. Not real sure why.
   */
  fflush (mailpipe);

  if (! (gimp_file_save (run_mode,
			 image_ID,
			 drawable_ID,
			 tmpname,
			 tmpname) && valid_file (tmpname)))
    {
      goto error;
    }

  if (mail_info.encapsulation == ENCAPSULATION_UUENCODE)
    {
      /* fork off a uuencode process */
      if ((uupid = fork ()) < 0)
	{
	  g_message ("fork() failed: %s", g_strerror (errno));
	  goto error;
	}
      else if (uupid == 0)
	{
	  if (sane_dup2 (fileno (mailpipe), fileno (stdout)) == -1)
	    {
	      g_message ("dup2() failed: %s", g_strerror (errno));
	    }

	  execlp (UUENCODE, UUENCODE, tmpname, filename, NULL);

	  /* What are we doing here? exec must have failed */
	  g_message ("exec failed: uuencode: %s", g_strerror (errno));
	  _exit (127);
	}
      else
        {
	  wpid = waitpid (uupid, &process_status, 0);

	  if ((wpid < 0)
	      || !WIFEXITED (process_status)
	      || (WEXITSTATUS (process_status) != 0))
	    {
	      g_message ("mail didn't work or something on file '%s'",
                         gimp_filename_to_utf8 (tmpname));
	      goto error;
	    }
	}
    }
  else
    {
      /* This must be MIME stuff. Base64 away... */
      GError *error = NULL;

      if (! to64 (tmpname, mailpipe, &error))
        {
          g_message (error->message);
          g_error_free (error);
          goto error;
        }

      /* close off mime */
      if (mail_info.encapsulation == ENCAPSULATION_MIME)
        fprintf (mailpipe, "\n--GUMP-MIME-boundary--\n");
    }

  goto cleanup;

error:
  /* stop sendmail from doing anything */
  kill (mailpid, SIGINT);
  status = GIMP_PDB_EXECUTION_ERROR;

cleanup:
  /* close out the sendmail process */
  fclose (mailpipe);
  waitpid (mailpid, NULL, 0);

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
  GtkWidget     *label;
  GtkWidget     *vbox;
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

  dlg = gimp_dialog_new (_("Send as Mail"), PLUG_IN_BINARY,
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

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /* table */
  table = gtk_table_new (5, 2, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 2, 12);
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

  /* Encapsulation label */
  label = gtk_label_new (_("Encapsulation:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.2);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Encapsulation radiobuttons */
  vbox = gimp_int_radio_group_new (FALSE, NULL,
				   G_CALLBACK (gimp_radio_button_update),
				   &mail_info.encapsulation,
				   mail_info.encapsulation,

				   _("_MIME"),     ENCAPSULATION_MIME,     NULL,
				   _("_Uuencode"), ENCAPSULATION_UUENCODE, NULL,

				   NULL);
  gtk_table_attach (GTK_TABLE (table), vbox, 1, 2, row, row + 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vbox);
  row += 2;

  /* To entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.receipt);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
			     _("_Recipient:"), 0.0, 0.5,
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
			     _("_Sender:"), 0.0, 0.5,
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

  /* Comment entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  gtk_entry_set_max_length (GTK_ENTRY (entry), BUFFER_SIZE - 1);
  gtk_entry_set_text (GTK_ENTRY (entry), mail_info.comment);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
			     _("Comm_ent:"), 0.0, 0.5,
			     entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    mail_info.comment);


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

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  g_object_unref (text_buffer);

  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  g_signal_connect (text_buffer, "changed",
                    G_CALLBACK (mesg_body_callback),
		    NULL);

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

  if (mail_info.encapsulation == ENCAPSULATION_MIME)
    {
      fprintf (mailpipe, "MIME-Version: 1.0\n");
      fprintf (mailpipe, "Content-type: multipart/mixed; "
                         "boundary=GUMP-MIME-boundary\n");
    }

  fprintf (mailpipe, "\n\n");

  if (mail_info.encapsulation == ENCAPSULATION_MIME)
    {
      fprintf (mailpipe, "--GUMP-MIME-boundary\n");
      fprintf (mailpipe, "Content-type: text/plain; charset=UTF-8\n\n");
    }

  if (strlen (mail_info.comment) > 0)
    {
      fprintf (mailpipe, "%s", mail_info.comment);
      fprintf (mailpipe, "\n\n");
    }

  if (mesg_body)
    fprintf (mailpipe, "%s", mesg_body);

  fprintf (mailpipe, "\n\n");

  if (mail_info.encapsulation == ENCAPSULATION_MIME)
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

      bytes = g_base64_encode_step (in, step, TRUE, out, &state, &save);
      fwrite (out, 1, bytes, outfile);

      c += step;
    }

  bytes = g_base64_encode_close (TRUE, out, &state, &save);
  fwrite (out, 1, bytes, outfile);

  g_mapped_file_free (infile);

  return TRUE;
}

static gint
sane_dup2 (gint fd1,
           gint fd2)
{
  gint ret;

 retry:
  ret = dup2 (fd1, fd2);

  if (ret < 0 && errno == EINTR)
    goto retry;

  return ret;
}

static FILE *
sendmail_pipe (gchar **cmd,
               gint   *pid)
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
