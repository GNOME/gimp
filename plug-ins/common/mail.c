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
#include "gtk/gtk.h"
#include "libgimp/gimp.h"

#include "config.h"
#include "libgimp/stdplugins-intl.h"

static void query (void);
static void run (char *name,
		 int nparams,
		 GParam * param,
		 int *nreturn_vals,
		 GParam ** return_vals);


static gint save_image (char *filename,
			gint32 image_ID,
			gint32 drawable_ID,
			gint32 run_mode);

static gint save_dialog ();
static void close_callback (GtkWidget * widget, gpointer data);
static void ok_callback (GtkWidget * widget, gpointer data);
static void encap_callback (GtkWidget * widget, gpointer data);
static void receipt_callback (GtkWidget * widget, gpointer data);
static void from_callback (GtkWidget * widget, gpointer data);
static void subject_callback (GtkWidget * widget, gpointer data);
static void comment_callback (GtkWidget * widget, gpointer data);
static void filename_callback (GtkWidget * widget, gpointer data);
static void mesg_body_callback (GtkWidget * widget, gpointer data);
static int valid_file (char *filename);
static void create_headers (FILE * mailpipe);
static char *find_extension (char *filename);
static int to64(FILE *infile, FILE *outfile);
static void output64chunk(int c1, int c2, int c3, int pads, FILE *outfile);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,				/* init_proc */
  NULL,				/* quit_proc */
  query,			/* query_proc */
  run,				/* run_proc */
};


typedef struct
  {
    char receipt[256];
    char subject[256];
    char comment[256];
    char from[256];
    char filename[256];
    int  encapsulation;
  }
m_info;

static m_info mail_info = {
  /* I would a assume there is a better way to do this, but this works for now */
  "\0",
  "\0",
  "\0",
  "\0",
  "\0",
  ENCAPSULATION_MIME,  /* Change this to ENCAPSULATION_UUENCODE
			  if you prefer that as the default */
};

static gchar * mesg_body;
static int run_flag = 0;

MAIN ()

