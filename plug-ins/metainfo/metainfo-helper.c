/* metainfo-helper.c
 *
 * Copyright (C) 2014, Hartmut Kuhse <hatti@gimp.org>
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#include "metainfo-helper.h"

static GtkWidget *save_attributes_button = NULL;
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
gchar*
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
gint
string_index_of (const gchar* haystack, const gchar* needle, gint start_index)
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
glong
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
gchar*
string_substring (const gchar* string, glong offset, glong len)
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

GObject *
get_widget_from_label (GtkBuilder *builder, const gchar *label)
{
  GObject *obj;

  obj = gtk_builder_get_object (builder, label);

  return obj;
}

void
set_save_attributes_button (GtkButton *button)
{
  save_attributes_button = GTK_WIDGET (button);
}

void
set_save_attributes_button_sensitive (gboolean sensitive)
{
  gtk_widget_set_sensitive (save_attributes_button, sensitive);
}
