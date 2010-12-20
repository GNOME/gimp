/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpnumberpairentry.c
 * Copyright (C) 2006  Simon Budig       <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann      <sven@gimp.org>
 * Copyright (C) 2007  Martin Nordholts  <martin@svn.gnome.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpnumberpairentry.h"


/**
 * SECTION: gimpnumberpairentry
 * @title: GimpNumberPairEntry
 * @short_description: A #GtkEntry subclass to enter ratios.
 *
 * A #GtkEntry subclass to enter ratios.
 **/


#define EPSILON 0.000001


enum
{
  NUMBERS_CHANGED,
  RATIO_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LEFT_NUMBER,
  PROP_RIGHT_NUMBER,
  PROP_DEFAULT_LEFT_NUMBER,
  PROP_DEFAULT_RIGHT_NUMBER,
  PROP_USER_OVERRIDE,
  PROP_SEPARATORS,
  PROP_DEFAULT_TEXT,
  PROP_ALLOW_SIMPLIFICATION,
  PROP_MIN_VALID_VALUE,
  PROP_MAX_VALID_VALUE,
  PROP_RATIO,
  PROP_ASPECT
};

typedef enum
{
  PARSE_VALID,
  PARSE_CLEAR,
  PARSE_INVALID
} ParseResult;

typedef struct
{
  /* The current number pair displayed in the widget. */
  gdouble      left_number;
  gdouble      right_number;

  /* What number pair that should be displayed when not in
     user_override mode. */
  gdouble      default_left_number;
  gdouble      default_right_number;

  /* Whether or not the current value in the entry has been explicitly
   * set by the user.
   */
  gboolean     user_override;

  /* Is the font style currently set to ITALIC or NORMAL ? */
  gboolean     font_italic;

  /* What separators that are valid when parsing input, e.g. when the
   * widget is used for aspect ratio, valid separators are typically
   * ':' and '/'.
   */
  gunichar    *separators;
  glong        num_separators;

  /* A string to be shown in the entry when in automatic mode */
  gchar       *default_text;

  /* Whether or to not to divide the numbers with the greatest common
   * divisor when input ends in '='.
   */
  gboolean     allow_simplification;

  /* What range of values considered valid. */
  gdouble      min_valid_value;
  gdouble      max_valid_value;
} GimpNumberPairEntryPrivate;

#define GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE(obj) \
  ((GimpNumberPairEntryPrivate *) ((GimpNumberPairEntry *) (obj))->priv)


static void         gimp_number_pair_entry_finalize          (GObject             *entry);

static gboolean     gimp_number_pair_entry_valid_separator   (GimpNumberPairEntry *entry,
                                                              gunichar             candidate);
static void         gimp_number_pair_entry_ratio_to_fraction (gdouble              ratio,
                                                              gdouble             *numerator,
                                                              gdouble             *denominator);

static void         gimp_number_pair_entry_set_property      (GObject             *object,
                                                              guint                property_id,
                                                              const GValue        *value,
                                                              GParamSpec          *pspec);
static void         gimp_number_pair_entry_get_property      (GObject             *object,
                                                              guint                property_id,
                                                              GValue              *value,
                                                              GParamSpec          *pspec);
static void         gimp_number_pair_entry_changed           (GimpNumberPairEntry *entry);
static gboolean     gimp_number_pair_entry_events            (GtkWidget           *widget,
                                                              GdkEvent            *event);

static void         gimp_number_pair_entry_update_text       (GimpNumberPairEntry *entry);

static ParseResult  gimp_number_pair_entry_parse_text        (GimpNumberPairEntry *entry,
                                                              const gchar         *text,
                                                              gdouble             *left_value,
                                                              gdouble             *right_value);
static gboolean     gimp_number_pair_entry_numbers_in_range  (GimpNumberPairEntry *entry,
                                                              gdouble              left_number,
                                                              gdouble              right_number);

static gchar *      gimp_number_pair_entry_strdup_number_pair_string
                                                             (GimpNumberPairEntry *entry,
                                                              gdouble left_number,
                                                              gdouble right_number);



