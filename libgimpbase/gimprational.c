/*
 * gimprational.c
 *
 *  Created on: 19.09.2014
 *      Author: kuhse
 */

#include <stdlib.h>

#include "gimprational.h"


static gpointer                rational_copy                         (gpointer             data);
static gpointer                srational_copy                        (gpointer             data);


G_DEFINE_BOXED_TYPE (Rational, rational,
                     rational_copy,
                     rational_free)
G_DEFINE_BOXED_TYPE (SRational, srational,
                     srational_copy,
                     srational_free)


/**
 * string_to_rational:
 *
 * @string:     a #gchar array
 * @rational:  a pointer to a #Rational struct
 *
 * converts a string, representing one/more rational values into
 * a #Rational struct.
 *
 * Since: 2.10
 */
void
string_to_rational (gchar *string, Rational **rational)
{
  gchar **nom;
  gchar **rats;
  gint count;
  gint i;
  Rational *rval;
  GArray *rational_array;

  rats = g_strsplit (string, " ", -1);

  count = 0;
  while (rats[count])
    count++;

  rational_array = g_array_new (TRUE, TRUE, sizeof (RationalValue));

  for (i = 0; i < count; i++)
    {
      RationalValue or;

      nom = g_strsplit (rats[i], "/", 2);

      if (nom[0] && nom[1])
        {
          or.nom = (guint) atoi (nom [0]);
          or.denom = (guint) atoi (nom [1]);
        }
      else
        count--;

      rational_array = g_array_append_val (rational_array, or);

      g_strfreev (nom);
    }

  g_strfreev (rats);

  rval = g_slice_new (Rational);

  rval->rational_array = rational_array;
  rval->length = count;

  *rational = rval;
}

/**
 * rational_to_string:
 *
 * @rational : a pointer to a @Rational struct
 * @nom      : a pointer to a #gint array
 * @denom    : a pointer to a #gint array
 * @length   : a pointer to a #gint
 *
 * converts a @rational to nom/denum gchar arrays
 *
 * Since: 2.10
 */
void
rational_to_int (Rational *rational, gint **nom, gint **denom, gint *length)
{
  gint i;
  gint *_nom;
  gint *_denom;

  g_return_if_fail (rational != NULL);

  _nom = g_new (gint, rational->length);
  _denom = g_new (gint, rational->length);

  for (i = 0; i < rational->length; i++)
    {
      RationalValue one;
      one = g_array_index(rational->rational_array, RationalValue, i);
      _nom[i] = one.nom;
      _denom[i] = one.denom;
    }

  *nom = _nom;
  *denom = _denom;
  *length = rational->length;
}

/**
 * rational_copy:
 *
 * @data:          a #gpointer to a #Rational structure
 *
 * copy part of the #Rational type
 *
 * Since: 2.10
 */
static gpointer
rational_copy (gpointer data)
{
  struct _Rational *rational = (struct _Rational *) data;
  struct _Rational *copy = g_slice_new (Rational);
  gint i;
  GArray *rlacp;

  if (!data)
    return NULL;

  rlacp = g_array_new (TRUE, TRUE, sizeof (RationalValue));

  for (i = 0; i < rational->length; i++)
    {
      RationalValue or;
      RationalValue cor;

      or = g_array_index (rational->rational_array, RationalValue, i);

      cor.nom = or.nom;
      cor.denom = or.denom;

      rlacp = g_array_append_val (rlacp, cor);
    }

  copy->rational_array = rlacp;
  copy->length = rational->length;

  return (gpointer) copy;
}

/**
 * rational_free:
 *
 * @data:          a #gpointer to a #Rational structure
 *
 * free part of the #Rational type
 *
 * Since: 2.10
 */
void
rational_free (gpointer data)
{
  struct _Rational *rational = (struct _Rational *) data;

  if (rational)
    {
      if (rational->rational_array)
        g_array_free (rational->rational_array, TRUE);
      rational->length = 0;
    }
}

/**
 * string_to_srational:
 *
 * @string:     a #gchar array
 * @srational: a pointer to a #Rational struct
 *
 * converts a string, representing one/more srational values into
 * a #SRational struct.
 *
 * Since: 2.10
 */
void
string_to_srational (gchar *string, SRational **srational)
{
  gchar **nom;
  gchar **srats;
  gint count;
  gint i;
  SRational *srval;
  GArray *srational_array;

  srats = g_strsplit (string, " ", -1);

  count = 0;
  while (srats[count])
    count++;

  srational_array = g_array_new (TRUE, TRUE, sizeof (SRationalValue));

  for (i = 0; i < count; i++)
    {
      SRationalValue or;

      nom = g_strsplit (srats[i], "/", 2);

      if (nom[0] && nom[1])
        {
          or.nom = (gint) atoi (nom [0]);
          or.denom = (guint) atoi (nom [1]);
        }
      else
        count--;

      srational_array = g_array_append_val (srational_array, or);

      g_strfreev (nom);
    }

  g_strfreev (srats);

  srval = g_slice_new (SRational);

  srval->srational_array = srational_array;
  srval->length = count;

  *srational = srval;
}
/**
 * srational_copy:
 *
 * @data:          a #gpointer to a #SRational structure
 *
 * copy part of the #SRational type
 *
 * Since: 2.10
 */
static gpointer
srational_copy (gpointer data)
{
  struct _SRational *srational = (struct _SRational *) data;
  struct _SRational *copy = g_slice_new (SRational);
  gint i;
  GArray *rlacp;

  if (!data)
    return NULL;

  rlacp = g_array_new (TRUE, TRUE, sizeof (SRationalValue));

  for (i = 0; i < srational->length; i++)
    {
      SRationalValue or;
      SRationalValue cor;

      or = g_array_index (srational->srational_array, SRationalValue, i);

      cor.nom = or.nom;
      cor.denom = or.denom;

      rlacp = g_array_append_val (rlacp, cor);
    }

  copy->srational_array = rlacp;
  copy->length = srational->length;

  return (gpointer) copy;
}

/**
 * srational_free:
 *
 * @data:          a #gpointer to a #SRational structure
 *
 * free part of the #SRational type
 *
 * Since: 2.10
 */
void
srational_free (gpointer data)
{
  struct _SRational *srational = (struct _SRational *) data;

  if (srational)
    {
      if (srational->srational_array)
        g_array_free (srational->srational_array, TRUE);
      srational->length = 0;
    }
}
