/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppropwidgets.c
 * Copyright (C) 2002-2007  Michael Natterer <mitch@gimp.org>
 *                          Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gegl-paramspecs.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "gimpwidgets.h"
#include "gimpwidgets-private.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimppropwidgets
 * @title: GimpPropWidgets
 * @short_description: Editable views on #GObject properties.
 *
 * Editable views on #GObject properties.
 **/


/*  utility function prototypes  */

static void         set_param_spec                      (GObject     *object,
                                                         GtkWidget   *widget,
                                                         GParamSpec  *param_spec);
static void         set_radio_spec                      (GObject     *object,
                                                         GParamSpec  *param_spec);
static GParamSpec * get_param_spec                      (GObject     *object);

static GParamSpec * find_param_spec                     (GObject     *object,
                                                         const gchar *property_name,
                                                         const gchar *strloc);
static GParamSpec * check_param_spec_quiet              (GObject     *object,
                                                         const gchar *property_name,
                                                         GType        type,
                                                         const gchar *strloc);
static GParamSpec * check_param_spec                    (GObject     *object,
                                                         const gchar *property_name,
                                                         GType        type,
                                                         const gchar *strloc);
static GParamSpec * check_param_spec_w                  (GObject     *object,
                                                         const gchar *property_name,
                                                         GType        type,
                                                         const gchar *strloc);
static GParamSpec * check_param_specs_w                 (GObject     *object,
                                                         const gchar *property_name,
                                                         GType        type1,
                                                         GType        type2,
                                                         const gchar *strloc);

static gboolean     get_numeric_values                  (GObject     *object,
                                                         GParamSpec  *param_spec,
                                                         gdouble     *value,
                                                         gdouble     *lower,
                                                         gdouble     *upper,
                                                         const gchar *strloc);

static void         connect_notify                      (GObject     *config,
                                                         const gchar *property_name,
                                                         GCallback    callback,
                                                         gpointer     callback_data);

static gboolean     gimp_prop_widget_double_to_factor   (GBinding     *binding,
                                                         const GValue *from_value,
                                                         GValue       *to_value,
                                                         gpointer      user_data);
static gboolean     gimp_prop_widget_double_from_factor (GBinding     *binding,
                                                         const GValue *from_value,
                                                         GValue       *to_value,
                                                         gpointer      user_data);


/******************/
/*  check button  */
/******************/

/**
 * gimp_prop_check_button_new:
 * @config:            Object to which property is attached.
 * @property_name:     Name of boolean property controlled by checkbutton.
 * @label: (nullable): Label to give checkbutton (including mnemonic).
 *
 * Creates a #GtkCheckButton that displays and sets the specified
 * boolean property.
 * If @label is %NULL, the @property_name's nick will be used as label
 * of the returned button.
 *
 * Returns: (transfer full): The newly created #GtkCheckButton widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_check_button_new (GObject     *config,
                            const gchar *property_name,
                            const gchar *label)
{
  GParamSpec  *param_spec;
  GtkWidget   *button;
  const gchar *blurb;
  GtkWidget   *label_widget;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! label)
    label = g_param_spec_get_nick (param_spec);

  button = gtk_check_button_new_with_mnemonic (label);
  gtk_widget_set_visible (button, TRUE);

  /* Resize the label to its bold size to avoid a GUI twitch */
  label_widget = gtk_bin_get_child (GTK_BIN (button));
  if (label_widget)
    {
      GtkWidget      *temp_label = gtk_label_new (label);
      GtkRequisition  natural_size;

      gtk_widget_set_visible (temp_label, TRUE);

      gimp_label_set_attributes (GTK_LABEL (temp_label),
                                 PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                                 -1);

      gtk_widget_get_preferred_size (temp_label, NULL, &natural_size);
      gtk_widget_destroy (temp_label);

      gtk_widget_set_size_request (label_widget, natural_size.width, -1);
    }

  blurb = g_param_spec_get_blurb (param_spec);
  if (blurb)
    gimp_help_set_help_data (button, blurb, NULL);

  g_object_bind_property (config, property_name,
                          button, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (button, config, property_name);

  return button;
}


static void   gimp_prop_enum_check_button_callback (GtkWidget  *widget,
                                                    GObject    *config);
static void   gimp_prop_enum_check_button_notify   (GObject    *config,
                                                    GParamSpec *param_spec,
                                                    GtkWidget  *button);

/**
 * gimp_prop_enum_check_button_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of enum property controlled by checkbutton.
 * @label: (nullable): Label to give checkbutton (including mnemonic).
 * @false_value:   Enum value corresponding to unchecked state.
 * @true_value:    Enum value corresponding to checked state.
 *
 * Creates a #GtkCheckButton that displays and sets the specified
 * property of type Enum.  Note that this widget only allows two values
 * for the enum, one corresponding to the "checked" state and the
 * other to the "unchecked" state.
 * If @label is %NULL, the @property_name's nick will be used as label
 * of the returned button.
 *
 * Returns: (transfer full): The newly created #GtkCheckButton widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_enum_check_button_new (GObject     *config,
                                 const gchar *property_name,
                                 const gchar *label,
                                 gint         false_value,
                                 gint         true_value)
{
  GParamSpec  *param_spec;
  GtkWidget   *button;
  gint        value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! label)
    label = g_param_spec_get_nick (param_spec);

  g_object_get (config,
                property_name, &value,
                NULL);

  button = gtk_check_button_new_with_mnemonic (label);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                value == true_value);

  if (value != false_value && value != true_value)
    gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (button), TRUE);

  set_param_spec (G_OBJECT (button), button, param_spec);

  g_object_set_data (G_OBJECT (button), "false-value",
                     GINT_TO_POINTER (false_value));
  g_object_set_data (G_OBJECT (button), "true-value",
                     GINT_TO_POINTER (true_value));

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_prop_enum_check_button_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_enum_check_button_notify),
                  button);

  gtk_widget_show (button);

  gimp_widget_set_bound_property (button, config, property_name);

  return button;
}

static void
gimp_prop_enum_check_button_callback (GtkWidget *widget,
                                      GObject   *config)
{
  GParamSpec *param_spec;
  gint        false_value;
  gint        true_value;
  gint        value;
  gint        v;

  param_spec = get_param_spec (G_OBJECT (widget));
  if (! param_spec)
    return;

  false_value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                    "false-value"));
  true_value  = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                    "true-value"));

  value = (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) ?
           true_value : false_value);

  g_object_get (config, param_spec->name, &v, NULL);

  if (v != value)
    {
      g_object_set (config, param_spec->name, value, NULL);

      gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (widget), FALSE);
    }
}

static void
gimp_prop_enum_check_button_notify (GObject    *config,
                                    GParamSpec *param_spec,
                                    GtkWidget  *button)
{
  gint     value;
  gint     false_value;
  gint     true_value;
  gboolean active       = FALSE;
  gboolean inconsistent = FALSE;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  false_value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                                    "false-value"));
  true_value  = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                                    "true-value"));

  if (value == true_value)
    active = TRUE;
  else if (value != false_value)
    inconsistent = TRUE;

  gtk_toggle_button_set_inconsistent (GTK_TOGGLE_BUTTON (button),
                                      inconsistent);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)) != active)
    {
      g_signal_handlers_block_by_func (button,
                                       gimp_prop_enum_check_button_callback,
                                       config);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);

      g_signal_handlers_unblock_by_func (button,
                                         gimp_prop_enum_check_button_callback,
                                         config);
    }
}


/******************/
/*     switch     */
/******************/


/**
 * gimp_prop_switch_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of boolean property controlled by checkbutton.
 * @label:         Label to give checkbutton (including mnemonic).
 * @label_out: (out) (optional) (transfer none): The generated #GtkLabel
 * @switch_out: (out) (optional) (transfer none): The generated #GtkSwitch
 *
 * Creates a #GtkBox with a switch and a label that displays and sets the
 * specified boolean property.
 * If @label is %NULL, the @property_name's nick will be used as label.
 *
 * Returns: (transfer full): The newly created box containing a #GtkSwitch.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_switch_new (GObject     *config,
                      const gchar *property_name,
                      const gchar *label,
                      GtkWidget  **label_out,
                      GtkWidget  **switch_out)
{
  GParamSpec  *param_spec;
  const gchar *tooltip;
  GtkWidget   *plabel;
  GtkWidget   *pswitch;
  GtkWidget   *hbox;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! label)
    label = g_param_spec_get_nick (param_spec);

  tooltip = g_param_spec_get_blurb (param_spec);

  pswitch = gtk_switch_new ();
  g_object_bind_property (config, property_name, pswitch, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  gimp_help_set_help_data (pswitch, tooltip, NULL);
  gtk_widget_show (pswitch);

  plabel = gtk_label_new_with_mnemonic (label);
  gtk_label_set_xalign (GTK_LABEL (plabel), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (plabel), pswitch);
  gimp_help_set_help_data (plabel, tooltip, NULL);
  gtk_widget_show (plabel);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), plabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), pswitch, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  if (label_out)
    *label_out = plabel;
  if (switch_out)
    *switch_out = pswitch;

  gimp_widget_set_bound_property (hbox, config, property_name);

  return hbox;
}

/**
 * gimp_prop_toggle_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of boolean property controlled by the toggle button.
 * @label:         Label to give the toggle (including mnemonic).
 * @icon_name:     Icon to display in the toggle.
 * @image_out: (out) (optional) (transfer none): The generated #GtkImage
 *                 if @icon_name was not %NULL.
 *
 * Creates a [class@Gtk.ToggleButton] that sets the specified boolean
 * property.
 *
 * If @icon_name is %NULL, @label will be used. If @label is %NULL too,
 * the @property_name's nick will be used as label.
 *
 * Returns: (transfer full): The newly #GtkToggleButton.
 *
 * Since: 3.2
 */
GtkWidget *
gimp_prop_toggle_new (GObject      *config,
                      const gchar  *property_name,
                      const gchar  *icon_name,
                      const gchar  *label,
                      GtkWidget   **image_out)
{
  GParamSpec  *param_spec;
  const gchar *tooltip;
  GtkWidget   *button;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (icon_name == NULL && label == NULL)
    label = g_param_spec_get_nick (param_spec);

  tooltip = g_param_spec_get_blurb (param_spec);

  if (label != NULL)
    button = gtk_toggle_button_new_with_mnemonic (label);
  else
    button = gtk_toggle_button_new ();

  g_object_bind_property (config, property_name, button, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  gimp_help_set_help_data (button, tooltip, NULL);
  gtk_widget_show (button);

  if (image_out)
    *image_out = NULL;

  if (label == NULL)
    {
      GtkWidget *image;

      g_return_val_if_fail (icon_name != NULL, NULL);

      image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_show (image);

      if (image_out)
        *image_out = image;
    }

  gimp_widget_set_bound_property (button, config, property_name);

  return button;
}



/*************************/
/*  int/enum combo box   */
/*************************/

static void gimp_prop_int_combo_box_callback     (GtkWidget  *widget,
                                                  GObject    *config);
static void gimp_prop_int_combo_box_notify       (GObject    *config,
                                                  GParamSpec *param_spec,
                                                  GtkWidget  *widget);

static void gimp_prop_pointer_combo_box_callback (GtkWidget  *widget,
                                                  GObject    *config);
static void gimp_prop_pointer_combo_box_notify   (GObject    *config,
                                                  GParamSpec *param_spec,
                                                  GtkWidget  *combo_box);

/**
 * gimp_prop_int_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of int property controlled by combo box.
 * @store:         #GimpIntStore holding list of labels, values, etc.
 *
 * Creates a #GimpIntComboBox widget to display and set the specified
 * property.  The contents of the widget are determined by @store,
 * which should be created using gimp_int_store_new().
 *
 * Returns: (transfer full): The newly created #GimpIntComboBox widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_int_combo_box_new (GObject      *config,
                             const gchar  *property_name,
                             GimpIntStore *store)
{
  GParamSpec  *param_spec;
  GtkWidget   *combo_box;
  const gchar *blurb;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  /* Require property is integer valued type: INT or ENUM, and is writeable. */
  param_spec = check_param_specs_w (config, property_name, G_TYPE_PARAM_INT,
                                    G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX,
                            "model", store,
                            "visible", TRUE,
                            NULL);

  blurb = g_param_spec_get_blurb (param_spec);
  if (blurb)
    gimp_help_set_help_data (combo_box, blurb, NULL);

  g_object_bind_property (config, property_name,
                          combo_box, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (combo_box, config, property_name);

  return combo_box;
}

/**
 * gimp_prop_pointer_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of GType/gpointer property controlled by combo box.
 * @store:         #GimpIntStore holding list of labels, values, etc.
 *
 * Creates a #GimpIntComboBox widget to display and set the specified
 * property.  The contents of the widget are determined by @store,
 * which should be created using gimp_int_store_new().
 * Values are GType/gpointer data, and therefore must be stored in the
 * "user-data" column, instead of the usual "value" column.
 *
 * Returns: (transfer full): The newly created #GimpIntComboBox widget.
 *
 * Since: 2.10
 */
GtkWidget *
gimp_prop_pointer_combo_box_new (GObject      *config,
                                 const gchar  *property_name,
                                 GimpIntStore *store)
{
  GParamSpec *param_spec;
  GtkWidget  *combo_box;
  gpointer    property_value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_GTYPE, G_STRFUNC);
  if (! param_spec)
    {
      param_spec = check_param_spec_w (config, property_name,
                                       G_TYPE_PARAM_POINTER, G_STRFUNC);
      if (! param_spec)
        return NULL;
    }

  g_object_get (config,
                property_name, &property_value,
                NULL);

  /* We use a GimpIntComboBox but we cannot store gpointer in the
   * "value" column, because gpointer is not a subset of gint. Instead
   * we store the value in the "user-data" column which is a gpointer.
   */
  combo_box = g_object_new (GIMP_TYPE_INT_COMBO_BOX,
                            "model", store,
                            NULL);

  gimp_int_combo_box_set_active_by_user_data (GIMP_INT_COMBO_BOX (combo_box),
                                              property_value);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (gimp_prop_pointer_combo_box_callback),
                    config);

  set_param_spec (G_OBJECT (combo_box), combo_box, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_pointer_combo_box_notify),
                  combo_box);

  gimp_widget_set_bound_property (combo_box, config, property_name);

  gtk_widget_show (combo_box);

  return combo_box;
}

/**
 * gimp_prop_enum_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of enum property controlled by combo box.
 * @minimum:       Smallest allowed value of enum.
 * @maximum:       Largest allowed value of enum.
 *
 * Creates a #GimpIntComboBox widget to display and set the specified
 * enum property.  The @mimimum_value and @maximum_value give the
 * possibility of restricting the allowed range to a subset of the
 * enum.  If the two values are equal (e.g., 0, 0), then the full
 * range of the Enum is used.
 *
 * Returns: (transfer full): The newly created #GimpEnumComboBox widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_enum_combo_box_new (GObject     *config,
                              const gchar *property_name,
                              gint         minimum,
                              gint         maximum)
{
  GParamSpec   *param_spec;
  GtkListStore *store = NULL;
  GtkWidget    *combo_box;
  gint          value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  if (minimum != maximum)
    {
      store = gimp_enum_store_new_with_range (param_spec->value_type,
                                              minimum, maximum);
    }
  else if (param_spec->value_type == GIMP_TYPE_DESATURATE_MODE)
    {
      /* this is a bad hack, if we get more of those, we should probably
       * think of something less ugly
       */
      store = gimp_enum_store_new_with_values (param_spec->value_type,
                                               5,
                                               GIMP_DESATURATE_LUMINANCE,
                                               GIMP_DESATURATE_LUMA,
                                               GIMP_DESATURATE_LIGHTNESS,
                                               GIMP_DESATURATE_AVERAGE,
                                               GIMP_DESATURATE_VALUE);
    }

  if (store)
    {
      combo_box = g_object_new (GIMP_TYPE_ENUM_COMBO_BOX,
                                "model", store,
                                NULL);
      g_object_unref (store);
    }
  else
    {
      combo_box = gimp_enum_combo_box_new (param_spec->value_type);
    }

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo_box), value);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (gimp_prop_int_combo_box_callback),
                    config);

  set_param_spec (G_OBJECT (combo_box), combo_box, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_int_combo_box_notify),
                  combo_box);

  gtk_widget_show (combo_box);

  gimp_widget_set_bound_property (combo_box, config, property_name);

  return combo_box;
}

