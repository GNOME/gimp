/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#include "config.h"

#include <string.h> /* strlen */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase-private.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimprc.h"

#include "gegl/gimp-babl.h"

#include "pdb/gimppdb.h"
#include "pdb/gimp-pdb-compat.h"
#include "pdb/internal-procs.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginmanager-restore.h"

#include "paint/gimp-paint.h"

#include "xcf/xcf.h"
#include "file-data/file-data.h"

#include "gimp.h"
#include "gimp-contexts.h"
#include "gimp-data-factories.h"
#include "gimp-filter-history.h"
#include "gimp-memsize.h"
#include "gimp-modules.h"
#include "gimp-parasites.h"
#include "gimp-templates.h"
#include "gimp-units.h"
#include "gimp-utils.h"
#include "gimpbrush.h"
#include "gimpbuffer.h"
#include "gimpcontext.h"
#include "gimpdynamics.h"
#include "gimpdocumentlist.h"
#include "gimpextensionmanager.h"
#include "gimpgradient.h"
#include "gimpidtable.h"
#include "gimpimage.h"
#include "gimpimagefile.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpmybrush.h"
#include "gimppalette.h"
#include "gimpparasitelist.h"
#include "gimppattern.h"
#include "gimptemplate.h"
#include "gimptoolinfo.h"
#include "gimptreeproxy.h"

#include "gimp-intl.h"


/*  we need to register all enum types so they are known to the type
 *  system by name, re-use the files the pdb generated for libgimp
 */
void           gimp_enums_init           (void);
const gchar ** gimp_enums_get_type_names (gint *n_type_names);
#include "libgimp/gimpenums.c.tail"


enum
{
  INITIALIZE,
  RESTORE,
  EXIT,
  CLIPBOARD_CHANGED,
  FILTER_HISTORY_CHANGED,
  IMAGE_OPENED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VERBOSE
};


static void      gimp_constructed                    (GObject           *object);
static void      gimp_set_property                   (GObject           *object,
                                                      guint              property_id,
                                                      const GValue      *value,
                                                      GParamSpec        *pspec);
static void      gimp_get_property                   (GObject           *object,
                                                      guint              property_id,
                                                      GValue            *value,
                                                      GParamSpec        *pspec);
static void      gimp_dispose                        (GObject           *object);
static void      gimp_finalize                       (GObject           *object);

static gint64    gimp_get_memsize                    (GimpObject        *object,
                                                      gint64            *gui_size);

static void      gimp_real_initialize                (Gimp              *gimp,
                                                      GimpInitStatusFunc status_callback);
static void      gimp_real_restore                   (Gimp              *gimp,
                                                      GimpInitStatusFunc status_callback);
static gboolean  gimp_real_exit                      (Gimp              *gimp,
                                                      gboolean           force);

static void      gimp_global_config_notify           (GObject           *global_config,
                                                      GParamSpec        *param_spec,
                                                      GObject           *edit_config);
static void      gimp_edit_config_notify             (GObject           *edit_config,
                                                      GParamSpec        *param_spec,
                                                      GObject           *global_config);

static gboolean  gimp_exit_idle_cleanup_stray_images (Gimp              *gimp);


G_DEFINE_TYPE (Gimp, gimp, GIMP_TYPE_OBJECT)

#define parent_class gimp_parent_class

static guint gimp_signals[LAST_SIGNAL] = { 0, };


