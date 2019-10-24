/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <sys/types.h>

#include <glib.h>

#ifdef G_OS_WIN32
#define _WIN32_WINNT 0x0502
#include <winsock2.h>
#include <ws2tcpip.h>

typedef short sa_family_t;	/* Not defined by winsock */

#ifndef AI_ADDRCONFIG
/* Missing from mingw headers, but value is publicly documented
 * on http://msdn.microsoft.com/en-us/library/ms737530%28v=VS.85%29.aspx
 */
#define AI_ADDRCONFIG 0x0400
#endif
#include <libgimpbase/gimpwin32-io.h>
#else
#include <sys/socket.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif
#endif

#include <glib/gstdio.h>

#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include "script-fu-intl.h"

#include "scheme-wrapper.h"
#include "script-fu-server.h"

#ifdef G_OS_WIN32
#define CLOSESOCKET(fd) closesocket(fd)
#else
#define CLOSESOCKET(fd) close(fd)
#endif

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

#define ERROR_BYTE      1
#define RSP_LEN_H_BYTE  2
#define RSP_LEN_L_BYTE  3

/*
 *  Local Types
 */

typedef struct
{
  gchar *command;
  gint   filedes;
  gint   request_no;
} SFCommand;

typedef struct
{
  GtkWidget *ip_entry;
  GtkWidget *port_entry;
  GtkWidget *log_entry;

  gchar     *listen_ip;
  gint       port;
  gchar     *logfile;

  gboolean   run;
} ServerInterface;

typedef union
{
  sa_family_t              family;
  struct sockaddr_storage  ss;
  struct sockaddr          sa;
  struct sockaddr_in       sa_in;
  struct sockaddr_in6      sa_in6;
} sa_union;

/*
 *  Local Functions
 */

static void      server_start       (const gchar *listen_ip,
                                     gint         port,
                                     const gchar *logfile);
static gboolean  execute_command    (SFCommand   *cmd);
static gint      read_from_client   (gint         filedes);
static gint      make_socket        (const struct addrinfo
                                                 *ai);
static void      server_log         (const gchar *format,
                                     ...) G_GNUC_PRINTF (1, 2);
static void      server_quit        (void);

static gboolean  server_interface   (void);
static void      response_callback  (GtkWidget   *widget,
                                     gint         response_id,
                                     gpointer     data);
static void      print_socket_api_error (const gchar *api_name);

/*
 *  Local variables
 */
static gint         server_socks[2],
                    server_socks_used = 0;
static const gint   server_socks_len = sizeof (server_socks) /
                                       sizeof (server_socks[0]);
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
  NULL,  /*  ip entry widget      */

  NULL,  /*  ip to bind to        */
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

  ts_set_run_mode (run_mode);
  ts_set_print_flag (1);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      if (server_interface ())
        {
          server_mode = TRUE;

          /*  Start the server  */
          server_start (sint.listen_ip, sint.port, sint.logfile);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*  Set server_mode to TRUE  */
      server_mode = TRUE;

      /*  Start the server  */
      server_start ((params[1].data.d_string &&
                     strlen (params[1].data.d_string)) ?
                    params[1].data.d_string : "127.0.0.1",
                    params[2].data.d_int32,
                    params[3].data.d_string);
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      status = GIMP_PDB_CALLING_ERROR;
      g_warning ("Script-Fu server does not handle \"GIMP_RUN_WITH_LAST_VALS\"");

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

          CLOSESOCKET (fd);

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
  gint            sockno;

  /*  Set time struct  */
  if (timeout)
    {
      tv.tv_sec  = timeout / 1000;
      tv.tv_usec = timeout % 1000;
      tvp = &tv;
    }

  FD_ZERO (&fds);
  for (sockno = 0; sockno < server_socks_used; sockno++)
    {
      FD_SET (server_socks[sockno], &fds);
    }
  g_hash_table_foreach (clients, script_fu_server_add_fd, &fds);

  /* Block until input arrives on one or more active sockets
     or timeout occurs. */

  if (select (FD_SETSIZE, &fds, NULL, NULL, tvp) < 0)
    {
      print_socket_api_error ("select");
      return;
    }

  /* Service the server sockets if any has input pending. */
  for (sockno = 0; sockno < server_socks_used; sockno++)
    {
      sa_union                 client;
      gchar                    clientname[NI_MAXHOST];

      /* Connection request on original socket. */
      guint                    size = sizeof (client);
      gint                     new;
      guint                    portno;

      if (! FD_ISSET (server_socks[sockno], &fds))
        {
          continue;
        }

      new = accept (server_socks[sockno], &(client.sa), &size);

      if (new < 0)
        {
          print_socket_api_error ("accept");
          return;
        }

      /*  Associate the client address with the socket  */

      /* If all else fails ... */
      strncpy (clientname, "(error during host address lookup)", NI_MAXHOST-1);

      /* Lookup address */
      (void) getnameinfo (&(client.sa), size, clientname, sizeof (clientname),
                          NULL, 0, NI_NUMERICHOST);

      g_hash_table_insert (clients, GINT_TO_POINTER (new),
                           g_strdup (clientname));

      /* Determine port number */
      switch (client.family)
        {
          case AF_INET:
            portno = (guint) g_ntohs (client.sa_in.sin_port);
            break;
          case AF_INET6:
            portno = (guint) g_ntohs (client.sa_in6.sin6_port);
            break;
          default:
            portno = 0;
        }

      server_log ("Server: connect from host %s, port %d.\n",
                  clientname, portno);
    }

  /* Service the client sockets. */
  g_hash_table_foreach_remove (clients, script_fu_server_read_fd, &fds);
}