static void
gimp_prop_int_combo_box_callback (GtkWidget *widget,
                                  GObject   *config)
{
  GParamSpec  *param_spec;
  gint         value;

  param_spec = get_param_spec (G_OBJECT (widget));
  if (! param_spec)
    return;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value))
    {
      gint v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
gimp_prop_int_combo_box_notify (GObject    *config,
                                GParamSpec *param_spec,
                                GtkWidget  *combo_box)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo_box,
                                   gimp_prop_int_combo_box_callback,
                                   config);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo_box), value);

  g_signal_handlers_unblock_by_func (combo_box,
                                     gimp_prop_int_combo_box_callback,
                                     config);
}

static void
gimp_prop_pointer_combo_box_callback (GtkWidget *widget,
                                      GObject   *config)
{
  GParamSpec *param_spec;
  gpointer    value;

  param_spec = get_param_spec (G_OBJECT (widget));
  if (! param_spec)
    return;

  if (gimp_int_combo_box_get_active_user_data (GIMP_INT_COMBO_BOX (widget),
                                               &value))
    {
      gpointer v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
gimp_prop_pointer_combo_box_notify (GObject    *config,
                                    GParamSpec *param_spec,
                                    GtkWidget  *combo_box)
{
  gpointer value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo_box,
                                   gimp_prop_pointer_combo_box_callback,
                                   config);

  gimp_int_combo_box_set_active_by_user_data (GIMP_INT_COMBO_BOX (combo_box),
                                              value);

  g_signal_handlers_unblock_by_func (combo_box,
                                     gimp_prop_pointer_combo_box_callback,
                                     config);
}

/************************/
/*  boolean combo box   */
/************************/

static void   gimp_prop_boolean_combo_box_callback (GtkWidget   *combo,
                                                    GObject     *config);
static void   gimp_prop_boolean_combo_box_notify   (GObject     *config,
                                                    GParamSpec  *param_spec,
                                                    GtkWidget   *combo);


/**
 * gimp_prop_boolean_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of boolean property controlled by combo box.
 * @true_text:     Label used for entry corresponding to %TRUE value.
 * @false_text:    Label used for entry corresponding to %FALSE value.
 *
 * Creates a #GtkComboBox widget to display and set the specified
 * boolean property.  The combo box will have two entries, one
 * displaying the @true_text label, the other displaying the
 * @false_text label.
 *
 * Returns: (transfer full): The newly created #GtkComboBox widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_boolean_combo_box_new (GObject     *config,
                                 const gchar *property_name,
                                 const gchar *true_text,
                                 const gchar *false_text)
{
  GParamSpec *param_spec;
  GtkWidget  *combo;
  gboolean    value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  combo = gimp_int_combo_box_new (true_text,  TRUE,
                                  false_text, FALSE,
                                  NULL);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), value);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_prop_boolean_combo_box_callback),
                    config);

  set_param_spec (G_OBJECT (combo), combo, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_boolean_combo_box_notify),
                  combo);

  gimp_widget_set_bound_property (combo, config, property_name);

  gtk_widget_show (combo);

  return combo;
}

static void
gimp_prop_boolean_combo_box_callback (GtkWidget *combo,
                                      GObject   *config)
{
  GParamSpec  *param_spec;
  gint         value;

  param_spec = get_param_spec (G_OBJECT (combo));
  if (! param_spec)
    return;

  if (gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &value))
    {
      gint v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
gimp_prop_boolean_combo_box_notify (GObject    *config,
                                    GParamSpec *param_spec,
                                    GtkWidget  *combo)
{
  gboolean value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo,
                                   gimp_prop_boolean_combo_box_callback,
                                   config);

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo), value);

  g_signal_handlers_unblock_by_func (combo,
                                     gimp_prop_boolean_combo_box_callback,
                                     config);
}


/*****************/
/*  radio boxes  */
/*****************/

static void  gimp_prop_radio_button_callback (GtkWidget   *widget,
                                              GObject     *config);
static void  gimp_prop_radio_button_notify   (GObject     *config,
                                              GParamSpec  *param_spec,
                                              GtkWidget   *button);


/**
 * gimp_prop_enum_radio_frame_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of enum property controlled by the radio buttons.
 * @title: (nullable): Label for the frame holding the buttons
 * @minimum:       Smallest value of enum to be included.
 * @maximum:       Largest value of enum to be included.
 *
 * Creates a group of radio buttons which function to set and display
 * the specified enum property.  The @minimum and @maximum arguments
 * allow only a subset of the enum to be used.  If the two arguments
 * are equal (e.g., 0, 0), then the full range of the enum will be used.
 * If @title is %NULL, the @property_name's nick will be used as label
 * of the returned frame.
 *
 * Returns: (transfer full): A #GimpFrame containing the radio buttons.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_enum_radio_frame_new (GObject     *config,
                                const gchar *property_name,
                                const gchar *title,
                                gint         minimum,
                                gint         maximum)
{
  GParamSpec *param_spec;
  GtkWidget  *frame;
  GtkWidget  *button;
  gint        value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! title)
    title = g_param_spec_get_nick (param_spec);

  g_object_get (config,
                property_name, &value,
                NULL);

  if (minimum != maximum)
    {
      frame = gimp_enum_radio_frame_new_with_range (param_spec->value_type,
                                                    minimum, maximum,
                                                    gtk_label_new (title),
                                                    G_CALLBACK (gimp_prop_radio_button_callback),
                                                    config, NULL,
                                                    &button);
    }
  else
    {
      frame = gimp_enum_radio_frame_new (param_spec->value_type,
                                         gtk_label_new (title),
                                         G_CALLBACK (gimp_prop_radio_button_callback),
                                         config, NULL,
                                         &button);
    }

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);

  set_radio_spec (G_OBJECT (button), param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_radio_button_notify),
                  button);

  g_object_set_data (G_OBJECT (frame), "radio-button", button);

  gtk_widget_show (frame);

  gimp_widget_set_bound_property (frame, config, property_name);

  return frame;
}

/**
 * gimp_prop_enum_radio_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of enum property controlled by the radio buttons.
 * @minimum:       Smallest value of enum to be included.
 * @maximum:       Largest value of enum to be included.
 *
 * Creates a group of radio buttons which function to set and display
 * the specified enum property.  The @minimum and @maximum arguments
 * allow only a subset of the enum to be used.  If the two arguments
 * are equal (e.g., 0, 0), then the full range of the enum will be used.
 * If you want to assign a label to the group of radio buttons, use
 * gimp_prop_enum_radio_frame_new() instead of this function.
 *
 * Returns: (transfer full): A #GtkBox containing the radio buttons.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_enum_radio_box_new (GObject     *config,
                              const gchar *property_name,
                              gint         minimum,
                              gint         maximum)
{
  GParamSpec *param_spec;
  GtkWidget  *vbox;
  GtkWidget  *button;
  gint        value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  if (minimum != maximum)
    {
      vbox = gimp_enum_radio_box_new_with_range (param_spec->value_type,
                                                 minimum, maximum,
                                                 G_CALLBACK (gimp_prop_radio_button_callback),
                                                 config, NULL,
                                                 &button);
    }
  else
    {
      vbox = gimp_enum_radio_box_new (param_spec->value_type,
                                      G_CALLBACK (gimp_prop_radio_button_callback),
                                      config, NULL,
                                      &button);
    }

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);

  set_radio_spec (G_OBJECT (button), param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_radio_button_notify),
                  button);

  g_object_set_data (G_OBJECT (vbox), "radio-button", button);

  gimp_widget_set_bound_property (vbox, config, property_name);

  gtk_widget_show (vbox);

  return vbox;
}

/**
 * gimp_prop_int_radio_frame_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of enum property controlled by the radio buttons.
 * @title: (nullable): Label for the frame holding the buttons
 * @store:         #GimpIntStore holding list of labels, values, etc.
 *
 * Creates a group of radio buttons which function to set and display
 * the specified int property. If @title is %NULL, the
 * @property_name's nick will be used as label of the returned frame.
 *
 * Returns: (transfer full): A #GimpFrame containing the radio buttons.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_int_radio_frame_new (GObject      *config,
                               const gchar  *property_name,
                               const gchar  *title,
                               GimpIntStore *store)
{
  GParamSpec  *param_spec;
  const gchar *tooltip;
  GtkWidget   *frame;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_INT_STORE (store), NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_INT, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! title)
    title = g_param_spec_get_nick (param_spec);

  tooltip = g_param_spec_get_blurb (param_spec);

  frame = gimp_int_radio_frame_new_from_store (title, store);
  g_object_bind_property (config, property_name,
                          frame, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  gimp_help_set_help_data (frame, tooltip, NULL);
  gtk_widget_show (frame);

  gimp_widget_set_bound_property (frame, config, property_name);

  return frame;
}


/***********/
/*  label  */
/***********/

static void  gimp_prop_enum_label_notify (GObject    *config,
                                          GParamSpec *param_spec,
                                          GtkWidget  *label);

/**
 * gimp_prop_enum_label_new:
 * @config:         Object to which property is attached.
 * @property_name:  Name of enum property to be displayed.
 *
 * Returns: (transfer full): The newly created #GimpEnumLabel widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_enum_label_new (GObject     *config,
                          const gchar *property_name)
{
  GParamSpec *param_spec;
  GtkWidget  *label;
  gint        value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec (config, property_name,
                                 G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  label = gimp_enum_label_new (param_spec->value_type, value);
  gtk_widget_set_halign (label, GTK_ALIGN_START);

  set_param_spec (G_OBJECT (label), label, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_enum_label_notify),
                  label);

  gimp_widget_set_bound_property (label, config, property_name);

  gtk_widget_show (label);

  return label;
}

static void
gimp_prop_enum_label_notify (GObject    *config,
                             GParamSpec *param_spec,
                             GtkWidget  *label)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  gimp_enum_label_set_value (GIMP_ENUM_LABEL (label), value);
}


/**
 * gimp_prop_boolean_radio_frame_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of boolean property controlled by the radio buttons.
 * @title: (nullable): Label for the frame.
 * @true_text:     Label for the button corresponding to %TRUE.
 * @false_text:    Label for the button corresponding to %FALSE.
 *
 * Creates a pair of radio buttons which function to set and display
 * the specified boolean property.
 * If @title is %NULL, the @property_name's nick will be used as label
 * of the returned frame.
 *
 * Returns: (transfer full): A #GimpFrame containing the radio buttons.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_boolean_radio_frame_new (GObject     *config,
                                   const gchar *property_name,
                                   const gchar *title,
                                   const gchar *true_text,
                                   const gchar *false_text)
{
  GParamSpec *param_spec;
  GtkWidget  *frame;
  GtkWidget  *button;
  gboolean    value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! title)
    title = g_param_spec_get_nick (param_spec);

  g_object_get (config,
                property_name, &value,
                NULL);

  frame =
    gimp_int_radio_group_new (TRUE, title,
                              G_CALLBACK (gimp_prop_radio_button_callback),
                              config, NULL, value,

                              false_text, FALSE, &button,
                              true_text,  TRUE,  NULL,

                              NULL);

  set_radio_spec (G_OBJECT (button), param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_radio_button_notify),
                  button);

  g_object_set_data (G_OBJECT (frame), "radio-button", button);

  gimp_widget_set_bound_property (frame, config, property_name);

  gtk_widget_show (frame);

  return frame;
}

/**
 * gimp_prop_enum_icon_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of enum property controlled by the radio buttons.
 * @icon_prefix:   The prefix of the group of icon names to use.
 * @minimum:       Smallest value of enum to be included.
 * @maximum:       Largest value of enum to be included.
 *
 * Creates a horizontal box of radio buttons with named icons, which
 * function to set and display the value of the specified Enum
 * property.  The icon name for each icon is created by appending the
 * enum_value's nick to the given @icon_prefix.  See
 * gimp_enum_icon_box_new() for more information.
 *
 * Returns: (transfer full): A #libgimpwidgets-gimpenumiconbox containing the radio buttons.
 *
 * Since: 2.10
 */
GtkWidget *
gimp_prop_enum_icon_box_new (GObject     *config,
                             const gchar *property_name,
                             const gchar *icon_prefix,
                             gint         minimum,
                             gint         maximum)
{
  GParamSpec *param_spec;
  GtkWidget  *box;
  GtkWidget  *button;
  gint        value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_ENUM, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  if (minimum != maximum)
    {
      box = gimp_enum_icon_box_new_with_range (param_spec->value_type,
                                               minimum, maximum,
                                               icon_prefix,
                                               GTK_ICON_SIZE_MENU,
                                               G_CALLBACK (gimp_prop_radio_button_callback),
                                               config, NULL,
                                               &button);
    }
  else
    {
      box = gimp_enum_icon_box_new (param_spec->value_type,
                                    icon_prefix,
                                    GTK_ICON_SIZE_MENU,
                                    G_CALLBACK (gimp_prop_radio_button_callback),
                                    config, NULL,
                                    &button);
    }

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);

  set_radio_spec (G_OBJECT (button), param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_radio_button_notify),
                  button);

  gimp_widget_set_bound_property (box, config, property_name);

  gtk_widget_show (box);

  return box;
}

static void
gimp_prop_radio_button_callback (GtkWidget *widget,
                                 GObject   *config)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GParamSpec *param_spec;
      gint        value;
      gint        v;

      param_spec = get_param_spec (G_OBJECT (widget));
      if (! param_spec)
        return;

      value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp-item-data"));

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
gimp_prop_radio_button_notify (GObject    *config,
                               GParamSpec *param_spec,
                               GtkWidget  *button)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);
}


/*****************/
/*  adjustments  */
/*****************/

static void   gimp_prop_adjustment_callback (GtkAdjustment *adjustment,
                                             GObject       *config);
static void   gimp_prop_adjustment_notify   (GObject       *config,
                                             GParamSpec    *param_spec,
                                             GtkAdjustment *adjustment);

