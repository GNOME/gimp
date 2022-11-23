/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "core/ligma.h"
#include "core/ligmalist.h"
#include "core/ligmapaintinfo.h"

#include "ligma-paint.h"
#include "ligmaairbrush.h"
#include "ligmaclone.h"
#include "ligmaconvolve.h"
#include "ligmadodgeburn.h"
#include "ligmaeraser.h"
#include "ligmaheal.h"
#include "ligmaink.h"
#include "ligmamybrushcore.h"
#include "ligmapaintoptions.h"
#include "ligmapaintbrush.h"
#include "ligmapencil.h"
#include "ligmaperspectiveclone.h"
#include "ligmasmudge.h"


/*  local function prototypes  */

static void   ligma_paint_register (Ligma        *ligma,
                                   GType        paint_type,
                                   GType        paint_options_type,
                                   const gchar *identifier,
                                   const gchar *blurb,
                                   const gchar *icon_name);


/*  public functions  */

void
ligma_paint_init (Ligma *ligma)
{
  LigmaPaintRegisterFunc register_funcs[] =
  {
    ligma_dodge_burn_register,
    ligma_smudge_register,
    ligma_convolve_register,
    ligma_perspective_clone_register,
    ligma_heal_register,
    ligma_clone_register,
    ligma_mybrush_core_register,
    ligma_ink_register,
    ligma_airbrush_register,
    ligma_eraser_register,
    ligma_paintbrush_register,
    ligma_pencil_register
  };

  gint i;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma->paint_info_list = ligma_list_new (LIGMA_TYPE_PAINT_INFO, FALSE);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->paint_info_list),
                               "paint infos");

  ligma_container_freeze (ligma->paint_info_list);

  for (i = 0; i < G_N_ELEMENTS (register_funcs); i++)
    {
      register_funcs[i] (ligma, ligma_paint_register);
    }

  ligma_container_thaw (ligma->paint_info_list);
}

void
ligma_paint_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_paint_info_set_standard (ligma, NULL);

  if (ligma->paint_info_list)
    {
      ligma_container_foreach (ligma->paint_info_list,
                              (GFunc) g_object_run_dispose, NULL);
      g_clear_object (&ligma->paint_info_list);
    }
}


/*  private functions  */

static void
ligma_paint_register (Ligma        *ligma,
                     GType        paint_type,
                     GType        paint_options_type,
                     const gchar *identifier,
                     const gchar *blurb,
                     const gchar *icon_name)
{
  LigmaPaintInfo *paint_info;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (g_type_is_a (paint_type, LIGMA_TYPE_PAINT_CORE));
  g_return_if_fail (g_type_is_a (paint_options_type, LIGMA_TYPE_PAINT_OPTIONS));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (blurb != NULL);

  paint_info = ligma_paint_info_new (ligma,
                                    paint_type,
                                    paint_options_type,
                                    identifier,
                                    blurb,
                                    icon_name);

  ligma_container_add (ligma->paint_info_list, LIGMA_OBJECT (paint_info));
  g_object_unref (paint_info);

  if (paint_type == LIGMA_TYPE_PAINTBRUSH)
    ligma_paint_info_set_standard (ligma, paint_info);
}
