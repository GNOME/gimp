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


/*  Execution types  */
typedef struct _IntExec    IntExec;
typedef struct _PlugInExec PlugInExec;
typedef struct _ExtExec    ExtExec;
typedef struct _TempExec   TempExec;


struct _IntExec
{
  /*  Function called to marshal arguments  */
  GValueArray * (* marshal_func) (GimpProcedure     *procedure,
                                  Gimp              *gimp,
                                  GimpContext       *context,
                                  GimpProgress      *progress,
                                  const GValueArray *args);
};

struct _PlugInExec
{
  /*  Where is the executable on disk?  */
  gchar *filename;
};

struct _ExtExec
{
  /*  Where is the executable on disk?  */
  gchar *filename;
};

struct _TempExec
{
  /*  Plug-in that registered this temp proc  */
  void *plug_in;
};


#define GIMP_IS_PROCEDURE(obj) ((obj) != NULL)


struct _GimpProcedure
{
  /*  Flags  */
  gboolean     static_proc;    /* Is the procedure allocated?                */
  gboolean     static_strings; /* Are the procedure's strings allocated?     */

  /*  Procedure information  */
  gchar       *name;           /* Procedure name                             */
  gchar       *original_name;  /* Procedure name before canonicalization     */
  gchar       *blurb;          /* Short procedure description                */
  gchar       *help;           /* Detailed help instructions                 */
  gchar       *author;         /* Author field                               */
  gchar       *copyright;      /* Copyright field                            */
  gchar       *date;           /* Date field                                 */
  gchar       *deprecated;     /* Replacement if the procedure is deprecated */

  /*  Procedure type  */
  GimpPDBProcType  proc_type;  /* Type of procedure                          */

  /*  Input arguments  */
  gint32       num_args;       /* Number of procedure arguments              */
  GParamSpec **args;           /* Array of procedure arguments               */

  /*  Output values  */
  gint32       num_values;     /* Number of return values                    */
  GParamSpec **values;         /* Array of return values                     */

  /*  Method of procedure execution  */
  union _ExecMethod
  {
    IntExec     internal;      /* Execution information for internal procs   */
    PlugInExec  plug_in;       /* ..................... for plug-ins         */
    ExtExec     extension;     /* ..................... for extensions       */
    TempExec    temporary;     /* ..................... for temp procs       */
  } exec_method;
};


GimpProcedure * gimp_procedure_new                (void);
void            gimp_procedure_free               (GimpProcedure    *procedure);

GimpProcedure * gimp_procedure_init               (GimpProcedure    *procedure,
                                                   gint              n_arguments,
                                                   gint              n_return_vals);

void            gimp_procedure_set_strings        (GimpProcedure    *procedure,
                                                   gchar            *name,
                                                   gchar            *original_name,
                                                   gchar            *blurb,
                                                   gchar            *help,
                                                   gchar            *author,
                                                   gchar            *copyright,
                                                   gchar            *date,
                                                   gchar            *deprecated);
void           gimp_procedure_set_static_strings  (GimpProcedure    *procedure,
                                                   gchar            *name,
                                                   gchar            *original_name,
                                                   gchar            *blurb,
                                                   gchar            *help,
                                                   gchar            *author,
                                                   gchar            *copyright,
                                                   gchar            *date,
                                                   gchar            *deprecated);
void           gimp_procedure_take_strings        (GimpProcedure    *procedure,
                                                   gchar            *name,
                                                   gchar            *original_name,
                                                   gchar            *blurb,
                                                   gchar            *help,
                                                   gchar            *author,
                                                   gchar            *copyright,
                                                   gchar            *date,
                                                   gchar            *deprecated);

void            gimp_procedure_add_argument       (GimpProcedure    *procedure,
                                                   GParamSpec       *pspec);
void            gimp_procedure_add_return_value   (GimpProcedure    *procedure,
                                                   GParamSpec       *pspec);

void            gimp_procedure_add_compat_arg     (GimpProcedure    *procedure,
                                                   Gimp             *gimp,
                                                   GimpPDBArgType    arg_type,
                                                   const gchar      *name,
                                                   const gchar      *desc);
void            gimp_procedure_add_compat_value   (GimpProcedure    *procedure,
                                                   Gimp             *gimp,
                                                   GimpPDBArgType    arg_type,
                                                   const gchar      *name,
                                                   const gchar      *desc);

GValueArray   * gimp_procedure_get_arguments      (GimpProcedure    *procedure);
GValueArray   * gimp_procedure_get_return_values  (GimpProcedure    *procedure,
                                                   gboolean          success);

GValueArray   * gimp_procedure_execute            (GimpProcedure    *procedure,
                                                   Gimp             *gimp,
                                                   GimpContext      *context,
                                                   GimpProgress     *progress,
                                                   GValueArray      *args);


#endif  /*  __GIMP_PROCEDURE_H__  */