/**
 * gimp_prop_spin_button_new:
 * @config:            Object to which property is attached.
 * @property_name:     Name of double property controlled by the spin button.
 * @step_increment:    Step size.
 * @page_increment:    Page size.
 * @digits:            Number of digits after decimal point to display.
 *
 * Creates a spin button to set and display the value of the
 * specified double property.
 *
 * If you wish to change the widget's range relatively to the
 * @property_name's range, use gimp_prop_widget_set_factor().
 *
 * Returns: (transfer full): A new #libgimpwidgets-gimpspinbutton.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_spin_button_new (GObject     *config,
                           const gchar *property_name,
                           gdouble      step_increment,
                           gdouble      page_increment,
                           gint         digits)
{
  GParamSpec    *param_spec;
  GtkWidget     *spinbutton;
  GtkAdjustment *adjustment;
  GBinding      *binding;
  gdouble        value;
  gdouble        lower;
  gdouble        upper;

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! get_numeric_values (config,
                            param_spec, &value, &lower, &upper, G_STRFUNC))
    return NULL;

  if (! G_IS_PARAM_SPEC_DOUBLE (param_spec))
    digits = 0;

  adjustment = gtk_adjustment_new (value, lower, upper,
                                   step_increment, page_increment, 0);

  spinbutton = gimp_spin_button_new (adjustment, step_increment, digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  set_param_spec (G_OBJECT (adjustment), spinbutton, param_spec);

  binding = g_object_bind_property (config,     property_name,
                                    spinbutton, "value",
                                    G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_set_data (G_OBJECT (adjustment),
                     "gimp-prop-adjustment-binding",
                     binding);

  gimp_widget_set_bound_property (spinbutton, config, property_name);

  gtk_widget_show (spinbutton);

  return spinbutton;
}

/**
 * gimp_prop_label_spin_new:
 * @config:            Object to which property is attached.
 * @digits:            Number of digits after decimal point to display.
 *
 * Creates a #GimpLabelSpin to set and display the value of the
 * specified double property.
 *
 * Returns: (transfer full): A new #libgimpwidgets-gimpspinbutton.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_label_spin_new (GObject     *config,
                          const gchar *property_name,
                          gint         digits)
{
  const gchar   *label;
  GParamSpec    *param_spec;
  GtkWidget     *widget;
  gdouble        value;
  gdouble        lower;
  gdouble        upper;

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! get_numeric_values (config,
                            param_spec, &value, &lower, &upper, G_STRFUNC))
    return NULL;

  if (! G_IS_PARAM_SPEC_DOUBLE (param_spec))
    digits = 0;

  label = g_param_spec_get_nick (param_spec);
  widget = gimp_label_spin_new (label, value, lower, upper, digits);

  g_object_bind_property (config, property_name,
                          widget, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (widget, config, property_name);

  return widget;
}

/**
 * gimp_prop_spin_scale_new:
 * @config:         Object to which property is attached.
 * @property_name:  Name of double or int property controlled by the
 *                  spin button.
 * @step_increment: Step size.
 * @page_increment: Page size.
 * @digits:         Number of digits after decimal point to display.
 *                  This is only used for double properties. In case of
 *                  int properties, `digits = 0` is implied.
 *
 * Creates a spin scale to set and display the value of the specified
 * int or double property.
 *
 * By default, the @property_name's nick will be used as label of the
 * returned widget. Use gimp_spin_scale_set_label() to change this.
 *
 * If you wish to change the widget's range relatively to the
 * @property_name's range, use gimp_prop_widget_set_factor().
 *
 * Returns: (transfer full): A new #libgimpwidgets-gimpspinbutton.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_spin_scale_new (GObject     *config,
                          const gchar *property_name,
                          gdouble      step_increment,
                          gdouble      page_increment,
                          gint         digits)
{
  GParamSpec    *param_spec;
  GtkWidget     *spinscale;
  GtkAdjustment *adjustment;
  const gchar   *label;
  const gchar   *tooltip;
  GBinding      *binding;
  gdouble        value;
  gdouble        lower;
  gdouble        upper;

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! get_numeric_values (config,
                            param_spec, &value, &lower, &upper, G_STRFUNC))
    return NULL;

  if (! G_IS_PARAM_SPEC_DOUBLE (param_spec))
    digits = 0;

  adjustment = gtk_adjustment_new (value, lower, upper,
                                   step_increment, page_increment, 0);
  label = g_param_spec_get_nick (param_spec);

  spinscale = gimp_spin_scale_new (adjustment, label, digits);

  set_param_spec (G_OBJECT (adjustment), spinscale, param_spec);

  if (GEGL_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      GeglParamSpecDouble *gspec = GEGL_PARAM_SPEC_DOUBLE (param_spec);

      gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (spinscale),
                                        gspec->ui_minimum, gspec->ui_maximum);
      gimp_spin_scale_set_gamma (GIMP_SPIN_SCALE (spinscale), gspec->ui_gamma);
    }
  else if (GEGL_IS_PARAM_SPEC_INT (param_spec))
    {
      GeglParamSpecInt *gspec = GEGL_PARAM_SPEC_INT (param_spec);

      gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (spinscale),
                                        gspec->ui_minimum, gspec->ui_maximum);
      gimp_spin_scale_set_gamma (GIMP_SPIN_SCALE (spinscale), gspec->ui_gamma);
    }

  tooltip = g_param_spec_get_blurb (param_spec);
  gimp_help_set_help_data (spinscale, tooltip, NULL);

  binding = g_object_bind_property (config, property_name,
                                    spinscale, "value",
                                    G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  g_object_set_data (G_OBJECT (adjustment),
                     "gimp-prop-adjustment-binding",
                     binding);

  gtk_widget_show (spinscale);

  gimp_widget_set_bound_property (spinscale, config, property_name);

  return spinscale;
}

/**
 * gimp_prop_widget_set_factor:
 * @widget:         Property widget.
 * @factor:         Multiplier to convert the @widget's range and
 *                  map appropriately to the property's range it is
 *                  associated to.
 * @step_increment: Step size.
 * @page_increment: Page size.
 * @digits:         Number of digits after decimal point to display.
 *
 * Change the display factor of the property @widget relatively to the
 * property it was bound to. Currently the only types of widget accepted
 * as input are those created by gimp_prop_spin_scale_new() and
 * gimp_prop_spin_button_new().
 *
 * If @factor is 1.0, then the config property and the widget display
 * map exactly.
 *
 * If @factor is not 1.0, the widget's range will be computed based of
 * @property_name's range multiplied by @factor. A typical usage would
 * be to display a [0.0, 1.0] range as [0.0, 100.0] by setting 100.0 as
 * @factor. This function can only be used with double properties.
 *
 * The @step_increment and @page_increment can be set to new increments
 * you want to get for this new range. If you set them to 0.0 or
 * negative values, new increments will be computed based on the new
 * @factor and previous factor.
 *
 * Since: 3.0
 */
void
gimp_prop_widget_set_factor (GtkWidget *widget,
                             gdouble    factor,
                             gdouble    step_increment,
                             gdouble    page_increment,
                             gint       digits)
{
  GtkAdjustment *adjustment;
  GParamSpec    *param_spec;
  GBinding      *binding;
  GObject       *config;
  gchar         *property_name;
  gdouble       *factor_store;
  gdouble        old_factor = 1.0;
  gdouble        f;

  g_return_if_fail (GTK_IS_SPIN_BUTTON (widget));
  g_return_if_fail (factor != 0.0);
  g_return_if_fail (digits >= 0);

  adjustment = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));

  param_spec = get_param_spec (G_OBJECT (adjustment));
  g_return_if_fail (param_spec != NULL && G_IS_PARAM_SPEC_DOUBLE (param_spec));

  /* Get the old factor and recompute new values. */
  factor_store = g_object_get_data (G_OBJECT (adjustment),
                                    "gimp-prop-adjustment-factor");
  if (factor_store)
    {
      old_factor = *factor_store;
    }
  else
    {
      factor_store = g_new (gdouble, 1);
      g_object_set_data_full (G_OBJECT (adjustment),
                              "gimp-prop-adjustment-factor",
                              factor_store, (GDestroyNotify) g_free);
    }

  *factor_store = factor;

  f = factor / old_factor;

  if (step_increment <= 0)
    step_increment = f * gtk_adjustment_get_step_increment (adjustment);

  if (page_increment <= 0)
    page_increment = f * gtk_adjustment_get_page_increment (adjustment);

  /* Remove the old binding. */
  binding = g_object_get_data (G_OBJECT (adjustment),
                               "gimp-prop-adjustment-binding");
  g_return_if_fail (binding != NULL);
  config = g_binding_dup_source (binding);

  /* This binding should not have outlived the config object. */
  g_return_if_fail (config != NULL);

  property_name = g_strdup (g_binding_get_source_property (binding));
  g_binding_unbind (binding);

  /* Reconfigure the scale object. */
  gtk_adjustment_configure (adjustment,
                            f * gtk_adjustment_get_value (adjustment),
                            f * gtk_adjustment_get_lower (adjustment),
                            f * gtk_adjustment_get_upper (adjustment),
                            step_increment,
                            page_increment,
                            f * gtk_adjustment_get_page_size (adjustment));

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (widget), digits);

  /* Finally create a new binding. */
  if (factor != 1.0)
    {
      gdouble *user_data;

      user_data = g_new0 (gdouble, 1);
      *user_data = factor;
      /* With @factor == 1.0, this is equivalent to a
       * g_object_bind_property().
       */
      binding = g_object_bind_property_full (config, property_name,
                                             widget, "value",
                                             G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                                             gimp_prop_widget_double_to_factor,
                                             gimp_prop_widget_double_from_factor,
                                             user_data, (GDestroyNotify) g_free);
    }
  else
    {
      binding = g_object_bind_property (config, property_name,
                                        widget, "value",
                                        G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
    }
  g_object_set_data (G_OBJECT (adjustment),
                     "gimp-prop-adjustment-binding",
                     binding);
  g_object_unref (config);
  g_free (property_name);
}

/**
 * gimp_prop_hscale_new:
 * @config:         Object to which property is attached.
 * @property_name:  Name of integer or double property controlled by the scale.
 * @step_increment: Step size.
 * @page_increment: Page size.
 * @digits:         Number of digits after decimal point to display.
 *
 * Creates a horizontal scale to control the value of the specified
 * integer or double property.
 *
 * Returns: (transfer full): A new #GtkScale.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_hscale_new (GObject     *config,
                      const gchar *property_name,
                      gdouble      step_increment,
                      gdouble      page_increment,
                      gint         digits)
{
  GParamSpec    *param_spec;
  GtkWidget     *scale;
  GtkAdjustment *adjustment;
  gdouble        value;
  gdouble        lower;
  gdouble        upper;

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! get_numeric_values (config,
                            param_spec, &value, &lower, &upper, G_STRFUNC))
    return NULL;

  if (! G_IS_PARAM_SPEC_DOUBLE (param_spec))
    digits = 0;

  adjustment = gtk_adjustment_new (value, lower, upper,
                                   step_increment, page_increment, 0.0);

  scale = g_object_new (GTK_TYPE_SCALE,
                        "orientation", GTK_ORIENTATION_HORIZONTAL,
                        "adjustment",  adjustment,
                        "digits",      digits,
                        NULL);

  set_param_spec (G_OBJECT (adjustment), scale, param_spec);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_prop_adjustment_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_adjustment_notify),
                  adjustment);

  gimp_widget_set_bound_property (scale, config, property_name);

  gtk_widget_show (scale);

  return scale;
}

/**
 * gimp_prop_scale_entry_new:
 * @config:         Object to which property is attached.
 * @label: (nullable): The text for the #GtkLabel which will appear left of
 *                     the #GtkScale.
 * @property_name:  Name of integer or double property controlled by the scale.
 * @factor:         Optional multiplier to convert @property_name's
 *                  range into the #GimpScaleEntry's range. The common
 *                  usage is to set 1.0.
 *                  For non-double properties, no other values than 1.0
 *                  are acceptable.
 * @limit_scale:    %FALSE if the range of possible values of the
 *                  GtkScale should be the same as of the GtkSpinButton.
 * @lower_limit:    The scale's lower boundary if @scale_limits is %TRUE.
 * @upper_limit:    The scale's upper boundary if @scale_limits is %TRUE.
 *
 * Creates a #GimpScaleEntry (slider and spin button) to set and display
 * the value of a specified int or double property with sensible default
 * settings depending on the range (decimal places, increments, etc.).
 * These settings can be overridden by the relevant widget methods.
 *
 * If @label is %NULL, the @property_name's nick will be used as label
 * of the returned object.
 *
 * If @factor is not 1.0, the widget's range will be computed based of
 * @property_name's range multiplied by @factor. A typical usage would
 * be to display a [0.0, 1.0] range as [0.0, 100.0] by setting 100.0 as
 * @factor.
 *
 * See gimp_scale_entry_set_bounds() for more information on
 * @limit_scale, @lower_limit and @upper_limit.
 *
 * Returns: (transfer full): The newly allocated #GimpScaleEntry.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_scale_entry_new (GObject     *config,
                           const gchar *property_name,
                           const gchar *label,
                           gdouble      factor,
                           gboolean     limit_scale,
                           gdouble      lower_limit,
                           gdouble      upper_limit)
{
  GtkWidget   *widget;
  GParamSpec  *param_spec;
  const gchar *tooltip;
  gdouble      value;
  gdouble      lower;
  gdouble      upper;
  gint         digits = -1;

  g_return_val_if_fail (factor != 0.0, NULL);

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_return_val_if_fail (G_IS_PARAM_SPEC_DOUBLE (param_spec) || factor == 1.0, NULL);

  if (! get_numeric_values (config,
                            param_spec, &value, &lower, &upper, G_STRFUNC))
    return NULL;

  if (! label)
    label = g_param_spec_get_nick (param_spec);

  if (G_IS_PARAM_SPEC_INT (param_spec) || G_IS_PARAM_SPEC_UINT (param_spec))
    digits = 0;

  widget = gimp_scale_entry_new (label, value, lower * factor, upper * factor, digits);
  if (limit_scale)
    gimp_scale_entry_set_bounds (GIMP_SCALE_ENTRY (widget),
                                 lower_limit, upper_limit,
                                 FALSE);

  tooltip = g_param_spec_get_blurb (param_spec);
  gimp_help_set_help_data (widget, tooltip, NULL);

  if (factor != 1.0)
    {
      gdouble *user_data;

      user_data = g_new0 (gdouble, 1);
      *user_data = factor;
      /* With @factor == 1.0, this is equivalent to a
       * g_object_bind_property().
       */
      g_object_bind_property_full (config, property_name,
                                   widget, "value",
                                   G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                                   gimp_prop_widget_double_to_factor,
                                   gimp_prop_widget_double_from_factor,
                                   user_data, (GDestroyNotify) g_free);
    }
  else
    {
      g_object_bind_property (config, property_name,
                              widget, "value",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    }

  gimp_widget_set_bound_property (widget, config, property_name);

  return widget;
}


static void
gimp_prop_adjustment_callback (GtkAdjustment *adjustment,
                               GObject       *config)
{
  GParamSpec *param_spec;
  gdouble     value;
  gdouble    *factor;

  param_spec = get_param_spec (G_OBJECT (adjustment));
  if (! param_spec)
    return;

  value = gtk_adjustment_get_value (adjustment);

  factor = g_object_get_data (G_OBJECT (adjustment),
                              "gimp-prop-adjustment-factor");
  if (factor)
    value /= *factor;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != (gint) value)
        g_object_set (config, param_spec->name, (gint) value, NULL);
    }
  else if (G_IS_PARAM_SPEC_UINT (param_spec))
    {
      guint v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != (guint) value)
        g_object_set (config, param_spec->name, (guint) value, NULL);
    }
  else if (G_IS_PARAM_SPEC_LONG (param_spec))
    {
      glong v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != (glong) value)
        g_object_set (config, param_spec->name, (glong) value, NULL);
    }
  else if (G_IS_PARAM_SPEC_ULONG (param_spec))
    {
      gulong v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != (gulong) value)
        g_object_set (config, param_spec->name, (gulong) value, NULL);
    }
  else if (G_IS_PARAM_SPEC_INT64 (param_spec))
    {
      gint64 v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != (gint64) value)
        g_object_set (config, param_spec->name, (gint64) value, NULL);
    }
  else if (G_IS_PARAM_SPEC_UINT64 (param_spec))
    {
      guint64 v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != (guint64) value)
        g_object_set (config, param_spec->name, (guint64) value, NULL);
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      gdouble v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
gimp_prop_adjustment_notify (GObject       *config,
                             GParamSpec    *param_spec,
                             GtkAdjustment *adjustment)
{
  gdouble  value;
  gdouble *factor;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint int_value;

      g_object_get (config, param_spec->name, &int_value, NULL);

      value = int_value;
    }
  else if (G_IS_PARAM_SPEC_UINT (param_spec))
    {
      guint uint_value;

      g_object_get (config, param_spec->name, &uint_value, NULL);

      value = uint_value;
    }
  else if (G_IS_PARAM_SPEC_LONG (param_spec))
    {
      glong long_value;

      g_object_get (config, param_spec->name, &long_value, NULL);

      value = long_value;
    }
  else if (G_IS_PARAM_SPEC_ULONG (param_spec))
    {
      gulong ulong_value;

      g_object_get (config, param_spec->name, &ulong_value, NULL);

      value = ulong_value;
    }
  else if (G_IS_PARAM_SPEC_INT64 (param_spec))
    {
      gint64 int64_value;

      g_object_get (config, param_spec->name, &int64_value, NULL);

      value = int64_value;
    }
  else if (G_IS_PARAM_SPEC_UINT64 (param_spec))
    {
      guint64 uint64_value;

      g_object_get (config, param_spec->name, &uint64_value, NULL);

#if defined _MSC_VER && (_MSC_VER < 1300)
      value = (gint64) uint64_value;
#else
      value = uint64_value;
#endif
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      g_object_get (config, param_spec->name, &value, NULL);
    }
  else
    {
      g_warning ("%s: unhandled param spec of type %s",
                 G_STRFUNC, G_PARAM_SPEC_TYPE_NAME (param_spec));
      return;
    }

  factor = g_object_get_data (G_OBJECT (adjustment),
                              "gimp-prop-adjustment-factor");
  if (factor)
    value *= *factor;

  if (gtk_adjustment_get_value (adjustment) != value)
    {
      g_signal_handlers_block_by_func (adjustment,
                                       gimp_prop_adjustment_callback,
                                       config);

      gtk_adjustment_set_value (adjustment, value);

      g_signal_handlers_unblock_by_func (adjustment,
                                         gimp_prop_adjustment_callback,
                                         config);
    }
}