G_DEFINE_TYPE (GimpNumberPairEntry, gimp_number_pair_entry, GTK_TYPE_ENTRY)


#define parent_class gimp_number_pair_entry_parent_class

/* What the user shall end the input with when simplification is desired. */
#define SIMPLIFICATION_CHAR  ((gunichar) '=')

#define DEFAULT_SEPARATOR    ((gunichar) ',')


static guint entry_signals[LAST_SIGNAL] = { 0 };


static void
gimp_number_pair_entry_class_init (GimpNumberPairEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GimpNumberPairEntryPrivate));

  entry_signals[NUMBERS_CHANGED] =
    g_signal_new ("numbers-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpNumberPairEntryClass, numbers_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  entry_signals[RATIO_CHANGED] =
    g_signal_new ("ratio-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpNumberPairEntryClass, ratio_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->numbers_changed = NULL;
  klass->ratio_changed   = NULL;

  object_class->set_property = gimp_number_pair_entry_set_property;
  object_class->get_property = gimp_number_pair_entry_get_property;
  object_class->finalize     = gimp_number_pair_entry_finalize;

  g_object_class_install_property (object_class, PROP_LEFT_NUMBER,
                                   g_param_spec_double ("left-number",
                                                        "Left number", NULL,
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        100.0,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_RIGHT_NUMBER,
                                   g_param_spec_double ("right-number",
                                                        "Right number", NULL,
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        100.0,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DEFAULT_LEFT_NUMBER,
                                   g_param_spec_double ("default-left-number",
                                                        "Default left number", NULL,
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        100.0,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DEFAULT_RIGHT_NUMBER,
                                   g_param_spec_double ("default-right-number",
                                                        "Default right number", NULL,
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        100.0,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_USER_OVERRIDE,
                                   g_param_spec_boolean ("user-override",
                                                         "User override",
                                                         "Whether the widget is in 'user override' mode",
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SEPARATORS,
                                   g_param_spec_string ("separators",
                                                        "Separators",
                                                        "A string of valid separators",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class, PROP_DEFAULT_TEXT,
                                   g_param_spec_string ("default-text",
                                                        "Default text",
                                                        "String to show when in automatic mode",
                                                        NULL,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_ALLOW_SIMPLIFICATION,
                                   g_param_spec_boolean ("allow-simplification",
                                                         "Allow simplification",
                                                         "Whether to allow simplification",
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MIN_VALID_VALUE,
                                   g_param_spec_double ("min-valid-value",
                                                        "Min valid value",
                                                        "Minimum value valid when parsing input",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        G_MINDOUBLE,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_MAX_VALID_VALUE,
                                   g_param_spec_double ("max-valid-value",
                                                        "Max valid value",
                                                        "Maximum value valid when parsing input",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_RATIO,
                                   g_param_spec_double ("ratio",
                                                        "Ratio", NULL,
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_ASPECT,
                                   g_param_spec_enum ("aspect",
                                                      "Aspect", NULL,
                                                      GIMP_TYPE_ASPECT_TYPE,
                                                      GIMP_ASPECT_SQUARE,
                                                      GIMP_PARAM_READWRITE));
}

static void
gimp_number_pair_entry_init (GimpNumberPairEntry *entry)
{
  GimpNumberPairEntryPrivate *priv;

  entry->priv = G_TYPE_INSTANCE_GET_PRIVATE (entry,
                                             GIMP_TYPE_NUMBER_PAIR_ENTRY,
                                             GimpNumberPairEntryPrivate);

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  priv->left_number          = 1.0;
  priv->right_number         = 1.0;
  priv->default_left_number  = 1.0;
  priv->default_right_number = 1.0;
  priv->user_override        = FALSE;
  priv->font_italic          = FALSE;
  priv->separators           = NULL;
  priv->default_text         = NULL;
  priv->num_separators       = 0;
  priv->allow_simplification = FALSE;
  priv->min_valid_value      = G_MINDOUBLE;
  priv->max_valid_value      = G_MAXDOUBLE;

  g_signal_connect (entry, "changed",
                    G_CALLBACK (gimp_number_pair_entry_changed),
                    NULL);
  g_signal_connect (entry, "focus-out-event",
                    G_CALLBACK (gimp_number_pair_entry_events),
                    NULL);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (gimp_number_pair_entry_events),
                    NULL);

  gtk_widget_set_direction (GTK_WIDGET (entry), GTK_TEXT_DIR_LTR);
}

static void
gimp_number_pair_entry_finalize (GObject *object)
{
  GimpNumberPairEntryPrivate *priv;

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (object);

  if (priv->separators)
    {
      g_free (priv->separators);
      priv->separators     = NULL;
      priv->num_separators = 0;
    }

  if (priv->default_text)
    {
      g_free (priv->default_text);
      priv->default_text = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_number_pair_entry_new:
 * @separators:           The allowed separators.
 * @allow_simplification: Whether to do simplification on the entered term.
 * @min_valid_value:      The minimum allowed result value.
 * @max_valid_value:      The maximum allowed result value.
 *
 * Creates a new #GimpNumberPairEntry widget, which is a GtkEntry that
 * accepts two numbers separated by a separator. Typical input example
 * with a 'x' separator: "377x233".
 *
 * The widget supports simplification of the entered ratio when the
 * input ends in '=', if "allow-simplification" is TRUE.
 *
 * The "separators" property contains a string of characters valid as
 * separators when parsing input. The first separator is used when
 * displaying the current values.
 *
 * It is possible to specify what range of values that shall be
 * considered as valid when parsing user input, by changing
 * "min-valid-value" and "max-valid-value".
 *
 * The first separator of @separators is used to display the current
 * value.
 *
 * Return value: The new #GimpNumberPairEntry widget.
 *
 * Since: 2.4
 **/
GtkWidget *
gimp_number_pair_entry_new (const gchar *separators,
                            gboolean     allow_simplification,
                            gdouble      min_valid_value,
                            gdouble      max_valid_value)
{
  return g_object_new (GIMP_TYPE_NUMBER_PAIR_ENTRY,
                       "separators",           separators,
                       "allow-simplification", allow_simplification,
                       "min-valid-value",      min_valid_value,
                       "max-valid-value",      max_valid_value,
                       NULL);
}

static void
gimp_number_pair_entry_ratio_to_fraction (gdouble  ratio,
                                          gdouble *numerator,
                                          gdouble *denominator)
{
  gdouble  remainder, next_cf;
  gint     p0, p1, p2;
  gint     q0, q1, q2;

  /* calculate the continued fraction to approximate the desired ratio */

  p0 = 1;
  q0 = 0;
  p1 = floor (ratio);
  q1 = 1;

  remainder = ratio - p1;

  while (fabs (remainder) >= 0.0001 &&
         fabs (((gdouble) p1 / q1) - ratio) > 0.0001)
    {
      remainder = 1.0 / remainder;

      next_cf = floor (remainder);

      p2 = next_cf * p1 + p0;
      q2 = next_cf * q1 + q0;

      /* remember the last two fractions */
      p0 = p1;
      q0 = q1;
      p1 = p2;
      q1 = q2;

      remainder = remainder - next_cf;
    }

  /* only use the calculated fraction if it is "reasonable" */
  if (p1 < 1000 && q1 < 1000)
    {
      *numerator   = p1;
      *denominator = q1;
    }
  else
    {
      *numerator   = ratio;
      *denominator = 1.0;
    }
}

/**
 * gimp_number_pair_entry_set_ratio:
 * @entry: A #GimpNumberPairEntry widget.
 * @ratio: Ratio to set in the widget.
 *
 * Sets the numbers of the #GimpNumberPairEntry to have the desired
 * ratio. If the new ratio is different than the previous ratio, the
 * "ratio-changed" signal is emitted.
 *
 * An attempt is made to convert the decimal number into a fraction
 * with left_number and right_number < 1000.
 *
 * Since: 2.4
 **/
void
gimp_number_pair_entry_set_ratio (GimpNumberPairEntry *entry,
                                  gdouble              ratio)
{
  gdouble numerator;
  gdouble denominator;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  gimp_number_pair_entry_ratio_to_fraction (ratio, &numerator, &denominator);

  gimp_number_pair_entry_set_values (entry, numerator, denominator);
}

/**
 * gimp_number_pair_entry_get_ratio:
 * @entry: A #GimpNumberPairEntry widget.
 *
 * Retrieves the ratio of the numbers displayed by a #GimpNumberPairEntry.
 *
 * Returns: The ratio value.
 *
 * Since: 2.4
 **/
gdouble
gimp_number_pair_entry_get_ratio (GimpNumberPairEntry *entry)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_val_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry), 1.0);

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  return priv->left_number / priv->right_number;
}

/**
 * gimp_number_pair_entry_set_values:
 * @entry: A #GimpNumberPairEntry widget.
 * @left:  Left number in the entry.
 * @right: Right number in the entry.
 *
 * Forces setting the numbers displayed by a #GimpNumberPairEntry,
 * ignoring if the user has set his/her own value. The state of
 * user-override will not be changed.
 *
 * Since: 2.4
 **/
void
gimp_number_pair_entry_set_values (GimpNumberPairEntry *entry,
                                   gdouble              left,
                                   gdouble              right)
{
  GimpNumberPairEntryPrivate *priv;
  GimpAspectType              old_aspect;
  gdouble                     old_ratio;
  gdouble                     old_left_number;
  gdouble                     old_right_number;
  gboolean                    numbers_changed = FALSE;
  gboolean                    ratio_changed   = FALSE;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  /* Store current values */

  old_left_number  = priv->left_number;
  old_right_number = priv->right_number;
  old_ratio        = gimp_number_pair_entry_get_ratio (entry);
  old_aspect       = gimp_number_pair_entry_get_aspect (entry);


  /* Freeze notification */

  g_object_freeze_notify (G_OBJECT (entry));


  /* Set the new numbers and update the entry */

  priv->left_number  = left;
  priv->right_number = right;

  g_object_notify (G_OBJECT (entry), "left-number");
  g_object_notify (G_OBJECT (entry), "right-number");

  gimp_number_pair_entry_update_text (entry);


  /* Find out what has changed */

  if (fabs (old_ratio - gimp_number_pair_entry_get_ratio (entry)) > EPSILON)
    {
      g_object_notify (G_OBJECT (entry), "ratio");

      ratio_changed = TRUE;

      if (old_aspect != gimp_number_pair_entry_get_aspect (entry))
        g_object_notify (G_OBJECT (entry), "aspect");
    }

  if (old_left_number  != priv->left_number ||
      old_right_number != priv->right_number)
    {
      numbers_changed = TRUE;
    }


  /* Thaw */

  g_object_thaw_notify (G_OBJECT (entry));


  /* Emit relevant signals */

  if (numbers_changed)
    g_signal_emit (entry, entry_signals[NUMBERS_CHANGED], 0);

  if (ratio_changed)
    g_signal_emit (entry, entry_signals[RATIO_CHANGED], 0);
}

/**
 * gimp_number_pair_entry_get_values:
 * @entry: A #GimpNumberPairEntry widget.
 * @left:  Pointer of where to store the left number (may be %NULL).
 * @right: Pointer of to store the right number (may be %NULL).
 *
 * Gets the numbers displayed by a #GimpNumberPairEntry.
 *
 * Since: 2.4
 **/
void
gimp_number_pair_entry_get_values (GimpNumberPairEntry *entry,
                                   gdouble             *left,
                                   gdouble             *right)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  if (left != NULL)
    *left  = priv->left_number;

  if (right != NULL)
    *right = priv->right_number;
}

/**
 * gimp_number_pair_entry_set_default_text:
 * @entry:  A #GimpNumberPairEntry widget.
 * @string: Default string.
 *
 * Causes the entry to show a given string when in automatic mode,
 * instead of the default numbers. The only thing this does is making
 * the #GimpNumberPairEntry showing this string, the internal state
 * and API calls are not affected.
 *
 * Set the default string to %NULL to display default values as
 * normal.
 *
 * Since: 2.4
 */
void
gimp_number_pair_entry_set_default_text (GimpNumberPairEntry *entry,
                                         const gchar         *string)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  g_free (priv->default_text);
  priv->default_text = g_strdup (string);

  gimp_number_pair_entry_update_text (entry);

  g_object_notify (G_OBJECT (entry), "default-text");
}

/**
 * gimp_number_pair_entry_get_default_text:
 * @entry:  A #GimpNumberPairEntry widget.
 *
 * Returns: the string manually set to be shown, or %NULL if values are
 *          shown in a normal fashion.
 *
 * Since: 2.4
 */
const gchar *
gimp_number_pair_entry_get_default_text (GimpNumberPairEntry *entry)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_val_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry), NULL);

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  return priv->default_text;
}

/**
 * gimp_number_pair_entry_set_aspect:
 * @entry:  A #GimpNumberPairEntry widget.
 * @aspect: The new aspect.
 *
 * Sets the aspect of the ratio by swapping the left_number and
 * right_number if necessary (or setting them to 1.0 in case that
 * @aspect is %GIMP_ASPECT_SQUARE).
 *
 * Since: 2.4
 **/
void
gimp_number_pair_entry_set_aspect (GimpNumberPairEntry *entry,
                                   GimpAspectType       aspect)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  if (gimp_number_pair_entry_get_aspect (entry) == aspect)
    return;

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  switch (aspect)
    {
    case GIMP_ASPECT_SQUARE:
      gimp_number_pair_entry_set_values (entry,
                                         priv->left_number,
                                         priv->left_number);
      break;

    case GIMP_ASPECT_LANDSCAPE:
    case GIMP_ASPECT_PORTRAIT:
      gimp_number_pair_entry_set_values (entry,
                                         priv->right_number,
                                         priv->left_number);
      break;
    }
}

/**
 * gimp_number_pair_entry_get_aspect:
 * @entry: A #GimpNumberPairEntry widget.
 *
 * Gets the aspect of the ratio displayed by a #GimpNumberPairEntry.
 *
 * Returns: The entry's current aspect.
 *
 * Since: 2.4
 **/
GimpAspectType
gimp_number_pair_entry_get_aspect (GimpNumberPairEntry *entry)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_val_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry), GIMP_ASPECT_SQUARE);

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  if (priv->left_number > priv->right_number)
    {
      return GIMP_ASPECT_LANDSCAPE;
    }
  else if (priv->left_number < priv->right_number)
    {
      return GIMP_ASPECT_PORTRAIT;
    }
  else
    {
      return GIMP_ASPECT_SQUARE;
    }
}

static void
gimp_number_pair_entry_modify_font (GimpNumberPairEntry *entry,
                                    gboolean             italic)
{
  GimpNumberPairEntryPrivate *priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);
  PangoContext               *context;
  PangoFontDescription       *font_desc;

  if (priv->font_italic == italic)
    return;

  context = gtk_widget_get_pango_context (GTK_WIDGET (entry));
  font_desc = pango_context_get_font_description (context);
  font_desc = pango_font_description_copy (font_desc);

  pango_font_description_set_style (font_desc,
                                    italic ?
                                    PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);

  gtk_widget_override_font (GTK_WIDGET (entry), font_desc);

  pango_font_description_free (font_desc);

  priv->font_italic = italic;
}


