/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
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
 *
 * $Revision$
 */                                                                             

#include "gimpsignal.h"

/** 
 * gimp_signal: 
 * @signum:   selects signal to be handled see man 5 signal
 * @handler:  handler that maps to signum. Invoked by O/S. 
 *            handler gets signal that caused invocation. 
 * @sa_flags: preferences. OR'ed SA_<xxx>. See signal.h 
 *
 * This function furnishes a workalike for signal(2) but
 * which internally invokes sigaction(2) after certain
 * sa_flags are set; these primarily to ensure restarting
 * of interrupted system calls. See sigaction(2)  It is a 
 * aid to transition and not new development: that effort 
 * should employ sigaction directly. [<gosgood@idt.net> 18.04.2000] 
 *
 * Cause handler to be run when signum is delivered.  We
 * use sigaction(2) rather than signal(2) so that we can control the
 * signal hander's environment completely via sa_flags: some signal(2)
 * implementations differ in their sematics, so we need to nail down
 * exactly what we want. [<austin@gimp.org> 06.04.2000]
 *
 * Returns: RetSigType (a reference to a signal handling function)  
 *
 */

/* Courtesy of Austin Donnelly 06-04-2000 to address bug #2742 */

RetSigType
gimp_signal_private (gint signum, void (*gimp_sighandler)(int), gint sa_flags)
{
  int ret;
  struct sigaction sa;
  struct sigaction osa;

  /* this field is a union of sa_sighandler.sa_sighandler1 and */
  /* sa_sigaction1 - don't set both at once...                 */

  sa.sa_handler   = gimp_sighandler;

  /* Mask all signals while handler runs to avoid re-entrancy
   * problems. */
  sigfillset (&sa.sa_mask);

  sa.sa_flags = sa_flags;

  ret = sigaction (signum, &sa, &osa);
  if (ret < 0)
    gimp_fatal_error ("unable to set handler for signal %d\n", signum);
  return osa.sa_handler;
}
