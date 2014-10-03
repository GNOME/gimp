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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h> /* strlen */

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimprc.h"

#include "pdb/gimppdb.h"
#include "pdb/gimp-pdb-compat.h"
#include "pdb/internal-procs.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginmanager-restore.h"

#include "paint/gimp-paint.h"

#include "text/gimp-fonts.h"

#include "xcf/xcf.h"

#include "gimp.h"
#include "gimp-contexts.h"
#include "gimp-gradients.h"
#include "gimp-memsize.h"
#include "gimp-modules.h"
#include "gimp-parasites.h"
#include "gimp-templates.h"
#include "gimp-units.h"
#include "gimp-utils.h"
#include "gimpbrush-load.h"
#include "gimpbrush.h"
#include "gimpbrushclipboard.h"
#include "gimpbrushgenerated-load.h"
#include "gimpbrushpipe-load.h"
#include "gimpbuffer.h"
#include "gimpcontext.h"
#include "gimpdatafactory.h"
#include "gimpdynamics.h"
#include "gimpdynamics-load.h"
#include "gimpdocumentlist.h"
#include "gimpgradient-load.h"
#include "gimpgradient.h"
#include "gimpidtable.h"
#include "gimpimage.h"
#include "gimpimagefile.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimppalette-load.h"
#include "gimppalette.h"
#include "gimpparasitelist.h"
#include "gimppattern-load.h"
#include "gimppattern.h"
#include "gimppatternclipboard.h"
#include "gimptagcache.h"
#include "gimptemplate.h"
#include "gimptoolinfo.h"
#include "gimptoolpreset.h"
#include "gimptoolpreset-load.h"

#include "gimp-intl.h"


enum
{
  INITIALIZE,
  RESTORE,
  EXIT,
  BUFFER_CHANGED,
  IMAGE_OPENED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VERBOSE
};


static void      gimp_constructed          (GObject           *object);
static void      gimp_set_property         (GObject           *object,
                                            guint              property_id,
                                            const GValue      *value,
                                            GParamSpec        *pspec);
static void      gimp_get_property         (GObject           *object,
                                            guint              property_id,
                                            GValue            *value,
                                            GParamSpec        *pspec);
static void      gimp_dispose              (GObject           *object);
static void      gimp_finalize             (GObject           *object);

static gint64    gimp_get_memsize          (GimpObject        *object,
                                            gint64            *gui_size);

static void      gimp_real_initialize      (Gimp              *gimp,
                                            GimpInitStatusFunc status_callback);
static void      gimp_real_restore         (Gimp              *gimp,
                                            GimpInitStatusFunc status_callback);
static gboolean  gimp_real_exit            (Gimp              *gimp,
                                            gboolean           force);

static void      gimp_global_config_notify (GObject           *global_config,
                                            GParamSpec        *param_spec,
                                            GObject           *edit_config);
static void      gimp_edit_config_notify   (GObject           *edit_config,
                                            GParamSpec        *param_spec,
                                            GObject           *global_config);


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
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);

  gimp_signals[RESTORE] =
    g_signal_new ("restore",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass, restore),
                  NULL, NULL,
                  gimp_marshal_VOID__POINTER,
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

  gimp_signals[BUFFER_CHANGED] =
    g_signal_new ("buffer-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass, buffer_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_signals[IMAGE_OPENED] =
    g_signal_new ("image-opened",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpClass, image_opened),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT,
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
  klass->buffer_changed          = NULL;

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

  gimp->images = gimp_list_new_weak (GIMP_TYPE_IMAGE, FALSE);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->images), "images");

  gimp->next_guide_ID        = 1;
  gimp->next_sample_point_ID = 1;
  gimp->image_table          = gimp_id_table_new ();
  gimp->item_table           = gimp_id_table_new ();

  gimp->displays = g_object_new (GIMP_TYPE_LIST,
                                 "children-type", GIMP_TYPE_OBJECT,
                                 "policy",        GIMP_CONTAINER_POLICY_WEAK,
                                 "append",        TRUE,
                                 NULL);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->displays), "displays");
  gimp->next_display_ID = 1;

  gimp->named_buffers = gimp_list_new (GIMP_TYPE_BUFFER, TRUE);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->named_buffers),
                               "named buffers");

  gimp->tool_info_list = gimp_list_new (GIMP_TYPE_TOOL_INFO, FALSE);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->tool_info_list),
                               "tool infos");

  gimp->templates = gimp_list_new (GIMP_TYPE_TEMPLATE, TRUE);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->templates), "templates");
}