/**
 * gimp_number_pair_entry_set_user_override:
 * @entry:         A #GimpNumberPairEntry widget.
 * @user_override: %TRUE sets the entry in user overridden mode,
 *                 %FALSE disables.
 *
 * When the entry is not in user overridden mode, the values will
 * change when the default values are changed. When in user overridden
 * mode, setting default values will not affect the active values.
 *
 * Since: 2.4
 **/
void
gimp_number_pair_entry_set_user_override (GimpNumberPairEntry *entry,
                                          gboolean             user_override)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  priv->user_override = user_override;

  if (! user_override)
    {
      gimp_number_pair_entry_set_default_values (entry,
                                                 priv->default_left_number,
                                                 priv->default_right_number);
    }

  gimp_number_pair_entry_modify_font (entry, ! user_override);

  g_object_notify (G_OBJECT (entry), "user-override");
}

/**
 * gimp_number_pair_entry_get_user_override:
 * @entry: A #GimpNumberPairEntry widget.
 *
 * Returns: Whether or not the the widget is in user overridden mode.
 *
 * Since: 2.4
 **/
gboolean
gimp_number_pair_entry_get_user_override (GimpNumberPairEntry *entry)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_val_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry), FALSE);

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  return priv->user_override;
}

static void
gimp_number_pair_entry_changed (GimpNumberPairEntry *entry)
{
  gimp_number_pair_entry_modify_font (entry, FALSE);
}

