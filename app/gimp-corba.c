/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Elliot Lee
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
 *
 * GNOME CORBA integration for the GIMP.
 * 
 * Author: Elliot Lee <sopwith@redhat.com>
 */

#include "config.h"
#include "gimp-corba.h"

void
gimp_corba_init(void)
{
  const char *goad_id;

  goad_id = goad_server_activation_id();

  /* Check for various functionality requests in the goad_id here... */

  gimp_bonobo_init();
				
}