static void
gimp_class_init (GimpClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);

  gimp_signals[INITIALIZE] =
    g_signal_new ("initialize",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass, initialize),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  gimp_signals[RESTORE] =
    g_signal_new ("restore",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass, restore),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  gimp_signals[EXIT] =
    g_signal_new ("exit",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass, exit),
                  g_signal_accumulator_true_handled, NULL,
                  gimp_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_BOOLEAN);

  gimp_signals[CLIPBOARD_CHANGED] =
    g_signal_new ("clipboard-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass, clipboard_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_signals[FILTER_HISTORY_CHANGED] =
    g_signal_new ("filter-history-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass,
                                   filter_history_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_signals[IMAGE_OPENED] =
    g_signal_new ("image-opened",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass, image_opened),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_FILE);

  object_class->constructed      = gimp_constructed;
  object_class->set_property     = gimp_set_property;
  object_class->get_property     = gimp_get_property;
  object_class->dispose          = gimp_dispose;
  object_class->finalize         = gimp_finalize;

  gimp_object_class->get_memsize = gimp_get_memsize;

  klass->initialize              = gimp_real_initialize;
  klass->restore                 = gimp_real_restore;
  klass->exit                    = gimp_real_exit;
  klass->clipboard_changed       = NULL;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose", NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_init (Gimp *gimp)
{
  gimp->be_verbose       = FALSE;
  gimp->no_data          = FALSE;
  gimp->no_interface     = FALSE;
  gimp->show_gui         = TRUE;
  gimp->use_shm          = FALSE;
  gimp->use_cpu_accel    = TRUE;
  gimp->message_handler  = GIMP_CONSOLE;
  gimp->show_playground  = FALSE;
  gimp->stack_trace_mode = GIMP_STACK_TRACE_NEVER;
  gimp->pdb_compat_mode  = GIMP_PDB_COMPAT_OFF;

  gimp_gui_init (gimp);

  gimp->parasites = gimp_parasite_list_new ();

  gimp_enums_init ();

  gimp_units_init (gimp);

  gimp->images = gimp_list_new_weak (GIMP_TYPE_IMAGE, FALSE);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->images), "images");

  gimp->next_guide_id        = 1;
  gimp->next_sample_point_id = 1;
  gimp->image_table          = gimp_id_table_new ();
  gimp->item_table           = gimp_id_table_new ();

  gimp->displays = g_object_new (GIMP_TYPE_LIST,
                                 "children-type", GIMP_TYPE_OBJECT,
                                 "policy",        GIMP_CONTAINER_POLICY_WEAK,
                                 "append",        TRUE,
                                 NULL);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->displays), "displays");
  gimp->next_display_id = 1;

  gimp->named_buffers = gimp_list_new (GIMP_TYPE_BUFFER, TRUE);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->named_buffers),
                               "named buffers");

  gimp_data_factories_init (gimp);

  gimp->tool_info_list = g_object_new (GIMP_TYPE_LIST,
                                       "children-type", GIMP_TYPE_TOOL_INFO,
                                       "append",        TRUE,
                                       NULL);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->tool_info_list),
                               "tool infos");

  gimp->tool_item_list = g_object_new (GIMP_TYPE_LIST,
                                       "children-type", GIMP_TYPE_TOOL_ITEM,
                                       "append",        TRUE,
                                       NULL);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->tool_item_list),
                               "tool items");

  gimp->tool_item_ui_list = gimp_tree_proxy_new_for_container (
    gimp->tool_item_list);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->tool_item_ui_list),
                               "ui tool items");

  gimp->documents = gimp_document_list_new (gimp);

  gimp->templates = gimp_list_new (GIMP_TYPE_TEMPLATE, TRUE);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->templates), "templates");
}

