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

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppropwidgets.h"

#include "gimpcoloroptions.h"
#include "gimpgegltool.h"

#include "gimp-intl.h"


enum
{
  COLUMN_NAME,
  COLUMN_LABEL,
  COLUMN_ICON_NAME,
  N_COLUMNS
};


/*  local function prototypes  */

static void   gimp_gegl_tool_dialog            (GimpImageMapTool  *im_tool);

static void   gimp_gegl_tool_operation_changed (GtkWidget         *widget,
                                                GimpGeglTool      *tool);


G_DEFINE_TYPE (GimpGeglTool, gimp_gegl_tool, GIMP_TYPE_OPERATION_TOOL)

#define parent_class gimp_gegl_tool_parent_class


void
gimp_gegl_tool_register (GimpToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (GIMP_TYPE_GEGL_TOOL,
                GIMP_TYPE_COLOR_OPTIONS,
                gimp_color_options_gui,
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
  GimpImageMapToolClass *im_tool_class = GIMP_IMAGE_MAP_TOOL_CLASS (klass);

  im_tool_class->dialog_desc = _("GEGL Operation");

  im_tool_class->dialog      = gimp_gegl_tool_dialog;
}

static void
gimp_gegl_tool_init (GimpGeglTool *tool)
{
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
    "display",
    "hidden",
    "input",
    "output",
    "programming",
    "transform",
    "video"
  };
  static const gchar * const name_blacklist[] =
  {
    /* these ops are already added to the menus via
     * filters-actions or drawable-actions
     */
    "gegl:alien-map",
    "gegl:antialias",
    "gegl:apply-lens",
    "gegl:bump-map",
    "gegl:c2g",
    "gegl:cartoon",
    "gegl:cell-noise",
    "gegl:channel-mixer",
    "gegl:checkerboard",
    "gegl:color",
    "gegl:color-reduction",
    "gegl:color-temperature",
    "gegl:color-to-alpha",
    "gegl:convolution-matrix",
    "gegl:cubism",
    "gegl:deinterlace",
    "gegl:difference-of-gaussians",
    "gegl:dropshadow",
    "gegl:edge-laplace",
    "gegl:edge-sobel",
    "gegl:emboss",
    "gegl:exposure",
    "gegl:fractal-trace",
    "gegl:gaussian-blur",
    "gegl:grid",
    "gegl:invert-linear",
    "gegl:invert-gamma",
    "gegl:lens-distortion",
    "gegl:mono-mixer",
    "gegl:mosaic",
    "gegl:motion-blur-circular",
    "gegl:motion-blur-linear",
    "gegl:motion-blur-zoom",
    "gegl:noise-cie-lch",
    "gegl:noise-hsv",
    "gegl:noise-hurl",
    "gegl:noise-pick",
    "gegl:noise-reduction",
    "gegl:noise-rgb",
    "gegl:noise-slur",
    "gegl:noise-spread",
    "gegl:panorama-projection",
    "gegl:perlin-noise",
    "gegl:photocopy",
    "gegl:pixelize",
    "gegl:plasma",
    "gegl:polar-coordinates",
    "gegl:red-eye-removal",
    "gegl:ripple",
    "gegl:simplex-noise",
    "gegl:shift",
    "gegl:softglow",
    "gegl:stretch-contrast",
    "gegl:stretch-contrast-hsv",
    "gegl:texturize-canvas",
    "gegl:tile-glass",
    "gegl:tile-seamless",
    "gegl:unsharp-mask",
    "gegl:value-invert",
    "gegl:vignette",
    "gegl:waves",
    "gegl:whirl-pinch",
    "gegl:wind",

    /* these ops are blacklisted for other reasons */
    "gegl:contrast-curve",
    "gegl:convert-format", /* pointless */
    "gegl:fill-path",
    "gegl:grey", /* we use gimp's op */
    "gegl:hstack", /* pointless */
    "gegl:introspect", /* pointless */
    "gegl:layer", /* we use gimp's ops */
    "gegl:matting-global", /* used in the foreground select tool */
    "gegl:opacity", /* poinless */
    "gegl:path",
    "gegl:posterize", /* we use gimp's op */
    "gegl:seamless-clone", /* used in the seamless clone tool */
    "gegl:text", /* we use gimp's text rendering */
    "gegl:threshold", /* we use gimp's op */
    "gegl:tile", /* pointless */
    "gegl:vector-stroke",
  };

  gchar **categories;
  gint    i;

  /* Operations with no name are abstract base classes */
  if (! name)
    return TRUE;

  if (g_str_has_prefix (name, "gimp"))
    return TRUE;

  for (i = 0; i < G_N_ELEMENTS (name_blacklist); i++)
    {
      if (! strcmp (name, name_blacklist[i]))
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
  const gchar        *categories;
  guint               n_ops;
  gint                i;

  if (! type)
    return classes;

  klass = GEGL_OPERATION_CLASS (g_type_class_ref (type));
  ops = g_type_children (type, &n_ops);

  categories = gegl_operation_class_get_key (klass, "categories");

  if (! gimp_gegl_tool_operation_blacklisted (klass->name, categories))
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
  GimpGeglTool      *tool   = GIMP_GEGL_TOOL (image_map_tool);
  GimpOperationTool *o_tool = GIMP_OPERATION_TOOL (image_map_tool);
  GtkListStore      *store;
  GtkCellRenderer   *cell;
  GtkWidget         *main_vbox;
  GtkWidget         *hbox;
  GtkWidget         *combo;
  GList             *opclasses;
  GList             *iter;

  GIMP_IMAGE_MAP_TOOL_CLASS (parent_class)->dialog (image_map_tool);

  main_vbox = gimp_image_map_tool_dialog_get_vbox (image_map_tool);

  /*  The operation combo box  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), hbox, 0);
  gtk_widget_show (hbox);

  store = gtk_list_store_new (N_COLUMNS,
                              G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

  opclasses = gimp_get_geglopclasses ();

  for (iter = opclasses; iter; iter = iter->next)
    {
      GeglOperationClass *opclass = GEGL_OPERATION_CLASS (iter->data);
      const gchar        *icon_name;
      const gchar        *label;

      if (g_str_has_prefix (opclass->name, "gegl:"))
        {
          label     = opclass->name + strlen ("gegl:");
          icon_name = GIMP_STOCK_GEGL;
        }
      else
        {
          label     = opclass->name;
          icon_name = NULL;
        }

      gtk_list_store_insert_with_values (store, NULL, -1,
                                         COLUMN_NAME,      opclass->name,
                                         COLUMN_LABEL,     label,
                                         COLUMN_ICON_NAME, icon_name,
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
                                 "icon-name", COLUMN_ICON_NAME);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), cell,
                                 "text", COLUMN_LABEL);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_gegl_tool_operation_changed),
                    tool);

  tool->operation_combo = combo;

  tool->description_label = gtk_label_new ("");
  gtk_label_set_line_wrap (GTK_LABEL (tool->description_label), TRUE);
  gtk_misc_set_alignment (GTK_MISC (tool->description_label), 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (main_vbox), tool->description_label,
                      FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (main_vbox), tool->description_label, 1);

  /*  The options vbox  */
  o_tool->options_gui =
    gtk_label_new (_("Select an operation from the list above"));
  gimp_label_set_attributes (GTK_LABEL (o_tool->options_gui),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_misc_set_padding (GTK_MISC (o_tool->options_gui), 0, 4);
  gtk_container_add (GTK_CONTAINER (o_tool->options_box),
                     o_tool->options_gui);
  gtk_widget_show (o_tool->options_gui);
}

static void
gimp_gegl_tool_operation_changed (GtkWidget    *widget,
                                  GimpGeglTool *tool)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *operation;

  if (! gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

  gtk_tree_model_get (model, &iter,
                      COLUMN_NAME, &operation,
                      -1);

  if (operation)
    {
      const gchar *description;

      description = gegl_operation_get_key (operation, "description");

      if (description)
        {
          gtk_label_set_text (GTK_LABEL (tool->description_label), description);
          gtk_widget_show (tool->description_label);
        }
      else
        {
          gtk_widget_hide (tool->description_label);
        }

      gimp_operation_tool_set_operation (GIMP_OPERATION_TOOL (tool),
                                         operation, NULL, NULL);
      g_free (operation);
    }
}
