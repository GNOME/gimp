/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimp.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef WAIT_ANY
#define WAIT_ANY -1
#endif

#include <glib.h>

#ifndef G_OS_WIN32
#include "libgimpbase/gimpsignal.h"
#else
#include <signal.h>
#endif

#ifdef HAVE_IPC_H
#include <sys/ipc.h>
#endif

#ifdef HAVE_SHM_H
#include <sys/shm.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#  define STRICT
#  include <windows.h>
#  undef RGB
#endif
#ifdef __EMX__
#  include <fcntl.h>
#endif

#include "libgimpbase/gimpbasetypes.h"

#include "libgimpbase/gimpenv.h"
#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimp.h"


#define WRITE_BUFFER_SIZE  1024

void gimp_extension_process (guint        timeout);
void gimp_extension_ack     (void);
void gimp_read_expect_msg   (WireMessage *msg,
			     gint         type);


#ifndef G_OS_WIN32
static void       gimp_plugin_sigfatal_handler (gint            sig_num);
#endif
static gboolean   gimp_plugin_io_error_handler (GIOChannel     *channel,
						GIOCondition    cond,
						gpointer        data);

static gboolean   gimp_write                   (GIOChannel     *channel,
						guint8         *buf,
						gulong          count,
                                                gpointer        user_data);
static gboolean   gimp_flush                   (GIOChannel     *channel,
                                                gpointer        user_data);
static void       gimp_loop                    (void);
static void       gimp_config                  (GPConfig       *config);
static void       gimp_proc_run                (GPProcRun      *proc_run);
static void       gimp_temp_proc_run           (GPProcRun      *proc_run);
static void       gimp_message_func            (const gchar    *log_domain,
						GLogLevelFlags  log_level,
						const gchar    *message,
						gpointer        data);
static void       gimp_process_message         (WireMessage    *msg);
static void       gimp_close                   (void);


GIOChannel *_readchannel  = NULL;
GIOChannel *_writechannel = NULL;

gint    _shm_ID   = -1;
guchar *_shm_addr = NULL;

guint gimp_major_version = GIMP_MAJOR_VERSION;
guint gimp_minor_version = GIMP_MINOR_VERSION;
guint gimp_micro_version = GIMP_MICRO_VERSION;

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
static HANDLE shm_handle;
#endif

static gdouble  _gamma_val;
static gboolean _install_cmap;
static gint     _min_colors;
static gint     _gdisp_ID = -1;

static gchar   *progname = NULL;
static guint8   write_buffer[WRITE_BUFFER_SIZE];
static guint    write_buffer_index = 0;

static GimpStackTraceMode stack_trace_mode = GIMP_STACK_TRACE_NEVER;

static GHashTable *temp_proc_ht = NULL;

#ifdef G_OS_WIN32
static GimpPlugInInfo *PLUG_IN_INFO_PTR;
#define PLUG_IN_INFO (*PLUG_IN_INFO_PTR)
void
set_gimp_PLUG_IN_INFO_PTR (GimpPlugInInfo *p)
{
  PLUG_IN_INFO_PTR = p;
}
#else
#ifndef __EMX__
extern GimpPlugInInfo PLUG_IN_INFO;
#else
static GimpPlugInInfo PLUG_IN_INFO;
void set_gimp_PLUG_IN_INFO (const GimpPlugInInfo *p)
{
  PLUG_IN_INFO = *p;
}
#endif
#endif


