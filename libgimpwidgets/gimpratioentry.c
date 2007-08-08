/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpratioentry.c
 * Copyright (C) 2006  Simon Budig  <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann  <sven@gimp.org>
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"

#include "gimpwidgetstypes.h"

#include "gimpratioentry.h"


#define EPSILON 0.000001


enum
{
  RATIO_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_RATIO,
  PROP_NUMERATOR,
  PROP_DENOMINATOR,
  PROP_ASPECT
};


static void      gimp_ratio_entry_set_property   (GObject            *object,
                                                  guint               property_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);
static void      gimp_ratio_entry_get_property   (GObject            *object,
                                                  guint               property_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);
static gboolean  gimp_ratio_entry_events         (GtkWidget          *widget,
                                                  GdkEvent           *event);
static void      gimp_ratio_entry_format_text    (GimpRatioEntry     *entry);
static void      gimp_ratio_entry_parse_text     (GimpRatioEntry     *entry,
                                                  const gchar        *text);



G_DEFINE_TYPE (GimpRatioEntry, gimp_ratio_entry, GTK_TYPE_ENTRY)

#define parent_class gimp_ratio_entry_parent_class

static guint entry_signals[LAST_SIGNAL] = { 0 };


static void
gimp_ratio_entry_class_init (GimpRatioEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  entry_signals[RATIO_CHANGED] =
    g_signal_new ("ratio-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpRatioEntryClass, ratio_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  klass->ratio_changed = NULL;

  object_class->set_property = gimp_ratio_entry_set_property;
  object_class->get_property = gimp_ratio_entry_get_property;

  g_object_class_install_property (object_class, PROP_RATIO,
                                   g_param_spec_double ("ratio",
                                                        "Ratio", NULL,
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_NUMERATOR,
                                   g_param_spec_double ("numerator",
                                                        "Numerator of the ratio", NULL,
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_DENOMINATOR,
                                   g_param_spec_double ("denominator",
                                                        "Denominator of the ratio", NULL,
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
gimp_ratio_entry_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpRatioEntry *entry = GIMP_RATIO_ENTRY (object);

  switch (property_id)
    {
    case PROP_RATIO:
      gimp_ratio_entry_set_ratio (entry, g_value_get_double (value));
      break;

    case PROP_NUMERATOR:
      gimp_ratio_entry_set_fraction (entry,
                                     g_value_get_double (value),
                                     entry->denominator);
      break;

    case PROP_DENOMINATOR:
      gimp_ratio_entry_set_fraction (entry,
                                     entry->numerator,
                                     g_value_get_double (value));
      break;

    case PROP_ASPECT:
      gimp_ratio_entry_set_aspect (entry, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_ratio_entry_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpRatioEntry *entry = GIMP_RATIO_ENTRY (object);

  switch (property_id)
    {
    case PROP_RATIO:
      g_value_set_double (value, gimp_ratio_entry_get_ratio (entry));
      break;

    case PROP_NUMERATOR:
      g_value_set_double (value, entry->numerator);
      break;

    case PROP_DENOMINATOR:
      g_value_set_double (value, entry->denominator);
      break;

    case PROP_ASPECT:
      g_value_set_enum (value, gimp_ratio_entry_get_aspect (entry));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_ratio_entry_init (GimpRatioEntry *entry)
{
  entry->numerator   = 1;
  entry->denominator = 1;

  gtk_entry_set_text (GTK_ENTRY (entry), "1:1");

  g_signal_connect (entry, "focus-out-event",
                    G_CALLBACK (gimp_ratio_entry_events),
                    NULL);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (gimp_ratio_entry_events),
                    NULL);
}

/**
 * gimp_ratio_entry_new:
 *
 * Return value: a new #GimpRatioEntry widget
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_ratio_entry_new (void)
{
  return g_object_new (GIMP_TYPE_RATIO_ENTRY, NULL);
}

/**
 * gimp_ratio_entry_set_ratio:
 * @entry: a #GimpRatioEntry widget
 * @ratio: ratio to set in the widget
 *
 * Sets the ratio displayed by a #GimpRatioEntry. If the new ratio is
 * different than the previous ratio, the "ratio-changed" signal is
 * emitted.
 *
 * An attempt is made to convert the decimal number into a fraction with
 * numerator and denominator < 1000.
 *
 * Since: GIMP 2.4
 **/
void
gimp_ratio_entry_set_ratio (GimpRatioEntry *entry,
                            gdouble         ratio)
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
    gimp_ratio_entry_set_fraction (entry, p1, q1);
  else
    gimp_ratio_entry_set_fraction (entry, ratio, 1.0);
}

/**
 * gimp_ratio_entry_get_ratio:
 * @entry: a #GimpRatioEntry widget
 *
 * Retrieves the ratio value displayed by a #GimpRatioEntry.
 *
 * Returns: The ratio value.
 *
 * Since: GIMP 2.4
 **/
gdouble
gimp_ratio_entry_get_ratio (GimpRatioEntry *entry)
{
  return entry->denominator == 0.0 ? entry->numerator :
                                     entry->numerator / entry->denominator;
}

/**
 * gimp_ratio_entry_set_fraction:
 * @entry: a #GimpRatioEntry widget
 * @numerator: numerator of the fraction to set in the widget
 * @denominator: denominator of the fraction to set in the widget
 *
 * Sets the fraction displayed by a #GimpRatioEntry. If the resulting
 * ratio is different to the previously set ratio, the "ratio-changed"
 * signal is emitted.
 *
 * If the denominator is zero, the #GimpRatioEntry will silently
 * convert it to 1.0.
 *
 * Since: GIMP 2.4
 **/
void
gimp_ratio_entry_set_fraction (GimpRatioEntry *entry,
                               gdouble         numerator,
                               gdouble         denominator)
{
  GimpAspectType  old_aspect;
  gdouble         old_ratio;

  g_return_if_fail (GIMP_IS_RATIO_ENTRY (entry));

  old_aspect = gimp_ratio_entry_get_aspect (entry);
  old_ratio  = gimp_ratio_entry_get_ratio (entry);

  entry->numerator   = numerator;
  entry->denominator = denominator;

  if (entry->denominator < 0)
    {
      entry->numerator *= -1;
      entry->denominator *= -1;
    }

  if (entry->denominator < EPSILON)
    entry->denominator = 1.0;

  gimp_ratio_entry_format_text (entry);

  g_object_freeze_notify (G_OBJECT (entry));

  g_object_notify (G_OBJECT (entry), "numerator");
  g_object_notify (G_OBJECT (entry), "denominator");

  if (fabs (old_ratio - entry->numerator / entry->denominator) > EPSILON)
    {
      g_object_notify (G_OBJECT (entry), "ratio");

      if (old_aspect != gimp_ratio_entry_get_aspect (entry))
        g_object_notify (G_OBJECT (entry), "aspect");

      g_object_thaw_notify (G_OBJECT (entry));

      g_signal_emit (entry, entry_signals[RATIO_CHANGED], 0);
    }
  else
    {
      g_object_thaw_notify (G_OBJECT (entry));
    }
}

/**
 * gimp_ratio_entry_get_fraction:
 * @entry: a #GimpRatioEntry widget
 * @numerator: pointer to store the numerator of the fraction
 * @denominator: pointer to store the denominator of the fraction
 *
 * Gets the fraction displayed by a #GimpRatioEntry.
 *
 * The denominator may be zero if the #GimpRatioEntry shows just a single
 * value. You can use #gimp_ratio_entry_get_ratio to retrieve the ratio
 * as a single decimal value.
 *
 * Since: GIMP 2.4
 **/
void
gimp_ratio_entry_get_fraction (GimpRatioEntry *entry,
                               gdouble        *numerator,
                               gdouble        *denominator)
{
  g_return_if_fail (GIMP_IS_RATIO_ENTRY (entry));
  g_return_if_fail (numerator != NULL);
  g_return_if_fail (denominator != NULL);

  *numerator = entry->numerator;
  *denominator = entry->denominator;
}

/**
 * gimp_ratio_entry_set_aspect:
 * @entry: a #GimpRatioEntry widget
 * @aspect: the new aspect
 *
 * Sets the aspect of the ratio by swapping the numerator and denominator
 * (or setting them to 1.0 in case that @aspect is %GIMP_ASPECT_SQUARE).
 *
 * Since: GIMP 2.4
 **/
void
gimp_ratio_entry_set_aspect (GimpRatioEntry *entry,
                             GimpAspectType  aspect)
{
  g_return_if_fail (GIMP_IS_RATIO_ENTRY (entry));

  if (gimp_ratio_entry_get_aspect (entry) == aspect)
    return;

  switch (aspect)
    {
    case GIMP_ASPECT_SQUARE:
      gimp_ratio_entry_set_fraction (entry, 1.0, 1.0);
      break;

    case GIMP_ASPECT_LANDSCAPE:
    case GIMP_ASPECT_PORTRAIT:
      gimp_ratio_entry_set_fraction (entry,
                                     entry->denominator, entry->numerator);
      break;
    }
}

/**
 * gimp_ratio_entry_get_aspect:
 * @entry: a #GimpRatioEntry widget
 *
 * Gets the aspect of the ratio displayed by a #GimpRatioEntry.
 *
 * Returns: The entry's current aspect.
 *
 * Since: GIMP 2.4
 **/
GimpAspectType
gimp_ratio_entry_get_aspect (GimpRatioEntry *entry)
{
  g_return_val_if_fail (GIMP_IS_RATIO_ENTRY (entry), GIMP_ASPECT_SQUARE);

  if (entry->numerator > entry->denominator)
    {
      return GIMP_ASPECT_LANDSCAPE;
    }
  else if (entry->numerator < entry->denominator)
    {
      return GIMP_ASPECT_PORTRAIT;
    }
  else
    {
      return GIMP_ASPECT_SQUARE;
    }
}

static gboolean
gimp_ratio_entry_events (GtkWidget *widget,
                         GdkEvent  *event)
{
  GimpRatioEntry *entry = GIMP_RATIO_ENTRY (widget);
  const gchar    *text;

  switch (event->type)
    {
    case GDK_KEY_PRESS:
      if (((GdkEventKey *) event)->keyval != GDK_Return)
        break;
      /*  else fall through  */

    case GDK_FOCUS_CHANGE:
      text = gtk_entry_get_text (GTK_ENTRY (entry));
      gimp_ratio_entry_parse_text (entry, text);
      break;

    default:
      /*  do nothing  */
      break;
    }

  return FALSE;
}

static void
gimp_ratio_entry_format_text (GimpRatioEntry *entry)
{
  gchar *buffer;

  buffer = g_strdup_printf ("%g:%g", entry->numerator, entry->denominator);

  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  g_free (buffer);
}

static void
gimp_ratio_entry_parse_text (GimpRatioEntry *entry,
                             const gchar    *text)
{
  gint    count;
  gchar   op1, op2, dummy;
  gdouble num = 1.0, denom = 0.0;

  count = sscanf (text, " %lf %c %lf %c %c ", &num, &op1, &denom, &op2, &dummy);

  switch (count)
    {
    case EOF:
    case 0:
      gimp_ratio_entry_set_fraction (entry, 1.0, 1.0);
      break;

    case 1:
      gimp_ratio_entry_set_fraction (entry, num, 1.0);
      break;

    case 2:
      if (op1 == '=')
        gimp_ratio_entry_set_ratio (entry, num);
      break;

    case 3:
      if (op1 == ':' || op1 == '/')
        gimp_ratio_entry_set_fraction (entry, num, denom);
      break;

    case 4:
      if ((op1 == ':' || op1 == '/') && denom != 0 && op2 == '=')
        {
          if (fabs (denom - 1.0) < EPSILON)
            gimp_ratio_entry_set_ratio (entry, num / denom);
          else
            gimp_ratio_entry_set_fraction (entry, num / denom, 1.0);
        }
      break;

    case 5:
      /* we have additional stuff at the end - malformed input. */
      break;

    default:
      break;
    }

  /* sanitize text */
  gimp_ratio_entry_format_text (entry);

  gtk_editable_set_position (GTK_EDITABLE (entry), -1);
}
