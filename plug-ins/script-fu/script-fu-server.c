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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "config.h"

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "script-fu-intl.h"

#include "gtk/gtk.h"
#include "siod.h"
#include "script-fu-server.h"

#define COMMAND_HEADER  3
#define RESPONSE_HEADER 4
#define MAGIC   'G'

#ifdef NO_DIFFTIME
#define difftime(a,b) (((double)(a)) - ((double)(b)))
#endif

#ifndef NO_FD_SET
#  define SELECT_MASK fd_set
#else
#  ifndef _AIX
     typedef long fd_mask;
#  endif
#  if defined(_IBMR2)
#    define SELECT_MASK void
#  else
#    define SELECT_MASK int
#  endif
#endif


/*  image information  */

/*  Header format for incoming commands...
 *    bytes: 1          2          3
 *           MAGIC      CMD_LEN_H  CMD_LEN_L
 */

/*  Header format for outgoing responses...
 *    bytes: 1          2          3          4
 *           MAGIC      ERROR?     RSP_LEN_H  RSP_LEN_L
 */

#define MAGIC_BYTE      0

#define CMD_LEN_H_BYTE  1
#define CMD_LEN_L_BYTE  2

#define ERROR           1
#define RSP_LEN_H_BYTE  2
#define RSP_LEN_L_BYTE  3

/*
 *  Local Structures
 */

typedef struct
{
  gchar *command;
  gint   filedes;
  gint   request_no;
} SFCommand;

typedef struct
{
  GtkWidget *port_entry;
  GtkWidget *log_entry;

  gint       port;
  gchar     *logfile;

  gint       run;
} ServerInterface;

/*
 *  Local Functions
 */

static void   server_start       (gint       port,
				  gchar     *logfile);
static gint   execute_command    (SFCommand *cmd);
static gint   read_from_client   (gint       filedes);
static gint   make_socket        (guint      port);
static void   server_log         (gchar     *format,
				     ...);
static void   server_quit        (void);

static gint   server_interface   (void);
static void   ok_callback        (GtkWidget *widget,
				  gpointer   data);

/*
 *  Global variables
 */
gint server_mode = FALSE;

/*
 *  Local variables
 */
static gint   server_sock;
static GList *command_queue = NULL;
static gint   queue_length = 0;
static gint   request_no = 0;
static FILE  *server_log_file = NULL;
static GHashTable *clientname_ht = NULL;
static SELECT_MASK server_active, server_read;

static ServerInterface sint =
{
  NULL,  /*  port entry widget  */
  NULL,  /*  log entry widget  */

  10008, /*  default port number  */
  NULL,  /*  use stdout  */

  FALSE  /*  run  */
};

extern gint   script_fu_done;
extern char   siod_err_msg[];
extern LISP   repl_return_val;

/*
 *  Server interface functions
 */

void
script_fu_server_run (char     *name,
		      int       nparams,
		      GimpParam   *params,
		      int      *nreturn_vals,
		      GimpParam  **return_vals)
{
  static GimpParam values[1];
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  GimpRunModeType run_mode;

  run_mode = params[0].data.d_int32;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (server_interface ())
	{
	  server_mode = TRUE;

	  /*  Start the server  */
	  server_start (sint.port, sint.logfile);
	}
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Set server_mode to TRUE  */
      server_mode = TRUE;

      /*  Start the server  */
      server_start (params[1].data.d_int32, params[2].data.d_string);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      status = GIMP_PDB_CALLING_ERROR;
      g_warning ("Script-Fu server does handle \"GIMP_RUN_WITH_LAST_VALS\"");

    default:
      break;
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = GIMP_PDB_STATUS;
  values[0].data.d_status = status;
}

