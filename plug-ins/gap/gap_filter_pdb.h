/*  gap_filter_pdb.h
 *
 * GAP ... Gimp Animation Plugins
 *
 */
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

#ifndef _GAP_FILTER_PDB_H
#define _GAP_FILTER_PDB_H

#include "libgimp/gimp.h"

typedef enum
{  PTYP_ANY                     = 0,
   PTYP_ITERATOR                = 1,
   PTYP_CAN_OPERATE_ON_DRAWABLE = 2 
} t_proc_type;


typedef enum
{  PAPP_CONSTANT                = 0,
   PTYP_VARYING_LINEAR          = 1
} t_apply_mode;


/* ------------------------
 * gap_filter_pdb.h
 * ------------------------
 */

gint p_call_plugin(char *plugin_name, gint32 image_id, gint32 layer_id, GimpRunModeType run_mode);
int  p_save_xcf(gint32 image_id, char *sav_name);
gint p_get_data(char *key);
void p_set_data(char *key, gint plugin_data_len);
gint p_procedure_available(char  *proc_name, t_proc_type ptype);
char * p_get_iterator_proc(char *plugin_name);

int p_constraint_proc_sel1(gchar *proc_name);
int p_constraint_proc_sel2(gchar *proc_name);
int p_constraint_proc(gchar *proc_name);


#endif
