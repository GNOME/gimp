/***************************************************
 * file: gtclPDB.c
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

#include <ctype.h>
#include <tcl.h>
#include <stdlib.h>
#include <string.h>
#include <libgimp/gimp.h>
#include "gtcl.h"

static char *proc_types[] = {
  "undefined",
  "plug-in",
  "Extension",
};

static char *param_types[] = {
  "int32",
  "int16",
  "int8",
  "float",
  "string",
  "int32array",
  "int16array",
  "int8array",
  "floatarray",
  "stringarray",
  "color",
  "region",
  "display",
  "image",
  "layer",
  "channel",
  "drawable",
  "selection",
  "boundary",
  "path",
  "status",
  "end"
};

static int str_to_typeenum(char *type);
static int list_to_pdef(Tcl_Interp *interp, char *list, GParamDef *p);

/*
 * add the procedural-database to a Tcl Interpreter
 */
int
Gtcl_PDBInit(Tcl_Interp *interp){
  char **proc_list;
  char *proc_name;
  char *proc_blurb;
  char *proc_help;
  char *proc_author;
  char *proc_copyright;
  char *proc_date;
  int proc_type;
  int nparams;
  int nreturn_vals;
  GParamDef *params, *return_vals;
  int num_procs, i, j;
  char whole_proc[2048];
  char arglist[400];
  char carglist[400];

  gimp_query_database (".*", ".*", ".*", ".*", ".*", ".*", ".*",
		       &num_procs, &proc_list);
  for (i = 0; i < num_procs; i++) {
    memset(whole_proc, 0, 2048);
    memset(arglist, 0, 400);
    memset(carglist, 0, 400);
    proc_name = strdup (proc_list[i]);
		/*  fprintf(stderr, "(proc %d/%d %s)\n", i, num_procs, proc_name);*/
      /*  lookup the procedure  */
    if(gimp_query_procedure(proc_name, &proc_blurb, &proc_help, &proc_author,
			    &proc_copyright, &proc_date, &proc_type,
			    &nparams, &nreturn_vals,
			    &params, &return_vals) == TRUE) {
      cvtfrom(proc_name);
      sprintf(carglist, "gimp-run-procedure %s ", proc_name);
      for(j=0;j<nparams;j++){
	if (strcmp(params[j].name, "run_mode")==0){
	  strcat(carglist, "1 ");
	} else {
	  strcat(arglist, params[j].name);
	  strcat(arglist, " ");
	  strcat(carglist, "$");
	  strcat(carglist, params[j].name);
	  strcat(carglist, " ");
	}
      }
      sprintf(whole_proc, "proc %s {%s} {\n global GimpPDBCmd\n  set GimpPDBCmd %s\n   update\n   return [%s]\n}\n\n",
	      proc_name, arglist, proc_name, carglist);
#if 0
      fprintf(stderr, "%s", whole_proc);
#endif
      Tcl_GlobalEval(interp, whole_proc);
      free(proc_name);

      g_free(proc_blurb);
      g_free(proc_help);
      g_free(proc_author);
      g_free(proc_copyright);
      g_free(proc_date);
      g_free(params);
      g_free(return_vals);

    }
  }
  return TCL_OK;
}

/*
 * run a procedure from the PDB, really the heart of
 * this thing... Virtually everything goes through here.
 */