static void
server_progress_start (const gchar *message,
                       gboolean     cancelable,
                       gpointer     user_data)
{
  /* do nothing */
}

static void
server_progress_end (gpointer user_data)
{
  /* do nothing */
}

static void
server_progress_set_text (const gchar *message,
                          gpointer     user_data)
{
  /* do nothing */
}

static void
server_progress_set_value (gdouble   percentage,
                           gpointer  user_data)
{
  /* do nothing */
}


/*
 * Suppress progress popups by installing progress handlers that do nothing.
 */
static const gchar *
server_progress_install (void)
{
  GimpProgressVtable vtable = { 0, };

  vtable.start     = server_progress_start;
  vtable.end       = server_progress_end;
  vtable.set_text  = server_progress_set_text;
  vtable.set_value = server_progress_set_value;

  return gimp_progress_install_vtable (&vtable, NULL);
}

static void
server_progress_uninstall (const gchar *progress)
{
  gimp_progress_uninstall (progress);
}

static void
server_start (const gchar *listen_ip,
              gint         port,
              const gchar *logfile)
{
  struct addrinfo *ai;
  struct addrinfo *ai_curr;
  struct addrinfo  hints;
  gint             e;
  gint             sockno;
  gchar           *port_s;
  const gchar     *progress;

  memset (&hints, 0, sizeof (hints));
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
  hints.ai_socktype = SOCK_STREAM;

  port_s = g_strdup_printf ("%d", port);
  e = getaddrinfo (listen_ip, port_s, &hints, &ai);
  g_free (port_s);

  if (e != 0)
    {
      g_printerr ("getaddrinfo: %s\n", gai_strerror (e));
      return;
    }

  for (ai_curr = ai, sockno = 0;
       ai_curr != NULL && sockno < server_socks_len;
       ai_curr = ai_curr->ai_next, sockno++)
    {
      /* Create the socket and set it up to accept connections.          */
      /* This may fail if there's a server running on this port already. */
      server_socks[sockno] = make_socket (ai_curr);

      if (listen (server_socks[sockno], 5) < 0)
        {
          print_socket_api_error ("listen");
          freeaddrinfo (ai);
          return;
        }
    }

  server_socks_used = sockno;

  /*  Setup up the server log file  */
  if (logfile && *logfile)
    server_log_file = g_fopen (logfile, "a");
  else
    server_log_file = NULL;

  if (! server_log_file)
    server_log_file = stdout;

  /*  Set up the clientname hash table  */
  clients = g_hash_table_new_full (g_direct_hash, NULL,
                                   NULL, (GDestroyNotify) g_free);

  progress = server_progress_install ();

  server_log ("Script-Fu server initialized and listening...\n");

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

  server_progress_uninstall (progress);

  freeaddrinfo (ai);
  server_quit ();
}

