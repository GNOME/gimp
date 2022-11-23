/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmabase/ligmabase-private.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmarc.h"

#include "gegl/ligma-babl.h"

#include "pdb/ligmapdb.h"
#include "pdb/ligma-pdb-compat.h"
#include "pdb/internal-procs.h"

#include "plug-in/ligmapluginmanager.h"
#include "plug-in/ligmapluginmanager-restore.h"

#include "paint/ligma-paint.h"

#include "xcf/xcf.h"
#include "file-data/file-data.h"

#include "ligma.h"
#include "ligma-contexts.h"
#include "ligma-data-factories.h"
#include "ligma-filter-history.h"
#include "ligma-memsize.h"
#include "ligma-modules.h"
#include "ligma-parasites.h"
#include "ligma-templates.h"
#include "ligma-units.h"
#include "ligma-utils.h"
#include "ligmabrush.h"
#include "ligmabuffer.h"
#include "ligmacontext.h"
#include "ligmadynamics.h"
#include "ligmadocumentlist.h"
#include "ligmaextensionmanager.h"
#include "ligmagradient.h"
#include "ligmaidtable.h"
#include "ligmaimage.h"
#include "ligmaimagefile.h"
#include "ligmalist.h"
#include "ligmamarshal.h"
#include "ligmamybrush.h"
#include "ligmapalette.h"
#include "ligmaparasitelist.h"
#include "ligmapattern.h"
#include "ligmatemplate.h"
#include "ligmatoolinfo.h"
#include "ligmatreeproxy.h"

#include "ligma-intl.h"


/*  we need to register all enum types so they are known to the type
 *  system by name, re-use the files the pdb generated for libligma
 */
void           ligma_enums_init           (void);
const gchar ** ligma_enums_get_type_names (gint *n_type_names);
#include "libligma/ligmaenums.c.tail"


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


static void      ligma_constructed          (GObject           *object);
static void      ligma_set_property         (GObject           *object,
                                            guint              property_id,
                                            const GValue      *value,
                                            GParamSpec        *pspec);
static void      ligma_get_property         (GObject           *object,
                                            guint              property_id,
                                            GValue            *value,
                                            GParamSpec        *pspec);
static void      ligma_dispose              (GObject           *object);
static void      ligma_finalize             (GObject           *object);

static gint64    ligma_get_memsize          (LigmaObject        *object,
                                            gint64            *gui_size);

static void      ligma_real_initialize      (Ligma              *ligma,
                                            LigmaInitStatusFunc status_callback);
static void      ligma_real_restore         (Ligma              *ligma,
                                            LigmaInitStatusFunc status_callback);
static gboolean  ligma_real_exit            (Ligma              *ligma,
                                            gboolean           force);

static void      ligma_global_config_notify (GObject           *global_config,
                                            GParamSpec        *param_spec,
                                            GObject           *edit_config);
static void      ligma_edit_config_notify   (GObject           *edit_config,
                                            GParamSpec        *param_spec,
                                            GObject           *global_config);


G_DEFINE_TYPE (Ligma, ligma, LIGMA_TYPE_OBJECT)

#define parent_class ligma_parent_class

static guint ligma_signals[LAST_SIGNAL] = { 0, };


