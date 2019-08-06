/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin-private.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_PLUG_IN_PRIVATE_H__
#define __GIMP_PLUG_IN_PRIVATE_H__

G_BEGIN_DECLS


typedef struct _GimpPlugInMenuBranch GimpPlugInMenuBranch;

struct _GimpPlugInMenuBranch
{
  gchar *menu_path;
  gchar *menu_label;
};

struct _GimpPlugInPrivate
{
  GIOChannel *read_channel;
  GIOChannel *write_channel;
  guint       extension_source_id;

  gchar      *translation_domain_name;
  GFile      *translation_domain_path;

  gchar      *help_domain_name;
  GFile      *help_domain_uri;

  GList      *menu_branches;

  GList      *temp_procedures;
};


void         _gimp_plug_in_query             (GimpPlugIn      *plug_in);
void         _gimp_plug_in_init              (GimpPlugIn      *plug_in);
void         _gimp_plug_in_run               (GimpPlugIn      *plug_in);
void         _gimp_plug_in_quit              (GimpPlugIn      *plug_in);

GIOChannel * _gimp_plug_in_get_read_channel  (GimpPlugIn      *plug_in);
GIOChannel * _gimp_plug_in_get_write_channel (GimpPlugIn      *plug_in);

void         _gimp_plug_in_read_expect_msg   (GimpPlugIn      *plug_in,
                                              GimpWireMessage *msg,
                                              gint             type);
gboolean     _gimp_plug_in_extension_read    (GIOChannel      *channel,
                                              GIOCondition     condition,
                                              gpointer         data);
void         _gimp_plug_in_single_message    (GimpPlugIn      *plug_in);


G_END_DECLS

#endif /* __GIMP_PLUG_IN_PRIVATE_H__ */
