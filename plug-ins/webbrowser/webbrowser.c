/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Misha Dynin
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
	Web Browser v0.3 -- opens a URL in Netscape

	    by Misha Dynin <misha@xcf.berkeley.edu>

	For more information, see webbrowser.readme, as well as

	    http://www.xcf.berkeley.edu/~misha/gimp/

 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <ctype.h>
#ifdef __EMX__
#include <process.h>
#endif

#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#include <shellapi.h>
#else
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmu/WinUtil.h>	/* for XmuClientWindow() */
#endif

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


/* Browser program name -- start in case it's not already running */
#define BROWSER_PROGNAME	"netscape"

/* Open a new window with request? */
#define OPEN_URL_NEW_WINDOW	1
#define OPEN_URL_CURRENT_WINDOW	0

#define URL_BUF_SIZE 255


static void query (void);
static void run   (gchar   *name,
		   gint     nparams,
		   GParam  *param,
		   gint    *nreturn_vals,
		   GParam **return_vals);

static gint open_url_dialog     (void);
static void ok_callback         (GtkWidget *widget,
				 gpointer   data);
static void about_callback      (GtkWidget *widget,
				 gpointer   data);
static void new_window_callback (GtkWidget *widget,
				 gpointer   data);
static void url_callback        (GtkWidget *widget,
				 gpointer   data);
#ifndef G_OS_WIN32
static gint mozilla_remote      (gchar     *command);
#endif

static gint open_url            (gchar     *url,
				 gint       new_window);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

typedef struct
{
  gchar url[URL_BUF_SIZE];
  gint  new_window;
} u_info;

static u_info url_info =
{
  "http://www.gimp.org/",  /* Default URL */
  OPEN_URL_NEW_WINDOW,     /* Change to ...CURRENT_WINDOW if
			    * you prefer that as the default
			    */
};

static gboolean run_flag = FALSE;

MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
    { PARAM_STRING, "url", "URL of a document to open" },
    { PARAM_INT32,  "new_window", "Create a new window or use existing one?" },
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("extension_web_browser",
			  "open URL in Netscape",
			  "You need to have Netscape installed",
			  "Misha Dynin <misha@xcf.berkeley.edu>",
			  "Misha Dynin, Jamie Zawinski, Spencer Kimball & Peter Mattis",
			  "1997",
			  N_("<Toolbox>/Xtns/Web Browser/Open URL..."),
			  NULL,
			  PROC_EXTENSION,
			  nargs, 0,
			  args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam values[1];
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  if (strcmp (name, "extension_web_browser") == 0)
    {
      switch (run_mode)
	{
	case RUN_INTERACTIVE:
	  INIT_I18N_UI ();
	  /* Possibly retrieve data */
	  gimp_get_data ("extension_web_browser", &url_info);

	  if (!open_url_dialog ())
	    return;
	  break;

	case RUN_NONINTERACTIVE:
	  /*  Make sure all the arguments are there!  */
	  if (nparams != 3)
	    {
	      status = STATUS_CALLING_ERROR;
	    }
	  else
	    {
	      strncpy (url_info.url, param[1].data.d_string, URL_BUF_SIZE);
	      url_info.new_window = param[2].data.d_int32;
	    }
	  break;

	case RUN_WITH_LAST_VALS:
	  gimp_get_data ("extension_web_browser", &url_info);
	  break;

	default:
	  break;
	}

      if (status == STATUS_SUCCESS)
	{
	  if (!open_url (url_info.url, url_info.new_window))
	    values[0].data.d_status = STATUS_EXECUTION_ERROR;

	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("extension_web_browser", &url_info, sizeof (u_info));

	  values[0].data.d_status = STATUS_SUCCESS;
	}
      else
	values[0].data.d_status = STATUS_EXECUTION_ERROR;
    }
  else
    g_assert_not_reached ();
}

static gint
start_browser (gchar *prog,
	       gchar *url)
{
#ifdef G_OS_WIN32

  ShellExecute (HWND_DESKTOP, "open", url, NULL, "C:\\", SW_SHOWNORMAL);

#else
#ifndef __EMX__
  pid_t cpid;

  if ((cpid = fork()) == 0)
    {
      execlp (prog, prog, url, NULL);
      exit (1);
    }

  return (cpid > 0);
#else
  return (spawnlp (P_NOWAIT, prog, prog, url, NULL) != -1);
#endif
#endif /* !G_OS_WIN32 */
}

