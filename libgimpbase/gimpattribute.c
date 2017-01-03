/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpattribute.c
 * Copyright (C) 2014  Hartmut Kuhse <hatti@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#endif

#include "gimpbasetypes.h"
#include "gimpattribute.h"
#include "gimprational.h"

#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_regex_unref0(var) ((var == NULL) ? NULL : (var = (g_regex_unref (var), NULL)))
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))

typedef struct _TagTime TagTime;
typedef struct _TagDate TagDate;

struct _TagTime {
    int tag_time_sec;
    int tag_time_min;
    int tag_time_hour;
    int tag_tz_hour;
    int tag_tz_min;
  };

struct _TagDate {
    int tag_year;
    int tag_month;
    int tag_day;
  };


typedef struct _GimpAttributeClass GimpAttributeClass;
typedef struct _GimpAttributePrivate GimpAttributePrivate;

struct _GimpAttribute {
  GObject               parent_instance;
  GimpAttributePrivate *priv;
};

struct _GimpAttributeClass {
  GObjectClass parent_class;
};

struct _GimpAttributePrivate {
  gchar                      *name;
  gchar                      *sorted_name;

  gchar                      *tag_value;
  gchar                      *interpreted_value;

  gchar                      *exif_type;

  GimpAttributeValueType      value_type;
  GimpAttributeTagType        tag_type;

  gchar                      *attribute_type;
  gchar                      *attribute_ifd;
  gchar                      *attribute_tag;
  gboolean                    is_new_name_space;

  gboolean                    has_structure;
  GimpAttributeStructureType  structure_type;
  GSList                     *attribute_structure;
};

enum  {
  PROP_0,
  PROP_NAME,
  PROP_LONG_VALUE,
  PROP_SLONG_VALUE,
  PROP_SHORT_VALUE,
  PROP_SSHORT_VALUE,
  PROP_DATE_VALUE,
  PROP_TIME_VALUE,
  PROP_ASCII_VALUE,
  PROP_BYTE_VALUE,
  PROP_MULTIPLE_VALUE,
  PROP_RATIONAL_VALUE,
  PROP_SRATIONAL_VALUE
};

static const gchar *xmp_namespaces[] =
{
    "dc",
    "xmp",
    "xmpRights",
    "xmpMM",
    "xmpBJ",
    "xmpTPg",
    "xmpDM",
    "pdf",
    "photoshop",
    "crs",
    "tiff",
    "exif",
    "aux",
    "plus",
    "digiKam",
    "iptc",
    "iptcExt",
    "Iptc4xmpCore",
    "Iptc4xmpExt",
    "MicrosoftPhoto",
    "kipi",
    "mediapro",
    "expressionmedia",
    "MP",
    "MPRI",
    "MPReg",
    "mwg-rs"
};


static gpointer gimp_attribute_parent_class = NULL;
static gint counter = 0;


static GimpAttribute*          gimp_attribute_construct              (GType                object_type,
                                                                      const gchar         *name);
static void                    gimp_attribute_finalize               (GObject             *obj);
static void                    gimp_attribute_get_property           (GObject             *object,
                                                                      guint                property_id,
                                                                      GValue              *value,
                                                                      GParamSpec          *pspec);
static void                    gimp_attribute_set_property           (GObject             *object,
                                                                      guint                property_id,
                                                                      const GValue        *value,
                                                                      GParamSpec          *pspec);
static void                    gimp_attribute_set_name               (GimpAttribute       *attribute,
                                                                      const gchar         *value);
static gboolean                get_tag_time                          (gchar               *input,
                                                                      TagTime             *tm);
static gboolean                get_tag_date                          (gchar               *input,
                                                                      TagDate             *dt);
static gchar *                 gimp_attribute_escape_value           (gchar               *name,
                                                                      gchar               *value,
                                                                      gboolean            *encoded);
static gchar *                 gimp_attribute_get_structure_number   (gchar               *cptag,
                                                                      gint                 start,
                                                                      gint                *struct_number);

static gchar*                  string_replace_str                    (const gchar         *original,
                                                                      const gchar         *old_pattern,
                                                                      const gchar         *replacement);
static gint                    string_index_of                       (gchar               *haystack,
                                                                      gchar               *needle,
                                                                      gint                 start_index);
static glong                   string_strnlen                        (gchar               *str,
                                                                      glong                maxlen);
static gchar*                  string_substring                      (gchar               *attribute,
                                                                      glong                offset,
                                                                      glong                len);


/**
 * gimp_attribute_new_string:
 *
 * @name:         a constant #gchar that describes the name of the attribute
 * @value:        a #gchar value, representing a time
 * @type:         a #GimpAttributeTagType
 *
 * Creates a new #GimpAttribute object with @name as name and a #gchar @value
 * as value of type @type.
 * @value is converted to the correct type.
 *
 * Return value:  a new @GimpAttribute
 *
 * Since: 2.10
 */
GimpAttribute*
gimp_attribute_new_string (const gchar *name,
                           gchar *value,
                           GimpAttributeValueType type)
{
  GimpAttribute *attribute = NULL;
  GimpAttributePrivate *private;

  attribute = gimp_attribute_construct (GIMP_TYPE_ATTRIBUTE, name);
  if (attribute)
    {
      private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
      private->tag_value = g_strdup (value);
      private->value_type = type;
    }

  return attribute;
}

/**
 * gimp_attribute_get_name:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value: full name of the #GimpAttribute object
 * e.g. "Exif.Image.XResolution"
 *
 * Since: 2.10
 */
const gchar*
gimp_attribute_get_name (GimpAttribute* attribute)
{
  GimpAttributePrivate *private;
  g_return_val_if_fail (attribute != NULL, NULL);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  return (const gchar *) private->name;
}

/**
 * gimp_attribute_get_sortable_name:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value: name of the #GimpAttribute object for sorting
 * e.g. from tag:
 *  "xmp.xmpMM.History[2]..."
 * it returns:
 *  "xmp.xmpMM.History[000002]..."
 *
 * Since: 2.10
 */
const gchar*
gimp_attribute_get_sortable_name (GimpAttribute* attribute)
{
  GimpAttributePrivate *private;
  g_return_val_if_fail (attribute != NULL, NULL);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  if (private->sorted_name)
    return (const gchar *) private->sorted_name;
  else
    return (const gchar *) private->name;
}

