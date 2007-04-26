#include <stdio.h>
/* xmp-parse.c - simple parser for XMP metadata
 *
 * Copyright (C) 2004-2007, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* This code implements a simple parser for XMP metadata.  Its API is
 * based on the one provided by GMarkupParser (part of Glib).
 *
 * This is not a full RDF parser: it shares some of the limitations
 * inherited from glib (UTF-8 only, no special entities) and supports
 * RDF only to the extent needed for XMP.  XMP defines several
 * "schemas" containing a list of "properties".  Each property in a
 * schema has one value, which can be a simple type (e.g., integer or
 * text) or a structured type (rdf:Alt, rdf:Bag, rdf:Seq).  As there
 * is no need to support a much deeper nesting of elements, this
 * parser does not try to maintain an arbitrarily large stack of
 * elements.  Also, it does not support RDF features that are
 * forbidden by the XMP specs, such as rdf:parseType="Litteral".
 *
 * The design goals for this parser are: support all RDF features
 * needed for XMP (at least the features explicitely described in the
 * XMP spec), be tolerant in case unknown elements or attributes are
 * found, be as simple as possible, avoid building a DOM tree.
 *
 * TODO:
 * - support UCS-2 and UCS-4 besides UTF-8 (copy and convert the data)
 * - write a decent scanner for finding <?xpacket...?> as recommended
 *   in the XMP specification (including support for UCS-2 and UCS-4)
 * - provide an API for passing unknown elements or tags to the caller
 * - think about re-writing this using a better XML parser (expat?)
 *   instead of the GMarkup parser
 */

#ifndef WITHOUT_GIMP
#  include "config.h"
#  include <string.h>
#  include "libgimp/stdplugins-intl.h"
#else
#  include <string.h>
#  define _(String) (String)
#  define N_(String) (String)
#endif
#include "base64.h"
#include "xmp-parse.h"

GQuark
xmp_parse_error_quark (void)
{
  static GQuark error_quark = 0;

  if (error_quark == 0)
    error_quark = g_quark_from_static_string ("xmp-parse-error-quark");

  return error_quark;
}

/* The current version of XMP (January 2004) is relatively simple in
 * that only a few elements (<rdf:Description>, <rdf:Alt>, <rdf:Bag>,
 * <rdf:Seq>) may include other elements and no deep nesting is
 * allowed.  As a result, it is possible to include all allowed
 * combinations directly into the state instead of having to maintain
 * a separate stack besides the simple state enum.  There is only a
 * 1-element stack (saved_state) used for some special cases such as
 * when skipping unknown tags or parsing a property with qualifiers.
 *
 * Here is a quick overview of the structure of an XMP document and
 * the corresponding state after reading each element.  Depending on
 * the contents of each property, we can have the following cases
 * that are summarized as STATE_INSIDE... below:
 * - structured types can use any valid combination of the states
 *   between STATE_INSIDE_QDESC and STATE_INSIDE_SEQ_LI_RSC;
 * - simple property types contain some text and no other element so
 *   the only state will be STATE_INSIDE_PROPERTY while reading that
 *   text;
 * - if the shorthand notation is used for some simple properties,
 *   then they will be written as attributes of a top level
 *   rdf:Description instead of being a separate element, so the
 *   state will not go deeper than STATE_INSIDE_TOPLEVEL_DESC.
 *
 * (init)                                 STATE_START
 * <?xpacket begin='' id='...'?>           STATE_INSIDE_XPACKET
 *  <x:xmpmeta xmlns:x='adobe:ns:meta/'>    STATE_INSIDE_XMPMETA
 *   <rdf:RDF xmlns:rdf='...'>               STATE_INSIDE_RDF
 *    <rdf:Description rdf:about='' ...>      STATE_INSIDE_TOPLEVEL_DESC
 *     <foo:bar>                               STATE_INSIDE_PROPERTY
 *       ... (simple or structured property)    STATE_INSIDE...
 *     </foo:bar>                             STATE_INSIDE_TOPLEVEL_DESC
 *     <foo:baz>                               STATE_INSIDE_PROPERTY
 *       ... (simple or structured property)    STATE_INSIDE...
 *     </foo:baz>                             STATE_INSIDE_TOPLEVEL_DESC
 *    </rdf:Description>                     STATE_INSIDE_RDF
 *    <rdf:Description ...>                   STATE_INSIDE_TOPLEVEL_DESC
 *     ... (some properties)                   STATE_INSIDE_PROPERTY
 *    </rdf:Description>                     STATE_INSIDE_RDF
 *    ...
 *   </rdf:RDF>                             STATE_AFTER_RDF
 *  </x:xmpmeta>                           STATE_AFTER_XMPMETA
 * <?xpacket end='r'?>                    STATE_AFTER_XPACKET
 *
 * Note: The abbreviation QDESC is used for the properties with
 * qualifiers (when <rdf:Description> is used deeper than at the top
 * level inside <rdf:RDF>).  In that case, QDESC_VALUE contains the
 * value of the property and QDESC_QUAL is used for each of the
 * optional qualifiers (which are currently ignored).
 */
typedef enum
{
  STATE_START,
  STATE_INSIDE_XPACKET,
  STATE_INSIDE_XMPMETA,
  STATE_INSIDE_RDF,
  STATE_INSIDE_TOPLEVEL_DESC,
  STATE_INSIDE_PROPERTY,
  STATE_INSIDE_QDESC,
  STATE_INSIDE_QDESC_VALUE,
  STATE_INSIDE_QDESC_QUAL,
  STATE_INSIDE_STRUCT_ADD_NS,
  STATE_INSIDE_STRUCT,
  STATE_INSIDE_STRUCT_ELEMENT,
  STATE_INSIDE_ALT,
  STATE_INSIDE_ALT_LI,
  STATE_INSIDE_ALT_LI_RSC,
  STATE_INSIDE_ALT_LI_RSC_IMG,
  STATE_INSIDE_BAG,
  STATE_INSIDE_BAG_LI,
  STATE_INSIDE_BAG_LI_RSC,
  STATE_INSIDE_SEQ,
  STATE_INSIDE_SEQ_LI,
  STATE_INSIDE_SEQ_LI_RSC,
  STATE_AFTER_RDF,
  STATE_AFTER_XMPMETA,
  STATE_AFTER_XPACKET,
  STATE_SKIPPING_UNKNOWN_ELEMENTS,
  STATE_SKIPPING_IGNORED_ELEMENTS,
  STATE_ERROR
} XMPParseState;

