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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include "gtk/gtk.h"

#include "libgimp/gimp.h"
#include "libgimp/gimpprotocol.h"
#include "libgimp/gimpwire.h"

#include "dgimp_procs.h"

#define DEBUG_DGIMP

/* External functions.
 */
extern void      gimp_extension_process  (guint  timeout);

/* Declare local functions.
 */
static void      quit   (void);
static void      query  (void);
static void      run    (char      *name,
			 int        nparams,
			 GParam    *param,
			 int       *nreturn_vals,
			 GParam   **return_vals);

static void      dgimp_install_procedures  (void);
static void      dgimp_init_server         (void);
static void      dgimp_init_lgps           (void);
static void      dgimp_init_tcp_ip_args    (char      ***args,
					    char        *host);
static gint      dgimp_make_socket         (gint        *port);
static void      dgimp_process             (guint        timeout);
static void      dgimp_configure_lgp       (struct sockaddr_in *clientname,
					    gint         new);
static void      dgimp_handle_lgp_msg      (int          fd,
					    WireMessage *msg);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  quit,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

GList *dgimp_lgp_list = NULL;

static char  *dgimp_lgp_executable = NULL;
static gint   dgimp_server_port = 10007;
static gint   dgimp_server_sock;
static gint   dgimp_gimp_port;

static DGimpProc dgimp_procs[] =
{
  { "gimp_invert", dgimp_invert_proc }
};

static int dgimp_num_procs = sizeof (dgimp_procs) / sizeof (dgimp_procs[0]);

extern int    _writefd;
extern int    _readfd;

MAIN ();

static void
quit ()
{
  GList *list = dgimp_lgp_list;
  DGimpLGP *lgp;

  while (list)
    {
      lgp = (DGimpLGP *) list->data;

#if defined(DEBUG_DGIMP)
      g_print ("Shutting down lightweight GIMP process...\n");
#endif

      gp_quit_write (lgp->filedes);
      g_free (lgp);

      list = list->next;
    }
}

static void
query ()
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
  };
  static GParamDef *return_vals = NULL;
  static int nargs = sizeof (args) / sizeof (args[0]);
  static int nreturn_vals = 0;

  gimp_install_procedure ("plug_in_dgimp",
			  "Distributed GIMP",
			  "More help here later",
			  "Spencer Kimball",
			  "Spencer Kimball",
			  "1997",
			  "<Toolbox>/Xtns/Distributed GIMP",
			  NULL,
			  PROC_EXTENSION,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (char    *name,
     int      nparams,
     GParam  *param,
     int     *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      break;

    case RUN_NONINTERACTIVE:
      break;

    default:
      break;
    }

  dgimp_install_procedures ();
  dgimp_init_server ();
  dgimp_init_lgps ();

  /*  Enter the temp procedure processing loop  */
  while (1)
    {
      gimp_extension_process (100);
      dgimp_process (100);
    }

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;
}

static void
dgimp_install_procedures ()
{
  int i;
  int nargs;
  int nreturn_vals;
  int proc_type;
  char *blurb;
  char *new_blurb;
  char *help;
  char *author;
  char *copyright;
  char *date;
  GParamDef *args;
  GParamDef *return_vals;

  for (i = 0; i < dgimp_num_procs; i++)
    if (gimp_query_procedure (dgimp_procs[i].name,
			      &blurb,
			      &help,
			      &author,
			      &copyright,
			      &date,
			      &proc_type,
			      &nargs,
			      &nreturn_vals,
			      &args,
			      &return_vals))
	{
	  new_blurb = g_new (char, strlen (blurb) + strlen ("  (DISTRIBUTED)") + 1);
	  sprintf (new_blurb, "%s%s", blurb, "  (DISTRIBUTED)");

	  gimp_install_temp_proc (dgimp_procs[i].name,
				  new_blurb,
				  help,
				  author,
				  copyright,
				  date,
				  "",
				  NULL,
				  PROC_TEMPORARY,
				  nargs, nreturn_vals,
				  args, return_vals,
				  dgimp_procs[i].run_proc);
	}
}

static void
dgimp_init_server ()
{
  GParam *return_vals;
  int nreturn_vals;

  /*  The server port #  */
  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "dgimp-server-port",
				    PARAM_END);
  if (return_vals[0].data.d_status == STATUS_SUCCESS &&
      return_vals[1].data.d_string != NULL)
    dgimp_server_port = (int) atof (return_vals[1].data.d_string);

  /* Create the socket and set it up to accept connections. */
  dgimp_server_sock = dgimp_make_socket (&dgimp_server_port);

  if (listen (dgimp_server_sock, 1) < 0)
    {
      perror ("listen");
      return;
    }