static gint
open_url (gchar *url,
	  gint   new_window)
{
  gchar buf[512];

  while (isspace (*url))
    ++url;

#ifndef G_OS_WIN32
  sprintf (buf, "openURL(%s%s)", url, new_window ? ",new-window" : "");

  if (mozilla_remote (buf))
    return (TRUE);
#endif
  return (start_browser (BROWSER_PROGNAME, url));
}

static gint
open_url_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *table;
  GSList    *group;

  gint    argc;
  gchar **argv;
  gchar   buffer[256];

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("webbrowser");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  dlg = gimp_dialog_new (_("Open URL"), "webbbrowser",
			 gimp_plugin_help_func, "filters/webbrowser.html",
			 GTK_WIN_POS_MOUSE,
			 FALSE, TRUE, FALSE,

			 _("About"), about_callback,
			 NULL, NULL, NULL, FALSE, FALSE,
			 _("OK"), ok_callback,
			 NULL, NULL, NULL, TRUE, FALSE,
			 _("Cancel"), gtk_widget_destroy,
			 NULL, 1, NULL, FALSE, TRUE,

			 NULL);

  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit),
		      NULL);

  /* table */
  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* URL */
  entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, 200, 0);
  g_snprintf (buffer, sizeof (buffer), "%s", url_info.url);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
			     _("URL:"), 1.0, 0.5,
			     entry, 1, FALSE);
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      GTK_SIGNAL_FUNC (url_callback),
		      &url_info.url);
  gtk_widget_show (entry);

  /* Window */
  hbox = gtk_hbox_new (FALSE, 4);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
			     _("Window:"), 1.0, 0.5,
			     hbox, 1, FALSE);

  button = gtk_radio_button_new_with_label (NULL, _("New"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (url_info.new_window == OPEN_URL_NEW_WINDOW)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (new_window_callback),
		      (gpointer) OPEN_URL_NEW_WINDOW);
  gtk_widget_show (button);

  button = gtk_radio_button_new_with_label (group, _("Current"));
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
  if (url_info.new_window == OPEN_URL_CURRENT_WINDOW)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      GTK_SIGNAL_FUNC (new_window_callback),
		      (gpointer) OPEN_URL_CURRENT_WINDOW);
  gtk_widget_show (button);

  gtk_widget_show (dlg);

  gtk_main ();
  gdk_flush ();

  return run_flag;
}