typedef struct
{
  gint         depth;
  gchar       *uri;
  gchar       *prefix;
  gint         prefix_len;
  gpointer     ns_user_data;
} XMLNameSpace;

struct _XMPParseContext
{
  const XMPParser *parser;
  XMPParseFlags    flags;

  gpointer         user_data;
  GDestroyNotify   user_data_dnotify;

  XMPParseState    state;
  gint             depth;

  GSList          *namespaces;
  gchar           *xmp_prefix;
  guint            xmp_prefix_len;
  gchar           *rdf_prefix;
  guint            rdf_prefix_len;

  gchar           *property;
  XMLNameSpace    *property_ns;
  XMPParseType     property_type;
  gchar          **prop_value;
  gint             prop_cur_value;
  gint             prop_max_value;
  gboolean         prop_missing_value;
  /* used when skipping tags, or inside a struct or property qualifier */
  XMPParseState    saved_state;
  gint             saved_depth;

  GMarkupParseContext *markup_context;
};

#ifdef DEBUG_XMP_PARSER
static const char *state_names[] =
{
  "START",
  "INSIDE_XPACKET",
  "INSIDE_XMPMETA",
  "INSIDE_RDF",
  "INSIDE_TOPLEVEL_DESC",
  "INSIDE_PROPERTY",
  "INSIDE_QDESC",
  "INSIDE_QDESC_VALUE",
  "INSIDE_QDESC_QUAL",
  "INSIDE_STRUCT_ADD_NS",
  "INSIDE_STRUCT",
  "INSIDE_STRUCT_ELEMENT",
  "INSIDE_ALT",
  "INSIDE_ALT_LI",
  "INSIDE_ALT_LI_RSC",
  "INSIDE_ALT_LI_RSC_IMG",
  "INSIDE_BAG",
  "INSIDE_BAG_LI",
  "INSIDE_BAG_LI_RSC",
  "INSIDE_SEQ",
  "INSIDE_SEQ_LI",
  "INSIDE_SEQ_LI_RSC",
  "AFTER_RDF",
  "AFTER_XMPMETA",
  "AFTER_XPACKET",
  "SKIPPING_UNKNOWN_ELEMENTS",
  "SKIPPING_IGNORED_ELEMENTS",
  "ERROR",
};
#endif

/* report an error and propagate it */
static void
parse_error (XMPParseContext  *context,
             GError          **error,
             XMPParseError     code,
             const gchar      *format,
             ...)
{
  GError  *tmp_error;

  if (code == XMP_ERROR_NO_XPACKET)
    tmp_error = g_error_new (XMP_PARSE_ERROR, code,
                             _("Error: No XMP packet found"));
  else
    {
      gchar   *s;
      va_list  args;
      gint     line_number;
      gint     char_number;

      va_start (args, format);
      s = g_strdup_vprintf (format, args);
      va_end (args);
      g_markup_parse_context_get_position (context->markup_context,
                                           &line_number,
                                           &char_number);
      tmp_error = g_error_new (XMP_PARSE_ERROR, code,
                               _("Error on line %d char %d: %s"),
                               line_number, char_number, s);
      g_free (s);
    }

  context->state = STATE_ERROR;
  if (context->parser->error)
    (*context->parser->error) (context, tmp_error, context->user_data);

  g_propagate_error (error, tmp_error);
}

/* report an error if an unexpected element is found in the wrong context */
static void
parse_error_element (XMPParseContext  *context,
                     GError          **error,
                     const gchar      *expected_element,
                     gboolean          optional,
                     const gchar      *found_element)
{
  if (optional == TRUE)
    parse_error (context, error, XMP_ERROR_UNEXPECTED_ELEMENT,
                 _("Expected text or optional element <%s>, found <%s> instead"),
                 expected_element, found_element);
  else
    parse_error (context, error, XMP_ERROR_UNEXPECTED_ELEMENT,
                 _("Expected element <%s>, found <%s> instead"),
                 expected_element, found_element);
}

/* skip an unknown element (unknown property) and its contents */
static void
unknown_element (XMPParseContext  *context,
                 GError          **error,
                 const gchar      *element_name)
{
#ifdef DEBUG_XMP_PARSER
  g_print ("XMP: SKIPPING %s\n", element_name);
#endif
  if (context->flags & XMP_FLAG_NO_UNKNOWN_ELEMENTS)
    parse_error (context, error, XMP_ERROR_UNKNOWN_ELEMENT,
                 _("Unknown element <%s>"),
                 element_name);
  else
    {
      context->saved_depth = context->depth;
      context->saved_state = context->state;
      context->state = STATE_SKIPPING_UNKNOWN_ELEMENTS;
    }
}

/* skip and element and all other elements that it may contain */
static void
ignore_element (XMPParseContext *context)
{
  context->saved_depth = context->depth;
  context->saved_state = context->state;
  context->state = STATE_SKIPPING_IGNORED_ELEMENTS;
}

