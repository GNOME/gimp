/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis and others
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

#ifndef __GIMP_TOOL_MODULE_H__
#define __GIMP_TOOL_MODULE_H__

#include <gmodule.h>

#define GIMP_TYPE_TOOL_MODULE            (gimp_tool_module_get_type ())
#define GIMP_TOOL_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_MODULE, GimpToolModule))
#define GIMP_TOOL_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_MODULE, GimpToolModuleClass))
#define GIMP_IS_TOOL_MODULE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_MODULE))
#define GIMP_IS_TOOL_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_MODULE))
#define GIMP_TOOL_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_MODULE, GimpToolModuleClass))


typedef struct _GimpToolModuleClass GimpToolModuleClass;


struct _GimpToolModule
{
  GTypeModule	parent_instance;
  
  GModule	*module;
  gchar         *filename;
  
  void       (* register_tool) (Gimp                     *gimp,
                                GimpToolRegisterCallback  callback);
                          
  gboolean   (* register_type) (GimpToolModule           *module);

};

struct _GimpToolModuleClass
{
  GTypeModuleClass  parent_class;

};


GType            gimp_tool_module_get_type (void) G_GNUC_CONST;
GimpToolModule * gimp_tool_module_new      (const gchar              *filename, 
                                            GimpToolRegisterCallback  callback,
                                            gpointer                  register_data);


#endif  /*  __GIMP_TOOL_MODULE_H__  */
