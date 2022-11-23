/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapropwidgets.c
 * Copyright (C) 2002-2007  Michael Natterer <mitch@ligma.org>
 *                          Sven Neumann <sven@ligma.org>
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

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"
#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "ligmawidgets.h"
#include "ligmawidgets-private.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmapropwidgets
 * @title: LigmaPropWidgets
 * @short_description: Editable views on #GObject properties.
 *
 * Editable views on #GObject properties.
 **/


/*  utility function prototypes  */

static void         set_param_spec     (GObject     *object,
                                        GtkWidget   *widget,
                                        GParamSpec  *param_spec);
static void         set_radio_spec     (GObject     *object,
                                        GParamSpec  *param_spec);
static GParamSpec * get_param_spec     (GObject     *object);

static GParamSpec * find_param_spec    (GObject     *object,
                                        const gchar *property_name,
                                        const gchar *strloc);
static GParamSpec * check_param_spec_quiet (
                                        GObject     *object,
                                        const gchar *property_name,
                                        GType        type,
                                        const gchar *strloc);
static GParamSpec * check_param_spec   (GObject     *object,
                                        const gchar *property_name,
                                        GType        type,
                                        const gchar *strloc);
static GParamSpec * check_param_spec_w (GObject     *object,
                                        const gchar *property_name,
                                        GType        type,
                                        const gchar *strloc);

static gboolean     get_numeric_values (GObject     *object,
                                        GParamSpec  *param_spec,
                                        gdouble     *value,
                                        gdouble     *lower,
                                        gdouble     *upper,
                                        const gchar *strloc);

static void         connect_notify     (GObject     *config,
                                        const gchar *property_name,
                                        GCallback    callback,
                                        gpointer     callback_data);

static gboolean ligma_prop_widget_double_to_factor   (GBinding     *binding,
                                                     const GValue *from_value,
                                                     GValue       *to_value,
                                                     gpointer      user_data);
static gboolean ligma_prop_widget_double_from_factor (GBinding     *binding,
                                                     const GValue *from_value,
                                                     GValue       *to_value,
                                                     gpointer      user_data);


/******************/
/*  check button  */
/******************/

/**
 * ligma_prop_check_button_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of boolean property controlled by checkbutton.
 * @label:         Label to give checkbutton (including mnemonic).
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
ligma_prop_check_button_new (GObject     *config,
                            const gchar *property_name,
                            const gchar *label)
{
  GParamSpec  *param_spec;
  GtkWidget   *button;
  const gchar *blurb;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_BOOLEAN, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! label)
    label = g_param_spec_get_nick (param_spec);

  button = gtk_check_button_new_with_mnemonic (label);
  gtk_widget_show (button);

  blurb = g_param_spec_get_blurb (param_spec);
  if (blurb)
    ligma_help_set_help_data (button, blurb, NULL);

  g_object_bind_property (config, property_name,
                          button, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  ligma_widget_set_bound_property (button, config, property_name);

  return button;
}


static void   ligma_prop_enum_check_button_callback (GtkWidget  *widget,
                                                    GObject    *config);
static void   ligma_prop_enum_check_button_notify   (GObject    *config,
                                                    GParamSpec *param_spec,
                                                    GtkWidget  *button);

/**
 * ligma_prop_enum_check_button_new:
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
ligma_prop_enum_check_button_new (GObject     *config,
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
                    G_CALLBACK (ligma_prop_enum_check_button_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_enum_check_button_notify),
                  button);

  gtk_widget_show (button);

  ligma_widget_set_bound_property (button, config, property_name);

  return button;
}

static void
ligma_prop_enum_check_button_callback (GtkWidget *widget,
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
ligma_prop_enum_check_button_notify (GObject    *config,
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
                                       ligma_prop_enum_check_button_callback,
                                       config);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), active);

      g_signal_handlers_unblock_by_func (button,
                                         ligma_prop_enum_check_button_callback,
                                         config);
    }
}


/******************/
/*     switch     */
/******************/


/**
 * ligma_prop_switch_new:
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
ligma_prop_switch_new (GObject     *config,
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
  ligma_help_set_help_data (pswitch, tooltip, NULL);
  gtk_widget_show (pswitch);

  plabel = gtk_label_new_with_mnemonic (label);
  gtk_label_set_xalign (GTK_LABEL (plabel), 0.0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (plabel), pswitch);
  ligma_help_set_help_data (plabel, tooltip, NULL);
  gtk_widget_show (plabel);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), plabel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), pswitch, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  if (label_out)
    *label_out = plabel;
  if (switch_out)
    *switch_out = pswitch;

  ligma_widget_set_bound_property (hbox, config, property_name);

  return hbox;
}


/*************************/
/*  int/enum combo box   */
/*************************/

static void ligma_prop_int_combo_box_callback     (GtkWidget  *widget,
                                                  GObject    *config);
static void ligma_prop_int_combo_box_notify       (GObject    *config,
                                                  GParamSpec *param_spec,
                                                  GtkWidget  *widget);

static void ligma_prop_pointer_combo_box_callback (GtkWidget  *widget,
                                                  GObject    *config);
static void ligma_prop_pointer_combo_box_notify   (GObject    *config,
                                                  GParamSpec *param_spec,
                                                  GtkWidget  *combo_box);

/**
 * ligma_prop_int_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of int property controlled by combo box.
 * @store:         #LigmaIntStore holding list of labels, values, etc.
 *
 * Creates a #LigmaIntComboBox widget to display and set the specified
 * property.  The contents of the widget are determined by @store,
 * which should be created using ligma_int_store_new().
 *
 * Returns: (transfer full): The newly created #LigmaIntComboBox widget.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_int_combo_box_new (GObject      *config,
                             const gchar  *property_name,
                             LigmaIntStore *store)
{
  GParamSpec  *param_spec;
  GtkWidget   *combo_box;
  const gchar *blurb;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  /* Require property is integer valued type: INT or ENUM, and is writeable. */
  param_spec = check_param_spec_quiet (config, property_name,
                                       G_TYPE_PARAM_INT, G_STRFUNC);
  if (param_spec)
    {
      param_spec = check_param_spec_w (config, property_name,
                                     G_TYPE_PARAM_INT, G_STRFUNC);
      if (! param_spec)
        return NULL;
    }
  else
    {
      param_spec = check_param_spec_quiet (config, property_name,
                                          G_TYPE_PARAM_ENUM, G_STRFUNC);
      if (param_spec)
        {
          param_spec = check_param_spec_w (config, property_name,
                                           G_TYPE_PARAM_ENUM, G_STRFUNC);
          if (! param_spec)
            return NULL;
        }
      else
        {
          g_warning ("%s: property '%s' of %s is not integer valued.",
                     G_STRFUNC,
                     param_spec->name,
                     g_type_name (param_spec->owner_type));
          return NULL;
        }
    }

  combo_box = g_object_new (LIGMA_TYPE_INT_COMBO_BOX,
                            "model", store,
                            "visible", TRUE,
                            NULL);

  blurb = g_param_spec_get_blurb (param_spec);
  if (blurb)
    ligma_help_set_help_data (combo_box, blurb, NULL);

  g_object_bind_property (config, property_name,
                          combo_box, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  ligma_widget_set_bound_property (combo_box, config, property_name);

  return combo_box;
}

/**
 * ligma_prop_pointer_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of GType/gpointer property controlled by combo box.
 * @store:         #LigmaIntStore holding list of labels, values, etc.
 *
 * Creates a #LigmaIntComboBox widget to display and set the specified
 * property.  The contents of the widget are determined by @store,
 * which should be created using ligma_int_store_new().
 * Values are GType/gpointer data, and therefore must be stored in the
 * "user-data" column, instead of the usual "value" column.
 *
 * Returns: (transfer full): The newly created #LigmaIntComboBox widget.
 *
 * Since: 2.10
 */
GtkWidget *
ligma_prop_pointer_combo_box_new (GObject      *config,
                                 const gchar  *property_name,
                                 LigmaIntStore *store)
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

  /* We use a LigmaIntComboBox but we cannot store gpointer in the
   * "value" column, because gpointer is not a subset of gint. Instead
   * we store the value in the "user-data" column which is a gpointer.
   */
  combo_box = g_object_new (LIGMA_TYPE_INT_COMBO_BOX,
                            "model", store,
                            NULL);

  ligma_int_combo_box_set_active_by_user_data (LIGMA_INT_COMBO_BOX (combo_box),
                                              property_value);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (ligma_prop_pointer_combo_box_callback),
                    config);

  set_param_spec (G_OBJECT (combo_box), combo_box, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_pointer_combo_box_notify),
                  combo_box);

  ligma_widget_set_bound_property (combo_box, config, property_name);

  gtk_widget_show (combo_box);

  return combo_box;
}

/**
 * ligma_prop_enum_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of enum property controlled by combo box.
 * @minimum:       Smallest allowed value of enum.
 * @maximum:       Largest allowed value of enum.
 *
 * Creates a #LigmaIntComboBox widget to display and set the specified
 * enum property.  The @mimimum_value and @maximum_value give the
 * possibility of restricting the allowed range to a subset of the
 * enum.  If the two values are equal (e.g., 0, 0), then the full
 * range of the Enum is used.
 *
 * Returns: (transfer full): The newly created #LigmaEnumComboBox widget.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_enum_combo_box_new (GObject     *config,
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
      store = ligma_enum_store_new_with_range (param_spec->value_type,
                                              minimum, maximum);
    }
  else if (param_spec->value_type == LIGMA_TYPE_DESATURATE_MODE)
    {
      /* this is a bad hack, if we get more of those, we should probably
       * think of something less ugly
       */
      store = ligma_enum_store_new_with_values (param_spec->value_type,
                                               5,
                                               LIGMA_DESATURATE_LUMINANCE,
                                               LIGMA_DESATURATE_LUMA,
                                               LIGMA_DESATURATE_LIGHTNESS,
                                               LIGMA_DESATURATE_AVERAGE,
                                               LIGMA_DESATURATE_VALUE);
    }

  if (store)
    {
      combo_box = g_object_new (LIGMA_TYPE_ENUM_COMBO_BOX,
                                "model", store,
                                NULL);
      g_object_unref (store);
    }
  else
    {
      combo_box = ligma_enum_combo_box_new (param_spec->value_type);
    }

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo_box), value);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (ligma_prop_int_combo_box_callback),
                    config);

  set_param_spec (G_OBJECT (combo_box), combo_box, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_int_combo_box_notify),
                  combo_box);

  gtk_widget_show (combo_box);

  ligma_widget_set_bound_property (combo_box, config, property_name);

  return combo_box;
}

