/***************************************************
 * file: gtcl_misc.c
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
#include <stdlib.h>
#include <string.h>
#include <tcl.h>
#include <libgimp/gimp.h>
#include "gtcl.h"

int
Argv_to_GParam(Tcl_Interp *interp, char *name, int ac, char **av,
	      GParam *par){
  GParam *pc;
  int i;
  for(i=0;i<ac;i++){
    pc=&par[i];
    switch(pc->type) {

    case PARAM_INT32:
      if (isalpha(*av[i])) {
	char *t;
	DPRINTF(3,(stderr, "i32 ht lu: %s\n", av[i]));
	if ((t=Tcl_GetVar2(interp, GtclConst, av[i], TCL_GLOBAL_ONLY))
	    == NULL){
	  Tcl_SetResult(interp, name, TCL_VOLATILE);
	  Tcl_AppendResult(interp, ": argument error: ", av[i],
			   " unknown constant", (char *)NULL);
	  return TCL_ERROR;
	}
	pc->data.d_int32 = strtol(t, (char **)NULL, 0);
      } else {
	pc->data.d_int32 = strtol(av[i], (char **)NULL, 0);
      }
      DPRINTF(2,(stderr, " (i32)%d ", pc->data.d_int32));
      break;

    case PARAM_INT16:
      if (isalpha(*av[i])) {
	char *t;
	DPRINTF(3,(stderr, "i16 ht lu: %s\n", av[i]));
	if ((t=Tcl_GetVar2(interp, GtclConst, av[i], TCL_GLOBAL_ONLY))
	    == NULL){
	  Tcl_SetResult(interp, name, TCL_VOLATILE);
	  Tcl_AppendResult(interp, ": argument error: ", av[i],
			   " unknown constant", (char *)NULL);
	  return TCL_ERROR;
	}
	pc->data.d_int16 = strtol(t, (char **)NULL, 0);
      } else {
	pc->data.d_int16 = strtol(av[i], (char **)NULL, 0);
      }
      DPRINTF(2,(stderr, " (i16)%d ", pc->data.d_int16));
      break;

    case PARAM_INT8:
      if (isalpha(*av[i])) {
	char *t;

	DPRINTF(3,(stderr, "ht lu: %s\n", av[i]));
	if ((t=Tcl_GetVar2(interp, GtclConst, av[i], TCL_GLOBAL_ONLY))
	    == NULL){
	  Tcl_SetResult(interp, name, TCL_VOLATILE);
	  Tcl_AppendResult(interp, ": argument error: ", av[i],
			   " unknown constant", (char *)NULL);
	  return TCL_ERROR;
	}
	pc->data.d_int8 = strtol(t, (char **)NULL, 0);
      } else {
	pc->data.d_int8 = strtol(av[i], (char **)NULL, 0);
      }
      DPRINTF(2,(stderr, " (i8)%d ", pc->data.d_int8));
      break;

    case PARAM_FLOAT:
      if (isalpha(*av[i])) {
	char *t;
	
	DPRINTF(3,(stderr, "fl ht lu: %s\n", av[i]));
	if ((t=Tcl_GetVar2(interp, GtclConst, av[i], TCL_GLOBAL_ONLY))
	    == NULL){
	  Tcl_SetResult(interp, name, TCL_VOLATILE);
	  Tcl_AppendResult(interp, ": argument error: ", av[i],
			   " unknown constant", (char *)NULL);
	  return TCL_ERROR;
	}
	pc->data.d_float = strtod(t, (char **)NULL);
      } else {
	pc->data.d_float = strtod(av[i], (char **)NULL);
      }
      DPRINTF(2,(stderr, " (f.%s.)%g ", av[i], pc->data.d_float));
      break;

    case PARAM_STRING:
      pc->data.d_string = av[i];
      DPRINTF(2,(stderr, " `%s' ", pc->data.d_string));
      break;

    case PARAM_INT32ARRAY:  {
      int tc, j;
      char **ar;
      if (Tcl_SplitList(interp, av[i], &tc, &ar)==TCL_ERROR){
	return TCL_ERROR;
      }
      pc->data.d_int32array = (gint32*)malloc(sizeof(gint32)*tc);

      for(j=0;j<tc;j++){
	if (isalpha(*ar[j])) {
	  char *t;
	
	  DPRINTF(3,(stderr, "i32a ht lu: %s\n", ar[j]));
	  if ((t=Tcl_GetVar2(interp, GtclConst, ar[j], TCL_GLOBAL_ONLY))
	      == NULL){
	    Tcl_SetResult(interp, name, TCL_VOLATILE);
	    Tcl_AppendResult(interp, ": argument error: ", ar[j],
			     " unknown constant", (char *)NULL);
	    return TCL_ERROR;
	  }
	  pc->data.d_int32array[j] = strtol(t, (char **)NULL, 0);
	} else {
	  pc->data.d_int32array[j] = strtol(ar[j], (char **)NULL, 0);
	}
      }
      Tcl_Free((char *)ar);
    }
    break;

    case PARAM_INT16ARRAY: {
      int tc, j;
      char **ar;
      if (Tcl_SplitList(interp, av[i], &tc, &ar)==TCL_ERROR){
	return TCL_ERROR;
      }
      pc->data.d_int16array = (gint16*)malloc(sizeof(gint16)*tc);
      for(j=0;j<tc;j++){
	if (isalpha(*ar[j])) {
	  char *t;

	  DPRINTF(3,(stderr, "i16a ht lu: %s\n", av[i]));
	  if ((t=Tcl_GetVar2(interp, GtclConst, ar[j], TCL_GLOBAL_ONLY))
	      == NULL){
	    Tcl_SetResult(interp, name, TCL_VOLATILE);
	    Tcl_AppendResult(interp, ": argument error: ", ar[j],
			     " unknown constant", (char *)NULL);
	    return TCL_ERROR;
	  }
	  pc->data.d_int16array[j] = strtol(t, (char **)NULL, 0);
	} else {
	  pc->data.d_int16array[j] = strtol(ar[j], (char **)NULL, 0);
	}
      }
      Tcl_Free((char *)ar);
    }
    break;

    case PARAM_INT8ARRAY: {
      int tc, j;
      char **ar;
      if (Tcl_SplitList(interp, av[i], &tc, &ar)==TCL_ERROR){
	return TCL_ERROR;
      }
      pc->data.d_int8array = (gint8*)malloc(sizeof(gint8)*tc);
      for(j=0;j<tc;j++){
	if (isalpha(*ar[j])) {
	  char *t;

	  DPRINTF(3,(stderr, "i8a ht lu: %s\n", av[i]));
	  if ((t=Tcl_GetVar2(interp, GtclConst, ar[j], TCL_GLOBAL_ONLY))
	      == NULL){
	    Tcl_SetResult(interp, name, TCL_VOLATILE);
	    Tcl_AppendResult(interp, ": argument error: ", ar[j],
			     " unknown constant", (char *)NULL);
	    return TCL_ERROR;
	  }
	  pc->data.d_int8array[j] = strtol(t, (char **)NULL, 0);
	} else {
	  pc->data.d_int8array[j] = strtol(ar[j], (char **)NULL, 0);
	}
      }
      Tcl_Free((char *)ar);
    }
    break;

    case PARAM_FLOATARRAY: {
      int tc, j;
      char **ar;
      if (Tcl_SplitList(interp, av[i], &tc, &ar)==TCL_ERROR){
	return TCL_ERROR;
      }
      DPRINTF(2, (stderr, "fla("));
      pc->data.d_floatarray = (gdouble*)malloc(sizeof(gdouble)*tc);
      for(j=0;j<tc;j++){
	DPRINTF(2, (stderr, "%s ", ar[j]));
	if (isalpha(*ar[j])) {
	  char *t;
	  DPRINTF(3,(stderr, "fla ht lu: %s\n", ar[j]));
	  if ((t=Tcl_GetVar2(interp, GtclConst, ar[j], TCL_GLOBAL_ONLY))
	      == NULL){
	    Tcl_SetResult(interp, name, TCL_VOLATILE);
	    Tcl_AppendResult(interp, ": argument error: ", ar[j],
			     " unknown constant", (char *)NULL);
	    return TCL_ERROR;
	  }
	  pc->data.d_floatarray[j] = strtod(t, (char **)NULL);
	} else {
	  pc->data.d_floatarray[j] = strtod(ar[j], (char **)NULL);
	}
      }
      DPRINTF(2,(stderr, ") "));
      Tcl_Free((char *)ar);
    }
    break;

    case PARAM_STRINGARRAY: {
      int tc;
      if (Tcl_SplitList(interp, av[i], &tc, &pc->data.d_stringarray)
	  ==TCL_ERROR){
	Tcl_SetResult(interp, "argument error in stringarray", TCL_STATIC);
	return TCL_ERROR;
      }
      if (tc != (pc-1)->data.d_int32) {
	char tt[150];
	Tcl_SetResult(interp, "argument error in stringarray:", TCL_STATIC);
	sprintf(tt, "expected %d, got %d\n", (pc-1)->data.d_int32, tc);
	Tcl_AppendResult(interp, tt, (char *)NULL);
      }
    }
    break;

    case PARAM_COLOR: {
      int tc;
      char **ar;
      if (Tcl_SplitList(interp, av[i], &tc, &ar)==TCL_ERROR){
	return TCL_ERROR;
      }
      if(tc!=3) {
	Tcl_SetResult(interp, "argument type error, expected RGB", TCL_STATIC);
	Tcl_Free((char *)ar);
	return TCL_ERROR;
      }
      pc->data.d_color.red = strtol(ar[0], (char **)NULL, 0);
      pc->data.d_color.green = strtol(ar[1], (char **)NULL, 0);
      pc->data.d_color.blue = strtol(ar[2], (char **)NULL, 0);
      Tcl_Free((char *)ar);
    }
      DPRINTF(2,(stderr, " {%d %d %d}  ", pc->data.d_color.red, pc->data.d_color.green, pc->data.d_color.blue));
    break;

    case PARAM_REGION:
      Tcl_SetResult(interp, "Unsupported argument type: region", TCL_STATIC);
      return TCL_ERROR;
      break;

    case PARAM_DISPLAY:
    case PARAM_IMAGE:
    case PARAM_LAYER:
    case PARAM_CHANNEL:
    case PARAM_DRAWABLE:
    case PARAM_SELECTION:
      pc->data.d_int32 = strtol(av[i], (char **)NULL, 0);
      DPRINTF(2,(stderr, " (dsp,img,lay,chan,drw,sel)%d ", pc->data.d_int32));
      break;

    case PARAM_BOUNDARY:
      Tcl_SetResult(interp, "Unsupported argument type: Boundary", TCL_STATIC);
      return TCL_ERROR;
      break;

    case PARAM_PATH:
      Tcl_SetResult(interp, "Unsupported argument type: Path", TCL_STATIC);
      return TCL_ERROR;
      break;

    case PARAM_STATUS:
      Tcl_SetResult(interp, "Unsupported argument type: Status", TCL_STATIC);
      return TCL_ERROR;
      break;

    default:
      Tcl_SetResult(interp, "Unknown argument type", TCL_STATIC);
      return TCL_ERROR;
      
    }
  }
  return TCL_OK;
}

int
GParam_to_Argv(Tcl_Interp *interp, char *p_name, int p_nrv,
	       GParam *vals, char **av){
  int i;
  char t[100];

  for(i=0;i<p_nrv;i++) {
    memset(t, 0, 100);
    switch(vals[i].type) {
    case PARAM_INT32:
      sprintf(t, "%d", vals[i].data.d_int32);
      break;
    case PARAM_INT16:
      sprintf(t, "%d", vals[i].data.d_int16);
      break;
    case PARAM_INT8:
      sprintf(t, "%d", vals[i].data.d_int8);
      break;
    case PARAM_FLOAT:
      sprintf(t, "%g", vals[i].data.d_float);
      break;
    case PARAM_STRING:
      strcpy(t,vals[i].data.d_string);
      break;
    case PARAM_INT32ARRAY: {
      char **ar, *t1;
      int c, j;
      c=vals[i-1].data.d_int32;
      ar = (char **)malloc(sizeof(char *)*c);
      for(j=0;j<c;j++){
	sprintf(t, "%d", vals[i].data.d_int32array[j]);
	ar[j] = strdup(t);
      }
      t1 = Tcl_Merge(c, ar);
      for(j=0;j<c;j++){
	free(ar[j]);
      }
      free(ar);
      strcpy(t, t1);
      free(t1);
    }
    break;
    case PARAM_INT16ARRAY: {
      char **ar, *t1;
      int c, j;
      c=vals[i-1].data.d_int32;
      ar = (char **)malloc(sizeof(char *)*c);
      for(j=0;j<c;j++){
	sprintf(t, "%d", vals[i].data.d_int16array[j]);
	ar[j] = strdup(t);
      }
      t1=Tcl_Merge(c, ar);
      for(j=0;j<c;j++){
	free(ar[j]);
      }
      free(ar);
      strcpy(t, t1);
      free(t1);
    }
    break;
    case PARAM_INT8ARRAY:{
      char **ar, *t1;
      int c, j;
      c=vals[i-1].data.d_int32;
      ar = (char **)malloc(sizeof(char *)*c);
      for(j=0;j<c;j++){
	sprintf(t, "%d", vals[i].data.d_int8array[j]);
	ar[j] = strdup(t);
      }
      t1=Tcl_Merge(c, ar);
      for(j=0;j<c;j++){
	free(ar[j]);
      }
      free(ar);
      strcpy(t, t1);
      free(t1);
    }
    break;
    case PARAM_STRINGARRAY: {
      char *t1;
      t1=Tcl_Merge(vals[i-1].data.d_int32, vals[i].data.d_stringarray);
      strcpy(t, t1);
      free(t1);
    }
    break;
    case PARAM_COLOR: {
      char *rgb[3], *t1;
      sprintf(t, "%d", vals[i].data.d_color.red);
      rgb[0] = strdup(t);
      sprintf(t, "%d", vals[i].data.d_color.green);
      rgb[1] = strdup(t);
      sprintf(t, "%d", vals[i].data.d_color.blue);
      rgb[2] = strdup(t);
      t1=Tcl_Merge(3, rgb);
      free(rgb[0]); free(rgb[1]); free(rgb[2]);
      strcpy(t, t1);
      free(t1);
    }
    break;
    case PARAM_REGION:
      Tcl_SetResult(interp, "Unsupported return type: Region",
		    TCL_STATIC);
      return TCL_ERROR;
      break;
    case PARAM_DISPLAY:
    case PARAM_IMAGE:
    case PARAM_LAYER:
    case PARAM_CHANNEL:
    case PARAM_DRAWABLE:
    case PARAM_SELECTION:
      sprintf(t, "%d", vals[i].data.d_int32);
      break;
    case PARAM_BOUNDARY:
      Tcl_SetResult(interp, "Unsupported return type: Boundary",
		    TCL_STATIC);
      return TCL_ERROR;
      break;
    case PARAM_PATH:
      Tcl_SetResult(interp, "Unsupported return type: Path", TCL_STATIC);
      return TCL_ERROR;
      break;
    case PARAM_STATUS:
      Tcl_SetResult(interp, "pdb: multiple return status", TCL_STATIC);
      return TCL_ERROR;
      break;
    default:
      Tcl_SetResult(interp, "Unknown return type", TCL_STATIC);
      return TCL_ERROR;
      break;
    }
    av[i] = strdup(t);
  }
  return TCL_OK;
}


void
cvtfrom (char *str) {
  for(;*str;str++) {
    if (*str == '_') {
      *str = '-';
    }
  }
}

void
cvtto (char *str) {
  for(;*str;str++) {
    if (*str == '-') {
      *str = '_';
    }
  }
}