static void
gimp_constructed (GObject *object)
{
  Gimp *gimp = GIMP (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_modules_init (gimp);

  gimp_paint_init (gimp);

  gimp->extension_manager = gimp_extension_manager_new (gimp);
  gimp->plug_in_manager   = gimp_plug_in_manager_new (gimp);
  gimp->pdb               = gimp_pdb_new (gimp);

  xcf_init (gimp);
  file_data_init (gimp);

  /*  create user and default context  */
  gimp_contexts_init (gimp);

  /* Initialize the extension manager early as its contents may be used
   * at the very start (e.g. the splash image).
   */
  gimp_extension_manager_initialize (gimp->extension_manager);
}

static void
gimp_set_property (GObject           *object,
                   guint              property_id,
                   const GValue      *value,
                   GParamSpec        *pspec)
{
  Gimp *gimp = GIMP (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      gimp->be_verbose = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_get_property (GObject    *object,
                   guint       property_id,
                   GValue     *value,
                   GParamSpec *pspec)
{
  Gimp *gimp = GIMP (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      g_value_set_boolean (value, gimp->be_verbose);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_dispose (GObject *object)
{
  Gimp *gimp = GIMP (object);

  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  gimp_data_factories_clear (gimp);

  gimp_filter_history_clear (gimp);

  g_clear_object (&gimp->edit_config);
  g_clear_object (&gimp->config);

  gimp_contexts_exit (gimp);

  g_clear_object (&gimp->image_new_last_template);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_finalize (GObject *object)
{
  Gimp  *gimp      = GIMP (object);
  GList *standards = NULL;

  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  standards = g_list_prepend (standards,
                              gimp_brush_get_standard (gimp->user_context));
  standards = g_list_prepend (standards,
                              gimp_dynamics_get_standard (gimp->user_context));
  standards = g_list_prepend (standards,
                              gimp_mybrush_get_standard (gimp->user_context));
  standards = g_list_prepend (standards,
                              gimp_pattern_get_standard (gimp->user_context));
  standards = g_list_prepend (standards,
                              gimp_gradient_get_standard (gimp->user_context));
  standards = g_list_prepend (standards,
                              gimp_palette_get_standard (gimp->user_context));

  g_clear_object (&gimp->templates);
  g_clear_object (&gimp->documents);

  gimp_tool_info_set_standard (gimp, NULL);

  g_clear_object (&gimp->tool_item_list);
  g_clear_object (&gimp->tool_item_ui_list);

  if (gimp->tool_info_list)
    {
      gimp_container_foreach (gimp->tool_info_list,
                              (GFunc) g_object_run_dispose, NULL);
      g_clear_object (&gimp->tool_info_list);
    }

  file_data_exit (gimp);
  xcf_exit (gimp);

  g_clear_object (&gimp->pdb);

  gimp_data_factories_exit (gimp);

  g_clear_object (&gimp->named_buffers);
  g_clear_object (&gimp->clipboard_buffer);
  g_clear_object (&gimp->clipboard_image);
  g_clear_object (&gimp->displays);
  g_clear_object (&gimp->item_table);
  g_clear_object (&gimp->image_table);
  g_clear_object (&gimp->images);
  g_clear_object (&gimp->plug_in_manager);
  g_clear_object (&gimp->extension_manager);

  if (gimp->module_db)
    gimp_modules_exit (gimp);

  gimp_paint_exit (gimp);

  g_clear_object (&gimp->parasites);
  g_clear_object (&gimp->default_folder);

  g_clear_pointer (&gimp->session_name, g_free);

  if (gimp->context_list)
    {
      GList *list;

      g_warning ("%s: list of contexts not empty upon exit (%d contexts left)\n",
                 G_STRFUNC, g_list_length (gimp->context_list));

      for (list = gimp->context_list; list; list = g_list_next (list))
        g_printerr ("stale context: %s\n", gimp_object_get_name (list->data));

      g_list_free (gimp->context_list);
      gimp->context_list = NULL;
    }

  g_list_foreach (standards, (GFunc) g_object_unref, NULL);
  g_list_free (standards);

  gimp_units_exit (gimp);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_get_memsize (GimpObject *object,
                  gint64     *gui_size)
{
  Gimp   *gimp    = GIMP (object);
  gint64  memsize = 0;

  memsize += gimp_g_list_get_memsize (gimp->user_units, 0 /* FIXME */);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->parasites),
                                      gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->paint_info_list),
                                      gui_size);

  memsize += gimp_g_object_get_memsize (G_OBJECT (gimp->module_db));
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->plug_in_manager),
                                      gui_size);

  memsize += gimp_g_list_get_memsize_foreach (gimp->filter_history,
                                              (GimpMemsizeFunc)
                                              gimp_object_get_memsize,
                                              gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->image_table), 0);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->item_table),  0);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->displays), gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->clipboard_image),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->clipboard_buffer),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->named_buffers),
                                      gui_size);

  memsize += gimp_data_factories_get_memsize (gimp, gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->pdb), gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->tool_info_list),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->standard_tool_info),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->documents),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->templates),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->image_new_last_template),
                                      gui_size);

  memsize += gimp_g_list_get_memsize (gimp->context_list, 0);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->default_context),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->user_context),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_real_initialize (Gimp               *gimp,
                      GimpInitStatusFunc  status_callback)
{
  if (gimp->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  status_callback (_("Initialization"), NULL, 0.0);

  /*  set the last values used to default values  */
  gimp->image_new_last_template =
    gimp_config_duplicate (GIMP_CONFIG (gimp->config->default_image));

  /*  add data objects that need the user context  */
  gimp_data_factories_add_builtin (gimp);

  /*  register all internal procedures  */
  status_callback (NULL, _("Internal Procedures"), 0.2);
  internal_procs_init (gimp->pdb);
  gimp_pdb_compat_procs_register (gimp->pdb, gimp->pdb_compat_mode);

  gimp_plug_in_manager_initialize (gimp->plug_in_manager, status_callback);

  status_callback (NULL, "", 1.0);
}

static void
gimp_real_restore (Gimp               *gimp,
                   GimpInitStatusFunc  status_callback)
{
  if (gimp->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  gimp_plug_in_manager_restore (gimp->plug_in_manager,
                                gimp_get_user_context (gimp), status_callback);

  /*  initialize babl fishes  */
  status_callback (_("Initialization"), "Babl Fishes", 0.0);
  gimp_babl_init_fishes (status_callback);

  gimp->restored = TRUE;
}

static gboolean
gimp_real_exit (Gimp     *gimp,
                gboolean  force)
{
  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  gimp_plug_in_manager_exit (gimp->plug_in_manager);
  gimp_extension_manager_exit (gimp->extension_manager);
  gimp_modules_unload (gimp);

  gimp_data_factories_save (gimp);

  gimp_templates_save (gimp);
  gimp_parasiterc_save (gimp);
  gimp_unitrc_save (gimp);

  return FALSE; /* continue exiting */
}

Gimp *
gimp_new (const gchar       *name,
          const gchar       *session_name,
          GFile             *default_folder,
          gboolean           be_verbose,
          gboolean           no_data,
          gboolean           no_fonts,
          gboolean           no_interface,
          gboolean           use_shm,
          gboolean           use_cpu_accel,
          gboolean           console_messages,
          gboolean           show_playground,
          gboolean           show_debug_menu,
          GimpStackTraceMode stack_trace_mode,
          GimpPDBCompatMode  pdb_compat_mode)
{
  Gimp *gimp;

  g_return_val_if_fail (name != NULL, NULL);

  gimp = g_object_new (GIMP_TYPE_GIMP,
                       "name",    name,
                       "verbose", be_verbose ? TRUE : FALSE,
                       NULL);

  if (default_folder)
    gimp->default_folder = g_object_ref (default_folder);

  gimp->session_name     = g_strdup (session_name);
  gimp->no_data          = no_data          ? TRUE : FALSE;
  gimp->no_fonts         = no_fonts         ? TRUE : FALSE;
  gimp->no_interface     = no_interface     ? TRUE : FALSE;
  gimp->use_shm          = use_shm          ? TRUE : FALSE;
  gimp->use_cpu_accel    = use_cpu_accel    ? TRUE : FALSE;
  gimp->console_messages = console_messages ? TRUE : FALSE;
  gimp->show_playground  = show_playground  ? TRUE : FALSE;
  gimp->show_debug_menu  = show_debug_menu  ? TRUE : FALSE;
  gimp->stack_trace_mode = stack_trace_mode;
  gimp->pdb_compat_mode  = pdb_compat_mode;

  return gimp;
}

/**
 * gimp_set_show_gui:
 * @gimp:
 * @show:
 *
 * Test cases that tests the UI typically don't want any windows to be
 * presented during the test run. Allow them to set this.
 **/
void
gimp_set_show_gui (Gimp     *gimp,
                   gboolean  show_gui)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->show_gui = show_gui;
}

/**
 * gimp_get_show_gui:
 * @gimp:
 *
 * Returns: %TRUE if the GUI should be shown, %FALSE otherwise.
 **/
gboolean
gimp_get_show_gui (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  return gimp->show_gui;
}

static void
gimp_global_config_notify (GObject    *global_config,
                           GParamSpec *param_spec,
                           GObject    *edit_config)
{
  GValue global_value = G_VALUE_INIT;
  GValue edit_value   = G_VALUE_INIT;

  g_value_init (&global_value, param_spec->value_type);
  g_value_init (&edit_value,   param_spec->value_type);

  g_object_get_property (global_config, param_spec->name, &global_value);
  g_object_get_property (edit_config,   param_spec->name, &edit_value);

  if (g_param_values_cmp (param_spec, &global_value, &edit_value))
    {
      g_signal_handlers_block_by_func (edit_config,
                                       gimp_edit_config_notify,
                                       global_config);

      g_object_set_property (edit_config, param_spec->name, &global_value);

      g_signal_handlers_unblock_by_func (edit_config,
                                         gimp_edit_config_notify,
                                         global_config);
    }

  g_value_unset (&global_value);
  g_value_unset (&edit_value);
}

static void
gimp_edit_config_notify (GObject    *edit_config,
                         GParamSpec *param_spec,
                         GObject    *global_config)
{
  GValue edit_value   = G_VALUE_INIT;
  GValue global_value = G_VALUE_INIT;

  g_value_init (&edit_value,   param_spec->value_type);
  g_value_init (&global_value, param_spec->value_type);

  g_object_get_property (edit_config,   param_spec->name, &edit_value);
  g_object_get_property (global_config, param_spec->name, &global_value);

  if (g_param_values_cmp (param_spec, &edit_value, &global_value))
    {
      if (param_spec->flags & GIMP_CONFIG_PARAM_RESTART)
        {
#ifdef GIMP_CONFIG_DEBUG
          g_print ("NOT Applying edit_config change of '%s' to global_config "
                   "because it needs restart\n",
                   param_spec->name);
#endif
        }
      else
        {
#ifdef GIMP_CONFIG_DEBUG
          g_print ("Applying edit_config change of '%s' to global_config\n",
                   param_spec->name);
#endif
          g_signal_handlers_block_by_func (global_config,
                                           gimp_global_config_notify,
                                           edit_config);

          g_object_set_property (global_config, param_spec->name, &edit_value);

          g_signal_handlers_unblock_by_func (global_config,
                                             gimp_global_config_notify,
                                             edit_config);
        }
    }

  g_value_unset (&edit_value);
  g_value_unset (&global_value);
}

void
gimp_load_config (Gimp  *gimp,
                  GFile *alternate_system_gimprc,
                  GFile *alternate_gimprc)
{
  GimpRc *gimprc;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (alternate_system_gimprc == NULL ||
                    G_IS_FILE (alternate_system_gimprc));
  g_return_if_fail (alternate_gimprc == NULL ||
                    G_IS_FILE (alternate_gimprc));
  g_return_if_fail (gimp->config == NULL);
  g_return_if_fail (gimp->edit_config == NULL);

  if (gimp->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  /*  this needs to be done before gimprc loading because gimprc can
   *  use user defined units
   */
  gimp_unitrc_load (gimp);

  gimprc = gimp_rc_new (G_OBJECT (gimp),
                        alternate_system_gimprc,
                        alternate_gimprc,
                        gimp->be_verbose);

  gimp->config = GIMP_CORE_CONFIG (gimprc);

  gimp->edit_config = gimp_config_duplicate (GIMP_CONFIG (gimp->config));

  g_signal_connect_object (gimp->config, "notify",
                           G_CALLBACK (gimp_global_config_notify),
                           gimp->edit_config, 0);
  g_signal_connect_object (gimp->edit_config, "notify",
                           G_CALLBACK (gimp_edit_config_notify),
                           gimp->config, 0);

  if (! gimp->show_playground)
    {
      gboolean    use_opencl;
      gboolean    use_npd_tool;
      gboolean    use_seamless_clone_tool;

      /* Playground preferences is shown by default for unstable
       * versions and if the associated CLI option was set. Additionally
       * we want to show it if any of the playground options had been
       * enabled. Otherwise you might end up getting blocked with a
       * playground feature and forget where you can even disable it.
       *
       * Also we check this once at start when loading config, and not
       * inside preferences-dialog.c because we don't want to end up
       * with inconsistent behavior where you open once the Preferences,
       * deactivate features, then back to preferences and the tab is
       * gone.
       */

      g_object_get (gimp->edit_config,
                    "use-opencl",                     &use_opencl,
                    "playground-npd-tool",            &use_npd_tool,
                    "playground-seamless-clone-tool", &use_seamless_clone_tool,
                    NULL);
      if (use_opencl || use_npd_tool || use_seamless_clone_tool)
        gimp->show_playground = TRUE;
    }
}

void
gimp_initialize (Gimp               *gimp,
                 GimpInitStatusFunc  status_callback)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (status_callback != NULL);
  g_return_if_fail (GIMP_IS_CORE_CONFIG (gimp->config));

  if (gimp->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  g_signal_emit (gimp, gimp_signals[INITIALIZE], 0, status_callback);
}

/**
 * gimp_restore:
 * @gimp: a #Gimp object
 * @error: a #GError for uncessful loading.
 *
 * This function always succeeds. If present, @error may be filled for
 * possible feedback on data which failed to load. It doesn't imply any
 * fatale error.
 **/
void
gimp_restore (Gimp                *gimp,
              GimpInitStatusFunc   status_callback,
              GError             **error)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (status_callback != NULL);

  if (gimp->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  /*  initialize  the global parasite table  */
  status_callback (_("Looking for data files"), _("Parasites"), 0.0);
  gimp_parasiterc_load (gimp);

  /*  initialize the lists of gimp brushes, dynamics, patterns etc.  */
  gimp_data_factories_load (gimp, status_callback);

  /*  initialize the template list  */
  status_callback (NULL, _("Templates"), 0.8);
  gimp_templates_load (gimp);

  /*  initialize the module list  */
  status_callback (NULL, _("Modules"), 0.9);
  gimp_modules_load (gimp);

  g_signal_emit (gimp, gimp_signals[RESTORE], 0, status_callback);

  /* when done, make sure everything is clean, to clean out dirty
   * states from data objects which reference each other and got
   * dirtied by loading the referenced object
   */
  gimp_data_factories_data_clean (gimp);
}

/**
 * gimp_is_restored:
 * @gimp: a #Gimp object
 *
 * Returns: %TRUE if GIMP is completely started, %FALSE otherwise.
 **/
gboolean
gimp_is_restored (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  return gimp->initialized && gimp->restored;
}

/**
 * gimp_exit:
 * @gimp: a #Gimp object
 * @force: whether to force the application to quit
 *
 * Exit this GIMP session. Unless @force is %TRUE, the user is queried
 * whether unsaved images should be saved and can cancel the operation.
 **/
void
gimp_exit (Gimp     *gimp,
           gboolean  force)
{
  gboolean handled;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  g_signal_emit (gimp, gimp_signals[EXIT], 0,
                 force ? TRUE : FALSE,
                 &handled);

  if (handled)
    return;

  g_idle_add_full (G_PRIORITY_LOW,
                   (GSourceFunc) gimp_exit_idle_cleanup_stray_images,
                   gimp, NULL);
}

GList *
gimp_get_image_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->images)->queue->head;
}

