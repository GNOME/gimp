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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_PROCEDURE_H__
#define __GIMP_PROCEDURE_H__


/*  Argument marshalling procedures  */
typedef Argument * (* ArgMarshal) (ProcRecord   *procedure,
                                   Gimp         *gimp,
                                   GimpContext  *context,
                                   GimpProgress *progress,
                                   Argument     *args);


/*  Execution types  */
typedef struct _IntExec    IntExec;
typedef struct _PlugInExec PlugInExec;
typedef struct _ExtExec    ExtExec;
typedef struct _TempExec   TempExec;


struct _IntExec
{
  ArgMarshal  marshal_func;   /*  Function called to marshal arguments  */
};

struct _PlugInExec
{
  gchar      *filename;       /*  Where is the executable on disk?  */
};

struct _ExtExec
{
  gchar      *filename;       /*  Where is the executable on disk?  */
};

struct _TempExec
{
  void       *plug_in;        /*  Plug-in that registered this temp proc  */
};


/*  Structure for a procedure  */

struct _ProcRecord
{
  /*  Procedure information  */
  gchar       *name;          /*  Procedure name  */
  gchar       *original_name; /*  Procedure name before canonicalization  */
  gchar       *blurb;         /*  Short procedure description  */
  gchar       *help;          /*  Detailed help instructions  */
  gchar       *author;        /*  Author field  */
  gchar       *copyright;     /*  Copyright field  */
  gchar       *date;          /*  Date field  */

  gchar       *deprecated;    /*  Replacement if the procedure is deprecated  */

  /*  Procedure type  */
  GimpPDBProcType  proc_type; /*  Type of procedure--Internal, Plug-In, Extension  */

  /*  Input arguments  */
  gint32       num_args;      /*  Number of procedure arguments  */
  ProcArg     *args;          /*  Array of procedure arguments   */

  /*  Output values  */
  gint32       num_values;    /*  Number of return values        */
  ProcArg     *values;        /*  Array of return values         */

  /*  Method of procedure execution  */
  union _ExecMethod
  {
    IntExec     internal;     /*  Execution information for internal procs  */
    PlugInExec  plug_in;      /*  ..................... for plug-ins  */
    ExtExec     extension;    /*  ..................... for extensions  */
    TempExec    temporary;    /*  ..................... for temp procs  */
  } exec_method;
};


/*  Functions  */

ProcRecord   * gimp_procedure_init              (ProcRecord       *procedure,
                                                 gint              n_arguments,
                                                 gint              n_return_vals);

void           gimp_procedure_add_argument      (ProcRecord       *procedure,
                                                 GimpPDBArgType    arg_type,
                                                 GParamSpec       *pspec);
void           gimp_procedure_add_return_value  (ProcRecord       *procedure,
                                                 GimpPDBArgType    arg_type,
                                                 GParamSpec       *pspec);

void           gimp_procedure_add_compat_arg    (ProcRecord       *procedure,
                                                 Gimp             *gimp,
                                                 GimpPDBArgType    arg_type,
                                                 const gchar      *name,
                                                 const gchar      *desc);
void           gimp_procedure_add_compat_value  (ProcRecord       *procedure,
                                                 Gimp             *gimp,
                                                 GimpPDBArgType    arg_type,
                                                 const gchar      *name,
                                                 const gchar      *desc);

Argument     * gimp_procedure_get_arguments     (const ProcRecord *procedure);
Argument     * gimp_procedure_get_return_values (const ProcRecord *procedure,
                                                 gboolean          success);

Argument     * gimp_procedure_execute           (ProcRecord       *procedure,
                                                 Gimp             *gimp,
                                                 GimpContext      *context,
                                                 GimpProgress     *progress,
                                                 Argument         *args,
                                                 gint              n_args,
                                                 gint             *n_return_vals);


#endif  /*  __GIMP_PROCEDURE_H__  */