/**
 * gimp_attribute_get_attribute_type:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value: attribute type of the #GimpAttribute object
 * e.g. "exif", "iptc", ...
 *
 * Since: 2.10
 */
const gchar*
gimp_attribute_get_attribute_type (GimpAttribute* attribute)
{
  GimpAttributePrivate *private;
  g_return_val_if_fail (attribute != NULL, NULL);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  return (const gchar *) private->attribute_type;
}

/**
 * gimp_attribute_get_attribute_ifd:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value: ifd of the #GimpAttribute object
 *
 * Since: 2.10
 */
const gchar*
gimp_attribute_get_attribute_ifd (GimpAttribute* attribute)
{
  GimpAttributePrivate *private;
  g_return_val_if_fail (attribute != NULL, NULL);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  return (const gchar *) private->attribute_ifd;
}

/**
 * gimp_attribute_get_attribute_tag:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value: tag of the #GimpAttribute object
 *
 * Since: 2.10
 */
const gchar*
gimp_attribute_get_attribute_tag (GimpAttribute* attribute)
{
  GimpAttributePrivate *private;
  g_return_val_if_fail (attribute != NULL, NULL);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  return (const gchar *) private->attribute_tag;
}

/**
 * gimp_attribute_set_value_type:
 *
 * @attribute:    a @GimpAttribute
 * @value_type:   a @GimpAttributeValueType
 *
 * sets value type of the #GimpAttribute object
 *
 * Since: 2.10
 */
void
gimp_attribute_set_value_type (GimpAttribute* attribute,
                               GimpAttributeValueType value_type)
{
  GimpAttributePrivate *private;
  g_return_if_fail (attribute != NULL);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  private->value_type = value_type;
}

/**
 * gimp_attribute_get_value_type:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value: value type of the #GimpAttribute object
 *
 * Since: 2.10
 */
GimpAttributeValueType
gimp_attribute_get_value_type (GimpAttribute* attribute)
{
  GimpAttributePrivate *private;
  g_return_val_if_fail (attribute != NULL, TYPE_INVALID);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  return private->value_type;
}

/**
 * gimp_attribute_get_attribute_structure:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value: attribute_structure of the #GimpAttribute object
 * This is the start value of a xmpBag sequence.
 *
 * Since: 2.10
 */
GSList *
gimp_attribute_get_attribute_structure (GimpAttribute* attribute)
{
  GimpAttributePrivate *private;
  g_return_val_if_fail (attribute != NULL, NULL);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  return private->attribute_structure;
}
/**
 * gimp_attribute_is_new_namespace:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value: if namespace must be defined or not
 *
 * Since: 2.10
 */
const gboolean
gimp_attribute_is_new_namespace (GimpAttribute* attribute)
{
  GimpAttributePrivate *private;
  g_return_val_if_fail (attribute != NULL, FALSE);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  return (const gboolean) private->is_new_name_space;
}

/**
 * gimp_attribute_get_value:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value:  #GValue of the #GimpAttribute object
 * The type of value is represented by glib-types
 *
 * Since: 2.10
 */
GValue
gimp_attribute_get_value (GimpAttribute *attribute)
{
  GimpAttributePrivate *private;
  GValue val     = G_VALUE_INIT;
  gchar                *value_string;

  g_return_val_if_fail(GIMP_IS_ATTRIBUTE (attribute), val);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  value_string = private->tag_value;

  switch (private->value_type)
    {
    case TYPE_LONG:
      {
        g_value_init (&val, G_TYPE_ULONG);
        g_value_set_ulong (&val, (gulong) atol (value_string));
      }
      break;
    case TYPE_SLONG:
      {
        g_value_init (&val, G_TYPE_LONG);
        g_value_set_long (&val, (glong) atol (value_string));
      }
      break;
    case TYPE_SHORT:
      {
        g_value_init (&val, G_TYPE_UINT);
        g_value_set_uint (&val, (guint) atoi (value_string));
      }
      break;
    case TYPE_SSHORT:
      {
        g_value_init (&val, G_TYPE_INT);
        g_value_set_int (&val, (gint) atoi (value_string));
      }
      break;
    case TYPE_DATE:
      {
        g_value_init (&val, G_TYPE_STRING);
        g_value_set_string (&val, value_string);
      }
      break;
    case TYPE_TIME:
      {
        g_value_init (&val, G_TYPE_STRING);
        g_value_set_string (&val, value_string);
      }
      break;
    case TYPE_ASCII:
      {
        g_value_init (&val, G_TYPE_STRING);
        g_value_set_string (&val, value_string);
      }
      break;
    case TYPE_UNICODE:
      {
        g_value_init (&val, G_TYPE_STRING);
        g_value_set_string (&val, value_string);
      }
      break;
    case TYPE_BYTE:
      {
        g_value_init (&val, G_TYPE_UCHAR);
        g_value_set_uchar (&val, value_string[0]);
      }
      break;
    case TYPE_MULTIPLE:
      {
        gchar **result;

        g_value_init (&val, G_TYPE_STRV);
        result = g_strsplit (value_string, "\n", 0);
        g_value_set_boxed (&val, result);
      }
      break;
    case TYPE_RATIONAL:
      {
        gchar **nom;
        gchar **rats;
        gint count;
        gint i;
        Rational *rval;
        GArray *rational_array;

        rats = g_strsplit (value_string, " ", -1);

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

        rval = g_slice_new (Rational);

        rval->rational_array = rational_array;
        rval->length = count;

        g_value_init (&val, GIMP_TYPE_RATIONAL);
        g_value_set_boxed (&val, rval);

        g_strfreev (rats);
      }
      break;
    case TYPE_SRATIONAL:
      {
        gchar **nom;
        gchar **srats;
        gint count;
        gint i;
        SRational *srval;
        GArray *srational_array;

        srats = g_strsplit (value_string, " ", -1);

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

        srval = g_slice_new (SRational);

        srval->srational_array = srational_array;
        srval->length = count;

        g_value_init (&val, GIMP_TYPE_SRATIONAL);
        g_value_set_boxed (&val, srval);

        g_strfreev (srats);
      }
      break;
    case TYPE_UNKNOWN:
      default:
      break;
    }

  return val;
}