int
gimp_main (int   argc,
	   char *argv[])
{
  gchar *basename;

#ifdef G_OS_WIN32
  gint i, j, k;

  g_assert (PLUG_IN_INFO_PTR != NULL);

  /* Check for exe file name with spaces in the path having been split up
   * by buggy NT C runtime, or something. I don't know why this happens
   * on NT (including w2k), but not on w95/98.
   */

  for (i = 1; i < argc; i++)
    {
      k = strlen (argv[i]);
      if (k > 10)
	if (g_strcasecmp (argv[i] + k - 4, ".exe") == 0)
	  {
	    /* Found the end of the executable name, most probably.
	     * Splice the parts of the name back together.
	     */
	    GString *s;

	    s = g_string_new (argv[0]);
	    for (j = 1; j <= i; j++)
	      {
		s = g_string_append_c (s, ' ');
		s = g_string_append (s, argv[j]);
	      }
	    argv[0] = s->str;
	    /* Move rest of argv down */
	    for (j = 1; j < argc - i; j++)
	      argv[j] = argv[j + i];
	    argv[argc - i] = NULL;
	    argc -= i;
	    break;
	  }
    }
#endif

  if ((argc != 6) || (strcmp (argv[1], "-gimp") != 0))
    {
      g_printerr ("%s is a gimp plug-in and must be run by the gimp to be used\n", argv[0]);
      return 1;
    }

  progname = argv[0];

  basename = g_path_get_basename (progname);

  g_set_prgname (basename);

  g_free (basename);

  stack_trace_mode = (GimpStackTraceMode) CLAMP (atoi (argv[5]),
						 GIMP_STACK_TRACE_NEVER,
						 GIMP_STACK_TRACE_ALWAYS);

#ifndef G_OS_WIN32
  /* No use catching these on Win32, the user won't get any meaningful
   * stack trace from glib anyhow. It's better to let Windows inform
   * about the program error, and offer debugging if the plug-in
   * has been built with MSVC, and the user has MSVC installed.
   */
  gimp_signal_private (SIGHUP,  gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGINT,  gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGQUIT, gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGBUS,  gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGSEGV, gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGTERM, gimp_plugin_sigfatal_handler, 0);
  gimp_signal_private (SIGFPE,  gimp_plugin_sigfatal_handler, 0);

  /* Ignore SIGPIPE from crashing Gimp */
  gimp_signal_private (SIGPIPE, SIG_IGN, 0);

  /* Restart syscalls interrupted by SIGCHLD */
  gimp_signal_private (SIGCHLD, SIG_DFL, SA_RESTART);
#endif

  _readchannel  = g_io_channel_unix_new (atoi (argv[2]));
  _writechannel = g_io_channel_unix_new (atoi (argv[3]));
#ifdef __EMX__
  setmode (g_io_channel_unix_get_fd (_readchannel), O_BINARY);
  setmode (g_io_channel_unix_get_fd (_writechannel), O_BINARY);
#endif

  g_io_channel_set_encoding (_readchannel, NULL, NULL);
  g_io_channel_set_encoding (_writechannel, NULL, NULL);

  g_io_channel_set_buffered (_readchannel, FALSE);
  g_io_channel_set_buffered (_writechannel, FALSE);

  g_io_channel_set_close_on_unref (_readchannel, TRUE);
  g_io_channel_set_close_on_unref (_writechannel, TRUE);

  gp_init ();
  wire_set_writer (gimp_write);
  wire_set_flusher (gimp_flush);

  /* set handler both for the "LibGimp" and ""
     domains */

  g_log_set_handler ("LibGimp",
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_func,
		     NULL);
  g_log_set_handler (NULL,
		     G_LOG_LEVEL_MESSAGE,
		     gimp_message_func,
		     NULL);

  if (strcmp (argv[4], "-query") == 0)
    {
      if (PLUG_IN_INFO.init_proc)
        gp_has_init_write (_writechannel, NULL);
        
      if (PLUG_IN_INFO.query_proc)
	(* PLUG_IN_INFO.query_proc) ();
	
      gimp_close ();
      return 0;
    }

  if (strcmp (argv[4], "-init") == 0)
    {
      if (PLUG_IN_INFO.init_proc)
	(* PLUG_IN_INFO.init_proc) ();
	
      gimp_close ();
      return 0;
    }

  temp_proc_ht = g_hash_table_new (g_str_hash, g_str_equal);

  g_io_add_watch (_readchannel,
		  G_IO_ERR | G_IO_HUP,
		  gimp_plugin_io_error_handler,
		  NULL);

  gimp_loop ();
  return 0;
}

