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

#include "config.h"

#include <glib.h>		/* For G_OS_WIN32 */

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>

#ifdef G_OS_WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */

#include <gtk/gtk.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "script-fu-intl.h"

#include "siod-wrapper.h"
#include "script-fu-server.h"


#define COMMAND_HEADER  3
#define RESPONSE_HEADER 4
#define MAGIC           'G'

#ifndef HAVE_DIFFTIME
#define difftime(a,b) (((gdouble)(a)) - ((gdouble)(b)))
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

  gboolean   run;
} ServerInterface;

/*
 *  Local Functions
 */

static void      server_start       (gint         port,
				     const gchar *logfile);
static gboolean  execute_command    (SFCommand   *cmd);
static gint      read_from_client   (gint         filedes);
static gint      make_socket        (guint        port);
static void      server_log         (const gchar *format,
				     ...) G_GNUC_PRINTF (1, 2);
static void      server_quit        (void);

static gboolean  server_interface   (void);
static void      response_callback  (GtkWidget   *widget,
                                     gint         response_id,
				     gpointer     data);


/*
 *  Local variables
 */
static gint         server_sock;
static GList       *command_queue   = NULL;
static gint         queue_length    = 0;
static gint         request_no      = 0;
static FILE        *server_log_file = NULL;
static GHashTable  *clients         = NULL;
static gboolean     script_fu_done  = FALSE;
static gboolean     server_mode     = FALSE;

static ServerInterface sint =
{
  NULL,  /*  port entry widget    */
  NULL,  /*  log entry widget     */

  10008, /*  default port number  */
  NULL,  /*  use stdout           */

  FALSE  /*  run                  */
};

/*
 *  Server interface functions
 */

void
script_fu_server_quit (void)
{
  script_fu_done = TRUE;
}

gint
script_fu_server_get_mode (void)
{
  return server_mode;
}

