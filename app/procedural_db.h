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

#include <glib.h>

#include "gimpimageF.h"

/*  Procedural database types  */
typedef enum
{
  PDB_INT32,
  PDB_INT16,
  PDB_INT8,
  PDB_FLOAT,
  PDB_STRING,
  PDB_INT32ARRAY,
  PDB_INT16ARRAY,
  PDB_INT8ARRAY,
  PDB_FLOATARRAY,
  PDB_STRINGARRAY,
  PDB_COLOR,
  PDB_REGION,
  PDB_DISPLAY,
  PDB_IMAGE,
  PDB_LAYER,
  PDB_CHANNEL,
  PDB_DRAWABLE,
  PDB_SELECTION,
  PDB_BOUNDARY,
  PDB_PATH,
  PDB_PARASITE,
  PDB_STATUS,
  PDB_END
} PDBArgType;

/*  Error types  */
typedef enum
{
  PDB_EXECUTION_ERROR,
  PDB_CALLING_ERROR,
  PDB_PASS_THROUGH,
  PDB_SUCCESS,
  PDB_CANCEL
} PDBStatusType;


/*  Procedure types  */
typedef enum /*< chop=PDB_ >*/
{
  PDB_INTERNAL,
  PDB_PLUGIN,
  PDB_EXTENSION,
  PDB_TEMPORARY
} PDBProcType;


/*  Argument type  */
typedef struct _Argument Argument;

struct _Argument
{
  PDBArgType    arg_type;       /*  argument type  */

  union _ArgValue
  {
    gint32      pdb_int;        /*  Integer type  */
    gdouble     pdb_float;      /*  Floating point type  */
    gpointer    pdb_pointer;    /*  Pointer type  */
  } value;
};


/*  Argument marshalling procedures  */
typedef Argument * (* ArgMarshal) (Argument *);


/*  Execution types  */
typedef struct _IntExec IntExec;
typedef struct _PlugInExec PlugInExec;
typedef struct _ExtExec ExtExec;
typedef struct _TempExec TempExec;
typedef struct _NetExec NetExec;

struct _IntExec
{
  ArgMarshal  marshal_func;   /*  Function called to marshal arguments  */
};

struct _PlugInExec
{
  gchar *     filename;       /*  Where is the executable on disk?  */
};

struct _ExtExec
{
  gchar *     filename;       /*  Where is the executable on disk?  */
};

struct _TempExec
{
  void *      plug_in;        /*  Plug-in that registered this temp proc  */
};

struct _NetExec
{
  gchar *     host;           /*  Host responsible for procedure execution  */
  gint32      port;           /*  Port on host to send data to  */
};


/*  Structure for a procedure argument  */
typedef struct _ProcArg ProcArg;

struct _ProcArg
{
  PDBArgType  arg_type;       /*  Argument type (int, char, char *, etc)  */
  char *      name;           /*  Argument name  */
  char *      description;    /*  Argument description  */
};


/*  Structure for a procedure  */

typedef struct _ProcRecord ProcRecord;

struct _ProcRecord
{
  /*  Procedure information  */
  gchar *     name;           /*  Procedure name  */
  gchar *     blurb;          /*  Short procedure description  */
  gchar *     help;           /*  Detailed help instructions  */
  gchar *     author;         /*  Author field  */
  gchar *     copyright;      /*  Copyright field  */
  gchar *     date;           /*  Date field  */

  /*  Procedure type  */
  PDBProcType proc_type;      /*  Type of procedure--Internal, Plug-In, Extension  */

  /*  Input arguments  */
  gint32      num_args;       /*  Number of procedure arguments  */
  ProcArg *   args;           /*  Array of procedure arguments  */

  /*  Output values  */
  gint32      num_values;     /*  Number of return values  */
  ProcArg *   values;         /*  Array of return values  */

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
void          procedural_db_init         (void);
void          procedural_db_free         (void);
void          procedural_db_register     (ProcRecord *);
void          procedural_db_unregister   (gchar      *);
ProcRecord *  procedural_db_lookup       (gchar      *);
Argument *    procedural_db_execute      (gchar      *,
					  Argument   *);
Argument *    procedural_db_run_proc     (gchar      *,
					  gint       *,
					  ...);
Argument *    procedural_db_return_args  (ProcRecord *,
					  int);
void          procedural_db_destroy_args (Argument   *,
					  int);
void          pdb_add_image              (GimpImage  *gimage);
gint          pdb_image_to_id            (GimpImage  *gimage);
GimpImage *   pdb_id_to_image            (gint id);
void          pdb_remove_image           (GimpImage  *image);

/* "type" should really be a PDBArgType, but we can cope with
 *  out-of-range values.
 */
const char *  pdb_type_name (gint type); /* really exists in _cmds.c file */

extern GHashTable *procedural_ht;

#endif  /*  __PROCEDURAL_DB_H__  */