/*************/
/*  memsize  */
/*************/

static void   gimp_prop_memsize_callback (GimpMemsizeEntry *entry,
                                          GObject          *config);
static void   gimp_prop_memsize_notify   (GObject          *config,
                                          GParamSpec       *param_spec,
                                          GimpMemsizeEntry *entry);

/**
 * gimp_prop_memsize_entry_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of memsize property.
 *
 * Creates a #GimpMemsizeEntry (spin button and option menu) to set
 * and display the value of the specified memsize property.  See
 * gimp_memsize_entry_new() for more information.
 *
 * Returns: (transfer full): A new #GimpMemsizeEntry.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_memsize_entry_new (GObject     *config,
                             const gchar *property_name)
{
  GParamSpec       *param_spec;
  GParamSpecUInt64 *uint64_spec;
  GtkWidget        *entry;
  guint64           value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   GIMP_TYPE_PARAM_MEMSIZE, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  uint64_spec = G_PARAM_SPEC_UINT64 (param_spec);

  g_return_val_if_fail (uint64_spec->minimum <= GIMP_MAX_MEMSIZE, NULL);
  g_return_val_if_fail (uint64_spec->maximum <= GIMP_MAX_MEMSIZE, NULL);

  entry = gimp_memsize_entry_new (value,
                                  uint64_spec->minimum,
                                  uint64_spec->maximum);

  set_param_spec (G_OBJECT (entry),
                  gimp_memsize_entry_get_spinbutton (GIMP_MEMSIZE_ENTRY (entry)),
                  param_spec);

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (gimp_prop_memsize_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_memsize_notify),
                  entry);

  gimp_widget_set_bound_property (entry, config, property_name);

  gtk_widget_show (entry);

  return entry;
}


static void
gimp_prop_memsize_callback (GimpMemsizeEntry *entry,
                            GObject          *config)
{
  GParamSpec *param_spec;
  guint64     value;
  guint64     v;

  param_spec = get_param_spec (G_OBJECT (entry));
  if (! param_spec)
    return;

  g_return_if_fail (G_IS_PARAM_SPEC_UINT64 (param_spec));

  value = gimp_memsize_entry_get_value (entry);

  g_object_get (config, param_spec->name, &v, NULL);

  if (v != value)
    g_object_set (config, param_spec->name, value, NULL);
}

static void
gimp_prop_memsize_notify (GObject          *config,
                          GParamSpec       *param_spec,
                          GimpMemsizeEntry *entry)
{
  guint64  value;

  g_return_if_fail (G_IS_PARAM_SPEC_UINT64 (param_spec));

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (gimp_memsize_entry_get_value (entry) != value)
    {
      g_signal_handlers_block_by_func (entry,
                                       gimp_prop_memsize_callback,
                                       config);

      gimp_memsize_entry_set_value (entry, value);

      g_signal_handlers_unblock_by_func (entry,
                                         gimp_prop_memsize_callback,
                                         config);
    }
}


/***********/
/*  label  */
/***********/

/**
 * gimp_prop_label_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of string property.
 *
 * Creates a #GtkLabel to display the value of the specified property.
 * The property should be a string property or at least transformable
 * to a string.  If the user should be able to edit the string, use
 * gimp_prop_entry_new() instead.
 *
 * Returns: (transfer full): A new #GtkLabel widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_label_new (GObject     *config,
                     const gchar *property_name)
{
  GParamSpec    *param_spec;
  GtkWidget     *label;
  const gchar   *blurb;
  GBindingFlags  flags = G_BINDING_SYNC_CREATE;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  label = gtk_label_new (NULL);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_show (label);

  blurb = g_param_spec_get_blurb (param_spec);
  if (blurb)
    gimp_help_set_help_data (label, blurb, NULL);

  /* As labels are read-only widgets, allow read-only properties (though
   * we still keep possibility for bidirectional binding, which can be
   * useful even with labels).
   */
  if (param_spec->flags & G_PARAM_WRITABLE)
    flags |= G_BINDING_BIDIRECTIONAL;

  g_object_bind_property (config, property_name,
                          label, "label", flags);

  gimp_widget_set_bound_property (label, config, property_name);

  return label;
}


/***********/
/*  entry  */
/***********/

static void   gimp_prop_entry_callback (GtkWidget  *entry,
                                        GObject    *config);
static void   gimp_prop_entry_notify   (GObject    *config,
                                        GParamSpec *param_spec,
                                        GtkWidget  *entry);

/**
 * gimp_prop_entry_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of string property.
 * @max_len:       Maximum allowed length of string.
 *
 * Creates a #GtkEntry to set and display the value of the specified
 * string property.
 *
 * Returns: (transfer full): A new #GtkEntry widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_entry_new (GObject     *config,
                     const gchar *property_name,
                     gint         max_len)
{
  GParamSpec *param_spec;
  GtkWidget  *entry;
  gchar      *value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec (config, property_name,
                                 G_TYPE_PARAM_STRING, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), value ? value : "");

  g_free (value);

  if (max_len > 0)
    gtk_entry_set_max_length (GTK_ENTRY (entry), max_len);

  gtk_editable_set_editable (GTK_EDITABLE (entry),
                             param_spec->flags & G_PARAM_WRITABLE);
  gtk_widget_set_hexpand (entry, TRUE);

  set_param_spec (G_OBJECT (entry), entry, param_spec);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_prop_entry_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_entry_notify),
                  entry);

  gimp_widget_set_bound_property (entry, config, property_name);

  gtk_widget_show (entry);

  return entry;
}

static void
gimp_prop_entry_callback (GtkWidget *entry,
                          GObject   *config)
{
  GParamSpec  *param_spec;
  const gchar *value;
  gchar       *v;

  param_spec = get_param_spec (G_OBJECT (entry));
  if (! param_spec)
    return;

  value = gtk_entry_get_text (GTK_ENTRY (entry));

  g_object_get (config, param_spec->name, &v, NULL);

  if (g_strcmp0 (v, value))
    {
      g_signal_handlers_block_by_func (config,
                                       gimp_prop_entry_notify,
                                       entry);

      g_object_set (config, param_spec->name, value, NULL);

      g_signal_handlers_unblock_by_func (config,
                                         gimp_prop_entry_notify,
                                         entry);
    }

  g_free (v);
}

static void
gimp_prop_entry_notify (GObject    *config,
                        GParamSpec *param_spec,
                        GtkWidget  *entry)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (entry,
                                   gimp_prop_entry_callback,
                                   config);

  gtk_entry_set_text (GTK_ENTRY (entry), value ? value : "");

  g_signal_handlers_unblock_by_func (entry,
                                     gimp_prop_entry_callback,
                                     config);

  g_free (value);
}

/*****************/
/*  label entry  */
/*****************/

/**
 * gimp_prop_label_entry_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of string property.
 * @max_len:       Maximum allowed length of string (set to negative for
 *                 no maximum).
 *
 * Creates a #GimpLabelEntry to set and display the value of the
 * specified string property.
 *
 * Returns: (transfer full): A new #GtkEntry widget.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_label_entry_new (GObject     *config,
                           const gchar *property_name,
                           gint         max_len)
{
  GParamSpec  *param_spec;
  GtkWidget   *label_entry;
  GtkWidget   *entry;
  const gchar *label;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec (config, property_name,
                                 G_TYPE_PARAM_STRING, G_STRFUNC);
  if (! param_spec)
    return NULL;

  label = g_param_spec_get_nick (param_spec);
  label_entry = gimp_label_entry_new (label);
  entry = gimp_label_entry_get_entry (GIMP_LABEL_ENTRY (label_entry));

  if (max_len > 0)
    gtk_entry_set_max_length (GTK_ENTRY (entry), max_len);

  gtk_editable_set_editable (GTK_EDITABLE (entry),
                             param_spec->flags & G_PARAM_WRITABLE);
  gtk_widget_set_hexpand (entry, TRUE);

  set_param_spec (G_OBJECT (label_entry), label_entry, param_spec);

  g_object_bind_property (config,      property_name,
                          label_entry, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (label_entry, config, property_name);

  gtk_widget_show (label_entry);

  return label_entry;
}


/*****************/
/*  text buffer  */
/*****************/

static void   gimp_prop_text_buffer_callback (GtkTextBuffer *text_buffer,
                                              GObject       *config);
static void   gimp_prop_text_buffer_notify   (GObject       *config,
                                              GParamSpec    *param_spec,
                                              GtkTextBuffer *text_buffer);

/**
 * gimp_prop_text_buffer_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of string property.
 * @max_len:       Maximum allowed length of text (in characters).
 *
 * Creates a #GtkTextBuffer to set and display the value of the
 * specified string property.  Unless the string is expected to
 * contain multiple lines or a large amount of text, use
 * gimp_prop_entry_new() instead.  See #GtkTextView for information on
 * how to insert a text buffer into a visible widget.
 *
 * If @max_len is 0 or negative, the text buffer allows an unlimited
 * number of characters to be entered.
 *
 * Returns: (transfer full): A new #GtkTextBuffer.
 *
 * Since: 2.4
 */
GtkTextBuffer *
gimp_prop_text_buffer_new (GObject     *config,
                           const gchar *property_name,
                           gint         max_len)
{
  GParamSpec    *param_spec;
  GtkTextBuffer *text_buffer;
  gchar         *value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_STRING, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  text_buffer = gtk_text_buffer_new (NULL);
  gtk_text_buffer_set_text (text_buffer, value ? value : "", -1);

  g_free (value);

  if (max_len > 0)
    g_object_set_data (G_OBJECT (text_buffer), "max-len",
                       GINT_TO_POINTER (max_len));

  set_param_spec (G_OBJECT (text_buffer), NULL, param_spec);

  g_signal_connect (text_buffer, "changed",
                    G_CALLBACK (gimp_prop_text_buffer_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_text_buffer_notify),
                  text_buffer);

  return text_buffer;
}

static void
gimp_prop_text_buffer_callback (GtkTextBuffer *text_buffer,
                                GObject       *config)
{
  GParamSpec  *param_spec;
  GtkTextIter  start_iter;
  GtkTextIter  end_iter;
  gchar       *text;
  gint         max_len;

  param_spec = get_param_spec (G_OBJECT (text_buffer));
  if (! param_spec)
    return;

  gtk_text_buffer_get_bounds (text_buffer, &start_iter, &end_iter);
  text = gtk_text_buffer_get_text (text_buffer, &start_iter, &end_iter, FALSE);

  max_len = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (text_buffer),
                                                "max-length"));

  if (max_len > 0 && g_utf8_strlen (text, -1) > max_len)
    {
      g_message (dngettext (GETTEXT_PACKAGE "-libgimp",
                            "This text input field is limited to %d character.",
                            "This text input field is limited to %d characters.",
                            max_len), max_len);

      gtk_text_buffer_get_iter_at_offset (text_buffer,
                                          &start_iter, max_len - 1);
      gtk_text_buffer_get_end_iter (text_buffer, &end_iter);

      /*  this calls us recursively, but in the else branch  */
      gtk_text_buffer_delete (text_buffer, &start_iter, &end_iter);
    }
  else
    {
      gchar *v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (g_strcmp0 (v, text))
        {
          g_signal_handlers_block_by_func (config,
                                           gimp_prop_text_buffer_notify,
                                           text_buffer);

          g_object_set (config, param_spec->name, text,  NULL);

          g_signal_handlers_unblock_by_func (config,
                                             gimp_prop_text_buffer_notify,
                                             text_buffer);
        }

      g_free (v);
    }

  g_free (text);
}

static void
gimp_prop_text_buffer_notify (GObject       *config,
                              GParamSpec    *param_spec,
                              GtkTextBuffer *text_buffer)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (text_buffer,
                                   gimp_prop_text_buffer_callback,
                                   config);

  gtk_text_buffer_set_text (text_buffer, value ? value : "", -1);

  g_signal_handlers_unblock_by_func (text_buffer,
                                     gimp_prop_text_buffer_callback,
                                     config);

  g_free (value);
}


/***********************/
/*  string combo box   */
/***********************/

static void   gimp_prop_string_combo_box_callback (GtkWidget   *widget,
                                                   GObject     *config);
static void   gimp_prop_string_combo_box_notify   (GObject     *config,
                                                   GParamSpec  *param_spec,
                                                   GtkWidget   *widget);

static gboolean
          gimp_prop_choice_combo_box_is_sensitive (const gchar *nick,
                                                   GimpChoice  *choice);


/**
 * gimp_prop_string_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of int property controlled by combo box.
 * @model:         #GtkTreeStore holding list of values
 * @id_column:     column in @store that holds string IDs
 * @label_column:  column in @store that holds labels to use in the combo-box
 *
 * Creates a #GimpStringComboBox widget to display and set the
 * specified property.  The contents of the widget are determined by
 * @store.
 *
 * Returns: (transfer full): The newly created #GimpStringComboBox widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_string_combo_box_new (GObject      *config,
                                const gchar  *property_name,
                                GtkTreeModel *model,
                                gint          id_column,
                                gint          label_column)
{
  GParamSpec *param_spec;
  GtkWidget  *combo_box;
  gchar      *value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_STRING, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  combo_box = gimp_string_combo_box_new (model, id_column, label_column);

  gimp_string_combo_box_set_active (GIMP_STRING_COMBO_BOX (combo_box), value);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (gimp_prop_string_combo_box_callback),
                    config);

  set_param_spec (G_OBJECT (combo_box), combo_box, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_string_combo_box_notify),
                  combo_box);

  gimp_widget_set_bound_property (combo_box, config, property_name);

  gtk_widget_show (combo_box);

  return combo_box;
}

/**
 * gimp_prop_choice_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of %GimpChoice property controlled by combo box.
 *
 * Creates a #GimpStringComboBox widget to display and set the
 * specified property.
 *
 * Returns: (transfer full): The newly created #GimpStringComboBox widget.
 */
GtkWidget *
gimp_prop_choice_combo_box_new (GObject     *config,
                                const gchar *property_name)
{
  GParamSpec   *param_spec;
  GimpChoice   *choice;
  GtkWidget    *combo_box;
  GtkListStore *store;
  GList        *values;
  GList        *iter;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   GIMP_TYPE_PARAM_CHOICE, G_STRFUNC);
  if (! param_spec)
    return NULL;

  choice = gimp_param_spec_choice_get_choice (param_spec);
  values = gimp_choice_list_nicks (choice);
  store  = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  for (iter = values; iter; iter = iter->next)
    {
      const gchar *nick  = iter->data;
      const gchar *label = gimp_choice_get_label (choice, nick);

      gtk_list_store_insert_with_values (store, NULL, -1,
                                         0, nick,
                                         1, label,
                                         -1);

    }

  combo_box = gimp_string_combo_box_new (GTK_TREE_MODEL (store), 0, 1);
  g_object_unref (store);

  gimp_string_combo_box_set_sensitivity (GIMP_STRING_COMBO_BOX (combo_box),
                                         (GimpStringSensitivityFunc) gimp_prop_choice_combo_box_is_sensitive,
                                         choice, NULL);
  g_signal_connect_swapped (choice, "sensitivity-changed",
                            G_CALLBACK (gtk_widget_queue_draw),
                            combo_box);

  g_object_bind_property (config,    property_name,
                          combo_box, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (combo_box, config, property_name);

  gtk_widget_show (combo_box);

  return combo_box;
}

