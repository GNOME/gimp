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
 *   This works fine for .99.10. I havent actually tried it in combination with
 *   the gz plugin, but it works with all other file types. I will eventually get
 *   around to making sure it works with gz.
 *
 *  To use: 1) image->File->mail image
 *          2) when the mail dialog popups up, fill it out. Only to: and filename are required
 *             note: the filename needs to a type that the image can be saved as. otherwise
 *                   you will just send an empty message.
 *          3) click ok and it should be on its way
 *
 *
 * NOTE: You probabaly need sendmail installed. If your sendmail is in an odd spot
 *       you can change the #define below. If you use qmail or other MTA's, and this
 *       works after changing the MAILER, let me know how well or what changes were
 *       needed.
 *
 * NOTE: Uuencode is needed. If it is in the path, it should work fine as is. Other-
 *       wise just change the UUENCODE.
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
 *  Version history
 *       .5  - 6/30/97 - inital relese
 *       .51 - 7/3/97  - fixed a few spelling errors and the like
 *       .65 - 7/4/97  - a fairly significant revision. changed it from a file
 *                       plugin to an image plugin.
 *                     - Changed some strcats into strcpy to be a bit more robust.
 *                     - added the abilty to specify the filename you want it sent as
 *                     - no more annoying hassles with the file saves as dialog
 *                     - plugin now registers itself as <image>/File/Mail image
 *       .7  - 9/12/97 - (RB) added support for MIME encapsulation
 *       .71 - 9/17/97 - (RB) included Base64 encoding functions from mpack
 *                       instead of using external program.
 *                     - General cleanup of the MIME handling code.
 *       .80 - 6/23/98 - Added a text box so you can compose real messages.
 *       .85 - 3/19/99 - Added a "From:" field. Made it check gimprc for a
 *                       "gump-from" token and use it. Also made "run with last 
 *                        values" work.
 * As always: The utility of this plugin is left as an exercise for the reader
 *
 */
#ifndef MAILER
#define MAILER "/usr/lib/sendmail"
#endif

#ifndef UUENCODE
#define UUENCODE "uuencode"
#endif

#define ENCAPSULATION_UUENCODE 0
#define ENCAPSULATION_MIME     1

#define BUFFER_SIZE            256

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
#ifdef __EMX__
#include <fcntl.h>
#include <process.h>
#endif

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


static void   query (void);
static void   run   (gchar   *name,
		     gint     nparams,
		     GimpParam  *param,
		     gint    *nreturn_vals,
		     GimpParam **return_vals);

static GimpPDBStatusType save_image (gchar  *filename,
			       gint32  image_ID,
			       gint32  drawable_ID,
			       gint32  run_mode);

static gint   save_dialog          (void);
static void   ok_callback          (GtkWidget *widget,
				    gpointer   data);
static void   mail_entry_callback  (GtkWidget *widget,
				    gpointer   data);
static void   mesg_body_callback   (GtkWidget *widget,
				    gpointer   data);

static gint   valid_file     (gchar *filename);
static void   create_headers (FILE  *mailpipe);
static char * find_extension (gchar *filename);
static gint   to64           (FILE  *infile,
			      FILE  *outfile);
static void   output64chunk  (gint   c1,
			      gint   c2,
			      gint   c3,
			      gint   pads,
			      FILE  *outfile);

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

