/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_H__
#define __LIGMA_H__


#include "ligmaobject.h"
#include "ligma-gui.h"


#define LIGMA_TYPE_LIGMA            (ligma_get_type ())
#define LIGMA(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LIGMA, Ligma))
#define LIGMA_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LIGMA, LigmaClass))
#define LIGMA_IS_LIGMA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LIGMA))
#define LIGMA_IS_LIGMA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LIGMA))


typedef struct _LigmaClass LigmaClass;

struct _Ligma
{
  LigmaObject              parent_instance;

  LigmaCoreConfig         *config;
  LigmaCoreConfig         *edit_config; /* don't use this one, it's just
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
  LigmaMessageHandlerType  message_handler;
  gboolean                console_messages;
  gboolean                show_playground;
  gboolean                show_debug_menu;
  LigmaStackTraceMode      stack_trace_mode;
  LigmaPDBCompatMode       pdb_compat_mode;

  LigmaGui                 gui;         /* gui vtable */

  gboolean                restored;    /* becomes TRUE in ligma_restore() */
  gboolean                initialized; /* Fully initialized (only set once at start). */
  gboolean                query_all;   /* Force query all plug-ins. */

  gint                    busy;
  guint                   busy_idle_id;

  GList                  *user_units;
  gint                    n_user_units;

  LigmaParasiteList       *parasites;

  LigmaContainer          *paint_info_list;
  LigmaPaintInfo          *standard_paint_info;

  LigmaModuleDB           *module_db;
  gboolean                write_modulerc;

  LigmaExtensionManager   *extension_manager;
  LigmaPlugInManager      *plug_in_manager;

  GList                  *filter_history;

  LigmaContainer          *images;
  guint32                 next_guide_id;
  guint32                 next_sample_point_id;
  LigmaIdTable            *image_table;
  LigmaIdTable            *item_table;

  LigmaContainer          *displays;
  gint                    next_display_id;

  GList                  *image_windows;

  LigmaImage              *clipboard_image;
  LigmaBuffer             *clipboard_buffer;
  LigmaContainer          *named_buffers;

  LigmaDataFactory        *brush_factory;
  LigmaDataFactory        *dynamics_factory;
  LigmaDataFactory        *mybrush_factory;
  LigmaDataFactory        *pattern_factory;
  LigmaDataFactory        *gradient_factory;
  LigmaDataFactory        *palette_factory;
  LigmaDataFactory        *font_factory;
  LigmaDataFactory        *tool_preset_factory;

  LigmaTagCache           *tag_cache;

  LigmaPDB                *pdb;

  LigmaContainer          *tool_info_list;
  LigmaToolInfo           *standard_tool_info;

  LigmaContainer          *tool_item_list;
  LigmaContainer          *tool_item_ui_list;

  /*  the opened and saved images in MRU order  */
  LigmaContainer          *documents;

  /*  image_new values  */
  LigmaContainer          *templates;
  LigmaTemplate           *image_new_last_template;

  /*  the list of all contexts  */
  GList                  *context_list;

  /*  the default context which is initialized from ligmarc  */
  LigmaContext            *default_context;

  /*  the context used by the interface  */
  LigmaContext            *user_context;
};

struct _LigmaClass
{
  LigmaObjectClass  parent_class;

  void     (* initialize)             (Ligma               *ligma,
                                       LigmaInitStatusFunc  status_callback);
  void     (* restore)                (Ligma               *ligma,
                                       LigmaInitStatusFunc  status_callback);
  gboolean (* exit)                   (Ligma               *ligma,
                                       gboolean            force);

  void     (* clipboard_changed)      (Ligma               *ligma);

  void     (* filter_history_changed) (Ligma               *ligma);

  /*  emitted if an image is loaded and opened with a display  */
  void     (* image_opened)           (Ligma               *ligma,
                                       GFile              *file);
};


GType          ligma_get_type               (void) G_GNUC_CONST;

Ligma         * ligma_new                    (const gchar         *name,
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
                                            LigmaStackTraceMode   stack_trace_mode,
                                            LigmaPDBCompatMode    pdb_compat_mode);
void           ligma_set_show_gui           (Ligma                *ligma,
                                            gboolean             show_gui);
gboolean       ligma_get_show_gui           (Ligma                *ligma);

void           ligma_load_config            (Ligma                *ligma,
                                            GFile               *alternate_system_ligmarc,
                                            GFile               *alternate_ligmarc);
void           ligma_initialize             (Ligma                *ligma,
                                            LigmaInitStatusFunc   status_callback);
void           ligma_restore                (Ligma                *ligma,
                                            LigmaInitStatusFunc   status_callback,
                                            GError             **error);
gboolean       ligma_is_restored            (Ligma                *ligma);

void           ligma_exit                   (Ligma                *ligma,
                                            gboolean             force);

GList        * ligma_get_image_iter         (Ligma                *ligma);
GList        * ligma_get_display_iter       (Ligma                *ligma);
GList        * ligma_get_image_windows      (Ligma                *ligma);
GList        * ligma_get_paint_info_iter    (Ligma                *ligma);
GList        * ligma_get_tool_info_iter     (Ligma                *ligma);
GList        * ligma_get_tool_item_iter     (Ligma                *ligma);
GList        * ligma_get_tool_item_ui_iter  (Ligma                *ligma);

LigmaObject   * ligma_get_clipboard_object   (Ligma                *ligma);

void           ligma_set_clipboard_image    (Ligma                *ligma,
                                            LigmaImage           *image);
LigmaImage    * ligma_get_clipboard_image    (Ligma                *ligma);

void           ligma_set_clipboard_buffer   (Ligma                *ligma,
                                            LigmaBuffer          *buffer);
LigmaBuffer   * ligma_get_clipboard_buffer   (Ligma                *ligma);

LigmaImage    * ligma_create_image           (Ligma                *ligma,
                                            gint                 width,
                                            gint                 height,
                                            LigmaImageBaseType    type,
                                            LigmaPrecision        precision,
                                            gboolean             attach_comment);

void           ligma_set_default_context    (Ligma                *ligma,
                                            LigmaContext         *context);
LigmaContext  * ligma_get_default_context    (Ligma                *ligma);

void           ligma_set_user_context       (Ligma                *ligma,
                                            LigmaContext         *context);
LigmaContext  * ligma_get_user_context       (Ligma                *ligma);

LigmaToolInfo * ligma_get_tool_info          (Ligma                *ligma,
                                            const gchar         *tool_name);

void           ligma_message                (Ligma                *ligma,
                                            GObject             *handler,
                                            LigmaMessageSeverity  severity,
                                            const gchar         *format,
                                            ...) G_GNUC_PRINTF (4, 5);
void           ligma_message_valist         (Ligma                *ligma,
                                            GObject             *handler,
                                            LigmaMessageSeverity  severity,
                                            const gchar         *format,
                                            va_list              args) G_GNUC_PRINTF (4, 0);
void           ligma_message_literal        (Ligma                *ligma,
                                            GObject             *handler,
                                            LigmaMessageSeverity  severity,
                                            const gchar         *message);

void           ligma_filter_history_changed (Ligma                *ligma);

void           ligma_image_opened           (Ligma                *ligma,
                                            GFile               *file);

GFile        * ligma_get_temp_file          (Ligma                *ligma,
                                            const gchar         *extension);


#endif  /* __LIGMA_H__ */