static void
gimp_prop_string_combo_box_callback (GtkWidget *widget,
                                     GObject   *config)
{
  GParamSpec  *param_spec;
  gchar       *value;
  gchar       *v;

  param_spec = get_param_spec (G_OBJECT (widget));
  if (! param_spec)
    return;

  value = gimp_string_combo_box_get_active (GIMP_STRING_COMBO_BOX (widget));

  g_object_get (config, param_spec->name, &v, NULL);

  if (g_strcmp0 (v, value))
    g_object_set (config, param_spec->name, value, NULL);

  g_free (value);
  g_free (v);
}

static void
gimp_prop_string_combo_box_notify (GObject    *config,
                                   GParamSpec *param_spec,
                                   GtkWidget  *combo_box)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo_box,
                                   gimp_prop_string_combo_box_callback,
                                   config);

  gimp_string_combo_box_set_active (GIMP_STRING_COMBO_BOX (combo_box), value);

  g_signal_handlers_unblock_by_func (combo_box,
                                     gimp_prop_string_combo_box_callback,
                                     config);

  g_free (value);
}

static gboolean
gimp_prop_choice_combo_box_is_sensitive (const gchar *nick,
                                         GimpChoice  *choice)
{
  return gimp_choice_is_valid (choice, nick);
}


/*************************/
/*  choice radio frame   */
/*************************/

static gboolean
gimp_prop_widget_choice_is_sensitive  (gint          value,
                                       gpointer      user_data,
                                       gint         *new_value,
                                       gpointer      data);
static gboolean
gimp_prop_widget_choice_string_to_int (GBinding     *binding,
                                       const GValue *from_value,
                                       GValue       *to_value,
                                       gpointer      user_data);
static gboolean
gimp_prop_widget_choice_int_to_string (GBinding     *binding,
                                       const GValue *from_value,
                                       GValue       *to_value,
                                       gpointer      user_data);


/**
 * gimp_prop_choice_radio_frame_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of %GimpChoice property controlled by radio buttons.
 *
 * Creates a [class@GimpUi.IntRadioFrame] widget to display and set the
 * specified [class@Gimp.Choice] property.
 *
 * Returns: (transfer full): The newly created #GimpIntRadioFrame widget.
 */
GtkWidget *
gimp_prop_choice_radio_frame_new (GObject     *config,
                                  const gchar *property_name)
{
  GParamSpec   *param_spec;
  GimpChoice   *choice;
  GtkWidget    *frame;
  GimpIntStore *store;
  GList        *values;
  GList        *iter;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   GIMP_TYPE_PARAM_CHOICE, G_STRFUNC);
  if (! param_spec)
    return NULL;

  choice = gimp_param_spec_choice_get_choice (param_spec);
  values = gimp_choice_list_nicks (choice);
  store = g_object_new (GIMP_TYPE_INT_STORE, NULL);

  for (iter = values; iter; iter = iter->next)
    {
      const gchar *nick  = iter->data;
      const gchar *label = gimp_choice_get_label (choice, nick);
      gint         id    = gimp_choice_get_id (choice, nick);

      gtk_list_store_insert_with_values (GTK_LIST_STORE (store), NULL, -1,
                                         GIMP_INT_STORE_VALUE, id,
                                         GIMP_INT_STORE_LABEL, label,
                                         -1);

    }

  frame = gimp_int_radio_frame_new_from_store (NULL, store);
  gimp_int_radio_frame_set_title (GIMP_INT_RADIO_FRAME (frame), g_param_spec_get_nick (param_spec), TRUE);
  gimp_help_set_help_data (frame, g_param_spec_get_blurb (param_spec), NULL);
  g_object_unref (store);

  gimp_int_radio_frame_set_sensitivity (GIMP_INT_RADIO_FRAME (frame),
                                        (GimpIntRadioFrameSensitivityFunc) gimp_prop_widget_choice_is_sensitive,
                                        choice, NULL);

  g_object_bind_property_full (config,  property_name,
                               frame,   "value",
                               G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                               gimp_prop_widget_choice_string_to_int,
                               gimp_prop_widget_choice_int_to_string,
                               choice, NULL);

  gimp_widget_set_bound_property (frame, config, property_name);

  gtk_widget_show (frame);

  return frame;
}

static gboolean
gimp_prop_widget_choice_is_sensitive (gint      value,
                                      gpointer  user_data,
                                      gint     *new_value,
                                      gpointer  data)
{
  GimpChoice *choice = GIMP_CHOICE (data);
  GList      *values;
  GList      *iter;

  values = gimp_choice_list_nicks (choice);

  for (iter = values; iter; iter = iter->next)
    {
      if (gimp_choice_get_id (choice, iter->data) == value)
        return gimp_choice_is_valid (choice, iter->data);
    }

  return FALSE;
}

static gboolean
gimp_prop_widget_choice_string_to_int (GBinding     *binding,
                                       const GValue *from_value,
                                       GValue       *to_value,
                                       gpointer      user_data)
{
  GimpChoice  *choice = GIMP_CHOICE (user_data);
  const gchar *val = g_value_get_string (from_value);

  g_value_set_int (to_value, gimp_choice_get_id (choice, val));

  return TRUE;
}

static gboolean
gimp_prop_widget_choice_int_to_string (GBinding     *binding,
                                       const GValue *from_value,
                                       GValue       *to_value,
                                       gpointer      user_data)
{
  GimpChoice *choice = GIMP_CHOICE (user_data);
  gint        val    = g_value_get_int (from_value);
  GList      *values;
  GList      *iter;

  values = gimp_choice_list_nicks (choice);

  for (iter = values; iter; iter = iter->next)
    {
      if (gimp_choice_get_id (choice, iter->data) == val)
        {
          g_value_set_string (to_value, iter->data);
          break;
        }
    }

  return (iter != NULL);
}

/*************************/
/*  file chooser button  */
/*************************/

static GtkWidget * gimp_prop_file_chooser_button_setup    (GtkWidget      *button,
                                                           GObject        *config,
                                                           GParamSpec     *param_spec);
static void        gimp_prop_file_chooser_button_callback (GtkFileChooser *button,
                                                           GObject        *config);
static void        gimp_prop_file_chooser_button_notify   (GObject        *config,
                                                           GParamSpec     *param_spec,
                                                           GtkFileChooser *button);


/**
 * gimp_prop_file_chooser_new:
 * @config:            Object to which property is attached.
 * @property_name:     Name of a %GimpParamSpecFile property.
 * @label: (nullable): Label of the widget.
 * @title: (nullable): Title of the file dialog.
 *
 * Creates a [class@GimpUi.FileChooser] to edit the specified file
 * property. @property_name must be a %GimpParamSpecFile with an action
 * other than [enum@Gimp.FileChooserAction.ANY].
 *
 * If @label is %NULL, @property_name's `nick` text will be used
 * instead.
 *
 * Returns: (transfer full): A new #GtkFileChooserButton.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_file_chooser_new (GObject     *config,
                            const gchar *property_name,
                            const gchar *label,
                            const gchar *title)
{
  GimpFileChooserAction  action;
  GParamSpec            *pspec;
  GtkWidget             *widget;
  GFile                 *file = NULL;
  const gchar           *tooltip;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  pspec = find_param_spec (config, property_name, G_STRFUNC);
  if (! pspec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC, g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! GIMP_IS_PARAM_SPEC_FILE (pspec))
    {
      g_warning ("%s: property '%s' of %s is not a GIMP_PARAM_SPEC_FILE.",
                 G_STRFUNC, pspec->name, g_type_name (pspec->owner_type));
      return NULL;
    }

  action = gimp_param_spec_file_get_action (pspec);
  if (action == GIMP_FILE_CHOOSER_ACTION_ANY)
    {
      g_warning ("%s: property '%s' of %s must not use action GIMP_FILE_CHOOSER_ACTION_ANY.",
                 G_STRFUNC, pspec->name, g_type_name (pspec->owner_type));
      return NULL;
    }

  if (! label)
    label = g_param_spec_get_nick (pspec);

  g_object_get (config,
                property_name, &file,
                NULL);

  widget = gimp_file_chooser_new (action, label, title, file);

  tooltip = g_param_spec_get_blurb (pspec);
  gimp_help_set_help_data (widget, tooltip, NULL);

  g_object_bind_property (config, property_name,
                          widget, "file",
                          G_BINDING_BIDIRECTIONAL);

  g_clear_object (&file);

  return widget;
}

/**
 * gimp_prop_file_chooser_button_new:
 * @config:        object to which property is attached.
 * @property_name: name of path property.
 * @title: (nullable): the title of the browse dialog.
 * @action:        the open mode for the widget.
 *
 * Creates a #GtkFileChooserButton to edit the specified path property.
 * @property_name must represent either a GIMP_PARAM_SPEC_CONFIG_PATH or
 * a G_PARAM_SPEC_OBJECT where `value_type == G_TYPE_FILE`.
 *
 * Note that #GtkFileChooserButton implements the #GtkFileChooser
 * interface; you can use the #GtkFileChooser API with it.
 *
 * Returns: (transfer full): A new #GtkFileChooserButton.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_file_chooser_button_new (GObject              *config,
                                   const gchar          *property_name,
                                   const gchar          *title,
                                   GtkFileChooserAction  action)
{
  GParamSpec *param_spec;
  GtkWidget  *button;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC, g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return NULL;
    }

  if (! GIMP_IS_PARAM_SPEC_CONFIG_PATH (param_spec) &&
      (! G_IS_PARAM_SPEC_OBJECT (param_spec) || param_spec->value_type != G_TYPE_FILE))
    {
      g_warning ("%s: property '%s' of %s is neither a GIMP_PARAM_SPEC_CONFIG_PATH "
                 "nor a G_PARAM_SPEC_OBJECT of value type G_TYPE_FILE.",
                 G_STRFUNC, param_spec->name,
                 g_type_name (param_spec->owner_type));
      return NULL;
    }

  if (! title)
    title = g_param_spec_get_nick (param_spec);

  button = gtk_file_chooser_button_new (title, action);

  return gimp_prop_file_chooser_button_setup (button, config, param_spec);
}

/**
 * gimp_prop_file_chooser_button_new_with_dialog:
 * @config:        object to which property is attached.
 * @property_name: name of path property.
 * @dialog:        the #GtkFileChooserDialog widget to use.
 *
 * Creates a #GtkFileChooserButton to edit the specified path property.
 *
 * The button uses @dialog as it's file-picking window. Note that @dialog
 * must be a #GtkFileChooserDialog (or subclass) and must not have
 * %GTK_DIALOG_DESTROY_WITH_PARENT set.
 *
 * Note that #GtkFileChooserButton implements the #GtkFileChooser
 * interface; you can use the #GtkFileChooser API with it.
 *
 * Returns: (transfer full): A new #GtkFileChooserButton.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_file_chooser_button_new_with_dialog (GObject     *config,
                                               const gchar *property_name,
                                               GtkWidget   *dialog)
{
  GParamSpec *param_spec;
  GtkWidget  *button;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  g_return_val_if_fail (GTK_IS_FILE_CHOOSER_DIALOG (dialog), NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   GIMP_TYPE_PARAM_CONFIG_PATH, G_STRFUNC);
  if (! param_spec)
    return NULL;

  button = gtk_file_chooser_button_new_with_dialog (dialog);

  return gimp_prop_file_chooser_button_setup (button, config, param_spec);
}

static GtkWidget *
gimp_prop_file_chooser_button_setup (GtkWidget  *button,
                                     GObject    *config,
                                     GParamSpec *param_spec)
{
  GFile *file = NULL;

  if (GIMP_IS_PARAM_SPEC_CONFIG_PATH (param_spec))
    {
      gchar *value;

      g_object_get (config,
                    param_spec->name, &value,
                    NULL);

      if (value)
        {
          file = gimp_file_new_for_config_path (value, NULL);
          g_free (value);
        }
    }
  else /* G_FILE */
    {
      g_object_get (config,
                    param_spec->name, &file,
                    NULL);
    }

  if (file)
    {
      gchar *basename = g_file_get_basename (file);

      if (basename && basename[0] == '.')
        gtk_file_chooser_set_show_hidden (GTK_FILE_CHOOSER (button), TRUE);

      g_free (basename);

      gtk_file_chooser_set_file (GTK_FILE_CHOOSER (button), file, NULL);
      g_object_unref (file);
    }

  set_param_spec (G_OBJECT (button), button, param_spec);

  g_signal_connect (button, "file-set",
                    G_CALLBACK (gimp_prop_file_chooser_button_callback),
                    config);

  connect_notify (config, param_spec->name,
                  G_CALLBACK (gimp_prop_file_chooser_button_notify),
                  button);

  gtk_widget_set_visible (button, TRUE);

  return button;
}

static void
gimp_prop_file_chooser_button_callback (GtkFileChooser *button,
                                        GObject        *config)
{
  GParamSpec *param_spec;
  GFile      *file;

  param_spec = get_param_spec (G_OBJECT (button));
  if (! param_spec)
    return;

  file = gtk_file_chooser_get_file (button);

  if (GIMP_IS_PARAM_SPEC_CONFIG_PATH (param_spec))
    {
      gchar *value = NULL;
      gchar *v;

      if (file)
        {
          value = gimp_file_get_config_path (file, NULL);
        }

      g_object_get (config, param_spec->name, &v, NULL);

      if (g_strcmp0 (v, value))
        {
          g_signal_handlers_block_by_func (config,
                                           gimp_prop_file_chooser_button_notify,
                                           button);

          g_object_set (config, param_spec->name, value, NULL);

          g_signal_handlers_unblock_by_func (config,
                                             gimp_prop_file_chooser_button_notify,
                                             button);
        }

      g_free (value);
      g_free (v);
    }
  else /* G_FILE */
    {
      GFile *f = NULL;

      g_object_get (config, param_spec->name, &f, NULL);

      if (! f || ! file || ! g_file_equal (f, file))
        {
          g_signal_handlers_block_by_func (config,
                                           gimp_prop_file_chooser_button_notify,
                                           button);

          g_object_set (config, param_spec->name, file, NULL);

          g_signal_handlers_unblock_by_func (config,
                                             gimp_prop_file_chooser_button_notify,
                                             button);
        }
      g_clear_object (&f);
    }
  g_clear_object (&file);
}

static void
gimp_prop_file_chooser_button_notify (GObject        *config,
                                      GParamSpec     *param_spec,
                                      GtkFileChooser *button)
{
  GFile *file = NULL;

  if (GIMP_IS_PARAM_SPEC_CONFIG_PATH (param_spec))
    {
      gchar *value = NULL;

      g_object_get (config,
                    param_spec->name, &value,
                    NULL);

      if (value)
        {
          file = gimp_file_new_for_config_path (value, NULL);
          g_free (value);
        }
    }
  else
    {
      g_object_get (config,
                    param_spec->name, &file,
                    NULL);
    }

  g_signal_handlers_block_by_func (button,
                                   gimp_prop_file_chooser_button_callback,
                                   config);

  if (file)
    {
      gtk_file_chooser_set_file (button, file, NULL);
      g_object_unref (file);
    }
  else
    {
      gtk_file_chooser_unselect_all (button);
    }

  g_signal_handlers_unblock_by_func (button,
                                     gimp_prop_file_chooser_button_callback,
                                     config);
}


/*****************/
/*  path editor  */
/*****************/

static void   gimp_prop_path_editor_path_callback     (GimpPathEditor *editor,
                                                       GObject        *config);
static void   gimp_prop_path_editor_writable_callback (GimpPathEditor *editor,
                                                       GObject        *config);
static void   gimp_prop_path_editor_path_notify       (GObject        *config,
                                                       GParamSpec     *param_spec,
                                                       GimpPathEditor *editor);
static void   gimp_prop_path_editor_writable_notify   (GObject        *config,
                                                       GParamSpec     *param_spec,
                                                       GimpPathEditor *editor);

