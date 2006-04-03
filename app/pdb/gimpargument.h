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

#ifndef __GIMP_ARGUMENT_H__
#define __GIMP_ARGUMENT_H__


struct _GimpArgument
{
  GValue value;
};

struct _GimpArgumentSpec
{
  GParamSpec *pspec;
};


void   gimp_argument_init        (GimpArgument     *arg,
                                  GimpArgumentSpec *spec);
void   gimp_argument_init_compat (GimpArgument     *arg,
                                  GimpPDBArgType    arg_type);

void   gimp_arguments_destroy    (GimpArgument     *args,
                                  gint              n_args);

GimpPDBArgType   gimp_argument_type_to_pdb_arg_type (GType           type);
gchar          * gimp_pdb_arg_type_to_string        (GimpPDBArgType  type);


#endif  /*  __GIMP_ARGUMENT_H__  */