static gboolean
execute_command (SFCommand *cmd)
{
  guchar      buffer[RESPONSE_HEADER];
  GString    *response;
  time_t      clocknow;
  gboolean    error;
  gint        i;
  gdouble     total_time;
  GTimer     *timer;

  server_log ("Processing request #%d\n", cmd->request_no);
  timer = g_timer_new ();

  response = g_string_new (NULL);
  ts_register_output_func (ts_gstring_output_func, response);

  /*  run the command  */
  if (ts_interpret_string (cmd->command) != 0)
    {
      error = TRUE;

      server_log ("%s\n", response->str);
    }
  else
    {
      error = FALSE;

      if (response->len == 0)
        g_string_assign (response, ts_get_success_msg ());

      total_time = g_timer_elapsed (timer, NULL);
      time (&clocknow);
      server_log ("Request #%d processed in %.3f seconds, finishing on %s",
                  cmd->request_no, total_time, ctime (&clocknow));
    }
  g_timer_destroy (timer);

  buffer[MAGIC_BYTE]     = MAGIC;
  buffer[ERROR_BYTE]     = error ? TRUE : FALSE;
  buffer[RSP_LEN_H_BYTE] = (guchar) (response->len >> 8);
  buffer[RSP_LEN_L_BYTE] = (guchar) (response->len & 0xFF);

  /*  Write the response to the client  */
  for (i = 0; i < RESPONSE_HEADER; i++)
    if (cmd->filedes > 0 && send (cmd->filedes, buffer + i, 1, 0) < 0)
      {
        /*  Write error  */
        print_socket_api_error ("send");
        return FALSE;
      }

  for (i = 0; i < response->len; i++)
    if (cmd->filedes > 0 && send (cmd->filedes, response->str + i, 1, 0) < 0)
      {
        /*  Write error  */
        print_socket_api_error ("send");
        return FALSE;
      }

  g_string_free (response, TRUE);

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
      nbytes = recv (filedes, buffer + i, COMMAND_HEADER - i, 0);

      if (nbytes < 0)
        {
#ifndef G_OS_WIN32
          if (errno == EINTR)
            continue;
#endif
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
      nbytes = recv (filedes, command + i, command_len - i, 0);

      if (nbytes <= 0)
        {
#ifndef G_OS_WIN32
          if (nbytes < 0 && errno == EINTR)
            continue;
#endif
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
make_socket (const struct addrinfo *ai)
{
  gint                    sock;
  gint                    v = 1;

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
          print_socket_api_error ("WSAStartup");
          gimp_quit ();
        }
    }
#endif

  /* Create the socket. */
  sock = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (sock < 0)
    {
      print_socket_api_error ("socket");
      gimp_quit ();
    }

  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));

#ifdef IPV6_V6ONLY
  /* Only listen on IPv6 addresses, otherwise bind() will fail. */
  if (ai->ai_family == AF_INET6)
    {
      v = 1;
      if (setsockopt (sock, IPPROTO_IPV6, IPV6_V6ONLY, &v, sizeof(v)) < 0)
        {
          print_socket_api_error ("setsockopt");
          gimp_quit();
        }
    }
#endif

  if (bind (sock, ai->ai_addr, ai->ai_addrlen) < 0)
    {
      print_socket_api_error ("bind");
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
  gint sockno;

  for (sockno = 0; sockno < server_socks_used; sockno++)
    {
      CLOSESOCKET (server_socks[sockno]);
    }

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
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkWidget *hbox;
  GtkWidget *image;
  GtkWidget *label;

  INIT_I18N();

  gimp_ui_init ("script-fu", FALSE);

  dlg = gimp_dialog_new (_("Script-Fu Server Options"), "gimp-script-fu",
                         NULL, 0,
                         gimp_standard_help_func, "plug-in-script-fu-server",

                         _("_Cancel"),       GTK_RESPONSE_CANCEL,
                         _("_Start Server"), GTK_RESPONSE_OK,

                         NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dlg),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dlg, "response",
                    G_CALLBACK (response_callback),
                    NULL);
  g_signal_connect (dlg, "destroy",
                    G_CALLBACK (gtk_main_quit),
                    NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*  The table to hold port, logfile and listen-to entries  */
  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* The server ip to listen to */
  sint.ip_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (sint.ip_entry), "127.0.0.1");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Listen on IP:"), 0.0, 0.5,
                             sint.ip_entry, 1, FALSE);

  /*  The server port  */
  sint.port_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (sint.port_entry), "10008");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("Server port:"), 0.0, 0.5,
                             sint.port_entry, 1, FALSE);

  /*  The server logfile  */
  sint.log_entry = gtk_entry_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Server logfile:"), 0.0, 0.5,
                             sint.log_entry, 1, FALSE);

  /* Warning */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  image = gtk_image_new_from_icon_name (GIMP_ICON_DIALOG_WARNING,
                                        GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (hbox), image, TRUE, TRUE, 0);
  gtk_widget_show (image);

  label = gtk_label_new (_("Listening on an IP address other than "
                           "127.0.0.1 (especially 0.0.0.0) can allow "
                           "attackers to remotely execute arbitrary code "
                           "on this machine."));
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

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
      g_free (sint.listen_ip);

      sint.port      = atoi (gtk_entry_get_text (GTK_ENTRY (sint.port_entry)));
      sint.logfile   = g_strdup (gtk_entry_get_text (GTK_ENTRY (sint.log_entry)));
      sint.listen_ip = g_strdup (gtk_entry_get_text (GTK_ENTRY (sint.ip_entry)));
      sint.run       = TRUE;
    }

  gtk_widget_destroy (widget);
}

