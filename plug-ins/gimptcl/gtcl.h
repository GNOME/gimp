/***************************************************
 * file: gtcl.h
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

int Gtcl_PDBInit(Tcl_Interp *interp);
int Gtcl_ConstInit(Tcl_Interp *interp);

int Gtcl_GimpRunProc(ClientData data, Tcl_Interp *interp, int ac, char *av[]);
int Gtcl_QueryDBProc(ClientData data, Tcl_Interp *interp, int ac, char *av[]);
int Gtcl_QueryDB(ClientData data, Tcl_Interp *interp, int ac, char *av[]);
int Gtcl_InstallProc(ClientData data, Tcl_Interp *interp, int ac, char *av[]);
int Gtcl_GimpMain(ClientData data, Tcl_Interp *interp, int ac, char *av[]);
int Gtcl_SetData(ClientData data, Tcl_Interp *interp, int ac, char *av[]);
int Gtcl_GetData(ClientData data, Tcl_Interp *interp, int ac, char *av[]);

int Argv_to_GParam(Tcl_Interp *interp, char *name, int ac, char **av,
		   GParam *parr);
int GParam_to_Argv(Tcl_Interp *interp, char *name, int ac, GParam *vals,
		   char **av);

void gtcl_addconst(Tcl_Interp *interp);

void cvtfrom (char *str);
void cvtto (char *str);

extern char *GtclConst;
extern char *GtclProcs;

#define debugging 1

#ifdef debugging
extern int debuglevel;
#define DPRINTF(l,m) if(debuglevel>=l) fprintf m
#else
#define DPRINTF(l,m)
#endif

#ifdef __linux__
#define Tcl_Free free
#endif