/* skip an unknown attribute (or abort if flags forbid unknown attributes) */
static void
unknown_attribute (XMPParseContext  *context,
                   GError          **error,
                   const gchar      *element_name,
                   const gchar      *attribute_name,
                   const gchar      *attribute_value)
{
  if (context->flags & XMP_FLAG_NO_UNKNOWN_ATTRIBUTES)
    parse_error (context, error, XMP_ERROR_UNKNOWN_ATTRIBUTE,
                 _("Unknown attribute \"%s\"=\"%s\" in element <%s>"),
                 attribute_name, attribute_value, element_name);
#ifdef DEBUG_XMP_PARSER
  g_print ("skipping unknown attribute \"%s\"=\"%s\" in element <%s>\n",
           attribute_name, attribute_value, element_name);
#endif
}

static gboolean
is_whitespace_string (const gchar *string)
{
  const gchar *p;

  if (string == NULL)
    return TRUE;
  /* XML accepts only 4 ASCII chars as whitespace and no other UNICODE chars */
  for (p = string; *p; ++p)
    if (*p != ' ' && *p != '\t' && *p != '\r' && *p != '\n')
      return FALSE;
  return TRUE;
}

/* new namespace/schema seen - add it to the list of namespaces */
static void
push_namespace (XMPParseContext  *context,
                const gchar      *uri,
                const gchar      *prefix,
                GError          **error)
{
  XMLNameSpace *ns;

  if (! strcmp (uri, "adobe:ns:meta/"))
    {
      g_free (context->xmp_prefix);
      context->xmp_prefix = g_strdup (prefix); /* XMP recommends "x:" */
      context->xmp_prefix_len = strlen (prefix);
      return;
    }
  if (! strcmp (uri, "http://www.w3.org/1999/02/22-rdf-syntax-ns#"))
    {
      g_free (context->rdf_prefix);
      context->rdf_prefix = g_strdup (prefix); /* XMP recommends "rdf:" */
      context->rdf_prefix_len = strlen (prefix);
      return;
    }
  ns = g_new (XMLNameSpace, 1);
  ns->depth = context->depth;
  ns->uri = g_strdup (uri);
  ns->prefix = g_strdup (prefix);
  ns->prefix_len = strlen (prefix);
  context->namespaces = g_slist_prepend (context->namespaces, ns);
  if (context->parser->start_schema)
    ns->ns_user_data = (*context->parser->start_schema) (context,
                                                         ns->uri,
                                                         ns->prefix,
                                                         context->user_data,
                                                         error);
  else
    ns->ns_user_data = NULL;
}

/* free all namespaces that are deeper than the current element depth */
static void
pop_namespaces (XMPParseContext  *context,
                GError          **error)
{
  XMLNameSpace *ns;

  if (context->namespaces == NULL)
    return;
  ns = context->namespaces->data;
  while (ns->depth >= context->depth)
    {
      if (context->parser->end_schema)
        (*context->parser->end_schema) (context,
                                        ns->ns_user_data,
                                        context->user_data,
                                        error);
      g_free (ns->uri);
      g_free (ns->prefix);
      context->namespaces = g_slist_delete_link (context->namespaces,
                                                 context->namespaces);
      if (context->namespaces == NULL)
        break;
      ns = context->namespaces->data;
    }
}

/* checks if an element name starts with the prefix of the given namespace */
static gboolean
has_ns_prefix (const gchar  *name,
               XMLNameSpace *ns)
{
  if (ns == NULL)
    return FALSE;
  return ((strncmp (name, ns->prefix, ns->prefix_len) == 0)
          && (name[ns->prefix_len] == ':'));
}

/* checks if an element or attribute matches a target with the given prefix */
static gboolean
matches_with_prefix (const gchar *name,
                     const gchar *prefix,
                     guint        prefix_len,
                     const gchar *target_name)
{
  if (prefix == NULL)
    return FALSE;
  return ((strncmp (name, prefix, prefix_len) == 0)
          && (strlen (name) > prefix_len + 1)
          && (name[prefix_len] == ':')
          && (strcmp (name + prefix_len + 1, target_name) == 0));
}

/* checks if an element or attribute matches a target in the RDF namespace */
static gboolean
matches_rdf (const gchar     *name,
             XMPParseContext *context,
             const gchar     *target_name)
{
  if (context->rdf_prefix != NULL)
    return matches_with_prefix (name,
                                context->rdf_prefix,
                                context->rdf_prefix_len,
                                target_name);
  else
    return matches_with_prefix (name, "rdf", 3, target_name);
}

/* add a new property to the schema referenced by its prefix */
/* the value(s) of the property will be added later by add_property_value() */
static XMLNameSpace *
new_property_in_ns (XMPParseContext  *context,
                    const gchar      *element_name)
{
  GSList       *list;
  XMLNameSpace *ns;

  g_return_val_if_fail (context->property == NULL, NULL);
  g_return_val_if_fail (context->prop_cur_value == -1, NULL);
  /* element_name is a new property if it starts with a known prefix */
  for (list = context->namespaces;
       list != NULL;
       list = g_slist_next (list))
    {
      ns = list->data;
      if (has_ns_prefix (element_name, ns))
        {
          context->property = g_strdup (element_name + ns->prefix_len + 1);
          context->property_type = XMP_PTYPE_UNKNOWN;
          context->property_ns = ns;
          context->prop_missing_value = FALSE;
          return ns;
        }
    }
  return NULL;
}

