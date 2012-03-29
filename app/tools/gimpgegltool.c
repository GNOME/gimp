/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpdrawable.h"
#include "core/gimperror.h"
#include "core/gimpimage.h"
#include "core/gimpimagemap.h"
#include "core/gimpparamspecs-duplicate.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"

#include "display/gimpdisplay.h"

#include "gimpgegltool.h"
#include "gimpimagemapoptions.h"

#include "gimp-intl.h"


enum
{
  COLUMN_NAME,
  COLUMN_LABEL,
  COLUMN_STOCK_ID,
  N_COLUMNS
};


/*  local function prototypes  */

static void       gimp_gegl_tool_finalize          (GObject           *object);

static gboolean   gimp_gegl_tool_initialize        (GimpTool          *tool,
                                                    GimpDisplay       *display,
                                                    GError           **error);

static GeglNode * gimp_gegl_tool_get_operation     (GimpImageMapTool  *im_tool,
                                                    GObject          **config);
static void       gimp_gegl_tool_map               (GimpImageMapTool  *im_tool);
static void       gimp_gegl_tool_dialog            (GimpImageMapTool  *im_tool);
static void       gimp_gegl_tool_reset             (GimpImageMapTool  *im_tool);

static void       gimp_gegl_tool_config_notify     (GObject           *object,
                                                    GParamSpec        *pspec,
                                                    GimpGeglTool      *tool);

static void       gimp_gegl_tool_operation_changed (GtkWidget         *widget,
                                                    GimpGeglTool      *tool);


G_DEFINE_TYPE (GimpGeglTool, gimp_gegl_tool, GIMP_TYPE_IMAGE_MAP_TOOL)

#define parent_class gimp_gegl_tool_parent_class


void
gimp_gegl_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_GEGL_TOOL,
                GIMP_TYPE_IMAGE_MAP_OPTIONS, NULL,
                0,
                "gimp-gegl-tool",
                _("GEGL Operation"),
                _("GEGL Tool: Use an arbitrary GEGL operation"),
                N_("_GEGL Operation..."), NULL,
                NULL, GIMP_HELP_TOOL_GEGL,
                GIMP_STOCK_GEGL,
                data);
}

static void
gimp_gegl_tool_class_init (GimpGeglToolClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpToolClass         *tool_class    = GIMP_TOOL_CLASS (klass);
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  object_class->finalize       = gimp_gegl_tool_finalize;

  tool_class->initialize       = gimp_gegl_tool_initialize;

  im_tool_class->dialog_desc   = _("GEGL Operation");

  im_tool_class->get_operation = gimp_gegl_tool_get_operation;
  im_tool_class->map           = gimp_gegl_tool_map;
  im_tool_class->dialog        = gimp_gegl_tool_dialog;
  im_tool_class->reset         = gimp_gegl_tool_reset;
}

static void
gimp_gegl_tool_init (GimpGeglTool *tool)
{
}

static void
gimp_gegl_tool_finalize (GObject *object)
{
  GimpGeglTool *tool = GIMP_GEGL_TOOL (object);

  if (tool->operation)
    {
      g_free (tool->operation);
      tool->operation = NULL;
    }

  if (tool->config)
    {
      g_object_unref (tool->config);
      tool->config = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_gegl_tool_initialize (GimpTool     *tool,
                           GimpDisplay  *display,
                           GError      **error)
{
  GimpGeglTool *g_tool   = GIMP_GEGL_TOOL (tool);
  GimpImage    *image    = gimp_display_get_image (display);
  GimpDrawable *drawable = gimp_image_get_active_drawable (image);

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("GEGL operations do not operate on indexed layers."));
      return FALSE;
    }

  if (g_tool->config)
    gimp_config_reset (GIMP_CONFIG (g_tool->config));

  return GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);
}

static GeglNode *
gimp_gegl_tool_get_operation (GimpImageMapTool  *im_tool,
                              GObject          **config)
{
  return g_object_new (GEGL_TYPE_NODE, NULL);
}