static gboolean
gimp_number_pair_entry_events (GtkWidget *widget,
                               GdkEvent  *event)
{
  GimpNumberPairEntry        *entry;
  GimpNumberPairEntryPrivate *priv;
  gboolean                    force_user_override;

  entry               = GIMP_NUMBER_PAIR_ENTRY (widget);
  priv                = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);
  force_user_override = FALSE;

  switch (event->type)
    {
    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (kevent->keyval != GDK_KEY_Return   &&
            kevent->keyval != GDK_KEY_KP_Enter &&
            kevent->keyval != GDK_KEY_ISO_Enter)
          break;

        /* If parsing was done due to widgets focus being lost, we only change
         * to user-override mode if the values differ from the default ones. If
         * Return was pressed however, we always switch to user-override mode.
         */
        force_user_override = TRUE;
      }
      /* Fall through */

    case GDK_FOCUS_CHANGE:
      {
        const gchar *text;
        ParseResult  parse_result;
        gdouble      left_value;
        gdouble      right_value;

        text = gtk_entry_get_text (GTK_ENTRY (entry));

        parse_result = gimp_number_pair_entry_parse_text (entry,
                                                          text,
                                                          &left_value,
                                                          &right_value);
        switch (parse_result)
          {
          case PARSE_VALID:
            {
              if (priv->left_number  != left_value  ||
                  priv->right_number != right_value ||
                  force_user_override)
                {
                  gimp_number_pair_entry_set_values (entry,
                                                     left_value,
                                                     right_value);

                  priv->user_override = TRUE;
                  g_object_notify (G_OBJECT (entry), "user-override");
                }
            }
            break;

          case PARSE_CLEAR:

            gimp_number_pair_entry_set_values (entry,
                                               priv->default_left_number,
                                               priv->default_right_number);

            priv->user_override = FALSE;
            g_object_notify (G_OBJECT (entry), "user-override");
            break;

          default:
            break;
          }

        /* Make sure the entry text is up to date */

        gimp_number_pair_entry_update_text (entry);

        gtk_editable_set_position (GTK_EDITABLE (entry), -1);
      }
      break;

    default:
      break;
    }

  return FALSE;
}