/* store a value for the current property - if the element containing the */
/* value is being parsed but the actual value has not been seen yet, then */
/* call this function with a NULL value so that its data structure is */
/* allocated now; it will be updated later with update_property_value() */
static void
add_property_value (XMPParseContext *context,
                    XMPParseType     type,
                    gchar           *name,
                    gchar           *value)
{
  g_return_if_fail (context->property != NULL);
  if (type == XMP_PTYPE_TEXT || type == XMP_PTYPE_RESOURCE)
    g_return_if_fail (context->prop_cur_value < 0);
  if (context->property_type != type)
    {
      /* make sure that we are not mixing different types in this property */
      g_return_if_fail (context->property_type == XMP_PTYPE_UNKNOWN);
      context->property_type = type;
    }
  if (context->prop_cur_value + 3 >= context->prop_max_value)
    {
      context->prop_max_value += 10;
      context->prop_value = g_realloc (context->prop_value,
                                       sizeof (gchar *)
                                       * context->prop_max_value);
    }
  /* some types store a name and a value; most others store only a value */
  if (type == XMP_PTYPE_ALT_LANG
      || type == XMP_PTYPE_STRUCTURE
      || type == XMP_PTYPE_ALT_THUMBS) /* for thumbnails, name is the size */
    {
      context->prop_cur_value++;
      context->prop_value[context->prop_cur_value] = name;
    }
  else
    g_assert (name == NULL);
  context->prop_cur_value++;
  context->prop_value[context->prop_cur_value] = value;
  context->prop_value[context->prop_cur_value + 1] = NULL;
  /* if value was NULL, then we must update it later */
  context->prop_missing_value = (value == NULL);
}

/* update a value that has been allocated but not stored yet */
static void
update_property_value (XMPParseContext *context,
                       gchar           *value)
{
  g_return_if_fail (context->property != NULL);
  g_return_if_fail (context->prop_cur_value >= 0);
  g_return_if_fail (context->prop_missing_value == TRUE);
  context->prop_value[context->prop_cur_value] = value;
  context->prop_missing_value = FALSE;
}

/* invoke the 'set_property' callback and free the temporary structures */
static void
propagate_property (XMPParseContext  *context,
                    GError          **error)
{
  g_return_if_fail (context->property != NULL);
  g_return_if_fail (context->prop_cur_value >= 0);
  if (context->parser->set_property)
    (*context->parser->set_property) (context,
                                      context->property,
                                      context->property_type,
                                      (const gchar **)(context->prop_value),
                                      context->property_ns->ns_user_data,
                                      context->user_data,
                                      error);
  if (! (context->flags & XMP_FLAG_DEFER_VALUE_FREE))
    {
      while (context->prop_cur_value >= 0)
        {
          g_free (context->prop_value[context->prop_cur_value]);
          context->prop_cur_value--;
        }
      g_free (context->prop_value);
    }
  context->prop_value = NULL;
  context->prop_cur_value = -1;
  context->prop_max_value = 0;
  g_free (context->property);
  context->property = NULL;
  context->property_ns = NULL;
}