void
script_fu_server_run (const gchar      *name,
		      gint              nparams,
		      const GimpParam  *params,
		      gint             *nreturn_vals,
		      GimpParam       **return_vals)
{
  static GimpParam   values[1];
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  GimpRunMode        run_mode;

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

static void
script_fu_server_add_fd (gpointer key,
                         gpointer value,
                         gpointer data)
{
  FD_SET (GPOINTER_TO_INT (key), (SELECT_MASK *) data);
}

static gboolean
script_fu_server_read_fd (gpointer key,
                          gpointer value,
                          gpointer data)
{
  gint fd = GPOINTER_TO_INT (key);

  if (FD_ISSET (fd, (SELECT_MASK *) data))
    {
      if (read_from_client (fd) < 0)
        {
          GList *list;

          server_log ("Server: disconnect from host %s.\n", (gchar *) value);

          close (fd);

          /*  Invalidate the file descriptor for pending commands
              from the disconnected client.  */
          for (list = command_queue; list; list = list->next)
            {
              SFCommand *cmd = (SFCommand *) command_queue->data;

              if (cmd->filedes == fd)
                cmd->filedes = -1;
            }

          return TRUE;  /*  remove this client from the hash table  */
        }
    }

  return FALSE;
}

void
script_fu_server_listen (gint timeout)
{
  struct timeval  tv;
  struct timeval *tvp = NULL;
  SELECT_MASK     fds;

  /*  Set time struct  */
  if (timeout)
    {
      tv.tv_sec  = timeout / 1000;
      tv.tv_usec = timeout % 1000;
      tvp = &tv;
    }

  FD_ZERO (&fds);
  FD_SET (server_sock, &fds);
  g_hash_table_foreach (clients, script_fu_server_add_fd, &fds);

  /* Block until input arrives on one or more active sockets
     or timeout occurs. */

  if (select (FD_SETSIZE, &fds, NULL, NULL, tvp) < 0)
    {
      perror ("select");
      return;
    }

  /* Service the server socket if it has input pending. */
  if (FD_ISSET (server_sock, &fds))
    {
      struct sockaddr_in  clientname;

      /* Connection request on original socket. */
      guint size = sizeof (clientname);
      gint  new  = accept (server_sock,
                           (struct sockaddr *) &clientname, &size);

      if (new < 0)
        {
          perror ("accept");
          return;
        }

      /*  Associate the client address with the socket  */
      g_hash_table_insert (clients,
                           GINT_TO_POINTER (new),
                           g_strdup (inet_ntoa (clientname.sin_addr)));

      server_log ("Server: connect from host %s, port %d.\n",
                  inet_ntoa (clientname.sin_addr),
                  (unsigned int) ntohs (clientname.sin_port));
    }

  /* Service the client sockets. */
  g_hash_table_foreach_remove (clients, script_fu_server_read_fd, &fds);
}

static void
server_start (gint         port,
	      const gchar *logfile)
{
  /* First of all, create the socket and set it up to accept connections. */
  /* This may fail if there's a server running on this port already.      */
  server_sock = make_socket (port);

  if (listen (server_sock, 5) < 0)
    {
      perror ("listen");
      return;
    }

  /*  Setup up the server log file  */
  if (logfile && *logfile)
    server_log_file = fopen (logfile, "a");
  else
    server_log_file = NULL;

  if (! server_log_file)
    server_log_file = stdout;

  /*  Set up the clientname hash table  */
  clients = g_hash_table_new_full (g_direct_hash, NULL,
                                   NULL, (GDestroyNotify) g_free);

  server_log ("Script-fu server initialized and listening...\n");

  /*  Loop until the server is finished  */
  while (! script_fu_done)
    {
      script_fu_server_listen (0);

      while (command_queue)
	{
          SFCommand *cmd = (SFCommand *) command_queue->data;

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
}

static gboolean
execute_command (SFCommand *cmd)
{
  guchar       buffer[RESPONSE_HEADER];
  const gchar *response;
  time_t       clock1;
  time_t       clock2;
  gint         response_len;
  gboolean     error;
  gint         i;

  server_log ("Processing request #%d\n", cmd->request_no);
  time (&clock1);

  /*  run the command  */
  if (siod_interpret_string (cmd->command) != 0)
    {
      error = TRUE;
      response = siod_get_error_msg ();
      response_len = strlen (response);

      server_log ("%s\n", response);
    }
  else
    {
      error = FALSE;

      response = siod_get_success_msg ();
      response_len = strlen (response);

      time (&clock2);
      server_log ("Request #%d processed in %f seconds, finishing on %s",
		  cmd->request_no, difftime (clock2, clock1), ctime (&clock2));
    }

  buffer[MAGIC_BYTE]     = MAGIC;
  buffer[ERROR]          = error ? TRUE : FALSE;
  buffer[RSP_LEN_H_BYTE] = (guchar) (response_len >> 8);
  buffer[RSP_LEN_L_BYTE] = (guchar) (response_len & 0xFF);

  /*  Write the response to the client  */
  for (i = 0; i < RESPONSE_HEADER; i++)
    if (cmd->filedes > 0 && write (cmd->filedes, buffer + i, 1) < 0)
      {
	/*  Write error  */
	perror ("write");
	return FALSE;
      }

  for (i = 0; i < response_len; i++)
    if (cmd->filedes > 0 && write (cmd->filedes, response + i, 1) < 0)
      {
	/*  Write error  */
	perror ("write");
	return FALSE;
      }

  return FALSE;
}

static gint
read_from_client (gint filedes)
{
  SFCommand *cmd;
  guchar     buffer[COMMAND_HEADER];
  gchar     *command;
  gchar     *clientaddr;
  time_t     clock;
  gint       command_len;
  gint       nbytes;
  gint       i;

  for (i = 0; i < COMMAND_HEADER;)
    {
      nbytes = read (filedes, buffer + i, COMMAND_HEADER - i);

      if (nbytes < 0)
        {
          if (errno == EINTR)
            continue;

          server_log ("Error reading command header.\n");
          return -1;
        }

      if (nbytes == 0)
        return -1;  /* EOF */

      i += nbytes;
    }

  if (buffer[MAGIC_BYTE] != MAGIC)
    {
      server_log ("Error in script-fu command transmission.\n");
      return -1;
    }

  command_len = (buffer [CMD_LEN_H_BYTE] << 8) | buffer [CMD_LEN_L_BYTE];
  command = g_new (gchar, command_len + 1);

  for (i = 0; i < command_len;)
    {
      nbytes = read (filedes, command + i, command_len - i);

      if (nbytes <= 0)
        {
          if (nbytes < 0 && errno == EINTR)
            continue;

           server_log ("Error reading command.  Read %d out of %d bytes.\n",
                       i, command_len);
           g_free (command);
           return -1;
        }

      i += nbytes;
    }

  command[command_len] = '\0';
  cmd = g_new (SFCommand, 1);

  cmd->filedes    = filedes;
  cmd->command    = command;
  cmd->request_no = request_no ++;

  /*  Add the command to the queue  */
  command_queue = g_list_append (command_queue, cmd);
  queue_length ++;

  /*  Get the client address from the address/socket table  */
  clientaddr = g_hash_table_lookup (clients, GINT_TO_POINTER (cmd->filedes));
  time (&clock);
  server_log ("Received request #%d from IP address %s: %s on %s,"
	      "[Request queue length: %d]",
	      cmd->request_no,
              clientaddr ? clientaddr : "<invalid>",
              cmd->command, ctime (&clock), queue_length);

  return 0;
}

static gint
make_socket (guint port)
{
  struct sockaddr_in name;
  gint               sock;
  gint               v = 1;

  /*  Win32 needs the winsock library initialized.  */
#ifdef G_OS_WIN32
  static gboolean    winsock_initialized = FALSE;

  if (! winsock_initialized)
    {
      WORD    wVersionRequested = MAKEWORD (2, 2);
      WSADATA wsaData;

      if (WSAStartup (wVersionRequested, &wsaData) == 0)
        {
          winsock_initialized = TRUE;
        }
      else
        {
          perror ("Can't initialize the Winsock DLL");
          gimp_quit ();
        }
    }
#endif

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      gimp_quit ();
    }

  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));

  /* Give the socket a name. */
  name.sin_family      = AF_INET;
  name.sin_port        = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);

  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      perror ("bind");
      gimp_quit ();
    }

  return sock;
}

