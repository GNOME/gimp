/* The GIMP -- an image manipulation program
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
 *   GUMP - Gimp Useless Mail Plugin (or Gump Useless Mail Plugin if you prefer)
 *          version about .85 I would say... give or take a few decimal points
 *
 *
 *   by Adrian Likins <adrian@gimp.org>
 *      MIME encapsulation by Reagan Blundell <reagan@emails.net>
 *
 *
 *
 *   Based heavily on gz.c by Daniel Risacher
 *
 *     Lets you choose to send a image to the mail from the file save as dialog.
 *      images are piped to uuencode and then to mail...
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
 * As always: The utility of this plugin is left as an exercise for the reader
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <gtk/gtk.h>

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
  /* length: header (24) + pixel_data (473) */
  "\0\0\1\361"
  /* pixdata_type (0x2010002) */
  "\2\1\0\2"
  /* rowstride (64) */
  "\0\0\0@"
  /* width (16) */
  "\0\0\0\20"
  /* height (16) */
  "\0\0\0\20"
  /* pixel_data: */
  "\261\0\0\0\0\3\0\0\0e\0\0\0\377\40\40\40\377\211\0\0\0\377\1\0\0\0e\203"
  "\0\0\0\0\2\0\0\0\377ttt\377\211\365\365\365\377\2DDD\377\0\0\0\377\203"
  "\0\0\0\0\15\0\0\0\377\365\365\365\377ggg\377\361\361\361\377\343\343"
  "\343\377\365\365\365\377\343\343\343\377\363\363\363\377\343\343\343"
  "\377\330\330\330\377VVV\377\270\270\270\377\0\0\0\377\203\0\0\0\0\15"
  "\0\0\0\377\365\365\365\377\343\343\343\377hhh\377\363\363\363\377\345"
  "\345\345\377\362\362\362\377\343\343\343\377\326\326\326\377YYY\377\326"
  "\326\326\377\260\260\260\377\0\0\0\377\203\0\0\0\0\2\0\0\0\377\365\365"
  "\365\377\202\343\343\343\377\11eee\377\363\363\363\377\343\343\343\377"
  "\333\333\333\377YYY\377\343\343\343\377\323\323\323\377\260\260\260\377"
  "\0\0\0\377\203\0\0\0\0\15\0\0\0\377\365\365\365\377\343\343\343\377\336"
  "\336\336\377\177\177\177\377YYY\377\365\365\365\377YYY\377nnn\377\330"
  "\330\330\377\306\306\306\377\257\257\257\377\0\0\0\377\203\0\0\0\0\4"
  "\0\0\0\377\365\365\365\377\340\340\340\377yyy\377\202\343\343\343\377"
  "\1YYY\377\202\333\333\333\377\4eee\377\266\266\266\377\257\257\257\377"
  "\0\0\0\377\203\0\0\0\0\15\0\0\0\377\365\365\365\377{{{\377\343\343\343"
  "\377\323\323\323\377\334\334\334\377\325\325\325\377\333\333\333\377"
  "\324\324\324\377\270\270\270\377JJJ\377\265\265\265\377\0\0\0\377\203"
  "\0\0\0\0\3\0\0\0\377ZZZ\377\252\252\252\377\203\253\253\253\377\1\252"
  "\252\252\377\202\254\254\254\377\4\255\255\255\377\262\262\262\377DD"
  "D\377\0\0\0\377\203\0\0\0\0\1\0\0\0e\213\0\0\0\377\1\0\0\0e\262\0\0\0"
  "\0"};



#define ENCAPSULATION_UUENCODE 0
#define ENCAPSULATION_MIME     1

#define BUFFER_SIZE            256

#ifndef SENDMAIL
#define SENDMAIL "/usr/lib/sendmail"
#endif

#ifndef UUENCODE
#define UUENCODE "uuencode"
#endif

#define PLUG_IN_NAME  "plug_in_mail_image"
#define HELP_ID       "plug-in-mail-image"