static gchar * mesg_body = "\0";
static gint    run_flag  = 0;

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
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_mail_image",
			  "pipe files to uuencode then mail them",
			  "You need to have uuencode and mail installed",
			  "Adrian Likins, Reagan Blundell",
			  "Adrian Likins, Reagan Blundell, Daniel Risacher, Spencer Kimball and Peter Mattis",
			  "1995-1997",
			  N_("<Image>/File/Mail Image..."),
			  "RGB*, GRAY*, INDEXED*",
			  GIMP_PLUGIN,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GimpParam  *param,
     gint    *nreturn_vals,
     GimpParam **return_vals)
{
  static GimpParam values[2];
  GimpRunModeType  run_mode;
  GimpPDBStatusType   status = GIMP_PDB_SUCCESS;
  gint32        image_ID;
  gint32        drawable_ID;

  INIT_I18N_UI();

  run_mode    = param[0].data.d_int32;
  image_ID    = param[1].data.d_image;
  drawable_ID = param[2].data.d_drawable;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, "plug_in_mail_image") == 0)
    {
      switch (run_mode)
	{
	case GIMP_RUN_INTERACTIVE:
	  gimp_get_data ("plug_in_mail_image", &mail_info);
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
	  gimp_get_data ("plug_in_mail_image", &mail_info);
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
	    gimp_set_data ("plug_in_mail_image", &mail_info, sizeof(m_info));
	}
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static GimpPDBStatusType
save_image (gchar  *filename,
	    gint32  image_ID,
	    gint32  drawable_ID,
	    gint32  run_mode)
{
  GimpParam *params;
  gint    retvals;
  gchar  *ext;
  gchar  *tmpname;
  gchar   mailcmdline[512];
  gint    pid;
  gint    status;
  gint    process_status;
  FILE   *mailpipe;
  FILE   *infile;

  if (NULL == (ext = find_extension (filename)))
    return GIMP_PDB_CALLING_ERROR;

  /* get a temp name with the right extension and save into it. */
  params = gimp_run_procedure ("gimp_temp_name",
			       &retvals,
			       GIMP_PDB_STRING, ext + 1,
			       GIMP_PDB_END);

  tmpname = g_strdup (params[1].data.d_string);
  gimp_destroy_params (params, retvals);

  /* construct the "sendmail user@location" line */
  strcpy (mailcmdline, MAILER);
  strcat (mailcmdline, " ");
  strcat (mailcmdline, mail_info.receipt);

  /* create a pipe to sendmail */
#ifndef __EMX__
  mailpipe = popen (mailcmdline, "w");
#else
  mailpipe = popen (mailcmdline, "wb");
#endif
  create_headers (mailpipe);
  
  /* This is necessary to make the comments and headers work correctly. Not real sure why */
  fflush (mailpipe);      

  params = gimp_run_procedure ("gimp_file_save",
			       &retvals,
			       GIMP_PDB_INT32, run_mode,
			       GIMP_PDB_IMAGE, image_ID,
			       GIMP_PDB_DRAWABLE, drawable_ID,
			       GIMP_PDB_STRING, tmpname,
			       GIMP_PDB_STRING, tmpname,
			       GIMP_PDB_END);
  

  /*  need to figure a way to make sure the user is trying to save
   *  in an approriate format but this can wait....
   */

  status = params[0].data.d_status;

  if (! valid_file (tmpname) ||
      status != GIMP_PDB_SUCCESS)
    {
      unlink (tmpname);
      g_free (tmpname);
      return status;
    }

  if (mail_info.encapsulation == ENCAPSULATION_UUENCODE)
    {
#ifndef __EMX__
      /* fork off a uuencode process */
      if ((pid = fork ()) < 0)
	{
	  g_message ("mail: fork failed: %s\n", g_strerror (errno));
	  g_free (tmpname);
	  return GIMP_PDB_EXECUTION_ERROR;
	}
      else if (pid == 0)
	{
	  if (-1 == dup2 (fileno (mailpipe), fileno (stdout)))
	    {
	      g_message ("mail: dup2 failed: %s\n", g_strerror (errno));
	    }

	  execlp (UUENCODE, UUENCODE, tmpname, filename, NULL);
	  /* What are we doing here? exec must have failed */
	  g_message ("mail: exec failed: uuencode: %s\n", g_strerror (errno));

	  /* close the pipe now */
	  pclose (mailpipe);
	  g_free (tmpname);
	  _exit (127);
	}
      else
#else /* __EMX__ */
      int tfd;
      /* save fileno(stdout) */
      tfd = dup (fileno (stdout));
      if (dup2 (fileno (mailpipe), fileno (stdout)) == -1)
	{
	  g_message ("mail: dup2 failed: %s\n", g_strerror (errno));
	  close (tfd);
	  g_free (tmpname);
	  return GIMP_PDB_EXECUTION_ERROR;
	}
      fcntl (tfd, F_SETFD, FD_CLOEXEC);
      pid = spawnlp (P_NOWAIT, UUENCODE, UUENCODE, tmpname, filename, NULL);
      /* restore fileno(stdout) */
      dup2 (tfd, fileno (stdout));
      close (tfd);
      if (pid == -1)
	{
	  g_message ("mail: spawn failed: %s\n", g_strerror (errno));
	  g_free (tmpname);
	  return GIMP_PDB_EXECUTION_ERROR;
	}
#endif
        {
	  waitpid (pid, &process_status, 0);

	  if (!WIFEXITED (process_status) ||
	      WEXITSTATUS (process_status) != 0)
	    {
	      g_message ("mail: mail didnt work or something on file %s\n", tmpname);
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
  GtkWidget *dlg;
  GtkWidget *entry;
  GtkWidget *table;
  GtkWidget *table2;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *text;
  GtkWidget *vscrollbar;

  gchar      buffer[BUFFER_SIZE];
  gchar     *gump_from;

  gimp_ui_init ("mail", FALSE);

  /* check gimprc for a preffered "From:" address */
  gump_from = gimp_gimprc_query ("gump-from");         

  if (gump_from)
    {
      strncpy (mail_info.from, gump_from, BUFFER_SIZE);
      g_free (gump_from);
    }

  dlg = gimp_dialog_new (_("Send to Mail"), "mail",
			 gimp_standard_help_func, "filters/mail.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("OK"), ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* table */
  table = gtk_table_new (7, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);

  /* to: dialog */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.receipt);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Recipient:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (mail_entry_callback),
		      &mail_info.receipt);

  /* From entry */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.from);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Sender:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (mail_entry_callback),
		      &mail_info.from);

  /* Subject entry */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.subject);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
			     _("Subject:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (mail_entry_callback),
		      &mail_info.subject);

  /* Comment entry */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.comment);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 3,
			     _("Comment:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (mail_entry_callback),
		      &mail_info.comment);

  /* Filename entry */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  g_snprintf (buffer, sizeof (buffer), "%s", mail_info.filename);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 4,
			     _("Filename:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (mail_entry_callback),
		      &mail_info.filename);

  /* comment  */
  table2 = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacing (GTK_TABLE (table2), 0, 2);
  gtk_table_set_col_spacing (GTK_TABLE (table2), 0, 2);

  gtk_table_attach (GTK_TABLE (table), table2, 
		    0, 2, 5, 6,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);

  text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (text), TRUE);
  gtk_table_attach (GTK_TABLE (table2), text, 
		    0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_set_usize (text, 200, 100);
  gtk_widget_show (text);
  gtk_signal_connect (GTK_OBJECT (text), "changed",
		      GTK_SIGNAL_FUNC (mesg_body_callback),
		      mesg_body);
  gtk_widget_show (table2);

  vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
  gtk_table_attach (GTK_TABLE (table2), vscrollbar, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vscrollbar);

  /* Encapsulation label */
  label = gtk_label_new (_("Encapsulation:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 6, 7,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /* Encapsulation radiobuttons */
  vbox = gimp_radio_group_new2 (FALSE, NULL,
				gimp_radio_button_update,
				&mail_info.encapsulation,
				(gpointer) mail_info.encapsulation,

				_("Uuencode"),
				(gpointer) ENCAPSULATION_UUENCODE, NULL,
				_("MIME"),
				(gpointer) ENCAPSULATION_MIME, NULL,

				NULL);
  gtk_table_attach (GTK_TABLE (table), vbox, 1, 2, 6, 8,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vbox);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}

static gint 
valid_file (gchar *filename)
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
find_content_type (gchar *filename)
{
  /* This function returns a MIME Content-type: value based on the
     filename it is given.  */
  gchar *type_mappings[20] =
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
  gchar *mimetype = malloc(100);
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
find_extension (gchar *filename)
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
	  g_message (_("mail: some sort of error with the file extension or lack thereof \n"));
	  
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
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  run_flag = TRUE;
  
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
mail_entry_callback (GtkWidget *widget,
		     gpointer   data)
{
  strncpy ((gchar *) data, gtk_entry_get_text (GTK_ENTRY (widget)), BUFFER_SIZE);
}

static void 
mesg_body_callback (GtkWidget *widget,
		    gpointer   data)
{
  mesg_body = gtk_editable_get_chars (GTK_EDITABLE (widget), 0,
				      gtk_text_get_length (GTK_TEXT (widget)));
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
  fprintf (mailpipe, mesg_body); 
  fprintf (mailpipe, "\n\n");
  if (mail_info.encapsulation == ENCAPSULATION_MIME )
    {
      char *content;
      content = find_content_type (mail_info.filename);
      fprintf (mailpipe, "--GUMP-MIME-boundary\n");
      fprintf (mailpipe, "Content-type: %s\n",content);
      fprintf (mailpipe, "Content-transfer-encoding: base64\n");
      fprintf (mailpipe, "Content-disposition: attachment; filename=\"%s\"\n",mail_info.filename);
      fprintf (mailpipe, "Content-description: %s\n\n",mail_info.filename);
      free(content);
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
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Carnegie
 * Mellon University not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  Carnegie Mellon University makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
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