int
Gtcl_GimpRunProc(ClientData data, Tcl_Interp *interp, int ac, char *av[]){
  GParam *par, *vals;
  char *p_blurb, *p_help, *p_author, *p_copy, *p_date, **rv_a;
  int p_type, p_npar, p_nrv;
  GParamDef *p_par, *p_rv;
  char *p_name;
  int i;

  rv_a=(char **)NULL;
  p_name = strdup(av[1]);

  /* first try the name as given */
  if (gimp_query_procedure(p_name, &p_blurb, &p_help, &p_author, &p_copy,
			   &p_date, &p_type, &p_npar, &p_nrv, &p_par,
			   &p_rv) == FALSE) {
    /* nope?, try `tr - _` */
    cvtto(p_name);
    if (gimp_query_procedure(p_name, &p_blurb, &p_help, &p_author, &p_copy,
			     &p_date, &p_type, &p_npar, &p_nrv, &p_par,
			     &p_rv) == FALSE) {
      Tcl_SetResult(interp, "gimp-run-procedure invalid command: ",
		    TCL_STATIC);
      Tcl_AppendResult(interp, p_name, (char *)NULL);
      return TCL_ERROR;
    }
  }

  ac-=2; /* subtract off own name and the proc name */

  if (ac != p_npar){
    Tcl_SetResult(interp, "gimp-run-procedure: ", TCL_STATIC);
    Tcl_AppendResult(interp, p_name, " : Wrong # args\n",
		     "usage: ", p_name, (char *)NULL);
    for (i=0;i<p_npar;i++){
      Tcl_AppendResult(interp, " ", p_par[i].name, (char *)NULL);
    }
    return TCL_ERROR;
  }

  par = (GParam *)malloc(sizeof(GParam)*ac);
  for(i=0;i<p_npar;i++){
    par[i].type = p_par[i].type;
  }
  if(Argv_to_GParam(interp, p_name, p_npar, av+2, par)==TCL_ERROR){
    free(par);
    return TCL_ERROR;
  }

  DPRINTF(1,(stderr, "\nGimp PDB Running: (%s:", p_name));

  vals = gimp_run_procedure2 (p_name, &p_nrv, p_npar, par);
  if (! vals) {
    Tcl_SetResult(interp, "pdb: no status returned from", TCL_STATIC);
    Tcl_AppendResult(interp, p_name, (char *)NULL);
    free(par);
    return TCL_ERROR;
  }

  DPRINTF(1,(stderr, " returned %d)\n", vals[0].data.d_status));
  switch (vals[0].data.d_status) {
  case STATUS_EXECUTION_ERROR:
    gimp_destroy_params (vals, p_nrv);
    free(par);
    Tcl_SetResult(interp, "pdb: exec failed for ", TCL_STATIC);
    cvtfrom(p_name);
    Tcl_AppendResult(interp, p_name, (char *)NULL);
    return TCL_ERROR;
    break;
  case STATUS_CALLING_ERROR:
    gimp_destroy_params (vals, p_nrv);

    Tcl_SetResult(interp, "pdb: invalid arguments for ", TCL_STATIC);
    Tcl_AppendResult(interp, p_name, (char *)NULL);
    return TCL_ERROR;
    break;
  case STATUS_SUCCESS:
    rv_a=(char **)malloc(sizeof(char *)*p_nrv-1);
    if(GParam_to_Argv(interp, p_name, p_nrv-1, &vals[1], rv_a)==TCL_ERROR){
      gimp_destroy_params (vals, p_nrv);
      return TCL_ERROR;
    }
  }
  
  if(p_nrv==2){
    Tcl_SetResult(interp, rv_a[0], TCL_VOLATILE);
  } else {
    char *t;
    t=Tcl_Merge(p_nrv-1, rv_a);
    Tcl_SetResult(interp, t, TCL_DYNAMIC);
  }

  for(i=0;i<p_nrv-1;i++){
    free(rv_a[i]);
  }

  free((char *)rv_a);
  free(par);
  g_free(p_blurb);
  g_free(p_help);
  g_free(p_author);
  g_free(p_copy);
  g_free(p_date);
  g_free(p_par);
  g_free(p_rv);
  gimp_destroy_params (vals, p_nrv);

  return TCL_OK;
}

/*
 * query the database for info on a procedure
 */