/* called from the GMarkupParser */
static void
start_element_handler  (GMarkupParseContext  *markup_context,
                        const gchar          *element_name,
                        const gchar         **attribute_names,
                        const gchar         **attribute_values,
                        gpointer              user_data,
                        GError              **error)
{
  XMPParseContext *context = user_data;
  gint             attr;

#ifdef DEBUG_XMP_PARSER
  g_print ("[%25s/%17s] %d <%s>\n",
           state_names[context->state],
           (context->saved_state == STATE_ERROR
            ? "-"
            : state_names[context->saved_state]),
           context->depth, element_name);
#endif
  context->depth++;
  for (attr = 0; attribute_names[attr] != NULL; ++attr)
    if (g_str_has_prefix (attribute_names[attr], "xmlns:"))
      push_namespace (context, attribute_values[attr],
                      attribute_names[attr] + sizeof ("xmlns:") - 1, error);
  switch (context->state)
    {
    case STATE_INSIDE_XPACKET:
      if (! strcmp (element_name, "x:xmpmeta")
	  || ! strcmp (element_name, "x:xapmeta")
          || matches_with_prefix (element_name, context->xmp_prefix,
                                  context->xmp_prefix_len, "xmpmeta"))
	context->state = STATE_INSIDE_XMPMETA;
      else if (matches_rdf (element_name, context, "RDF"))
        {
          /* the x:xmpmeta element is missing, but this is allowed */
          context->depth++;
          context->state = STATE_INSIDE_RDF;
        }
      else
	parse_error_element (context, error, "x:xmpmeta",
                             FALSE, element_name);
      break;

    case STATE_INSIDE_XMPMETA:
      if (matches_rdf (element_name, context, "RDF"))
	context->state = STATE_INSIDE_RDF;
      else
	parse_error_element (context, error, "rdf:RDF",
                             FALSE, element_name);
      break;

    case STATE_INSIDE_RDF:
      if (matches_rdf (element_name, context, "Description"))
	{
          XMLNameSpace *ns;
          gboolean      about_seen = FALSE;

	  context->state = STATE_INSIDE_TOPLEVEL_DESC;
	  for (attr = 0; attribute_names[attr] != NULL; ++attr)
	    {
              if (matches_rdf (attribute_names[attr], context, "about")
                  || ! strcmp (attribute_names[attr], "about") /* old style */)
                about_seen = TRUE;
	      else if (g_str_has_prefix (attribute_names[attr], "xmlns"))
                ; /* the namespace has already been pushed on the stack */
	      else
                {
                  ns = new_property_in_ns (context, attribute_names[attr]);
                  if (ns != NULL)
                    {
                      /* RDF shorthand notation */
                      add_property_value (context, XMP_PTYPE_TEXT, NULL,
                                          g_strdup (attribute_values[attr]));
                      propagate_property (context, error);
                    }
                  else
                    unknown_attribute (context, error, element_name,
                                       attribute_names[attr],
                                       attribute_values[attr]);
                }
            }
          if ((about_seen == FALSE)
              && (context->flags & XMP_FLAG_NO_MISSING_ABOUT))
            parse_error (context, error, XMP_ERROR_MISSING_ABOUT,
                             _("Required attribute rdf:about missing in <%s>"),
                             element_name);
	}
      else
	parse_error_element (context, error, "rdf:Description",
                                 FALSE, element_name);
      break;

    case STATE_INSIDE_TOPLEVEL_DESC:
      {
        XMLNameSpace *ns;

        ns = new_property_in_ns (context, element_name);
        if (ns != NULL)
          {
            context->state = STATE_INSIDE_PROPERTY;
            for (attr = 0; attribute_names[attr] != NULL; ++attr)
              {
                if (matches_rdf (attribute_names[attr], context, "resource"))
                  add_property_value (context, XMP_PTYPE_RESOURCE, NULL,
                                      g_strdup (attribute_values[attr]));
                else if (matches_rdf (attribute_names[attr], context,
                                      "parseType")
                         && ! strcmp (attribute_values[attr], "Resource"))
                  {
                    context->saved_state = STATE_INSIDE_TOPLEVEL_DESC;
                    context->state = STATE_INSIDE_STRUCT_ADD_NS;
                  }
                else
                  unknown_attribute (context, error, element_name,
                                     attribute_names[attr],
                                     attribute_values[attr]);
              }
          }
        else
          unknown_element (context, error, element_name);
      }
      break;

    case STATE_INSIDE_PROPERTY:
      if (matches_rdf (element_name, context, "Description"))
	{
          context->saved_state = context->state;
	  context->state = STATE_INSIDE_QDESC;
	  for (attr = 0; attribute_names[attr] != NULL; ++attr)
	    {
	      if (g_str_has_prefix (attribute_names[attr], "xmlns"))
		{
		  /* this desc. is a structure, not a property qualifier */
		  context->state = STATE_INSIDE_STRUCT_ADD_NS;
		}
	      else
                unknown_attribute (context, error, element_name,
                                   attribute_names[attr],
                                   attribute_values[attr]);
	    }
	}
      else if (matches_rdf (element_name, context, "Alt"))
	context->state = STATE_INSIDE_ALT;
      else if (matches_rdf (element_name, context, "Bag"))
	context->state = STATE_INSIDE_BAG;
      else if (matches_rdf (element_name, context, "Seq"))
	context->state = STATE_INSIDE_SEQ;
      else
        unknown_element (context, error, element_name);
      break;

    case STATE_INSIDE_QDESC:
      if (matches_rdf (element_name, context, "value"))
	context->state = STATE_INSIDE_QDESC_VALUE;
      else
	context->state = STATE_INSIDE_QDESC_QUAL;
      break;

    case STATE_INSIDE_STRUCT_ADD_NS:
    case STATE_INSIDE_STRUCT:
      {
        GSList       *ns_list;
        XMLNameSpace *ns;
        gboolean      found;

        /* compare with namespaces in scope of current rdf:Description */
        found = FALSE;
        ns_list = context->namespaces;
        while (ns_list != NULL)
          {
            ns = ns_list->data;
            if (ns->depth < context->depth - 2)
              break;
            if (has_ns_prefix (element_name, ns))
              {
                if (context->state == STATE_INSIDE_STRUCT_ADD_NS)
                  {
                    /* first element - save the namespace prefix and uri */
                    add_property_value (context, XMP_PTYPE_STRUCTURE,
                                        g_strdup (ns->prefix),
                                        g_strdup (ns->uri));
                  }
                context->state = STATE_INSIDE_STRUCT_ELEMENT;
                add_property_value (context, XMP_PTYPE_STRUCTURE,
                                    g_strdup (element_name
                                              + ns->prefix_len + 1),
                                    NULL);
                found = TRUE;
                break;
              }
            ns_list = ns_list->next;
          }
        if (found == FALSE)
          unknown_element (context, error, element_name);
      }
      break;

    case STATE_INSIDE_ALT:
      if (matches_rdf (element_name, context, "li"))
	{
	  context->state = STATE_INSIDE_ALT_LI;
	  for (attr = 0; attribute_names[attr] != NULL; ++attr)
	    {
	      if (matches_rdf (attribute_names[attr], context, "parseType")
                  && ! strcmp (attribute_values[attr], "Resource"))
                context->state = STATE_INSIDE_ALT_LI_RSC;
	      else if (! strcmp (attribute_names[attr], "xml:lang"))
                add_property_value (context, XMP_PTYPE_ALT_LANG,
                                    g_strdup (attribute_values[attr]),
                                    NULL);
	      else
                unknown_attribute (context, error, element_name,
                                   attribute_names[attr],
                                   attribute_values[attr]);
	    }
          /* rdf:Alt is not an ordered list, but some broken XMP files use */
          /* it instead of rdf:Seq.  Workaround: if we did not find some */
          /* attributes for the valid cases ALT_LANG or ALT_LI_RSC, then */
          /* we pretend that we are parsing a real list (bug #343315). */
          if ((context->property_type != XMP_PTYPE_ALT_LANG)
              && (context->state != STATE_INSIDE_ALT_LI_RSC))
            add_property_value (context, XMP_PTYPE_ORDERED_LIST,
                                NULL, NULL);
	}
      else
	parse_error_element (context, error, "rdf:li",
                                 FALSE, element_name);
      break;

    case STATE_INSIDE_BAG:
      if (matches_rdf (element_name, context, "li"))
	{
          context->state = STATE_INSIDE_BAG_LI;
	  for (attr = 0; attribute_names[attr] != NULL; ++attr)
	    {
	      if (matches_rdf (attribute_names[attr], context, "parseType")
                  && ! strcmp (attribute_values[attr], "Resource"))
                context->state = STATE_INSIDE_BAG_LI_RSC;
	      else
                unknown_attribute (context, error, element_name,
                                   attribute_names[attr],
                                   attribute_values[attr]);
	    }
          if (context->state != STATE_INSIDE_BAG_LI_RSC)
            add_property_value (context, XMP_PTYPE_UNORDERED_LIST,
                                NULL, NULL);
	}
      else
	parse_error_element (context, error, "rdf:li",
                                 FALSE, element_name);
      break;

    case STATE_INSIDE_SEQ:
      if (matches_rdf (element_name, context, "li"))
	{
          context->state = STATE_INSIDE_SEQ_LI;
	  for (attr = 0; attribute_names[attr] != NULL; ++attr)
	    {
	      if (matches_rdf (attribute_names[attr], context, "parseType")
                  && ! strcmp (attribute_values[attr], "Resource"))
                context->state = STATE_INSIDE_SEQ_LI_RSC;
	      else
                unknown_attribute (context, error, element_name,
                                   attribute_names[attr],
                                   attribute_values[attr]);
	    }
          if (context->state != STATE_INSIDE_SEQ_LI_RSC)
            add_property_value (context, XMP_PTYPE_ORDERED_LIST,
                                NULL, NULL);
	}
      else
	parse_error_element (context, error, "rdf:li",
                                 FALSE, element_name);
      break;

    case STATE_INSIDE_BAG_LI:
    case STATE_INSIDE_SEQ_LI:
      if (matches_rdf (element_name, context, "Description"))
        {
          context->saved_state = context->state;
          context->state = STATE_INSIDE_QDESC;
        }
      else
	parse_error_element (context, error, "rdf:Description",
                             TRUE, element_name);
      break;

    case STATE_INSIDE_ALT_LI_RSC:
      /* store the thumbnail image and ignore the other elements */
      if (! strcmp (element_name, "xapGImg:image")) /* FIXME */
        context->state = STATE_INSIDE_ALT_LI_RSC_IMG;
      else if (! strcmp (element_name, "xapGImg:format")
               || ! strcmp (element_name, "xapGImg:width")
               || ! strcmp (element_name, "xapGImg:height"))
        ignore_element (context);
      else
        unknown_element (context, error, element_name);
      break;

    case STATE_INSIDE_BAG_LI_RSC:
    case STATE_INSIDE_SEQ_LI_RSC:
      unknown_element (context, error, element_name);
      break;

    case STATE_SKIPPING_UNKNOWN_ELEMENTS:
    case STATE_SKIPPING_IGNORED_ELEMENTS:
      break;

    default:
      parse_error (context, error, XMP_ERROR_PARSE,
                   _("Nested elements (<%s>) are not allowed in this context"),
                   element_name);
      break;
    }
}