static void
query ()
{

  static GParamDef args[] =
  {
    {PARAM_INT32, "run_mode", "Interactive, non-interactive"},
    {PARAM_IMAGE, "image", "Input image"},
    {PARAM_DRAWABLE, "drawable", "Drawable to save"},
    {PARAM_STRING, "filename", "The name of the file to save the image in"},
    {PARAM_STRING, "receipt", "The email address to send to"},
    {PARAM_STRING, "from", "The email address for the From: field"},
    {PARAM_STRING, "subject", "The subject"},
    {PARAM_STRING, "comment", "The Comment"},
    {PARAM_INT32,  "encapsulation", "Uuencode, MIME"},
  };
  static int nargs = sizeof (args) / sizeof (args[0]);
  static GParamDef *return_vals = NULL;
  static int nreturn_vals = 0;

  INIT_I18N();

  gimp_install_procedure ("plug_in_mail_image",
			  _("pipe files to uuencode then mail them"),
			  _("You need to have uuencode and mail installed"),
			  "Adrian Likins, Reagan Blundell",
			  "Adrian Likins, Reagan Blundell, Daniel Risacher, Spencer Kimball and Peter Mattis",
			  "1995-1997",
			  _("<Image>/File/Mail image"),
			  "RGB*, GRAY*, INDEXED*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);

}


static void
run (char *name,
     int nparams,
     GParam * param,
     int *nreturn_vals,
     GParam ** return_vals)
{
  static GParam values[2];
  GRunModeType run_mode;
  gint32 drawable_ID;
  GStatusType status = STATUS_SUCCESS;
  gint32 image_ID;

  INIT_I18N();

  run_mode = param[0].data.d_int32;
  drawable_ID = param[2].data.d_drawable;
  image_ID = param[1].data.d_image;
  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  if (strcmp (name, "plug_in_mail_image") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  gimp_get_data ("plug_in_mail_image", &mail_info);
	  if (!save_dialog ())
	    return;
	  break;
	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 9)
	    status = STATUS_CALLING_ERROR;
	  if(status == STATUS_SUCCESS)
	    {
	      /* this hasnt been tested yet */
	      strncpy (mail_info.filename, param[3].data.d_string,256);
	      strncpy (mail_info.receipt, param[4].data.d_string,256);
	      strncpy (mail_info.receipt, param[5].data.d_string,256);
	      strncpy (mail_info.subject, param[6].data.d_string,256);
	      strncpy (mail_info.comment, param[7].data.d_string,256);
	      mail_info.encapsulation = param[8].data.d_int32;
	    }
	  break;
	case RUN_WITH_LAST_VALS:
	  gimp_get_data ("plug_in_mail_image", &mail_info);
	  break;
	  
	default:
	  break;
	}

      if (run_mode == RUN_INTERACTIVE)
	gimp_set_data ("plug_in_mail_image", &mail_info, sizeof(m_info));

      *nreturn_vals = 1;
      if (save_image (mail_info.filename,
		      image_ID,
		      drawable_ID,
		      run_mode))
	{
	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
  else
    g_assert (FALSE);
}

static gint
save_image (char *filename,
	    gint32 image_ID,
	    gint32 drawable_ID,
	    gint32 run_mode)
{

  GParam *params;
  gint retvals;
  char *ext;
  char *tmpname;
  char mailcmdline[512];
  int pid;
  int status;
  FILE *mailpipe;
  FILE *infile;

  if (NULL == (ext = find_extension (filename)))
    return -1;

  /* get a temp name with the right extension and save into it. */
  params = gimp_run_procedure ("gimp_temp_name",
			       &retvals,
			       PARAM_STRING, ext + 1,
			       PARAM_END);
  tmpname = params[1].data.d_string;

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
			       PARAM_INT32, run_mode,
			       PARAM_IMAGE, image_ID,
			       PARAM_DRAWABLE, drawable_ID,
			       PARAM_STRING, tmpname,
			       PARAM_STRING, tmpname,
			       PARAM_END);

  

  /* need to figure a way to make sure the user is trying to save in an approriate format */
  /* but this can wait....                                                                */

  if (params[0].data.d_status == FALSE || !valid_file (tmpname))
    {
      unlink (tmpname);
      return -1;
    }

  if( mail_info.encapsulation == ENCAPSULATION_UUENCODE ) {
#ifndef __EMX__
      /* fork off a uuencode process */
      if ((pid = fork ()) < 0)
	  {
	      g_message ("mail: fork failed: %s\n", g_strerror (errno));
	      return -1;
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
	  close(tfd);
	  return -1;
	}
      fcntl(tfd, F_SETFD, FD_CLOEXEC);
      pid = spawnlp (P_NOWAIT, UUENCODE, UUENCODE, tmpname, filename, NULL);
      /* restore fileno(stdout) */
      dup2 (tfd, fileno (stdout));
      close(tfd);
      if (pid == -1)
	{
	  g_message ("mail: spawn failed: %s\n", g_strerror (errno));
	  return -1;
	}
#endif
	  {
	      waitpid (pid, &status, 0);
	      
	      if (!WIFEXITED (status) ||
		  WEXITSTATUS (status) != 0)
		  {
		      g_message ("mail: mail didnt work or something on file %s\n", tmpname);
		      return 0;
		  }
	  }
  }
  else {  /* This must be MIME stuff. Base64 away... */
      infile = fopen(tmpname,"r");
      to64(infile,mailpipe);
      /* close off mime */
      if( mail_info.encapsulation == ENCAPSULATION_MIME ) {
	  fprintf(mailpipe, "\n--GUMP-MIME-boundary--\n");
      }
  }
  /* delete the tmpfile that was generated */
  unlink (tmpname);
  return TRUE;
}