GList *
gimp_get_display_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->displays)->queue->head;
}

GList *
gimp_get_image_windows (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_list_copy (gimp->image_windows);
}

GList *
gimp_get_paint_info_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->paint_info_list)->queue->head;
}

GList *
gimp_get_tool_info_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->tool_info_list)->queue->head;
}

GList *
gimp_get_tool_item_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->tool_item_list)->queue->head;
}

GList *
gimp_get_tool_item_ui_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->tool_item_ui_list)->queue->head;
}

GimpObject *
gimp_get_clipboard_object (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->clipboard_image)
    return GIMP_OBJECT (gimp->clipboard_image);

  return GIMP_OBJECT (gimp->clipboard_buffer);
}

void
gimp_set_clipboard_image (Gimp      *gimp,
                          GimpImage *image)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  g_clear_object (&gimp->clipboard_buffer);
  g_set_object (&gimp->clipboard_image, image);

  /* we want the signal emission */
  g_signal_emit (gimp, gimp_signals[CLIPBOARD_CHANGED], 0);
}

GimpImage *
gimp_get_clipboard_image (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->clipboard_image;
}

void
gimp_set_clipboard_buffer (Gimp       *gimp,
                           GimpBuffer *buffer)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (buffer == NULL || GIMP_IS_BUFFER (buffer));

  g_clear_object (&gimp->clipboard_image);
  g_set_object (&gimp->clipboard_buffer, buffer);

  /* we want the signal emission */
  g_signal_emit (gimp, gimp_signals[CLIPBOARD_CHANGED], 0);
}