static void
gimp_gegl_tool_map (GimpImageMapTool *image_map_tool)
{
  GimpGeglTool  *tool = GIMP_GEGL_TOOL (image_map_tool);
  GParamSpec   **pspecs;
  guint          n_pspecs;
  gint           i;

  if (! tool->config)
    return;

  pspecs = gegl_operation_list_properties (tool->operation, &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *gegl_pspec = pspecs[i];
      GParamSpec *gimp_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (tool->config),
                                                             gegl_pspec->name);

      if (gimp_pspec)
        {
          GValue value = { 0, };

          g_value_init (&value, gimp_pspec->value_type);

          g_object_get_property (G_OBJECT (tool->config), gimp_pspec->name,
                                 &value);

          if (GIMP_IS_PARAM_SPEC_RGB (gimp_pspec))
            {
              GeglColor *gegl_color = gegl_color_new (NULL);
              GimpRGB    gimp_color;

              gimp_value_get_rgb (&value, &gimp_color);
              g_value_unset (&value);

              gegl_color_set_rgba (gegl_color,
                                   gimp_color.r,
                                   gimp_color.g,
                                   gimp_color.b,
                                   gimp_color.a);

              g_value_init (&value, gegl_pspec->value_type);
              g_value_take_object (&value, gegl_color);
            }

          gegl_node_set_property (image_map_tool->operation, gegl_pspec->name,
                                  &value);

          g_value_unset (&value);
        }
    }

  g_free (pspecs);
}

static gboolean
gimp_gegl_tool_operation_blacklisted (const gchar *name,
                                      const gchar *categories_str)
{
  static const gchar * const category_blacklist[] =
  {
    "compositors",
    "core",
    "debug",
    "hidden",
    "input",
    "output",
    "programming",
    "transform",
    "video"
  };
  static const gchar * const name_blacklist[] =
  {
    "gegl:convert-format",
    "gegl:introspect",
    "gegl:path",
    "gegl:text",
    "gegl:layer",
    "gegl:contrast-curve",
    "gegl:fill-path",
    "gegl:vector-stroke",
    "gegl:lens-correct",
    "gegl:hstack",
    "gimp-",
    "gimp:"
  };

  gchar **categories;
  gint    i;

  /* Operations with no name are abstract base classes */
  if (! name)
    return TRUE;

  for (i = 0; i < G_N_ELEMENTS (name_blacklist); i++)
    {
      if (g_str_has_prefix (name, name_blacklist[i]))
        return TRUE;
    }

  if (! categories_str)
    return FALSE;

  categories = g_strsplit (categories_str, ":", 0);

  for (i = 0; i < G_N_ELEMENTS (category_blacklist); i++)
    {
      gint j;

      for (j = 0; categories[j]; j++)
        if (! strcmp (categories[j], category_blacklist[i]))
          {
            g_strfreev (categories);
            return TRUE;
          }
    }

  g_strfreev (categories);

  return FALSE;
}


/* Builds a GList of the class structures of all subtypes of type.
 */
static GList *
gimp_get_subtype_classes (GType  type,
                          GList *classes)
{
  GeglOperationClass *klass;
  GType              *ops;
  guint               n_ops;
  gint                i;

  if (! type)
    return classes;

  klass = GEGL_OPERATION_CLASS (g_type_class_ref (type));
  ops = g_type_children (type, &n_ops);

  if (! gimp_gegl_tool_operation_blacklisted (klass->name,
                                              gegl_operation_class_get_key (klass,
                                                "categories")))
    classes = g_list_prepend (classes, klass);

  for (i = 0; i < n_ops; i++)
    classes = gimp_get_subtype_classes (ops[i], classes);

  if (ops)
    g_free (ops);

  return classes;
}

static gint
gimp_gegl_tool_compare_operation_names (GeglOperationClass *a,
                                        GeglOperationClass *b)
{
  return strcmp (a->name, b->name);
}

static GList *
gimp_get_geglopclasses (void)
{
  GList *opclasses;

  opclasses = gimp_get_subtype_classes (GEGL_TYPE_OPERATION, NULL);

  opclasses = g_list_sort (opclasses,
                           (GCompareFunc)
                           gimp_gegl_tool_compare_operation_names);

  return opclasses;
}


/*****************/
/*  Gegl dialog  */
/*****************/