static gint
save_dialog ()
{

  /* argh, guess this all needs to be struct that i can pass to the ok_callback
     so i can get the text from it then.  Seems a bit ugly, but maybe its better.
     I dunno.  */
  GtkWidget *dlg;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *table;
  GtkWidget *table2;
  GtkWidget *label;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *text;
  GtkWidget *vscrollbar;
  GSList *group;

  gint argc;
  gchar **argv;
  gchar buffer[32];
  gint nreturn_vals;
  GParam *return_vals;
  
  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("mail");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  /* check gimprc for a preffered "From:" address */
  return_vals = gimp_run_procedure ("gimp_gimprc_query",
                                    &nreturn_vals,
                                    PARAM_STRING, "gump-from",
                                    PARAM_END);         

  /* check to see if we actually got a value */
  if (return_vals[0].data.d_status == STATUS_SUCCESS &&
      return_vals[1].data.d_string != NULL)
    strncpy (mail_info.from, return_vals[1].data.d_string , 256);

  gimp_destroy_params (return_vals, nreturn_vals);
  
  dlg = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dlg), _("Send to mail"));
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) close_callback, NULL);
  /* action area   */
  /* Okay buton */
  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);


  /* cancel button */
  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_widget_show (button);

  /* table */
  table = gtk_table_new (7, 3, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 10);
  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  /*  To:  Label */
  label = gtk_label_new ("To:");
  gtk_table_attach (GTK_TABLE (table), label,
		    0, 1, 0, 1,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_show (label);

  /* to: dialog */
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry,
		    1, 3, 0, 1, 
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_set_usize (entry, 200, 0);
  sprintf (buffer, "%s", mail_info.receipt);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) receipt_callback, &mail_info.receipt);
  gtk_widget_show (entry);



  /*  From:  Label */
  label = gtk_label_new ("From:");
  gtk_table_attach (GTK_TABLE (table), label,
		    0, 1, 1, 2,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_show (label);

  /* from: dialog */
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry,
		    1, 3, 1, 2, 
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_set_usize (entry, 200, 0);
  
  sprintf (buffer, "%s", mail_info.from);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) from_callback, &mail_info.from);
  gtk_widget_show (entry);




  /*  subject Label */
  label = gtk_label_new ("Subject:");
  gtk_table_attach (GTK_TABLE (table), label,
		    0, 1, 2, 3,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_show (label);

  /* Subject entry */
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry,
		    1, 3, 2, 3,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_set_usize (entry, 200, 0);
  sprintf (buffer, "%s", mail_info.subject);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) subject_callback, &mail_info.subject);
  gtk_widget_show (entry);


  /* Comment label  */
  label = gtk_label_new ("Comment:");
  gtk_table_attach (GTK_TABLE (table), label, 
		    0, 1, 3, 4, 
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_show (label);

  /* Comment dialog */
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 
		    1, 3, 3, 4,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_set_usize (entry, 200, 0);
  sprintf (buffer, "%s", mail_info.comment);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) comment_callback, &mail_info.comment);
  gtk_widget_show (entry);


  /* filename label  */
  label = gtk_label_new (_("Filename:"));
  gtk_table_attach (GTK_TABLE (table), label, 
		    0, 1, 4, 5, 
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_show (label);

  /* Filename dialog */
  entry = gtk_entry_new ();
  gtk_table_attach (GTK_TABLE (table), entry, 
		    1, 3, 4, 5,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_set_usize (entry, 200, 0);
  sprintf (buffer, "%s", mail_info.filename);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) filename_callback, &mail_info.filename);
  gtk_widget_show (entry);

  /* comment  */
    table2 = gtk_table_new (2, 2, FALSE);
   gtk_table_set_row_spacing (GTK_TABLE (table2), 0, 2);
   gtk_table_set_col_spacing (GTK_TABLE (table2), 0, 2);
   /*   gtk_box_pack_start (GTK_BOX (box2), table2, TRUE, TRUE, 0);
	gtk_widget_show (table2); */
   gtk_table_attach (GTK_TABLE (table), table2, 
		    0, 3, 5, 6,
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
		      (GtkSignalFunc) mesg_body_callback, mesg_body);
  gtk_widget_show (table2);

  vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
  gtk_table_attach (GTK_TABLE (table2), vscrollbar, 1, 2, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (vscrollbar);
  

  /* Encapsulation label */
  label = gtk_label_new (_("Encapsulation:"));
  gtk_table_attach( GTK_TABLE (table), label ,
		    0, 1, 7, 8,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0);
  gtk_widget_show(label);

  /* Encapsulation radiobutton */
  button1 = gtk_radio_button_new_with_label( NULL, _("Uuencode"));
  group = gtk_radio_button_group( GTK_RADIO_BUTTON( button1 ) );
  button2 = gtk_radio_button_new_with_label( group, _("MIME" ));
  if( mail_info.encapsulation == ENCAPSULATION_UUENCODE ) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button1),TRUE);
  } else {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button2),TRUE);
  }
  gtk_signal_connect (GTK_OBJECT (button1), "toggled",
		      (GtkSignalFunc) encap_callback,
		      (gpointer) "uuencode" );
  gtk_signal_connect (GTK_OBJECT (button2), "toggled",
		      (GtkSignalFunc) encap_callback,
		      (gpointer) "mime" );

  gtk_table_attach( GTK_TABLE (table), button1,
		    1, 2, 6, 7,
		    GTK_EXPAND | GTK_FILL, 
		    GTK_EXPAND | GTK_FILL,
		    0, 0 );
  gtk_widget_show( button1 );

  gtk_table_attach( GTK_TABLE (table), button2,
		    2, 3, 6, 7,
		    GTK_EXPAND | GTK_FILL,
		    GTK_EXPAND | GTK_FILL,
		    0, 0 );
  gtk_widget_show( button2 );


  gtk_widget_show (dlg);
  gtk_main ();
  gdk_flush ();
  return run_flag;

}

