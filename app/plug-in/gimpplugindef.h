/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugindef.h
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

#ifndef __LIGMA_PLUG_IN_DEF_H__
#define __LIGMA_PLUG_IN_DEF_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_PLUG_IN_DEF            (ligma_plug_in_def_get_type ())
#define LIGMA_PLUG_IN_DEF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PLUG_IN_DEF, LigmaPlugInDef))
#define LIGMA_PLUG_IN_DEF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PLUG_IN_DEF, LigmaPlugInDefClass))
#define LIGMA_IS_PLUG_IN_DEF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PLUG_IN_DEF))
#define LIGMA_IS_PLUG_IN_DEF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PLUG_IN_DEF))


typedef struct _LigmaPlugInDefClass LigmaPlugInDefClass;

struct _LigmaPlugInDef
{
  LigmaObject  parent_instance;

  GFile      *file;
  GSList     *procedures;
  gchar      *help_domain_name;
  gchar      *help_domain_uri;
  gint64      mtime;
  gboolean    needs_query;  /* Does the plug-in need to be queried ?     */
  gboolean    has_init;     /* Does the plug-in need to be initialized ? */
};

struct _LigmaPlugInDefClass
{
  LigmaObjectClass  parent_class;
};


GType           ligma_plug_in_def_get_type (void) G_GNUC_CONST;

LigmaPlugInDef * ligma_plug_in_def_new      (GFile               *file);

void   ligma_plug_in_def_add_procedure     (LigmaPlugInDef       *plug_in_def,
                                           LigmaPlugInProcedure *proc);
void   ligma_plug_in_def_remove_procedure  (LigmaPlugInDef       *plug_in_def,
                                           LigmaPlugInProcedure *proc);

void   ligma_plug_in_def_set_help_domain   (LigmaPlugInDef       *plug_in_def,
                                           const gchar         *domain_name,
                                           const gchar         *domain_uri);

void   ligma_plug_in_def_set_mtime         (LigmaPlugInDef       *plug_in_def,
                                           gint64               mtime);
void   ligma_plug_in_def_set_needs_query   (LigmaPlugInDef       *plug_in_def,
                                           gboolean             needs_query);
void   ligma_plug_in_def_set_has_init      (LigmaPlugInDef       *plug_in_def,
                                           gboolean             has_init);


#endif /* __LIGMA_PLUG_IN_DEF_H__ */