static void
ligma_prop_int_combo_box_callback (GtkWidget *widget,
                                  GObject   *config)
{
  GParamSpec  *param_spec;
  gint         value;

  param_spec = get_param_spec (G_OBJECT (widget));
  if (! param_spec)
    return;

  if (ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget), &value))
    {
      gint v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
ligma_prop_int_combo_box_notify (GObject    *config,
                                GParamSpec *param_spec,
                                GtkWidget  *combo_box)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo_box,
                                   ligma_prop_int_combo_box_callback,
                                   config);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo_box), value);

  g_signal_handlers_unblock_by_func (combo_box,
                                     ligma_prop_int_combo_box_callback,
                                     config);
}

static void
ligma_prop_pointer_combo_box_callback (GtkWidget *widget,
                                      GObject   *config)
{
  GParamSpec *param_spec;
  gpointer    value;

  param_spec = get_param_spec (G_OBJECT (widget));
  if (! param_spec)
    return;

  if (ligma_int_combo_box_get_active_user_data (LIGMA_INT_COMBO_BOX (widget),
                                               &value))
    {
      gpointer v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
ligma_prop_pointer_combo_box_notify (GObject    *config,
                                    GParamSpec *param_spec,
                                    GtkWidget  *combo_box)
{
  gpointer value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo_box,
                                   ligma_prop_pointer_combo_box_callback,
                                   config);

  ligma_int_combo_box_set_active_by_user_data (LIGMA_INT_COMBO_BOX (combo_box),
                                              value);

  g_signal_handlers_unblock_by_func (combo_box,
                                     ligma_prop_pointer_combo_box_callback,
                                     config);
}

/************************/
/*  boolean combo box   */
/************************/

static void   ligma_prop_boolean_combo_box_callback (GtkWidget   *combo,
                                                    GObject     *config);
static void   ligma_prop_boolean_combo_box_notify   (GObject     *config,
                                                    GParamSpec  *param_spec,
                                                    GtkWidget   *combo);


/**
 * ligma_prop_boolean_combo_box_new:
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
ligma_prop_boolean_combo_box_new (GObject     *config,
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

  combo = ligma_int_combo_box_new (true_text,  TRUE,
                                  false_text, FALSE,
                                  NULL);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo), value);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ligma_prop_boolean_combo_box_callback),
                    config);

  set_param_spec (G_OBJECT (combo), combo, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_boolean_combo_box_notify),
                  combo);

  ligma_widget_set_bound_property (combo, config, property_name);

  gtk_widget_show (combo);

  return combo;
}

static void
ligma_prop_boolean_combo_box_callback (GtkWidget *combo,
                                      GObject   *config)
{
  GParamSpec  *param_spec;
  gint         value;

  param_spec = get_param_spec (G_OBJECT (combo));
  if (! param_spec)
    return;

  if (ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo), &value))
    {
      gint v;

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
ligma_prop_boolean_combo_box_notify (GObject    *config,
                                    GParamSpec *param_spec,
                                    GtkWidget  *combo)
{
  gboolean value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo,
                                   ligma_prop_boolean_combo_box_callback,
                                   config);

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo), value);

  g_signal_handlers_unblock_by_func (combo,
                                     ligma_prop_boolean_combo_box_callback,
                                     config);
}


/*****************/
/*  radio boxes  */
/*****************/

static void  ligma_prop_radio_button_callback (GtkWidget   *widget,
                                              GObject     *config);
static void  ligma_prop_radio_button_notify   (GObject     *config,
                                              GParamSpec  *param_spec,
                                              GtkWidget   *button);


/**
 * ligma_prop_enum_radio_frame_new:
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
 * Returns: (transfer full): A #LigmaFrame containing the radio buttons.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_enum_radio_frame_new (GObject     *config,
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
      frame = ligma_enum_radio_frame_new_with_range (param_spec->value_type,
                                                    minimum, maximum,
                                                    gtk_label_new (title),
                                                    G_CALLBACK (ligma_prop_radio_button_callback),
                                                    config, NULL,
                                                    &button);
    }
  else
    {
      frame = ligma_enum_radio_frame_new (param_spec->value_type,
                                         gtk_label_new (title),
                                         G_CALLBACK (ligma_prop_radio_button_callback),
                                         config, NULL,
                                         &button);
    }

  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);

  set_radio_spec (G_OBJECT (button), param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_radio_button_notify),
                  button);

  g_object_set_data (G_OBJECT (frame), "radio-button", button);

  gtk_widget_show (frame);

  ligma_widget_set_bound_property (frame, config, property_name);

  return frame;
}

/**
 * ligma_prop_enum_radio_box_new:
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
 * ligma_prop_enum_radio_frame_new() instead of this function.
 *
 * Returns: (transfer full): A #GtkBox containing the radio buttons.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_enum_radio_box_new (GObject     *config,
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
      vbox = ligma_enum_radio_box_new_with_range (param_spec->value_type,
                                                 minimum, maximum,
                                                 G_CALLBACK (ligma_prop_radio_button_callback),
                                                 config, NULL,
                                                 &button);
    }
  else
    {
      vbox = ligma_enum_radio_box_new (param_spec->value_type,
                                      G_CALLBACK (ligma_prop_radio_button_callback),
                                      config, NULL,
                                      &button);
    }

  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);

  set_radio_spec (G_OBJECT (button), param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_radio_button_notify),
                  button);

  g_object_set_data (G_OBJECT (vbox), "radio-button", button);

  ligma_widget_set_bound_property (vbox, config, property_name);

  gtk_widget_show (vbox);

  return vbox;
}

/**
 * ligma_prop_int_radio_frame_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of enum property controlled by the radio buttons.
 * @title: (nullable): Label for the frame holding the buttons
 * @store:         #LigmaIntStore holding list of labels, values, etc.
 *
 * Creates a group of radio buttons which function to set and display
 * the specified int property. If @title is %NULL, the
 * @property_name's nick will be used as label of the returned frame.
 *
 * Returns: (transfer full): A #LigmaFrame containing the radio buttons.
 *
 * Since: 3.0
 */
GtkWidget *
ligma_prop_int_radio_frame_new (GObject      *config,
                               const gchar  *property_name,
                               const gchar  *title,
                               LigmaIntStore *store)
{
  GParamSpec  *param_spec;
  const gchar *tooltip;
  GtkWidget   *frame;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_INT_STORE (store), NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   G_TYPE_PARAM_INT, G_STRFUNC);
  if (! param_spec)
    return NULL;

  if (! title)
    title = g_param_spec_get_nick (param_spec);

  tooltip = g_param_spec_get_blurb (param_spec);

  frame = ligma_int_radio_frame_new_from_store (title, store);
  g_object_bind_property (config, property_name,
                          frame, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  ligma_help_set_help_data (frame, tooltip, NULL);
  gtk_widget_show (frame);

  ligma_widget_set_bound_property (frame, config, property_name);

  return frame;
}


/***********/
/*  label  */
/***********/

static void  ligma_prop_enum_label_notify (GObject    *config,
                                          GParamSpec *param_spec,
                                          GtkWidget  *label);

/**
 * ligma_prop_enum_label_new:
 * @config:         Object to which property is attached.
 * @property_name:  Name of enum property to be displayed.
 *
 * Returns: (transfer full): The newly created #LigmaEnumLabel widget.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_enum_label_new (GObject     *config,
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

  label = ligma_enum_label_new (param_spec->value_type, value);
  gtk_widget_set_halign (label, GTK_ALIGN_START);

  set_param_spec (G_OBJECT (label), label, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_enum_label_notify),
                  label);

  ligma_widget_set_bound_property (label, config, property_name);

  gtk_widget_show (label);

  return label;
}

static void
ligma_prop_enum_label_notify (GObject    *config,
                             GParamSpec *param_spec,
                             GtkWidget  *label)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  ligma_enum_label_set_value (LIGMA_ENUM_LABEL (label), value);
}


/**
 * ligma_prop_boolean_radio_frame_new:
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
 * Returns: (transfer full): A #LigmaFrame containing the radio buttons.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_boolean_radio_frame_new (GObject     *config,
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
    ligma_int_radio_group_new (TRUE, title,
                              G_CALLBACK (ligma_prop_radio_button_callback),
                              config, NULL, value,

                              false_text, FALSE, &button,
                              true_text,  TRUE,  NULL,

                              NULL);

  set_radio_spec (G_OBJECT (button), param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_radio_button_notify),
                  button);

  g_object_set_data (G_OBJECT (frame), "radio-button", button);

  ligma_widget_set_bound_property (frame, config, property_name);

  gtk_widget_show (frame);

  return frame;
}

/**
 * ligma_prop_enum_icon_box_new:
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
 * ligma_enum_icon_box_new() for more information.
 *
 * Returns: (transfer full): A #libligmawidgets-ligmaenumiconbox containing the radio buttons.
 *
 * Since: 2.10
 */
GtkWidget *
ligma_prop_enum_icon_box_new (GObject     *config,
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
      box = ligma_enum_icon_box_new_with_range (param_spec->value_type,
                                               minimum, maximum,
                                               icon_prefix,
                                               GTK_ICON_SIZE_MENU,
                                               G_CALLBACK (ligma_prop_radio_button_callback),
                                               config, NULL,
                                               &button);
    }
  else
    {
      box = ligma_enum_icon_box_new (param_spec->value_type,
                                    icon_prefix,
                                    GTK_ICON_SIZE_MENU,
                                    G_CALLBACK (ligma_prop_radio_button_callback),
                                    config, NULL,
                                    &button);
    }

  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);

  set_radio_spec (G_OBJECT (button), param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_radio_button_notify),
                  button);

  ligma_widget_set_bound_property (box, config, property_name);

  gtk_widget_show (box);

  return box;
}