static int 
valid_file (char *filename)
{
  int stat_res;
  struct stat buf;

  stat_res = stat (filename, &buf);

  if ((0 == stat_res) && (buf.st_size > 0))
    return 1;
  else
    return 0;
}

char *
find_content_type (char *filename)
{
    /* This function returns a MIME Content-type: value based on the
       filename it is given.  */
    char *type_mappings[20] = {"gif" , "image/gif",
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

    char *ext;
    char *mimetype = malloc(100);
    int i=0;
    ext = find_extension(filename);
    if(!ext) {
	strcpy( mimetype, "application/octet-stream");
	return mimetype;
    }
    
    while( type_mappings[i] ) {
	if( strcmp( ext+1, type_mappings[i] ) == 0 ) {
	    strcpy(mimetype,type_mappings[i+1]);
	    return mimetype;
	}
	i += 2;
    }
    strcpy(mimetype,"image/x-");
    strncat(mimetype,ext+1,91);
    mimetype[99]='\0';
    return mimetype;

}

static char *
find_extension (char *filename)
{
  char *filename_copy;
  char *ext;

  /* this whole routine needs to be redone so it works for xccfgz and .gz files */
  /* not real sure where to start......                                         */
  /* right now saving for .xcfgz works but not .xcf.gz                          */
  /* this is all pretty close to straight from gz. It needs to be changed to    */
  /* work better for this plugin                                                */
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
close_callback (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static void
ok_callback (GtkWidget * widget, gpointer data)
{
  run_flag = 1;
  
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
encap_callback (GtkWidget * widget, gpointer data)
{
    /* Ignore the toggle-off signal, we are only interested in
       what is being set */
    if( ! GTK_TOGGLE_BUTTON( widget )->active ) {
	return;
    }
    if(strcmp(data,"uuencode")==0)
	mail_info.encapsulation = ENCAPSULATION_UUENCODE;
    if(strcmp(data,"mime")==0)
	mail_info.encapsulation = ENCAPSULATION_MIME;
}

static void
receipt_callback (GtkWidget * widget, gpointer data)
{
  strncpy (mail_info.receipt, gtk_entry_get_text (GTK_ENTRY (widget)), 256);
}

static void
from_callback (GtkWidget *widget, gpointer data)
{
  strncpy (mail_info.from, gtk_entry_get_text (GTK_ENTRY (widget)), 256);
}

static void
subject_callback (GtkWidget * widget, gpointer data)
{
  strncpy (mail_info.subject, gtk_entry_get_text (GTK_ENTRY (widget)), 256);
}


static void
comment_callback (GtkWidget * widget, gpointer data)
{
  strncpy (mail_info.comment, gtk_entry_get_text (GTK_ENTRY (widget)), 256);
}


static void
filename_callback (GtkWidget * widget, gpointer data)
{
  strncpy (mail_info.filename, gtk_entry_get_text (GTK_ENTRY (widget)), 256);
}

static void 
mesg_body_callback (GtkWidget * widget, gpointer data)
{
  mesg_body = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, gtk_text_get_length(GTK_TEXT(widget)));
} 

static void
create_headers (FILE * mailpipe)
{
  /* create all the mail header stuff. Feel free to add your own */
  /* It is advisable to leave the X-Mailer header though, as     */
  /* there is a possibilty of a Gimp mail scanner/reader in the  */
  /* future. It will probabaly need that header.                 */

  fprintf (mailpipe, "To: %s \n", mail_info.receipt);
  fprintf (mailpipe, "Subject: %s \n", mail_info.subject);
  fprintf (mailpipe, "From: %s \n", mail_info.from);
  fprintf (mailpipe, "X-Mailer: GIMP Useless Mail Program v.85\n");


  if(mail_info.encapsulation == ENCAPSULATION_MIME ){
      fprintf (mailpipe, "MIME-Version: 1.0\n");
      fprintf (mailpipe, "Content-type: multipart/mixed; boundary=GUMP-MIME-boundary\n");
  }
  fprintf (mailpipe, "\n\n");
  if(mail_info.encapsulation == ENCAPSULATION_MIME ) {
      fprintf (mailpipe, "--GUMP-MIME-boundary\n");
      fprintf (mailpipe, "Content-type: text/plain; charset=US-ASCII\n\n");
  }
  fprintf (mailpipe, mail_info.comment);
  fprintf (mailpipe, "\n\n");
  fprintf (mailpipe, mesg_body); 
  fprintf (mailpipe, "\n\n");
  if(mail_info.encapsulation == ENCAPSULATION_MIME ) {
      char *content;
      content=find_content_type(mail_info.filename);
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


static char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int to64(infile, outfile) 
FILE *infile, *outfile;
{
    int c1, c2, c3, ct=0, written=0;

    while ((c1 = getc(infile)) != EOF) {
        c2 = getc(infile);
        if (c2 == EOF) {
            output64chunk(c1, 0, 0, 2, outfile);
        } else {
            c3 = getc(infile);
            if (c3 == EOF) {
                output64chunk(c1, c2, 0, 1, outfile);
            } else {
                output64chunk(c1, c2, c3, 0, outfile);
            }
        }
        ct += 4;
        if (ct > 71) {
            putc('\n', outfile);
	    written += 73;
            ct = 0;
        }
    }
    if (ct) {
	putc('\n', outfile);
	ct++;
    }
    return written + ct;
}

static void
output64chunk(c1, c2, c3, pads, outfile)
FILE *outfile;
{
    putc(basis_64[c1>>2], outfile);
    putc(basis_64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)], outfile);
    if (pads == 2) {
        putc('=', outfile);
        putc('=', outfile);
    } else if (pads) {
        putc(basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)], outfile);
        putc('=', outfile);
    } else {
        putc(basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)], outfile);
        putc(basis_64[c3 & 0x3F], outfile);
    }
}










