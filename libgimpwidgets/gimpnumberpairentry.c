/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpnumberpairentry.c
 * Copyright (C) 2006  Simon Budig       <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann      <sven@gimp.org>
 * Copyright (C) 2007  Martin Nordholts  <martin@svn.gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpnumberpairentry.h"


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
  PROP_RATIO,
  PROP_ASPECT
};

struct _GimpNumberPairEntryPrivate
{
  /* The current number pair displayed in the widget. */
  gdouble      left_number;
  gdouble      right_number;

  /* What number pair that should be displayed when not in user_override mode. */
  gdouble      default_left_number;
  gdouble      default_right_number;

  /* Whether or not the current value in the entry has been explicitly set by
   * the user.
   */
  gboolean     user_override;

  /* What separators that are valid when parsing input, e.g. when the widget is
   * used for aspect ratio, valid separators are typically ':' and '/'.
   */
  const gchar *separators;

  /* Whether or to not to divide the numbers with the greatest common divisor
   * when input ends in '='.
   */
  gboolean     allow_simplification;

  /* What range of values considered valid. */
  gdouble      min_valid_value;
  gdouble      max_valid_value;
};


static void           gimp_number_pair_entry_set_ratio         (GimpNumberPairEntry *entry,
                                                                gdouble              ratio);
static gdouble        gimp_number_pair_entry_get_ratio         (GimpNumberPairEntry *entry);
static void           gimp_number_pair_entry_set_aspect        (GimpNumberPairEntry *entry,
                                                                GimpAspectType       aspect);
static GimpAspectType gimp_number_pair_entry_get_aspect        (GimpNumberPairEntry *entry);

static gint           gimp_number_pair_entry_valid_separator   (GimpNumberPairEntry *entry,
                                                                gchar                canditate);

static void           gimp_number_pair_entry_ratio_to_fraction (gdouble              ratio,
                                                                gdouble             *numerator,
                                                                gdouble             *denominator);

static void           gimp_number_pair_entry_set_property      (GObject             *object,
                                                                guint                property_id,
                                                                const GValue        *value,
                                                                GParamSpec          *pspec);
static void           gimp_number_pair_entry_get_property      (GObject             *object,
                                                                guint                property_id,
                                                                GValue              *value,
                                                                GParamSpec          *pspec);
static gboolean       gimp_number_pair_entry_events            (GtkWidget           *widgett,
                                                                GdkEvent            *event);

static void           gimp_number_pair_entry_update_text       (GimpNumberPairEntry *entry);

static void           gimp_number_pair_entry_parse_text        (GimpNumberPairEntry *entry,
                                                                const gchar         *text);
static gboolean       gimp_number_pair_entry_numbers_in_range  (GimpNumberPairEntry *entry,
                                                                gdouble              left_number,
                                                                gdouble              right_number);

static gchar *        gimp_number_pair_entry_strdup_number_pair_string
                                                               (GimpNumberPairEntry *entry,
                                                                gdouble              left_number,
                                                                gdouble              right_number);



G_DEFINE_TYPE (GimpNumberPairEntry, gimp_number_pair_entry, GTK_TYPE_ENTRY)


#define parent_class gimp_number_pair_entry_parent_class

/* What the user shall end the input with when simplification is desired. */
#define SIMPLIFICATION_CHAR        '='


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

  g_object_class_install_property (object_class, PROP_RATIO,
                                   g_param_spec_double ("ratio",
                                                        "Ratio", NULL,
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE));
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
  entry->priv = G_TYPE_INSTANCE_GET_PRIVATE (entry,
                                             GIMP_TYPE_NUMBER_PAIR_ENTRY,
                                             GimpNumberPairEntryPrivate);
  entry->priv->left_number          = 1.0;
  entry->priv->right_number         = 1.0;
  entry->priv->default_left_number  = 1.0;
  entry->priv->default_right_number = 1.0;
  entry->priv->user_override        = FALSE;
  entry->priv->separators           = NULL;
  entry->priv->allow_simplification = FALSE;
  entry->priv->min_valid_value      = G_MINDOUBLE;
  entry->priv->max_valid_value      = G_MAXDOUBLE;

  g_signal_connect (entry, "focus-out-event",
                    G_CALLBACK (gimp_number_pair_entry_events),
                    NULL);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (gimp_number_pair_entry_events),
                    NULL);
}

