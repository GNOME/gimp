/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-def.h
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

#ifndef __PLUG_IN_DEF_H__
#define __PLUG_IN_DEF_H__

#include <time.h>

struct _PlugInDef
{
  gchar    *prog;
  GSList   *procedures;
  gchar    *locale_domain_name;
  gchar    *locale_domain_path;
  gchar    *help_domain_name;
  gchar    *help_domain_uri;
  time_t    mtime;
  gboolean  needs_query;  /* Does the plug-in need to be queried ?     */
  gboolean  has_init;     /* Does the plug-in need to be initialized ? */
};


PlugInDef * plug_in_def_new                    (const gchar         *prog);
void        plug_in_def_free                   (PlugInDef           *plug_in_def);

void        plug_in_def_add_procedure          (PlugInDef           *plug_in_def,
                                                GimpPlugInProcedure *proc);
void        plug_in_def_remove_procedure       (PlugInDef           *plug_in_def,
                                                GimpPlugInProcedure *proc);

void        plug_in_def_set_locale_domain_name (PlugInDef           *plug_in_def,
                                                const gchar         *domain_name);
void        plug_in_def_set_locale_domain_path (PlugInDef           *plug_in_def,
                                                const gchar         *domain_path);

void        plug_in_def_set_help_domain_name   (PlugInDef           *plug_in_def,
                                                const gchar         *domain_name);
void        plug_in_def_set_help_domain_uri    (PlugInDef           *plug_in_def,
                                                const gchar         *domain_uri);

void        plug_in_def_set_mtime              (PlugInDef           *plug_in_def,
                                                time_t               mtime);
void        plug_in_def_set_needs_query        (PlugInDef           *plug_in_def,
                                                gboolean             needs_query);
void        plug_in_def_set_has_init           (PlugInDef           *plug_in_def,
                                                gboolean             has_init);


#endif /* __PLUG_IN_DEF_H__ */
