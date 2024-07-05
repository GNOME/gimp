/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __SCRIPT_FU_ARG_H__
#define __SCRIPT_FU_ARG_H__

void        script_fu_arg_free                    (SFArg         *arg);
void        script_fu_arg_reset                   (SFArg         *arg,
                                                   gboolean       should_reset_ids);

void        script_fu_arg_add_argument            (SFArg         *arg,
                                                   GimpProcedure *procedure,
                                                   const gchar   *name,
                                                   const gchar   *nick);
void        script_fu_arg_append_repr_from_gvalue (SFArg         *arg,
                                                   GString       *result_string,
                                                   GValue        *gvalue);
void        script_fu_arg_append_repr_from_self   (SFArg         *arg,
                                                   GString       *result_string);

void        script_fu_arg_reset_name_generator    (void);
void        script_fu_arg_generate_name_and_nick  (SFArg         *arg,
                                                   const gchar  **name,
                                                   const gchar  **nick);

void        script_fu_arg_init_resource           (SFArg         *arg,
                                                   GType          resource_type);

#endif /*  __SCRIPT_FU_ARG__  */
