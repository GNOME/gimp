/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugindef.h
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

#ifndef __GIMP_PLUG_IN_DEF_H__
#define __GIMP_PLUG_IN_DEF_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_PLUG_IN_DEF            (gimp_plug_in_def_get_type ())
#define GIMP_PLUG_IN_DEF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUG_IN_DEF, GimpPlugInDef))
#define GIMP_PLUG_IN_DEF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUG_IN_DEF, GimpPlugInDefClass))
#define GIMP_IS_PLUG_IN_DEF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUG_IN_DEF))
#define GIMP_IS_PLUG_IN_DEF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PLUG_IN_DEF))


typedef struct _GimpPlugInDefClass GimpPlugInDefClass;

struct _GimpPlugInDef
{
  GimpObject  parent_instance;

  GFile      *file;
  GSList     *procedures;
  gchar      *help_domain_name;
  gchar      *help_domain_uri;
  gint64      mtime;
  gboolean    needs_query;  /* Does the plug-in need to be queried ?     */
  gboolean    has_init;     /* Does the plug-in need to be initialized ? */
};

struct _GimpPlugInDefClass
{
  GimpObjectClass  parent_class;
};


GType           gimp_plug_in_def_get_type (void) G_GNUC_CONST;

GimpPlugInDef * gimp_plug_in_def_new      (GFile               *file);

void   gimp_plug_in_def_add_procedure     (GimpPlugInDef       *plug_in_def,
                                           GimpPlugInProcedure *proc);
void   gimp_plug_in_def_remove_procedure  (GimpPlugInDef       *plug_in_def,
                                           GimpPlugInProcedure *proc);

void   gimp_plug_in_def_set_help_domain   (GimpPlugInDef       *plug_in_def,
                                           const gchar         *domain_name,
                                           const gchar         *domain_uri);

void   gimp_plug_in_def_set_mtime         (GimpPlugInDef       *plug_in_def,
                                           gint64               mtime);
void   gimp_plug_in_def_set_needs_query   (GimpPlugInDef       *plug_in_def,
                                           gboolean             needs_query);
void   gimp_plug_in_def_set_has_init      (GimpPlugInDef       *plug_in_def,
                                           gboolean             has_init);


#endif /* __GIMP_PLUG_IN_DEF_H__ */
