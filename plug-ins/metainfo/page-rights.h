/* page-rights.h
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

#ifndef __PAGE_RIGHTS_H__
#define __PAGE_RIGHTS_H__

#include <glib-object.h>

G_BEGIN_DECLS

void                          page_rights_start                      (GtkBuilder      *builder,
                                                                      GimpAttributes  *attributes);
void                          page_rights_get_attributes             (GimpAttributes **attributes);

G_END_DECLS

#endif /* __PAGE_RIGHTS_H__ */
