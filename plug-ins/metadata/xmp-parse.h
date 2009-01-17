/* xmp-parse.h - simple parser for XMP metadata
 *
 * Copyright (C) 2004, Raphaël Quinet <raphael@gimp.org>
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

#ifndef XMP_PARSE_H
#define XMP_PARSE_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  XMP_ERROR_NO_XPACKET,
  XMP_ERROR_BAD_ENCODING,
  XMP_ERROR_PARSE,
  XMP_ERROR_MISSING_ABOUT,
  XMP_ERROR_UNKNOWN_ELEMENT,
  XMP_ERROR_UNKNOWN_ATTRIBUTE,
  XMP_ERROR_UNEXPECTED_ELEMENT,
  XMP_ERROR_INVALID_CONTENT,
  XMP_ERROR_INVALID_COMMENT
} XMPParseError;

#define XMP_PARSE_ERROR xmp_parse_error_quark ()

GQuark xmp_parse_error_quark (void);

typedef enum
{
  XMP_FLAG_FIND_XPACKET           = 1 << 0, /* allow text before <?xpacket */
  XMP_FLAG_NO_COMMENTS            = 1 << 1, /* no XML comments allowed */
  XMP_FLAG_NO_UNKNOWN_ELEMENTS    = 1 << 2, /* no unknown XML elements */
  XMP_FLAG_NO_UNKNOWN_ATTRIBUTES  = 1 << 3, /* no unknown XML attributes */
  XMP_FLAG_NO_MISSING_ABOUT       = 1 << 4, /* schemas must have rdf:about */
  XMP_FLAG_DEFER_VALUE_FREE       = 1 << 5  /* prop. value[] freed by caller */
} XMPParseFlags;

typedef enum
{
  XMP_PTYPE_TEXT,           /* value in value[0] */
  XMP_PTYPE_RESOURCE,       /* value in value[0] */
  XMP_PTYPE_ORDERED_LIST,   /* values in value[0..n] */
  XMP_PTYPE_UNORDERED_LIST, /* values in value[0..n] */
  XMP_PTYPE_ALT_THUMBS,     /* values in value[0..n] */
  XMP_PTYPE_ALT_LANG,  /* lang in value[0,2..n*2], text in value[1,3..n*2+1] */
  XMP_PTYPE_STRUCTURE, /* ns prefix in name[0], ns uri in name[1], */
                    /* name in value[2,4..n*2+2], value in value[3,5..n*2+3] */
  XMP_PTYPE_UNKNOWN
} XMPParseType;

typedef struct _XMPParseContext XMPParseContext;
typedef struct _XMPParser XMPParser;

struct _XMPParser
{
  /* Called whenever the parser sees a new namespace (usually an XMP
   * schema) except for the basic RDF and XMP namespaces.  The value
   * returned by this callback will be passed as "ns_user_data" to the
   * callbacks end_schema and set_property.  It is allowed to return
   * the pointers ns_uri or ns_prefix because they will remain valid
   * at least until end_schema is called.
   */
  gpointer    (*start_schema)   (XMPParseContext     *context,
                                 const gchar         *ns_uri,
                                 const gchar         *ns_prefix,
                                 gpointer             user_data,
                                 GError             **error);

  /* Called when a namespace goes out of scope.  The ns_user_data will
   * be the one that was returned by start_schema for the
   * corresponding schema.
   */
  void        (*end_schema)     (XMPParseContext     *context,
                                 gpointer             ns_user_data,
                                 gpointer             user_data,
                                 GError             **error);

  /* Called for each property that is defined in the XMP packet.  The
   * way the value of the property is returned depends on its type.
   * See the definition of XMPParseType for details.
   */
  void        (*set_property)   (XMPParseContext     *context,
                                 const gchar         *name,
                                 XMPParseType         type,
                                 const gchar        **value,
                                 gpointer             ns_user_data,
                                 gpointer             user_data,
                                 GError             **error);

  /* Called on error, including one set by other methods in the
   * vtable.  The GError should not be freed.
   */
  void        (*error)          (XMPParseContext     *context,
                                 GError              *error,
                                 gpointer             user_data);
};

XMPParseContext * xmp_parse_context_new   (const XMPParser *parser,
                                           XMPParseFlags    flags,
                                           gpointer         user_data,
                                           GDestroyNotify   user_data_dnotify);

void              xmp_parse_context_free  (XMPParseContext *context);

gboolean          xmp_parse_context_parse (XMPParseContext *context,
                                           const gchar     *text,
                                           gssize           text_len,
                                           GError         **error);

gboolean          xmp_parse_context_end_parse (XMPParseContext  *context,
                                               GError          **error);
G_END_DECLS

#endif /* XMP_PARSE_H */