static void
ligma_class_init (LigmaClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);

  ligma_signals[INITIALIZE] =
    g_signal_new ("initialize",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaClass, initialize),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  ligma_signals[RESTORE] =
    g_signal_new ("restore",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaClass, restore),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  ligma_signals[EXIT] =
    g_signal_new ("exit",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaClass, exit),
                  g_signal_accumulator_true_handled, NULL,
                  ligma_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_BOOLEAN);

  ligma_signals[CLIPBOARD_CHANGED] =
    g_signal_new ("clipboard-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaClass, clipboard_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_signals[FILTER_HISTORY_CHANGED] =
    g_signal_new ("filter-history-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaClass,
                                   filter_history_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_signals[IMAGE_OPENED] =
    g_signal_new ("image-opened",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaClass, image_opened),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_FILE);

  object_class->constructed      = ligma_constructed;
  object_class->set_property     = ligma_set_property;
  object_class->get_property     = ligma_get_property;
  object_class->dispose          = ligma_dispose;
  object_class->finalize         = ligma_finalize;

  ligma_object_class->get_memsize = ligma_get_memsize;

  klass->initialize              = ligma_real_initialize;
  klass->restore                 = ligma_real_restore;
  klass->exit                    = ligma_real_exit;
  klass->clipboard_changed       = NULL;

  g_object_class_install_property (object_class, PROP_VERBOSE,
                                   g_param_spec_boolean ("verbose", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_init (Ligma *ligma)
{
  ligma->be_verbose       = FALSE;
  ligma->no_data          = FALSE;
  ligma->no_interface     = FALSE;
  ligma->show_gui         = TRUE;
  ligma->use_shm          = FALSE;
  ligma->use_cpu_accel    = TRUE;
  ligma->message_handler  = LIGMA_CONSOLE;
  ligma->show_playground  = FALSE;
  ligma->stack_trace_mode = LIGMA_STACK_TRACE_NEVER;
  ligma->pdb_compat_mode  = LIGMA_PDB_COMPAT_OFF;

  ligma_gui_init (ligma);

  ligma->parasites = ligma_parasite_list_new ();

  ligma_enums_init ();

  ligma_units_init (ligma);

  ligma->images = ligma_list_new_weak (LIGMA_TYPE_IMAGE, FALSE);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->images), "images");

  ligma->next_guide_id        = 1;
  ligma->next_sample_point_id = 1;
  ligma->image_table          = ligma_id_table_new ();
  ligma->item_table           = ligma_id_table_new ();

  ligma->displays = g_object_new (LIGMA_TYPE_LIST,
                                 "children-type", LIGMA_TYPE_OBJECT,
                                 "policy",        LIGMA_CONTAINER_POLICY_WEAK,
                                 "append",        TRUE,
                                 NULL);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->displays), "displays");
  ligma->next_display_id = 1;

  ligma->named_buffers = ligma_list_new (LIGMA_TYPE_BUFFER, TRUE);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->named_buffers),
                               "named buffers");

  ligma_data_factories_init (ligma);

  ligma->tool_info_list = g_object_new (LIGMA_TYPE_LIST,
                                       "children-type", LIGMA_TYPE_TOOL_INFO,
                                       "append",        TRUE,
                                       NULL);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->tool_info_list),
                               "tool infos");

  ligma->tool_item_list = g_object_new (LIGMA_TYPE_LIST,
                                       "children-type", LIGMA_TYPE_TOOL_ITEM,
                                       "append",        TRUE,
                                       NULL);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->tool_item_list),
                               "tool items");

  ligma->tool_item_ui_list = ligma_tree_proxy_new_for_container (
    ligma->tool_item_list);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->tool_item_ui_list),
                               "ui tool items");

  ligma->documents = ligma_document_list_new (ligma);

  ligma->templates = ligma_list_new (LIGMA_TYPE_TEMPLATE, TRUE);
  ligma_object_set_static_name (LIGMA_OBJECT (ligma->templates), "templates");
}