/**
 * gimp_number_pair_entry_strdup_number_pair_string:
 * @entry:
 * @left_number:
 * @right_number:
 *
 * Returns: allocated data, must be g_free:d.
 **/
static gchar *
gimp_number_pair_entry_strdup_number_pair_string (GimpNumberPairEntry *entry,
                                                  gdouble              left_number,
                                                  gdouble              right_number)
{
  GimpNumberPairEntryPrivate *priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);
  gchar                       sep[8];
  gint                        len;

  if (priv->num_separators > 0)
    len = g_unichar_to_utf8 (priv->separators[0], sep);
  else
    len = g_unichar_to_utf8 (DEFAULT_SEPARATOR, sep);

  sep[len] = '\0';

  return g_strdup_printf ("%g%s%g", left_number, sep, right_number);
}

static void
gimp_number_pair_entry_update_text (GimpNumberPairEntry *entry)
{
  GimpNumberPairEntryPrivate *priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);
  gchar                      *buffer;

  if (! priv->user_override &&
      priv->default_text != NULL)
    {
      /* Instead of the numbers, show the string explicitly set by a
       * client to show when in automatic mode.
       */
      buffer = g_strdup (priv->default_text);
    }
  else
    {
      buffer = gimp_number_pair_entry_strdup_number_pair_string (entry,
                                                                 priv->left_number,
                                                                 priv->right_number);
    }

  g_signal_handlers_block_by_func (entry,
                                   gimp_number_pair_entry_changed, NULL);

  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  g_free (buffer);

  g_signal_handlers_unblock_by_func (entry,
                                     gimp_number_pair_entry_changed, NULL);
}