static void
gimp_close (void)
{
  if (PLUG_IN_INFO.quit_proc)
    (* PLUG_IN_INFO.quit_proc) ();

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
  CloseHandle (shm_handle);
#else
#ifdef HAVE_SHM_H
  if ((_shm_ID != -1) && _shm_addr)
    shmdt ((char*) _shm_addr);
#endif
#endif

  gp_quit_write (_writechannel, NULL);
}

void
gimp_quit (void)
{
  gimp_close ();
  exit (0);
}


gint32
gimp_default_display (void)
{
  return _gdisp_ID;
}

static void
gimp_message_func (const gchar    *log_domain,
		   GLogLevelFlags  log_level,
		   const gchar    *message,
		   gpointer        data)
{
  gimp_message ((gchar *) message);
}

void
gimp_install_procedure (gchar        *name,
			gchar        *blurb,
			gchar        *help,
			gchar        *author,
			gchar        *copyright,
			gchar        *date,
			gchar        *menu_path,
			gchar        *image_types,
			gint          type,
			gint          nparams,
			gint          nreturn_vals,
			GimpParamDef *params,
			GimpParamDef *return_vals)
{
  GPProcInstall proc_install;

  proc_install.name         = name;
  proc_install.blurb        = blurb;
  proc_install.help         = help;
  proc_install.author       = author;
  proc_install.copyright    = copyright;
  proc_install.date         = date;
  proc_install.menu_path    = menu_path;
  proc_install.image_types  = image_types;
  proc_install.type         = type;
  proc_install.nparams      = nparams;
  proc_install.nreturn_vals = nreturn_vals;
  proc_install.params       = (GPParamDef *) params;
  proc_install.return_vals  = (GPParamDef *) return_vals;

  if (! gp_proc_install_write (_writechannel, &proc_install, NULL))
    gimp_quit ();
}

void
gimp_install_temp_proc (gchar        *name,
			gchar        *blurb,
			gchar        *help,
			gchar        *author,
			gchar        *copyright,
			gchar        *date,
			gchar        *menu_path,
			gchar        *image_types,
			gint          type,
			gint          nparams,
			gint          nreturn_vals,
			GimpParamDef *params,
			GimpParamDef *return_vals,
			GimpRunProc   run_proc)
{
  gimp_install_procedure (name,
			  blurb, help,
			  author, copyright, date,
			  menu_path,
			  image_types,
			  type,
			  nparams, nreturn_vals,
			  params, return_vals);

  /*  Insert the temp proc run function into the hash table  */
  g_hash_table_insert (temp_proc_ht, g_strdup (name), (gpointer) run_proc);
}

void
gimp_uninstall_temp_proc (gchar *name)
{
  GPProcUninstall proc_uninstall;
  gpointer hash_name;
  gboolean found;
  proc_uninstall.name = name;

  if (! gp_proc_uninstall_write (_writechannel, &proc_uninstall, NULL))
    gimp_quit ();
  
  found = g_hash_table_lookup_extended (temp_proc_ht, name, &hash_name, NULL);
  if (found)
    {
      g_hash_table_remove (temp_proc_ht, (gpointer) name);
      g_free (hash_name);
    }
}