int
Gtcl_QueryDBProc(ClientData data, Tcl_Interp *interp, int ac, char *av[]){
  char *blurb, *help, *author, *copyright, *date, **tl0, *tl1[3];
  char *t, *p_name;
  int type, npar, nrv, i;
  GParamDef *par_d, *rv_d;

  if (ac!=2) {
    Tcl_SetResult(interp, "gimp-query-dbproc: wrong # arguments", TCL_STATIC);
    return TCL_ERROR;
  }
  p_name = strdup(av[1]);
  cvtto(p_name);
  if(gimp_query_procedure(av[1], &blurb, &help, &author, &copyright,
			  &date, &type, &npar, &nrv, &par_d, &rv_d) == FALSE) {
    if(gimp_query_procedure(p_name, &blurb, &help, &author, &copyright,
			    &date, &type, &npar, &nrv, &par_d, &rv_d)
       == FALSE) {
      Tcl_SetResult(interp, "gimp-query-dbproc: invalid command: ",
		    TCL_STATIC);
      Tcl_AppendResult(interp, av[1], (char *)NULL);
      return TCL_ERROR;
    }
  }
  free(p_name);
  Tcl_AppendElement(interp, av[1]);
  Tcl_AppendElement(interp, blurb);
  Tcl_AppendElement(interp, help);
  Tcl_AppendElement(interp, author);
  Tcl_AppendElement(interp, copyright);
  Tcl_AppendElement(interp, date);
  Tcl_AppendElement(interp, proc_types[type]);
  tl0=(char**)malloc(sizeof(char*) * npar);
  for(i=0;i<npar;i++){
    tl1[0]=param_types[par_d[i].type];
    tl1[1]=par_d[i].name;
    tl1[2]=par_d[i].description;
    t=Tcl_Merge(3, tl1);
    tl0[i]=t;
  }
  t = Tcl_Merge(npar, tl0);
  Tcl_AppendElement(interp, t);
  Tcl_Free(t);
  for(i=0;i<npar;i++)
    Tcl_Free(tl0[i]);
  free(tl0);

  tl0=(char**)malloc(sizeof(char*) * nrv);
  for(i=0;i<nrv;i++){
    tl1[0]=param_types[rv_d[i].type];
    tl1[1]=rv_d[i].name;
    tl1[2]=rv_d[i].description;
    t=Tcl_Merge(3, tl1);
    tl0[i]=t;
  }
  t = Tcl_Merge(nrv, tl0);
  Tcl_AppendElement(interp, t);
  Tcl_Free(t);
  for(i=0;i<nrv;i++)
    Tcl_Free(tl0[i]);
  free(tl0);

  g_free(blurb);
  g_free(help);
  g_free(author);
  g_free(copyright);
  g_free(date);
  g_free(par_d);
  g_free(rv_d);
  return TCL_OK;
}

/*
 * query the database for any or all procedures
 */
int
Gtcl_QueryDB(ClientData data, Tcl_Interp *interp, int ac, char *av[]){
  char **procs, *r, *p_name;
  int nproc, i;
  if(ac != 8) {
    Tcl_SetResult(interp, "gimp-query-db: wrong # args", TCL_STATIC);
    return TCL_ERROR;
  }
  p_name = strdup(av[1]);
  cvtto(p_name);
  gimp_query_database (p_name, av[2], av[3], av[4], av[5], av[6], av[7],
		       &nproc, &procs);
  free(p_name);
  for(i=0;i<nproc;i++){
    cvtfrom(procs[i]);
  }
  r=Tcl_Merge(nproc, procs);
  for(i=1;i<nproc;i++){
    free(procs[i]);
  }
  free(procs);
  Tcl_SetResult(interp, r, TCL_DYNAMIC);
  return TCL_OK;
}

/*
 * install a procedure in the database
 */
