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


typedef void           (* GimpThreadEnterFunc)     (Gimp          *gimp);
typedef void           (* GimpThreadLeaveFunc)     (Gimp          *gimp);
typedef GimpObject   * (* GimpCreateDisplayFunc)   (GimpImage     *gimage,
                                                    gdouble        scale);
typedef void           (* GimpSetBusyFunc)         (Gimp          *gimp);
typedef void           (* GimpUnsetBusyFunc)       (Gimp          *gimp);
typedef void           (* GimpMessageFunc)         (Gimp          *gimp,
                                                    const gchar   *domain,
                                                    const gchar   *message);
typedef void           (* GimpMenusInitFunc)       (Gimp          *gimp,
                                                    GSList        *plug_in_defs,
                                                    const gchar   *std_domain);
typedef void           (* GimpMenusCreateFunc)     (Gimp          *gimp,
                                                    PlugInProcDef *proc_def,
                                                    const gchar   *locale_domain,
                                                    const gchar   *help_domain);
typedef void           (* GimpMenusDeleteFunc)     (Gimp          *gimp,
                                                    const gchar   *menu_path);
typedef GimpProgress * (* GimpProgressStartFunc)   (Gimp          *gimp,
                                                    gint           gdisp_ID,
                                                    const gchar   *message,
                                                    GCallback      cancel_cb,
                                                    gpointer       cancel_data);
typedef GimpProgress * (* GimpProgressRestartFunc) (Gimp          *gimp,
                                                    GimpProgress  *progress,
                                                    const gchar   *message,
                                                    GCallback      cancel_cb,
                                                    gpointer       cancel_data);
typedef void           (* GimpProgressUpdateFunc)  (Gimp          *gimp,
                                                    GimpProgress  *progress,
                                                    gdouble        percentage);
typedef void           (* GimpProgressEndFunc)     (Gimp          *gimp,
                                                    GimpProgress  *progress);
typedef void           (* GimpPDBDialogsCheckFunc) (Gimp          *gimp);
typedef const gchar  * (* GimpGetProgramClassFunc) (Gimp          *gimp);
typedef gchar        * (* GimpGetDisplayNameFunc)  (Gimp          *gimp,
                                                    gint           gdisp_ID,
                                                    gint          *monitor_number);
typedef const gchar  * (* GimpGetThemeDirFunc)     (Gimp          *);


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

  GimpThreadEnterFunc     gui_threads_enter_func;
  GimpThreadLeaveFunc     gui_threads_leave_func;
  GimpCreateDisplayFunc   gui_create_display_func;
  GimpSetBusyFunc         gui_set_busy_func;
  GimpUnsetBusyFunc       gui_unset_busy_func;
  GimpMessageFunc         gui_message_func;
  GimpMenusInitFunc       gui_menus_init_func;
  GimpMenusCreateFunc     gui_menus_create_func;
  GimpMenusDeleteFunc     gui_menus_delete_func;
  GimpProgressStartFunc   gui_progress_start_func;
  GimpProgressRestartFunc gui_progress_restart_func;
  GimpProgressUpdateFunc  gui_progress_update_func;
  GimpProgressEndFunc     gui_progress_end_func;
  GimpPDBDialogsCheckFunc gui_pdb_dialogs_check_func;
  GimpGetProgramClassFunc gui_get_program_class_func;
  GimpGetDisplayNameFunc  gui_get_display_name_func;
  GimpGetThemeDirFunc     gui_get_theme_dir_func;

  gint                    busy;
  guint                   busy_idle_id;

  GList                  *user_units;
  gint                    n_user_units;

  GimpParasiteList       *parasites;

  GimpContainer          *paint_info_list;

  GimpModuleDB           *module_db;
  gboolean                write_modulerc;

  GSList                 *plug_in_defs;
  gboolean                write_pluginrc;

  GSList                 *plug_in_proc_defs;
  GSList                 *plug_in_locale_domains;
  GSList                 *plug_in_help_domains;

  PlugIn                 *current_plug_in;
  GSList                 *open_plug_ins;
  GSList                 *plug_in_stack;
  ProcRecord             *last_plug_in;

  PlugInShm              *plug_in_shm;
  GimpEnvironTable       *environ_table;
  GimpPlugInDebug        *plug_in_debug;

  GimpContainer          *images;
  gint                    next_image_ID;
  guint32                 next_guide_ID;
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

  GHashTable             *procedural_ht;
  GHashTable             *procedural_compat_ht;
  GList                  *procedural_db_data_list;

  GSList                 *load_procs;
  GSList                 *save_procs;

  GimpContainer          *tool_info_list;
  GimpToolInfo           *standard_tool_info;

  /*  the opened and saved images in MRU order  */
  GimpContainer          *documents;

  /*  image_new values  */
  GimpContainer          *templates;
  GimpTemplate           *image_new_last_template;
  gboolean                have_current_cut_buffer;

  /*  the list of all contexts  */
  GList                  *context_list;

  /*  the default context which is initialized from gimprc  */
  GimpContext            *default_context;

  /*  the context used by the interface  */
  GimpContext            *user_context;

  /*  the currently active context  */
  GimpContext            *current_context;
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


