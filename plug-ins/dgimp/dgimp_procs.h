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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __DGIMP_PROCS_H__

/*  Structures
 */
typedef struct _DGimpProc DGimpProc;
typedef struct _DGimpLGP DGimpLGP;

struct _DGimpProc
{
  char     *name;
  GRunProc  run_proc;
};

struct _DGimpLGP
{
  gint      filedes;
  gint      available;
};


/*  Function prototypes
 */
#define DGIMP_PROTO(func) \
void  func (char    *name, \
	    int      nparams, \
	    GParam  *params, \
	    int     *nreturn_vals, \
	    GParam **return_vals)

DGIMP_PROTO (dgimp_invert_proc);

GParam *     dgimp_convert_params (int      nparams,
				   GParam  *params);
gint         dgimp_auto_dist      (gchar   *proc_name,
				   int      nparams,
				   GParam  *params,
				   int      x1,
				   int      y1,
				   int      x2,
				   int      y2);


/*  External variables
 */
extern GList *dgimp_lgp_list;

/*  Static variables
 */
static GParam success_value[1] = { { PARAM_STATUS, STATUS_SUCCESS } };
static GParam failure_value[1] = { { PARAM_STATUS, STATUS_EXECUTION_ERROR } };


#endif /*  __DGIMP_PROCS_H__  */