static void
ligma_prop_radio_button_callback (GtkWidget *widget,
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
                                                  "ligma-item-data"));

      g_object_get (config, param_spec->name, &v, NULL);

      if (v != value)
        g_object_set (config, param_spec->name, value, NULL);
    }
}

static void
ligma_prop_radio_button_notify (GObject    *config,
                               GParamSpec *param_spec,
                               GtkWidget  *button)
{
  gint value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (button), value);
}


/*****************/
/*  adjustments  */
/*****************/

static void   ligma_prop_adjustment_callback (GtkAdjustment *adjustment,
                                             GObject       *config);
static void   ligma_prop_adjustment_notify   (GObject       *config,
                                             GParamSpec    *param_spec,
                                             GtkAdjustment *adjustment);

/**
 * ligma_prop_spin_button_new:
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
 * @property_name's range, use ligma_prop_widget_set_factor().
 *
 * Returns: (transfer full): A new #libligmawidgets-ligmaspinbutton.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_spin_button_new (GObject     *config,
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

  spinbutton = ligma_spin_button_new (adjustment, step_increment, digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  set_param_spec (G_OBJECT (adjustment), spinbutton, param_spec);

  binding = g_object_bind_property (config,     property_name,
                                    spinbutton, "value",
                                    G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_set_data (G_OBJECT (adjustment),
                     "ligma-prop-adjustment-binding",
                     binding);

  ligma_widget_set_bound_property (spinbutton, config, property_name);

  gtk_widget_show (spinbutton);

  return spinbutton;
}

/**
 * ligma_prop_label_spin_new:
 * @config:            Object to which property is attached.
 * @digits:            Number of digits after decimal point to display.
 *
 * Creates a #LigmaLabelSpin to set and display the value of the
 * specified double property.
 *
 * Returns: (transfer full): A new #libligmawidgets-ligmaspinbutton.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_label_spin_new (GObject     *config,
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
  widget = ligma_label_spin_new (label, value, lower, upper, digits);

  g_object_bind_property (config, property_name,
                          widget, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  ligma_widget_set_bound_property (widget, config, property_name);

  return widget;
}

/**
 * ligma_prop_spin_scale_new:
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
 * returned widget. Use ligma_spin_scale_set_label() to change this.
 *
 * If you wish to change the widget's range relatively to the
 * @property_name's range, use ligma_prop_widget_set_factor().
 *
 * Returns: (transfer full): A new #libligmawidgets-ligmaspinbutton.
 *
 * Since: 3.0
 */
GtkWidget *
ligma_prop_spin_scale_new (GObject     *config,
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

  spinscale = ligma_spin_scale_new (adjustment, label, digits);

  set_param_spec (G_OBJECT (adjustment), spinscale, param_spec);

  if (GEGL_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      GeglParamSpecDouble *gspec = GEGL_PARAM_SPEC_DOUBLE (param_spec);

      ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (spinscale),
                                        gspec->ui_minimum, gspec->ui_maximum);
      ligma_spin_scale_set_gamma (LIGMA_SPIN_SCALE (spinscale), gspec->ui_gamma);
    }
  else if (GEGL_IS_PARAM_SPEC_INT (param_spec))
    {
      GeglParamSpecInt *gspec = GEGL_PARAM_SPEC_INT (param_spec);

      ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (spinscale),
                                        gspec->ui_minimum, gspec->ui_maximum);
      ligma_spin_scale_set_gamma (LIGMA_SPIN_SCALE (spinscale), gspec->ui_gamma);
    }

  tooltip = g_param_spec_get_blurb (param_spec);
  ligma_help_set_help_data (spinscale, tooltip, NULL);

  binding = g_object_bind_property (config, property_name,
                                    spinscale, "value",
                                    G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  g_object_set_data (G_OBJECT (adjustment),
                     "ligma-prop-adjustment-binding",
                     binding);

  gtk_widget_show (spinscale);

  ligma_widget_set_bound_property (spinscale, config, property_name);

  return spinscale;
}

/**
 * ligma_prop_widget_set_factor:
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
 * as input are those created by ligma_prop_spin_scale_new() and
 * ligma_prop_spin_button_new().
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
ligma_prop_widget_set_factor (GtkWidget *widget,
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
                                    "ligma-prop-adjustment-factor");
  if (factor_store)
    {
      old_factor = *factor_store;
    }
  else
    {
      factor_store = g_new (gdouble, 1);
      g_object_set_data_full (G_OBJECT (adjustment),
                              "ligma-prop-adjustment-factor",
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
                               "ligma-prop-adjustment-binding");
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
                                             ligma_prop_widget_double_to_factor,
                                             ligma_prop_widget_double_from_factor,
                                             user_data, (GDestroyNotify) g_free);
    }
  else
    {
      binding = g_object_bind_property (config, property_name,
                                        widget, "value",
                                        G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
    }
  g_object_set_data (G_OBJECT (adjustment),
                     "ligma-prop-adjustment-binding",
                     binding);
  g_object_unref (config);
  g_free (property_name);
}

/**
 * ligma_prop_hscale_new:
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
ligma_prop_hscale_new (GObject     *config,
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
                    G_CALLBACK (ligma_prop_adjustment_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_adjustment_notify),
                  adjustment);

  ligma_widget_set_bound_property (scale, config, property_name);

  gtk_widget_show (scale);

  return scale;
}

/**
 * ligma_prop_scale_entry_new:
 * @config:         Object to which property is attached.
 * @label: (nullable): The text for the #GtkLabel which will appear left of
 *                     the #GtkHScale.
 * @property_name:  Name of integer or double property controlled by the scale.
 * @factor:         Optional multiplier to convert @property_name's
 *                  range into the #LigmaScaleEntry's range. The common
 *                  usage is to set 1.0.
 *                  For non-double properties, no other values than 1.0
 *                  are acceptable.
 * @limit_scale:    %FALSE if the range of possible values of the
 *                  GtkHScale should be the same as of the GtkSpinButton.
 * @lower_limit:    The scale's lower boundary if @scale_limits is %TRUE.
 * @upper_limit:    The scale's upper boundary if @scale_limits is %TRUE.
 *
 * Creates a #LigmaScaleEntry (slider and spin button) to set and display
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
 * See ligma_scale_entry_set_bounds() for more information on
 * @limit_scale, @lower_limit and @upper_limit.
 *
 * Returns: (transfer full): The newly allocated #LigmaScaleEntry.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_scale_entry_new (GObject     *config,
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

  widget = ligma_scale_entry_new (label, value, lower * factor, upper * factor, digits);
  if (limit_scale)
    ligma_scale_entry_set_bounds (LIGMA_SCALE_ENTRY (widget),
                                 lower_limit, upper_limit,
                                 FALSE);

  tooltip = g_param_spec_get_blurb (param_spec);
  ligma_help_set_help_data (widget, tooltip, NULL);

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
                                   ligma_prop_widget_double_to_factor,
                                   ligma_prop_widget_double_from_factor,
                                   user_data, (GDestroyNotify) g_free);
    }
  else
    {
      g_object_bind_property (config, property_name,
                              widget, "value",
                              G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

    }

  ligma_widget_set_bound_property (widget, config, property_name);

  return widget;
}


static void
ligma_prop_adjustment_callback (GtkAdjustment *adjustment,
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
                              "ligma-prop-adjustment-factor");
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
ligma_prop_adjustment_notify (GObject       *config,
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
                              "ligma-prop-adjustment-factor");
  if (factor)
    value *= *factor;

  if (gtk_adjustment_get_value (adjustment) != value)
    {
      g_signal_handlers_block_by_func (adjustment,
                                       ligma_prop_adjustment_callback,
                                       config);

      gtk_adjustment_set_value (adjustment, value);

      g_signal_handlers_unblock_by_func (adjustment,
                                         ligma_prop_adjustment_callback,
                                         config);
    }
}


/*************/
/*  memsize  */
/*************/

static void   ligma_prop_memsize_callback (LigmaMemsizeEntry *entry,
                                          GObject          *config);
static void   ligma_prop_memsize_notify   (GObject          *config,
                                          GParamSpec       *param_spec,
                                          LigmaMemsizeEntry *entry);