/**
 * gimp_attribute_get_string:
 *
 * @attribute:    a @GimpAttribute
 *
 * Return value:  newly allocated #gchar string representation of the #GimpAttribute object.
 * The return is always a valid value according to the #GimpAttributeValueType
 *
 * Since: 2.10
 */
gchar*
gimp_attribute_get_string (GimpAttribute *attribute)
{
  GimpAttributePrivate *private;
  gchar                *val = NULL;
  TagDate               dt = {0};
  TagTime               tm = {0};

  g_return_val_if_fail(GIMP_IS_ATTRIBUTE (attribute), NULL);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  switch (private->value_type)
    {
    case TYPE_INVALID:
      break;
    case TYPE_LONG:
      {
        val = g_strdup_printf ("%lu", (gulong) atol (private->tag_value));
      }
      break;
    case TYPE_SLONG:
      {
        val = g_strdup_printf ("%ld", (glong) atol (private->tag_value));
      }
      break;
    case TYPE_FLOAT:
      {
        val = g_strdup_printf ("%.5f", (gfloat) atof (private->tag_value));
      }
      break;
    case TYPE_DOUBLE:
      {
        val = g_strdup_printf ("%.6f", (gdouble) atof (private->tag_value));
      }
      break;
    case TYPE_SHORT:
      {
        val = g_strdup_printf ("%u", (gushort) atoi (private->tag_value));
      }
      break;
    case TYPE_SSHORT:
      {
        val = g_strdup_printf ("%d", (gshort) atoi (private->tag_value));
      }
      break;
    case TYPE_DATE:
      {
        get_tag_date (private->tag_value, &dt);

        val = g_strdup_printf ("%4d-%02d-%02d", dt.tag_year,
                                            dt.tag_month,
                                            dt.tag_day);
      }
      break;
    case TYPE_TIME:
      {
        gchar *tz_h = NULL;
        get_tag_time (private->tag_value, &tm);

        tz_h = tm.tag_tz_hour < 0 ?
            g_strdup_printf ("-%02d", tm.tag_tz_hour) :
            g_strdup_printf ("+%02d", tm.tag_tz_hour);

        val = g_strdup_printf ("%02d:%02d:%02d%s:%02d", tm.tag_time_hour,
                                                  tm.tag_time_min,
                                                  tm.tag_time_sec,
                                                  tz_h,
                                                  tm.tag_tz_min);
        g_free (tz_h);
      }
      break;
    case TYPE_ASCII:
      {
        val = g_strdup (private->tag_value);
      }
      break;
    case TYPE_UNICODE:
      {
        val = g_strdup (private->tag_value);
      }
      break;
    case TYPE_BYTE:
      {
        val = g_strdup_printf ("%c", private->tag_value[0]);
      }
      break;
    case TYPE_MULTIPLE:
      {
        val = g_strdup (private->tag_value);
      }
      break;
    case TYPE_RATIONAL:
      {
        gint i;
        GString        *string;
        Rational      *rational;

        string_to_rational (private->tag_value, &rational);

        string = g_string_new (NULL);

        for (i = 0; i < rational->length; i++)
          {
            RationalValue or;

            or = g_array_index (rational->rational_array, RationalValue, i);

            g_string_append_printf (string, "%u/%u ",
                                    or.nom,
                                    or.denom);
          }
        g_string_truncate (string, string->len-1);
        val = g_string_free (string, FALSE);
      }
      break;
    case TYPE_SRATIONAL:
      {
        gint i;
        GString        *string;
        SRational      *srational;

        string_to_srational (private->tag_value, &srational);

        string = g_string_new (NULL);

        for (i = 0; i < srational->length; i++)
          {
            SRationalValue or;

            or = g_array_index (srational->srational_array, SRationalValue, i);

            g_string_append_printf (string, "%u/%u ",
                                    or.nom,
                                    or.denom);
          }

        g_string_truncate (string, string->len-1);
        val = g_string_free (string, FALSE);
      }
      break;
    case TYPE_UNKNOWN:
      default:
      break;
    }
  return val;
}

/**
 * gimp_attribute_get_interpreted_string:
 *
 * @attribute:    a #GimpAttribute
 *
 * returns the interpreted value or the pure string, if no
 * interpreted value is found
 *
 * Since: 2.10
 */
const gchar *
gimp_attribute_get_interpreted_string (GimpAttribute *attribute)
{
  GimpAttributePrivate *private;

  g_return_val_if_fail(GIMP_IS_ATTRIBUTE (attribute), NULL);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  if (private->interpreted_value)
    return (const gchar *) private->interpreted_value;
  else
    return (const gchar *) private->tag_value;
}

/**
 * gimp_attribute_set_interpreted_string:
 *
 * @attribute:    a #GimpAttribute
 * @value:        a constant #gchar
 *
 * sets the interpreted string of the #GimpAttribute object to
 * a @value
 *
 * @value can be freed after
 *
 * Since: 2.10
 */
void
gimp_attribute_set_interpreted_string (GimpAttribute* attribute, const gchar* value)
{
  GimpAttributePrivate *private;

  g_return_if_fail(GIMP_IS_ATTRIBUTE (attribute));
  g_return_if_fail (value != NULL);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  private->interpreted_value = g_strdup (value);
}

/**
 * gimp_attribute_get_tag_type:
 *
 * @attribute:    a @GimpAttribute
 *
 *
 * Return value:  #GimpAttributeTagType
 * The tag type of the Attribute
 *
 * Since: 2.10
 */
GimpAttributeTagType
gimp_attribute_get_tag_type (GimpAttribute *attribute)
{
  GimpAttributePrivate *private;

  g_return_val_if_fail (attribute != NULL, TAG_INVALID);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  return private->tag_type;
}

void
gimp_attribute_print (GimpAttribute *attribute)
{
  const gchar    *tag;
  const gchar    *interpreted;
  gchar          *value;

  g_return_if_fail(GIMP_IS_ATTRIBUTE (attribute));

  tag = gimp_attribute_get_name (attribute);
  value = gimp_attribute_get_string (attribute);
  interpreted = gimp_attribute_get_interpreted_string (attribute);

  g_print ("---\n%p\nTag: %s\n\tValue:%s\n\tInterpreted value:%s\n", attribute, tag, value, interpreted);

}