static void
gimp_gegl_tool_dialog (GimpImageMapTool *image_map_tool)
{
  GimpGeglTool    *tool = GIMP_GEGL_TOOL (image_map_tool);
  GtkListStore    *store;
  GtkCellRenderer *cell;
  GtkWidget       *main_vbox;
  GtkWidget       *hbox;
  GtkWidget       *label;
  GtkWidget       *combo;
  GList           *opclasses;
  GList           *iter;

  main_vbox = gimp_image_map_tool_dialog_get_vbox (image_map_tool);

  /*  The operation combo box  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Operation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = gtk_list_store_new (N_COLUMNS,
                              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  opclasses = gimp_get_geglopclasses ();

  for (iter = opclasses; iter; iter = iter->next)
    {
      GeglOperationClass *opclass = GEGL_OPERATION_CLASS (iter->data);
      const gchar        *stock_id;
      const gchar        *label;

      if (g_str_has_prefix (opclass->name, "gegl:"))
        {
          label    = opclass->name + strlen ("gegl:");
          stock_id = GIMP_STOCK_GEGL;
        }
      else
        {
          label    = opclass->name;
          stock_id = NULL;
        }

      gtk_list_store_insert_with_values (store, NULL, -1,
                                         COLUMN_NAME,     opclass->name,
                                         COLUMN_LABEL,    label,
                                         COLUMN_STOCK_ID, stock_id,
                                         -1);
    }

  g_list_free (opclasses);

  combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, FALSE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell,
                                 "stock-id", COLUMN_STOCK_ID);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell,
                                 "text", COLUMN_LABEL);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_gegl_tool_operation_changed),
                    tool);

  tool->operation_combo = combo;

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  /*  The options vbox  */
  tool->options_frame = gimp_frame_new (_("Operation Settings"));
  gtk_box_pack_start (GTK_BOX (main_vbox), tool->options_frame,
                      FALSE, FALSE, 0);
  gtk_widget_show (tool->options_frame);

  tool->options_table = gtk_label_new (_("Select an operation from the list above"));
  gimp_label_set_attributes (GTK_LABEL (tool->options_table),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_misc_set_padding (GTK_MISC (tool->options_table), 0, 4);
  gtk_container_add (GTK_CONTAINER (tool->options_frame), tool->options_table);
  gtk_widget_show (tool->options_table);
}

static void
gimp_gegl_tool_reset (GimpImageMapTool *image_map_tool)
{
  GimpGeglTool *tool = GIMP_GEGL_TOOL (image_map_tool);

  if (tool->config)
    gimp_config_reset (GIMP_CONFIG (tool->config));
}

static void
gimp_gegl_tool_config_notify (GObject      *object,
                              GParamSpec   *pspec,
                              GimpGeglTool *tool)
{
  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}

static GValue *
gimp_gegl_tool_config_value_new (GParamSpec *pspec)
{
  GValue *value = g_slice_new0 (GValue);

  g_value_init (value, pspec->value_type);

  return value;
}

static void
gimp_gegl_tool_config_value_free (GValue *value)
{
  g_value_unset (value);
  g_slice_free (GValue, value);
}

static GHashTable *
gimp_gegl_tool_config_get_properties (GObject *object)
{
  GHashTable *properties = g_object_get_data (object, "properties");

  if (! properties)
    {
      properties = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          (GDestroyNotify) g_free,
                                          (GDestroyNotify) gimp_gegl_tool_config_value_free);

      g_object_set_data_full (object, "properties", properties,
                              (GDestroyNotify) g_hash_table_unref);
    }

  return properties;
}

static GValue *
gimp_gegl_tool_config_value_get (GObject    *object,
                                 GParamSpec *pspec)
{
  GHashTable *properties = gimp_gegl_tool_config_get_properties (object);
  GValue     *value;

  value = g_hash_table_lookup (properties, pspec->name);

  if (! value)
    {
      value = gimp_gegl_tool_config_value_new (pspec);
      g_hash_table_insert (properties, g_strdup (pspec->name), value);
    }

  return value;
}

static void
gimp_gegl_tool_config_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GValue *val = gimp_gegl_tool_config_value_get (object, pspec);

  g_value_copy (value, val);
}

static void
gimp_gegl_tool_config_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GValue *val = gimp_gegl_tool_config_value_get (object, pspec);

  g_value_copy (val, value);
}