/**
 * gimp_number_pair_entry_new:
 * @separators:           A string of valid separators.
 * @allow_simplification: Whether or not to divide the numbers with the greatest
 *                        common divider when input ends in '='.
 * @min_valid_value:      Minimum value of a number considered valid when
 *                        parsing user input.
 * @max_valid_value:      Maximum value of a number considered valid when
 *                        parsing user input.
 *
 * Creates a new #GimpNumberPairEntry widget, which is a GtkEntry that accepts
 * two numbers separated by a separator. Typical input example with a 'x'
 * separator: "377x233".
 *
 * The first separator of @separators is used to display the current value.
 *
 * Return value: The new #GimpNumberPairEntry widget.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_number_pair_entry_new (const gchar *separators,
                            gboolean     allow_simplification,
                            gdouble      min_valid_value,
                            gdouble      max_valid_value)
{
  GimpNumberPairEntry *entry;

  g_return_val_if_fail (separators != NULL,      NULL);
  g_return_val_if_fail (strlen (separators) > 0, NULL);

  entry = g_object_new (GIMP_TYPE_NUMBER_PAIR_ENTRY, NULL);

  entry->priv->separators           = separators;
  entry->priv->allow_simplification = allow_simplification;
  entry->priv->min_valid_value      = min_valid_value;
  entry->priv->max_valid_value      = max_valid_value;

  return GTK_WIDGET (entry);
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
 * @entry: A #gimpnumberpairentry widget.
 * @ratio: Ratio to set in the widget.
 *
 * Sets the numbers of the #GimpNumberPairEntry to have the desired ratio. If
 * the new ratio is different than the previous ratio, the "ratio-changed"
 * signal is emitted.
 *
 * An attempt is made to convert the decimal number into a fraction with
 * left_number and right_number < 1000.
 *
 * Since: GIMP 2.4
 **/
static void
gimp_number_pair_entry_set_ratio (GimpNumberPairEntry *entry,
                                  gdouble              ratio)
{
  gdouble numerator;
  gdouble denominator;

  gimp_number_pair_entry_ratio_to_fraction (ratio, &numerator, &denominator);

  gimp_number_pair_entry_set_values (entry, numerator, denominator);
}

/**
 * gimp_number_pair_entry_get_ratio:
 * @entry: A #gimpnumberpairentry widget.
 *
 * Retrieves the ratio of the numbers displayed by a #GimpNumberPairEntry.
 *
 * Returns: The ratio value.
 *
 * Since: GIMP 2.4
 **/
static gdouble
gimp_number_pair_entry_get_ratio (GimpNumberPairEntry *entry)
{
  return entry->priv->left_number / entry->priv->right_number;
}

/**
 * gimp_number_pair_entry_set_values:
 * @entry:        A #GimpNumberPairEntry widget.
 * @left_number:  Left number in the entry.
 * @right_number: Right number in the entry.
 *
 * Forces setting the numbers displayed by a #GimpNumberPairEntry, ignoring if
 * the user has set his/her own value. The state of user-override will not be
 * changed.
 *
 * Since: GIMP 2.4
 **/
void
gimp_number_pair_entry_set_values (GimpNumberPairEntry *entry,
                                   gdouble              left_number,
                                   gdouble              right_number)
{
  GimpAspectType old_aspect;
  gdouble        old_ratio;
  gdouble        old_left_number;
  gdouble        old_right_number;
  gboolean       numbers_changed   = FALSE;
  gboolean       ratio_changed     = FALSE;

  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));


  /* Store current values */

  old_left_number     = entry->priv->left_number;
  old_right_number    = entry->priv->right_number;
  old_ratio           = gimp_number_pair_entry_get_ratio (entry);


  /* Freeze notification */

  g_object_freeze_notify (G_OBJECT (entry));


  /* Set the new numbers and update the entry */

  entry->priv->left_number  = left_number;
  entry->priv->right_number = right_number;

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

  if (old_left_number  != entry->priv->left_number ||
      old_right_number != entry->priv->right_number)
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
 * @entry:        A #GimpNumberPairEntry widget.
 * @left_number:  Pointer of where to store the left number (may be %NULL).
 * @right_number: Pointer of to store the right number (may be %NULL).
 *
 * Gets the numbers displayed by a #GimpNumberPairEntry.
 *
 * Since: GIMP 2.4
 **/
