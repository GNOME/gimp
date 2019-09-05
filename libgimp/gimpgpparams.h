/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpgpparams.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_GP_PARAMS_H__
#define __GIMP_GP_PARAMS_H__

G_BEGIN_DECLS


GParamSpec     * _gimp_gp_param_def_to_param_spec (const GPParamDef     *param_def);
void             _gimp_param_spec_to_gp_param_def (GParamSpec           *pspec,
                                                   GPParamDef           *param_def);

GimpValueArray * _gimp_gp_params_to_value_array   (gpointer              gimp,
                                                   GParamSpec          **pspecs,
                                                   gint                  n_pspecs,
                                                   const GPParam        *params,
                                                   gint                  n_params,
                                                   gboolean              return_values);
GPParam        * _gimp_value_array_to_gp_params   (const GimpValueArray *args,
                                                   gboolean              full_copy);
void             _gimp_gp_params_free             (GPParam              *params,
                                                   gint                  n_params,
                                                   gboolean              full_copy);


G_END_DECLS

#endif /* __GIMP_GP_PARAMS_H__ */