GimpParam *
gimp_run_procedure (gchar *name,
		    gint  *nreturn_vals,
		    ...)
{
  GPProcRun       proc_run;
  GPProcReturn   *proc_return;
  WireMessage     msg;
  GimpPDBArgType  param_type;
  GimpParam      *return_vals;
  va_list         args;
  gint            i;

  proc_run.name = name;
  proc_run.nparams = 0;
  proc_run.params = NULL;

  va_start (args, nreturn_vals);
  param_type = va_arg (args, GimpPDBArgType);

  while (param_type != GIMP_PDB_END)
    {
      switch (param_type)
	{
	case GIMP_PDB_INT32:
        case GIMP_PDB_DISPLAY:
        case GIMP_PDB_IMAGE:
        case GIMP_PDB_LAYER:
        case GIMP_PDB_CHANNEL:
        case GIMP_PDB_DRAWABLE:
        case GIMP_PDB_SELECTION:
        case GIMP_PDB_BOUNDARY:
        case GIMP_PDB_PATH:
        case GIMP_PDB_STATUS:
	  (void) va_arg (args, gint);
	  break;
	case GIMP_PDB_INT16:
	  (void) va_arg (args, gint);
	  break;
	case GIMP_PDB_INT8:
	  (void) va_arg (args, gint);
	  break;
        case GIMP_PDB_FLOAT:
          (void) va_arg (args, gdouble);
          break;
        case GIMP_PDB_STRING:
          (void) va_arg (args, gchar *);
          break;
        case GIMP_PDB_INT32ARRAY:
          (void) va_arg (args, gint32 *);
          break;
        case GIMP_PDB_INT16ARRAY:
          (void) va_arg (args, gint16 *);
          break;
        case GIMP_PDB_INT8ARRAY:
          (void) va_arg (args, gint8 *);
          break;
        case GIMP_PDB_FLOATARRAY:
          (void) va_arg (args, gdouble *);
          break;
        case GIMP_PDB_STRINGARRAY:
          (void) va_arg (args, gchar **);
          break;
        case GIMP_PDB_COLOR:
	  (void) va_arg (args, GimpRGB *);
          break;
        case GIMP_PDB_PARASITE:
          (void) va_arg (args, GimpParasite *);
          break;
        case GIMP_PDB_REGION:
          break;
	case GIMP_PDB_END:
	  break;
	}

      proc_run.nparams += 1;
      param_type = va_arg (args, GimpPDBArgType);
    }

  va_end (args);

  proc_run.params = g_new (GPParam, proc_run.nparams);

  va_start (args, nreturn_vals);

  for (i = 0; i < proc_run.nparams; i++)
    {
      proc_run.params[i].type = va_arg (args, GimpPDBArgType);

      switch (proc_run.params[i].type)
	{
	case GIMP_PDB_INT32:
	  proc_run.params[i].data.d_int32 = (gint32) va_arg (args, gint);
	  break;
	case GIMP_PDB_INT16:
	  proc_run.params[i].data.d_int16 = (gint16) va_arg (args, gint);
	  break;
	case GIMP_PDB_INT8:
	  proc_run.params[i].data.d_int8 = (gint8) va_arg (args, gint);
	  break;
        case GIMP_PDB_FLOAT:
          proc_run.params[i].data.d_float = (gdouble) va_arg (args, gdouble);
          break;
        case GIMP_PDB_STRING:
          proc_run.params[i].data.d_string = va_arg (args, gchar *);
          break;
        case GIMP_PDB_INT32ARRAY:
          proc_run.params[i].data.d_int32array = va_arg (args, gint32 *);
          break;
        case GIMP_PDB_INT16ARRAY:
          proc_run.params[i].data.d_int16array = va_arg (args, gint16 *);
          break;
        case GIMP_PDB_INT8ARRAY:
          proc_run.params[i].data.d_int8array = va_arg (args, gint8 *);
          break;
        case GIMP_PDB_FLOATARRAY:
          proc_run.params[i].data.d_floatarray = va_arg (args, gdouble *);
          break;
        case GIMP_PDB_STRINGARRAY:
          proc_run.params[i].data.d_stringarray = va_arg (args, gchar **);
          break;
        case GIMP_PDB_COLOR:
	  proc_run.params[i].data.d_color = *va_arg (args, GimpRGB *);
          break;
        case GIMP_PDB_REGION:
          break;
        case GIMP_PDB_DISPLAY:
	  proc_run.params[i].data.d_display = va_arg (args, gint32);
          break;
        case GIMP_PDB_IMAGE:
	  proc_run.params[i].data.d_image = va_arg (args, gint32);
          break;
        case GIMP_PDB_LAYER:
	  proc_run.params[i].data.d_layer = va_arg (args, gint32);
          break;
        case GIMP_PDB_CHANNEL:
	  proc_run.params[i].data.d_channel = va_arg (args, gint32);
          break;
        case GIMP_PDB_DRAWABLE:
	  proc_run.params[i].data.d_drawable = va_arg (args, gint32);
          break;
        case GIMP_PDB_SELECTION:
	  proc_run.params[i].data.d_selection = va_arg (args, gint32);
          break;
        case GIMP_PDB_BOUNDARY:
	  proc_run.params[i].data.d_boundary = va_arg (args, gint32);
          break;
        case GIMP_PDB_PATH:
	  proc_run.params[i].data.d_path = va_arg (args, gint32);
          break;
        case GIMP_PDB_PARASITE:
	  {
	    GimpParasite *parasite = va_arg (args, GimpParasite *);

	    if (parasite == NULL)
	      {
		proc_run.params[i].data.d_parasite.name = NULL;
		proc_run.params[i].data.d_parasite.data = NULL;
	      }
	    else
	      {
		proc_run.params[i].data.d_parasite.name  = parasite->name;
		proc_run.params[i].data.d_parasite.flags = parasite->flags;
		proc_run.params[i].data.d_parasite.size  = parasite->size;
		proc_run.params[i].data.d_parasite.data  = parasite->data;
	      }
	  }
	  break;
        case GIMP_PDB_STATUS:
	  proc_run.params[i].data.d_status = va_arg (args, gint32);
          break;
	case GIMP_PDB_END:
	  break;
	}
    }

  va_end (args);

  if (! gp_proc_run_write (_writechannel, &proc_run, NULL))
    gimp_quit ();

  gimp_read_expect_msg (&msg, GP_PROC_RETURN);

  proc_return = msg.data;
  *nreturn_vals = proc_return->nparams;
  return_vals = (GimpParam*) proc_return->params;

  switch (return_vals[0].data.d_status)
    {
    case GIMP_PDB_EXECUTION_ERROR:
      /*g_warning ("an execution error occured while trying to run: \"%s\"", name);*/
      break;
    case GIMP_PDB_CALLING_ERROR:
      g_warning ("a calling error occured while trying to run: \"%s\"", name);
      break;
    default:
      break;
    }

  g_free (proc_run.params);
  g_free (proc_return->name);
  g_free (proc_return);

  return return_vals;
}