/**
 * gimp_attribute_get_gps_degree:
 *
 * @attribute:    a @GimpAttribute
 *
 * returns the degree representation of the #GimpAttribute GPS value,
 * -999.9 otherwise.
 *
 * Return value:  #gdouble degree representation of the #GimpAttributes GPS value
 *
 * Since: 2.10
 */
gdouble
gimp_attribute_get_gps_degree (GimpAttribute *attribute)
{
  gdouble               return_val;
  GimpAttributePrivate *private;

  g_return_val_if_fail (attribute != NULL, TAG_INVALID);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  if (private->value_type == TYPE_RATIONAL)
    {
      gint i;
      gdouble        r_val    = 0.0;
      Rational      *rational;

      string_to_rational (private->tag_value, &rational);

      for (i = 0; i < rational->length; i++)
        {
          RationalValue or;

          or = g_array_index (rational->rational_array, RationalValue, i);

          r_val = (gfloat)or.nom / (gfloat)or.denom;

          if (i == 0)
            return_val = r_val;
          else if (i == 1)
            return_val = return_val + (r_val / 60);
          else if (i == 2)
            return_val = return_val + (r_val / 3600);
          else return -999.9;
        }

      return return_val;
    }
  else
    return -999.9;

}

/**
 * gimp_attribute_get_xml:
 *
 * @attribute:    a @GimpAttribute
 *
 * returns the xml representation of the #GimpAttribute
 * in form:
 *
 * <tag name="name" type="GimpAttributeValueType">
 * <value [encoding=base64]>value</value>
 * [<interpreted [encoding=base64]>interpreted value</interpreted>]
 * </tag>
 *
 * Return value:  #gchar xml representation of the #GimpAttribute object
 *
 * Since: 2.10
 */
gchar*
gimp_attribute_get_xml (GimpAttribute *attribute)
{
  gchar                *v_escaped            = NULL;
  gchar                *i_escaped            = NULL;
  gchar                *interpreted_tag_elem = NULL;
  gboolean              is_interpreted       = FALSE;
  gboolean              encoded;
  gchar                *start_tag_elem;
  gchar                *end_tag_elem;
  gchar                *value_tag_elem;
  gchar                *struct_tag_elem      = NULL;
  gboolean              utf                  = TRUE;
  GimpAttributePrivate *private;
  GString              *xml;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTE (attribute), NULL);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  xml = g_string_new (NULL);

  if(private->interpreted_value && g_strcmp0 (private->tag_value, private->interpreted_value))
    {
      is_interpreted = TRUE;
    }

  v_escaped = gimp_attribute_escape_value (private->name, private->tag_value, &encoded);

  start_tag_elem = g_strdup_printf ("  <attribute name=\"%s\" type=\"%d\">\n", private->name, private->value_type);
  end_tag_elem   = g_strdup_printf ("  </attribute>");

  g_string_append (xml, start_tag_elem);

  if (encoded)
    {
      value_tag_elem = g_strdup_printf ("    <value encoding=\"base64\">%s</value>\n", v_escaped);
    }
  else
    {
      value_tag_elem = g_strdup_printf ("    <value>%s</value>\n", v_escaped);
    }

  g_string_append (xml, value_tag_elem);

  if (private->has_structure)
    {
      struct_tag_elem = g_strdup_printf ("    <structure>%d</structure>\n", private->structure_type);
      g_string_append (xml, struct_tag_elem);
    }

  if (is_interpreted && utf)
    {
      i_escaped = gimp_attribute_escape_value (private->name, private->interpreted_value, &encoded);

      if (encoded)
        {
          interpreted_tag_elem = g_strdup_printf ("    <interpreted encoding=\"base64\">%s</interpreted>\n", i_escaped);
        }
      else
        {
          interpreted_tag_elem = g_strdup_printf ("    <interpreted>%s</interpreted>\n", i_escaped);
        }

      g_string_append (xml, interpreted_tag_elem);
    }

  g_string_append (xml, end_tag_elem);

  g_free (v_escaped);
  g_free (start_tag_elem);
  g_free (end_tag_elem);
  g_free (value_tag_elem);
  if (i_escaped)
    g_free (i_escaped);
  if (interpreted_tag_elem)
    g_free (interpreted_tag_elem);
  if (struct_tag_elem)
    g_free (interpreted_tag_elem);

  return g_string_free (xml, FALSE);
}

/**
 * gimp_attribute_get_value_type_from_string:
 *
 * @exiv_tag_type_string:    a @gchar
 *
 * converts a string representation tag type from gexiv2/exiv2
 * to a #GimpAttributeValueType
 *
 * Return value: #GimpAttributeValueType
 *
 * Since: 2.10
 */
GimpAttributeValueType
gimp_attribute_get_value_type_from_string (const gchar* string)
{
  GimpAttributeValueType type;
  gchar *lowchar;

  if (! string)
    return TYPE_INVALID;

  lowchar = g_ascii_strdown (string, -1);

  if (!g_strcmp0 (lowchar, "invalid"))
    type = TYPE_INVALID;
  else if (!g_strcmp0 (lowchar, "byte"))
    type = TYPE_BYTE;
  else if (!g_strcmp0 (lowchar, "ascii"))
    type = TYPE_ASCII;
  else if (!g_strcmp0 (lowchar, "short"))
    type = TYPE_SHORT;
  else if (!g_strcmp0 (lowchar, "long"))
    type = TYPE_LONG;
  else if (!g_strcmp0 (lowchar, "rational"))
    type = TYPE_RATIONAL;
  else if (!g_strcmp0 (lowchar, "sbyte"))
    type = TYPE_BYTE;
  else if (!g_strcmp0 (lowchar, "undefined"))
    type = TYPE_ASCII;
  else if (!g_strcmp0 (lowchar, "sshort"))
    type = TYPE_SSHORT;
  else if (!g_strcmp0 (lowchar, "slong"))
    type = TYPE_SLONG;
  else if (!g_strcmp0 (lowchar, "srational"))
    type = TYPE_SRATIONAL;
  else if (!g_strcmp0 (lowchar, "float"))
    type = TYPE_FLOAT;
  else if (!g_strcmp0 (lowchar, "double"))
    type = TYPE_DOUBLE;
  else if (!g_strcmp0 (lowchar, "ifd"))
    type = TYPE_ASCII;
  else if (!g_strcmp0 (lowchar, "string"))
    type = TYPE_UNICODE;
  else if (!g_strcmp0 (lowchar, "date"))
    type = TYPE_DATE;
  else if (!g_strcmp0 (lowchar, "time"))
    type = TYPE_TIME;
  else if (!g_strcmp0 (lowchar, "comment"))
    type = TYPE_UNICODE;
  else if (!g_strcmp0 (lowchar, "directory"))
    type = TYPE_UNICODE;
  else if (!g_strcmp0 (lowchar, "xmptext"))
    type = TYPE_ASCII;
  else if (!g_strcmp0 (lowchar, "xmpalt"))
    type = TYPE_MULTIPLE;
  else if (!g_strcmp0 (lowchar, "xmpbag"))
    type = TYPE_MULTIPLE;
  else if (!g_strcmp0 (lowchar, "xmpseq"))
    type = TYPE_MULTIPLE;
  else if (!g_strcmp0 (lowchar, "langalt"))
    type = TYPE_MULTIPLE;
  else
    type = TYPE_INVALID;

  g_free (lowchar);

  return type;
}