GType         gimp_get_type             (void) G_GNUC_CONST;

Gimp        * gimp_new                  (const gchar        *name,
                                         const gchar        *session_name,
                                         gboolean            be_verbose,
                                         gboolean            no_data,
                                         gboolean            no_fonts,
                                         gboolean            no_interface,
                                         gboolean            use_shm,
                                         gboolean            console_messages,
                                         GimpStackTraceMode  stack_trace_mode,
                                         GimpPDBCompatMode   pdb_compat_mode);

void          gimp_load_config          (Gimp               *gimp,
                                         const gchar        *alternate_system_gimprc,
                                         const gchar        *alternate_gimprc);
void          gimp_initialize           (Gimp               *gimp,
                                         GimpInitStatusFunc  status_callback);
void          gimp_restore              (Gimp               *gimp,
                                         GimpInitStatusFunc  status_callback);

void          gimp_exit                 (Gimp               *gimp,
                                         gboolean            force);

void          gimp_set_global_buffer    (Gimp               *gimp,
                                         GimpBuffer         *buffer);

void          gimp_threads_enter        (Gimp               *gimp);
void          gimp_threads_leave        (Gimp               *gimp);

void          gimp_set_busy             (Gimp               *gimp);
void          gimp_set_busy_until_idle  (Gimp               *gimp);
void          gimp_unset_busy           (Gimp               *gimp);

void          gimp_message              (Gimp               *gimp,
                                         const gchar        *domain,
                                         const gchar        *message);
void          gimp_menus_init           (Gimp               *gimp,
                                         GSList             *plug_in_defs,
                                         const gchar        *std_plugins_domain);
void          gimp_menus_create_entry   (Gimp               *gimp,
                                         PlugInProcDef      *proc_def,
                                         const gchar        *locale_domain,
                                         const gchar        *help_domain);
void          gimp_menus_delete_entry   (Gimp               *gimp,
                                         const gchar        *menu_path);
GimpProgress *gimp_start_progress       (Gimp               *gimp,
                                         gint                gdisp_ID,
                                         const gchar        *message,
                                         GCallback           cancel_cb,
                                         gpointer            cancel_data);
GimpProgress *gimp_restart_progress     (Gimp               *gimp,
                                         GimpProgress       *progress,
                                         const gchar        *message,
                                         GCallback           cancel_cb,
                                         gpointer            cancel_data);
void          gimp_update_progress      (Gimp               *gimp,
                                         GimpProgress       *progress,
                                         gdouble             percentage);
void          gimp_end_progress         (Gimp               *gimp,
                                         GimpProgress       *progress);
void          gimp_pdb_dialogs_check    (Gimp               *gimp);
const gchar * gimp_get_program_class    (Gimp               *gimp);
gchar       * gimp_get_display_name     (Gimp               *gimp,
                                         gint                gdisp_ID,
                                         gint               *monitor_number);
const gchar * gimp_get_theme_dir        (Gimp               *gimp);

GimpImage   * gimp_create_image         (Gimp               *gimp,
					 gint                width,
					 gint                height,
					 GimpImageBaseType   type,
					 gboolean            attach_comment);

GimpObject  * gimp_create_display       (Gimp               *gimp,
                                         GimpImage          *gimage,
                                         gdouble             scale);

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