static void
ligma_constructed (GObject *object)
{
  Ligma *ligma = LIGMA (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_modules_init (ligma);

  ligma_paint_init (ligma);

  ligma->extension_manager = ligma_extension_manager_new (ligma);
  ligma->plug_in_manager   = ligma_plug_in_manager_new (ligma);
  ligma->pdb               = ligma_pdb_new (ligma);

  xcf_init (ligma);
  file_data_init (ligma);

  /*  create user and default context  */
  ligma_contexts_init (ligma);

  /* Initialize the extension manager early as its contents may be used
   * at the very start (e.g. the splash image).
   */
  ligma_extension_manager_initialize (ligma->extension_manager);
}

static void
ligma_set_property (GObject           *object,
                   guint              property_id,
                   const GValue      *value,
                   GParamSpec        *pspec)
{
  Ligma *ligma = LIGMA (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      ligma->be_verbose = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_get_property (GObject    *object,
                   guint       property_id,
                   GValue     *value,
                   GParamSpec *pspec)
{
  Ligma *ligma = LIGMA (object);

  switch (property_id)
    {
    case PROP_VERBOSE:
      g_value_set_boolean (value, ligma->be_verbose);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_dispose (GObject *object)
{
  Ligma *ligma = LIGMA (object);

  if (ligma->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  ligma_data_factories_clear (ligma);

  ligma_filter_history_clear (ligma);

  g_clear_object (&ligma->edit_config);
  g_clear_object (&ligma->config);

  ligma_contexts_exit (ligma);

  g_clear_object (&ligma->image_new_last_template);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_finalize (GObject *object)
{
  Ligma  *ligma      = LIGMA (object);
  GList *standards = NULL;

  if (ligma->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  standards = g_list_prepend (standards,
                              ligma_brush_get_standard (ligma->user_context));
  standards = g_list_prepend (standards,
                              ligma_dynamics_get_standard (ligma->user_context));
  standards = g_list_prepend (standards,
                              ligma_mybrush_get_standard (ligma->user_context));
  standards = g_list_prepend (standards,
                              ligma_pattern_get_standard (ligma->user_context));
  standards = g_list_prepend (standards,
                              ligma_gradient_get_standard (ligma->user_context));
  standards = g_list_prepend (standards,
                              ligma_palette_get_standard (ligma->user_context));

  g_clear_object (&ligma->templates);
  g_clear_object (&ligma->documents);

  ligma_tool_info_set_standard (ligma, NULL);

  g_clear_object (&ligma->tool_item_list);
  g_clear_object (&ligma->tool_item_ui_list);

  if (ligma->tool_info_list)
    {
      ligma_container_foreach (ligma->tool_info_list,
                              (GFunc) g_object_run_dispose, NULL);
      g_clear_object (&ligma->tool_info_list);
    }

  file_data_exit (ligma);
  xcf_exit (ligma);

  g_clear_object (&ligma->pdb);

  ligma_data_factories_exit (ligma);

  g_clear_object (&ligma->named_buffers);
  g_clear_object (&ligma->clipboard_buffer);
  g_clear_object (&ligma->clipboard_image);
  g_clear_object (&ligma->displays);
  g_clear_object (&ligma->item_table);
  g_clear_object (&ligma->image_table);
  g_clear_object (&ligma->images);
  g_clear_object (&ligma->plug_in_manager);
  g_clear_object (&ligma->extension_manager);

  if (ligma->module_db)
    ligma_modules_exit (ligma);

  ligma_paint_exit (ligma);

  g_clear_object (&ligma->parasites);
  g_clear_object (&ligma->default_folder);

  g_clear_pointer (&ligma->session_name, g_free);

  if (ligma->context_list)
    {
      GList *list;

      g_warning ("%s: list of contexts not empty upon exit (%d contexts left)\n",
                 G_STRFUNC, g_list_length (ligma->context_list));

      for (list = ligma->context_list; list; list = g_list_next (list))
        g_printerr ("stale context: %s\n", ligma_object_get_name (list->data));

      g_list_free (ligma->context_list);
      ligma->context_list = NULL;
    }

  g_list_foreach (standards, (GFunc) g_object_unref, NULL);
  g_list_free (standards);

  ligma_units_exit (ligma);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_get_memsize (LigmaObject *object,
                  gint64     *gui_size)
{
  Ligma   *ligma    = LIGMA (object);
  gint64  memsize = 0;

  memsize += ligma_g_list_get_memsize (ligma->user_units, 0 /* FIXME */);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->parasites),
                                      gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->paint_info_list),
                                      gui_size);

  memsize += ligma_g_object_get_memsize (G_OBJECT (ligma->module_db));
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->plug_in_manager),
                                      gui_size);

  memsize += ligma_g_list_get_memsize_foreach (ligma->filter_history,
                                              (LigmaMemsizeFunc)
                                              ligma_object_get_memsize,
                                              gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->image_table), 0);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->item_table),  0);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->displays), gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->clipboard_image),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->clipboard_buffer),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->named_buffers),
                                      gui_size);

  memsize += ligma_data_factories_get_memsize (ligma, gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->pdb), gui_size);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->tool_info_list),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->standard_tool_info),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->documents),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->templates),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->image_new_last_template),
                                      gui_size);

  memsize += ligma_g_list_get_memsize (ligma->context_list, 0);

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->default_context),
                                      gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (ligma->user_context),
                                      gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_real_initialize (Ligma               *ligma,
                      LigmaInitStatusFunc  status_callback)
{
  if (ligma->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  status_callback (_("Initialization"), NULL, 0.0);

  /*  set the last values used to default values  */
  ligma->image_new_last_template =
    ligma_config_duplicate (LIGMA_CONFIG (ligma->config->default_image));

  /*  add data objects that need the user context  */
  ligma_data_factories_add_builtin (ligma);

  /*  register all internal procedures  */
  status_callback (NULL, _("Internal Procedures"), 0.2);
  internal_procs_init (ligma->pdb);
  ligma_pdb_compat_procs_register (ligma->pdb, ligma->pdb_compat_mode);

  ligma_plug_in_manager_initialize (ligma->plug_in_manager, status_callback);

  status_callback (NULL, "", 1.0);
}

static void
ligma_real_restore (Ligma               *ligma,
                   LigmaInitStatusFunc  status_callback)
{
  if (ligma->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  ligma_plug_in_manager_restore (ligma->plug_in_manager,
                                ligma_get_user_context (ligma), status_callback);

  /*  initialize babl fishes  */
  status_callback (_("Initialization"), "Babl Fishes", 0.0);
  ligma_babl_init_fishes (status_callback);

  ligma->restored = TRUE;
}

static gboolean
ligma_real_exit (Ligma     *ligma,
                gboolean  force)
{
  if (ligma->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  ligma_plug_in_manager_exit (ligma->plug_in_manager);
  ligma_extension_manager_exit (ligma->extension_manager);
  ligma_modules_unload (ligma);

  ligma_data_factories_save (ligma);

  ligma_templates_save (ligma);
  ligma_parasiterc_save (ligma);
  ligma_unitrc_save (ligma);

  return FALSE; /* continue exiting */
}

Ligma *
ligma_new (const gchar       *name,
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
          LigmaStackTraceMode stack_trace_mode,
          LigmaPDBCompatMode  pdb_compat_mode)
{
  Ligma *ligma;

  g_return_val_if_fail (name != NULL, NULL);

  ligma = g_object_new (LIGMA_TYPE_LIGMA,
                       "name",    name,
                       "verbose", be_verbose ? TRUE : FALSE,
                       NULL);

  if (default_folder)
    ligma->default_folder = g_object_ref (default_folder);

  ligma->session_name     = g_strdup (session_name);
  ligma->no_data          = no_data          ? TRUE : FALSE;
  ligma->no_fonts         = no_fonts         ? TRUE : FALSE;
  ligma->no_interface     = no_interface     ? TRUE : FALSE;
  ligma->use_shm          = use_shm          ? TRUE : FALSE;
  ligma->use_cpu_accel    = use_cpu_accel    ? TRUE : FALSE;
  ligma->console_messages = console_messages ? TRUE : FALSE;
  ligma->show_playground  = show_playground  ? TRUE : FALSE;
  ligma->show_debug_menu  = show_debug_menu  ? TRUE : FALSE;
  ligma->stack_trace_mode = stack_trace_mode;
  ligma->pdb_compat_mode  = pdb_compat_mode;

  return ligma;
}

/**
 * ligma_set_show_gui:
 * @ligma:
 * @show:
 *
 * Test cases that tests the UI typically don't want any windows to be
 * presented during the test run. Allow them to set this.
 **/
void
ligma_set_show_gui (Ligma     *ligma,
                   gboolean  show_gui)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma->show_gui = show_gui;
}

/**
 * ligma_get_show_gui:
 * @ligma:
 *
 * Returns: %TRUE if the GUI should be shown, %FALSE otherwise.
 **/
gboolean
ligma_get_show_gui (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  return ligma->show_gui;
}

static void
ligma_global_config_notify (GObject    *global_config,
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
                                       ligma_edit_config_notify,
                                       global_config);

      g_object_set_property (edit_config, param_spec->name, &global_value);

      g_signal_handlers_unblock_by_func (edit_config,
                                         ligma_edit_config_notify,
                                         global_config);
    }

  g_value_unset (&global_value);
  g_value_unset (&edit_value);
}

