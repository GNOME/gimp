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
#include "gimp-gui.h"


#define GIMP_TYPE_GIMP            (gimp_get_type ())
#define GIMP(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GIMP, Gimp))
#define GIMP_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GIMP, GimpClass))
#define GIMP_IS_GIMP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GIMP))
#define GIMP_IS_GIMP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GIMP))


typedef struct _GimpClass GimpClass;

struct _Gimp
{
  GimpObject              parent_instance;

  GimpCoreConfig         *config;
  GimpCoreConfig         *edit_config; /* don't use this one, it's just
                                        * for the preferences dialog
                                        */
  gchar                  *session_name;

  gboolean                be_verbose;
  gboolean                no_data;
  gboolean                no_fonts;
  gboolean                no_interface;
  gboolean                use_shm;
  GimpMessageHandlerType  message_handler;
  gboolean                console_messages;
  GimpStackTraceMode      stack_trace_mode;
  GimpPDBCompatMode       pdb_compat_mode;

  GimpGui                 gui; /* gui vtable */

  gint                    busy;
  guint                   busy_idle_id;

  GList                  *user_units;
  gint                    n_user_units;

  GimpParasiteList       *parasites;

  GimpContainer          *paint_info_list;
  GimpPaintInfo          *standard_paint_info;

  GimpModuleDB           *module_db;
  gboolean                write_modulerc;

  GimpPlugInManager      *plug_in_manager;

  GimpContainer          *images;
  gint                    next_image_ID;
  guint32                 next_guide_ID;
  guint32                 next_sample_point_ID;
  GHashTable             *image_table;

  gint                    next_item_ID;
  GHashTable             *item_table;

  GimpContainer          *displays;
  gint                    next_display_ID;

  GimpBuffer             *global_buffer;
  GimpContainer          *named_buffers;

  GimpContainer          *fonts;

  GimpDataFactory        *brush_factory;
  GimpDataFactory        *pattern_factory;
  GimpDataFactory        *gradient_factory;
  GimpDataFactory        *palette_factory;

  GimpPDB                *pdb;

  GimpContainer          *tool_info_list;
  GimpToolInfo           *standard_tool_info;

  /*  the opened and saved images in MRU order  */
  GimpContainer          *documents;

  /*  image_new values  */
  GimpContainer          *templates;
  GimpTemplate           *image_new_last_template;

  /*  the list of all contexts  */
  GList                  *context_list;

  /*  the default context which is initialized from gimprc  */
  GimpContext            *default_context;

  /*  the context used by the interface  */
  GimpContext            *user_context;
};

struct _GimpClass
{
  GimpObjectClass  parent_class;

  void     (* initialize)     (Gimp               *gimp,
                               GimpInitStatusFunc  status_callback);
  void     (* restore)        (Gimp               *gimp,
                               GimpInitStatusFunc  status_callback);
  gboolean (* exit)           (Gimp               *gimp,
                               gboolean            force);

  void     (* buffer_changed) (Gimp               *gimp);
};


GType          gimp_get_type             (void) G_GNUC_CONST;

Gimp         * gimp_new                  (const gchar         *name,
                                          const gchar         *session_name,
                                          gboolean             be_verbose,
                                          gboolean             no_data,
                                          gboolean             no_fonts,
                                          gboolean             no_interface,
                                          gboolean             use_shm,
                                          gboolean             console_messages,
                                          GimpStackTraceMode   stack_trace_mode,
                                          GimpPDBCompatMode    pdb_compat_mode);

void           gimp_load_config          (Gimp                *gimp,
                                          const gchar         *alternate_system_gimprc,
                                          const gchar         *alternate_gimprc);
void           gimp_initialize           (Gimp                *gimp,
                                          GimpInitStatusFunc   status_callback);
void           gimp_restore              (Gimp                *gimp,
                                          GimpInitStatusFunc   status_callback);

void           gimp_exit                 (Gimp                *gimp,
                                          gboolean             force);

void           gimp_set_global_buffer    (Gimp                *gimp,
                                          GimpBuffer          *buffer);

GimpImage    * gimp_create_image         (Gimp                *gimp,
                                          gint                 width,
                                          gint                 height,
                                          GimpImageBaseType    type,
                                          gboolean             attach_comment);

void           gimp_set_default_context  (Gimp                *gimp,
                                          GimpContext         *context);
GimpContext  * gimp_get_default_context  (Gimp                *gimp);

void           gimp_set_user_context     (Gimp                *gimp,
                                          GimpContext         *context);
GimpContext  * gimp_get_user_context     (Gimp                *gimp);

GimpToolInfo * gimp_get_tool_info        (Gimp                *gimp,
                                          const gchar         *tool_name);

void           gimp_message              (Gimp                *gimp,
                                          GObject             *handler,
                                          GimpMessageSeverity  severity,
                                          const gchar         *format,
                                          ...) G_GNUC_PRINTF(4,5);
void           gimp_message_valist       (Gimp                *gimp,
                                          GObject             *handler,
                                          GimpMessageSeverity  severity,
                                          const gchar         *format,
                                          va_list              args);


#endif  /* __GIMP_H__ */
