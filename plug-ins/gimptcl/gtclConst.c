/***************************************************
 * file: gtclConst.c
 *
 * Copyright (c) 1996 Eric L. Hernes (erich@rrnet.com)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */

#include <stdlib.h>
#include <tcl.h>

#include <glib.h>
#include <libgimp/gimp.h>

#include "gtcl.h"
#include "gtclEnums.h"

#include <stdio.h>

char* GtclConst="GtclConst";

#define Gtcl_CAdd(c)  sprintf(t, "%d", c);                                  \
                      sprintf(t1, "%s", # c);                               \
                      Tcl_SetVar2(interp, GtclConst, t1, t,                 \
                                  TCL_GLOBAL_ONLY);                         \
                      cvtfrom(t1);                                          \
                      Tcl_SetVar2(interp, GtclConst, t1, t,                 \
                                  TCL_GLOBAL_ONLY);

#define Gtcl_CAddf(c)  sprintf(t, "%g", c);                                 \
                       sprintf(t1, "%s", # c);                              \
                       Tcl_SetVar2(interp, GtclConst, t1, t,                \
                                   TCL_GLOBAL_ONLY);                        \
                       cvtfrom(t1);                                         \
                       Tcl_SetVar2(interp, GtclConst, t1, t,                \
                                   TCL_GLOBAL_ONLY);

int
Gtcl_ConstInit(Tcl_Interp *interp){
  char t[30], t1[30];

  Tcl_PkgProvide(interp, "GtclConstant", "1.0");
  /* the auto-generated ones from <gimpenums.h> */
#include "gtclenums.h"

  /* and a few manifest constants from <glib.h> */
  Gtcl_CAddf(G_MINFLOAT);
  Gtcl_CAddf(G_MAXFLOAT);
  Gtcl_CAddf(G_MINDOUBLE);
  Gtcl_CAddf(G_MAXDOUBLE);
  Gtcl_CAdd(G_MINSHORT);
  Gtcl_CAdd(G_MAXSHORT);
  Gtcl_CAdd(G_MININT);
  Gtcl_CAdd(G_MAXINT);
  Gtcl_CAdd(G_MINLONG);
  Gtcl_CAdd(G_MAXLONG);
  
  /* and a few others */
  Gtcl_CAdd(TRUE);
  Gtcl_CAdd(FALSE);

  /* for gimp-text */
#define NEW_LAYER -1
#define PIXELS 0
#define POINTS 1
  Gtcl_CAdd(NEW_LAYER);
  Gtcl_CAdd(PIXELS);
  Gtcl_CAdd(POINTS);
  return TCL_OK;
}