#if defined(DEBUG_DGIMP)
  g_print ("dgimp server initialized and listening for connections...\n");
#endif
}

static void
dgimp_init_lgps ()
{
  GParam *return_vals;
  int nreturn_vals;
  char **args;
  char *host_list;
  char *host;
  pid_t pid;

  /*  The list of hosts for distributed processing  */
  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "dgimp-host-list",
				    PARAM_END);
  if (return_vals[0].data.d_status != STATUS_SUCCESS)
    {
      g_warning ("Unable to query for a dgimp host list.  Specify \"dgimp-host-list\" in the gimprc.");
      return;
    }
  if ((host_list = return_vals[1].data.d_string) == NULL)
    {
      g_warning ("No hosts specified by dgimp-host-list variable...");
      return;
    }

  /*  The location in the filesystem of the LGP executable (constant across hosts)  */
  return_vals = gimp_run_procedure ("gimp_gimprc_query",
				    &nreturn_vals,
				    PARAM_STRING, "dgimp-lgp-executable",
				    PARAM_END);
  if (return_vals[0].data.d_status != STATUS_SUCCESS)
    {
      g_warning ("No LGP executable.  Specify \"dgimp-lgp-executable\" in the gimprc.");
      return;
    }
  if ((dgimp_lgp_executable = return_vals[1].data.d_string) == NULL)
    {
      g_warning ("No LGP executable specified by dgimp-lgp-executable variable...");
      return;
    }

  host = strtok (host_list, " ");
  while (host)
    {
#if defined(DEBUG_DGIMP)
      g_print ("Invoking a lightweight GIMP process (LGP) on host %s...\n", host);
#endif
      /*  Initialize the arguments for invoking the LGP by TCP-IP  */
      dgimp_init_tcp_ip_args (&args, host);

      /* Fork another process for starting the LGP
       */
      if ((pid = fork ()) == 0)
	{
	  /* Execute the LGP starting program. The "_exit" call should never
	   *  be reached, unless some strange error condition
	   *  exists.
	   */
	  execvp (args[0], args);
	  _exit (1);
	}
      else if (pid == -1)
	g_warning ("unable to run LGP for host: %s\n", host);

      host = strtok (NULL, " ");
    }
}

static void
dgimp_init_tcp_ip_args (char ***args, char *host)
{
  char server_port_buf[12];
  char gimp_port_buf[12];
  char *localhost;
  struct hostent *he;
  struct in_addr inp;

  /*  The localhost inet address  */
  he = gethostbyname ("zarathustra");

  memcpy (&inp, he->h_addr_list[0], he->h_length);
  localhost = inet_ntoa (inp);

  /*  Allocate room for the arguments  */
  *args = g_new (gchar *, 7);

  sprintf (server_port_buf, "%d", dgimp_server_port);
  sprintf (gimp_port_buf, "%d", dgimp_gimp_port);

  /*  The arguments  */
  (*args)[0] = g_strdup ("rsh");
  (*args)[1] = g_strdup (host);
  (*args)[2] = g_strdup (dgimp_lgp_executable);
  (*args)[3] = g_strdup (localhost);
  (*args)[4] = g_strdup (server_port_buf);
  (*args)[5] = g_strdup (gimp_port_buf);
  (*args)[6] = NULL;
}

static gint
dgimp_make_socket (gint *port)
{
  gint sock;
  struct sockaddr_in name;
  gint v;
  gint initial_port = *port;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      gimp_quit ();
    }
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (*port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  while (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      (*port)++;  /*  Try the next sequential port number  */
      name.sin_port = htons (*port);
      if (*port > initial_port + 10)
	{
	  g_warning ("Unable to start dgimp server on ports (%d - %d).", initial_port, *port);
	  gimp_quit ();
	}
    }

  return sock;
}

