/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgpcompat.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_GP_COMPAT_H__
#define __GIMP_GP_COMPAT_H__


GParamSpec     * _gimp_gp_compat_param_spec   (GimpPDBArgType   arg_type,
                                               const gchar     *name,
                                               const gchar     *nick,
                                               const gchar     *blurb);

GType            _gimp_pdb_arg_type_to_gtype  (GimpPDBArgType   type);
GimpPDBArgType   _gimp_pdb_gtype_to_arg_type  (GType            type);

GimpValueArray * _gimp_params_to_value_array  (const GimpParam *params,
                                               gint             n_params,
                                               gboolean         full_copy);
GimpParam      * _gimp_value_array_to_params  (GimpValueArray  *args,
                                               gboolean         full_copy);


#endif /* __GIMP_GP_COMPAT_H__ */
