/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gradient editor module copyight (C) 1996-1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
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


#ifndef _GRADIENT_H_
#define _GRADIENT_H_


#include "procedural_db.h"


/***** Functions *****/

void gradients_init(int no_data);
void gradients_free(void);

void grad_get_color_at(double pos, double *r, double *g, double *b, double *a);

void grad_create_gradient_editor(void);
void grad_free_gradient_editor(void);
void gradients_check_dialogs(void);


/***** Procedural database exports *****/

extern ProcRecord gradients_get_list_proc;
extern ProcRecord gradients_get_active_proc;
extern ProcRecord gradients_set_active_proc;
extern ProcRecord gradients_sample_uniform_proc;
extern ProcRecord gradients_sample_custom_proc;
extern ProcRecord gradients_close_popup_proc;
extern ProcRecord gradients_set_popup_proc;
extern ProcRecord gradients_popup_proc;
extern ProcRecord gradients_get_gradient_data_proc;

#endif