GimpBuffer *
gimp_get_clipboard_buffer (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->clipboard_buffer;
}

GimpImage *
gimp_create_image (Gimp              *gimp,
                   gint               width,
                   gint               height,
                   GimpImageBaseType  type,
                   GimpPrecision      precision,
                   gboolean           attach_comment)
{
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  image = gimp_image_new (gimp, width, height, type, precision);

  if (attach_comment)
    {
      const gchar *comment;

      comment = gimp_template_get_comment (gimp->config->default_image);

      if (comment)
        {
          GimpParasite *parasite = gimp_parasite_new ("gimp-comment",
                                                      GIMP_PARASITE_PERSISTENT,
                                                      strlen (comment) + 1,
                                                      comment);
          gimp_image_parasite_attach (image, parasite, FALSE);
          gimp_parasite_free (parasite);
        }
    }

  return image;
}

void
gimp_set_default_context (Gimp        *gimp,
                          GimpContext *context)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  g_set_object (&gimp->default_context, context);
}

GimpContext *
gimp_get_default_context (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->default_context;
}

void
gimp_set_user_context (Gimp        *gimp,
                       GimpContext *context)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (context == NULL || GIMP_IS_CONTEXT (context));

  g_set_object (&gimp->user_context, context);
}

GimpContext *
gimp_get_user_context (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->user_context;
}

