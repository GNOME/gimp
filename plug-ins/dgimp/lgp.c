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
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
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

#include "config.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpprotocol.h"
#include "libgimp/gimpwire.h"

#include "lgp_procs.h"

#define DEBUG_LGP

#define WRITE_BUFFER_SIZE  1024

/*  Functions
 */

extern void       gimp_extension_process (guint timeout);

static RETSIGTYPE lgp_signal         (int signum);
static int        lgp_write          (int fd, guint8 *buf, gulong count);
static int        lgp_flush          (int fd);
static void       lgp_init_procs     (void);
static gint       lgp_connect_server (char * host,
				      gint   port);


/*  Static Variables
 */
static gchar *dgimp_server_host;
static gint   dgimp_server_port;
static gint   dgimp_gimp_port;
static gint   lgp_sock;
static guint8 write_buffer[WRITE_BUFFER_SIZE];
static guint  write_buffer_index = 0;

extern int _readfd;
extern int _writefd;

/*  need to define this for libgimp  */
GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  NULL,    /* query_proc */
  NULL,    /* run_proc */
};


/*  Calling format for LGP:
 *
 *   lgp dgimp_server_host dgimp_server_port dgimp_gimp_port
 */

int
main (int   argc,
      char *argv[])
{
  if (argc != 4)
    {
      g_warning ("Incorrect invocation of LGP:\n\t\t"
		 "(lgp dgimp_server_host dgimp_server_port dgimp_gimp_port)");
      exit (1);
    }

  signal (SIGHUP,  lgp_signal);
  signal (SIGINT,  lgp_signal);
  signal (SIGQUIT, lgp_signal);
  signal (SIGBUS,  lgp_signal);
  signal (SIGSEGV, lgp_signal);
  signal (SIGPIPE, lgp_signal);
  signal (SIGTERM, lgp_signal);

  dgimp_server_host = g_strdup (argv[1]);
  dgimp_server_port = atoi (argv[2]);
  dgimp_gimp_port = atoi (argv[3]);

  lgp_sock = lgp_connect_server (dgimp_server_host, dgimp_server_port);

  /*  Set the read and write file descriptors to the full-duplex
   *   data connection opened to the DGIMP server
   */
  _readfd = lgp_sock;
  _writefd = lgp_sock;

  gp_init ();
  wire_set_writer (lgp_write);
  wire_set_flusher (lgp_flush);

  /*  Init the temp procs defined by this LGP  */
  lgp_init_procs ();

#if defined(DEBUG_LGP)
  g_print ("LGP started with:\n");
  g_print ("\tdgimp_server_host: %s\n", dgimp_server_host);
  g_print ("\tdgimp_server_port: %d\n", dgimp_server_port);
  g_print ("\tdgimp_gimp_port:   %d\n", dgimp_gimp_port);
  fflush (stdout);
#endif

  while (1)
    gimp_extension_process (0);

  return 0;
}

static RETSIGTYPE
lgp_signal (int signum)
{
  static int caught_fatal_sig = 0;

  if (caught_fatal_sig)
    kill (getpid (), signum);
  caught_fatal_sig = 1;

  g_warning ("\nLGP: %s caught\n", g_strsignal (signum));
  g_debug ("LGP");

  gimp_quit ();
}

static int
lgp_write (int fd, guint8 *buf, gulong count)
{
  gulong bytes;

  while (count > 0)
    {
      if ((write_buffer_index + count) >= WRITE_BUFFER_SIZE)
	{
	  bytes = WRITE_BUFFER_SIZE - write_buffer_index;
	  memcpy (&write_buffer[write_buffer_index], buf, bytes);
	  write_buffer_index += bytes;
	  if (!wire_flush (fd))
	    return FALSE;
	}
      else
	{
	  bytes = count;
	  memcpy (&write_buffer[write_buffer_index], buf, bytes);
	  write_buffer_index += bytes;
	}

      buf += bytes;
      count -= bytes;
    }

  return TRUE;
}

static int
lgp_flush (int fd)
{
  int count;
  int bytes;

  if (write_buffer_index > 0)
    {
      count = 0;
      while (count != write_buffer_index)
        {
	  do {
	    bytes = write (fd, &write_buffer[count], (write_buffer_index - count));
	  } while ((bytes == -1) && (errno == EAGAIN));

	  if (bytes == -1)
	    return FALSE;

          count += bytes;
        }

      write_buffer_index = 0;
    }

  return TRUE;
}

static void
lgp_init_procs (void)
{
  lgp_invert_install ();
}

static gint
lgp_connect_server (char *host,
		    gint  port)
{
  gint sock;
  struct sockaddr_in name;
  struct hostent *hostinfo;

  /* Create the socket */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket (client)");
      exit (1);
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  if ((hostinfo = gethostbyname (host)) == NULL)
    {
      g_warning ("Unknown host %s.\n", host);
      exit (1);
    }
  name.sin_addr = *(struct in_addr *) hostinfo->h_addr;

  if (connect (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      g_warning ("Unable to connect to dgimp server.");
      exit (1);
    }

#if defined(DEBUG_LGP)
  g_print ("Connect succeeded.\n");
  fflush (stdout);
#endif

  return sock;
}