static void
print_socket_api_error (const gchar *api_name)
{
#ifdef G_OS_WIN32
  /* Yes, this functionality really belongs to GLib. */
  const gchar *emsg;
  gchar unk[100];
  int number = WSAGetLastError ();

  switch (number)
    {
    case WSAEINTR:
      emsg = "Interrupted function call";
      break;
    case WSAEACCES:
      emsg = "Permission denied";
      break;
    case WSAEFAULT:
      emsg = "Bad address";
      break;
    case WSAEINVAL:
      emsg = "Invalid argument";
      break;
    case WSAEMFILE:
      emsg = "Too many open sockets";
      break;
    case WSAEWOULDBLOCK:
      emsg = "Resource temporarily unavailable";
      break;
    case WSAEINPROGRESS:
      emsg = "Operation now in progress";
      break;
    case WSAEALREADY:
      emsg = "Operation already in progress";
      break;
    case WSAENOTSOCK:
      emsg = "Socket operation on nonsocket";
      break;
    case WSAEDESTADDRREQ:
      emsg = "Destination address required";
      break;
    case WSAEMSGSIZE:
      emsg = "Message too long";
      break;
    case WSAEPROTOTYPE:
      emsg = "Protocol wrong type for socket";
      break;
    case WSAENOPROTOOPT:
      emsg = "Bad protocol option";
      break;
    case WSAEPROTONOSUPPORT:
      emsg = "Protocol not supported";
      break;
    case WSAESOCKTNOSUPPORT:
      emsg = "Socket type not supported";
      break;
    case WSAEOPNOTSUPP:
      emsg = "Operation not supported on transport endpoint";
      break;
    case WSAEPFNOSUPPORT:
      emsg = "Protocol family not supported";
      break;
    case WSAEAFNOSUPPORT:
      emsg = "Address family not supported by protocol family";
      break;
    case WSAEADDRINUSE:
      emsg = "Address already in use";
      break;
    case WSAEADDRNOTAVAIL:
      emsg = "Address not available";
      break;
    case WSAENETDOWN:
      emsg = "Network interface is not configured";
      break;
    case WSAENETUNREACH:
      emsg = "Network is unreachable";
      break;
    case WSAENETRESET:
      emsg = "Network dropped connection on reset";
      break;
    case WSAECONNABORTED:
      emsg = "Software caused connection abort";
      break;
    case WSAECONNRESET:
      emsg = "Connection reset by peer";
      break;
    case WSAENOBUFS:
      emsg = "No buffer space available";
      break;
    case WSAEISCONN:
      emsg = "Socket is already connected";
      break;
    case WSAENOTCONN:
      emsg = "Socket is not connected";
      break;
    case WSAESHUTDOWN:
      emsg = "Can't send after socket shutdown";
      break;
    case WSAETIMEDOUT:
      emsg = "Connection timed out";
      break;
    case WSAECONNREFUSED:
      emsg = "Connection refused";
      break;
    case WSAEHOSTDOWN:
      emsg = "Host is down";
      break;
    case WSAEHOSTUNREACH:
      emsg = "Host is unreachable";
      break;
    case WSAEPROCLIM:
      emsg = "Too many processes";
      break;
    case WSASYSNOTREADY:
      emsg = "Network subsystem is unavailable";
      break;
    case WSAVERNOTSUPPORTED:
      emsg = "Winsock.dll version out of range";
      break;
    case WSANOTINITIALISED:
      emsg = "Successful WSAStartup not yet performed";
      break;
    case WSAEDISCON:
      emsg = "Graceful shutdown in progress";
      break;
    case WSATYPE_NOT_FOUND:
      emsg = "Class type not found";
      break;
    case WSAHOST_NOT_FOUND:
      emsg = "Host not found";
      break;
    case WSATRY_AGAIN:
      emsg = "Nonauthoritative host not found";
      break;
    case WSANO_RECOVERY:
      emsg = "This is a nonrecoverable error";
      break;
    case WSANO_DATA:
      emsg = "Valid name, no data record of requested type";
      break;
    case WSA_INVALID_HANDLE:
      emsg = "Specified event object handle is invalid";
      break;
    case WSA_INVALID_PARAMETER:
      emsg = "One or more parameters are invalid";
      break;
    case WSA_IO_INCOMPLETE:
      emsg = "Overlapped I/O event object not in signaled state";
      break;
    case WSA_NOT_ENOUGH_MEMORY:
      emsg = "Insufficient memory available";
      break;
    case WSA_OPERATION_ABORTED:
      emsg = "Overlapped operation aborted";
      break;
    case WSAEINVALIDPROCTABLE:
      emsg = "Invalid procedure table from service provider";
      break;
    case WSAEINVALIDPROVIDER:
      emsg = "Invalid service provider version number";
      break;
    case WSAEPROVIDERFAILEDINIT:
      emsg = "Unable to initialize a service provider";
      break;
    case WSASYSCALLFAILURE:
      emsg = "System call failure";
      break;
    default:
      sprintf (unk, "Unknown WinSock error %d", number);
      emsg = unk;
      break;
    }

  g_printerr ("%s failed: %s\n", api_name, emsg);
#else
  perror (api_name);
#endif
}