static void
ok_callback (GtkWidget *widget,
	     gpointer   data)
{
  run_flag = TRUE;

  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
about_callback (GtkWidget *widget,
		gpointer   data)
{
  open_url ("http://www.xcf.berkeley.edu/~misha/gimp/", OPEN_URL_NEW_WINDOW);
}

static void
new_window_callback (GtkWidget *widget,
		     gpointer   data)
{
  if (GTK_TOGGLE_BUTTON (widget)->active)
    {
      url_info.new_window = (gint) data;
    }
}

static void
url_callback (GtkWidget *widget,
	      gpointer   data)
{
  strncpy (url_info.url, gtk_entry_get_text (GTK_ENTRY (widget)), URL_BUF_SIZE);
}

#ifndef G_OS_WIN32

/* -*- Mode:C; tab-width: 8 -*-
 * remote.c --- remote control of Netscape Navigator for Unix.
 * version 1.1.3, for Netscape Navigator 1.1 and newer.
 *
 * Copyright © 1996 Netscape Communications Corporation, all rights reserved.
 * Created: Jamie Zawinski <jwz@netscape.com>, 24-Dec-94.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * To compile:
 *
 *    cc -o netscape-remote remote.c -DSTANDALONE -lXmu -lX11
 *
 * To use:
 *
 *    netscape-remote -help
 *
 * Documentation for the protocol which this code implements may be found at:
 *
 *    http://home.netscape.com/newsref/std/x-remote.html
 *
 * Bugs and commentary to x_cbug@netscape.com.
 */



/* vroot.h is a header file which lets a client get along with `virtual root'
   window managers like swm, tvtwm, olvwm, etc.  If you don't have this header
   file, you can find it at "http://home.netscape.com/newsref/std/vroot.h".
   If you don't care about supporting virtual root window managers, you can
   comment this line out.
 */
/* #include "vroot.h" */
/* Begin vroot.h */
/*****************************************************************************/
/**                   Copyright 1991 by Andreas Stolcke                     **/
/**               Copyright 1990 by Solbourne Computer Inc.                 **/
/**                          Longmont, Colorado                             **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    name of Solbourne not be used in advertising                         **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    ANDREAS STOLCKE AND SOLBOURNE COMPUTER INC. DISCLAIMS ALL WARRANTIES **/
/**    WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF    **/
/**    MERCHANTABILITY  AND  FITNESS,  IN  NO  EVENT SHALL ANDREAS STOLCKE  **/
/**    OR SOLBOURNE BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL    **/
/**    DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA   **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/
/*
 * vroot.h -- Virtual Root Window handling header file
 *
 * This header file redefines the X11 macros RootWindow and DefaultRootWindow,
 * making them look for a virtual root window as provided by certain `virtual'
 * window managers like swm and tvtwm. If none is found, the ordinary root
 * window is returned, thus retaining backward compatibility with standard
 * window managers.
 * The function implementing the virtual root lookup remembers the result of
 * its last invocation to avoid overhead in the case of repeated calls
 * on the same display and screen arguments. 
 * The lookup code itself is taken from Tom LaStrange's ssetroot program.
 *
 * Most simple root window changing X programs can be converted to using
 * virtual roots by just including
 *
 * #include <X11/vroot.h>
 *
 * after all the X11 header files.  It has been tested on such popular
 * X clients as xphoon, xfroot, xloadimage, and xaqua.
 * It also works with the core clients xprop, xwininfo, xwd, and editres
 * (and is necessary to get those clients working under tvtwm).
 * It does NOT work with xsetroot; get the xsetroot replacement included in
 * the tvtwm distribution instead.
 *
 * Andreas Stolcke <stolcke@ICSI.Berkeley.EDU>, 9/7/90
 * - replaced all NULL's with properly cast 0's, 5/6/91
 * - free children list (suggested by Mark Martin <mmm@cetia.fr>), 5/16/91
 * - include X11/Xlib.h and support RootWindowOfScreen, too 9/17/91
 */

#ifndef _VROOT_H_
#define _VROOT_H_


static Window
VirtualRootWindowOfScreen(screen)
	Screen *screen;
{
	static Screen *save_screen = (Screen *)0;
	static Window root = (Window)0;

	if (screen != save_screen) {
		Display *dpy = DisplayOfScreen(screen);
		Atom __SWM_VROOT = None;
		int i;
		Window rootReturn, parentReturn, *children;
		unsigned int numChildren;

		root = RootWindowOfScreen(screen);

		/* go look for a virtual root */
		__SWM_VROOT = XInternAtom(dpy, "__SWM_VROOT", False);
		if (XQueryTree(dpy, root, &rootReturn, &parentReturn,
				 &children, &numChildren)) {
			for (i = 0; i < numChildren; i++) {
				Atom actual_type;
				int actual_format;
				unsigned long nitems, bytesafter;
				Window *newRoot = (Window *)0;

				if (XGetWindowProperty(dpy, children[i],
					__SWM_VROOT, 0, 1, False, XA_WINDOW,
					&actual_type, &actual_format,
					&nitems, &bytesafter,
					(unsigned char **) &newRoot) == Success
				    && newRoot) {
				    root = *newRoot;
				    break;
				}
			}
			if (children)
				XFree((char *)children);
		}

		save_screen = screen;
	}

	return root;
}

#undef RootWindowOfScreen
#define RootWindowOfScreen(s) VirtualRootWindowOfScreen(s)

#undef RootWindow
#define RootWindow(dpy,screen) VirtualRootWindowOfScreen(ScreenOfDisplay(dpy,screen))

#undef DefaultRootWindow
#define DefaultRootWindow(dpy) VirtualRootWindowOfScreen(DefaultScreenOfDisplay(dpy))

#endif /* _VROOT_H_ */
/* End vroot.h */


#ifdef DIAGNOSTIC
#ifdef STANDALONE
 static const char *progname = 0;
 static const char *expected_mozilla_version = "1.1";
#else  /* !STANDALONE */
 extern const char *progname;
 extern const char *expected_mozilla_version;
#endif /* !STANDALONE */
#endif /* DIAGNOSTIC */

#define MOZILLA_VERSION_PROP   "_MOZILLA_VERSION"
#define MOZILLA_LOCK_PROP      "_MOZILLA_LOCK"
#define MOZILLA_COMMAND_PROP   "_MOZILLA_COMMAND"
#define MOZILLA_RESPONSE_PROP  "_MOZILLA_RESPONSE"
static Atom XA_MOZILLA_VERSION  = 0;
static Atom XA_MOZILLA_LOCK     = 0;
static Atom XA_MOZILLA_COMMAND  = 0;
static Atom XA_MOZILLA_RESPONSE = 0;

static void
mozilla_remote_init_atoms (Display *dpy)
{
  if (! XA_MOZILLA_VERSION)
    XA_MOZILLA_VERSION = XInternAtom (dpy, MOZILLA_VERSION_PROP, False);
  if (! XA_MOZILLA_LOCK)
    XA_MOZILLA_LOCK = XInternAtom (dpy, MOZILLA_LOCK_PROP, False);
  if (! XA_MOZILLA_COMMAND)
    XA_MOZILLA_COMMAND = XInternAtom (dpy, MOZILLA_COMMAND_PROP, False);
  if (! XA_MOZILLA_RESPONSE)
    XA_MOZILLA_RESPONSE = XInternAtom (dpy, MOZILLA_RESPONSE_PROP, False);
}

static Window
mozilla_remote_find_window (Display *dpy)
{
  int i;
  Window root = RootWindowOfScreen (DefaultScreenOfDisplay (dpy));
  Window root2, parent, *kids;
  unsigned int nkids;
  Window result = 0;

  if (! XQueryTree (dpy, root, &root2, &parent, &kids, &nkids))
    {
#ifdef DIAGNOSTIC
      fprintf (stderr, "%s: XQueryTree failed on display %s\n", progname,
	       DisplayString (dpy));
#endif
      return (0);
    }

  /* root != root2 is possible with virtual root WMs. */

  if (! (kids && nkids))
    {
#ifdef DIAGNOSTIC
      fprintf (stderr, "%s: root window has no children on display %s\n",
	       progname, DisplayString (dpy));
#endif
      return (0);
    }

  for (i = nkids-1; i >= 0; i--)
    {
      Atom type;
      int format;
      unsigned long nitems, bytesafter;
      unsigned char *version = 0;
      Window w = XmuClientWindow (dpy, kids[i]);
      int status = XGetWindowProperty (dpy, w, XA_MOZILLA_VERSION,
				       0, (65536 / sizeof (long)),
				       False, XA_STRING,
				       &type, &format, &nitems, &bytesafter,
				       &version);
      if (! version)
	continue;

      XFree (version);
      if (status == Success && type != None)
	{
	  result = w;
	  break;
	}
    }

  return result;
}

static char *lock_data = 0;

static int
mozilla_remote_obtain_lock (Display *dpy, Window window)
{
  Bool locked = False;
  Bool waited = False;

  if (! lock_data)
    {
      lock_data = (char *) malloc (255);
      sprintf (lock_data, "pid%d@", getpid ());
      if (gethostname (lock_data + strlen (lock_data), 100))
	{
	  return (-1);
	}
    }

  do
    {
      int result;
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytes_after;
      unsigned char *data = 0;

      XGrabServer (dpy);   /* ################################# DANGER! */

      result = XGetWindowProperty (dpy, window, XA_MOZILLA_LOCK,
				   0, (65536 / sizeof (long)),
				   False, /* don't delete */
				   XA_STRING,
				   &actual_type, &actual_format,
				   &nitems, &bytes_after,
				   &data);
      if (result != Success || actual_type == None)
	{
	  /* It's not now locked - lock it. */
#ifdef DEBUG_PROPS
	  fprintf (stderr, "%s: (writing " MOZILLA_LOCK_PROP
		   " \"%s\" to 0x%x)\n",
		   progname, lock_data, (unsigned int) window);
#endif
	  XChangeProperty (dpy, window, XA_MOZILLA_LOCK, XA_STRING, 8,
			   PropModeReplace, (unsigned char *) lock_data,
			   strlen (lock_data));
	  locked = True;
	}

      XUngrabServer (dpy); /* ################################# danger over */
      XSync (dpy, False);

      if (! locked)
	{
	  /* We tried to grab the lock this time, and failed because someone
	     else is holding it already.  So, wait for a PropertyDelete event
	     to come in, and try again. */

#ifdef DIAGNOSTIC
	  fprintf (stderr, "%s: window 0x%x is locked by %s; waiting...\n",
		   progname, (unsigned int) window, data);
#endif
	  waited = True;

	  while (1)
	    {
	      XEvent event;
	      XNextEvent (dpy, &event);
	      if (event.xany.type == DestroyNotify &&
		  event.xdestroywindow.window == window)
		{
#ifdef DIAGNOSTIC
		  fprintf (stderr, "%s: window 0x%x unexpectedly destroyed.\n",
			   progname, (unsigned int) window);
#endif
		  return (6);
		}
	      else if (event.xany.type == PropertyNotify &&
		       event.xproperty.state == PropertyDelete &&
		       event.xproperty.window == window &&
		       event.xproperty.atom == XA_MOZILLA_LOCK)
		{
		  /* Ok!  Someone deleted their lock, so now we can try
		     again. */
#ifdef DEBUG_PROPS
		  fprintf (stderr, "%s: (0x%x unlocked, trying again...)\n",
			   progname, (unsigned int) window);
#endif
		  break;
		}
	    }
	}
      if (data)
	XFree (data);
    }
  while (! locked);

#ifdef DIAGNOSTIC
  if (waited)
    fprintf (stderr, "%s: obtained lock.\n", progname);
#endif
  return (0);
}


static void
mozilla_remote_free_lock (Display *dpy, Window window)
{
  int result;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = 0;

#ifdef DEBUG_PROPS
	  fprintf (stderr, "%s: (deleting " MOZILLA_LOCK_PROP
		   " \"%s\" from 0x%x)\n",
		   progname, lock_data, (unsigned int) window);
#endif

  result = XGetWindowProperty (dpy, window, XA_MOZILLA_LOCK,
			       0, (65536 / sizeof (long)),
			       True, /* atomic delete after */
			       XA_STRING,
			       &actual_type, &actual_format,
			       &nitems, &bytes_after,
			       &data);
  if (result != Success)
    {
#ifdef DIAGNOSTIC
      fprintf (stderr, "%s: unable to read and delete " MOZILLA_LOCK_PROP
	       " property\n",
	       progname);
#endif
      return;
    }
  else if (!data || !*data)
    {
#ifdef DIAGNOSTIC
      fprintf (stderr, "%s: invalid data on " MOZILLA_LOCK_PROP
	       " of window 0x%x.\n",
	       progname, (unsigned int) window);
#endif
      return;
    }
  else if (strcmp ((char *) data, lock_data))
    {
#ifdef DIAGNOSTIC
      fprintf (stderr, "%s: " MOZILLA_LOCK_PROP
	       " was stolen!  Expected \"%s\", saw \"%s\"!\n",
	       progname, lock_data, data);
#endif
      return;
    }

  if (data)
    XFree (data);
}


static int
mozilla_remote_command (Display *dpy, Window window, const char *command)
{
  int result = 0;
  Bool done = False;

#ifdef DEBUG_PROPS
  fprintf (stderr, "%s: (writing " MOZILLA_COMMAND_PROP " \"%s\" to 0x%x)\n",
	   progname, command, (unsigned int) window);
#endif

  XChangeProperty (dpy, window, XA_MOZILLA_COMMAND, XA_STRING, 8,
		   PropModeReplace, (unsigned char *) command,
		   strlen (command));

  while (!done)
    {
      XEvent event;
      XNextEvent (dpy, &event);
      if (event.xany.type == DestroyNotify &&
	  event.xdestroywindow.window == window)
	{
	  /* Print to warn user...*/
#ifdef DIAGNOSTIC
	  fprintf (stderr, "%s: window 0x%x was destroyed.\n",
		   progname, (unsigned int) window);
#endif
	  result = 6;
	  goto DONE;
	}
      else if (event.xany.type == PropertyNotify &&
	       event.xproperty.state == PropertyNewValue &&
	       event.xproperty.window == window &&
	       event.xproperty.atom == XA_MOZILLA_RESPONSE)
	{
	  Atom actual_type;
	  int actual_format;
	  unsigned long nitems, bytes_after;
	  unsigned char *data = 0;

	  result = XGetWindowProperty (dpy, window, XA_MOZILLA_RESPONSE,
				       0, (65536 / sizeof (long)),
				       True, /* atomic delete after */
				       XA_STRING,
				       &actual_type, &actual_format,
				       &nitems, &bytes_after,
				       &data);
#ifdef DEBUG_PROPS
	  if (result == Success && data && *data)
	    {
	      fprintf (stderr, "%s: (server sent " MOZILLA_RESPONSE_PROP
		       " \"%s\" to 0x%x.)\n",
		       progname, data, (unsigned int) window);
	    }
#endif

	  if (result != Success)
	    {
#ifdef DIAGNOSTIC
	      fprintf (stderr, "%s: failed reading " MOZILLA_RESPONSE_PROP
		       " from window 0x%0x.\n",
		       progname, (unsigned int) window);
#endif
	      result = 6;
	      done = True;
	    }
	  else if (!data || strlen((char *) data) < 5)
	    {
#ifdef DIAGNOSTIC
	      fprintf (stderr, "%s: invalid data on " MOZILLA_RESPONSE_PROP
		       " property of window 0x%0x.\n",
		       progname, (unsigned int) window);
#endif
	      result = 6;
	      done = True;
	    }
	  else if (*data == '1')	/* positive preliminary reply */
	    {
#ifdef DIAGNOSTIC
	      fprintf (stderr, "%s: %s\n", progname, data + 4);
#endif
	      /* keep going */
	      done = False;
	    }
#if 1
	  else if (!strncmp ((char *)data, "200", 3)) /* positive completion */
	    {
	      result = 0;
	      done = True;
	    }
#endif
	  else if (*data == '2')		/* positive completion */
	    {
#ifdef DIAGNOSTIC
	      fprintf (stderr, "%s: %s\n", progname, data + 4);
#endif
	      result = 0;
	      done = True;
	    }
	  else if (*data == '3')	/* positive intermediate reply */
	    {
#ifdef DIAGNOSTIC
	      fprintf (stderr, "%s: internal error: "
		       "server wants more information?  (%s)\n",
		       progname, data);
#endif
	      result = 3;
	      done = True;
	    }
	  else if (*data == '4' ||	/* transient negative completion */
		   *data == '5')	/* permanent negative completion */
	    {
#ifdef DIAGNOSTIC
	      fprintf (stderr, "%s: %s\n", progname, data + 4);
#endif
	      result = (*data - '0');
	      done = True;
	    }
	  else
	    {
#ifdef DIAGNOSTIC
	      fprintf (stderr,
		       "%s: unrecognised " MOZILLA_RESPONSE_PROP
		       " from window 0x%x: %s\n",
		       progname, (unsigned int) window, data);
#endif
	      result = 6;
	      done = True;
	    }

	  if (data)
	    XFree (data);
	}
#ifdef DEBUG_PROPS
      else if (event.xany.type == PropertyNotify &&
	       event.xproperty.window == window &&
	       event.xproperty.state == PropertyDelete &&
	       event.xproperty.atom == XA_MOZILLA_COMMAND)
	{
	  fprintf (stderr, "%s: (server 0x%x has accepted "
		   MOZILLA_COMMAND_PROP ".)\n",
		   progname, (unsigned int) window);
	}
#endif /* DEBUG_PROPS */
    }

 DONE:

  return result;
}

static int
mozilla_remote (char *command)
{
  int status = 0;
  Display *dpy;
  Window window;

  if (!(dpy = XOpenDisplay (NULL)))
    return (0);

  mozilla_remote_init_atoms (dpy);

  if (!(window = mozilla_remote_find_window (dpy)))
    return (0);

  XSelectInput (dpy, window, (PropertyChangeMask|StructureNotifyMask));

  if (mozilla_remote_obtain_lock (dpy, window))
    return (0);

  status = mozilla_remote_command (dpy, window, command);

  /* When status = 6, it means the window has been destroyed */
  /* It is invalid to free the lock when window is destroyed. */

  if ( status != 6 )
    mozilla_remote_free_lock (dpy, window);

  return (!status);
}

#endif /* !G_OS_WIN32 */