/**
 * gimp_attribute_is_valid:
 *
 * @attribute:    a @GimpAttribute
 *
 * checks, if @attribute is valid.
 * A @GimpAttribute is valid, if the @GimpAttributeValueType
 * has a valid entry.
 *
 * Return value: @gboolean: TRUE if valid, FALSE otherwise
 *
 * Since: 2.10
 */
gboolean
gimp_attribute_is_valid (GimpAttribute *attribute)
{
  GimpAttributePrivate *private;

  g_return_val_if_fail (attribute != NULL, FALSE);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  if (private->value_type == TYPE_INVALID)
    return FALSE;
  else
    return TRUE;
}

/*
 * internal functions
 */

/**
 * gimp_attribute_set_name:
 *
 * @attribute:    a #GimpAttribute
 * @value:        a constant #gchar
 *
 * sets the name of the #GimpAttribute object to
 * a @value
 *
 * @value can be freed after
 *
 * Since: 2.10
 */
static void
gimp_attribute_set_name (GimpAttribute* attribute, const gchar* value)
{
  gchar                **split_name;
  gchar                 *lowchar;
  GimpAttributePrivate  *private;

  g_return_if_fail (attribute != NULL);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  _g_free0 (private->name);

  private->name = g_strdup (value);

  private->name = string_replace_str (private->name, "Iptc4xmpExt", "iptcExt");
  private->name = string_replace_str (private->name, "Iptc4xmpCore", "iptc");

//  if (! g_strcmp0 (private->name, "Exif.Image.ResolutionUnit"))
//    {
//      g_print ("found: %s\n", private->name);
//    }

  split_name = g_strsplit (private->name, ".", 3);

  if (split_name [0])
    private->attribute_type = split_name [0];
  if (split_name [1])
    private->attribute_ifd = split_name [1];
  if (split_name [2])
    private->attribute_tag = split_name [2];

  attribute->priv->is_new_name_space = FALSE;

  lowchar = g_ascii_strdown (private->attribute_type, -1);

  if (!g_strcmp0 (lowchar, "exif"))
    {
      private->tag_type = TAG_EXIF;
    }
  else if (!g_strcmp0 (lowchar, "xmp"))
    {
      gint     j;
      gint     p1                 = 0;
      gint     p2                 = 0;
      gboolean is_known           = FALSE;
      gchar   *structure_tag_name = NULL;

      structure_tag_name = g_strdup (private->name);

      private->tag_type = TAG_XMP;

      while (p2 != -1)
        {
          p2 = string_index_of (structure_tag_name, "[", p1);

          if (p2 > -1)
            {
              gchar *struct_string = NULL;
              gint   struct_number;

              private->has_structure = TRUE;
              private->structure_type = STRUCTURE_TYPE_BAG; /* there's no way to get the real type from gexiv2 */

              struct_string = string_substring (structure_tag_name, 0, p2);
              private->attribute_structure = g_slist_prepend (private->attribute_structure, struct_string);
              structure_tag_name = gimp_attribute_get_structure_number (structure_tag_name, p2, &struct_number);

              p1 = p2 + 1;
            }
        }

      if (g_strcmp0 (private->name, structure_tag_name))
        private->sorted_name = structure_tag_name;
      else
        private->sorted_name = NULL;

      for (j = 0; j < G_N_ELEMENTS (xmp_namespaces); j++)
        {
          if (! g_strcmp0 (private->attribute_ifd, xmp_namespaces[j]))
            {
              is_known = TRUE;
              break;
            }
        }
      if (! is_known)
        {
          private->is_new_name_space = TRUE;
        }
    }
  else if (!g_strcmp0 (lowchar, "iptc"))
    {
      private->tag_type = TAG_IPTC;
    }
  else if (!g_strcmp0 (lowchar, "gimp"))
    {
      private->tag_type = TAG_GIMP;
    }
  else
    {
      private->tag_type = TAG_MISC;
    }

  g_free (lowchar);
}

/**
 * gimp_attribute_set_structure_type:
 *
 * @attribute:    a #GimpAttribute
 * @value:        a GimpAttributeStructureType
 *
 * Sets the structure type, Bag, Seq, None, etc.
 * The structure is not checked.
 * The structure type is only relevant for
 * GimpAttribute, that are part of a structure.
 *
 * Since : 2.10
 */
void
gimp_attribute_set_structure_type (GimpAttribute              *attribute,
                                   GimpAttributeStructureType  value)
{
  GimpAttributePrivate *private;

  g_return_if_fail (attribute != NULL);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  private->structure_type = value;
}

/**
 * gimp_attribute_get_structure_type:
 *
 * @attribute:    a #GimpAttribute
 *
 * The structure type, Bag, Seq, None, etc.
 * Return value: a #GimpAttributeStructureType
 *
 * Since : 2.10
 */
GimpAttributeStructureType
gimp_attribute_get_structure_type (GimpAttribute *attribute)
{
  GimpAttributePrivate *private;

  g_return_val_if_fail (attribute != NULL, STRUCTURE_TYPE_NONE);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  return private->structure_type;

}