static void   query (void);
static void   run   (const gchar      *name,
		     gint              nparams,
		     const GimpParam  *param,
		     gint             *nreturn_vals,
		     GimpParam       **return_vals);

static GimpPDBStatusType save_image (const gchar *filename,
			             gint32       image_ID,
			             gint32       drawable_ID,
				     gint32       run_mode);

static gint    save_dialog          (void);
static void    mail_entry_callback  (GtkWidget     *widget,
                                     gpointer       data);
static void    mesg_body_callback   (GtkTextBuffer *buffer,
                                     gpointer       data);

static gint    valid_file     (const gchar *filename);
static void    create_headers (FILE        *mailpipe);
static gchar * find_extension (const gchar *filename);
static gint    to64           (FILE        *infile,
                               FILE        *outfile);
static void    output64chunk  (gint         c1,
                               gint         c2,
                               gint         c3,
                               gint         pads,
                               FILE        *outfile);

GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

typedef struct
{
  gchar receipt[BUFFER_SIZE];
  gchar subject[BUFFER_SIZE];
  gchar comment[BUFFER_SIZE];
  gchar from[BUFFER_SIZE];
  gchar filename[BUFFER_SIZE];
  gint  encapsulation;
}
m_info;

static m_info mail_info =
{
  /* I would a assume there is a better way to do this, but this works for now */
  "\0",
  "\0",
  "\0",
  "\0",
  "\0",
  ENCAPSULATION_MIME,  /* Change this to ENCAPSULATION_UUENCODE
			  if you prefer that as the default */
};

static gchar * mesg_body = NULL;


MAIN ()