/**
 * gimp_prop_path_editor_new:
 * @config:                 object to which property is attached.
 * @path_property_name:     name of path property.
 * @writable_property_name: name of writable path property.
 * @filechooser_title:      window title of #GtkFileChooserDialog widget.
 *
 * Creates a #GimpPathEditor to edit the specified path and writable
 * path properties.
 *
 * Returns: (transfer full): A new #GimpPathEditor.
 **/
GtkWidget *
gimp_prop_path_editor_new (GObject     *config,
                           const gchar *path_property_name,
                           const gchar *writable_property_name,
                           const gchar *filechooser_title)
{
  GParamSpec *path_param_spec;
  GParamSpec *writable_param_spec = NULL;
  GtkWidget  *editor;
  gchar      *value;
  gchar      *filename;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (path_property_name != NULL, NULL);

  path_param_spec = check_param_spec_w (config, path_property_name,
                                        GIMP_TYPE_PARAM_CONFIG_PATH, G_STRFUNC);
  if (! path_param_spec)
    return NULL;

  if (writable_property_name)
    {
      writable_param_spec = check_param_spec_w (config, writable_property_name,
                                                GIMP_TYPE_PARAM_CONFIG_PATH,
                                                G_STRFUNC);
      if (! writable_param_spec)
        return NULL;
    }

  g_object_get (config,
                path_property_name, &value,
                NULL);

  filename = value ? gimp_config_path_expand (value, TRUE, NULL) : NULL;
  g_free (value);

  editor = gimp_path_editor_new (filechooser_title, filename);
  g_free (filename);

  if (writable_property_name)
    {
      g_object_get (config,
                    writable_property_name, &value,
                    NULL);

      filename = value ? gimp_config_path_expand (value, TRUE, NULL) : NULL;
      g_free (value);

      gimp_path_editor_set_writable_path (GIMP_PATH_EDITOR (editor), filename);
      g_free (filename);
    }

  g_object_set_data (G_OBJECT (editor), "gimp-config-param-spec-path",
                     path_param_spec);

  g_signal_connect (editor, "path-changed",
                    G_CALLBACK (gimp_prop_path_editor_path_callback),
                    config);

  connect_notify (config, path_property_name,
                  G_CALLBACK (gimp_prop_path_editor_path_notify),
                  editor);

  if (writable_property_name)
    {
      g_object_set_data (G_OBJECT (editor), "gimp-config-param-spec-writable",
                         writable_param_spec);

      g_signal_connect (editor, "writable-changed",
                        G_CALLBACK (gimp_prop_path_editor_writable_callback),
                        config);

      connect_notify (config, writable_property_name,
                      G_CALLBACK (gimp_prop_path_editor_writable_notify),
                      editor);
    }

  gimp_widget_set_bound_property (editor, config, path_property_name);

  gtk_widget_show (editor);

  return editor;
}

static void
gimp_prop_path_editor_path_callback (GimpPathEditor *editor,
                                     GObject        *config)
{
  GParamSpec *path_param_spec;
  GParamSpec *writable_param_spec;
  gchar      *value;
  gchar      *utf8;

  path_param_spec     = g_object_get_data (G_OBJECT (editor),
                                           "gimp-config-param-spec-path");
  writable_param_spec = g_object_get_data (G_OBJECT (editor),
                                           "gimp-config-param-spec-writable");
  if (! path_param_spec)
    return;

  value = gimp_path_editor_get_path (editor);
  utf8 = value ? gimp_config_path_unexpand (value, TRUE, NULL) : NULL;
  g_free (value);

  g_signal_handlers_block_by_func (config,
                                   gimp_prop_path_editor_path_notify,
                                   editor);

  g_object_set (config,
                path_param_spec->name, utf8,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_path_editor_path_notify,
                                     editor);

  g_free (utf8);

  if (writable_param_spec)
    {
      value = gimp_path_editor_get_writable_path (editor);
      utf8 = value ? gimp_config_path_unexpand (value, TRUE, NULL) : NULL;
      g_free (value);

      g_signal_handlers_block_by_func (config,
                                       gimp_prop_path_editor_writable_notify,
                                       editor);

      g_object_set (config,
                    writable_param_spec->name, utf8,
                    NULL);

      g_signal_handlers_unblock_by_func (config,
                                         gimp_prop_path_editor_writable_notify,
                                         editor);

      g_free (utf8);
    }
}

static void
gimp_prop_path_editor_writable_callback (GimpPathEditor *editor,
                                         GObject        *config)
{
  GParamSpec *param_spec;
  gchar      *value;
  gchar      *utf8;

  param_spec = g_object_get_data (G_OBJECT (editor),
                                  "gimp-config-param-spec-writable");
  if (! param_spec)
    return;

  value = gimp_path_editor_get_writable_path (editor);
  utf8 = value ? gimp_config_path_unexpand (value, TRUE, NULL) : NULL;
  g_free (value);

  g_signal_handlers_block_by_func (config,
                                   gimp_prop_path_editor_writable_notify,
                                   editor);

  g_object_set (config,
                param_spec->name, utf8,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_path_editor_writable_notify,
                                     editor);

  g_free (utf8);
}

static void
gimp_prop_path_editor_path_notify (GObject        *config,
                                   GParamSpec     *param_spec,
                                   GimpPathEditor *editor)
{
  gchar *value;
  gchar *filename;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  filename = value ? gimp_config_path_expand (value, TRUE, NULL) : NULL;
  g_free (value);

  g_signal_handlers_block_by_func (editor,
                                   gimp_prop_path_editor_path_callback,
                                   config);

  gimp_path_editor_set_path (editor, filename);

  g_signal_handlers_unblock_by_func (editor,
                                     gimp_prop_path_editor_path_callback,
                                     config);

  g_free (filename);
}

static void
gimp_prop_path_editor_writable_notify (GObject        *config,
                                       GParamSpec     *param_spec,
                                       GimpPathEditor *editor)
{
  gchar *value;
  gchar *filename;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  filename = value ? gimp_config_path_expand (value, TRUE, NULL) : NULL;
  g_free (value);

  g_signal_handlers_block_by_func (editor,
                                   gimp_prop_path_editor_writable_callback,
                                   config);

  gimp_path_editor_set_writable_path (editor, filename);

  g_signal_handlers_unblock_by_func (editor,
                                     gimp_prop_path_editor_writable_callback,
                                     config);

  g_free (filename);
}


/***************/
/*  sizeentry  */
/***************/

static void   gimp_prop_size_entry_callback    (GimpSizeEntry *entry,
                                                GObject       *config);
static void   gimp_prop_size_entry_notify      (GObject       *config,
                                                GParamSpec    *param_spec,
                                                GimpSizeEntry *entry);
static void   gimp_prop_size_entry_notify_unit (GObject       *config,
                                                GParamSpec    *param_spec,
                                                GimpSizeEntry *entry);
static gint   gimp_prop_size_entry_num_chars   (gdouble        lower,
                                                gdouble        upper);


/**
 * gimp_prop_size_entry_new:
 * @config:             Object to which property is attached.
 * @property_name:      Name of int or double property.
 * @property_is_pixel:  When %TRUE, the property value is in pixels,
 *                      and in the selected unit otherwise.
 * @unit_property_name: Name of unit property.
 * @unit_format:        A printf-like unit-format string as is used with
 *                      gimp_unit_menu_new().
 * @update_policy:      How the automatic pixel <-> real-world-unit
 *                      calculations should be done.
 * @resolution:         The resolution (in dpi) for the field.
 *
 * Creates a #GimpSizeEntry to set and display the specified double or
 * int property, and its associated unit property.  Note that this
 * function is only suitable for creating a size entry holding a
 * single value.  Use gimp_prop_coordinates_new() to create a size
 * entry holding two values.
 *
 * Returns: (transfer full): A new #GimpSizeEntry widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_size_entry_new (GObject                   *config,
                          const gchar               *property_name,
                          gboolean                   property_is_pixel,
                          const gchar               *unit_property_name,
                          const gchar               *unit_format,
                          GimpSizeEntryUpdatePolicy  update_policy,
                          gdouble                    resolution)
{
  GtkWidget  *entry;
  GParamSpec *param_spec;
  GParamSpec *unit_param_spec;
  gboolean    show_pixels;
  gboolean    show_percent;
  gdouble     value;
  gdouble     lower;
  gdouble     upper;
  GimpUnit   *unit_value;
  gint        entry_width;

  param_spec = find_param_spec (config, property_name, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! get_numeric_values (config,
                            param_spec, &value, &lower, &upper, G_STRFUNC))
    return NULL;

  if (unit_property_name)
    {
      GValue value = G_VALUE_INIT;

      unit_param_spec = check_param_spec_w (config, unit_property_name,
                                            GIMP_TYPE_PARAM_UNIT, G_STRFUNC);
      if (! unit_param_spec)
        return NULL;

      g_value_init (&value, unit_param_spec->value_type);

      g_value_set_object (&value, gimp_unit_pixel ());
      show_pixels = (g_param_value_validate (unit_param_spec,
                                             &value) == FALSE);

      g_value_set_object (&value, gimp_unit_percent ());
      show_percent = (g_param_value_validate (unit_param_spec,
                                              &value) == FALSE);

      g_value_unset (&value);

      g_object_get (config,
                    unit_property_name, &unit_value,
                    NULL);
    }
  else
    {
      unit_param_spec = NULL;
      unit_value      = gimp_unit_inch ();
      show_pixels     = FALSE;
      show_percent    = FALSE;
    }

  entry_width = gimp_prop_size_entry_num_chars (lower, upper) + 1 +
                gimp_unit_get_scaled_digits (unit_value, resolution);
  entry_width = MAX (entry_width, 7);

  entry = gimp_size_entry_new (1, unit_value, unit_format,
                               show_pixels, show_percent, FALSE,
                               entry_width, update_policy);

  set_param_spec (NULL,
                  gimp_size_entry_get_help_widget (GIMP_SIZE_ENTRY (entry), 0),
                  param_spec);

  if (unit_param_spec)
    set_param_spec (NULL,
                    gimp_size_entry_get_unit_combo  (GIMP_SIZE_ENTRY (entry)),
                    unit_param_spec);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (entry), unit_value);

  if (update_policy == GIMP_SIZE_ENTRY_UPDATE_SIZE)
    gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0,
                                    resolution, FALSE);

  gimp_size_entry_set_value_boundaries (GIMP_SIZE_ENTRY (entry), 0,
                                        lower, upper);

  g_object_set_data (G_OBJECT (entry), "value-is-pixel",
                     GINT_TO_POINTER (property_is_pixel ? TRUE : FALSE));

  if (property_is_pixel)
    gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, value);
  else
    gimp_size_entry_set_value (GIMP_SIZE_ENTRY (entry), 0, value);

  g_object_set_data (G_OBJECT (entry), "gimp-config-param-spec",
                     param_spec);

  g_signal_connect (entry, "refval-changed",
                    G_CALLBACK (gimp_prop_size_entry_callback),
                    config);
  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (gimp_prop_size_entry_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_size_entry_notify),
                  entry);

  if (unit_property_name)
    {
      g_object_set_data (G_OBJECT (entry), "gimp-config-param-spec-unit",
                         unit_param_spec);

      g_signal_connect (entry, "unit-changed",
                        G_CALLBACK (gimp_prop_size_entry_callback),
                        config);

      connect_notify (config, unit_property_name,
                      G_CALLBACK (gimp_prop_size_entry_notify_unit),
                      entry);
    }

  gimp_widget_set_bound_property (entry, config, property_name);

  gtk_widget_show (entry);

  return entry;
}

static void
gimp_prop_size_entry_callback (GimpSizeEntry *entry,
                               GObject       *config)
{
  GParamSpec *param_spec;
  GParamSpec *unit_param_spec;
  gdouble     value;
  gboolean    value_is_pixel;
  GimpUnit   *unit_value;

  param_spec = g_object_get_data (G_OBJECT (entry), "gimp-config-param-spec");
  if (! param_spec)
    return;

  unit_param_spec = g_object_get_data (G_OBJECT (entry),
                                       "gimp-config-param-spec-unit");

  value_is_pixel = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry),
                                                       "value-is-pixel"));

  if (value_is_pixel)
    value = gimp_size_entry_get_refval (entry, 0);
  else
    value = gimp_size_entry_get_value (entry, 0);

  unit_value = gimp_size_entry_get_unit (entry);

  if (unit_param_spec)
    {
      GimpUnit *old_unit;

      g_object_get (config,
                    unit_param_spec->name, &old_unit,
                    NULL);

      if (unit_value == old_unit)
        unit_param_spec = NULL;
    }

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      g_object_set (config,
                    param_spec->name, ROUND (value),

                    unit_param_spec ?
                    unit_param_spec->name : NULL, unit_value,

                    NULL);
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      g_object_set (config,
                    param_spec->name, value,

                    unit_param_spec ?
                    unit_param_spec->name : NULL, unit_value,

                    NULL);
    }
}

static void
gimp_prop_size_entry_notify (GObject       *config,
                             GParamSpec    *param_spec,
                             GimpSizeEntry *entry)
{
  gdouble  value;
  gdouble  entry_value;
  gboolean value_is_pixel;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint int_value;

      g_object_get (config,
                    param_spec->name, &int_value,
                    NULL);

      value = int_value;
    }
  else
    {
      g_object_get (config,
                    param_spec->name, &value,
                    NULL);
    }

  value_is_pixel = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry),
                                                       "value-is-pixel"));

  if (value_is_pixel)
    entry_value = gimp_size_entry_get_refval (entry, 0);
  else
    entry_value = gimp_size_entry_get_value (entry, 0);

  if (value != entry_value)
    {
      g_signal_handlers_block_by_func (entry,
                                       gimp_prop_size_entry_callback,
                                       config);

      if (value_is_pixel)
        gimp_size_entry_set_refval (entry, 0, value);
      else
        gimp_size_entry_set_value (entry, 0, value);

      g_signal_handlers_unblock_by_func (entry,
                                         gimp_prop_size_entry_callback,
                                         config);
    }
}

static void
gimp_prop_size_entry_notify_unit (GObject       *config,
                                  GParamSpec    *param_spec,
                                  GimpSizeEntry *entry)
{
  GimpUnit *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (value != gimp_size_entry_get_unit (entry))
    {
      g_signal_handlers_block_by_func (entry,
                                       gimp_prop_size_entry_callback,
                                       config);

      gimp_size_entry_set_unit (entry, value);

      g_signal_handlers_unblock_by_func (entry,
                                         gimp_prop_size_entry_callback,
                                         config);
    }
}

static gint
gimp_prop_size_entry_num_chars (gdouble lower,
                                gdouble upper)
{
  gint lower_chars = log (fabs (lower)) / log (10);
  gint upper_chars = log (fabs (upper)) / log (10);

  if (lower < 0.0)
    lower_chars++;

  if (upper < 0.0)
    upper_chars++;

  return MAX (lower_chars, upper_chars);
}


/*****************/
/*  coordinates  */
/*****************/

static void   gimp_prop_coordinates_callback    (GimpSizeEntry *entry,
                                                 GObject       *config);
static void   gimp_prop_coordinates_notify_x    (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 GimpSizeEntry *entry);
static void   gimp_prop_coordinates_notify_y    (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 GimpSizeEntry *entry);
static void   gimp_prop_coordinates_notify_unit (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 GimpSizeEntry *entry);