/* called from the GMarkupParser */
static void
end_element_handler    (GMarkupParseContext  *markup_context,
                        const gchar          *element_name,
                        gpointer              user_data,
                        GError              **error)
{
  XMPParseContext *context = user_data;

#ifdef DEBUG_XMP_PARSER
  g_print ("[%25s/%17s] %d </%s>\n",
           state_names[context->state],
           (context->saved_state == STATE_ERROR
            ? "-"
            : state_names[context->saved_state]),
           context->depth, element_name);
#endif
  switch (context->state)
    {
    case STATE_INSIDE_PROPERTY:
      context->state = STATE_INSIDE_TOPLEVEL_DESC;
      if (context->property != NULL)
        {
          if (context->prop_cur_value < 0)
            {
              /* if not set yet, then property was empty */
              add_property_value (context, XMP_PTYPE_TEXT,
                                  NULL, g_strdup (""));
            }
          propagate_property (context, error);
        }
      break;

    case STATE_INSIDE_STRUCT:
      context->state = context->saved_state;
      if (context->property != NULL)
        propagate_property (context, error);
      break;

    case STATE_INSIDE_ALT:
    case STATE_INSIDE_BAG:
    case STATE_INSIDE_SEQ:
      context->state = STATE_INSIDE_PROPERTY;
      break;

    case STATE_INSIDE_QDESC:
      context->state = context->saved_state;
      break;

    case STATE_INSIDE_QDESC_VALUE:
    case STATE_INSIDE_QDESC_QUAL:
      context->state = STATE_INSIDE_QDESC;
      break;

    case STATE_INSIDE_STRUCT_ELEMENT:
      context->state = STATE_INSIDE_STRUCT;
      if ((context->prop_cur_value >= 0)
          && (context->prop_value[context->prop_cur_value] == NULL))
        update_property_value (context, g_strdup (""));
      break;

    case STATE_INSIDE_ALT_LI:
      context->state = STATE_INSIDE_ALT;
      if ((context->prop_cur_value >= 0)
          && (context->prop_value[context->prop_cur_value] == NULL))
        update_property_value (context, g_strdup (""));
      break;

    case STATE_INSIDE_ALT_LI_RSC:
      context->state = STATE_INSIDE_ALT;
      break;

    case STATE_INSIDE_ALT_LI_RSC_IMG:
      context->state = STATE_INSIDE_ALT_LI_RSC;
      break;

    case STATE_INSIDE_BAG_LI:
    case STATE_INSIDE_BAG_LI_RSC:
      context->state = STATE_INSIDE_BAG;
      break;

    case STATE_INSIDE_SEQ_LI:
    case STATE_INSIDE_SEQ_LI_RSC:
      context->state = STATE_INSIDE_SEQ;
      break;

    case STATE_INSIDE_TOPLEVEL_DESC:
      g_return_if_fail (matches_rdf (element_name, context, "Description"));
      context->state = STATE_INSIDE_RDF;
      break;

    case STATE_INSIDE_RDF:
      g_return_if_fail (matches_rdf (element_name, context, "RDF"));
      context->state = STATE_AFTER_RDF;
      break;

    case STATE_AFTER_RDF:
      g_return_if_fail (! strcmp (element_name, "x:xmpmeta")
                        || ! strcmp (element_name, "x:xapmeta")
                        || matches_with_prefix (element_name,
                                                context->xmp_prefix,
                                                context->xmp_prefix_len,
                                                "xmpmeta"));
      context->state = STATE_AFTER_XMPMETA;
      break;

    case STATE_SKIPPING_UNKNOWN_ELEMENTS:
    case STATE_SKIPPING_IGNORED_ELEMENTS:
      if (context->depth == context->saved_depth)
        {
          /* resume normal processing */
          context->state = context->saved_state;
          context->saved_state = STATE_ERROR;
        }
      break;

    default:
      parse_error (context, error, XMP_ERROR_PARSE,
		       _("End of element <%s> not expected in this context"),
                         element_name);
      break;
    }
  pop_namespaces (context, error);
  context->depth--;
}

