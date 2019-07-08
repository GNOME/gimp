/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpRc deserialization routines
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "config-types.h"

#include "gimprc-deserialize.h"
#include "gimprc-unknown.h"

#include "gimp-intl.h"


static GTokenType gimp_rc_deserialize_unknown (GimpConfig *config,
                                               GScanner   *scanner);


gboolean
gimp_rc_deserialize (GimpConfig *config,
                     GScanner   *scanner,
                     gint        nest_level,
                     gpointer    data)
{
  GObjectClass  *klass;
  GParamSpec   **property_specs;
  guint          n_property_specs;
  guint          i;
  guint          scope_id;
  guint          old_scope_id;
  GTokenType     token;
  GTokenType     next;

  g_return_val_if_fail (GIMP_IS_CONFIG (config), FALSE);

  klass = G_OBJECT_GET_CLASS (config);

  property_specs = g_object_class_list_properties (klass, &n_property_specs);
  if (! property_specs)
    return TRUE;

  scope_id = g_type_qname (G_TYPE_FROM_INSTANCE (config));
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  for (i = 0; i < n_property_specs; i++)
    {
      GParamSpec *prop_spec = property_specs[i];

      if (prop_spec->flags & GIMP_CONFIG_PARAM_SERIALIZE)
        {
          g_scanner_scope_add_symbol (scanner, scope_id,
                                      prop_spec->name, prop_spec);
        }
    }

  g_free (property_specs);

  g_object_freeze_notify (G_OBJECT (config));

  token = G_TOKEN_LEFT_PAREN;

  while (TRUE)
    {
      next = g_scanner_peek_next_token (scanner);

      if (G_UNLIKELY (next != token && ! (token == G_TOKEN_SYMBOL &&
                                          next  == G_TOKEN_IDENTIFIER)))
        {
          break;
        }

      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_IDENTIFIER:
          token = gimp_rc_deserialize_unknown (config, scanner);
          break;

        case G_TOKEN_SYMBOL:
          token = gimp_config_deserialize_property (config,
                                                    scanner, nest_level);
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  g_scanner_set_scope (scanner, old_scope_id);

  g_object_thaw_notify (G_OBJECT (config));

  if (token == G_TOKEN_NONE)
    return FALSE;

  /* If the unknown token value couldn't be parsed the default error
     message is rather confusing.  We try to produce something more
     meaningful here ...
   */
  if (token == G_TOKEN_STRING && next == G_TOKEN_IDENTIFIER)
    {
      g_scanner_unexp_token (scanner, G_TOKEN_SYMBOL, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
      return FALSE;
    }

  return gimp_config_deserialize_return (scanner, token, nest_level);
}

static GTokenType
gimp_rc_deserialize_unknown (GimpConfig *config,
                             GScanner   *scanner)
{
  gchar *key;
  guint  old_scope_id;

  old_scope_id = g_scanner_set_scope (scanner, 0);

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;

  key = g_strdup (scanner->value.v_identifier);

  g_scanner_get_next_token (scanner);

  g_scanner_set_scope (scanner, old_scope_id);

  if (! g_utf8_validate (scanner->value.v_string, -1, NULL))
    {
      g_scanner_error (scanner,
                       _("value for token %s is not a valid UTF-8 string"),
                       key);
      g_free (key);
      return G_TOKEN_NONE;
    }

  gimp_rc_add_unknown_token (config, key, scanner->value.v_string);
  g_free (key);

  return G_TOKEN_RIGHT_PAREN;
}