static void
ligma_edit_config_notify (GObject    *edit_config,
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
      if (param_spec->flags & LIGMA_CONFIG_PARAM_RESTART)
        {
#ifdef LIGMA_CONFIG_DEBUG
          g_print ("NOT Applying edit_config change of '%s' to global_config "
                   "because it needs restart\n",
                   param_spec->name);
#endif
        }
      else
        {
#ifdef LIGMA_CONFIG_DEBUG
          g_print ("Applying edit_config change of '%s' to global_config\n",
                   param_spec->name);
#endif
          g_signal_handlers_block_by_func (global_config,
                                           ligma_global_config_notify,
                                           edit_config);

          g_object_set_property (global_config, param_spec->name, &edit_value);

          g_signal_handlers_unblock_by_func (global_config,
                                             ligma_global_config_notify,
                                             edit_config);
        }
    }

  g_value_unset (&edit_value);
  g_value_unset (&global_value);
}

void
ligma_load_config (Ligma  *ligma,
                  GFile *alternate_system_ligmarc,
                  GFile *alternate_ligmarc)
{
  LigmaRc *ligmarc;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (alternate_system_ligmarc == NULL ||
                    G_IS_FILE (alternate_system_ligmarc));
  g_return_if_fail (alternate_ligmarc == NULL ||
                    G_IS_FILE (alternate_ligmarc));
  g_return_if_fail (ligma->config == NULL);
  g_return_if_fail (ligma->edit_config == NULL);

  if (ligma->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  /*  this needs to be done before ligmarc loading because ligmarc can
   *  use user defined units
   */
  ligma_unitrc_load (ligma);

  ligmarc = ligma_rc_new (G_OBJECT (ligma),
                        alternate_system_ligmarc,
                        alternate_ligmarc,
                        ligma->be_verbose);

  ligma->config = LIGMA_CORE_CONFIG (ligmarc);

  ligma->edit_config = ligma_config_duplicate (LIGMA_CONFIG (ligma->config));

  g_signal_connect_object (ligma->config, "notify",
                           G_CALLBACK (ligma_global_config_notify),
                           ligma->edit_config, 0);
  g_signal_connect_object (ligma->edit_config, "notify",
                           G_CALLBACK (ligma_edit_config_notify),
                           ligma->config, 0);

  if (! ligma->show_playground)
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

      g_object_get (ligma->edit_config,
                    "use-opencl",                     &use_opencl,
                    "playground-npd-tool",            &use_npd_tool,
                    "playground-seamless-clone-tool", &use_seamless_clone_tool,
                    NULL);
      if (use_opencl || use_npd_tool || use_seamless_clone_tool)
        ligma->show_playground = TRUE;
    }
}

