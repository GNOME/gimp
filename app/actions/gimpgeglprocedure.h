/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmageglprocedure.h
 * Copyright (C) 2016 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_GEGL_PROCEDURE_H__
#define __LIGMA_GEGL_PROCEDURE_H__


#include "pdb/ligmaprocedure.h"


#define LIGMA_TYPE_GEGL_PROCEDURE            (ligma_gegl_procedure_get_type ())
#define LIGMA_GEGL_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GEGL_PROCEDURE, LigmaGeglProcedure))
#define LIGMA_GEGL_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GEGL_PROCEDURE, LigmaGeglProcedureClass))
#define LIGMA_IS_GEGL_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GEGL_PROCEDURE))
#define LIGMA_IS_GEGL_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GEGL_PROCEDURE))
#define LIGMA_GEGL_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_GEGL_PROCEDURE, LigmaGeglProcedureClass))


typedef struct _LigmaGeglProcedure      LigmaGeglProcedure;
typedef struct _LigmaGeglProcedureClass LigmaGeglProcedureClass;

struct _LigmaGeglProcedure
{
  LigmaProcedure  parent_instance;

  gchar         *operation;

  LigmaRunMode    default_run_mode;
  LigmaObject    *default_settings;

  gchar         *menu_label;
};

struct _LigmaGeglProcedureClass
{
  LigmaProcedureClass parent_class;
};


GType           ligma_gegl_procedure_get_type (void) G_GNUC_CONST;

LigmaProcedure * ligma_gegl_procedure_new      (Ligma        *ligma,
                                              LigmaRunMode  default_run_mode,
                                              LigmaObject  *default_settings,
                                              const gchar *operation,
                                              const gchar *name,
                                              const gchar *menu_label,
                                              const gchar *tooltip,
                                              const gchar *icon_name,
                                              const gchar *help_id);


#endif /* __LIGMA_GEGL_PROCEDURE_H__ */
