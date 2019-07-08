/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help Browser - URI functions
 * Copyright (C) 2001  Jacob Schroeder  <jacob@convergence.de>
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

#include <string.h>

#include <glib.h>

#include "uri.h"

/*  #define URI_DEBUG 1  */

typedef enum
{
  URI_UNKNOWN,
  URI_ABSURI,
  URI_NETPATH,
  URI_ABSPATH,
  URI_RELPATH,
  URI_QUERY,
  URI_EMPTY,
  URI_FRAGMENT,
  URI_INVALID
} UriType;


static UriType
uri_get_type (const gchar *uri)
{
  gchar        c;
  const gchar *cptr;
  UriType      type = URI_UNKNOWN;

  if (!uri)
    return type;

  cptr = uri;
  c = *cptr++;

  if (g_ascii_isalpha (c))
    {
      type = URI_RELPATH;  /* assume relative path */

      while ((c = *cptr++))
        {
          if (g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.')
            continue;

          if (c == ':')
            {
              /* it was a scheme */
              type = URI_ABSURI;
            }
          break;
        }
    }
  else
    {
      switch (c)
        {
        case '/':
          if (*cptr == '/')
            {
              cptr++;
              type = URI_NETPATH;
            }
          else
            {
              type = URI_ABSPATH;
            }
          break;
        case '?':
          type = URI_QUERY;
          break;
        case '#':
          type = URI_FRAGMENT;
          break;
        case '\0':
          type = URI_EMPTY;
          break;
        default:
          type = URI_RELPATH;
          break;
        }
    }

#ifdef URI_DEBUG
  g_print ("uri_get_type (\"%s\") -> ", uri);
  switch (type)
    {
    case URI_UNKNOWN:  g_print ("unknown");  break;
    case URI_ABSURI:   g_print ("absuri");   break;
    case URI_NETPATH:  g_print ("netpath");  break;
    case URI_ABSPATH:  g_print ("abspath");  break;
    case URI_RELPATH:  g_print ("relpath");  break;
    case URI_QUERY:    g_print ("query");    break;
    case URI_EMPTY:    g_print ("empty");    break;
    case URI_FRAGMENT: g_print ("fragment"); break;
    case URI_INVALID:  g_print ("invalid");  break;
    }
  g_print ("\n");
#endif

  return type;
}

gchar *
uri_to_abs (const gchar *uri,
            const gchar *base_uri)
{
  gchar        c;
  const gchar *cptr;
  gchar       *retval    = NULL;
  UriType      uri_type  = URI_UNKNOWN;
  UriType      base_type = URI_UNKNOWN;

  gint base_cnt    =  0;  /* no of chars to be copied from base URI  */
  gint uri_cnt     =  0;  /* no of chars to be copied from URI       */
  gint sep_cnt     =  0;  /* no of chars to be inserted between them */

  const gchar *sep_str = ""; /* string to insert between base and uri */
  const gchar *part;
  const gchar *last_segment = NULL;

#ifdef URI_DEBUG
  g_print ("uri_to_abs (\"%s\", \"%s\")\n", uri, base_uri);
#endif

  /* this function does not use the algorithm that is being proposed
   * in RFC 2396. Instead it analyses the first characters of each
   * URI to determine its kind (abs, net, path, ...).
   * After that it locates the missing parts in the base URI and then
   * concats everything into a newly allocated string.
   */

  /* determine the kind of the URIs */
  uri_type = uri_get_type (uri);

  if (uri_type != URI_ABSURI)
    {
      base_type = uri_get_type (base_uri);

      if (base_type != URI_ABSURI)
        return NULL;  /*  neither uri nor base uri are absolute  */
    }

  /* find missing parts in base URI */
  switch (uri_type)
    {
    case URI_ABSURI:
      /* base uri not needed */
      break;

    case URI_QUERY:
      /* ??? last segment? */
      uri_type = URI_RELPATH;
    case URI_NETPATH:  /* base scheme */
    case URI_ABSPATH:  /* base scheme and authority */
    case URI_RELPATH:  /* base scheme, authority and path */
      cptr = base_uri;

      /* skip scheme */
      while ((c = *cptr++) && c != ':')
        ; /* nada */

      base_cnt = cptr - base_uri; /* incl : */

      if (*cptr != '/')
        {
          /* completion not possible */
          return NULL;
        }

      if (uri_type == URI_NETPATH)
        break;

      /* skip authority */
      if (cptr[0] == '/' && cptr[1] == '/')
        {
          part = cptr;
          cptr += 2;

          while ((c = *cptr++) && c != '/' && c != '?' && c != '#')
            ; /* nada */

          cptr--;
          base_cnt += cptr - part;
        }

      if (uri_type == URI_ABSPATH)
        break;

      /* skip path */
      if (*cptr != '/')
        {
          sep_cnt = 1;
          sep_str = "/";
          break;
        }

      part = cptr;

      g_assert (*cptr == '/');

      while ((c = *cptr++) && c != '?' && c != '#')
        {
          if (c == '/')
            last_segment = cptr - 1;
        };

      g_assert (last_segment);

      cptr = last_segment;

      while ((c = *uri) && c == '.' && cptr > part)
        {
          gint shift_segment = 0;

          c = uri[1];

          if (c == '.' )
            {
              c = uri[2];
              shift_segment = 1;
            }

          if (c == '/')
            {
              uri += 2;
            }
          else if (c == 0 || c == '?' || c == '#')
            {
              uri += 1;
            }
          else
            {
              break;
            }

          g_assert (*cptr == '/');

          if (shift_segment)
            {
              uri += 1;
              while (cptr > part && *--cptr != '/')
                ; /* nada */
            }
        }

      base_cnt += cptr - part + 1;
      break;

    case URI_EMPTY:
    case URI_FRAGMENT:
      /* use whole base uri */
      base_cnt = strlen (base_uri);
      break;

    case URI_UNKNOWN:
    case URI_INVALID:
      return NULL;
    }

  /* do not include fragment part from the URI reference */
  for (cptr = uri; (c = *cptr) && c != '#'; cptr++)
    ; /* nada */

  uri_cnt = cptr - uri;

  /* allocate string and copy characters */

  retval = g_new (gchar, base_cnt + sep_cnt + uri_cnt + 1);

  if (base_cnt)
    strncpy (retval, base_uri, base_cnt);

  if (sep_cnt)
    strncpy (retval + base_cnt, sep_str, sep_cnt);

  if (uri_cnt)
    strncpy (retval + base_cnt + sep_cnt, uri, uri_cnt);

  retval[base_cnt + sep_cnt + uri_cnt] = '\0';

#ifdef URI_DEBUG
  g_print ("  ->  \"%s\"\n", retval);
#endif

  return retval;
}

#if 0
RFC 2396                   URI Generic Syntax                August 1998


A. Collected BNF for URI

      URI-reference = [ absoluteURI | relativeURI ] [ "#" fragment ]
      absoluteURI   = scheme ":" ( hier_part | opaque_part )
      relativeURI   = ( net_path | abs_path | rel_path ) [ "?" query ]

      hier_part     = ( net_path | abs_path ) [ "?" query ]
      opaque_part   = uric_no_slash *uric

      uric_no_slash = unreserved | escaped | ";" | "?" | ":" | "@" |
                      "&" | "=" | "+" | "$" | ","

      net_path      = "//" authority [ abs_path ]
      abs_path      = "/"  path_segments
      rel_path      = rel_segment [ abs_path ]

      rel_segment   = 1*( unreserved | escaped |
                          ";" | "@" | "&" | "=" | "+" | "$" | "," )

      scheme        = alpha *( alpha | digit | "+" | "-" | "." )

      authority     = server | reg_name

      reg_name      = 1*( unreserved | escaped | "$" | "," |
                          ";" | ":" | "@" | "&" | "=" | "+" )

      server        = [ [ userinfo "@" ] hostport ]
      userinfo      = *( unreserved | escaped |
                         ";" | ":" | "&" | "=" | "+" | "$" | "," )

      hostport      = host [ ":" port ]
      host          = hostname | IPv4address
      hostname      = *( domainlabel "." ) toplabel [ "." ]
      domainlabel   = alphanum | alphanum *( alphanum | "-" ) alphanum
      toplabel      = alpha | alpha *( alphanum | "-" ) alphanum
      IPv4address   = 1*digit "." 1*digit "." 1*digit "." 1*digit
      port          = *digit

      path          = [ abs_path | opaque_part ]
      path_segments = segment *( "/" segment )
      segment       = *pchar *( ";" param )
      param         = *pchar
      pchar         = unreserved | escaped |
                      ":" | "@" | "&" | "=" | "+" | "$" | ","

      query         = *uric

      fragment      = *uric

      uric          = reserved | unreserved | escaped
      reserved      = ";" | "/" | "?" | ":" | "@" | "&" | "=" | "+" |
                      "$" | ","
      unreserved    = alphanum | mark
      mark          = "-" | "_" | "." | "!" | "~" | "*" | "'" |
                      "(" | ")"

      escaped       = "%" hex hex
      hex           = digit | "A" | "B" | "C" | "D" | "E" | "F" |
                              "a" | "b" | "c" | "d" | "e" | "f"

      alphanum      = alpha | digit
      alpha         = lowalpha | upalpha

      lowalpha = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" | "i" |
                 "j" | "k" | "l" | "m" | "n" | "o" | "p" | "q" | "r" |
                 "s" | "t" | "u" | "v" | "w" | "x" | "y" | "z"
      upalpha  = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" |
                 "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" |
                 "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
      digit    = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" |
                 "8" | "9"

#endif