/**
 * ligma_prop_memsize_entry_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of memsize property.
 *
 * Creates a #LigmaMemsizeEntry (spin button and option menu) to set
 * and display the value of the specified memsize property.  See
 * ligma_memsize_entry_new() for more information.
 *
 * Returns: (transfer full): A new #LigmaMemsizeEntry.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_memsize_entry_new (GObject     *config,
                             const gchar *property_name)
{
  GParamSpec       *param_spec;
  GParamSpecUInt64 *uint64_spec;
  GtkWidget        *entry;
  guint64           value;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   LIGMA_TYPE_PARAM_MEMSIZE, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  uint64_spec = G_PARAM_SPEC_UINT64 (param_spec);

  g_return_val_if_fail (uint64_spec->minimum <= LIGMA_MAX_MEMSIZE, NULL);
  g_return_val_if_fail (uint64_spec->maximum <= LIGMA_MAX_MEMSIZE, NULL);

  entry = ligma_memsize_entry_new (value,
                                  uint64_spec->minimum,
                                  uint64_spec->maximum);

  set_param_spec (G_OBJECT (entry),
                  ligma_memsize_entry_get_spinbutton (LIGMA_MEMSIZE_ENTRY (entry)),
                  param_spec);

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (ligma_prop_memsize_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_memsize_notify),
                  entry);

  ligma_widget_set_bound_property (entry, config, property_name);

  gtk_widget_show (entry);

  return entry;
}


static void
ligma_prop_memsize_callback (LigmaMemsizeEntry *entry,
                            GObject          *config)
{
  GParamSpec *param_spec;
  guint64     value;
  guint64     v;

  param_spec = get_param_spec (G_OBJECT (entry));
  if (! param_spec)
    return;

  g_return_if_fail (G_IS_PARAM_SPEC_UINT64 (param_spec));

  value = ligma_memsize_entry_get_value (entry);

  g_object_get (config, param_spec->name, &v, NULL);

  if (v != value)
    g_object_set (config, param_spec->name, value, NULL);
}

static void
ligma_prop_memsize_notify (GObject          *config,
                          GParamSpec       *param_spec,
                          LigmaMemsizeEntry *entry)
{
  guint64  value;

  g_return_if_fail (G_IS_PARAM_SPEC_UINT64 (param_spec));

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (ligma_memsize_entry_get_value (entry) != value)
    {
      g_signal_handlers_block_by_func (entry,
                                       ligma_prop_memsize_callback,
                                       config);

      ligma_memsize_entry_set_value (entry, value);

      g_signal_handlers_unblock_by_func (entry,
                                         ligma_prop_memsize_callback,
                                         config);
    }
}


/***********/
/*  label  */
/***********/

/**
 * ligma_prop_label_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of string property.
 *
 * Creates a #GtkLabel to display the value of the specified property.
 * The property should be a string property or at least transformable
 * to a string.  If the user should be able to edit the string, use
 * ligma_prop_entry_new() instead.
 *
 * Returns: (transfer full): A new #GtkLabel widget.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_label_new (GObject     *config,
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
    ligma_help_set_help_data (label, blurb, NULL);

  /* As labels are read-only widgets, allow read-only properties (though
   * we still keep possibility for bidirectional binding, which can be
   * useful even with labels).
   */
  if (param_spec->flags & G_PARAM_WRITABLE)
    flags |= G_BINDING_BIDIRECTIONAL;

  g_object_bind_property (config, property_name,
                          label, "label", flags);

  ligma_widget_set_bound_property (label, config, property_name);

  return label;
}


/***********/
/*  entry  */
/***********/

static void   ligma_prop_entry_callback (GtkWidget  *entry,
                                        GObject    *config);
static void   ligma_prop_entry_notify   (GObject    *config,
                                        GParamSpec *param_spec,
                                        GtkWidget  *entry);

/**
 * ligma_prop_entry_new:
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
ligma_prop_entry_new (GObject     *config,
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

  set_param_spec (G_OBJECT (entry), entry, param_spec);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (ligma_prop_entry_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_entry_notify),
                  entry);

  ligma_widget_set_bound_property (entry, config, property_name);

  gtk_widget_show (entry);

  return entry;
}

static void
ligma_prop_entry_callback (GtkWidget *entry,
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
                                       ligma_prop_entry_notify,
                                       entry);

      g_object_set (config, param_spec->name, value, NULL);

      g_signal_handlers_unblock_by_func (config,
                                         ligma_prop_entry_notify,
                                         entry);
    }

  g_free (v);
}

static void
ligma_prop_entry_notify (GObject    *config,
                        GParamSpec *param_spec,
                        GtkWidget  *entry)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (entry,
                                   ligma_prop_entry_callback,
                                   config);

  gtk_entry_set_text (GTK_ENTRY (entry), value ? value : "");

  g_signal_handlers_unblock_by_func (entry,
                                     ligma_prop_entry_callback,
                                     config);

  g_free (value);
}

/*****************/
/*  label entry  */
/*****************/

/**
 * ligma_prop_label_entry_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of string property.
 * @max_len:       Maximum allowed length of string (set to negative for
 *                 no maximum).
 *
 * Creates a #LigmaLabelEntry to set and display the value of the
 * specified string property.
 *
 * Returns: (transfer full): A new #GtkEntry widget.
 *
 * Since: 3.0
 */