void
ligma_initialize (Ligma               *ligma,
                 LigmaInitStatusFunc  status_callback)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (status_callback != NULL);
  g_return_if_fail (LIGMA_IS_CORE_CONFIG (ligma->config));

  if (ligma->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  g_signal_emit (ligma, ligma_signals[INITIALIZE], 0, status_callback);
}

/**
 * ligma_restore:
 * @ligma: a #Ligma object
 * @error: a #GError for uncessful loading.
 *
 * This function always succeeds. If present, @error may be filled for
 * possible feedback on data which failed to load. It doesn't imply any
 * fatale error.
 **/
void
ligma_restore (Ligma                *ligma,
              LigmaInitStatusFunc   status_callback,
              GError             **error)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (status_callback != NULL);

  if (ligma->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  /*  initialize  the global parasite table  */
  status_callback (_("Looking for data files"), _("Parasites"), 0.0);
  ligma_parasiterc_load (ligma);

  /*  initialize the lists of ligma brushes, dynamics, patterns etc.  */
  ligma_data_factories_load (ligma, status_callback);

  /*  initialize the template list  */
  status_callback (NULL, _("Templates"), 0.8);
  ligma_templates_load (ligma);

  /*  initialize the module list  */
  status_callback (NULL, _("Modules"), 0.9);
  ligma_modules_load (ligma);

  g_signal_emit (ligma, ligma_signals[RESTORE], 0, status_callback);

  /* when done, make sure everything is clean, to clean out dirty
   * states from data objects which reference each other and got
   * dirtied by loading the referenced object
   */
  ligma_data_factories_data_clean (ligma);
}