static void
server_log (const gchar *format,
            ...)
{
  va_list  args;
  gchar   *buf;

  va_start (args, format);
  buf = g_strdup_vprintf (format, args);
  va_end (args);

  fputs (buf, server_log_file);
  g_free (buf);

  if (server_log_file != stdout)
    fflush (server_log_file);
}

static void
script_fu_server_shutdown_fd (gpointer key,
                              gpointer value,
                              gpointer data)
{
  shutdown (GPOINTER_TO_INT (key), 2);
}

static void
server_quit (void)
{
  close (server_sock);

  if (clients)
    {
      g_hash_table_foreach (clients, script_fu_server_shutdown_fd, NULL);
      g_hash_table_destroy (clients);
      clients = NULL;
    }

  while (command_queue)
    {
      SFCommand *cmd = command_queue->data;

      g_free (cmd->command);
      g_free (cmd);
    }

  g_list_free (command_queue);
  command_queue = NULL;
  queue_length  = 0;

  /*  Close the server log file  */
  if (server_log_file != stdout)
    fclose (server_log_file);

  server_log_file = NULL;
}

static gboolean
server_interface (void)
{
  GtkWidget *dlg;
  GtkWidget *table;

  INIT_I18N();

  gimp_ui_init ("script-fu", FALSE);

  dlg = gimp_dialog_new (_("Script-Fu Server Options"), "script-fu",
                         NULL, 0,
			 gimp_standard_help_func, "plug-in-script-fu-server",

			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			 GTK_STOCK_OK,     GTK_RESPONSE_OK,

			 NULL);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (response_callback),
                    NULL);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  /*  The table to hold port & logfile entries  */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
                      table, FALSE, FALSE, 0);

  /*  The server port  */
  sint.port_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (sint.port_entry), "10008");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("Server Port:"), 0.0, 0.5,
			     sint.port_entry, 1, FALSE);

  /*  The server logfile  */
  sint.log_entry = gtk_entry_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Server Logfile:"), 0.0, 0.5,
			     sint.log_entry, 1, FALSE);

  gtk_widget_show (table);
  gtk_widget_show (dlg);

  gtk_main ();

  return sint.run;
}

static void
response_callback (GtkWidget *widget,
                   gint       response_id,
                   gpointer   data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      g_free (sint.logfile);

      sint.port    = atoi (gtk_entry_get_text (GTK_ENTRY (sint.port_entry)));
      sint.logfile = g_strdup (gtk_entry_get_text (GTK_ENTRY (sint.log_entry)));
      sint.run     = TRUE;
    }

  gtk_widget_destroy (widget);
}