GtkWidget *
ligma_prop_label_entry_new (GObject     *config,
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
  label_entry = ligma_label_entry_new (label);
  entry = ligma_label_entry_get_entry (LIGMA_LABEL_ENTRY (label_entry));

  if (max_len > 0)
    gtk_entry_set_max_length (GTK_ENTRY (entry), max_len);

  gtk_editable_set_editable (GTK_EDITABLE (entry),
                             param_spec->flags & G_PARAM_WRITABLE);

  set_param_spec (G_OBJECT (label_entry), label_entry, param_spec);

  g_object_bind_property (config,      property_name,
                          label_entry, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  ligma_widget_set_bound_property (label_entry, config, property_name);

  gtk_widget_show (label_entry);

  return label_entry;
}


/*****************/
/*  text buffer  */
/*****************/

static void   ligma_prop_text_buffer_callback (GtkTextBuffer *text_buffer,
                                              GObject       *config);
static void   ligma_prop_text_buffer_notify   (GObject       *config,
                                              GParamSpec    *param_spec,
                                              GtkTextBuffer *text_buffer);

/**
 * ligma_prop_text_buffer_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of string property.
 * @max_len:       Maximum allowed length of text (in characters).
 *
 * Creates a #GtkTextBuffer to set and display the value of the
 * specified string property.  Unless the string is expected to
 * contain multiple lines or a large amount of text, use
 * ligma_prop_entry_new() instead.  See #GtkTextView for information on
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
ligma_prop_text_buffer_new (GObject     *config,
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
                    G_CALLBACK (ligma_prop_text_buffer_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_text_buffer_notify),
                  text_buffer);

  return text_buffer;
}

static void
ligma_prop_text_buffer_callback (GtkTextBuffer *text_buffer,
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
      g_message (dngettext (GETTEXT_PACKAGE "-libligma",
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
                                           ligma_prop_text_buffer_notify,
                                           text_buffer);

          g_object_set (config, param_spec->name, text,  NULL);

          g_signal_handlers_unblock_by_func (config,
                                             ligma_prop_text_buffer_notify,
                                             text_buffer);
        }

      g_free (v);
    }

  g_free (text);
}

static void
ligma_prop_text_buffer_notify (GObject       *config,
                              GParamSpec    *param_spec,
                              GtkTextBuffer *text_buffer)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (text_buffer,
                                   ligma_prop_text_buffer_callback,
                                   config);

  gtk_text_buffer_set_text (text_buffer, value ? value : "", -1);

  g_signal_handlers_unblock_by_func (text_buffer,
                                     ligma_prop_text_buffer_callback,
                                     config);

  g_free (value);
}


/***********************/
/*  string combo box   */
/***********************/

static void   ligma_prop_string_combo_box_callback (GtkWidget   *widget,
                                                   GObject     *config);
static void   ligma_prop_string_combo_box_notify   (GObject     *config,
                                                   GParamSpec  *param_spec,
                                                   GtkWidget   *widget);

/**
 * ligma_prop_string_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of int property controlled by combo box.
 * @model:         #GtkTreeStore holding list of values
 * @id_column:     column in @store that holds string IDs
 * @label_column:  column in @store that holds labels to use in the combo-box
 *
 * Creates a #LigmaStringComboBox widget to display and set the
 * specified property.  The contents of the widget are determined by
 * @store.
 *
 * Returns: (transfer full): The newly created #LigmaStringComboBox widget.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_string_combo_box_new (GObject      *config,
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

  combo_box = ligma_string_combo_box_new (model, id_column, label_column);

  ligma_string_combo_box_set_active (LIGMA_STRING_COMBO_BOX (combo_box), value);

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (ligma_prop_string_combo_box_callback),
                    config);

  set_param_spec (G_OBJECT (combo_box), combo_box, param_spec);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_string_combo_box_notify),
                  combo_box);

  ligma_widget_set_bound_property (combo_box, config, property_name);

  gtk_widget_show (combo_box);

  return combo_box;
}

static void
ligma_prop_string_combo_box_callback (GtkWidget *widget,
                                     GObject   *config)
{
  GParamSpec  *param_spec;
  gchar       *value;
  gchar       *v;

  param_spec = get_param_spec (G_OBJECT (widget));
  if (! param_spec)
    return;

  value = ligma_string_combo_box_get_active (LIGMA_STRING_COMBO_BOX (widget));

  g_object_get (config, param_spec->name, &v, NULL);

  if (g_strcmp0 (v, value))
    g_object_set (config, param_spec->name, value, NULL);

  g_free (value);
  g_free (v);
}

static void
ligma_prop_string_combo_box_notify (GObject    *config,
                                   GParamSpec *param_spec,
                                   GtkWidget  *combo_box)
{
  gchar *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (combo_box,
                                   ligma_prop_string_combo_box_callback,
                                   config);

  ligma_string_combo_box_set_active (LIGMA_STRING_COMBO_BOX (combo_box), value);

  g_signal_handlers_unblock_by_func (combo_box,
                                     ligma_prop_string_combo_box_callback,
                                     config);

  g_free (value);
}


/*************************/
/*  file chooser button  */
/*************************/

static GtkWidget * ligma_prop_file_chooser_button_setup    (GtkWidget      *button,
                                                           GObject        *config,
                                                           GParamSpec     *param_spec);
static void        ligma_prop_file_chooser_button_callback (GtkFileChooser *button,
                                                           GObject        *config);
static void        ligma_prop_file_chooser_button_notify   (GObject        *config,
                                                           GParamSpec     *param_spec,
                                                           GtkFileChooser *button);


/**
 * ligma_prop_file_chooser_button_new:
 * @config:        object to which property is attached.
 * @property_name: name of path property.
 * @title: (nullable): the title of the browse dialog.
 * @action:        the open mode for the widget.
 *
 * Creates a #GtkFileChooserButton to edit the specified path property.
 * @property_name must represent either a LIGMA_PARAM_SPEC_CONFIG_PATH or
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
ligma_prop_file_chooser_button_new (GObject              *config,
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

  if (! LIGMA_IS_PARAM_SPEC_CONFIG_PATH (param_spec) &&
      (! G_IS_PARAM_SPEC_OBJECT (param_spec) || param_spec->value_type != G_TYPE_FILE))
    {
      g_warning ("%s: property '%s' of %s is neither a LIGMA_PARAM_SPEC_CONFIG_PATH "
                 "nor a G_PARAM_SPEC_OBJECT of value type G_TYPE_FILE.",
                 G_STRFUNC, param_spec->name,
                 g_type_name (param_spec->owner_type));
      return NULL;
    }

  if (! title)
    title = g_param_spec_get_nick (param_spec);

  button = gtk_file_chooser_button_new (title, action);

  return ligma_prop_file_chooser_button_setup (button, config, param_spec);
}

/**
 * ligma_prop_file_chooser_button_new_with_dialog:
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
ligma_prop_file_chooser_button_new_with_dialog (GObject     *config,
                                               const gchar *property_name,
                                               GtkWidget   *dialog)
{
  GParamSpec *param_spec;
  GtkWidget  *button;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (property_name != NULL, NULL);
  g_return_val_if_fail (GTK_IS_FILE_CHOOSER_DIALOG (dialog), NULL);

  param_spec = check_param_spec_w (config, property_name,
                                   LIGMA_TYPE_PARAM_CONFIG_PATH, G_STRFUNC);
  if (! param_spec)
    return NULL;

  button = gtk_file_chooser_button_new_with_dialog (dialog);

  return ligma_prop_file_chooser_button_setup (button, config, param_spec);
}

static GtkWidget *
ligma_prop_file_chooser_button_setup (GtkWidget  *button,
                                     GObject    *config,
                                     GParamSpec *param_spec)
{
  GFile *file = NULL;

  if (LIGMA_IS_PARAM_SPEC_CONFIG_PATH (param_spec))
    {
      gchar *value;

      g_object_get (config,
                    param_spec->name, &value,
                    NULL);

      if (value)
        {
          file = ligma_file_new_for_config_path (value, NULL);
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
                    G_CALLBACK (ligma_prop_file_chooser_button_callback),
                    config);

  connect_notify (config, param_spec->name,
                  G_CALLBACK (ligma_prop_file_chooser_button_notify),
                  button);

  gtk_widget_show (button);

  return button;
}

static void
ligma_prop_file_chooser_button_callback (GtkFileChooser *button,
                                        GObject        *config)
{
  GParamSpec *param_spec;
  GFile      *file;

  param_spec = get_param_spec (G_OBJECT (button));
  if (! param_spec)
    return;

  file = gtk_file_chooser_get_file (button);

  if (LIGMA_IS_PARAM_SPEC_CONFIG_PATH (param_spec))
    {
      gchar *value = NULL;
      gchar *v;

      if (file)
        {
          value = ligma_file_get_config_path (file, NULL);
          g_object_unref (file);
        }

      g_object_get (config, param_spec->name, &v, NULL);

      if (g_strcmp0 (v, value))
        {
          g_signal_handlers_block_by_func (config,
                                           ligma_prop_file_chooser_button_notify,
                                           button);

          g_object_set (config, param_spec->name, value, NULL);

          g_signal_handlers_unblock_by_func (config,
                                             ligma_prop_file_chooser_button_notify,
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
                                           ligma_prop_file_chooser_button_notify,
                                           button);

          g_object_set (config, param_spec->name, file, NULL);

          g_signal_handlers_unblock_by_func (config,
                                             ligma_prop_file_chooser_button_notify,
                                             button);
        }
      g_clear_object (&f);
    }
  g_clear_object (&file);
}

static void
ligma_prop_file_chooser_button_notify (GObject        *config,
                                      GParamSpec     *param_spec,
                                      GtkFileChooser *button)
{
  gchar *value;
  GFile *file = NULL;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (value)
    {
      file = ligma_file_new_for_config_path (value, NULL);
      g_free (value);
    }

  g_signal_handlers_block_by_func (button,
                                   ligma_prop_file_chooser_button_callback,
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
                                     ligma_prop_file_chooser_button_callback,
                                     config);
}


/*****************/
/*  path editor  */
/*****************/

static void   ligma_prop_path_editor_path_callback     (LigmaPathEditor *editor,
                                                       GObject        *config);
static void   ligma_prop_path_editor_writable_callback (LigmaPathEditor *editor,
                                                       GObject        *config);
static void   ligma_prop_path_editor_path_notify       (GObject        *config,
                                                       GParamSpec     *param_spec,
                                                       LigmaPathEditor *editor);
static void   ligma_prop_path_editor_writable_notify   (GObject        *config,
                                                       GParamSpec     *param_spec,
                                                       LigmaPathEditor *editor);

/**
 * ligma_prop_path_editor_new:
 * @config:                 object to which property is attached.
 * @path_property_name:     name of path property.
 * @writable_property_name: name of writable path property.
 * @filechooser_title:      window title of #GtkFileChooserDialog widget.
 *
 * Creates a #LigmaPathEditor to edit the specified path and writable
 * path properties.
 *
 * Returns: (transfer full): A new #LigmaPathEditor.
 **/
GtkWidget *
ligma_prop_path_editor_new (GObject     *config,
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
                                        LIGMA_TYPE_PARAM_CONFIG_PATH, G_STRFUNC);
  if (! path_param_spec)
    return NULL;

  if (writable_property_name)
    {
      writable_param_spec = check_param_spec_w (config, writable_property_name,
                                                LIGMA_TYPE_PARAM_CONFIG_PATH,
                                                G_STRFUNC);
      if (! writable_param_spec)
        return NULL;
    }

  g_object_get (config,
                path_property_name, &value,
                NULL);

  filename = value ? ligma_config_path_expand (value, TRUE, NULL) : NULL;
  g_free (value);

  editor = ligma_path_editor_new (filechooser_title, filename);
  g_free (filename);

  if (writable_property_name)
    {
      g_object_get (config,
                    writable_property_name, &value,
                    NULL);

      filename = value ? ligma_config_path_expand (value, TRUE, NULL) : NULL;
      g_free (value);

      ligma_path_editor_set_writable_path (LIGMA_PATH_EDITOR (editor), filename);
      g_free (filename);
    }

  g_object_set_data (G_OBJECT (editor), "ligma-config-param-spec-path",
                     path_param_spec);

  g_signal_connect (editor, "path-changed",
                    G_CALLBACK (ligma_prop_path_editor_path_callback),
                    config);

  connect_notify (config, path_property_name,
                  G_CALLBACK (ligma_prop_path_editor_path_notify),
                  editor);

  if (writable_property_name)
    {
      g_object_set_data (G_OBJECT (editor), "ligma-config-param-spec-writable",
                         writable_param_spec);

      g_signal_connect (editor, "writable-changed",
                        G_CALLBACK (ligma_prop_path_editor_writable_callback),
                        config);

      connect_notify (config, writable_property_name,
                      G_CALLBACK (ligma_prop_path_editor_writable_notify),
                      editor);
    }

  ligma_widget_set_bound_property (editor, config, path_property_name);

  gtk_widget_show (editor);

  return editor;
}

static void
ligma_prop_path_editor_path_callback (LigmaPathEditor *editor,
                                     GObject        *config)
{
  GParamSpec *path_param_spec;
  GParamSpec *writable_param_spec;
  gchar      *value;
  gchar      *utf8;

  path_param_spec     = g_object_get_data (G_OBJECT (editor),
                                           "ligma-config-param-spec-path");
  writable_param_spec = g_object_get_data (G_OBJECT (editor),
                                           "ligma-config-param-spec-writable");
  if (! path_param_spec)
    return;

  value = ligma_path_editor_get_path (editor);
  utf8 = value ? ligma_config_path_unexpand (value, TRUE, NULL) : NULL;
  g_free (value);

  g_signal_handlers_block_by_func (config,
                                   ligma_prop_path_editor_path_notify,
                                   editor);

  g_object_set (config,
                path_param_spec->name, utf8,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     ligma_prop_path_editor_path_notify,
                                     editor);

  g_free (utf8);

  if (writable_param_spec)
    {
      value = ligma_path_editor_get_writable_path (editor);
      utf8 = value ? ligma_config_path_unexpand (value, TRUE, NULL) : NULL;
      g_free (value);

      g_signal_handlers_block_by_func (config,
                                       ligma_prop_path_editor_writable_notify,
                                       editor);

      g_object_set (config,
                    writable_param_spec->name, utf8,
                    NULL);

      g_signal_handlers_unblock_by_func (config,
                                         ligma_prop_path_editor_writable_notify,
                                         editor);

      g_free (utf8);
    }
}

static void
ligma_prop_path_editor_writable_callback (LigmaPathEditor *editor,
                                         GObject        *config)
{
  GParamSpec *param_spec;
  gchar      *value;
  gchar      *utf8;

  param_spec = g_object_get_data (G_OBJECT (editor),
                                  "ligma-config-param-spec-writable");
  if (! param_spec)
    return;

  value = ligma_path_editor_get_writable_path (editor);
  utf8 = value ? ligma_config_path_unexpand (value, TRUE, NULL) : NULL;
  g_free (value);

  g_signal_handlers_block_by_func (config,
                                   ligma_prop_path_editor_writable_notify,
                                   editor);

  g_object_set (config,
                param_spec->name, utf8,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     ligma_prop_path_editor_writable_notify,
                                     editor);

  g_free (utf8);
}