GimpToolInfo *
gimp_get_tool_info (Gimp        *gimp,
                    const gchar *tool_id)
{
  gpointer info;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (tool_id != NULL, NULL);

  info = gimp_container_get_child_by_name (gimp->tool_info_list, tool_id);

  return (GimpToolInfo *) info;
}

/**
 * gimp_message:
 * @gimp:     a pointer to the %Gimp object
 * @handler:  either a %GimpProgress or a %GtkWidget pointer
 * @severity: severity of the message
 * @format:   printf-like format string
 * @...:      arguments to use with @format
 *
 * Present a message to the user. How exactly the message is displayed
 * depends on the @severity, the @handler object and user preferences.
 **/
void
gimp_message (Gimp                *gimp,
              GObject             *handler,
              GimpMessageSeverity  severity,
              const gchar         *format,
              ...)
{
  va_list args;

  va_start (args, format);

  gimp_message_valist (gimp, handler, severity, format, args);

  va_end (args);
}

/**
 * gimp_message_valist:
 * @gimp:     a pointer to the %Gimp object
 * @handler:  either a %GimpProgress or a %GtkWidget pointer
 * @severity: severity of the message
 * @format:   printf-like format string
 * @args:     arguments to use with @format
 *
 * See documentation for gimp_message().
 **/