/**
 * gimp_attribute_has_structure:
 *
 * @attribute:    a #GimpAttribute
 *
 * Return value:  TRUE, if @attribute is part
 * of a structure.
 *
 * Since : 2.10
 */
gboolean
gimp_attribute_has_structure (GimpAttribute *attribute)
{
  GimpAttributePrivate *private;

  g_return_val_if_fail (attribute != NULL, FALSE);

  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  return private->has_structure;
}

/**
 * gimp_attribute_escape_value:
 *
 * @name: a gchar *
 * @value: a gchar*
 * @encoded: a pointer to a gboolean
 *
 * converts @value into an escaped string for GMarkup.
 * If @value is not valid UTF-8, it is converted
 * into a base64 coded string and @encoded is set to TRUE,
 * otherwise FALSE
 *
 * Return value:  a new allocated @gchar *
 *
 * Since : 2.10
 */
static gchar *
gimp_attribute_escape_value (gchar    *name,
                             gchar    *value,
                             gboolean *encoded)
{

  if (!g_utf8_validate (value, -1, NULL))
    {
      gchar *enc_val = NULL;

      *encoded = TRUE;
       enc_val = g_base64_encode ((const guchar *) value,
                                   strlen (value) + 1);

       return enc_val;
    }
  *encoded = FALSE;

  return g_markup_escape_text (value, -1);
}

/**
 * gimp_attribute_copy:
 *
 * @attribute:  a #GimpAttribute
 *
 * duplicates a  #GimpAttribute object
 *
 * Return value:  a new @GimpAttribute or %NULL
 *
 * Since : 2.10
 */
GimpAttribute*
gimp_attribute_copy (GimpAttribute *attribute)
{
  GimpAttribute        *new_attribute     = NULL;
  GimpAttributePrivate *private;
  GimpAttributePrivate *new_private;

  g_return_val_if_fail (GIMP_IS_ATTRIBUTE (attribute), NULL);

  new_attribute = (GimpAttribute*) g_object_new (GIMP_TYPE_ATTRIBUTE, NULL);

  if (new_attribute)
    {
      private = GIMP_ATTRIBUTE_GET_PRIVATE(attribute);
      new_private = GIMP_ATTRIBUTE_GET_PRIVATE(new_attribute);
      if (private->name)
        new_private->name = g_strdup (private->name);

      if (private->tag_value)
        new_private->tag_value = g_strdup (private->tag_value);

      if (private->interpreted_value)
        new_private->interpreted_value = g_strdup (private->interpreted_value);

      if (private->exif_type)
        new_private->exif_type = g_strdup (private->exif_type);

      if (private->attribute_type)
        new_private->attribute_type = g_strdup (private->attribute_type);

      if (private->attribute_ifd)
        new_private->attribute_ifd = g_strdup (private->attribute_ifd);

      if (private->attribute_tag)
        new_private->attribute_tag = g_strdup (private->attribute_tag);

      new_private->is_new_name_space = private->is_new_name_space;
      new_private->value_type = private->value_type;
      new_private->tag_type = private->tag_type;

      return new_attribute;
    }

  return NULL;
}

/**
 * gimp_attribute_construct:
 *
 * @object_type:  a #GType
 * @name:         a constant #gchar that describes the name of the attribute
 *
 * constructs a new #GimpAttribute object
 *
 * Return value:  a new @GimpAttribute or %NULL if no @name is set
 *
 * Since : 2.10
 */
static GimpAttribute*
gimp_attribute_construct (GType object_type,
                          const gchar *name)
{
  GimpAttribute * attribute = NULL;

  g_return_val_if_fail (name != NULL, NULL);

  attribute = (GimpAttribute*) g_object_new (object_type, NULL);
  if (attribute)
    {
      gimp_attribute_set_name (attribute, name);
    }
  return attribute;
}

/**
 * gimp_attribute_class_init:
 *
 * class initializer
 */
static void
gimp_attribute_class_init (GimpAttributeClass * klass)
{
  gimp_attribute_parent_class = g_type_class_peek_parent (klass);
  g_type_class_add_private (klass, sizeof(GimpAttributePrivate));

  G_OBJECT_CLASS (klass)->get_property = gimp_attribute_get_property;
  G_OBJECT_CLASS (klass)->set_property = gimp_attribute_set_property;
  G_OBJECT_CLASS (klass)->finalize = gimp_attribute_finalize;
}

/**
 * gimp_attribute_instance_init:
 *
 * instance initializer
 */
static void
gimp_attribute_instance_init (GimpAttribute * attribute)
{
  GimpAttributePrivate *private;
  attribute->priv = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  private->name                    = NULL;
  private->sorted_name             = NULL;
  private->tag_value               = NULL;
  private->interpreted_value       = NULL;
  private->exif_type               = NULL;
  private->value_type              = TYPE_INVALID;
  private->attribute_type          = NULL;
  private->attribute_ifd           = NULL;
  private->attribute_tag           = NULL;
  private->attribute_structure     = NULL;
  private->is_new_name_space       = FALSE;
  private->has_structure           = FALSE;
  private->structure_type          = STRUCTURE_TYPE_NONE;
}

/**
 * gimp_attribute_finalize:
 *
 * instance finalizer
 */
static void
gimp_attribute_finalize (GObject* obj)
{
  GimpAttribute        *attribute;
  GimpAttributePrivate *private;

  g_return_if_fail (GIMP_IS_ATTRIBUTE (obj));

  attribute = GIMP_ATTRIBUTE (obj);
  private = GIMP_ATTRIBUTE_GET_PRIVATE (attribute);

  counter++;

  if(private->name)
    _g_free0 (private->name);

  if(private->sorted_name)
    _g_free0 (private->sorted_name);

  if(private->tag_value)
    _g_free0 (private->tag_value);

  if(private->interpreted_value)
    _g_free0 (private->interpreted_value);

  if(private->exif_type)
    _g_free0 (private->exif_type);

  if(private->attribute_type)
    _g_free0 (private->attribute_type);

  if(private->attribute_ifd)
    _g_free0 (private->attribute_ifd);

  if(private->attribute_tag)
    _g_free0 (private->attribute_tag);

  if(private->attribute_structure)
    g_slist_free_full (private->attribute_structure, g_free);

  G_OBJECT_CLASS (gimp_attribute_parent_class)->finalize (obj);
  obj = NULL;
}

