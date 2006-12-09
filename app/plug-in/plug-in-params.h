/* GIMP - The GNU Image Manipulation Program
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

#ifndef __PLUG_IN_PARAMS_H__
#define __PLUG_IN_PARAMS_H__


GValueArray * plug_in_params_to_args (GParamSpec  **pspecs,
                                      gint          n_pspecs,
                                      GPParam      *params,
                                      gint          n_params,
                                      gboolean      return_values,
                                      gboolean      full_copy);
GPParam     * plug_in_args_to_params (GValueArray  *args,
                                      gboolean      full_copy);


#endif /* __PLUG_IN_PARAMS_H__ */