void
script_fu_server_listen (gint timeout)
{
  struct sockaddr_in clientname;
  struct timeval tv;
  struct timeval *tvp;
  gint i;
  gint size;

  /*  Set time struct  */
  if (timeout)
    {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout % 1000;
      tvp = &tv;
    }
  else
    tvp = NULL;

  /* Block until input arrives on one or more active sockets or timeout occurs. */
  server_read = server_active;
  if (select (FD_SETSIZE, &server_read, NULL, NULL, tvp) < 0)
    {
      perror ("select");
      return;
    }

  /* Service all the sockets with input pending. */
  for (i = 0; i < FD_SETSIZE; ++i)
    if (FD_ISSET (i, &server_read))
      {
	if (i == server_sock)
	  {
	    /* Connection request on original socket. */
	    gint new;

	    size = sizeof (clientname);
	    new = accept (server_sock,
			  (struct sockaddr *) &clientname,
			  &size);
	    if (new < 0)
	      {
		perror ("accept");
		return;
	      }

	    /*  Associate the client address with the socket  */
	    g_hash_table_insert (clientname_ht,
				 (gpointer) new,
				 g_strdup (inet_ntoa (clientname.sin_addr)));
	    /*
	    server_log ("Server: connect from host %s, port %d.\n",
			inet_ntoa (clientname.sin_addr),
			(unsigned int) ntohs (clientname.sin_port));
			*/

	    FD_SET (new, &server_active);
	  }
	else
	  {
	    if (read_from_client (i) < 0)
	      {
		/*  Disassociate the client address with the socket  */
		g_hash_table_remove (clientname_ht, (gpointer) i);

		/*
		server_log ("Server: disconnect from host %s, port %d.\n",
			    inet_ntoa (clientname.sin_addr),
			    (unsigned int) ntohs (clientname.sin_port));
			    */

		close (i);
		FD_CLR (i, &server_active);
	      }
	  }
      }
}

static void
server_start (gint   port,
	      gchar *logfile)
{
  SFCommand *cmd;

  /*  Set up the clientname hash table  */
  clientname_ht = g_hash_table_new (g_direct_hash, NULL);

  /*  Setup up the server log file  */
  if (logfile)
    server_log_file = fopen (logfile, "a");
  else
    server_log_file = NULL;
  if (server_log_file == NULL)
    server_log_file = stdout;

  /* Create the socket and set it up to accept connections. */
  server_sock = make_socket (port);
  if (listen (server_sock, 5) < 0)
    {
      perror ("listen");
      return;
    }

  server_log ("Script-fu initialized and listening...\n");

  /* Initialize the set of active sockets. */
  FD_ZERO (&server_active);
  FD_SET (server_sock, &server_active);

  /*  Loop until the server is finished  */
  while (! script_fu_done)
    {
      script_fu_server_listen (0);

      while (command_queue)
	{
	  /*  Get the current command  */
	  cmd = (SFCommand *) command_queue->data;

	  /*  Process the command  */
	  execute_command (cmd);

	  /*  Remove the command from the list  */
	  command_queue = g_list_remove (command_queue, cmd);
	  queue_length--;

	  /*  Free the request  */
	  g_free (cmd->command);
	  g_free (cmd);
	}
    }

  server_quit ();

  /*  Close the server log file  */
  if (server_log_file != stdout)
    fclose (server_log_file);
}

static gint
execute_command (SFCommand *cmd)
{
  guchar buffer[RESPONSE_HEADER];
  gchar *response;
  time_t clock1, clock2;
  gint response_len;
  gint error;
  gint i;

  /*  Get the client address from the address/socket table  */
  server_log ("Processing request #%d\n", cmd->request_no);
  time (&clock1);

  /*  run the command  */
  if (repl_c_string (cmd->command, 0, 0, 1) != 0)
    {
      error = TRUE;
      response_len = strlen (siod_err_msg);
      response = siod_err_msg;

      server_log ("%s\n", siod_err_msg);
    }
  else
    {
      error = FALSE;

      if (TYPEP (repl_return_val, tc_string))
	response = get_c_string (repl_return_val);
      else
	response = "Success";

      response_len = strlen (response);

      time (&clock2);
      server_log ("Request #%d processed in %f seconds, finishing on %s",
		  cmd->request_no, difftime (clock2, clock1), ctime (&clock2));
    }

  buffer[MAGIC_BYTE] = MAGIC;
  buffer[ERROR] = (error) ? 1 : 0;
  buffer[RSP_LEN_H_BYTE] = (guchar) (response_len >> 8);
  buffer[RSP_LEN_L_BYTE] = (guchar) (response_len & 0xFF);

  /*  Write the response to the client  */
  for (i = 0; i < RESPONSE_HEADER; i++)
    if (write (cmd->filedes, buffer + i, 1) < 0)
      {
	/*  Write error  */
	perror ("write");
	return 0;
      }

  for (i = 0; i < response_len; i++)
    if (write (cmd->filedes, response + i, 1) < 0)
      {
	/*  Write error  */
	perror ("write");
	return 0;
      }

  return 0;
}