/**
 * gimp_attribute_get_property
 *
 * instance get property
 */
static void
gimp_attribute_get_property (GObject * object,
                             guint property_id,
                             GValue * value,
                             GParamSpec * pspec)
{
}

/**
 * gimp_attribute_set_property
 *
 * instance set property
 */
static void
gimp_attribute_set_property (GObject * object,
                             guint property_id,
                             const GValue * value,
                             GParamSpec * pspec)
{
}

static gchar *
gimp_attribute_get_structure_number (gchar *cptag, gint start, gint *number)
{
  gint p1;
  gchar *tag;
  gchar *new_tag;
  gchar *oldnr;
  gchar *newnr;

  start++;

  tag = g_strdup (cptag);

  p1 = string_index_of (tag, "]", start);

  if (p1 > -1)
    {
      gchar *number_string = NULL;
      gint   len;

      len = p1-start;

      number_string = string_substring (tag, start, len);

      *number = atoi (number_string);

      oldnr = g_strdup_printf ("[%d]", *number);
      newnr = g_strdup_printf ("[%06d]", *number);
      new_tag = string_replace_str (tag, oldnr, newnr);

      g_free (oldnr);
      g_free (newnr);
      g_free (tag);
      g_free (number_string);

      return new_tag;
    }
  else
    {
      *number = -1;
      return NULL;
    }

}

/**
 * gimp_attribute_get_type
 *
 * Return value: #GimpAttribute type
 */
GType
gimp_attribute_get_type (void)
{
  static volatile gsize gimp_attribute_type_id__volatile = 0;
  if (g_once_init_enter(&gimp_attribute_type_id__volatile))
    {
      static const GTypeInfo g_define_type_info =
            { sizeof(GimpAttributeClass),
                (GBaseInitFunc) NULL,
                (GBaseFinalizeFunc) NULL,
                (GClassInitFunc) gimp_attribute_class_init,
                (GClassFinalizeFunc) NULL,
                NULL,
                sizeof(GimpAttribute),
                0,
                (GInstanceInitFunc) gimp_attribute_instance_init,
                NULL
            };

      GType gimp_attribute_type_id;
      gimp_attribute_type_id = g_type_register_static (G_TYPE_OBJECT,
                                                       "GimpAttribute",
                                                       &g_define_type_info,
                                                       0);

      g_once_init_leave(&gimp_attribute_type_id__volatile,
                        gimp_attribute_type_id);
    }
  return gimp_attribute_type_id__volatile;
}


/*
 * Helper functions
 */

/**
 * get_tag_time:
 *
 * @input:        a #gchar array
 * @tm:           a pointer to a #TagTime struct
 *
 * converts a ISO 8601 IPTC Time to a TagTime structure
 * Input is:
 *  HHMMSS
 *  HHMMSS:HHMM
 *
 * Return value:  #gboolean for success
 *
 * Since: 2.10
 */
static gboolean
get_tag_time (gchar *input, TagTime *tm)
{
  long     val;
  GString *string = NULL;
  gchar   *tmpdate;
  gboolean flong;

  g_return_val_if_fail (input != NULL, FALSE);

  input = g_strstrip (input);

  if (*input == '\0' || !g_ascii_isdigit (*input))
    return FALSE;

  string = g_string_new (NULL);

  val = strtoul (input, (char **)&input, 10);

  if (val < 25)  /* HH:MM:SS+/-HH:MM */
    {
      flong = FALSE;
      g_string_append_printf (string, "%02ld", val);
    }
  else
    {
      flong = TRUE;
      g_string_append_printf (string, "%06ld", val);
    }


  if (! flong) /* exactly 2 times */
    {
      input++;
      val = strtoul (input, (char **)&input, 10);
      g_string_append_printf (string, "%02ld", val);
      input++;
      val = strtoul (input, (char **)&input, 10);
      g_string_append_printf (string, "%02ld", val);
    }

  tmpdate = g_string_free (string, FALSE);

  val = strtoul (tmpdate, (char **)&tmpdate, 10);

  /* hhmmss */

  tm->tag_time_sec = val % 100;
  tm->tag_time_min = (val % 10000) / 100;
  tm->tag_time_hour = val / 10000;

  string = g_string_new (NULL);

  if (flong) /* :HHSS for time zone */
    {
      val = 0L;

      if (*input == ':')
        {
          input++;
          val = strtoul (input, (char **)&input, 10);
        }
      g_string_append_printf (string, "%05ld", val);
    }
  else
    {
      val = 0L;

      if (*input == '+' || *input == '-') /* +/- HH:MM for time zone */
        {
          val = strtoul (input, (char **)&input, 10);
          g_string_append_printf (string, "%02ld", val);
          input++;
          val = strtoul (input, (char **)&input, 10);
          g_string_append_printf (string, "%02ld", val);
        }
      else
        {
          g_string_append_printf (string, "%04ld", val);
        }
    }

  tmpdate = g_string_free (string, FALSE);

  val = strtoul (tmpdate, (char **)&tmpdate, 10);

  tm->tag_tz_min = (val % 100) / 100;
  tm->tag_tz_hour = val / 100;

  return *input == '\0';
}

/**
 * get_tag_date:
 *
 * @input:        a #gchar array
 * @dt:           a pointer to a #TagDate struct
 *
 * converts a ISO 8601 IPTC Date to a TagDate structure
 * Input is:
 * CCYYMMDD
 *
 * Return value:  #gboolean for success
 *
 * Since: 2.10
 */
static gboolean
get_tag_date (gchar *input, TagDate *dt)
{
  long     val;
  GString *string = NULL;
  gchar   *tmpdate;
  gboolean eod;

  g_return_val_if_fail (input != NULL, FALSE);

  input = g_strstrip (input);

  if (*input == '\0' || !g_ascii_isdigit (*input))
    return FALSE;

  string = g_string_new (NULL);
  eod = FALSE;

  while (! eod)
    {
      val = strtoul (input, (char **)&input, 10);

      if (*input == '-' ||
          *input == ':')
        {
          input++;
        }
      else
        {
          eod = TRUE;
        }

      if (val < 10)
        g_string_append_printf (string, "%02ld", val);
      else
        g_string_append_printf (string, "%ld", val);
    }
  tmpdate = g_string_free (string, FALSE);

  val = strtoul (tmpdate, (char **)&tmpdate, 10);

  /* YYYYMMDD */
  dt->tag_day = val % 100;
  dt->tag_month = (val % 10000) / 100;
  dt->tag_year = val / 10000;

  return *input == '\0';
}

