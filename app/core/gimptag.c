
#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "gimptag.h"

GimpTag
gimp_tag_from_string (const gchar      *string)
{
  if (g_utf8_strchr (string, -1, ','))
    {
      return 0;
    }

  return g_quark_from_string (string);
}

int
gimp_tag_compare_func (const void         *p1,
                       const void         *p2)
{
  return strcmp (g_quark_to_string (GPOINTER_TO_UINT (p1)),
                 g_quark_to_string (GPOINTER_TO_UINT (p2)));
}