void
gimp_read_expect_msg (WireMessage *msg, 
		      gint         type)
{
  while (TRUE)
    {
      if (! wire_read_msg (_readchannel, msg, NULL))
	gimp_quit ();
      
      if (msg->type != type)
	{
	  if (msg->type == GP_TEMP_PROC_RUN || msg->type == GP_QUIT)
	    {
	      gimp_process_message (msg);
	      continue;
	    }
	  else
	    g_error ("unexpected message: %d\n", msg->type);
	}
      else
	break;
    }
}


GimpParam *
gimp_run_procedure2 (gchar     *name,
		     gint      *nreturn_vals,
		     gint       nparams,
		     GimpParam *params)
{
  GPProcRun     proc_run;
  GPProcReturn *proc_return;
  WireMessage   msg;
  GimpParam    *return_vals;

  proc_run.name    = name;
  proc_run.nparams = nparams;
  proc_run.params  = (GPParam *) params;

  if (! gp_proc_run_write (_writechannel, &proc_run, NULL))
    gimp_quit ();

  gimp_read_expect_msg (&msg, GP_PROC_RETURN);
  
  proc_return = msg.data;
  *nreturn_vals = proc_return->nparams;
  return_vals = (GimpParam *) proc_return->params;

  switch (return_vals[0].data.d_status)
    {
    case GIMP_PDB_EXECUTION_ERROR:
      /*g_warning ("an execution error occured while trying to run: \"%s\"", name);*/
      break;
    case GIMP_PDB_CALLING_ERROR:
      g_warning ("a calling error occured while trying to run: \"%s\"", name);
      break;
    default:
      break;
    }

  g_free (proc_return->name);
  g_free (proc_return);

  return return_vals;
}

void
gimp_destroy_params (GimpParam *params,
		     gint       nparams)
{
  extern void _gp_params_destroy (GPParam *params, gint nparams);

  _gp_params_destroy ((GPParam*) params, nparams);
}

void
gimp_destroy_paramdefs (GimpParamDef *paramdefs,
			gint          nparams)
{
  while (nparams--)
    {
      g_free (paramdefs[nparams].name);
      g_free (paramdefs[nparams].description);
    }

  g_free (paramdefs);
}