/* called from the GMarkupParser */
static void
text_handler           (GMarkupParseContext  *markup_context,
                        const gchar          *text,
                        gsize                 text_len,
                        gpointer              user_data,
                        GError              **error)
{
  XMPParseContext *context = user_data;

  switch (context->state)
    {
    case STATE_INSIDE_PROPERTY:
      if (! is_whitespace_string (text))
        add_property_value (context, XMP_PTYPE_TEXT, NULL,
                            g_strndup (text, text_len));
      break;

    case STATE_INSIDE_STRUCT_ELEMENT:
    case STATE_INSIDE_ALT_LI:
    case STATE_INSIDE_BAG_LI:
    case STATE_INSIDE_SEQ_LI:
      if (! is_whitespace_string (text))
        update_property_value (context, g_strndup (text, text_len));
      break;

    case STATE_INSIDE_ALT_LI_RSC_IMG:
      {
        gint   max_size;
        gchar *decoded;
        gint   decoded_size;

#ifdef DEBUG_XMP_PARSER
        /* g_print ("XMP: Pushing text:\n%s\n", text); */
#endif
        max_size = text_len - text_len / 4 + 1;
        decoded = g_malloc (max_size);
        decoded_size = base64_decode (text, text_len, decoded, max_size,
                                      FALSE);
#ifdef DEBUG_XMP_PARSER
        if (decoded_size > 0)
          {
            /* FIXME: remove this debugging code */
            /*
            FILE *ttt;

            ttt = fopen ("/tmp/xmp-thumb.jpg", "wb");
            fwrite (decoded, decoded_size, 1, ttt);
            fclose (ttt);
            */
            g_print ("XMP: Thumb text len: %d (1/4 = %d)\nMax size: %d\nUsed size: %d\n", (int) text_len, (int) text_len / 4, max_size, decoded_size);
          }
#endif
        if (decoded_size > 0)
          {
            gint  *size_p;

            size_p = g_new (gint, 1);
            *size_p = decoded_size;
            add_property_value (context, XMP_PTYPE_ALT_THUMBS,
                                (char *) size_p, decoded);
          }
        else
          add_property_value (context, XMP_PTYPE_ALT_THUMBS, 0, NULL);
      }
      break;

    case STATE_INSIDE_QDESC_VALUE:
      if (! is_whitespace_string (text))
        {
          if (context->saved_state == STATE_INSIDE_PROPERTY)
            add_property_value (context, XMP_PTYPE_TEXT, NULL,
                                g_strndup (text, text_len));
          else
            update_property_value (context, g_strndup (text, text_len));
        }
      break;

    case STATE_INSIDE_QDESC_QUAL:
#ifdef DEBUG_XMP_PARSER
      g_print ("ignoring qualifier for part of \"%s\"[]: \"%.*s\"\n",
	       context->property,
	       (int)text_len, text);
#endif
      /* FIXME: notify the user? add a way to collect qualifiers? */
      break;

    case STATE_SKIPPING_UNKNOWN_ELEMENTS:
    case STATE_SKIPPING_IGNORED_ELEMENTS:
      break;

    default:
      if (! is_whitespace_string (text))
	parse_error (context, error, XMP_ERROR_INVALID_CONTENT,
			 _("The current element (<%s>) cannot contain text"),
                         g_markup_parse_context_get_element (markup_context));
      break;
    }
}

/* called from the GMarkupParser */
static void
passthrough_handler    (GMarkupParseContext  *markup_context,
                        const gchar          *passthrough_text,
                        gsize                 text_len,
                        gpointer              user_data,
                        GError              **error)
{
  XMPParseContext *context = user_data;

  switch (context->state)
    {
    case STATE_START:
    case STATE_AFTER_XPACKET:
      if ((text_len >= 21)
          && (! strncmp (passthrough_text, "<?xpacket begin=", 16)))
	context->state = STATE_INSIDE_XPACKET;
      else
	parse_error (context, error, XMP_ERROR_PARSE,
                     _("XMP packets must start with <?xpacket begin=...?>"));
      break;

    case STATE_AFTER_RDF:
      /* the x:xmpmeta element is missing */
      context->depth--;
      pop_namespaces (context, error);
      /*fallthrough*/
    case STATE_AFTER_XMPMETA:
      if ((text_len >= 19)
          && (! strncmp (passthrough_text, "<?xpacket end=", 14)))
	context->state = STATE_AFTER_XPACKET;
      else
	parse_error (context, error, XMP_ERROR_PARSE,
                     _("XMP packets must end with <?xpacket end=...?>"));
      break;

    default:
      if ((text_len >= 18)
          && (! strncmp (passthrough_text, "<?adobe-", 8)))
        ; /* ignore things like <?adobe-xap-filters esc="CRLF"?> */
      else if (! (context->flags & XMP_FLAG_NO_COMMENTS)
               && (text_len > 7)
               && (! strncmp (passthrough_text, "<!--", 4)))
        ; /* comments are not allowed in XMP, but let's ignore them */
      else
        parse_error (context, error, XMP_ERROR_INVALID_COMMENT,
                     _("XMP cannot contain XML comments or processing instructions"));
      break;
    }
}

/* called from the GMarkupParser */
static void
error_handler          (GMarkupParseContext *markup_context,
                        GError              *error,
                        gpointer             user_data)
{
  XMPParseContext *context = user_data;

  g_return_if_fail (context != NULL);

  context->state = STATE_ERROR;
  if ((error->domain != XMP_PARSE_ERROR) && (context->parser->error))
    (*context->parser->error) (context, error, context->user_data);
}

