/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpRc
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "gimpconfig.h"
#include "gimpconfig-serialize.h"
#include "gimprc.h"


static void  gimp_rc_config_iface_init (gpointer     iface,
                                        gpointer     iface_data);
static void  gimp_rc_serialize         (GObject     *object,
                                        gint         fd);


GType 
gimp_rc_get_type (void)
{
  static GType rc_type = 0;

  if (! rc_type)
    {
      static const GTypeInfo rc_info =
      {
        sizeof (GimpRcClass),
	NULL,           /* base_init      */
        NULL,           /* base_finalize  */
	NULL,           /* class_init     */
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpRc),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };
      static const GInterfaceInfo rc_iface_info = 
      { 
        gimp_rc_config_iface_init,
        NULL,           /* iface_finalize */ 
        NULL            /* iface_data     */
      };

      rc_type = g_type_register_static (GIMP_TYPE_GUI_CONFIG, 
                                        "GimpRc", 
                                        &rc_info, 0);

      g_type_add_interface_static (rc_type,
                                   GIMP_TYPE_CONFIG_INTERFACE,
                                   &rc_iface_info);
    }

  return rc_type;
}

static void
gimp_rc_config_iface_init (gpointer  iface,
                           gpointer  iface_data)
{
  GimpConfigInterface *config_iface = (GimpConfigInterface *) iface;

  config_iface->serialize = gimp_rc_serialize;
}

static void
gimp_rc_serialize (GObject *object,
                   gint     fd)
{
  gimp_config_serialize_properties (object, fd);
  gimp_config_serialize_unknown_tokens (object, fd);
}

GimpRc *
gimp_rc_new (void)
{
  GimpRc *gimprc;

  gimprc = GIMP_RC (g_object_new (GIMP_TYPE_RC, NULL));

  return gimprc;
}