static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32, "run_mode", "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE, "image", "Input image" },
    { GIMP_PDB_DRAWABLE, "drawable", "Drawable to save" },
    { GIMP_PDB_STRING, "filename", "The name of the file to save the image in" },
    { GIMP_PDB_STRING, "receipt", "The email address to send to" },
    { GIMP_PDB_STRING, "from", "The email address for the From: field" },
    { GIMP_PDB_STRING, "subject", "The subject" },
    { GIMP_PDB_STRING, "comment", "The Comment" },
    { GIMP_PDB_INT32,  "encapsulation", "Uuencode, MIME" }
  };

  gimp_install_procedure (PLUG_IN_NAME,
			  "pipe files to uuencode then mail them",
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

  gimp_plugin_menu_register (PLUG_IN_NAME, "<Image>/File/Send");
  gimp_plugin_icon_register (PLUG_IN_NAME,
                             GIMP_ICON_TYPE_INLINE_PIXBUF, mail_icon);
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

  if (strcmp (name, PLUG_IN_NAME) == 0)
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  gimp_get_data (PLUG_IN_NAME, &mail_info);
	  if (!save_dialog ())
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
	      /* this hasnt been tested yet */
	      strncpy (mail_info.filename, param[3].data.d_string, BUFFER_SIZE);
	      strncpy (mail_info.receipt, param[4].data.d_string, BUFFER_SIZE);
	      strncpy (mail_info.receipt, param[5].data.d_string, BUFFER_SIZE);
	      strncpy (mail_info.subject, param[6].data.d_string, BUFFER_SIZE);
	      strncpy (mail_info.comment, param[7].data.d_string, BUFFER_SIZE);
	      mail_info.encapsulation = param[8].data.d_int32;
	    }
	  break;

	case GIMP_RUN_WITH_LAST_VALS:
	  gimp_get_data (PLUG_IN_NAME, &mail_info);
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
	    gimp_set_data (PLUG_IN_NAME, &mail_info, sizeof(m_info));
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
  gchar  *ext;
  gchar  *tmpname;
  gchar   mailcmdline[512];
  gint    pid;
  gint    wpid;
  gint    process_status;
  FILE   *mailpipe;
  FILE   *infile;

  if (NULL == (ext = find_extension (filename)))
    return GIMP_PDB_CALLING_ERROR;

  /* get a temp name with the right extension and save into it. */
  tmpname = gimp_temp_name (ext + 1);

  /* construct the "sendmail user@location" line */
  strcpy (mailcmdline, SENDMAIL);
  strcat (mailcmdline, " ");
  strcat (mailcmdline, mail_info.receipt);

  /* create a pipe to sendmail */
  mailpipe = popen (mailcmdline, "w");

  create_headers (mailpipe);

  /* This is necessary to make the comments and headers work correctly. Not real sure why */
  fflush (mailpipe);

  if (! (gimp_file_save (run_mode,
			 image_ID,
			 drawable_ID,
			 tmpname,
			 tmpname) && valid_file (tmpname)) )
    {
      unlink (tmpname);
      g_free (tmpname);
      return GIMP_PDB_EXECUTION_ERROR;
    }

  if (mail_info.encapsulation == ENCAPSULATION_UUENCODE)
    {
      /* fork off a uuencode process */
      if ((pid = fork ()) < 0)
	{
	  g_message ("fork() failed: %s", g_strerror (errno));
	  g_free (tmpname);
	  return GIMP_PDB_EXECUTION_ERROR;
	}
      else if (pid == 0)
	{
	  if (-1 == dup2 (fileno (mailpipe), fileno (stdout)))
	    {
	      g_message ("dup2() failed: %s", g_strerror (errno));
	    }

	  execlp (UUENCODE, UUENCODE, tmpname, filename, NULL);
	  /* What are we doing here? exec must have failed */
	  g_message ("exec failed: uuencode: %s", g_strerror (errno));

	  /* close the pipe now */
	  pclose (mailpipe);
	  g_free (tmpname);
	  _exit (127);
	}
      else
        {
	  wpid = waitpid (pid, &process_status, 0);

	  if ((wpid < 0)
	      || !WIFEXITED (process_status)
	      || (WEXITSTATUS (process_status) != 0))
	    {
	      g_message ("mail didnt work or something on file '%s'",
                         gimp_filename_to_utf8 (tmpname));
	      g_free (tmpname);
	      return GIMP_PDB_EXECUTION_ERROR;
	    }
	}
    }
  else
    {  /* This must be MIME stuff. Base64 away... */
      infile = fopen(tmpname,"r");
      to64(infile,mailpipe);
      /* close off mime */
      if( mail_info.encapsulation == ENCAPSULATION_MIME )
	{
	  fprintf (mailpipe, "\n--GUMP-MIME-boundary--\n");
	}
    }

  /* delete the tmpfile that was generated */
  unlink (tmpname);
  g_free (tmpname);

  return GIMP_PDB_SUCCESS;
}


