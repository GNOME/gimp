/***************************************************
 * file: gtclMain.c
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

#include <string.h>
#include <stdlib.h>
#include <libgimp/gimp.h>
#include <tcl.h>
#include <tk.h>

#include "gtcl.h"

int debuglevel=0;

int Gtcl_Init(Tcl_Interp *interp);

static void gtcl_init(void);
static void gtcl_quit(void);
static void gtcl_query(void);
static void gtcl_run(char *name, int nparm, GParam *p, int *nrv, GParam **rv);
int Gimptcl_Init(Tcl_Interp *interp);

Tcl_Interp *theInterp=NULL;

GPlugInInfo PLUG_IN_INFO = {
  gtcl_init,
  gtcl_quit,
  gtcl_query,
  gtcl_run
};

int
main(int ac, char *av[]){
  char *tcllib_p, *tcllib;

/*  fprintf(stderr, "gimptcl: %d %s\n", ac, av[1]);*/

  if (ac < 2) {
    fprintf(stderr, "usage: %s <script>\n", av[0]);
    exit(1);
  }

  theInterp = Tcl_CreateInterp();
  Tcl_SetVar(theInterp, "tcl_interactive", "1", TCL_GLOBAL_ONLY);
  tcllib_p = Tcl_GetVar(theInterp, "tcl_library", TCL_GLOBAL_ONLY);
  tcllib = malloc(strlen(tcllib_p)+10);
  sprintf(tcllib, "%s/init.tcl", tcllib_p);
  Tcl_EvalFile(theInterp, tcllib);
  free(tcllib);
  free(tcllib_p);

  Gimptcl_Init(theInterp);
  Tcl_StaticPackage(theInterp, "Gimptcl", Gimptcl_Init, Gimptcl_Init);

  Tcl_EvalFile(theInterp, av[1]);

  return gimp_main(ac-1, av+1);
}

/*
 * add Gimp package to a Tcl Interp
 */