static void
ligma_prop_path_editor_path_notify (GObject        *config,
                                   GParamSpec     *param_spec,
                                   LigmaPathEditor *editor)
{
  gchar *value;
  gchar *filename;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  filename = value ? ligma_config_path_expand (value, TRUE, NULL) : NULL;
  g_free (value);

  g_signal_handlers_block_by_func (editor,
                                   ligma_prop_path_editor_path_callback,
                                   config);

  ligma_path_editor_set_path (editor, filename);

  g_signal_handlers_unblock_by_func (editor,
                                     ligma_prop_path_editor_path_callback,
                                     config);

  g_free (filename);
}

static void
ligma_prop_path_editor_writable_notify (GObject        *config,
                                       GParamSpec     *param_spec,
                                       LigmaPathEditor *editor)
{
  gchar *value;
  gchar *filename;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  filename = value ? ligma_config_path_expand (value, TRUE, NULL) : NULL;
  g_free (value);

  g_signal_handlers_block_by_func (editor,
                                   ligma_prop_path_editor_writable_callback,
                                   config);

  ligma_path_editor_set_writable_path (editor, filename);

  g_signal_handlers_unblock_by_func (editor,
                                     ligma_prop_path_editor_writable_callback,
                                     config);

  g_free (filename);
}


/***************/
/*  sizeentry  */
/***************/

static void   ligma_prop_size_entry_callback    (LigmaSizeEntry *entry,
                                                GObject       *config);
static void   ligma_prop_size_entry_notify      (GObject       *config,
                                                GParamSpec    *param_spec,
                                                LigmaSizeEntry *entry);
static void   ligma_prop_size_entry_notify_unit (GObject       *config,
                                                GParamSpec    *param_spec,
                                                LigmaSizeEntry *entry);
static gint   ligma_prop_size_entry_num_chars   (gdouble        lower,
                                                gdouble        upper);


/**
 * ligma_prop_size_entry_new:
 * @config:             Object to which property is attached.
 * @property_name:      Name of int or double property.
 * @property_is_pixel:  When %TRUE, the property value is in pixels,
 *                      and in the selected unit otherwise.
 * @unit_property_name: Name of unit property.
 * @unit_format:        A printf-like unit-format string as is used with
 *                      ligma_unit_menu_new().
 * @update_policy:      How the automatic pixel <-> real-world-unit
 *                      calculations should be done.
 * @resolution:         The resolution (in dpi) for the field.
 *
 * Creates a #LigmaSizeEntry to set and display the specified double or
 * int property, and its associated unit property.  Note that this
 * function is only suitable for creating a size entry holding a
 * single value.  Use ligma_prop_coordinates_new() to create a size
 * entry holding two values.
 *
 * Returns: (transfer full): A new #LigmaSizeEntry widget.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_size_entry_new (GObject                   *config,
                          const gchar               *property_name,
                          gboolean                   property_is_pixel,
                          const gchar               *unit_property_name,
                          const gchar               *unit_format,
                          LigmaSizeEntryUpdatePolicy  update_policy,
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
  LigmaUnit    unit_value;

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
                                            LIGMA_TYPE_PARAM_UNIT, G_STRFUNC);
      if (! unit_param_spec)
        return NULL;

      g_value_init (&value, unit_param_spec->value_type);

      g_value_set_int (&value, LIGMA_UNIT_PIXEL);
      show_pixels = (g_param_value_validate (unit_param_spec,
                                             &value) == FALSE);

      g_value_set_int (&value, LIGMA_UNIT_PERCENT);
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
      unit_value      = LIGMA_UNIT_INCH;
      show_pixels     = FALSE;
      show_percent    = FALSE;
    }

  entry = ligma_size_entry_new (1, unit_value, unit_format,
                               show_pixels, show_percent, FALSE,
                               ligma_prop_size_entry_num_chars (lower, upper) + 1 +
                               ligma_unit_get_scaled_digits (unit_value, resolution),
                               update_policy);

  set_param_spec (NULL,
                  ligma_size_entry_get_help_widget (LIGMA_SIZE_ENTRY (entry), 0),
                  param_spec);

  if (unit_param_spec)
    set_param_spec (NULL,
                    ligma_size_entry_get_unit_combo  (LIGMA_SIZE_ENTRY (entry)),
                    unit_param_spec);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (entry), unit_value);

  if (update_policy == LIGMA_SIZE_ENTRY_UPDATE_SIZE)
    ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 0,
                                    resolution, FALSE);

  ligma_size_entry_set_value_boundaries (LIGMA_SIZE_ENTRY (entry), 0,
                                        lower, upper);

  g_object_set_data (G_OBJECT (entry), "value-is-pixel",
                     GINT_TO_POINTER (property_is_pixel ? TRUE : FALSE));

  if (property_is_pixel)
    ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (entry), 0, value);
  else
    ligma_size_entry_set_value (LIGMA_SIZE_ENTRY (entry), 0, value);

  g_object_set_data (G_OBJECT (entry), "ligma-config-param-spec",
                     param_spec);

  g_signal_connect (entry, "refval-changed",
                    G_CALLBACK (ligma_prop_size_entry_callback),
                    config);
  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (ligma_prop_size_entry_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_size_entry_notify),
                  entry);

  if (unit_property_name)
    {
      g_object_set_data (G_OBJECT (entry), "ligma-config-param-spec-unit",
                         unit_param_spec);

      g_signal_connect (entry, "unit-changed",
                        G_CALLBACK (ligma_prop_size_entry_callback),
                        config);

      connect_notify (config, unit_property_name,
                      G_CALLBACK (ligma_prop_size_entry_notify_unit),
                      entry);
    }

  ligma_widget_set_bound_property (entry, config, property_name);

  gtk_widget_show (entry);

  return entry;
}

static void
ligma_prop_size_entry_callback (LigmaSizeEntry *entry,
                               GObject       *config)
{
  GParamSpec *param_spec;
  GParamSpec *unit_param_spec;
  gdouble     value;
  gboolean    value_is_pixel;
  LigmaUnit    unit_value;

  param_spec = g_object_get_data (G_OBJECT (entry), "ligma-config-param-spec");
  if (! param_spec)
    return;

  unit_param_spec = g_object_get_data (G_OBJECT (entry),
                                       "ligma-config-param-spec-unit");

  value_is_pixel = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry),
                                                       "value-is-pixel"));

  if (value_is_pixel)
    value = ligma_size_entry_get_refval (entry, 0);
  else
    value = ligma_size_entry_get_value (entry, 0);

  unit_value = ligma_size_entry_get_unit (entry);

  if (unit_param_spec)
    {
      LigmaUnit  old_unit;

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
ligma_prop_size_entry_notify (GObject       *config,
                             GParamSpec    *param_spec,
                             LigmaSizeEntry *entry)
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
    entry_value = ligma_size_entry_get_refval (entry, 0);
  else
    entry_value = ligma_size_entry_get_value (entry, 0);

  if (value != entry_value)
    {
      g_signal_handlers_block_by_func (entry,
                                       ligma_prop_size_entry_callback,
                                       config);

      if (value_is_pixel)
        ligma_size_entry_set_refval (entry, 0, value);
      else
        ligma_size_entry_set_value (entry, 0, value);

      g_signal_handlers_unblock_by_func (entry,
                                         ligma_prop_size_entry_callback,
                                         config);
    }
}

static void
ligma_prop_size_entry_notify_unit (GObject       *config,
                                  GParamSpec    *param_spec,
                                  LigmaSizeEntry *entry)
{
  LigmaUnit value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (value != ligma_size_entry_get_unit (entry))
    {
      g_signal_handlers_block_by_func (entry,
                                       ligma_prop_size_entry_callback,
                                       config);

      ligma_size_entry_set_unit (entry, value);

      g_signal_handlers_unblock_by_func (entry,
                                         ligma_prop_size_entry_callback,
                                         config);
    }
}

static gint
ligma_prop_size_entry_num_chars (gdouble lower,
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

static void   ligma_prop_coordinates_callback    (LigmaSizeEntry *entry,
                                                 GObject       *config);
static void   ligma_prop_coordinates_notify_x    (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 LigmaSizeEntry *entry);
static void   ligma_prop_coordinates_notify_y    (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 LigmaSizeEntry *entry);
static void   ligma_prop_coordinates_notify_unit (GObject       *config,
                                                 GParamSpec    *param_spec,
                                                 LigmaSizeEntry *entry);


/**
 * ligma_prop_coordinates_new:
 * @config:             Object to which property is attached.
 * @x_property_name:    Name of int or double property for X coordinate.
 * @y_property_name:    Name of int or double property for Y coordinate.
 * @unit_property_name: Name of unit property.
 * @unit_format:        A printf-like unit-format string as is used with
 *                      ligma_unit_menu_new().
 * @update_policy:      How the automatic pixel <-> real-world-unit
 *                      calculations should be done.
 * @xresolution:        The resolution (in dpi) for the X coordinate.
 * @yresolution:        The resolution (in dpi) for the Y coordinate.
 * @has_chainbutton:    Whether to add a chainbutton to the size entry.
 *
 * Creates a #LigmaSizeEntry to set and display two double or int
 * properties, which will usually represent X and Y coordinates, and
 * their associated unit property.
 *
 * Returns: (transfer full): A new #LigmaSizeEntry widget.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_coordinates_new (GObject                   *config,
                           const gchar               *x_property_name,
                           const gchar               *y_property_name,
                           const gchar               *unit_property_name,
                           const gchar               *unit_format,
                           LigmaSizeEntryUpdatePolicy  update_policy,
                           gdouble                    xresolution,
                           gdouble                    yresolution,
                           gboolean                   has_chainbutton)
{
  GtkWidget *entry;
  GtkWidget *chainbutton = NULL;

  entry = ligma_size_entry_new (2, LIGMA_UNIT_INCH, unit_format,
                               FALSE, FALSE, TRUE, 10,
                               update_policy);

  if (has_chainbutton)
    {
      chainbutton = ligma_chain_button_new (LIGMA_CHAIN_BOTTOM);
      gtk_grid_attach (GTK_GRID (entry), chainbutton, 1, 3, 2, 1);
      gtk_widget_show (chainbutton);
    }

  if (! ligma_prop_coordinates_connect (config,
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

  ligma_widget_set_bound_property (entry, config, x_property_name);

  gtk_widget_show (entry);

  return entry;
}

gboolean
ligma_prop_coordinates_connect (GObject     *config,
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
  LigmaUnit    unit_value;
  gdouble    *old_x_value;
  gdouble    *old_y_value;
  LigmaUnit   *old_unit_value;
  gboolean    chain_checked;

  g_return_val_if_fail (LIGMA_IS_SIZE_ENTRY (entry), FALSE);
  g_return_val_if_fail (ligma_size_entry_get_n_fields (LIGMA_SIZE_ENTRY (entry)) == 2,
                        FALSE);
  g_return_val_if_fail (chainbutton == NULL ||
                        LIGMA_IS_CHAIN_BUTTON (chainbutton), FALSE);

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
                                            LIGMA_TYPE_PARAM_UNIT, G_STRFUNC);
      if (! unit_param_spec)
        return FALSE;

      g_object_get (config,
                    unit_property_name, &unit_value,
                    NULL);
    }
  else
    {
      unit_param_spec = NULL;
      unit_value      = LIGMA_UNIT_INCH;
    }

  set_param_spec (NULL,
                  ligma_size_entry_get_help_widget (LIGMA_SIZE_ENTRY (entry), 0),
                  x_param_spec);
  set_param_spec (NULL,
                  ligma_size_entry_get_help_widget (LIGMA_SIZE_ENTRY (entry), 1),
                  y_param_spec);

  if (unit_param_spec)
    set_param_spec (NULL,
                    ligma_size_entry_get_unit_combo (LIGMA_SIZE_ENTRY (entry)),
                    unit_param_spec);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (entry), unit_value);

  switch (ligma_size_entry_get_update_policy (LIGMA_SIZE_ENTRY (entry)))
    {
    case LIGMA_SIZE_ENTRY_UPDATE_SIZE:
      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 0,
                                      xresolution, FALSE);
      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (entry), 1,
                                      yresolution, FALSE);
      chain_checked = (ABS (x_value - y_value) < 1);
      break;

    case LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION:
      chain_checked = (ABS (x_value - y_value) < LIGMA_MIN_RESOLUTION);
      break;

    default:
      chain_checked = (x_value == y_value);
      break;
    }

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (entry), 0,
                                         x_lower, x_upper);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (entry), 1,
                                         y_lower, y_upper);

  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (entry), 0, x_value);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (entry), 1, y_value);

  g_object_set_data (G_OBJECT (entry), "ligma-config-param-spec-x",
                     x_param_spec);
  g_object_set_data (G_OBJECT (entry), "ligma-config-param-spec-y",
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
        ligma_chain_button_set_active (LIGMA_CHAIN_BUTTON (chainbutton), TRUE);

      g_object_set_data (G_OBJECT (entry), "chainbutton", chainbutton);
    }

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (ligma_prop_coordinates_callback),
                    config);
  g_signal_connect (entry, "refval-changed",
                    G_CALLBACK (ligma_prop_coordinates_callback),
                    config);

  connect_notify (config, x_property_name,
                  G_CALLBACK (ligma_prop_coordinates_notify_x),
                  entry);
  connect_notify (config, y_property_name,
                  G_CALLBACK (ligma_prop_coordinates_notify_y),
                  entry);

  if (unit_property_name)
    {
      g_object_set_data (G_OBJECT (entry), "ligma-config-param-spec-unit",
                         unit_param_spec);

      old_unit_value  = g_new0 (LigmaUnit, 1);
      *old_unit_value = unit_value;
      g_object_set_data_full (G_OBJECT (entry), "old-unit-value",
                              old_unit_value,
                              (GDestroyNotify) g_free);

      g_signal_connect (entry, "unit-changed",
                        G_CALLBACK (ligma_prop_coordinates_callback),
                        config);

      connect_notify (config, unit_property_name,
                      G_CALLBACK (ligma_prop_coordinates_notify_unit),
                      entry);
    }

  return TRUE;
}

static void
ligma_prop_coordinates_callback (LigmaSizeEntry *entry,
                                GObject       *config)
{
  GParamSpec *x_param_spec;
  GParamSpec *y_param_spec;
  GParamSpec *unit_param_spec;
  gdouble     x_value;
  gdouble     y_value;
  LigmaUnit    unit_value;
  gdouble    *old_x_value;
  gdouble    *old_y_value;
  LigmaUnit   *old_unit_value;
  gboolean    backwards;

  x_param_spec = g_object_get_data (G_OBJECT (entry),
                                    "ligma-config-param-spec-x");
  y_param_spec = g_object_get_data (G_OBJECT (entry),
                                    "ligma-config-param-spec-y");

  if (! x_param_spec || ! y_param_spec)
    return;

  unit_param_spec = g_object_get_data (G_OBJECT (entry),
                                       "ligma-config-param-spec-unit");

  x_value    = ligma_size_entry_get_refval (entry, 0);
  y_value    = ligma_size_entry_get_refval (entry, 1);
  unit_value = ligma_size_entry_get_unit (entry);

  old_x_value    = g_object_get_data (G_OBJECT (entry), "old-x-value");
  old_y_value    = g_object_get_data (G_OBJECT (entry), "old-y-value");
  old_unit_value = g_object_get_data (G_OBJECT (entry), "old-unit-value");

  if (! old_x_value || ! old_y_value || (unit_param_spec && ! old_unit_value))
    return;

  /*
   * FIXME: if the entry was created using ligma_coordinates_new, then
   * the chain button is handled automatically and the following block
   * of code is unnecessary (and, in fact, redundant).
   */
  if (x_value != y_value)
    {
      GtkWidget *chainbutton;

      chainbutton = g_object_get_data (G_OBJECT (entry), "chainbutton");

      if (chainbutton &&
          ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (chainbutton)) &&
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
      (old_unit_value == NULL || *old_unit_value == unit_value))
    return;

  *old_x_value = x_value;
  *old_y_value = y_value;

  if (old_unit_value)
    *old_unit_value = unit_value;

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
ligma_prop_coordinates_notify_x (GObject       *config,
                                GParamSpec    *param_spec,
                                LigmaSizeEntry *entry)
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

  if (value != ligma_size_entry_get_refval (entry, 0))
    {
      gdouble *old_x_value = g_object_get_data (G_OBJECT (entry),
                                                "old-x-value");

      g_signal_handlers_block_by_func (entry,
                                       ligma_prop_coordinates_callback,
                                       config);

      ligma_size_entry_set_refval (entry, 0, value);

      if (old_x_value)
        *old_x_value = value;

      g_signal_emit_by_name (entry, "value-changed",
                             ligma_size_entry_get_value (entry, 0));

      g_signal_handlers_unblock_by_func (entry,
                                         ligma_prop_coordinates_callback,
                                         config);
    }
}

