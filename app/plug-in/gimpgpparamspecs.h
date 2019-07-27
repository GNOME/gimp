/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpgpparamspecs.h
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

#ifndef __GIMP_GP_PARAM_SPECS_H__
#define __GIMP_GP_PARAM_SPECS_H__


/*  function names start with an underscore because we incude code
 *  from libgimp/ which is private there
 */

void         _gimp_param_spec_to_gp_param_def (GParamSpec *pspec,
                                               GPParamDef *param_def);

GParamSpec * _gimp_gp_param_def_to_param_spec (Gimp       *gimp,
                                               GPParamDef *param_def);


#endif /* __GIMP_GP_PARAM_SPECS_H__ */