/**
 * string_replace_str:
 *
 * @original:          the original string
 * @old_pattern:       the pattern to replace
 * @replacement:       the replacement
 *
 * replaces @old_pattern by @replacement in @original
 * This routine is copied from VALA.
 *
 * Return value: the new string.
 *
 * Since: 2.10
 */
static gchar*
string_replace_str (const gchar* original, const gchar* old_pattern, const gchar* replacement) {
  gchar      *result        = NULL;
  GError     *_inner_error_ = NULL;

  g_return_val_if_fail (original != NULL, NULL);
  g_return_val_if_fail (old_pattern != NULL, NULL);
  g_return_val_if_fail (replacement != NULL, NULL);

  {
    GRegex           *regex  = NULL;
    const gchar      *_tmp0_ = NULL;
    gchar            *_tmp1_ = NULL;
    gchar            *_tmp2_ = NULL;
    GRegex           *_tmp3_ = NULL;
    GRegex           *_tmp4_ = NULL;
    gchar            *_tmp5_ = NULL;
    GRegex           *_tmp6_ = NULL;
    const gchar      *_tmp7_ = NULL;
    gchar            *_tmp8_ = NULL;
    gchar            *_tmp9_ = NULL;

    _tmp0_ = old_pattern;
    _tmp1_ = g_regex_escape_string (_tmp0_, -1);
    _tmp2_ = _tmp1_;
    _tmp3_ = g_regex_new (_tmp2_, 0, 0, &_inner_error_);
    _tmp4_ = _tmp3_;
    _g_free0 (_tmp2_);
    regex = _tmp4_;

    if (_inner_error_ != NULL)
      {
        if (_inner_error_->domain == G_REGEX_ERROR)
          {
            goto __catch0_g_regex_error;
          }
        g_critical ("file %s: line %d: unexpected error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
        g_clear_error (&_inner_error_);
        return NULL;
      }

    _tmp6_ = regex;
    _tmp7_ = replacement;
    _tmp8_ = g_regex_replace_literal (_tmp6_, original, (gssize) (-1), 0, _tmp7_, 0, &_inner_error_);
    _tmp5_ = _tmp8_;

    if (_inner_error_ != NULL)
      {
        _g_regex_unref0 (regex);
        if (_inner_error_->domain == G_REGEX_ERROR)
          {
            goto __catch0_g_regex_error;
          }
        _g_regex_unref0 (regex);
        g_critical ("file %s: line %d: unexpected error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
        g_clear_error (&_inner_error_);
        return NULL;
      }

    _tmp9_ = _tmp5_;
    _tmp5_ = NULL;
    result = _tmp9_;
    _g_free0 (_tmp5_);
    _g_regex_unref0 (regex);
    return result;
  }

  goto __finally0;
  __catch0_g_regex_error:
  {
    GError* e = NULL;
    e = _inner_error_;
    _inner_error_ = NULL;
    g_assert_not_reached ();
    _g_error_free0 (e);
  }

  __finally0:
  if (_inner_error_ != NULL)
    {
      g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
      g_clear_error (&_inner_error_);
      return NULL;
    }
}

/**
 * string_index_of:
 *
 * @haystack:          a #gchar
 * @needle:            a #gchar
 * @start_index:       a #gint
 *
 *
 * Return value:  #gint that points to the position
 * of @needle in @haystack, starting at @start_index,
 * or -1, if @needle is not found
 *
 * Since: 2.10
 */
static gint
string_index_of (gchar* haystack, gchar* needle, gint start_index)
{
  gint result = 0;
  gchar* temp1_ = NULL;
  g_return_val_if_fail (haystack != NULL, -1);
  g_return_val_if_fail (needle != NULL, -1);
  temp1_ = strstr (((gchar*) haystack) + start_index, needle);
  if (temp1_ != NULL)
    {
      result = (gint) (temp1_ - ((gchar*) haystack));
      return result;
    }
  else
    {
      result = -1;
      return result;
    }
}

/**
 * string_strnlen:
 *
 * @str:          a #gchar
 * @maxlen:       a #glong
 *
 * Returns the length of @str or @maxlen, if @str
 * is longer that @maxlen
 *
 * Return value:  #glong
 *
 * Since: 2.10
 */
static glong
string_strnlen (gchar* str, glong maxlen)
{
  glong result = 0L;
  gchar* temp1_ = NULL;
  temp1_ = memchr (str, 0, (gsize) maxlen);
  if (temp1_ == NULL)
    {
      return maxlen;
    }
  else
    {
      result = (glong) (temp1_ - str);
      return result;
    }
}

/**
 * string_substring:
 *
 * @string:          a #gchar
 * @offset:          a #glong
 * @len:             a #glong
 *
 * Returns a substring of @string, starting at @offset
 * with a legth of @len
 *
 * Return value:  #gchar to be freed if no longer use
 *
 * Since: 2.10
 */
static gchar*
string_substring (gchar* string, glong offset, glong len)
{
  gchar* result = NULL;
  glong string_length = 0L;
  gboolean _tmp0_ = FALSE;
  g_return_val_if_fail(string != NULL, NULL);
  if (offset >= ((glong) 0))
    {
      _tmp0_ = len >= ((glong) 0);
    }
  else
    {
      _tmp0_ = FALSE;
    }

  if (_tmp0_)
    {
      string_length = string_strnlen ((gchar*) string, offset + len);
    }
  else
    {
      string_length = (glong) strlen (string);
    }

  if (offset < ((glong) 0))
    {
      offset = string_length + offset;
      g_return_val_if_fail (offset >= ((glong ) 0), NULL);
    }
  else
    {
      g_return_val_if_fail (offset <= string_length, NULL);
    }
  if (len < ((glong) 0))
    {
      len = string_length - offset;
    }

  g_return_val_if_fail ((offset + len) <= string_length, NULL);
  result = g_strndup (((gchar*) string) + offset, (gsize) len);
  return result;
}