static void
gimp_constructed (GObject *object)
{
  Gimp *gimp = GIMP (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_units_init (gimp);
  gimp_modules_init (gimp);

  gimp->plug_in_manager = gimp_plug_in_manager_new (gimp);
  gimp->pdb             = gimp_pdb_new (gimp);

  xcf_init (gimp);

  gimp->documents = gimp_document_list_new (gimp);
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

  if (gimp->brush_factory)
    gimp_data_factory_data_free (gimp->brush_factory);

  if (gimp->dynamics_factory)
    gimp_data_factory_data_free (gimp->dynamics_factory);

  if (gimp->pattern_factory)
    gimp_data_factory_data_free (gimp->pattern_factory);

  if (gimp->gradient_factory)
    gimp_data_factory_data_free (gimp->gradient_factory);

  if (gimp->palette_factory)
    gimp_data_factory_data_free (gimp->palette_factory);

  if (gimp->tool_preset_factory)
    gimp_data_factory_data_free (gimp->tool_preset_factory);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_finalize (GObject *object)
{
  Gimp *gimp = GIMP (object);

  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  gimp_contexts_exit (gimp);

  if (gimp->image_new_last_template)
    {
      g_object_unref (gimp->image_new_last_template);
      gimp->image_new_last_template = NULL;
    }

  if (gimp->templates)
    {
      g_object_unref (gimp->templates);
      gimp->templates = NULL;
    }

  if (gimp->documents)
    {
      g_object_unref (gimp->documents);
      gimp->documents = NULL;
    }

  gimp_tool_info_set_standard (gimp, NULL);

  if (gimp->tool_info_list)
    {
      gimp_container_foreach (gimp->tool_info_list,
                              (GFunc) g_object_run_dispose, NULL);
      g_object_unref (gimp->tool_info_list);
      gimp->tool_info_list = NULL;
    }

  xcf_exit (gimp);

  if (gimp->pdb)
    {
      g_object_unref (gimp->pdb);
      gimp->pdb = NULL;
    }

  if (gimp->brush_factory)
    {
      g_object_unref (gimp->brush_factory);
      gimp->brush_factory = NULL;
    }

  if (gimp->dynamics_factory)
    {
      g_object_unref (gimp->dynamics_factory);
      gimp->dynamics_factory = NULL;
    }

  if (gimp->pattern_factory)
    {
      g_object_unref (gimp->pattern_factory);
      gimp->pattern_factory = NULL;
    }

  if (gimp->gradient_factory)
    {
      g_object_unref (gimp->gradient_factory);
      gimp->gradient_factory = NULL;
    }

  if (gimp->palette_factory)
    {
      g_object_unref (gimp->palette_factory);
      gimp->palette_factory = NULL;
    }

  if (gimp->tool_preset_factory)
    {
      g_object_unref (gimp->tool_preset_factory);
      gimp->tool_preset_factory = NULL;
    }

  if (gimp->tag_cache)
    {
      g_object_unref (gimp->tag_cache);
      gimp->tag_cache = NULL;
    }

  if (gimp->fonts)
    {
      g_object_unref (gimp->fonts);
      gimp->fonts = NULL;
    }

  if (gimp->named_buffers)
    {
      g_object_unref (gimp->named_buffers);
      gimp->named_buffers = NULL;
    }

  if (gimp->global_buffer)
    {
      g_object_unref (gimp->global_buffer);
      gimp->global_buffer = NULL;
    }

  if (gimp->displays)
    {
      g_object_unref (gimp->displays);
      gimp->displays = NULL;
    }

  if (gimp->item_table)
    {
      g_object_unref (gimp->item_table);
      gimp->item_table = NULL;
    }

  if (gimp->image_table)
    {
      g_object_unref (gimp->image_table);
      gimp->image_table = NULL;
    }

  if (gimp->images)
    {
      g_object_unref (gimp->images);
      gimp->images = NULL;
    }

  if (gimp->plug_in_manager)
    {
      g_object_unref (gimp->plug_in_manager);
      gimp->plug_in_manager = NULL;
    }

  if (gimp->module_db)
    gimp_modules_exit (gimp);

  gimp_paint_exit (gimp);

  if (gimp->parasites)
    {
      g_object_unref (gimp->parasites);
      gimp->parasites = NULL;
    }

  if (gimp->edit_config)
    {
      g_object_unref (gimp->edit_config);
      gimp->edit_config = NULL;
    }

  if (gimp->default_folder)
    {
      g_object_unref (gimp->default_folder);
      gimp->default_folder = NULL;
    }

  if (gimp->session_name)
    {
      g_free (gimp->session_name);
      gimp->session_name = NULL;
    }

  if (gimp->context_list)
    {
      g_list_free (gimp->context_list);
      gimp->context_list = NULL;
    }

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

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->image_table), 0);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->item_table),  0);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->displays), gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->global_buffer),
                                      gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->named_buffers),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->fonts),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->brush_factory),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->dynamics_factory),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->pattern_factory),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->gradient_factory),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->palette_factory),
                                      gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->tool_preset_factory),
                                      gui_size);

  memsize += gimp_object_get_memsize (GIMP_OBJECT (gimp->tag_cache),
                                      gui_size);

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
  static const GimpDataFactoryLoaderEntry brush_loader_entries[] =
  {
    { gimp_brush_load,           GIMP_BRUSH_FILE_EXTENSION,           FALSE },
    { gimp_brush_load,           GIMP_BRUSH_PIXMAP_FILE_EXTENSION,    FALSE },
    { gimp_brush_load_abr,       GIMP_BRUSH_PS_FILE_EXTENSION,        FALSE },
    { gimp_brush_load_abr,       GIMP_BRUSH_PSP_FILE_EXTENSION,       FALSE },
    { gimp_brush_generated_load, GIMP_BRUSH_GENERATED_FILE_EXTENSION, TRUE  },
    { gimp_brush_pipe_load,      GIMP_BRUSH_PIPE_FILE_EXTENSION,      FALSE }
  };

  static const GimpDataFactoryLoaderEntry dynamics_loader_entries[] =
  {
    { gimp_dynamics_load,        GIMP_DYNAMICS_FILE_EXTENSION,        TRUE  }
  };

  static const GimpDataFactoryLoaderEntry pattern_loader_entries[] =
  {
    { gimp_pattern_load,         GIMP_PATTERN_FILE_EXTENSION,         FALSE },
    { gimp_pattern_load_pixbuf,  NULL /* fallback loader */,          FALSE }
  };

  static const GimpDataFactoryLoaderEntry gradient_loader_entries[] =
  {
    { gimp_gradient_load,        GIMP_GRADIENT_FILE_EXTENSION,        TRUE  },
    { gimp_gradient_load_svg,    GIMP_GRADIENT_SVG_FILE_EXTENSION,    FALSE }
  };

  static const GimpDataFactoryLoaderEntry palette_loader_entries[] =
  {
    { gimp_palette_load,         GIMP_PALETTE_FILE_EXTENSION,         TRUE  }
  };

  static const GimpDataFactoryLoaderEntry tool_preset_loader_entries[] =
  {
    { gimp_tool_preset_load,     GIMP_TOOL_PRESET_FILE_EXTENSION,     TRUE  }
  };

  GimpData *clipboard_brush;
  GimpData *clipboard_pattern;

  if (gimp->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  status_callback (_("Initialization"), NULL, 0.0);

  gimp_fonts_init (gimp);

  gimp->brush_factory =
    gimp_data_factory_new (gimp,
                           GIMP_TYPE_BRUSH,
                           "brush-path", "brush-path-writable",
                           brush_loader_entries,
                           G_N_ELEMENTS (brush_loader_entries),
                           gimp_brush_new,
                           gimp_brush_get_standard);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->brush_factory),
                               "brush factory");

  gimp->dynamics_factory =
    gimp_data_factory_new (gimp,
                           GIMP_TYPE_DYNAMICS,
                           "dynamics-path", "dynamics-path-writable",
                           dynamics_loader_entries,
                           G_N_ELEMENTS (dynamics_loader_entries),
                           gimp_dynamics_new,
                           gimp_dynamics_get_standard);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->dynamics_factory),
                               "dynamics factory");

  gimp->pattern_factory =
    gimp_data_factory_new (gimp,
                           GIMP_TYPE_PATTERN,
                           "pattern-path", "pattern-path-writable",
                           pattern_loader_entries,
                           G_N_ELEMENTS (pattern_loader_entries),
                           NULL,
                           gimp_pattern_get_standard);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->pattern_factory),
                               "pattern factory");

  gimp->gradient_factory =
    gimp_data_factory_new (gimp,
                           GIMP_TYPE_GRADIENT,
                           "gradient-path", "gradient-path-writable",
                           gradient_loader_entries,
                           G_N_ELEMENTS (gradient_loader_entries),
                           gimp_gradient_new,
                           gimp_gradient_get_standard);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->gradient_factory),
                               "gradient factory");

  gimp->palette_factory =
    gimp_data_factory_new (gimp,
                           GIMP_TYPE_PALETTE,
                           "palette-path", "palette-path-writable",
                           palette_loader_entries,
                           G_N_ELEMENTS (palette_loader_entries),
                           gimp_palette_new,
                           gimp_palette_get_standard);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->palette_factory),
                               "palette factory");

  gimp->tool_preset_factory =
    gimp_data_factory_new (gimp,
                           GIMP_TYPE_TOOL_PRESET,
                           "tool-preset-path", "tool-preset-path-writable",
                           tool_preset_loader_entries,
                           G_N_ELEMENTS (tool_preset_loader_entries),
                           gimp_tool_preset_new,
                           NULL);
  gimp_object_set_static_name (GIMP_OBJECT (gimp->tool_preset_factory),
                               "tool preset factory");

  gimp->tag_cache = gimp_tag_cache_new ();

  gimp_paint_init (gimp);

  /* Set the last values used to default values. */
  gimp->image_new_last_template =
    gimp_config_duplicate (GIMP_CONFIG (gimp->config->default_image));

  /*  create user and default context  */
  gimp_contexts_init (gimp);

  /*  add the builtin FG -> BG etc. gradients  */
  gimp_gradients_init (gimp);

  /*  add the clipboard brush  */
  clipboard_brush = gimp_brush_clipboard_new (gimp);
  gimp_data_make_internal (GIMP_DATA (clipboard_brush),
                           "gimp-brush-clipboard");
  gimp_container_add (gimp_data_factory_get_container (gimp->brush_factory),
                      GIMP_OBJECT (clipboard_brush));
  g_object_unref (clipboard_brush);

  /*  add the clipboard pattern  */
  clipboard_pattern = gimp_pattern_clipboard_new (gimp);
  gimp_data_make_internal (GIMP_DATA (clipboard_pattern),
                           "gimp-pattern-clipboard");
  gimp_container_add (gimp_data_factory_get_container (gimp->pattern_factory),
                      GIMP_OBJECT (clipboard_pattern));
  g_object_unref (clipboard_pattern);

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

  gimp->restored = TRUE;
}