static gboolean
gimp_number_pair_entry_valid_separator (GimpNumberPairEntry *entry,
                                        gunichar             candidate)
{
  GimpNumberPairEntryPrivate *priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  if (priv->num_separators > 0)
    {
      gint i;

      for (i = 0; i < priv->num_separators; i++)
        if (priv->separators[i] == candidate)
          return TRUE;
    }
  else if (candidate == DEFAULT_SEPARATOR)
    {
      return TRUE;
    }

  return FALSE;
}

static ParseResult
gimp_number_pair_entry_parse_text (GimpNumberPairEntry *entry,
                                   const gchar         *text,
                                   gdouble             *left_value,
                                   gdouble             *right_value)
{
  GimpNumberPairEntryPrivate *priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  gdouble   new_left_number;
  gdouble   new_right_number;
  gboolean  simplify = FALSE;
  gchar    *end;

  /* try to parse a number */
  new_left_number = strtod (text, &end);

  if (end == text)
    return PARSE_CLEAR;
  else
    text = end;

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  /* check for a valid separator */
  if (! gimp_number_pair_entry_valid_separator (entry, g_utf8_get_char (text)))
    return PARSE_INVALID;
  else
    text = g_utf8_next_char (text);

  /* try to parse another number */
  new_right_number = strtod (text, &end);

  if (end == text)
    return PARSE_INVALID;
  else
    text = end;

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  /* check for the simplification char */
  if (g_utf8_get_char (text) == SIMPLIFICATION_CHAR)
    {
      simplify = priv->allow_simplification;
      text = g_utf8_next_char (text);
    }

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  /* check for trailing garbage */
  if (*text)
    return PARSE_INVALID;

  if (! gimp_number_pair_entry_numbers_in_range (entry,
                                                 new_left_number,
                                                 new_right_number))
    return PARSE_INVALID;

  if (simplify && new_right_number != 0.0)
    {
      gimp_number_pair_entry_ratio_to_fraction (new_left_number /
                                                new_right_number,
                                                left_value,
                                                right_value);
    }
  else
    {
      *left_value = new_left_number;
      *right_value = new_right_number;
    }

  return PARSE_VALID;
}