void
gimp_number_pair_entry_get_values (GimpNumberPairEntry *entry,
                                   gdouble             *left_number,
                                   gdouble             *right_number)
{
  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  if (left_number != NULL)
    *left_number  = entry->priv->left_number;

  if (right_number != NULL)
    *right_number = entry->priv->right_number;
}

/**
 * gimp_number_pair_entry_set_aspect:
 * @entry: A #gimpnumberpairentry widget.
 * @aspect: the new aspect
 *
 * Sets the aspect of the ratio by swapping the left_number and right_number if
 * necessary (or setting them to 1.0 in case that @aspect is
 * %GIMP_ASPECT_SQUARE).
 *
 * Since: GIMP 2.4
 **/
static void
gimp_number_pair_entry_set_aspect (GimpNumberPairEntry *entry,
                                   GimpAspectType       aspect)
{
  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  if (gimp_number_pair_entry_get_aspect (entry) == aspect)
    return;

  switch (aspect)
    {
    case GIMP_ASPECT_SQUARE:
      gimp_number_pair_entry_set_values (entry,
                                         entry->priv->left_number,
                                         entry->priv->left_number);
      break;

    case GIMP_ASPECT_LANDSCAPE:
    case GIMP_ASPECT_PORTRAIT:
      gimp_number_pair_entry_set_values (entry,
                                         entry->priv->right_number,
                                         entry->priv->left_number);
      break;
    }
}

/**
 * gimp_number_pair_entry_get_aspect:
 * @entry: A #gimpnumberpairentry widget.
 *
 * Gets the aspect of the ratio displayed by a #GimpNumberPairEntry.
 *
 * Returns: The entry's current aspect.
 *
 * Since: GIMP 2.4
 **/
static GimpAspectType
gimp_number_pair_entry_get_aspect (GimpNumberPairEntry *entry)
{
  g_return_val_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry), GIMP_ASPECT_SQUARE);

  if (entry->priv->left_number > entry->priv->right_number)
    {
      return GIMP_ASPECT_LANDSCAPE;
    }
  else if (entry->priv->left_number < entry->priv->right_number)
    {
      return GIMP_ASPECT_PORTRAIT;
    }
  else
    {
      return GIMP_ASPECT_SQUARE;
    }
}