static gboolean
gimp_real_exit (Gimp     *gimp,
                gboolean  force)
{
  if (gimp->be_verbose)
    g_print ("EXIT: %s\n", G_STRFUNC);

  gimp_plug_in_manager_exit (gimp->plug_in_manager);
  gimp_modules_unload (gimp);

  gimp_tag_cache_save (gimp->tag_cache);

  gimp_data_factory_data_save (gimp->brush_factory);
  gimp_data_factory_data_save (gimp->dynamics_factory);
  gimp_data_factory_data_save (gimp->pattern_factory);
  gimp_data_factory_data_save (gimp->gradient_factory);
  gimp_data_factory_data_save (gimp->palette_factory);
  gimp_data_factory_data_save (gimp->tool_preset_factory);

  gimp_fonts_reset (gimp);

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
  GValue global_value = { 0, };
  GValue edit_value   = { 0, };

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
  GValue edit_value   = { 0, };
  GValue global_value = { 0, };

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

  gimprc = gimp_rc_new (alternate_system_gimprc,
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

void
gimp_restore (Gimp               *gimp,
              GimpInitStatusFunc  status_callback)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (status_callback != NULL);

  if (gimp->be_verbose)
    g_print ("INIT: %s\n", G_STRFUNC);

  /*  initialize  the global parasite table  */
  status_callback (_("Looking for data files"), _("Parasites"), 0.0);
  gimp_parasiterc_load (gimp);

  /*  initialize the list of gimp brushes    */
  status_callback (NULL, _("Brushes"), 0.1);
  gimp_data_factory_data_init (gimp->brush_factory, gimp->user_context,
                               gimp->no_data);

  /*  initialize the list of gimp dynamics   */
  status_callback (NULL, _("Dynamics"), 0.2);
  gimp_data_factory_data_init (gimp->dynamics_factory, gimp->user_context,
                               gimp->no_data);

  /*  initialize the list of gimp patterns   */
  status_callback (NULL, _("Patterns"), 0.3);
  gimp_data_factory_data_init (gimp->pattern_factory, gimp->user_context,
                               gimp->no_data);

  /*  initialize the list of gimp palettes   */
  status_callback (NULL, _("Palettes"), 0.4);
  gimp_data_factory_data_init (gimp->palette_factory, gimp->user_context,
                               gimp->no_data);

  /*  initialize the list of gimp gradients  */
  status_callback (NULL, _("Gradients"), 0.5);
  gimp_data_factory_data_init (gimp->gradient_factory, gimp->user_context,
                               gimp->no_data);

  /*  initialize the list of fonts  */
  status_callback (NULL, _("Fonts (this may take a while)"), 0.6);
  if (! gimp->no_fonts)
    gimp_fonts_load (gimp);

  /*  initialize the list of gimp tool presets if we have a GUI  */
  if (! gimp->no_interface)
    {
      status_callback (NULL, _("Tool Presets"), 0.65);
      gimp_data_factory_data_init (gimp->tool_preset_factory, gimp->user_context,
                                   gimp->no_data);
    }

  /*  initialize the template list  */
  status_callback (NULL, _("Templates"), 0.7);
  gimp_templates_load (gimp);

  /*  initialize the module list  */
  status_callback (NULL, _("Modules"), 0.8);
  gimp_modules_load (gimp);

  /* update tag cache */
  status_callback (NULL, _("Updating tag cache"), 0.9);
  gimp_tag_cache_load (gimp->tag_cache);
  gimp_tag_cache_add_container (gimp->tag_cache,
                                gimp_data_factory_get_container (gimp->brush_factory));
  gimp_tag_cache_add_container (gimp->tag_cache,
                                gimp_data_factory_get_container (gimp->dynamics_factory));
  gimp_tag_cache_add_container (gimp->tag_cache,
                                gimp_data_factory_get_container (gimp->pattern_factory));
  gimp_tag_cache_add_container (gimp->tag_cache,
                                gimp_data_factory_get_container (gimp->gradient_factory));
  gimp_tag_cache_add_container (gimp->tag_cache,
                                gimp_data_factory_get_container (gimp->palette_factory));
  gimp_tag_cache_add_container (gimp->tag_cache,
                                gimp_data_factory_get_container (gimp->tool_preset_factory));

  g_signal_emit (gimp, gimp_signals[RESTORE], 0, status_callback);
}

/**
 * gimp_is_restored:
 * @gimp: a #Gimp object
 *
 * Return value: %TRUE if GIMP is completely started, %FALSE otherwise.
 **/
gboolean
gimp_is_restored (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  return gimp->restored;
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
}

GList *
gimp_get_image_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->images)->list;
}

