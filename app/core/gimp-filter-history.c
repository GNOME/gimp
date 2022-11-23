/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
 *
 * ligma-filter-history.c
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligma-filter-history.h"

#include "pdb/ligmaprocedure.h"


/*  local function prototypes  */

static gint   ligma_filter_history_compare (LigmaProcedure *proc1,
                                           LigmaProcedure *proc2);


/*  public functions  */

gint
ligma_filter_history_size (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), 0);

  return MAX (1, ligma->config->filter_history_size);
}

gint
ligma_filter_history_length (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), 0);

  return g_list_length (ligma->filter_history);
}

LigmaProcedure *
ligma_filter_history_nth (Ligma  *ligma,
                         gint   n)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_list_nth_data (ligma->filter_history, n);
}

void
ligma_filter_history_add (Ligma          *ligma,
                         LigmaProcedure *procedure)
{
  GList *link;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  /* return early if the procedure is already at the top */
  if (ligma->filter_history &&
      ligma_filter_history_compare (ligma->filter_history->data, procedure) == 0)
    return;

  /* ref new first then unref old, they might be the same */
  g_object_ref (procedure);

  link = g_list_find_custom (ligma->filter_history, procedure,
                             (GCompareFunc) ligma_filter_history_compare);

  if (link)
    {
      g_object_unref (link->data);
      ligma->filter_history = g_list_delete_link (ligma->filter_history, link);
    }

  ligma->filter_history = g_list_prepend (ligma->filter_history, procedure);

  link = g_list_nth (ligma->filter_history, ligma_filter_history_size (ligma));

  if (link)
    {
      g_object_unref (link->data);
      ligma->filter_history = g_list_delete_link (ligma->filter_history, link);
    }

  ligma_filter_history_changed (ligma);
}

void
ligma_filter_history_remove (Ligma          *ligma,
                            LigmaProcedure *procedure)
{
  GList *link;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_PROCEDURE (procedure));

  link = g_list_find_custom (ligma->filter_history, procedure,
                             (GCompareFunc) ligma_filter_history_compare);

  if (link)
    {
      g_object_unref (link->data);
      ligma->filter_history = g_list_delete_link (ligma->filter_history, link);

      ligma_filter_history_changed (ligma);
    }
}

void
ligma_filter_history_clear (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (ligma->filter_history)
    {
      g_list_free_full (ligma->filter_history, (GDestroyNotify) g_object_unref);
      ligma->filter_history = NULL;

      ligma_filter_history_changed (ligma);
    }
}


/*  private functions  */

static gint
ligma_filter_history_compare (LigmaProcedure *proc1,
                             LigmaProcedure *proc2)
{
  /*  the procedures can have the same name, but could still be two
   *  different filters using the same operation, so also compare
   *  their menu labels
   */
  return (ligma_procedure_name_compare (proc1, proc2) ||
          strcmp (ligma_procedure_get_menu_label (proc1),
                  ligma_procedure_get_menu_label (proc2)));
}