static void
gimp_gegl_tool_config_class_init (GObjectClass *klass,
                                  const gchar  *operation)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  gint         i;

  klass->set_property = gimp_gegl_tool_config_set_property;
  klass->get_property = gimp_gegl_tool_config_get_property;

  pspecs = gegl_operation_list_properties (operation, &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      if ((pspec->flags & G_PARAM_READABLE) &&
          (pspec->flags & G_PARAM_WRITABLE) &&
          strcmp (pspec->name, "input")     &&
          strcmp (pspec->name, "output"))
        {
          GParamSpec *copy = gimp_param_spec_duplicate (pspec);

          if (copy)
            {
              g_object_class_install_property (klass, i + 1, copy);
            }
        }
    }

  g_free (pspecs);
}

static GimpObject *
gimp_gegl_tool_get_config (GimpGeglTool *tool)
{
  static GHashTable *config_types = NULL;
  GType              config_type;

  if (! config_types)
    config_types = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          (GDestroyNotify) g_free,
                                          NULL);

  config_type = (GType) g_hash_table_lookup (config_types, tool->operation);

  if (! config_type)
    {
      GTypeInfo info =
      {
        sizeof (GimpObjectClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_gegl_tool_config_class_init,
        NULL,           /* class_finalize */
        tool->operation,
        sizeof (GimpObject),
        0,              /* n_preallocs */
        (GInstanceInitFunc) NULL,
      };

      const GInterfaceInfo config_info =
      {
        NULL, /* interface_init     */
        NULL, /* interface_finalize */
        NULL  /* interface_data     */
      };

      gchar *type_name = g_strdup_printf ("GimpGeglTool-%s-config",
                                          tool->operation);

      g_strcanon (type_name, G_CSET_DIGITS "-" G_CSET_a_2_z G_CSET_A_2_Z, '-');

      config_type = g_type_register_static (GIMP_TYPE_OBJECT, type_name,
                                            &info, 0);

      g_free (type_name);

      g_type_add_interface_static (config_type, GIMP_TYPE_CONFIG,
                                   &config_info);

      g_hash_table_insert (config_types,
                           g_strdup (tool->operation),
                           (gpointer) config_type);
    }

  return g_object_new (config_type, NULL);
}

static void
gimp_gegl_tool_operation_changed (GtkWidget    *widget,
                                  GimpGeglTool *tool)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

  if (tool->operation)
    {
      g_free (tool->operation);
      tool->operation = NULL;
    }

  if (tool->config)
    {
      g_object_unref (tool->config);
      tool->config = NULL;
    }

  gtk_tree_model_get (model, &iter,
                      COLUMN_NAME, &tool->operation,
                      -1);

  if (! tool->operation)
    return;

  if (GIMP_IMAGE_MAP_TOOL (tool)->image_map)
    {
      gimp_image_map_clear (GIMP_IMAGE_MAP_TOOL (tool)->image_map);
      g_object_unref (GIMP_IMAGE_MAP_TOOL (tool)->image_map);
      GIMP_IMAGE_MAP_TOOL (tool)->image_map = NULL;
    }

  gegl_node_set (GIMP_IMAGE_MAP_TOOL (tool)->operation,
                 "operation", tool->operation,
                 NULL);

  gimp_image_map_tool_create_map (GIMP_IMAGE_MAP_TOOL (tool));

  tool->config = gimp_gegl_tool_get_config (tool);

  if (tool->options_table)
    {
      gtk_widget_destroy (tool->options_table);
      tool->options_table = NULL;
    }

  if (tool->config)
    {
      g_signal_connect_object (tool->config, "notify",
                               G_CALLBACK (gimp_gegl_tool_config_notify),
                               G_OBJECT (tool), 0);

      tool->options_table =
        gimp_prop_table_new (G_OBJECT (tool->config),
                             G_TYPE_FROM_INSTANCE (tool->config),
                             GIMP_CONTEXT (GIMP_TOOL_GET_OPTIONS (tool)));
      gtk_container_add (GTK_CONTAINER (tool->options_frame),
                         tool->options_table);
      gtk_widget_show (tool->options_table);
    }

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}