GList *
gimp_get_display_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->displays)->list;
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

  return GIMP_LIST (gimp->paint_info_list)->list;
}

GList *
gimp_get_tool_info_iter (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return GIMP_LIST (gimp->tool_info_list)->list;
}

void
gimp_set_global_buffer (Gimp       *gimp,
                        GimpBuffer *buffer)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (buffer == NULL || GIMP_IS_BUFFER (buffer));

  if (buffer == gimp->global_buffer)
    return;

  if (gimp->global_buffer)
    g_object_unref (gimp->global_buffer);

  gimp->global_buffer = buffer;

  if (gimp->global_buffer)
    g_object_ref (gimp->global_buffer);

  g_signal_emit (gimp, gimp_signals[BUFFER_CHANGED], 0);
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
          gimp_image_parasite_attach (image, parasite);
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

  if (context != gimp->default_context)
    {
      if (gimp->default_context)
        g_object_unref (gimp->default_context);

      gimp->default_context = context;

      if (gimp->default_context)
        g_object_ref (gimp->default_context);
    }
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

  if (context != gimp->user_context)
    {
      if (gimp->user_context)
        g_object_unref (gimp->user_context);

      gimp->user_context = context;

      if (gimp->user_context)
        g_object_ref (gimp->user_context);
    }
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
  gchar       *path;
  GFile       *dir;
  GFile       *file;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (id == 0)
    pid = gimp_get_pid ();

  if (extension)
    basename = g_strdup_printf ("gimp-temp-%d%d.%s", pid, id++, extension);
  else
    basename = g_strdup_printf ("gimp-temp-%d%d", pid, id++);

  path = gimp_config_path_expand (GIMP_GEGL_CONFIG (gimp->config)->temp_path,
                                  TRUE, NULL);

  dir = g_file_new_for_path (path);
  g_free (path);

  file = g_file_get_child (dir, basename);
  g_free (basename);
  g_object_unref (dir);

  return file;
}