static void
dgimp_process (guint timeout)
{
  WireMessage msg;
  struct timeval tv;
  struct timeval *tvp;
  fd_set active_fd_set;
  fd_set read_fd_set;
  gint i, select_val;
  struct sockaddr_in clientname;
  size_t size;

  if (timeout)
    {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = timeout % 1000;
      tvp = &tv;
    }
  else
    tvp = NULL;

  /* Initialize the set of active sockets. */
  FD_ZERO (&active_fd_set);
  FD_SET (dgimp_server_sock, &active_fd_set);

  /* Block until input arrives on one or more active sockets. */
  read_fd_set = active_fd_set;
  select_val = select (FD_SETSIZE, &read_fd_set, NULL, NULL, tvp);
  if (select_val < 0)
    {
      perror ("select");
      return;
    }
  else if (select_val == 0)
    return;

  /* Service all the sockets with input pending. */
  for (i = 0; i < FD_SETSIZE; ++i)
    if (FD_ISSET (i, &read_fd_set))
      {
	if (i == dgimp_server_sock)
	  {
	    /* Connection request on original socket. */
	    gint new;
	    size = sizeof (clientname);
	    new = accept (dgimp_server_sock,
			  (struct sockaddr *) &clientname,
			  &size);
	    if (new < 0)
	      {
		perror ("accept");
		return;
	      }

#if defined(DEBUG_DGIMP)
	    g_print ("Server: connect from host %s, port %hd.\n",
		     inet_ntoa (clientname.sin_addr),
		     ntohs (clientname.sin_port));
#endif
	    FD_SET (new, &active_fd_set);

	    /*  Configure LGP  */
	    dgimp_configure_lgp (&clientname, new);
	  }
	else
	  {
	    /* Data arriving on an already-connected socket. */
	    if (! wire_read_msg (i, &msg))
	      {
#if defined(DEBUG_DGIMP)
		g_print ("Server: disconnect from host %s, port %hd.\n",
			 inet_ntoa (clientname.sin_addr),
			 ntohs (clientname.sin_port));
#endif
		close (i);
		FD_CLR (i, &active_fd_set);
	      }
	    else
	      dgimp_handle_lgp_msg (i, &msg);
	  }
      }
}

static void
dgimp_configure_lgp (struct sockaddr_in *clientname,
		     gint                new)
{
  GPConfig config;
  DGimpLGP *lgp;
  guchar *color_cube;

  lgp = g_new (DGimpLGP, 1);
  lgp->filedes = new;
  lgp->available = TRUE;

  config.version = GP_VERSION;
  config.tile_width = gimp_tile_width ();
  config.tile_height = gimp_tile_height ();
  config.shm_ID = -1;
  config.gamma = gimp_gamma ();
  config.install_cmap = gimp_install_cmap ();
  config.use_xshm = FALSE;
  color_cube = gimp_color_cube ();
  config.color_cube[0] = color_cube[0];
  config.color_cube[1] = color_cube[1];
  config.color_cube[2] = color_cube[2];
  config.color_cube[3] = color_cube[3];

  if (!gp_config_write (lgp->filedes, &config) ||
      !wire_flush (lgp->filedes))
    g_warning ("Failed on LGP configure\n");
  else
    dgimp_lgp_list = g_list_append (dgimp_lgp_list, lgp);
}

static void
dgimp_handle_lgp_msg (int          fd,
		      WireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      g_warning ("unexpected quit message received from LGP(should not happen)\n");
      break;
    case GP_CONFIG:
      g_warning ("unexpected config message received from LGP(should not happen)\n");
      break;
    case GP_TILE_REQ:
    case GP_TILE_ACK:
    case GP_TILE_DATA:
      g_warning ("unexpected tile message received from LGP(should not happen)\n");
      break;
    case GP_PROC_RUN:
      g_print ("Got a proc run request from LGP\n");
      /*  forward to GIMP  */
      if (!wire_write_msg (_writefd, msg) ||
	  !wire_flush (_writefd))
	{
	  g_warning ("Failed message forwarding to GIMP.\n");
	  return;
	}
      else
	{
	  g_print ("Waiting for return vals from GIMP...\n");
	  /*  retrieve return vals, pass through to LGP  */
	  if (!wire_read_msg (_readfd, msg))
	    {
	      g_warning ("Failed to retrieve return vals from GIMP.\n");
	      return;
	    }
	  g_print ("Forwarding return vals to LGP...\n");
	  if (!wire_write_msg (fd, msg) ||
	      !wire_flush (fd))
	    {
	      g_warning ("Failed to forward return vals to LGP.\n");
	      return;
	    }
	  g_print ("done.\n");
	}
      break;
    case GP_PROC_RETURN:
      g_warning ("unexpected proc return message received from LGP(should not happen)\n");
      break;
    case GP_TEMP_PROC_RUN:
      g_warning ("unexpected temp proc run message received from LGP(should not happen)\n");
      break;
    case GP_TEMP_PROC_RETURN:
      g_print ("LGP returning!\n");
      break;
    case GP_PROC_INSTALL:
      /*  don't forward this install request, as the main GIMP needn't know about it  */
      break;
    }
}