static gint
read_from_client (gint filedes)
{
  SFCommand *cmd;
  guchar buffer[COMMAND_HEADER];
  gchar *command;
  gchar *clientaddr;
  time_t clock;
  gint command_len;
  gint nbytes;
  gint i;

  for (i = 0; i < COMMAND_HEADER; i++)
    {
      if ((nbytes = read (filedes, buffer + i, 1)) < 0)
	{
	  /* Read error. */
	  perror ("read");
	  return 0;
	}
      else if (nbytes == 0)
	/* End-of-file. */
	return -1;
    }

  if (buffer[MAGIC_BYTE] != MAGIC)
    {
      server_log ("Error in script-fu command transmission.\n");
      return -1;
    }
  command_len = (buffer [CMD_LEN_H_BYTE] << 8) | buffer [CMD_LEN_L_BYTE];
  command = g_new (gchar, command_len + 1);

  for (i = 0; i < command_len; i++)
    if (read (filedes, command + i, 1) == 0)
      {
	server_log ("Error reading command.  Read %d out of %d bytes.\n", i, command_len);
	return -1;
      }

  command[command_len] = '\0';
  cmd = g_new (SFCommand, 1);
  cmd->filedes = filedes;
  cmd->command = command;
  cmd->request_no = request_no ++;

  /*  Add the command to the queue  */
  command_queue = g_list_append (command_queue, cmd);
  queue_length ++;

  /*  Get the client address from the address/socket table  */
  clientaddr = g_hash_table_lookup (clientname_ht, (gpointer) cmd->filedes);
  time (&clock);
  server_log ("Received request #%d from IP address %s: %s on %s, [Request queue length: %d]",
	      cmd->request_no, clientaddr, cmd->command, ctime (&clock), queue_length);

  return 0;
}

static gint
make_socket (guint port)
{
  gint sock;
  struct sockaddr_in name;
  gint v = 1;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      gimp_quit ();
    }
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      perror ("bind");
      gimp_quit ();
    }

  return sock;
}

static void
server_log (gchar *format, ...)
{
  va_list args;
  char *buf;

  va_start (args, format);
  buf = g_strdup_vprintf (format, args);
  va_end (args);

  fputs (buf, server_log_file);
  if (server_log_file != stdout)
    fflush (server_log_file);
}

static void
server_quit (void)
{
  int i;

  for (i = 0; i < FD_SETSIZE; ++i)
    if (FD_ISSET (i, &server_active))
      shutdown (i, 2);
}

static gint
server_interface (void)
{
  GtkWidget *dlg;
  GtkWidget *table;

  INIT_I18N_UI();

  gimp_ui_init ("script-fu", FALSE);

  dlg = gimp_dialog_new (_("Script-Fu Server Options"), "script-fu",
			 gimp_standard_help_func, "filters/script-fu.html", 
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

  /*  The table to hold port & logfile entries  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_container_border_width (GTK_CONTAINER (table), 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, TRUE, TRUE, 0);

  /*  The server port  */
  sint.port_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (sint.port_entry), "10008");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0, 
			     _("Server Port:"), 1.0, 0.5,
			     sint.port_entry, 1, TRUE);

  /*  The server logfile  */
  sint.log_entry = gtk_entry_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1, 
			     _("Server Logfile:"), 1.0, 0.5,
			     sint.log_entry, 1, TRUE);

  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return sint.run;
}

static void
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  sint.port = atoi (gtk_entry_get_text (GTK_ENTRY (sint.port_entry)));
  sint.logfile = g_strdup (gtk_entry_get_text (GTK_ENTRY (sint.log_entry)));
  sint.run = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}