void
gimp_message_valist (Gimp                *gimp,
                     GObject             *handler,
                     GimpMessageSeverity  severity,
                     const gchar         *format,
                     va_list              args)
{
  gchar *message;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (handler == NULL || G_IS_OBJECT (handler));
  g_return_if_fail (format != NULL);

  message = g_strdup_vprintf (format, args);

  gimp_show_message (gimp, handler, severity, NULL, message);

  g_free (message);
}

void
gimp_message_literal (Gimp                *gimp,
                      GObject             *handler,
                      GimpMessageSeverity  severity,
                      const gchar         *message)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (handler == NULL || G_IS_OBJECT (handler));
  g_return_if_fail (message != NULL);

  gimp_show_message (gimp, handler, severity, NULL, message);
}

void
gimp_filter_history_changed (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  g_signal_emit (gimp, gimp_signals[FILTER_HISTORY_CHANGED], 0);
}

void
gimp_image_opened (Gimp  *gimp,
                   GFile *file)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (gimp, gimp_signals[IMAGE_OPENED], 0, file);
}

GFile *
gimp_get_temp_file (Gimp        *gimp,
                    const gchar *extension)
{
  static gint  id = 0;
  static gint  pid;
  gchar       *basename;
  GFile       *dir;
  GFile       *file;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (id == 0)
    pid = gimp_get_pid ();

  if (extension)
    basename = g_strdup_printf ("gimp-temp-%d%d.%s", pid, id++, extension);
  else
    basename = g_strdup_printf ("gimp-temp-%d%d", pid, id++);

  dir = gimp_file_new_for_config_path (GIMP_GEGL_CONFIG (gimp->config)->temp_path,
                                       NULL);
  if (! g_file_query_exists (dir, NULL))
    {
      /* Try to make the temp directory if it doesn't exist.
       * Ignore any error.
       */
      g_file_make_directory_with_parents (dir, NULL, NULL);
    }
  file = g_file_get_child (dir, basename);
  g_free (basename);
  g_object_unref (dir);

  return file;
}

static gboolean
gimp_exit_idle_cleanup_stray_images (Gimp *gimp)
{
  GList *image_iter;

  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, TRUE);

  /* Get rid of images without display. We do this *after* handling the
   * usual exit callbacks and any other pending event, because the
   * things that are torn down there might have references to these
   * images, for instance GimpActions in the UI manager, or some plug-in
   * which was still running and had to get killed in gimp_exit() (cf. #11922).
   */
  while ((image_iter = gimp_get_image_iter (gimp)))
    {
      GimpImage *image = image_iter->data;

      g_object_unref (image);
    }

  return G_SOURCE_REMOVE ;
}
