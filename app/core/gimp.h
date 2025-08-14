/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

  GApplication           *app;

  GimpCoreConfig         *config;
  GimpCoreConfig         *edit_config; /* don't use this one, it's just
                                        * for the preferences dialog
                                        */
  gchar                  *session_name;
  GFile                  *default_folder;

  gboolean                be_verbose;
  gboolean                no_data;
  gboolean                no_fonts;
  gboolean                no_interface;
  gboolean                show_gui;
  gboolean                use_shm;
  gboolean                use_cpu_accel;
  GimpMessageHandlerType  message_handler;
  gboolean                console_messages;
  gboolean                show_playground;
  gboolean                show_debug_menu;
  GimpStackTraceMode      stack_trace_mode;
  GimpPDBCompatMode       pdb_compat_mode;

  GimpGui                 gui;         /* gui vtable */

  gboolean                restored;    /* becomes TRUE in gimp_restore() */
  gboolean                initialized; /* Fully initialized (only set once at start). */
  gboolean                query_all;   /* Force query all plug-ins. */

  gint                    busy;
  guint                   busy_idle_id;

  GList                  *user_units;

  GimpParasiteList       *parasites;

  GimpContainer          *paint_info_list;
  GimpPaintInfo          *standard_paint_info;

  GimpModuleDB           *module_db;
  gboolean                write_modulerc;

  GimpExtensionManager   *extension_manager;
  GimpPlugInManager      *plug_in_manager;

  GList                  *filter_history;

  GimpContainer          *images;
  guint32                 next_guide_id;
  guint32                 next_sample_point_id;
  GimpIdTable            *image_table;
  GimpIdTable            *item_table;
  GimpIdTable            *drawable_filter_table;

  GimpContainer          *displays;
  gint                    next_display_id;

  GList                  *image_windows;

  GObject                *controller_manager;

  GimpImage              *clipboard_image;
  GimpBuffer             *clipboard_buffer;
  GimpContainer          *named_buffers;

  GimpDataFactory        *brush_factory;
  GimpDataFactory        *dynamics_factory;
  GimpDataFactory        *mybrush_factory;
  GimpDataFactory        *pattern_factory;
  GimpDataFactory        *gradient_factory;
  GimpDataFactory        *palette_factory;
  GimpDataFactory        *font_factory;
  GimpDataFactory        *tool_preset_factory;

  GimpTagCache           *tag_cache;

  GimpPDB                *pdb;

  GimpContainer          *tool_info_list;
  GimpToolInfo           *standard_tool_info;

  GimpContainer          *tool_item_list;
  GimpContainer          *tool_item_ui_list;

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

  void     (* initialize)             (Gimp               *gimp,
                                       GimpInitStatusFunc  status_callback);
  void     (* restore)                (Gimp               *gimp,
                                       GimpInitStatusFunc  status_callback);
  gboolean (* exit)                   (Gimp               *gimp,
                                       gboolean            force);

  void     (* clipboard_changed)      (Gimp               *gimp);

  void     (* filter_history_changed) (Gimp               *gimp);

  /*  emitted if an image is loaded and opened with a display  */
  void     (* image_opened)           (Gimp               *gimp,
                                       GFile              *file);
};


GType          gimp_get_type               (void) G_GNUC_CONST;

Gimp         * gimp_new                    (const gchar         *name,
                                            const gchar         *session_name,
                                            GFile               *default_folder,
                                            gboolean             be_verbose,
                                            gboolean             no_data,
                                            gboolean             no_fonts,
                                            gboolean             no_interface,
                                            gboolean             use_shm,
                                            gboolean             use_cpu_accel,
                                            gboolean             console_messages,
                                            gboolean             show_playground,
                                            gboolean             show_debug_menu,
                                            GimpStackTraceMode   stack_trace_mode,
                                            GimpPDBCompatMode    pdb_compat_mode);
void           gimp_set_show_gui           (Gimp                *gimp,
                                            gboolean             show_gui);
gboolean       gimp_get_show_gui           (Gimp                *gimp);

void           gimp_load_config            (Gimp                *gimp,
                                            GFile               *alternate_system_gimprc,
                                            GFile               *alternate_gimprc);
void           gimp_initialize             (Gimp                *gimp,
                                            GimpInitStatusFunc   status_callback);
void           gimp_restore                (Gimp                *gimp,
                                            GimpInitStatusFunc   status_callback,
                                            GError             **error);
gboolean       gimp_is_restored            (Gimp                *gimp);

void           gimp_exit                   (Gimp                *gimp,
                                            gboolean             force);

GList        * gimp_get_image_iter         (Gimp                *gimp);
GList        * gimp_get_display_iter       (Gimp                *gimp);
GList        * gimp_get_image_windows      (Gimp                *gimp);
GList        * gimp_get_paint_info_iter    (Gimp                *gimp);
GList        * gimp_get_tool_info_iter     (Gimp                *gimp);
GList        * gimp_get_tool_item_iter     (Gimp                *gimp);
GList        * gimp_get_tool_item_ui_iter  (Gimp                *gimp);

GimpObject   * gimp_get_clipboard_object   (Gimp                *gimp);

void           gimp_set_clipboard_image    (Gimp                *gimp,
                                            GimpImage           *image);
GimpImage    * gimp_get_clipboard_image    (Gimp                *gimp);

void           gimp_set_clipboard_buffer   (Gimp                *gimp,
                                            GimpBuffer          *buffer);
GimpBuffer   * gimp_get_clipboard_buffer   (Gimp                *gimp);

GimpImage    * gimp_create_image           (Gimp                *gimp,
                                            gint                 width,
                                            gint                 height,
                                            GimpImageBaseType    type,
                                            GimpPrecision        precision,
                                            gboolean             attach_comment);

void           gimp_set_default_context    (Gimp                *gimp,
                                            GimpContext         *context);
GimpContext  * gimp_get_default_context    (Gimp                *gimp);

void           gimp_set_user_context       (Gimp                *gimp,
                                            GimpContext         *context);
GimpContext  * gimp_get_user_context       (Gimp                *gimp);

GimpToolInfo * gimp_get_tool_info          (Gimp                *gimp,
                                            const gchar         *tool_name);

void           gimp_set_last_template      (Gimp                *gimp,
                                            GimpTemplate        *_template);
GimpTemplate * gimp_get_last_template      (Gimp                *gimp);

void           gimp_message                (Gimp                *gimp,
                                            GObject             *handler,
                                            GimpMessageSeverity  severity,
                                            const gchar         *format,
                                            ...) G_GNUC_PRINTF (4, 5);
void           gimp_message_valist         (Gimp                *gimp,
                                            GObject             *handler,
                                            GimpMessageSeverity  severity,
                                            const gchar         *format,
                                            va_list              args) G_GNUC_PRINTF (4, 0);
void           gimp_message_literal        (Gimp                *gimp,
                                            GObject             *handler,
                                            GimpMessageSeverity  severity,
                                            const gchar         *message);

void           gimp_filter_history_changed (Gimp                *gimp);

void           gimp_image_opened           (Gimp                *gimp,
                                            GFile               *file);

GFile        * gimp_get_temp_file          (Gimp                *gimp,
                                            const gchar         *extension);

GimpDataFactory *
               gimp_get_data_factory       (Gimp                *gimp,
                                            GType                data_type);