/**
 * ligma_is_restored:
 * @ligma: a #Ligma object
 *
 * Returns: %TRUE if LIGMA is completely started, %FALSE otherwise.
 **/
gboolean
ligma_is_restored (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  return ligma->initialized && ligma->restored;
}

/**
 * ligma_exit:
 * @ligma: a #Ligma object
 * @force: whether to force the application to quit
 *
 * Exit this LIGMA session. Unless @force is %TRUE, the user is queried
 * whether unsaved images should be saved and can cancel the operation.
 **/
void
ligma_exit (Ligma     *ligma,
           gboolean  force)
{
  gboolean  handled;
  GList    *image_iter;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (ligma->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  g_signal_emit (ligma, ligma_signals[EXIT], 0,
                 force ? TRUE : FALSE,
                 &handled);

  if (handled)
    return;

  /* Get rid of images without display. We do this *after* handling the
   * usual exit callbacks, because the things that are torn down there
   * might have references to these images (for instance LigmaActions
   * in the UI manager).
   */
  while ((image_iter = ligma_get_image_iter (ligma)))
    {
      LigmaImage *image = image_iter->data;

      g_object_unref (image);
    }
}

GList *
ligma_get_image_iter (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return LIGMA_LIST (ligma->images)->queue->head;
}

GList *
ligma_get_display_iter (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return LIGMA_LIST (ligma->displays)->queue->head;
}

GList *
ligma_get_image_windows (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_list_copy (ligma->image_windows);
}

GList *
ligma_get_paint_info_iter (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return LIGMA_LIST (ligma->paint_info_list)->queue->head;
}

GList *
ligma_get_tool_info_iter (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return LIGMA_LIST (ligma->tool_info_list)->queue->head;
}

GList *
ligma_get_tool_item_iter (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return LIGMA_LIST (ligma->tool_item_list)->queue->head;
}

GList *
ligma_get_tool_item_ui_iter (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return LIGMA_LIST (ligma->tool_item_ui_list)->queue->head;
}

LigmaObject *
ligma_get_clipboard_object (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (ligma->clipboard_image)
    return LIGMA_OBJECT (ligma->clipboard_image);

  return LIGMA_OBJECT (ligma->clipboard_buffer);
}

void
ligma_set_clipboard_image (Ligma      *ligma,
                          LigmaImage *image)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  g_clear_object (&ligma->clipboard_buffer);
  g_set_object (&ligma->clipboard_image, image);

  /* we want the signal emission */
  g_signal_emit (ligma, ligma_signals[CLIPBOARD_CHANGED], 0);
}

LigmaImage *
ligma_get_clipboard_image (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return ligma->clipboard_image;
}

void
ligma_set_clipboard_buffer (Ligma       *ligma,
                           LigmaBuffer *buffer)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (buffer == NULL || LIGMA_IS_BUFFER (buffer));

  g_clear_object (&ligma->clipboard_image);
  g_set_object (&ligma->clipboard_buffer, buffer);

  /* we want the signal emission */
  g_signal_emit (ligma, ligma_signals[CLIPBOARD_CHANGED], 0);
}

LigmaBuffer *
ligma_get_clipboard_buffer (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return ligma->clipboard_buffer;
}