gdouble
gimp_gamma (void)
{
  return _gamma_val;
}

gboolean
gimp_install_cmap (void)
{
  return _install_cmap;
}

gint
gimp_min_colors (void)
{
  return _min_colors;
}

static void
gimp_process_message (WireMessage *msg)
{
  switch (msg->type)
    {
    case GP_QUIT:
      gimp_quit ();
      break;
    case GP_CONFIG:
      gimp_config (msg->data);
      break;
    case GP_TILE_REQ:
    case GP_TILE_ACK:
    case GP_TILE_DATA:
      g_warning ("unexpected tile message received (should not happen)\n");
      break;
    case GP_PROC_RUN:
      g_warning ("unexpected proc run message received (should not happen)\n");
      break;
    case GP_PROC_RETURN:
      g_warning ("unexpected proc return message received (should not happen)\n");
      break;
    case GP_TEMP_PROC_RUN:
      gimp_temp_proc_run (msg->data);
      break;
    case GP_TEMP_PROC_RETURN:
      g_warning ("unexpected temp proc return message received (should not happen)\n");
      break;
    case GP_PROC_INSTALL:
      g_warning ("unexpected proc install message received (should not happen)\n");
      break;
    case GP_HAS_INIT:
      g_warning ("unexpected has init message received (should not happen)\n");
      break;
    }
}

static void 
gimp_single_message (void)
{
  WireMessage msg;

  /* Run a temp function */
  if (! wire_read_msg (_readchannel, &msg, NULL))
    gimp_quit ();

  gimp_process_message (&msg);
  
  wire_destroy (&msg);
}

void
gimp_extension_process (guint timeout)
{
#ifndef G_OS_WIN32
  fd_set readfds;
  gint   select_val;
  struct timeval  tv;
  struct timeval *tvp;

  if (timeout)
    {
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = (timeout % 1000) * 1000;
      tvp = &tv;
    }
  else
    tvp = NULL;

  FD_ZERO (&readfds);
  FD_SET (g_io_channel_unix_get_fd (_readchannel), &readfds);

  if ((select_val = select (FD_SETSIZE, &readfds, NULL, NULL, tvp)) > 0)
    {
      gimp_single_message ();
    }
  else if (select_val == -1)
    {
      perror ("gimp_process");
      gimp_quit ();
    }
#else
  /* Zero means infinite wait for us, but g_poll and
   * g_io_channel_win32_poll use -1 to indicate
   * infinite wait.
   */
  GPollFD pollfd;

  if (timeout == 0)
    timeout = -1;

  g_io_channel_win32_make_pollfd (_readchannel, G_IO_IN, &pollfd);

  if (g_io_channel_win32_poll (&pollfd, 1, timeout) == 1)
    gimp_single_message ();
#endif
}

void
gimp_extension_ack (void)
{
  /*  Send an extension initialization acknowledgement  */
  if (! gp_extension_ack_write (_writechannel, NULL))
    gimp_quit ();
}

void
gimp_run_temp (void)
{
  gimp_single_message ();
}

gchar *
gimp_get_progname (void)
{
  return progname;
}

/**
 * gimp_attach_new_parasite:
 * @name: the name of the #GimpParasite to create and attach.
 * @flags: the flags set on the #GimpParasite.
 * @size: the size of the parasite data in bytes.
 * @data: a pointer to the data attached with the #GimpParasite.
 *
 * Convenience function that creates a parasite and attaches it
 * to the GIMP.
 *
 * See Also: gimp_parasite_attach()
 */
 

void
gimp_attach_new_parasite (const gchar    *name, 
			  gint            flags,
			  gint            size, 
			  const gpointer  data)
{
  GimpParasite *parasite = gimp_parasite_new (name, flags, size, data);

  gimp_parasite_attach (parasite);

  gimp_parasite_free (parasite);
}