static void
gimp_number_pair_entry_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpNumberPairEntry        *entry = GIMP_NUMBER_PAIR_ENTRY (object);
  GimpNumberPairEntryPrivate *priv;

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  switch (property_id)
    {
    case PROP_LEFT_NUMBER:
      gimp_number_pair_entry_set_values (entry,
                                         g_value_get_double (value),
                                         priv->right_number);
      break;
    case PROP_RIGHT_NUMBER:
      gimp_number_pair_entry_set_values (entry,
                                         priv->left_number,
                                         g_value_get_double (value));
      break;
    case PROP_DEFAULT_LEFT_NUMBER:
      gimp_number_pair_entry_set_default_values (entry,
                                                 g_value_get_double (value),
                                                 priv->default_right_number);
      break;
    case PROP_DEFAULT_RIGHT_NUMBER:
      gimp_number_pair_entry_set_default_values (entry,
                                                 priv->default_left_number,
                                                 g_value_get_double (value));
      break;
    case PROP_USER_OVERRIDE:
      gimp_number_pair_entry_set_user_override (entry,
                                                g_value_get_boolean (value));
      break;
    case PROP_SEPARATORS:
      g_free (priv->separators);
      priv->num_separators = 0;
      if (g_value_get_string (value))
        priv->separators = g_utf8_to_ucs4 (g_value_get_string (value), -1,
                                           NULL, &priv->num_separators, NULL);
      else
        priv->separators = NULL;
      break;
    case PROP_DEFAULT_TEXT:
      gimp_number_pair_entry_set_default_text (entry,
                                               g_value_get_string (value));
      break;
    case PROP_ALLOW_SIMPLIFICATION:
      priv->allow_simplification = g_value_get_boolean (value);
      break;
    case PROP_MIN_VALID_VALUE:
      priv->min_valid_value = g_value_get_double (value);
      break;
    case PROP_MAX_VALID_VALUE:
      priv->max_valid_value = g_value_get_double (value);
      break;
    case PROP_RATIO:
      gimp_number_pair_entry_set_ratio (entry, g_value_get_double (value));
      break;
    case PROP_ASPECT:
      gimp_number_pair_entry_set_aspect (entry, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_number_pair_entry_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GimpNumberPairEntry        *entry = GIMP_NUMBER_PAIR_ENTRY (object);
  GimpNumberPairEntryPrivate *priv;

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  switch (property_id)
    {
    case PROP_LEFT_NUMBER:
      g_value_set_double (value, priv->left_number);
      break;
    case PROP_RIGHT_NUMBER:
      g_value_set_double (value, priv->right_number);
      break;
    case PROP_DEFAULT_LEFT_NUMBER:
      g_value_set_double (value, priv->default_left_number);
      break;
    case PROP_DEFAULT_RIGHT_NUMBER:
      g_value_set_double (value, priv->default_right_number);
      break;
    case PROP_USER_OVERRIDE:
      g_value_set_boolean (value, priv->user_override);
      break;
    case PROP_SEPARATORS:
      g_value_take_string (value,
                           g_ucs4_to_utf8 (priv->separators,
                                           priv->num_separators,
                                           NULL, NULL, NULL));
      break;
    case PROP_ALLOW_SIMPLIFICATION:
      g_value_set_boolean (value, priv->allow_simplification);
      break;
    case PROP_DEFAULT_TEXT:
      g_value_set_string (value, priv->default_text);
      break;
    case PROP_MIN_VALID_VALUE:
      g_value_set_double (value, priv->min_valid_value);
      break;
    case PROP_MAX_VALID_VALUE:
      g_value_set_double (value, priv->max_valid_value);
      break;
    case PROP_RATIO:
      g_value_set_double (value, gimp_number_pair_entry_get_ratio (entry));
      break;
    case PROP_ASPECT:
      g_value_set_enum (value, gimp_number_pair_entry_get_aspect (entry));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * gimp_number_pair_entry_set_default_values:
 * @entry: A #GimpNumberPairEntry widget.
 * @left:  Default left value in the entry.
 * @right: Default right value in the entry.
 *
 * Since: 2.4
 **/
void
gimp_number_pair_entry_set_default_values (GimpNumberPairEntry *entry,
                                           gdouble              left,
                                           gdouble              right)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  priv->default_left_number  = left;
  priv->default_right_number = right;

  if (! priv->user_override)
    {
      gimp_number_pair_entry_set_values (entry,
                                         priv->default_left_number,
                                         priv->default_right_number);
    }
}

/**
 * gimp_number_pair_entry_get_default_values:
 * @entry: A #GimpNumberPairEntry widget.
 * @left:  Pointer of where to put left value.
 * @right: Pointer of where to put right value.
 *
 * Since: 2.4
 **/
void
gimp_number_pair_entry_get_default_values (GimpNumberPairEntry *entry,
                                           gdouble             *left,
                                           gdouble             *right)
{
  GimpNumberPairEntryPrivate *priv;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  if (left != NULL)
    *left = priv->default_left_number;

  if (right != NULL)
    *right = priv->default_right_number;
}

static gboolean
gimp_number_pair_entry_numbers_in_range (GimpNumberPairEntry *entry,
                                         gdouble              left_number,
                                         gdouble              right_number)
{
  GimpNumberPairEntryPrivate *priv = GIMP_NUMBER_PAIR_ENTRY_GET_PRIVATE (entry);

  return (left_number  >= priv->min_valid_value &&
          left_number  <= priv->max_valid_value &&
          right_number >= priv->min_valid_value &&
          right_number <= priv->max_valid_value);
}
