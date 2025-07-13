/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdbprogress.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#pragma once


#define GIMP_TYPE_PDB_PROGRESS            (gimp_pdb_progress_get_type ())
#define GIMP_PDB_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PDB_PROGRESS, GimpPdbProgress))
#define GIMP_PDB_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PDB_PROGRESS, GimpPdbProgressClass))
#define GIMP_IS_PDB_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PDB_PROGRESS))
#define GIMP_IS_PDB_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PDB_PROGRESS))
#define GIMP_PDB_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PDB_PROGRESS, GimpPdbProgressClass))


typedef struct _GimpPdbProgressClass  GimpPdbProgressClass;

struct _GimpPdbProgress
{
  GObject      object;

  gboolean     active;
  gdouble      value;

  GimpPDB     *pdb;
  GimpContext *context;
  gchar       *callback_name;
  gboolean     callback_busy;
};

struct _GimpPdbProgressClass
{
  GObjectClass  parent_class;

  GList        *progresses;
};


GType             gimp_pdb_progress_get_type        (void) G_GNUC_CONST;

GimpPdbProgress * gimp_pdb_progress_get_by_callback (GimpPdbProgressClass *klass,
                                                     const gchar          *callback_name);