int
Gimptcl_Init(Tcl_Interp *interp){
  char debuglevelvar[] = "DebugLevel";
  char GimpTclProcsVar[] = "GimpTclProcs";

  Tcl_PkgProvide(interp, "Gimptcl", "1.0");

  Tcl_CreateCommand(interp, "gimp-run-procedure", Gtcl_GimpRunProc,
		    (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
  Tcl_CreateCommand(interp, "gimp-query-dbproc", Gtcl_QueryDBProc,
		    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "gimp-query-db", Gtcl_QueryDB,
		    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "gimp-install-procedure", Gtcl_InstallProc,
		    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "gimp-set-data", Gtcl_SetData,
		    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
  Tcl_CreateCommand(interp, "gimp-get-data", Gtcl_GetData,
		    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
#ifdef SHLIB_METHOD
  Tcl_CreateCommand(interp, "gimp-main", Gtcl_GimpMain,
		    (ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
#endif
  Tcl_LinkVar(interp, debuglevelvar,  (char *)&debuglevel,
	      TCL_LINK_INT);
  Tcl_LinkVar(interp, GimpTclProcsVar, (char *)&GtclProcs,
	      TCL_LINK_STRING |TCL_LINK_READ_ONLY);

  /*
   * we have to delay adding PDB and Const to the interp
   * until gtcl_run is called, because we need
   * gimp_main to setup the IPC to the PDB.
   */
  if (!theInterp)
    theInterp=interp;

  return TCL_OK;
}

static void
gtcl_init(void){
  if (theInterp == NULL){
    fprintf(stderr, "gtcl_init, theInterp not initialized\n");
    return;
  }

  Tcl_Eval(theInterp, "info procs gimptcl_init");
  if (strcmp(theInterp->result, "gimptcl_init")==0){
    if(Tcl_Eval(theInterp, "gimptcl_init")==TCL_ERROR){
      fprintf(stderr, "Error in gtcl_init:\n\t%s\n", theInterp->result);
    } else {
      /*      printf("%s\n", theInterp->result);*/
    }
  }
}

static void
gtcl_quit(void){
  if (theInterp == NULL){
    fprintf(stderr, "gtcl_quit, theInterp not initialized\n");
    return;
  }
  Tcl_Eval(theInterp, "info procs gimptcl_quit");
  if (strcmp(theInterp->result, "gimptcl_quit")==0){
    if(Tcl_Eval(theInterp, "gimptcl_quit")==TCL_ERROR){
      fprintf(stderr, "Error in gtcl_quit:\n\t%s\n", theInterp->result);
    } else {
      /*      printf("%s\n", theInterp->result);*/
    }
  }
}

static void
gtcl_query(void){
  if (theInterp == NULL){
    fprintf(stderr, "gtcl_quit, theInterp not initialized\n");
    return;
  }
  Tcl_Eval(theInterp, "info procs gimptcl_query");
  if (strcmp(theInterp->result, "gimptcl_query")!=0){
    fprintf(stderr, "Whoa there, no query\n");
  } else {
    if(Tcl_Eval(theInterp, "gimptcl_query")==TCL_ERROR){
      fprintf(stderr, "Error in gtcl_query:%s\n", theInterp->result);
    } else {
      /*      printf("%s\n", theInterp->result);*/
    }
  }
}

static void
gtcl_run(char *name, int nparm, GParam *p, int *nrv, GParam **rv){
  char **pars, **rvs, *t, cmd[80];
  int rstat;

  if (theInterp == NULL){
    fprintf(stderr, "gtcl_run, theInterp not initialized\n");
    rstat=STATUS_EXECUTION_ERROR;
    goto error_done;
  }

  /* ok, add in our constants and the full PDB */
  Gtcl_PDBInit(theInterp);
  Gtcl_ConstInit(theInterp);
  Tcl_StaticPackage(theInterp, "GtclConstant", Gtcl_ConstInit, Gtcl_ConstInit);
  Tcl_StaticPackage(theInterp, "GtclPDB", Gtcl_PDBInit, Gtcl_PDBInit);

  Tcl_Eval(theInterp, "info procs gimptcl_run");
  if (strcmp(theInterp->result, "gimptcl_run")!=0){
    fprintf(stderr, "gimptcl_run: nothing to do");
    rstat = STATUS_EXECUTION_ERROR;
    goto error_done;
  }

  pars=(char **)malloc(sizeof(char *)*nparm);
  /*  fprintf(stderr, "gimptcl_run, need %d args\n", nparm);*/
  if(GParam_to_Argv(theInterp, "gimptcl_run", nparm, p, pars)==TCL_ERROR){
    rstat=STATUS_CALLING_ERROR;
    goto error_done;
  }

  t=Tcl_Merge(nparm, pars);
  free(pars);
  sprintf(cmd, "gimptcl_run %s", t);
  Tcl_Free(t);
  if (p[0].data.d_int32 == RUN_INTERACTIVE) {
    if (Tk_Init(theInterp) == TCL_OK) {
    } else {
      fprintf (stderr, "error in Tk_Init(): %s\n", theInterp->result);
    }
#if TK_MAJOR_VERSION == 8
    Tcl_StaticPackage(theInterp, "Tk", Tk_Init, Tk_SafeInit);
#else
    Tcl_StaticPackage(theInterp, "Tk", Tk_Init, NULL);
#endif
  }

  if(Tcl_Eval(theInterp, cmd)==TCL_ERROR){
    fprintf(stderr, "Error in gtcl_run:%s\n", theInterp->result);
    rstat=STATUS_CALLING_ERROR;
    goto error_done;
  }

  if (p[0].data.d_int32 == RUN_INTERACTIVE) {
    Tk_MainLoop();
    rstat=STATUS_SUCCESS;
    goto error_done; /* XXX a bit kludgey */
  }

  /*  fprintf(stderr, "gimptcl_run returned `%s'\n", theInterp->result);*/
  Tcl_SplitList(theInterp, theInterp->result, nrv, &rvs);
  /*  fprintf(stderr, "split into %d rets\n", *nrv);*/

  *rv=(GParam*)malloc(sizeof(GParam) * ((*nrv)+1));
  rv[0]->type=PARAM_STATUS;
  {
    char *b, *h, *a, *c, *d;
    int type, nparams, nrvs, i;
    GParamDef *p_params, *p_rvals;
    
    if(gimp_query_procedure(name, &b, &h, &a, &c, &d, &type, &nparams, &nrvs,
			    &p_params, &p_rvals) != TRUE) {
      rstat=STATUS_EXECUTION_ERROR;
      goto error_done;
    }
    /*    fprintf(stderr, "%s returns %d rets\n", name, nrvs);*/
    for(i=0;i<*nrv;i++){
      fprintf(stderr, "setting rv[%d] to %d", i+1, p_rvals[i].type);
      (*rv)[i+1].type= p_rvals[i].type;
      fprintf(stderr, "\n");
    }
    g_free(b);
    g_free(h);
    g_free(a);
    g_free(d);
  }

  if(Argv_to_GParam(theInterp, "gtcl_run", *nrv, rvs, &((*rv)[1]))==TCL_ERROR){
    rv[0]->data.d_status=STATUS_EXECUTION_ERROR;
  } else {
    rv[0]->data.d_status=STATUS_SUCCESS;
  }
  return;

error_done:
  *nrv=0;
  *rv=(GParam*)malloc(sizeof(GParam));
  rv[0]->type = PARAM_STATUS;
  rv[0]->data.d_status=rstat;
}