LigmaImage *
ligma_create_image (Ligma              *ligma,
                   gint               width,
                   gint               height,
                   LigmaImageBaseType  type,
                   LigmaPrecision      precision,
                   gboolean           attach_comment)
{
  LigmaImage *image;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  image = ligma_image_new (ligma, width, height, type, precision);

  if (attach_comment)
    {
      const gchar *comment;

      comment = ligma_template_get_comment (ligma->config->default_image);

      if (comment)
        {
          LigmaParasite *parasite = ligma_parasite_new ("ligma-comment",
                                                      LIGMA_PARASITE_PERSISTENT,
                                                      strlen (comment) + 1,
                                                      comment);
          ligma_image_parasite_attach (image, parasite, FALSE);
          ligma_parasite_free (parasite);
        }
    }

  return image;
}

void
ligma_set_default_context (Ligma        *ligma,
                          LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  g_set_object (&ligma->default_context, context);
}

LigmaContext *
ligma_get_default_context (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return ligma->default_context;
}

void
ligma_set_user_context (Ligma        *ligma,
                       LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  g_set_object (&ligma->user_context, context);
}

LigmaContext *
ligma_get_user_context (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return ligma->user_context;
}

LigmaToolInfo *
ligma_get_tool_info (Ligma        *ligma,
                    const gchar *tool_id)
{
  gpointer info;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (tool_id != NULL, NULL);

  info = ligma_container_get_child_by_name (ligma->tool_info_list, tool_id);

  return (LigmaToolInfo *) info;
}

/**
 * ligma_message:
 * @ligma:     a pointer to the %Ligma object
 * @handler:  either a %LigmaProgress or a %GtkWidget pointer
 * @severity: severity of the message
 * @format:   printf-like format string
 * @...:      arguments to use with @format
 *
 * Present a message to the user. How exactly the message is displayed
 * depends on the @severity, the @handler object and user preferences.
 **/
void
ligma_message (Ligma                *ligma,
              GObject             *handler,
              LigmaMessageSeverity  severity,
              const gchar         *format,
              ...)
{
  va_list args;

  va_start (args, format);

  ligma_message_valist (ligma, handler, severity, format, args);

  va_end (args);
}

/**
 * ligma_message_valist:
 * @ligma:     a pointer to the %Ligma object
 * @handler:  either a %LigmaProgress or a %GtkWidget pointer
 * @severity: severity of the message
 * @format:   printf-like format string
 * @args:     arguments to use with @format
 *
 * See documentation for ligma_message().
 **/
void
ligma_message_valist (Ligma                *ligma,
                     GObject             *handler,
                     LigmaMessageSeverity  severity,
                     const gchar         *format,
                     va_list              args)
{
  gchar *message;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (handler == NULL || G_IS_OBJECT (handler));
  g_return_if_fail (format != NULL);

  message = g_strdup_vprintf (format, args);

  ligma_show_message (ligma, handler, severity, NULL, message);

  g_free (message);
}

void
ligma_message_literal (Ligma                *ligma,
                      GObject             *handler,
                      LigmaMessageSeverity  severity,
                      const gchar         *message)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (handler == NULL || G_IS_OBJECT (handler));
  g_return_if_fail (message != NULL);

  ligma_show_message (ligma, handler, severity, NULL, message);
}

void
ligma_filter_history_changed (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  g_signal_emit (ligma, ligma_signals[FILTER_HISTORY_CHANGED], 0);
}

void
ligma_image_opened (Ligma  *ligma,
                   GFile *file)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (G_IS_FILE (file));

  g_signal_emit (ligma, ligma_signals[IMAGE_OPENED], 0, file);
}

GFile *
ligma_get_temp_file (Ligma        *ligma,
                    const gchar *extension)
{
  static gint  id = 0;
  static gint  pid;
  gchar       *basename;
  GFile       *dir;
  GFile       *file;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  if (id == 0)
    pid = ligma_get_pid ();

  if (extension)
    basename = g_strdup_printf ("ligma-temp-%d%d.%s", pid, id++, extension);
  else
    basename = g_strdup_printf ("ligma-temp-%d%d", pid, id++);

  dir = ligma_file_new_for_config_path (LIGMA_GEGL_CONFIG (ligma->config)->temp_path,
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
