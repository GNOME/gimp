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

#ifndef __PROCEDURAL_DB_H__
#define __PROCEDURAL_DB_H__


struct _Argument
{
  GimpPDBArgType  arg_type;       /*  argument type        */

  union _ArgValue
  {
    gint32      pdb_int;        /*  Integer type         */
    gdouble     pdb_float;      /*  Floating point type  */
    gpointer    pdb_pointer;    /*  Pointer type         */
    GimpRGB     pdb_color;      /*  Color type           */
  } value;
};


/*  Argument marshalling procedures  */
typedef Argument * (* ArgMarshal) (Gimp         *gimp,
                                   GimpContext  *context,
                                   GimpProgress *progress,
				   Argument     *args);


/*  Execution types  */
typedef struct _IntExec    IntExec;
typedef struct _PlugInExec PlugInExec;
typedef struct _ExtExec    ExtExec;
typedef struct _TempExec   TempExec;
typedef struct _NetExec    NetExec;


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

struct _NetExec
{
  gchar      *host;           /*  Host responsible for procedure execution  */
  gint32      port;           /*  Port on host to send data to  */
};


/*  Structure for a procedure argument  */

struct _ProcArg
{
  GimpPDBArgType  arg_type;    /*  Argument type (int, char, char *, etc)  */
  gchar          *name;        /*  Argument name  */
  gchar          *description; /*  Argument description  */
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
  ProcArg     *args;          /*  Array of procedure arguments  */

  /*  Output values  */
  gint32       num_values;    /*  Number of return values  */
  ProcArg     *values;        /*  Array of return values  */

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

void           procedural_db_init          (Gimp         *gimp);
void           procedural_db_free          (Gimp         *gimp);

void           procedural_db_init_procs    (Gimp         *gimp);

void           procedural_db_register      (Gimp         *gimp,
                                            ProcRecord   *procedure);
void           procedural_db_unregister    (Gimp         *gimp,
                                            const gchar  *name);
ProcRecord   * procedural_db_lookup        (Gimp         *gimp,
                                            const gchar  *name);

Argument     * procedural_db_execute       (Gimp         *gimp,
                                            GimpContext  *context,
                                            GimpProgress *progress,
                                            const gchar  *name,
                                            Argument     *args);
Argument     * procedural_db_run_proc      (Gimp         *gimp,
                                            GimpContext  *context,
                                            GimpProgress *progress,
                                            const gchar  *name,
                                            gint         *nreturn_vals,
                                            ...);

Argument     * procedural_db_return_args   (ProcRecord   *procedure,
                                            gboolean      success);
void           procedural_db_destroy_args  (Argument     *args,
                                            gint          nargs);

void           procedural_db_free_data     (Gimp         *gimp);
void           procedural_db_set_data      (Gimp         *gimp,
                                            const gchar  *identifier,
                                            gint32        bytes,
                                            const guint8 *data);
const guint8 * procedural_db_get_data      (Gimp         *gimp,
                                            const gchar  *identifier,
                                            gint32       *bytes);


#endif  /*  __PROCEDURAL_DB_H__  */
