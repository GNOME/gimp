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
#ifndef  __GIMP_UNIT_CMDS_H__
#define  __GIMP_UNIT_CMDS_H__

#include "procedural_db.h"

extern ProcRecord gimp_unit_get_number_of_units_proc;
extern ProcRecord gimp_unit_new_proc;
extern ProcRecord gimp_unit_get_deletion_flag_proc;
extern ProcRecord gimp_unit_set_deletion_flag_proc;
extern ProcRecord gimp_unit_get_identifier_proc;
extern ProcRecord gimp_unit_get_factor_proc;
extern ProcRecord gimp_unit_get_digits_proc;
extern ProcRecord gimp_unit_get_symbol_proc;
extern ProcRecord gimp_unit_get_abbreviation_proc;
extern ProcRecord gimp_unit_get_singular_proc;
extern ProcRecord gimp_unit_get_plural_proc;

#endif  /*  __GIMP_UNIT_CMDS_H__  */