#ifndef G_OS_WIN32
static void
gimp_plugin_sigfatal_handler (gint sig_num)
{
  switch (sig_num)
    {
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGABRT:
    case SIGTERM:
      g_printerr ("%s terminated: %s\n", progname, g_strsignal (sig_num));
      break;

    case SIGBUS:
    case SIGSEGV:
    case SIGFPE:
    case SIGPIPE:
    default:
      g_printerr ("%s: fatal error: %s\n", progname, g_strsignal (sig_num));
      switch (stack_trace_mode)
	{
	case GIMP_STACK_TRACE_NEVER:
	  break;

	case GIMP_STACK_TRACE_QUERY:
	  {
	    sigset_t sigset;

	    sigemptyset (&sigset);
	    sigprocmask (SIG_SETMASK, &sigset, NULL);
	    g_on_error_query (progname);
	  }
	  break;

	case GIMP_STACK_TRACE_ALWAYS:
	  {
	    sigset_t sigset;

	    sigemptyset (&sigset);
	    sigprocmask (SIG_SETMASK, &sigset, NULL);
	    g_on_error_stack_trace (progname);
	  }
	  break;
	}
      break;
    }

  gimp_quit ();
}
#endif

static gboolean
gimp_plugin_io_error_handler (GIOChannel   *channel,
			      GIOCondition  cond,
			      gpointer      data)
{
  g_printerr ("%s: fatal error: GIMP crashed\n", progname);
  gimp_quit ();

  /* never reached */
  return TRUE;
}