static void
ligma_prop_coordinates_notify_y (GObject       *config,
                                GParamSpec    *param_spec,
                                LigmaSizeEntry *entry)
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

  if (value != ligma_size_entry_get_refval (entry, 1))
    {
      gdouble *old_y_value = g_object_get_data (G_OBJECT (entry),
                                                "old-y-value");

      g_signal_handlers_block_by_func (entry,
                                       ligma_prop_coordinates_callback,
                                       config);

      ligma_size_entry_set_refval (entry, 1, value);

      if (old_y_value)
        *old_y_value = value;

      g_signal_emit_by_name (entry, "value-changed",
                             ligma_size_entry_get_value (entry, 1));

      g_signal_handlers_unblock_by_func (entry,
                                         ligma_prop_coordinates_callback,
                                         config);
    }
}

static void
ligma_prop_coordinates_notify_unit (GObject       *config,
                                   GParamSpec    *param_spec,
                                   LigmaSizeEntry *entry)
{
  LigmaUnit value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  if (value != ligma_size_entry_get_unit (entry))
    {
      g_signal_handlers_block_by_func (entry,
                                       ligma_prop_coordinates_callback,
                                       config);

      ligma_size_entry_set_unit (entry, value);

      g_signal_handlers_unblock_by_func (entry,
                                         ligma_prop_coordinates_callback,
                                         config);
    }
}


/****************/
/*  color area  */
/****************/

static void   ligma_prop_color_area_callback (GtkWidget  *widget,
                                             GObject    *config);
static void   ligma_prop_color_area_notify   (GObject    *config,
                                             GParamSpec *param_spec,
                                             GtkWidget  *area);

