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

#include "libgimp-intl.h"

/*
 * This is a complement of libgimpwidgets/gimppropwidgets.c
 * These are property functions for types from libgimp, such as
 * [class@Gimp.Resource] or [class@Gimp.Item] subclasses.
 */


typedef GtkWidget* (*GimpResourceWidgetCreator) (const gchar  *title,
                                                 const gchar  *label,
                                                 GimpResource *initial_resource);


/*  utility function prototypes  */

static GtkWidget  * gimp_prop_resource_chooser_factory   (GimpResourceWidgetCreator  widget_creator_func,
                                                          GObject                   *config,
                                                          const gchar               *property_name,
                                                          const gchar               *chooser_title);
static gchar      * gimp_utils_make_canonical_menu_label (const gchar               *path);


/**
 * gimp_prop_brush_chooser_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of a [class@Gimp.Brush] property.
 * @chooser_title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@GimpUi.BrushChooser] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@GimpUi.BrushChooser].
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
 * @chooser_title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@GimpUi.FontChooser] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@GimpUi.FontChooser].
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
 * @chooser_title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@GimpUi.GradientChooser] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@GimpUi.GradientChooser].
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
 * @chooser_title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@GimpUi.PaletteChooser] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@GimpUi.PaletteChooser].
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
 * @chooser_title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@GimpUi.PatternChooser] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@GimpUi.PatternChooser].
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

/**
 * gimp_prop_drawable_chooser_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of a [class@Gimp.Drawable] property.
 * @chooser_title: (nullable): title for the poppable dialog.
 *
 * Creates a [class@GimpUi.DrawableChooser] controlled by the specified property.
 *
 * Returns: (transfer full): A new [class@GimpUi.DrawableChooser].
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_drawable_chooser_new (GObject     *config,
                                const gchar *property_name,
                                const gchar *chooser_title)
{
  GParamSpec   *param_spec;
  GtkWidget    *prop_chooser;
  GimpDrawable *initial_drawable = NULL;
  gchar        *title            = NULL;
  const gchar  *label;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  g_return_val_if_fail (param_spec != NULL, NULL);
  g_return_val_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), G_TYPE_PARAM_OBJECT) &&
                        g_type_is_a (param_spec->value_type, GIMP_TYPE_DRAWABLE), NULL);

  g_object_get (config,
                property_name, &initial_drawable,
                NULL);

  label = g_param_spec_get_nick (param_spec);

  if (chooser_title == NULL)
    {
      gchar *canonical;

      canonical = gimp_utils_make_canonical_menu_label (label);
      if (g_type_is_a (param_spec->value_type, GIMP_TYPE_LAYER))
        title = g_strdup_printf (_("Choose layer: %s"), canonical);
      if (g_type_is_a (param_spec->value_type, GIMP_TYPE_CHANNEL))
        title = g_strdup_printf (_("Choose channel: %s"), canonical);
      else
        title = g_strdup_printf (_("Choose drawable: %s"), canonical);
      g_free (canonical);
    }
  else
    {
      title = g_strdup (chooser_title);
    }

  prop_chooser = gimp_drawable_chooser_new (title, label, param_spec->value_type, initial_drawable);
  g_clear_object (&initial_drawable);
  g_free (title);

  g_object_bind_property (prop_chooser, "drawable",
                          config,       property_name,
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  return prop_chooser;
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
  g_clear_object (&initial_resource);

  g_object_bind_property (prop_chooser, "resource",
                          config,       property_name,
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  return prop_chooser;
}

/* This is a copy of the similarly-named function in app/widgets/gimpwidgets-utils.c
 * I hesitated to put this maybe in libgimpwidgets/gimpwidgetsutils.h but for
 * now, let's not. If it's useful to more people, it's always easier to move the
 * function in rather than deprecating it.
 */
static gchar *
gimp_utils_make_canonical_menu_label (const gchar *path)
{
  gchar **split_path;
  gchar  *canon_path;

  /* The first underscore of each path item is a mnemonic. */
  split_path = g_strsplit (path, "_", 2);
  canon_path = g_strjoinv ("", split_path);
  g_strfreev (split_path);

  return canon_path;
}
