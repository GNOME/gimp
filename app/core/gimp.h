/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_H__
#define __GIMP_H__


#include "gimpobject.h"
#include "gimpimage-new.h"


typedef void (* GimpCreateDisplayFunc) (GimpImage *gimage);
typedef void (* GimpSetBusyFunc)       (Gimp      *gimp);
typedef void (* GimpUnsetBusyFunc)     (Gimp      *gimp);


#define GIMP_TYPE_GIMP            (gimp_get_type ())
#define GIMP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GIMP, Gimp))
#define GIMP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GIMP, GimpClass))
#define GIMP_IS_GIMP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GIMP))
#define GIMP_IS_GIMP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GIMP))


typedef struct _GimpClass GimpClass;

struct _Gimp
{
  GimpObject             parent_instance;

  GimpCoreConfig        *config;

  gboolean               be_verbose;
  gboolean               no_data;
  gboolean               no_interface;

  GimpCreateDisplayFunc  create_display_func;
  GimpSetBusyFunc        gui_set_busy_func;
  GimpUnsetBusyFunc      gui_unset_busy_func;

  gboolean               busy;
  guint                  busy_idle_id;

  GList                 *user_units;
  gint                   n_user_units;

  GimpParasiteList      *parasites;

  GimpContainer         *modules;
  gboolean               write_modulerc;

  GimpContainer         *images;
  gint                   next_image_ID;
  guint32                next_guide_ID;
  GHashTable            *image_table;

  gint                   next_drawable_ID;
  GHashTable            *drawable_table;

  TileManager           *global_buffer;
  GimpContainer         *named_buffers;

  GimpDataFactory       *brush_factory;
  GimpDataFactory       *pattern_factory;
  GimpDataFactory       *gradient_factory;
  GimpDataFactory       *palette_factory;

  GHashTable            *procedural_ht;

  GSList                *load_procs;
  GSList                *save_procs;

  GimpContainer         *tool_info_list;
  GimpToolInfo          *standard_tool_info;

  /*  the opened and saved images in MRU order  */
  GimpContainer         *documents;

  /*  image_new values  */
  GList                 *image_base_type_names;
  GList                 *fill_type_names;
  GimpImageNewValues     image_new_last_values;
  gboolean               have_current_cut_buffer;

  /*  the list of all contexts  */
  GList                 *context_list;

  /*  the hardcoded standard context  */
  GimpContext           *standard_context;

  /*  the default context which is initialized from gimprc  */
  GimpContext           *default_context;

  /*  the context used by the interface  */
  GimpContext           *user_context;

  /*  the currently active context  */
  GimpContext           *current_context;
};

struct _GimpClass
{
  GimpObjectClass  parent_class;
};


GType         gimp_get_type             (void);

Gimp        * gimp_new                  (gboolean            be_verbose,
                                         gboolean            no_data,
                                         gboolean            no_interface);

void          gimp_initialize           (Gimp               *gimp,
                                         GimpInitStatusFunc  status_callback);

void          gimp_restore              (Gimp               *gimp,
                                         GimpInitStatusFunc  status_callback,
					 gboolean            no_data);
void          gimp_shutdown             (Gimp               *gimp);

void          gimp_set_busy             (Gimp               *gimp);
void          gimp_set_busy_until_idle  (Gimp               *gimp);
void          gimp_unset_busy           (Gimp               *gimp);

GimpImage   * gimp_create_image         (Gimp               *gimp,
					 gint                width,
					 gint                height,
					 GimpImageBaseType   type,
					 gboolean            attach_comment);

void          gimp_create_display       (Gimp               *gimp,
					 GimpImage          *gimage);

/*
void          gimp_open_file            (Gimp               *gimp,
					 const gchar        *filename,
					 gboolean            with_display);
*/

GimpContext * gimp_create_context       (Gimp               *gimp,
					 const gchar        *name,
					 GimpContext        *template);

GimpContext * gimp_get_standard_context (Gimp               *gimp);

void          gimp_set_default_context  (Gimp               *gimp,
					 GimpContext        *context);
GimpContext * gimp_get_default_context  (Gimp               *gimp);

void          gimp_set_user_context     (Gimp               *gimp,
					 GimpContext        *context);
GimpContext * gimp_get_user_context     (Gimp               *gimp);

void          gimp_set_current_context  (Gimp               *gimp,
					 GimpContext        *context);
GimpContext * gimp_get_current_context  (Gimp               *gimp);


#endif  /* __GIMP_H__ */