/**
 * ligma_prop_color_area_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of RGB property.
 * @width:         Width of color area.
 * @height:        Height of color area.
 * @type:          How transparency is represented.
 *
 * Creates a #LigmaColorArea to set and display the value of an RGB
 * property.
 *
 * Returns: (transfer full): A new #LigmaColorArea widget.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_prop_color_area_new (GObject           *config,
                          const gchar       *property_name,
                          gint               width,
                          gint               height,
                          LigmaColorAreaType  type)
{
  GParamSpec *param_spec;
  GtkWidget  *area;
  LigmaRGB    *value;

  param_spec = check_param_spec_w (config, property_name,
                                   LIGMA_TYPE_PARAM_RGB, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  area = ligma_color_area_new (value, type,
                              GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
  gtk_widget_set_size_request (area, width, height);

  g_free (value);

  set_param_spec (G_OBJECT (area), area, param_spec);

  g_signal_connect (area, "color-changed",
                    G_CALLBACK (ligma_prop_color_area_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_color_area_notify),
                  area);

  ligma_widget_set_bound_property (area, config, property_name);

  gtk_widget_show (area);

  return area;
}

static void
ligma_prop_color_area_callback (GtkWidget *area,
                               GObject   *config)
{
  GParamSpec *param_spec;
  LigmaRGB     value;

  param_spec = get_param_spec (G_OBJECT (area));
  if (! param_spec)
    return;

  ligma_color_area_get_color (LIGMA_COLOR_AREA (area), &value);

  g_signal_handlers_block_by_func (config,
                                   ligma_prop_color_area_notify,
                                   area);

  g_object_set (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_unblock_by_func (config,
                                     ligma_prop_color_area_notify,
                                     area);
}

static void
ligma_prop_color_area_notify (GObject    *config,
                             GParamSpec *param_spec,
                             GtkWidget  *area)
{
  LigmaRGB *value;

  g_object_get (config,
                param_spec->name, &value,
                NULL);

  g_signal_handlers_block_by_func (area,
                                   ligma_prop_color_area_callback,
                                   config);

  ligma_color_area_set_color (LIGMA_COLOR_AREA (area), value);

  g_free (value);

  g_signal_handlers_unblock_by_func (area,
                                     ligma_prop_color_area_callback,
                                     config);
}

/**
 * ligma_prop_color_select_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of RGB property.
 * @width:         Width of the colorpreview in pixels.
 * @height:        Height of the colorpreview in pixels.
 * @type:          How transparency is represented.
 *
 * Creates a #LigmaColorButton to set and display the value of an RGB
 * property.
 *
 * Returns: (transfer full): A new #LigmaColorButton widget.
 *
 * Since: 3.0
 */
GtkWidget *
ligma_prop_color_select_new (GObject           *config,
                            const gchar       *property_name,
                            gint               width,
                            gint               height,
                            LigmaColorAreaType  type)
{
  GParamSpec *param_spec;
  GtkWidget  *button;
  LigmaRGB    *value;

  param_spec = check_param_spec_w (config, property_name,
                                   LIGMA_TYPE_PARAM_RGB, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  button = ligma_color_button_new (g_param_spec_get_nick (param_spec),
                                  width, height, value, type);

  g_free (value);

  g_object_bind_property (config, property_name,
                          button, "color",
                          G_BINDING_BIDIRECTIONAL);

  ligma_widget_set_bound_property (button, config, property_name);

  gtk_widget_show (button);

  return button;
}

/**
 * ligma_prop_label_color_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of RGB property.
 * @editable:      Whether the widget should allow color editability.
 *
 * Creates a #LigmaLabelColor to set and display the value of an RGB
 * property.
 *
 * Returns: (transfer full): A new #LigmaLabelColor widget.
 *
 * Since: 3.0
 */
GtkWidget *
ligma_prop_label_color_new (GObject     *config,
                           const gchar *property_name,
                           gboolean     editable)
{
  GParamSpec  *param_spec;
  GtkWidget   *prop_widget;
  const gchar *label;
  LigmaRGB     *value;

  param_spec = check_param_spec_w (config, property_name,
                                   LIGMA_TYPE_PARAM_RGB, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_object_get (config,
                property_name, &value,
                NULL);

  label = g_param_spec_get_nick (param_spec);

  prop_widget = ligma_label_color_new (label, value, editable);
  g_free (value);

  g_object_bind_property (config,      property_name,
                          prop_widget, "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  ligma_widget_set_bound_property (prop_widget, config, property_name);

  gtk_widget_show (prop_widget);

  return prop_widget;
}

/********************/
/*  unit combo box  */
/********************/

static void   ligma_prop_unit_combo_box_callback (GtkWidget  *combo,
                                                 GObject    *config);
static void   ligma_prop_unit_combo_box_notify   (GObject    *config,
                                                 GParamSpec *param_spec,
                                                 GtkWidget  *combo);

/**
 * ligma_prop_unit_combo_box_new:
 * @config:        Object to which property is attached.
 * @property_name: Name of Unit property.
 *
 * Creates a #LigmaUnitComboBox to set and display the value of a Unit
 * property.  See ligma_unit_combo_box_new() for more information.
 *
 * Returns: (transfer full): A new #LigmaUnitComboBox widget.
 *
 * Since: 2.8
 */
GtkWidget *
ligma_prop_unit_combo_box_new (GObject     *config,
                              const gchar *property_name)
{
  GParamSpec   *param_spec;
  GtkWidget    *combo;
  GtkTreeModel *model;
  LigmaUnit      unit;
  GValue        value = G_VALUE_INIT;
  gboolean      show_pixels;
  gboolean      show_percent;

  param_spec = check_param_spec_w (config, property_name,
                                   LIGMA_TYPE_PARAM_UNIT, G_STRFUNC);
  if (! param_spec)
    return NULL;

  g_value_init (&value, param_spec->value_type);

  g_value_set_int (&value, LIGMA_UNIT_PIXEL);
  show_pixels = (g_param_value_validate (param_spec, &value) == FALSE);

  g_value_set_int (&value, LIGMA_UNIT_PERCENT);
  show_percent = (g_param_value_validate (param_spec, &value) == FALSE);

  g_value_unset (&value);

  g_object_get (config,
                property_name, &unit,
                NULL);

  combo = ligma_unit_combo_box_new ();
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
  ligma_unit_store_set_has_pixels (LIGMA_UNIT_STORE (model), show_pixels);
  ligma_unit_store_set_has_percent (LIGMA_UNIT_STORE (model), show_percent);

  ligma_unit_combo_box_set_active (LIGMA_UNIT_COMBO_BOX (combo), unit);

  set_param_spec (G_OBJECT (combo), combo, param_spec);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ligma_prop_unit_combo_box_callback),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_unit_combo_box_notify),
                  combo);

  ligma_widget_set_bound_property (combo, config, property_name);

  gtk_widget_show (combo);

  return combo;
}

static void
ligma_prop_unit_combo_box_callback (GtkWidget *combo,
                                   GObject   *config)
{
  GParamSpec *param_spec;
  LigmaUnit    value;
  LigmaUnit    v;

  param_spec = get_param_spec (G_OBJECT (combo));
  if (! param_spec)
    return;

  value = ligma_unit_combo_box_get_active (LIGMA_UNIT_COMBO_BOX (combo));

  g_object_get (config, param_spec->name, &v, NULL);

  if (v != value)
    {
      /* FIXME ligma_unit_menu_update (menu, &unit); */

      g_signal_handlers_block_by_func (config,
                                       ligma_prop_unit_combo_box_notify,
                                       combo);

      g_object_set (config, param_spec->name, value, NULL);

      g_signal_handlers_unblock_by_func (config,
                                         ligma_prop_unit_combo_box_notify,
                                         combo);
    }
}

static void
ligma_prop_unit_combo_box_notify (GObject    *config,
                                 GParamSpec *param_spec,
                                 GtkWidget  *combo)
{
  LigmaUnit  unit;

  g_object_get (config,
                param_spec->name, &unit,
                NULL);

  g_signal_handlers_block_by_func (combo,
                                   ligma_prop_unit_combo_box_callback,
                                   config);

  ligma_unit_combo_box_set_active (LIGMA_UNIT_COMBO_BOX (combo), unit);

  /* FIXME ligma_unit_menu_update (menu, &unit); */

  g_signal_handlers_unblock_by_func (combo,
                                     ligma_prop_unit_combo_box_callback,
                                     config);
}


/***************/
/*  icon name  */
/***************/

/**
 * ligma_prop_icon_image_new:
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
ligma_prop_icon_image_new (GObject     *config,
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
    ligma_help_set_help_data (image, blurb, NULL);

  g_object_bind_property (config, property_name,
                          image, "icon-name",
                          G_BINDING_BIDIRECTIONAL);

  ligma_widget_set_bound_property (image, config, property_name);

  g_free (icon_name);

  return image;
}


/**************/
/*  expander  */
/**************/

static void   ligma_prop_expanded_notify (GtkExpander *expander,
                                         GParamSpec  *param_spec,
                                         GObject     *config);
static void   ligma_prop_expander_notify (GObject     *config,
                                         GParamSpec  *param_spec,
                                         GtkExpander *expander);


/**
 * ligma_prop_expander_new:
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
ligma_prop_expander_new (GObject     *config,
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
                    G_CALLBACK (ligma_prop_expanded_notify),
                    config);

  connect_notify (config, property_name,
                  G_CALLBACK (ligma_prop_expander_notify),
                  expander);

  ligma_widget_set_bound_property (expander, config, property_name);

  gtk_widget_show (expander);

  return expander;
}

static void
ligma_prop_expanded_notify (GtkExpander *expander,
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
ligma_prop_expander_notify (GObject     *config,
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
                                       ligma_prop_expanded_notify,
                                       config);

      gtk_expander_set_expanded (expander, value);

      g_signal_handlers_unblock_by_func (expander,
                                         ligma_prop_expanded_notify,
                                         config);
    }
}


/*******************************/
/*  private utility functions  */
/*******************************/

static GQuark ligma_prop_widgets_param_spec_quark (void) G_GNUC_CONST;

#define PARAM_SPEC_QUARK (ligma_prop_widgets_param_spec_quark ())

static GQuark
ligma_prop_widgets_param_spec_quark (void)
{
  static GQuark param_spec_quark = 0;

  if (! param_spec_quark)
    param_spec_quark = g_quark_from_static_string ("ligma-config-param-spec");

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
        ligma_help_set_help_data (widget, blurb, NULL);
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
      g_warning ("%s: property '%s' of %s is not a %s",
                 strloc,
                 param_spec->name,
                 g_type_name (param_spec->owner_type),
                 g_type_name (type));
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
ligma_prop_widget_double_to_factor (GBinding     *binding,
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
ligma_prop_widget_double_from_factor (GBinding     *binding,
                                     const GValue *from_value,
                                     GValue       *to_value,
                                     gpointer      user_data)
{
  gdouble *factor = (gdouble*) user_data;
  gdouble val     = g_value_get_double (from_value);

  g_value_set_double (to_value, val / (*factor));

  return TRUE;
}