static gboolean
gimp_write (GIOChannel *channel, 
	    guint8     *buf, 
	    gulong      count,
            gpointer    user_data)
{
  gulong bytes;

  while (count > 0)
    {
      if ((write_buffer_index + count) >= WRITE_BUFFER_SIZE)
	{
	  bytes = WRITE_BUFFER_SIZE - write_buffer_index;
	  memcpy (&write_buffer[write_buffer_index], buf, bytes);
	  write_buffer_index += bytes;
	  if (! wire_flush (channel, NULL))
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

static gboolean
gimp_flush (GIOChannel *channel,
            gpointer    user_data)
{
  GIOStatus  status;
  GError    *error = NULL;
  gsize      count;
  gsize      bytes;

  if (write_buffer_index > 0)
    {
      count = 0;
      while (count != write_buffer_index)
        {
	  do
	    {
	      bytes = 0;
	      status = g_io_channel_write_chars (channel,
						 &write_buffer[count],
						 (write_buffer_index - count),
						 &bytes,
						 &error);
	    }
	  while (status == G_IO_STATUS_AGAIN);

	  if (status != G_IO_STATUS_NORMAL)
	    {
	      if (error)
		{
		  g_warning ("%s: gimp_flush(): error: %s",
			     g_get_prgname (), error->message);
		  g_error_free (error);
		}
	      else
		{
		  g_warning ("%s: gimp_flush(): error",
			     g_get_prgname ());
		}

	      return FALSE;
	    }

          count += bytes;
        }

      write_buffer_index = 0;
    }

  return TRUE;
}

static void
gimp_loop (void)
{
  WireMessage msg;

  while (TRUE)
    {
      if (! wire_read_msg (_readchannel, &msg, NULL))
        {
	  gimp_close ();
          return;
        }

      switch (msg.type)
	{
	case GP_QUIT:
	  gimp_close ();
	  return;
	case GP_CONFIG:
	  gimp_config (msg.data);
	  break;
	case GP_TILE_REQ:
	case GP_TILE_ACK:
	case GP_TILE_DATA:
	  g_warning ("unexpected tile message received (should not happen)\n");
	  break;
	case GP_PROC_RUN:
	  gimp_proc_run (msg.data);
	  gimp_close ();
          return;
	case GP_PROC_RETURN:
	  g_warning ("unexpected proc return message received (should not happen)\n");
	  break;
	case GP_TEMP_PROC_RUN:
	  g_warning ("unexpected temp proc run message received (should not happen\n");
	  break;
	case GP_TEMP_PROC_RETURN:
	  g_warning ("unexpected temp proc return message received (should not happen\n");
	  break;
	case GP_PROC_INSTALL:
	  g_warning ("unexpected proc install message received (should not happen)\n");
	  break;
	case GP_HAS_INIT:
	  g_warning ("unexpected has init message received (should not happen)\n");
	  break;
	}

      wire_destroy (&msg);
    }
}

static void
gimp_config (GPConfig *config)
{
  extern gint _gimp_tile_width;
  extern gint _gimp_tile_height;

  if (config->version < GP_VERSION)
    {
      g_message ("Could not execute plug-in \"%s\"\n(%s)"
		 "because the GIMP is using an older version of the "
		 "plug-in protocol.",
		 g_get_prgname (), progname);
      gimp_quit ();
    }
  else if (config->version > GP_VERSION)
    {
      g_message ("Could not execute plug-in \"%s\"\n(%s)"
		 "because it uses an obsolete version of the " 
		 "plug-in protocol.",
		 g_get_prgname (), progname);
      gimp_quit ();
    }

  _gimp_tile_width  = config->tile_width;
  _gimp_tile_height = config->tile_height;
  _shm_ID           = config->shm_ID;
  _gamma_val        = config->gamma;
  _install_cmap     = config->install_cmap;
  _min_colors       = config->min_colors;
  _gdisp_ID         = config->gdisp_ID;

  if (_shm_ID != -1)
    {
#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
      /*
       * Use Win32 shared memory mechanisms for
       * transfering tile data
       */
      gchar fileMapName[128];
      gint  tileByteSize = _gimp_tile_width * _gimp_tile_height * 4;

      /* From the id, derive the file map name */
      g_snprintf (fileMapName, sizeof (fileMapName), "GIMP%d.SHM", _shm_ID);

      /* Open the file mapping */
      shm_handle = OpenFileMapping (FILE_MAP_ALL_ACCESS,
				    0, fileMapName);
      if (shm_handle)
	{
	  /* Map the shared memory into our address space for use */
	  _shm_addr = (guchar *) MapViewOfFile (shm_handle,
						FILE_MAP_ALL_ACCESS,
						0, 0, tileByteSize);

	  /* Verify that we mapped our view */
	  if (!_shm_addr)
	    {
	      g_warning ("MapViewOfFile error: %d... disabling shared memory transport",
			 GetLastError());
	    }
	}
      else
	{
	  g_warning ("OpenFileMapping error: %d... disabling shared memory transport",
		     GetLastError());
	}
#else
#ifdef HAVE_SHM_H
      _shm_addr = (guchar*) shmat (_shm_ID, 0, 0);

      if (_shm_addr == (guchar*) -1)
	g_error ("could not attach to gimp shared memory segment\n");
#endif
#endif
    }
}

static void
gimp_proc_run (GPProcRun *proc_run)
{
  GPProcReturn  proc_return;
  GimpParam    *return_vals;
  gint          nreturn_vals;

  if (PLUG_IN_INFO.run_proc)
    {
      (* PLUG_IN_INFO.run_proc) (proc_run->name,
				 proc_run->nparams,
				 (GimpParam*) proc_run->params,
				 &nreturn_vals,
				 &return_vals);

      proc_return.name = proc_run->name;
      proc_return.nparams = nreturn_vals;
      proc_return.params = (GPParam*) return_vals;

      if (! gp_proc_return_write (_writechannel, &proc_return, NULL))
	gimp_quit ();
    }
}

static void
gimp_temp_proc_run (GPProcRun *proc_run)
{
  GimpParam   *return_vals;
  gint         nreturn_vals;
  GimpRunProc  run_proc;

  run_proc = (GimpRunProc) g_hash_table_lookup (temp_proc_ht, 
						(gpointer) proc_run->name);

  if (run_proc)
    {
      (* run_proc) (proc_run->name,
		    proc_run->nparams,
		    (GimpParam*) proc_run->params,
		    &nreturn_vals,
		    &return_vals);

      /* No longer a return message */
/*       proc_return.name = proc_run->name; */
/*       proc_return.nparams = nreturn_vals; */
/*       proc_return.params = (GPParam*) return_vals; */

/*       if (! gp_temp_proc_return_write (_writechannel, &proc_return, NULL)) */
/* 	gimp_quit (); */
    }
}