/**
 * gimp_prop_coordinates_new:
 * @config:             Object to which property is attached.
 * @x_property_name:    Name of int or double property for X coordinate.
 * @y_property_name:    Name of int or double property for Y coordinate.
 * @unit_property_name: Name of unit property.
 * @unit_format:        A printf-like unit-format string as is used with
 *                      gimp_unit_menu_new().
 * @update_policy:      How the automatic pixel <-> real-world-unit
 *                      calculations should be done.
 * @xresolution:        The resolution (in dpi) for the X coordinate.
 * @yresolution:        The resolution (in dpi) for the Y coordinate.
 * @has_chainbutton:    Whether to add a chainbutton to the size entry.
 *
 * Creates a #GimpSizeEntry to set and display two double or int
 * properties, which will usually represent X and Y coordinates, and
 * their associated unit property.
 *
 * If @unit_format is %NULL, the unit will default to inch. Otherwise it
 * must be the name of a property of type %GimpParamUnit:
 *
 * Returns: (transfer full): A new #GimpSizeEntry widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_coordinates_new (GObject                   *config,
                           const gchar               *x_property_name,
                           const gchar               *y_property_name,
                           const gchar               *unit_property_name,
                           const gchar               *unit_format,
                           GimpSizeEntryUpdatePolicy  update_policy,
                           gdouble                    xresolution,
                           gdouble                    yresolution,
                           gboolean                   has_chainbutton)
{
  GimpUnit  *unit_type;
  GtkWidget *entry;
  GtkWidget *chainbutton     = NULL;
  gboolean   show_pixels     = FALSE;
  gboolean   show_percents   = FALSE;
  gboolean   show_resolution = TRUE;

  if (unit_property_name != NULL)
    {
      GParamSpec *pspec_unit = NULL;

      pspec_unit = check_param_spec_w (config, unit_property_name,
                                       GIMP_TYPE_PARAM_UNIT, G_STRFUNC);

      g_return_val_if_fail (pspec_unit != NULL, NULL);

      show_pixels   = gimp_param_spec_unit_pixel_allowed (pspec_unit);
      show_percents = gimp_param_spec_unit_percent_allowed (pspec_unit);

      if (show_pixels)
        show_resolution = FALSE;

      g_object_get (config, unit_property_name, &unit_type, NULL);
    }
  else
    {
      unit_type = gimp_unit_inch ();
    }

  entry = gimp_size_entry_new (2, unit_type, unit_format,
                               show_pixels, show_percents, show_resolution,
                               10, update_policy);

  if (has_chainbutton)
    {
      chainbutton = gimp_chain_button_new (GIMP_CHAIN_BOTTOM);
      gtk_grid_attach (GTK_GRID (entry), chainbutton, 1, 3, 2, 1);
      gtk_widget_show (chainbutton);
    }

  if (! gimp_prop_coordinates_connect (config,
                                       x_property_name,
                                       y_property_name,
                                       unit_property_name,
                                       entry,
                                       chainbutton,
                                       xresolution,
                                       yresolution))
    {
      gtk_widget_destroy (entry);
      return NULL;
    }

  gimp_widget_set_bound_property (entry, config, x_property_name);

  gtk_widget_show (entry);

  return entry;
}

gboolean
gimp_prop_coordinates_connect (GObject     *config,
                               const gchar *x_property_name,
                               const gchar *y_property_name,
                               const gchar *unit_property_name,
                               GtkWidget   *entry,
                               GtkWidget   *chainbutton,
                               gdouble      xresolution,
                               gdouble      yresolution)
{
  GParamSpec *x_param_spec;
  GParamSpec *y_param_spec;
  GParamSpec *unit_param_spec;
  gdouble     x_value, x_lower, x_upper;
  gdouble     y_value, y_lower, y_upper;
  GimpUnit   *unit_value;
  gdouble    *old_x_value;
  gdouble    *old_y_value;
  GimpUnit   *old_unit_value;
  gboolean    chain_checked;

  g_return_val_if_fail (GIMP_IS_SIZE_ENTRY (entry), FALSE);
  g_return_val_if_fail (gimp_size_entry_get_n_fields (GIMP_SIZE_ENTRY (entry)) == 2,
                        FALSE);
  g_return_val_if_fail (chainbutton == NULL ||
                        GIMP_IS_CHAIN_BUTTON (chainbutton), FALSE);

  x_param_spec = find_param_spec (config, x_property_name, G_STRFUNC);
  if (! x_param_spec)
    return FALSE;

  y_param_spec = find_param_spec (config, y_property_name, G_STRFUNC);
  if (! y_param_spec)
    return FALSE;

  if (! get_numeric_values (config, x_param_spec,
                            &x_value, &x_lower, &x_upper, G_STRFUNC) ||
      ! get_numeric_values (config, y_param_spec,
                            &y_value, &y_lower, &y_upper, G_STRFUNC))
    return FALSE;

  if (unit_property_name)
    {
      unit_param_spec = check_param_spec_w (config, unit_property_name,
                                            GIMP_TYPE_PARAM_UNIT, G_STRFUNC);
      if (! unit_param_spec)
        return FALSE;

      g_object_get (config,
                    unit_property_name, &unit_value,
                    NULL);
    }
  else
    {
      unit_param_spec = NULL;
      unit_value      = gimp_unit_inch ();
    }

  set_param_spec (NULL,
                  gimp_size_entry_get_help_widget (GIMP_SIZE_ENTRY (entry), 0),
                  x_param_spec);
  set_param_spec (NULL,
                  gimp_size_entry_get_help_widget (GIMP_SIZE_ENTRY (entry), 1),
                  y_param_spec);

  if (unit_param_spec)
    set_param_spec (NULL,
                    gimp_size_entry_get_unit_combo (GIMP_SIZE_ENTRY (entry)),
                    unit_param_spec);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (entry), unit_value);

  switch (gimp_size_entry_get_update_policy (GIMP_SIZE_ENTRY (entry)))
    {
    case GIMP_SIZE_ENTRY_UPDATE_SIZE:
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0,
                                      xresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 1,
                                      yresolution, FALSE);
      chain_checked = (ABS (x_value - y_value) < 1);
      break;

    case GIMP_SIZE_ENTRY_UPDATE_RESOLUTION:
      chain_checked = (ABS (x_value - y_value) < GIMP_MIN_RESOLUTION);
      break;

    default:
      chain_checked = (x_value == y_value);
      break;
    }

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 0,
                                         x_lower, x_upper);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (entry), 1,
                                         y_lower, y_upper);

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 0, x_value);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (entry), 1, y_value);

  g_object_set_data (G_OBJECT (entry), "gimp-config-param-spec-x",
                     x_param_spec);
  g_object_set_data (G_OBJECT (entry), "gimp-config-param-spec-y",
                     y_param_spec);

  old_x_value  = g_new0 (gdouble, 1);
  *old_x_value = x_value;
  g_object_set_data_full (G_OBJECT (entry), "old-x-value",
                          old_x_value,
                          (GDestroyNotify) g_free);

  old_y_value  = g_new0 (gdouble, 1);
  *old_y_value = y_value;
  g_object_set_data_full (G_OBJECT (entry), "old-y-value",
                          old_y_value,
                          (GDestroyNotify) g_free);

  if (chainbutton)
    {
      if (chain_checked)
        gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chainbutton), TRUE);

      g_object_set_data (G_OBJECT (entry), "chainbutton", chainbutton);
    }

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (gimp_prop_coordinates_callback),
                    config);
  g_signal_connect (entry, "refval-changed",
                    G_CALLBACK (gimp_prop_coordinates_callback),
                    config);

  connect_notify (config, x_property_name,
                  G_CALLBACK (gimp_prop_coordinates_notify_x),
                  entry);
  connect_notify (config, y_property_name,
                  G_CALLBACK (gimp_prop_coordinates_notify_y),
                  entry);

  if (unit_property_name)
    {
      g_object_set_data (G_OBJECT (entry), "gimp-config-param-spec-unit",
                         unit_param_spec);

      old_unit_value = unit_value;
      g_object_set_data_full (G_OBJECT (entry), "old-unit-value",
                              old_unit_value,
                              (GDestroyNotify) NULL);

      g_signal_connect (entry, "unit-changed",
                        G_CALLBACK (gimp_prop_coordinates_callback),
                        config);

      connect_notify (config, unit_property_name,
                      G_CALLBACK (gimp_prop_coordinates_notify_unit),
                      entry);
    }

  return TRUE;
}

static void
gimp_prop_coordinates_callback (GimpSizeEntry *entry,
                                GObject       *config)
{
  GParamSpec *x_param_spec;
  GParamSpec *y_param_spec;
  GParamSpec *unit_param_spec;
  gdouble     x_value;
  gdouble     y_value;
  GimpUnit   *unit_value;
  gdouble    *old_x_value;
  gdouble    *old_y_value;
  GimpUnit   *old_unit_value;
  gboolean    backwards;

  x_param_spec = g_object_get_data (G_OBJECT (entry),
                                    "gimp-config-param-spec-x");
  y_param_spec = g_object_get_data (G_OBJECT (entry),
                                    "gimp-config-param-spec-y");

  if (! x_param_spec || ! y_param_spec)
    return;

  unit_param_spec = g_object_get_data (G_OBJECT (entry),
                                       "gimp-config-param-spec-unit");

  x_value    = gimp_size_entry_get_refval (entry, 0);
  y_value    = gimp_size_entry_get_refval (entry, 1);
  unit_value = gimp_size_entry_get_unit (entry);

  old_x_value    = g_object_get_data (G_OBJECT (entry), "old-x-value");
  old_y_value    = g_object_get_data (G_OBJECT (entry), "old-y-value");
  old_unit_value = g_object_get_data (G_OBJECT (entry), "old-unit-value");

  if (! old_x_value || ! old_y_value || (unit_param_spec && ! old_unit_value))
    return;

  /*
   * FIXME: if the entry was created using gimp_coordinates_new, then
   * the chain button is handled automatically and the following block
   * of code is unnecessary (and, in fact, redundant).
   */
  if (x_value != y_value)
    {
      GtkWidget *chainbutton;

      chainbutton = g_object_get_data (G_OBJECT (entry), "chainbutton");

      if (chainbutton &&
          gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (chainbutton)) &&
          ! g_object_get_data (G_OBJECT (chainbutton), "constrains-ratio"))
        {
          if (x_value != *old_x_value)
            y_value = x_value;
          else if (y_value != *old_y_value)
            x_value = y_value;
        }
    }

  backwards = (*old_x_value == x_value);

  if (*old_x_value == x_value &&
      *old_y_value == y_value &&
      (old_unit_value == NULL || old_unit_value == unit_value))
    return;

  *old_x_value = x_value;
  *old_y_value = y_value;

  if (old_unit_value)
    old_unit_value = unit_value;

  if (unit_param_spec)
    g_object_set (config,
                  unit_param_spec->name, unit_value,
                  NULL);

  if (G_IS_PARAM_SPEC_INT (x_param_spec) &&
      G_IS_PARAM_SPEC_INT (y_param_spec))
    {
      if (backwards)
        g_object_set (config,
                      y_param_spec->name, ROUND (y_value),
                      x_param_spec->name, ROUND (x_value),
                      NULL);
      else
        g_object_set (config,
                      x_param_spec->name, ROUND (x_value),
                      y_param_spec->name, ROUND (y_value),
                      NULL);

    }
  else if (G_IS_PARAM_SPEC_DOUBLE (x_param_spec) &&
           G_IS_PARAM_SPEC_DOUBLE (y_param_spec))
    {
      if (backwards)
        g_object_set (config,
                      y_param_spec->name, y_value,
                      x_param_spec->name, x_value,
                      NULL);
      else
        g_object_set (config,
                      x_param_spec->name, x_value,
                      y_param_spec->name, y_value,
                      NULL);
    }
}

static void
gimp_prop_coordinates_notify_x (GObject       *config,
                                GParamSpec    *param_spec,
                                GimpSizeEntry *entry)
{
  gdouble value;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint int_value;

      g_object_get (config,
                    param_spec->name, &int_value,
                    NULL);

      value = int_value;
    }
  else
    {
      g_object_get (config,
                    param_spec->name, &value,
                    NULL);
    }

  if (value != gimp_size_entry_get_refval (entry, 0))
    {
      gdouble *old_x_value = g_object_get_data (G_OBJECT (entry),
                                                "old-x-value");

      g_signal_handlers_block_by_func (entry,
                                       gimp_prop_coordinates_callback,
                                       config);

      gimp_size_entry_set_refval (entry, 0, value);

      if (old_x_value)
        *old_x_value = value;

      g_signal_emit_by_name (entry, "value-changed",
                             gimp_size_entry_get_value (entry, 0));

      g_signal_handlers_unblock_by_func (entry,
                                         gimp_prop_coordinates_callback,
                                         config);
    }
}

static void
gimp_prop_coordinates_notify_y (GObject       *config,
                                GParamSpec    *param_spec,
                                GimpSizeEntry *entry)
{
  gdouble value;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint int_value;

      g_object_get (config,
                    param_spec->name, &int_value,
                    NULL);

      value = int_value;
    }
  else
    {
      g_object_get (config,
                    param_spec->name, &value,
                    NULL);
    }

  if (value != gimp_size_entry_get_refval (entry, 1))
    {
      gdouble *old_y_value = g_object_get_data (G_OBJECT (entry),
                                                "old-y-value");

      g_signal_handlers_block_by_func (entry,
                                       gimp_prop_coordinates_callback,
                                       config);

      gimp_size_entry_set_refval (entry, 1, value);

      if (old_y_value)
        *old_y_value = value;

      g_signal_emit_by_name (entry, "value-changed",
                             gimp_size_entry_get_value (entry, 1));

      g_signal_handlers_unblock_by_func (entry,
                                         gimp_prop_coordinates_callback,
                                         config);
    }
}

static void
gimp_prop_coordinates_notify_unit (GObject       *config,
                                   GParamSpec    *param_spec,
                                   GimpSizeEntry *entry)
{
  GimpUnit *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (value != gimp_size_entry_get_unit (entry))
    {
      g_signal_handlers_block_by_func (entry,
                                       gimp_prop_coordinates_callback,
                                       config);

      gimp_size_entry_set_unit (entry, value);

      g_signal_handlers_unblock_by_func (entry,
                                         gimp_prop_coordinates_callback,
                                         config);
    }
}


/****************/
/*  color area  */
/****************/

static void   gimp_prop_color_area_callback (GtkWidget  *widget,
                                             GObject    *config);
static void   gimp_prop_color_area_notify   (GObject    *config,
                                             GParamSpec *param_spec,
                                             GtkWidget  *area);

/**
 * gimp_prop_color_area_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of %GeglColor property.
 * @width:         Width of color area.
 * @height:        Height of color area.
 * @type:          How transparency is represented.
 *
 * Creates a #GimpColorArea to set and display the value of an RGB
 * property.
 *
 * Returns: (transfer full): A new #GimpColorArea widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_color_area_new (GObject           *config,
                          const gchar       *property_name,
                          gint               width,
                          gint               height,
                          GimpColorAreaType  type)
{
  GParamSpec *param_spec;
  GtkWidget  *area;
  GeglColor  *color = NULL;

  param_spec = check_param_specs_w (config, property_name, GEGL_TYPE_PARAM_COLOR,
                                    GIMP_TYPE_PARAM_COLOR, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &color,
                NULL);

  area = gimp_color_area_new (color, type, GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
  gtk_widget_set_size_request (area, width, height);

  g_clear_object (&color);

  set_param_spec (G_OBJECT (area), area, param_spec);

  g_signal_connect (area, "color-changed",
                    G_CALLBACK (gimp_prop_color_area_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_color_area_notify),
                  area);

  gimp_widget_set_bound_property (area, config, property_name);

  gtk_widget_show (area);

  return area;
}

static void
gimp_prop_color_area_callback (GtkWidget *area,
                               GObject   *config)
{
  GParamSpec *param_spec;
  GeglColor  *color;

  param_spec = get_param_spec (G_OBJECT (area));
  if (! param_spec)
    return;

  color = gimp_color_area_get_color (GIMP_COLOR_AREA (area));

  g_signal_handlers_block_by_func (config,
                                   gimp_prop_color_area_notify,
                                   area);

  g_object_set (config,
                param_spec->name, color,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     gimp_prop_color_area_notify,
                                     area);
  g_object_unref (color);
}

static void
gimp_prop_color_area_notify (GObject    *config,
                             GParamSpec *param_spec,
                             GtkWidget  *area)
{
  GeglColor *color = NULL;

  g_object_get (config,
                param_spec->name, &color,
                NULL);

  g_signal_handlers_block_by_func (area,
                                   gimp_prop_color_area_callback,
                                   config);

  gimp_color_area_set_color (GIMP_COLOR_AREA (area), color);

  g_clear_object (&color);

  g_signal_handlers_unblock_by_func (area,
                                     gimp_prop_color_area_callback,
                                     config);
}

/**
 * gimp_prop_color_select_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of [class@Gegl.Color] property.
 * @width:         Width of the colorpreview in pixels.
 * @height:        Height of the colorpreview in pixels.
 * @type:          How transparency is represented.
 *
 * Creates a #GimpColorButton to set and display the value of a color
 * property.
 *
 * Returns: (transfer full): A new #GimpColorButton widget.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_color_select_new (GObject           *config,
                            const gchar       *property_name,
                            gint               width,
                            gint               height,
                            GimpColorAreaType  type)
{
  GParamSpec *param_spec;
  GtkWidget  *button;
  GeglColor  *value = NULL;

  param_spec = check_param_specs_w (config, property_name, GEGL_TYPE_PARAM_COLOR,
                                    GIMP_TYPE_PARAM_COLOR, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  button = gimp_color_button_new (g_param_spec_get_nick (param_spec),
                                  width, height, value, type);

  g_clear_object (&value);

  g_object_bind_property (config, property_name,
                          button, "color",
                          G_BINDING_BIDIRECTIONAL);

  gimp_widget_set_bound_property (button, config, property_name);

  gtk_widget_show (button);

  return button;
}

/**
 * gimp_prop_label_color_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of RGB property.
 * @editable:      Whether the widget should allow color editability.
 *
 * Creates a #GimpLabelColor to set and display the value of an RGB
 * property.
 *
 * Returns: (transfer full): A new #GimpLabelColor widget.
 *
 * Since: 3.0
 */
