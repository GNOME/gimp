/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppropwidgets.c
 * Copyright (C) 2023 Jehan
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "libgimp/gimpui.h"

/**
 * SECTION: gimppropwidgets
 * @title: GimpPropWidgets
 * @short_description: Editable views on #GObject properties.
 *
 * Editable views on #GObject properties of types from libgimp, such as
 * [class@Gimp.Resource] or [class@Gimp.Item] subclasses.
 **/


typedef GtkWidget* (*GimpResourceWidgetCreator) (const gchar  *title,
                                                 const gchar  *label,
                                                 GimpResource *initial_resource);


/*  utility function prototypes  */

static GtkWidget  * gimp_prop_resource_chooser_factory  (GimpResourceWidgetCreator  widget_creator_func,
                                                         GObject                   *config,
                                                         const gchar               *property_name,
                                                         const gchar               *chooser_title);


/**
 * gimp_prop_brush_chooser_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of a [class@Gimp.Brush] property.
 * @title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@Gimp.BrushSelectButton] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@Gimp.BrushSelectButton].
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_brush_chooser_new (GObject     *config,
                             const gchar *property_name,
                             const gchar *chooser_title)
{
  return gimp_prop_resource_chooser_factory (gimp_brush_chooser_new,
                                             config, property_name, chooser_title);
}

/**
 * gimp_prop_font_chooser_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of a [class@Gimp.Font] property.
 * @title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@Gimp.FontSelectButton] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@Gimp.FontSelectButton].
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_font_chooser_new (GObject     *config,
                            const gchar *property_name,
                            const gchar *chooser_title)
{
  return gimp_prop_resource_chooser_factory (gimp_font_chooser_new,
                                             config, property_name, chooser_title);
}

/**
 * gimp_prop_gradient_chooser_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of a [class@Gimp.Gradient] property.
 * @title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@Gimp.GradientSelectButton] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@Gimp.GradientSelectButton].
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_gradient_chooser_new (GObject     *config,
                                const gchar *property_name,
                                const gchar *chooser_title)
{
  return gimp_prop_resource_chooser_factory (gimp_gradient_chooser_new,
                                             config, property_name, chooser_title);
}

/**
 * gimp_prop_palette_chooser_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of a [class@Gimp.Palette] property.
 * @title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@Gimp.PaletteSelectButton] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@Gimp.PaletteSelectButton].
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_palette_chooser_new (GObject     *config,
                               const gchar *property_name,
                               const gchar *chooser_title)
{
  return gimp_prop_resource_chooser_factory (gimp_palette_chooser_new,
                                             config, property_name, chooser_title);
}

/**
 * gimp_prop_pattern_chooser_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of a [class@Gimp.Pattern] property.
 * @title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@Gimp.PatternSelectButton] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@Gimp.PatternSelectButton].
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_pattern_chooser_new (GObject     *config,
                               const gchar *property_name,
                               const gchar *chooser_title)
{
  return gimp_prop_resource_chooser_factory (gimp_pattern_chooser_new,
                                             config, property_name, chooser_title);
}


/*******************************/
/*  private utility functions  */
/*******************************/

static GtkWidget *
gimp_prop_resource_chooser_factory (GimpResourceWidgetCreator  widget_creator_func,
                                    GObject                   *config,
                                    const gchar               *property_name,
                                    const gchar               *chooser_title)
{
  GParamSpec   *param_spec;
  GtkWidget    *prop_chooser;
  GimpResource *initial_resource;
  const gchar  *label;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  g_return_val_if_fail (param_spec != NULL, NULL);
  g_return_val_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_OBJECT) &&
                        g_type_is_a (param_spec->value_type, GIMP_TYPE_RESOURCE), NULL);

  g_object_get (config,
                property_name, &initial_resource,
                NULL);

  label = g_param_spec_get_nick (param_spec);

  /* Create the wrapped widget. For example, call gimp_font_chooser_new.
   * When initial_resource is NULL, the widget creator will set it's resource
   * property from context.
   */
  prop_chooser = widget_creator_func (chooser_title, label, initial_resource);
  g_object_unref (initial_resource);

  g_object_bind_property (prop_chooser, "resource",
                          config,       property_name,
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  return prop_chooser;
}