static gboolean
gimp_number_pair_entry_events (GtkWidget *widget,
                               GdkEvent  *event)
{
  GimpNumberPairEntry *entry = GIMP_NUMBER_PAIR_ENTRY (widget);
  const gchar         *text;

  switch (event->type)
    {
    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (kevent->keyval != GDK_Return)
          break;
      }
      /* Fall through */

    case GDK_FOCUS_CHANGE:
      text = gtk_entry_get_text (GTK_ENTRY (entry));
      gimp_number_pair_entry_parse_text (entry, text);
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
 * Returns allocated data, must be g_free:d.
 */
static gchar *
gimp_number_pair_entry_strdup_number_pair_string (GimpNumberPairEntry *entry,
                                                  gdouble              left_number,
                                                  gdouble              right_number)
{
  return g_strdup_printf ("%g%c%g",
                          left_number,
                          entry->priv->separators[0],
                          right_number);
}

static void
gimp_number_pair_entry_update_text (GimpNumberPairEntry *entry)
{
  gchar *buffer;

  buffer = gimp_number_pair_entry_strdup_number_pair_string (entry,
                                                             entry->priv->left_number,
                                                             entry->priv->right_number);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  g_free (buffer);
}

static gint
gimp_number_pair_entry_valid_separator (GimpNumberPairEntry *entry,
                                        gchar                candidate)
{
  const gchar *c;

  for (c = entry->priv->separators; *c; c++)
    if (*c == candidate)
      return TRUE;

  return FALSE;
}

static void
gimp_number_pair_entry_parse_text (GimpNumberPairEntry *entry,
                                   const gchar         *text)
{
  gdouble  new_left_number;
  gdouble  new_right_number;

  gchar    separator;
  gchar    simplification_char;
  gchar    dummy;

  gint     parsed_count;

  parsed_count = sscanf (text,
                         " %lf %c %lf %c %c ",
                         &new_left_number,
                         &separator,
                         &new_right_number,
                         &simplification_char,
                         &dummy);


  /* Analyze parsed data */

  switch (parsed_count)
    {
    case EOF:
    case 0:

      /* Unambigous user clear, start using defaults */

      gimp_number_pair_entry_set_values (entry,
                                         entry->priv->default_left_number,
                                         entry->priv->default_right_number);

      entry->priv->user_override = FALSE;
      break;

    case 3:

      /* Valid user entry, enter user-override mode */

      if (gimp_number_pair_entry_valid_separator (entry, separator) &&
          gimp_number_pair_entry_numbers_in_range (entry,
                                                   new_left_number,
                                                   new_right_number))
        {
          gimp_number_pair_entry_set_values (entry,
                                             new_left_number,
                                             new_right_number);

          entry->priv->user_override = TRUE;
        }

      break;


    case 4:

      if (entry->priv->allow_simplification                         &&
          gimp_number_pair_entry_valid_separator (entry, separator) &&
          simplification_char == SIMPLIFICATION_CHAR                &&
          new_right_number != 0.0                                   &&
          gimp_number_pair_entry_numbers_in_range (entry,
                                                   new_left_number,
                                                   new_right_number))
        {
          gimp_number_pair_entry_set_ratio (entry,
                                            new_left_number /
                                            new_right_number);

          entry->priv->user_override = TRUE;
        }

      break;

    case 1:
    case 2:
    case 5:
    default:

      /* Ambigous user input, reset to old values */

      break;
    }

  /* Mak sure the entry text is up to date */

  gimp_number_pair_entry_update_text (entry);

  gtk_editable_set_position (GTK_EDITABLE (entry), -1);
}

static void
gimp_number_pair_entry_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GimpNumberPairEntry *entry = GIMP_NUMBER_PAIR_ENTRY (object);

  switch (property_id)
    {
    case PROP_RATIO:
      gimp_number_pair_entry_set_ratio (entry, g_value_get_double (value));
      break;

    case PROP_LEFT_NUMBER:
      gimp_number_pair_entry_set_values (entry,
                                         g_value_get_double (value),
                                         entry->priv->right_number);
      break;

    case PROP_RIGHT_NUMBER:
      gimp_number_pair_entry_set_values (entry,
                                         entry->priv->left_number,
                                         g_value_get_double (value));
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
  GimpNumberPairEntry *entry = GIMP_NUMBER_PAIR_ENTRY (object);

  switch (property_id)
    {
    case PROP_RATIO:
      g_value_set_double (value, gimp_number_pair_entry_get_ratio (entry));
      break;

    case PROP_LEFT_NUMBER:
      g_value_set_double (value, entry->priv->left_number);
      break;

    case PROP_RIGHT_NUMBER:
      g_value_set_double (value, entry->priv->right_number);
      break;

    case PROP_ASPECT:
      g_value_set_enum (value, gimp_number_pair_entry_get_aspect (entry));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_number_pair_entry_set_default_values (GimpNumberPairEntry *entry,
                                           gdouble              left,
                                           gdouble              right)
{
  g_return_if_fail (GIMP_IS_NUMBER_PAIR_ENTRY (entry));

  entry->priv->default_left_number  = left;
  entry->priv->default_right_number = right;

  if (!entry->priv->user_override)
    {
      gimp_number_pair_entry_set_values (entry,
                                         entry->priv->default_left_number,
                                         entry->priv->default_right_number);
    }
}

static gboolean
gimp_number_pair_entry_numbers_in_range (GimpNumberPairEntry *entry,
                                         gdouble              left_number,
                                         gdouble              right_number)
{
  return left_number  >= entry->priv->min_valid_value &&
         left_number  <= entry->priv->max_valid_value &&
         right_number >= entry->priv->min_valid_value &&
         right_number <= entry->priv->max_valid_value;
}
