/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include "config.h"

#include <glib-object.h>

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimplist.h"
#include "core/gimppaintinfo.h"

#include "paint.h"
#include "gimpairbrush.h"
#include "gimpclone.h"
#include "gimpconvolve.h"
#include "gimpdodgeburn.h"
#include "gimperaser.h"
#include "gimppaintbrush.h"
#include "gimppencil.h"
#include "gimpsmudge.h"


/*  local function prototypes  */

static void   paint_register (Gimp  *gimp,
                              GType  paint_type,
                              GType  paint_options_type);


/*  public functions  */

void
paint_init (Gimp *gimp)
{
  GimpPaintRegisterFunc register_funcs[] =
  {
    gimp_smudge_register,
    gimp_dodge_burn_register,
    gimp_convolve_register,
    gimp_clone_register,
    gimp_airbrush_register,
    gimp_eraser_register,
    gimp_paintbrush_register,
    gimp_pencil_register,
  };

  gint i;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->paint_info_list = gimp_list_new (GIMP_TYPE_PAINT_INFO,
                                         GIMP_CONTAINER_POLICY_STRONG);
  gimp_object_set_name (GIMP_OBJECT (gimp->paint_info_list), "paint infos");

  for (i = 0; i < G_N_ELEMENTS (register_funcs); i++)
    {
      register_funcs[i] (gimp, paint_register);
    }
}

void
paint_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->paint_info_list)
    {
      g_object_unref (gimp->paint_info_list);
      gimp->paint_info_list = NULL;
    }
}


/*  private functions  */

static void
paint_register (Gimp  *gimp,
                GType  paint_type,
                GType  paint_options_type)
{
  GimpPaintInfo *paint_info;
  const gchar   *pdb_string;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (g_type_is_a (paint_type, GIMP_TYPE_PAINT_CORE));
  g_return_if_fail (g_type_is_a (paint_options_type, GIMP_TYPE_PAINT_OPTIONS));
  
  if (paint_type == GIMP_TYPE_PENCIL)
    {
      pdb_string = "gimp_pencil";
    }
  else if (paint_type == GIMP_TYPE_PAINTBRUSH)
    {
      pdb_string = "gimp_paintbrush_default";
    }
  else if (paint_type == GIMP_TYPE_ERASER)
    {
      pdb_string = "gimp_eraser_default";
    }
  else if (paint_type == GIMP_TYPE_AIRBRUSH)
    {
      pdb_string = "gimp_airbrush_default";
    }
  else if (paint_type == GIMP_TYPE_CLONE)
    {
      pdb_string = "gimp_clone_default";
    }
  else if (paint_type == GIMP_TYPE_CONVOLVE)
    {
      pdb_string = "gimp_convolve_default";
    }
  else if (paint_type == GIMP_TYPE_SMUDGE)
    {
      pdb_string = "gimp_smudge_default";
    }
  else if (paint_type == GIMP_TYPE_DODGE_BURN)
    {
      pdb_string = "gimp_dodgeburn_default";
    }
  else
    {
      pdb_string = "gimp_paintbrush_default";
    }

  paint_info = gimp_paint_info_new (gimp,
                                    paint_type,
                                    paint_options_type,
                                    pdb_string);

  gimp_container_add (gimp->paint_info_list, GIMP_OBJECT (paint_info));

  g_object_unref (paint_info);
}