int
Gtcl_InstallProc(ClientData data, Tcl_Interp *interp, int ac, char *av[]){
  GParamDef *args_p, *rets_p;
  char **args, **rets;
  int narg, nret, i, type;

  if(ac!=12){
    Tcl_SetResult(interp, "gimp-install-procedure: wrong # args:\n",
		  TCL_STATIC);
    Tcl_AppendResult(interp, "usage: ", av[0],
		     " <name> <blurb> <help> <author> <copyright> <date> "
		     "<menu-path> <image-types> <type> <args> <retvals>",
		     (char *)NULL);
    return(TCL_ERROR);
  }

  if (strncasecmp("plug", av[9], 4)==0){
    type=PROC_PLUG_IN;
  } else if (strcasecmp("extension", av[9])==0){
    type=PROC_EXTENSION;
  } else {
    Tcl_SetResult(interp, "unknown procedure type: `", TCL_STATIC);
    Tcl_AppendResult(interp, av[9], "'", (char *)NULL);
    return TCL_ERROR;
  }

  if (Tcl_SplitList(interp, av[10], &narg, &args)==TCL_ERROR) {
    return(TCL_ERROR);
  }
  if (Tcl_SplitList(interp, av[11], &nret, &rets)==TCL_ERROR) {
    return(TCL_ERROR);
  }
  args_p=(GParamDef*)malloc(sizeof(GParamDef)*narg);
  rets_p=(GParamDef*)malloc(sizeof(GParamDef)*nret);

  for(i=0;i<narg;i++){
    if (list_to_pdef(interp, args[i], &args_p[i])== TCL_ERROR){
      return TCL_ERROR;
    }
  }

  for(i=0;i<nret;i++){
    if (list_to_pdef(interp, rets[i], &rets_p[i])== TCL_ERROR){
      return TCL_ERROR;
    }
  }
#if 0
  fprintf(stderr, "proc_inst: %s [arg: %d] [ret: %d]\n", av[1], narg, nret);

  fprintf(stderr, "g_i_p(n=%s,\n      b=%s,\n      h=%s,\n      a=%s,\n",
	  av[1], av[2], av[3], av[4]);
  fprintf(stderr, "      c=%s,\n      d=%s,\n      m=%s,\n      i=%s,\n",
	  av[5], av[6], av[7], av[8]);
  fprintf(stderr, "      t=%d,\n     np=%d,\n     nr=%d)\n",
	  type, narg, nret);
#endif
  gimp_install_procedure(av[1], av[2], av[3], av[4], av[5], av[6], av[7],
			 av[8], type, narg, nret, args_p, rets_p);
  free(args_p);
  free(rets_p);
  return TCL_OK;
}

/*
 * the libgimp dispatcher -- needed for plugins
 */
int
Gtcl_GimpMain(ClientData data, Tcl_Interp *interp, int argc, char *argv[]){
  char **av, **av0, *av1;
  int ac0, i;
  av1=Tcl_GetVar(interp, "argv", TCL_GLOBAL_ONLY);
  Tcl_SplitList(interp, av1, &ac0, &av0);
  av=(char **)malloc(sizeof(char*)*ac0+1);

  av[0] = Tcl_GetVar(interp, "argv0", TCL_GLOBAL_ONLY);
  for(i=0;i<ac0;i++){
    av[i+1]=av0[i];
  }
  gimp_main(ac0+1, av);
  free((char*)av);
  free((char*)av0);
  return TCL_OK;
}

/*
 * static conveninence functions...
 */

/*
 * translate a 3 element list into a ParamDef structure
 */
static int
list_to_pdef(Tcl_Interp *interp, char *list, GParamDef *p){
  char **l;
  int n, t;
  if(Tcl_SplitList(interp, list, &n, &l)==TCL_ERROR){
    return TCL_ERROR;
  }
  if(n!=3) {
    Tcl_SetResult(interp, "ParamDef wasn't 3 elements", TCL_STATIC);
    return TCL_ERROR;
  }

  if((t=str_to_typeenum(l[0]))==-1) {
    Tcl_SetResult(interp, "ParamDef: unknown type `", TCL_STATIC);
    Tcl_AppendResult(interp, l[0], "'", (char *)NULL);
    return(TCL_ERROR);
  }
  p->type=t;
  p->name=strdup(l[1]);
  p->description=strdup(l[2]);
  Tcl_Free((char *)l);
  return TCL_OK;
}

static int
str_to_typeenum(char *type){
  int i;
  for(i=0;i<sizeof(param_types);i++)
    if(strcasecmp(type, param_types[i])==0) return i;
  return -1;
}


