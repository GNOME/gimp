/* Plugin-helper.h 
 * Copyright (C) 2000 Nathan Summers
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

#ifndef __PLUGIN_HELPER_H__
#define __PLUGIN_HELPER_H__

extern void plugin_module_install_procedure (gchar * name,
					     gchar * blurb,
					     gchar * help,
					     gchar * author,
					     gchar * copyright,
					     gchar * date,
					     gchar * menu_path,
					     gchar * image_types,
					     gint nparams,
					     gint nreturn_vals,
					     GimpParamDef * params,
					     GimpParamDef * return_vals,
					     GimpRunProc run_proc);


#endif /* __PLUGIN_HELPER_H__ */
