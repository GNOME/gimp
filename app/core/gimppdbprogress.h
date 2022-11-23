/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdbprogress.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PDB_PROGRESS_H__
#define __LIGMA_PDB_PROGRESS_H__

G_BEGIN_DECLS


#define LIGMA_TYPE_PDB_PROGRESS            (ligma_pdb_progress_get_type ())
#define LIGMA_PDB_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PDB_PROGRESS, LigmaPdbProgress))
#define LIGMA_PDB_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PDB_PROGRESS, LigmaPdbProgressClass))
#define LIGMA_IS_PDB_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PDB_PROGRESS))
#define LIGMA_IS_PDB_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PDB_PROGRESS))
#define LIGMA_PDB_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PDB_PROGRESS, LigmaPdbProgressClass))


typedef struct _LigmaPdbProgressClass  LigmaPdbProgressClass;

struct _LigmaPdbProgress
{
  GObject      object;

  gboolean     active;
  gdouble      value;

  LigmaPDB     *pdb;
  LigmaContext *context;
  gchar       *callback_name;
  gboolean     callback_busy;
};

struct _LigmaPdbProgressClass
{
  GObjectClass  parent_class;

  GList        *progresses;
};


GType             ligma_pdb_progress_get_type        (void) G_GNUC_CONST;

LigmaPdbProgress * ligma_pdb_progress_get_by_callback (LigmaPdbProgressClass *klass,
                                                     const gchar          *callback_name);


G_END_DECLS

#endif /* __LIGMA_PDB_PROGRESS_H__ */
