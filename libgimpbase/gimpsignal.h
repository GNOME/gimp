/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 *  $Revision$
 */
#ifndef __GIMP_SIGNAL_H__
#define __GIMP_SIGNAL_H__

/* A gimp-level interface to a Posix.1-compliant signal package lives here
 * For 1.2, this gimp-level interface mostly passes through to posix calls
 * without modification. Certain calls manipulate struct sigaction in
 * ways useful to Gimp.
 */

#include <signal.h>

#include <glib.h>

#ifdef __EMX__
/* hope this is right for OS/2 */
#define SA_RESTART SA_SYSV
#endif

/* GimpSignalHandlerFunc is a reference to a (signal handler) function 
 * that takes a signal ID and returns void. 
 * signal(2) returns such references; so does gimp_signal_private.
 */
typedef void (* GimpSignalHandlerFunc) (gint signum);

/* Internal implementation that can be DEFINEd into various flavors of
 * signal(2) lookalikes.
 */
GimpSignalHandlerFunc  gimp_signal_private (gint                   signum,
					    GimpSignalHandlerFunc  handler,
					    gint                   flags);

/* the gimp_signal_syscallrestart() lookalike looks like signal(2) but
 * quietly requests the restarting of system calls. Addresses #2742
 */
#define gimp_signal_syscallrestart(signum,handler) gimp_signal_private ((signum), (handler), SA_RESTART)

#endif /* __GIMP_SIGNAL_H__ */
