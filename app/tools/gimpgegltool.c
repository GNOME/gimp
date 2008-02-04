/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "display/gimpdisplay.h"

#include "gimpgegltool.h"
#include "gimpimagemapoptions.h"

#include "gimp-intl.h"


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
                _("GEGL Tool: Use an Abritrary GEGL Operation"),
                N_("_GEGL Operation..."), NULL,
                NULL, "foo", /* GIMP_HELP_TOOL_GEGL, */
                GIMP_STOCK_WILBER_EEK,
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

  im_tool_class->shell_desc    = _("GEGL Operation");

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
  GimpDrawable *drawable = gimp_image_get_active_drawable (display->image);

  if (! drawable)
    return FALSE;

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error (error, 0, 0,
                   _("GEGL operations do not operate on indexed layers."));
      return FALSE;
    }

  if (g_tool->config)
    gimp_config_reset (GIMP_CONFIG (g_tool->config));

  GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error);

  return TRUE;
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

  pspecs = gegl_list_properties (tool->operation, &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = { 0, };

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (tool->config), pspec->name, &value);
      gegl_node_set_property (image_map_tool->operation, pspec->name, &value);

      g_value_unset (&value);
    }

  g_free (pspecs);
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
  GtkWidget       *hbox;
  GtkWidget       *label;
  GtkWidget       *combo;
  gchar          **operations;
  guint            n_operations;
  gint             i;

  /*  The operation combo box  */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), hbox,
                      FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Operation:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = gtk_list_store_new (1, G_TYPE_STRING);

  operations = gegl_list_operations (&n_operations);

  for (i = 0; i < n_operations; i++)
    gtk_list_store_insert_with_values (store, NULL, -1,
                                       0, operations[i],
                                       -1);

  g_free (operations);

  combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell,
                                 "text", 0);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  g_object_unref (store);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_gegl_tool_operation_changed),
                    tool);

  tool->operation_combo = combo;

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

  /*  The options vbox  */
  tool->options_box = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (image_map_tool->main_vbox), tool->options_box,
                      FALSE, FALSE, 0);
  gtk_widget_show (tool->options_box);
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

static GimpObject *
gimp_gegl_tool_get_config (GimpGeglTool *tool)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  gint         i;

  pspecs = gegl_list_properties (tool->operation, &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      g_print ("property: %s\n", pspecs[i]->name);
    }

  g_free (pspecs);

  return NULL;
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
                      0, &tool->operation,
                      -1);

  if (! tool->operation)
    return;

  gegl_node_set (GIMP_IMAGE_MAP_TOOL (tool)->operation,
                 "operation", tool->operation,
                 NULL);

  tool->config = gimp_gegl_tool_get_config (tool);

  if (tool->config)
    g_signal_connect_object (tool->config, "notify",
                             G_CALLBACK (gimp_gegl_tool_config_notify),
                             G_OBJECT (tool), 0);

  gimp_image_map_tool_preview (GIMP_IMAGE_MAP_TOOL (tool));
}