static gint
save_dialog (void)
{
  GtkWidget     *dlg;
  GtkWidget     *entry;
  GtkWidget     *table;
  GtkWidget     *label;
  GtkWidget     *vbox;
  GtkWidget     *scrolled_window;
  GtkWidget     *text_view;
  GtkTextBuffer *text_buffer;
  gchar          buffer[BUFFER_SIZE];
  gchar         *gump_from;
  gboolean       run;

  gimp_ui_init ("mail", FALSE);

  /* check gimprc for a preferred "From:" address */
  gump_from = gimp_gimprc_query ("gump-from");

  if (gump_from)
    {
      strncpy (mail_info.from, gump_from, BUFFER_SIZE);
      g_free (gump_from);
    }

  dlg = gimp_dialog_new (_("Send as Mail"), "mail",
                         NULL, 0,
			 gimp_standard_help_func, HELP_ID,

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  /* table */
  table = gtk_table_new (7, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  /* to: dialog */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.receipt);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("_Recipient:"), 0.0, 0.5,
			     entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    &mail_info.receipt);

  /* From entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.from);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("_Sender:"), 0.0, 0.5,
			     entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    &mail_info.from);

  /* Subject entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.subject);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("S_ubject:"), 0.0, 0.5,
			     entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    &mail_info.subject);

  /* Comment entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.comment);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Comm_ent:"), 0.0, 0.5,
			     entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    &mail_info.comment);

  /* Filename entry */
  entry = gtk_entry_new ();
  gtk_widget_set_size_request (entry, 200, -1);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.filename);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
			     _("_Filename:"), 0.0, 0.5,
			     entry, 1, FALSE);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (mail_entry_callback),
                    &mail_info.filename);

  /* comment  */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_table_attach (GTK_TABLE (table), scrolled_window,
                    0, 2, 5, 6,
                    GTK_EXPAND | GTK_FILL,
                    GTK_EXPAND | GTK_FILL,
                    0, 0);
  gtk_widget_show (scrolled_window);

  text_buffer = gtk_text_buffer_new (NULL);

  text_view = gtk_text_view_new_with_buffer (text_buffer);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (text_view), GTK_WRAP_WORD);
  gtk_container_add (GTK_CONTAINER (scrolled_window), text_view);
  gtk_widget_show (text_view);

  g_object_unref (text_buffer);

  g_signal_connect (text_buffer, "changed",
                    G_CALLBACK (mesg_body_callback),
		    NULL);

  /* Encapsulation label */
  label = gtk_label_new (_("Encapsulation:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.2);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 6, 7,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Encapsulation radiobuttons */
  vbox = gimp_int_radio_group_new (FALSE, NULL,
				   G_CALLBACK (gimp_radio_button_update),
				   &mail_info.encapsulation,
				   mail_info.encapsulation,

				   _("_Uuencode"), ENCAPSULATION_UUENCODE, NULL,
				   _("_MIME"),     ENCAPSULATION_MIME,     NULL,

				   NULL);
  gtk_table_attach (GTK_TABLE (table), vbox, 1, 2, 6, 8,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vbox);

  gtk_widget_show (dlg);

  run = (gimp_dialog_run (GIMP_DIALOG (dlg)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dlg);

  return run;
}

static gint
valid_file (const gchar *filename)
{
  int stat_res;
  struct stat buf;

  stat_res = stat (filename, &buf);

  if ((0 == stat_res) && (buf.st_size > 0))
    return 1;
  else
    return 0;
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
    "ps", "application/postscript",
    "eps", "application/postscript",
    NULL, NULL
  };

  gchar *ext;
  gchar *mimetype = g_new (gchar, 100);
  gint i=0;

  ext = find_extension (filename);
  if (!ext)
    {
      strcpy (mimetype, "application/octet-stream");
      return mimetype;
    }

  while (type_mappings[i])
    {
      if (strcmp (ext+1, type_mappings[i]) == 0)
	{
	  strcpy (mimetype, type_mappings[i + 1]);
	  return mimetype;
	}
      i += 2;
    }
  strcpy (mimetype, "image/x-");
  strncat (mimetype, ext + 1, 91);
  mimetype[99] = '\0';

  return mimetype;
}

static gchar *
find_extension (const gchar *filename)
{
  gchar *filename_copy;
  gchar *ext;

  /* this whole routine needs to be redone so it works for xccfgz and gz files*/
  /* not real sure where to start......                                       */
  /* right now saving for .xcfgz works but not .xcf.gz                        */
  /* this is all pretty close to straight from gz. It needs to be changed to  */
  /* work better for this plugin                                              */
  /* ie, FIXME */

  /* we never free this copy - aren't we evil! */
  filename_copy = malloc (strlen (filename) + 1);
  strcpy (filename_copy, filename);

  /* find the extension, boy! */
  ext = strrchr (filename_copy, '.');

  while (1)
    {
      if (!ext || ext[1] == 0 || strchr (ext, '/'))
	{
	  g_message (_("some sort of error with the file extension or lack thereof"));

	  return NULL;
	}
      if (0 != strcmp(ext,".gz"))
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
		     gpointer   data)
{
  strncpy ((gchar *) data, gtk_entry_get_text (GTK_ENTRY (widget)), BUFFER_SIZE);
}

static void
mesg_body_callback (GtkTextBuffer *buffer,
		    gpointer       data)
{
  GtkTextIter start_iter;
  GtkTextIter end_iter;


  if (mesg_body)
    g_free (mesg_body);

  gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
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
  fprintf (mailpipe, "From: %s \n", mail_info.from);
  fprintf (mailpipe, "X-Mailer: GIMP Useless Mail Program v.85\n");

  if (mail_info.encapsulation == ENCAPSULATION_MIME )
    {
      fprintf (mailpipe, "MIME-Version: 1.0\n");
      fprintf (mailpipe, "Content-type: multipart/mixed; boundary=GUMP-MIME-boundary\n");
    }
  fprintf (mailpipe, "\n\n");
  if (mail_info.encapsulation == ENCAPSULATION_MIME )
    {
      fprintf (mailpipe, "--GUMP-MIME-boundary\n");
      fprintf (mailpipe, "Content-type: text/plain; charset=US-ASCII\n\n");
    }
  fprintf (mailpipe, mail_info.comment);
  fprintf (mailpipe, "\n\n");
  if (mesg_body)
    fprintf (mailpipe, mesg_body);
  fprintf (mailpipe, "\n\n");
  if (mail_info.encapsulation == ENCAPSULATION_MIME )
    {
      gchar *content = find_content_type (mail_info.filename);

      fprintf (mailpipe, "--GUMP-MIME-boundary\n");
      fprintf (mailpipe, "Content-type: %s\n",content);
      fprintf (mailpipe, "Content-transfer-encoding: base64\n");
      fprintf (mailpipe, "Content-disposition: attachment; filename=\"%s\"\n",
               mail_info.filename);
      fprintf (mailpipe, "Content-description: %s\n\n",mail_info.filename);

      g_free (content);
    }
}

/*
 * The following code taken from codes.c in the mpack-1.5 distribution
 * by Carnegie Mellon University.
 *
 *
 * (C) Copyright 1993,1994 by Carnegie Mellon University
 * All Rights Reserved.
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
/*
Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)

Permission to use, copy, modify, and distribute this material
for any purpose and without fee is hereby granted, provided
that the above copyright notice and this permission notice
appear in all copies, and that the name of Bellcore not be
used in advertising or publicity pertaining to this
material without the specific, prior written permission
of an authorized representative of Bellcore.  BELLCORE
MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY
OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS",
WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.  */


static gchar basis_64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static gint
to64 (FILE *infile,
      FILE *outfile)
{
  gint c1, c2, c3, ct = 0, written = 0;

  while ((c1 = getc (infile)) != EOF)
    {
      c2 = getc (infile);
      if (c2 == EOF)
	{
	  output64chunk (c1, 0, 0, 2, outfile);
        }
      else
	{
	  c3 = getc (infile);
	  if (c3 == EOF)
	    {
	      output64chunk (c1, c2, 0, 1, outfile);
            }
	  else
	    {
	      output64chunk (c1, c2, c3, 0, outfile);
            }
        }
      ct += 4;
      if (ct > 71)
	{
	  putc ('\n', outfile);
	  written += 73;
	  ct = 0;
        }
    }
  if (ct)
    {
      putc ('\n', outfile);
      ct++;
    }

  return written + ct;
}

static void
output64chunk (gint  c1,
	       gint  c2,
	       gint  c3,
	       gint  pads,
	       FILE *outfile)
{
  putc (basis_64[c1>>2], outfile);
  putc (basis_64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)], outfile);

  if (pads == 2)
    {
      putc ('=', outfile);
      putc ('=', outfile);
    }
  else if (pads)
    {
      putc (basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)], outfile);
      putc ('=', outfile);
    }
  else
    {
      putc (basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)], outfile);
      putc (basis_64[c3 & 0x3F], outfile);
    }
}