GtkWidget *
gimp_prop_label_color_new (GObject     *config,
                           const gchar *property_name,
                           gboolean     editable)
{
  GParamSpec  *param_spec;
  GtkWidget   *prop_widget;
  const gchar *label;
  GeglColor   *value;

  param_spec = check_param_specs_w (config, property_name, GEGL_TYPE_PARAM_COLOR,
                                    GIMP_TYPE_PARAM_COLOR, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  label = g_param_spec_get_nick (param_spec);

  prop_widget = gimp_label_color_new (label, value, editable);
  g_clear_object (&value);

  g_object_bind_property (config,      property_name,
                          prop_widget, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  gimp_widget_set_bound_property (prop_widget, config, property_name);

  gtk_widget_show (prop_widget);

  return prop_widget;
}

/********************/
/*  unit combo box  */
/********************/

static void   gimp_prop_unit_combo_box_callback (GtkWidget  *combo,
                                                 GObject    *config);
static void   gimp_prop_unit_combo_box_notify   (GObject    *config,
                                                 GParamSpec *param_spec,
                                                 GtkWidget  *combo);

/**
 * gimp_prop_unit_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of Unit property.
 *
 * Creates a #GimpUnitComboBox to set and display the value of a Unit
 * property.  See gimp_unit_combo_box_new() for more information.
 *
 * Returns: (transfer full): A new #GimpUnitComboBox widget.
 *
 * Since: 2.8
 */
GtkWidget *
gimp_prop_unit_combo_box_new (GObject     *config,
                              const gchar *property_name)
{
  GParamSpec   *param_spec;
  GtkWidget    *combo;
  GtkTreeModel *model;
  GimpUnit     *unit;
  GValue        value = G_VALUE_INIT;
  gboolean      show_pixels;
  gboolean      show_percent;

  param_spec = check_param_spec_w (config, property_name,
                                   GIMP_TYPE_PARAM_UNIT, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_value_init (&value, param_spec->value_type);

  g_value_set_object (&value, gimp_unit_pixel ());
  show_pixels = (g_param_value_validate (param_spec, &value) == FALSE);

  g_value_set_object (&value, gimp_unit_percent ());
  show_percent = (g_param_value_validate (param_spec, &value) == FALSE);

  g_value_unset (&value);

  g_object_get (config,
                property_name, &unit,
                NULL);

  combo = gimp_unit_combo_box_new ();
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
  gimp_unit_store_set_has_pixels (GIMP_UNIT_STORE (model), show_pixels);
  gimp_unit_store_set_has_percent (GIMP_UNIT_STORE (model), show_percent);

  gimp_unit_combo_box_set_active (GIMP_UNIT_COMBO_BOX (combo), unit);

  set_param_spec (G_OBJECT (combo), combo, param_spec);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_prop_unit_combo_box_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_unit_combo_box_notify),
                  combo);

  gimp_widget_set_bound_property (combo, config, property_name);

  gtk_widget_show (combo);

  return combo;
}

static void
gimp_prop_unit_combo_box_callback (GtkWidget *combo,
                                   GObject   *config)
{
  GParamSpec *param_spec;
  GimpUnit   *value;
  GimpUnit   *v;

  param_spec = get_param_spec (G_OBJECT (combo));
  if (! param_spec)
    return;

  value = gimp_unit_combo_box_get_active (GIMP_UNIT_COMBO_BOX (combo));

  g_object_get (config, param_spec->name, &v, NULL);

  if (v != value)
    {
      /* FIXME gimp_unit_menu_update (menu, &unit); */

      g_signal_handlers_block_by_func (config,
                                       gimp_prop_unit_combo_box_notify,
                                       combo);

      g_object_set (config, param_spec->name, value, NULL);

      g_signal_handlers_unblock_by_func (config,
                                         gimp_prop_unit_combo_box_notify,
                                         combo);
    }
}

static void
gimp_prop_unit_combo_box_notify (GObject    *config,
                                 GParamSpec *param_spec,
                                 GtkWidget  *combo)
{
  GimpUnit *unit;

  g_object_get (config,
                param_spec->name, &unit,
                NULL);

  g_signal_handlers_block_by_func (combo,
                                   gimp_prop_unit_combo_box_callback,
                                   config);

  gimp_unit_combo_box_set_active (GIMP_UNIT_COMBO_BOX (combo), unit);

  /* FIXME gimp_unit_menu_update (menu, &unit); */

  g_signal_handlers_unblock_by_func (combo,
                                     gimp_prop_unit_combo_box_callback,
                                     config);
}


/***************/
/*  icon name  */
/***************/

/**
 * gimp_prop_icon_image_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of string property.
 * @icon_size:     Size of desired icon image.
 *
 * Creates a widget to display a icon image representing the value of the
 * specified string property, which should encode an icon name.
 * See gtk_image_new_from_icon_name() for more information.
 *
 * Returns: (transfer full): A new #GtkImage widget.
 *
 * Since: 2.10
 */
GtkWidget *
gimp_prop_icon_image_new (GObject     *config,
                          const gchar *property_name,
                          GtkIconSize  icon_size)
{
  GParamSpec  *param_spec;
  GtkWidget   *image;
  gchar       *icon_name;
  const gchar *blurb;

  param_spec = check_param_spec (config, property_name,
                                 G_TYPE_PARAM_STRING, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &icon_name,
                NULL);

  image = gtk_image_new_from_icon_name (icon_name, icon_size);
  gtk_widget_show (image);

  blurb = g_param_spec_get_blurb (param_spec);
  if (blurb)
    gimp_help_set_help_data (image, blurb, NULL);

  g_object_bind_property (config, property_name,
                          image, "icon-name",
                          G_BINDING_BIDIRECTIONAL);

  gimp_widget_set_bound_property (image, config, property_name);

  g_free (icon_name);

  return image;
}


/**************/
/*  expander  */
/**************/

static void   gimp_prop_expanded_notify (GtkExpander *expander,
                                         GParamSpec  *param_spec,
                                         GObject     *config);
static void   gimp_prop_expander_notify (GObject     *config,
                                         GParamSpec  *param_spec,
                                         GtkExpander *expander);


/**
 * gimp_prop_expander_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of boolean property.
 * @label: (nullable): Label for expander.
 *
 * Creates a #GtkExpander controlled by the specified boolean property.
 * A value of %TRUE for the property corresponds to the expanded state
 * for the widget.
 * If @label is %NULL, the @property_name's nick will be used as label
 * of the returned widget.
 *
 * Returns: (transfer full): A new #GtkExpander widget.
 *
 * Since: 2.4
 */
GtkWidget *
gimp_prop_expander_new (GObject     *config,
                        const gchar *property_name,
                        const gchar *label)
{
  GParamSpec *param_spec;
  GtkWidget  *expander;
  gboolean    value;

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! label)
    label = g_param_spec_get_nick (param_spec);

  g_object_get (config,
                property_name, &value,
                NULL);

  expander = g_object_new (GTK_TYPE_EXPANDER,
                           "label",    label,
                           "expanded", value,
                           NULL);

  set_param_spec (G_OBJECT (expander), expander, param_spec);

  g_signal_connect (expander, "notify::expanded",
                    G_CALLBACK (gimp_prop_expanded_notify),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (gimp_prop_expander_notify),
                  expander);

  gimp_widget_set_bound_property (expander, config, property_name);

  gtk_widget_show (expander);

  return expander;
}

static void
gimp_prop_expanded_notify (GtkExpander *expander,
                           GParamSpec  *param_spec,
                           GObject     *config)
{
  param_spec = get_param_spec (G_OBJECT (expander));
  if (! param_spec)
    return;

  g_object_set (config,
                param_spec->name, gtk_expander_get_expanded (expander),
                NULL);
}

static void
gimp_prop_expander_notify (GObject     *config,
                           GParamSpec  *param_spec,
                           GtkExpander *expander)
{
  gboolean value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (gtk_expander_get_expanded (expander) != value)
    {
      g_signal_handlers_block_by_func (expander,
                                       gimp_prop_expanded_notify,
                                       config);

      gtk_expander_set_expanded (expander, value);

      g_signal_handlers_unblock_by_func (expander,
                                         gimp_prop_expanded_notify,
                                         config);
    }
}


/*******************************/
/*  private utility functions  */
/*******************************/

static GQuark gimp_prop_widgets_param_spec_quark (void) G_GNUC_CONST;

#define PARAM_SPEC_QUARK (gimp_prop_widgets_param_spec_quark ())

static GQuark
gimp_prop_widgets_param_spec_quark (void)
{
  static GQuark param_spec_quark = 0;

  if (! param_spec_quark)
    param_spec_quark = g_quark_from_static_string ("gimp-config-param-spec");

  return param_spec_quark;
}

static void
set_param_spec (GObject     *object,
                GtkWidget   *widget,
                GParamSpec  *param_spec)
{
  if (object)
    {
      g_object_set_qdata (object, PARAM_SPEC_QUARK, param_spec);
    }

  if (widget)
    {
      const gchar *blurb = g_param_spec_get_blurb (param_spec);

      if (blurb)
        gimp_help_set_help_data (widget, blurb, NULL);
    }
}

static void
set_radio_spec (GObject    *object,
                GParamSpec *param_spec)
{
  GSList *list;

  for (list = gtk_radio_button_get_group (GTK_RADIO_BUTTON (object));
       list;
       list = g_slist_next (list))
    {
      set_param_spec (list->data, NULL, param_spec);
    }
}

static GParamSpec *
get_param_spec (GObject *object)
{
  return g_object_get_qdata (object, PARAM_SPEC_QUARK);
}

static GParamSpec *
find_param_spec (GObject     *object,
                 const gchar *property_name,
                 const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                             property_name);

  if (! param_spec)
    g_warning ("%s: %s has no property named '%s'",
               strloc,
               g_type_name (G_TYPE_FROM_INSTANCE (object)),
               property_name);

  return param_spec;
}

/* Compare GType of GParamSpec of object's property to GType.
 * Return GParamSpec when equal, else NULL.
 */
static GParamSpec *
check_param_spec_quiet (GObject     *object,
                        const gchar *property_name,
                        GType        type,
                        const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = find_param_spec (object, property_name, strloc);

  if (param_spec && ! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), type))
    return NULL;
  else
    return param_spec;
}

static GParamSpec *
check_param_spec (GObject     *object,
                  const gchar *property_name,
                  GType        type,
                  const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = find_param_spec (object, property_name, strloc);

  if (param_spec && ! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), type))
    {
      g_warning ("%s: property '%s' of %s is not a %s but a %s",
                 strloc,
                 param_spec->name,
                 g_type_name (param_spec->owner_type),
                 g_type_name (type),
                 g_type_name (G_TYPE_FROM_INSTANCE (param_spec)));
      return NULL;
    }

  return param_spec;
}

static GParamSpec *
check_param_spec_w (GObject     *object,
                    const gchar *property_name,
                    GType        type,
                    const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = check_param_spec (object, property_name, type, strloc);

  if (param_spec &&
      (param_spec->flags & G_PARAM_WRITABLE) == 0)
    {
      g_warning ("%s: property '%s' of %s is not writable",
                 strloc,
                 param_spec->name,
                 g_type_name (param_spec->owner_type));
      return NULL;
    }

  return param_spec;
}

static GParamSpec *
check_param_specs_w (GObject     *object,
                     const gchar *property_name,
                     GType        type1,
                     GType        type2,
                     const gchar *strloc)
{
  GParamSpec *param_spec;

  param_spec = check_param_spec_quiet (object, property_name, type1, strloc);
  if (param_spec)
    {
      param_spec = check_param_spec_w (object, property_name, type1, strloc);

      if (! param_spec)
        return NULL;
    }
  else
    {
      param_spec = check_param_spec_quiet (object, property_name, type2, strloc);
      if (param_spec)
        {
          param_spec = check_param_spec_w (object, property_name, type2, strloc);
          if (! param_spec)
            return NULL;
        }
    }

  if (! param_spec)
    g_warning ("%s: property '%s' of %s must be of type %s or %s.",
               strloc, property_name,
               g_type_name (G_TYPE_FROM_INSTANCE (object)),
               g_type_name (type1),
               g_type_name (type2));

  return param_spec;
}

static gboolean
get_numeric_values (GObject     *object,
                    GParamSpec  *param_spec,
                    gdouble     *value,
                    gdouble     *lower,
                    gdouble     *upper,
                    const gchar *strloc)
{
  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      GParamSpecInt *int_spec = G_PARAM_SPEC_INT (param_spec);
      gint           int_value;

      g_object_get (object, param_spec->name, &int_value, NULL);

      *value = int_value;
      *lower = int_spec->minimum;
      *upper = int_spec->maximum;
    }
  else if (G_IS_PARAM_SPEC_UINT (param_spec))
    {
      GParamSpecUInt *uint_spec = G_PARAM_SPEC_UINT (param_spec);
      guint           uint_value;

      g_object_get (object, param_spec->name, &uint_value, NULL);

      *value = uint_value;
      *lower = uint_spec->minimum;
      *upper = uint_spec->maximum;
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      GParamSpecDouble *double_spec = G_PARAM_SPEC_DOUBLE (param_spec);

      g_object_get (object, param_spec->name, value, NULL);

      *lower = double_spec->minimum;
      *upper = double_spec->maximum;
    }
  else
    {
      g_warning ("%s: property '%s' of %s is not numeric",
                 strloc,
                 param_spec->name,
                 g_type_name (G_TYPE_FROM_INSTANCE (object)));
      return FALSE;
    }

  return TRUE;
}

static void
connect_notify (GObject     *config,
                const gchar *property_name,
                GCallback    callback,
                gpointer     callback_data)
{
  gchar *notify_name;

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name, callback, callback_data, 0);

  g_free (notify_name);
}


static gboolean
gimp_prop_widget_double_to_factor (GBinding     *binding,
                                   const GValue *from_value,
                                   GValue       *to_value,
                                   gpointer      user_data)
{
  gdouble *factor = (gdouble*) user_data;
  gdouble  val    = g_value_get_double (from_value);

  g_value_set_double (to_value, val * (*factor));

  return TRUE;
}

static gboolean
gimp_prop_widget_double_from_factor (GBinding     *binding,
                                     const GValue *from_value,
                                     GValue       *to_value,
                                     gpointer      user_data)
{
  gdouble *factor = (gdouble*) user_data;
  gdouble val     = g_value_get_double (from_value);

  g_value_set_double (to_value, val / (*factor));

  return TRUE;
}