static GMarkupParser markup_xmp_parser = {
  start_element_handler,
  end_element_handler,
  text_handler,
  passthrough_handler,
  error_handler
};

/**
 * xmp_parse_context_new:
 * @parser: a #XMPParser
 * @flags: one or more #XMPParseFlags
 * @user_data: user data to pass to #GMarkupParser functions
 * @user_data_dnotify: user data destroy notifier called when the parse context is freed
 *
 * Creates a new XMP parse context.  A parse context is used to parse
 * documents.  You can feed any number of documents containing XMP
 * metadata into a context, as long as no errors occur; once an error
 * occurs, the parse context can't continue to parse text (you have to
 * free it and create a new parse context).
 *
 * Return value: a new #XMPParseContext
 **/
XMPParseContext *
xmp_parse_context_new (const XMPParser *parser,
                       XMPParseFlags    flags,
                       gpointer         user_data,
                       GDestroyNotify   user_data_dnotify)
{
  XMPParseContext     *context;

  g_return_val_if_fail (parser != NULL, NULL);

  context = g_new (XMPParseContext, 1);

  context->parser = parser;
  context->flags = flags;
  context->user_data = user_data;
  context->user_data_dnotify = user_data_dnotify;

  context->markup_context = g_markup_parse_context_new (&markup_xmp_parser,
                                                        0, context, NULL);
  context->state = STATE_START;
  context->depth = 0;

  context->namespaces = NULL;
  context->xmp_prefix = NULL;
  context->xmp_prefix_len = 0;
  context->rdf_prefix = NULL;
  context->rdf_prefix_len = 0;

  context->property = NULL;
  context->property_ns = NULL;
  context->prop_value = NULL;
  context->prop_cur_value = -1;
  context->prop_max_value = 0;
  context->prop_missing_value = FALSE;

  context->saved_state = STATE_ERROR;
  context->saved_depth = 0;

  return context;
}

/**
 * xmp_parse_context_free:
 * @context: a #XMPParseContext
 *
 * Frees a #XMPParseContext.  Cannot be called from inside one of the
 * #XMPParser functions.
 *
 **/
void
xmp_parse_context_free (XMPParseContext *context)
{
  g_return_if_fail (context != NULL);
  if (context->user_data_dnotify)
    (* context->user_data_dnotify) (context->user_data);

  g_slist_free (context->namespaces);
  g_free (context->xmp_prefix);
  g_free (context->rdf_prefix);
  if (! (context->flags & XMP_FLAG_DEFER_VALUE_FREE))
    {
      while (context->prop_cur_value >= 0)
        {
          g_free (context->prop_value[context->prop_cur_value]);
          context->prop_cur_value--;
        }
      g_free (context->prop_value);
    }
  g_free (context->property);
  g_free (context);
}

/**
 * xmp_parse_context_parse:
 * @context: a #XMPParseContext
 * @text: chunk of text to parse
 * @text_len: length of @text in bytes
 * @error: return location for a #GError
 *
 * Feed some data to the #XMPParseContext.  The data need not be valid
 * UTF-8; an error will be signaled if it's invalid.  The data need
 * not be an entire document; you can feed a document into the parser
 * incrementally, via multiple calls to this function.  Typically, as
 * you receive data from a network connection or file, you feed each
 * received chunk of data into this function, aborting the process if
 * an error occurs.  Once an error is reported, no further data may be
 * fed to the #XMPParseContext; all errors are fatal.
 *
 * Return value: %FALSE if an error occurred, %TRUE on success
 **/
gboolean
xmp_parse_context_parse (XMPParseContext  *context,
                         const gchar      *text,
                         gssize            text_len,
                         GError          **error)
{
  g_return_val_if_fail (context != NULL, FALSE);
  g_return_val_if_fail (text != NULL, FALSE);
  g_return_val_if_fail (context->state != STATE_ERROR, FALSE);

  if ((context->flags & XMP_FLAG_FIND_XPACKET)
      && (context->state == STATE_START
          || context->state == STATE_AFTER_XPACKET))
    {
      /* There may be some arbitrary data before the <?xpacket...?>
       * marker so we should first find it according to the
       * recommendations of the XMP specification.  Once the start of
       * the XMP packet has been found, the GMarkupParser can start
       * its work on the text (including the <?xpacket...?> marker).
       */
      /* FIXME: wrong, wrong, wrong!  but useful for simple tests... */
      gint i, e;
      for (i = 0; i < text_len - 20; i++)
        if (! strncmp (text + i, "<?xpacket begin=", 16))
          {
            for (e = i; e < text_len - 10; e++)
              if (! strncmp (text + e, "<?xpacket end=", 14))
                break;
            while ((e < text_len) && *(text + e) != '>')
              e++;
            return g_markup_parse_context_parse (context->markup_context,
                                                 text + i, e - i + 1, error);
          }
      parse_error (context, error, XMP_ERROR_NO_XPACKET, NULL);
      return FALSE;
    }
  return g_markup_parse_context_parse (context->markup_context,
                                       text, text_len, error);
}

/**
 * xmp_parse_context_end_parse:
 * @context: a #XMPParseContext
 * @error: return location for a #GError
 *
 * Signals to the #XMPParseContext that all data has been fed into the
 * parse context with xmp_parse_context_parse().  This function
 * reports an error if the document did not contain any XMP packet or
 * if the document isn't complete, for example if elements are still
 * open.
 *
 * Return value: %TRUE on success, %FALSE if an error was set
 **/
gboolean
xmp_parse_context_end_parse (XMPParseContext  *context,
                             GError          **error)
{
  g_return_val_if_fail (context != NULL, FALSE);
  g_return_val_if_fail (context->state != STATE_ERROR, FALSE);

  if (context->state == STATE_START)
    parse_error (context, error, XMP_ERROR_NO_XPACKET, NULL);
  return g_markup_parse_context_end_parse (context->markup_context, error);
}
