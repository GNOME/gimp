/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The GIMP Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#pragma once

#include "gimphelptypes.h"

#include "gimphelpdomain.h"
#include "gimphelpitem.h"
#include "gimphelplocale.h"
#include "gimphelpprogress.h"


#define GIMP_HELP_DEFAULT_DOMAIN  "https://www.gimp.org/help"
#define GIMP_HELP_DEFAULT_ID      "gimp-main"
#define GIMP_HELP_DEFAULT_LOCALE  "en"

#define GIMP_HELP_PREFIX          "help"
#define GIMP_HELP_ENV_URI         "GIMP2_HELP_URI"

/* #define GIMP_HELP_DEBUG */


gboolean         gimp_help_init            (const gchar   **domain_names,
                                            const gchar   **domain_uris);
void             gimp_help_exit            (void);

void             gimp_help_register_domain (const gchar    *domain_name,
                                            const gchar    *domain_uri);
GimpHelpDomain * gimp_help_lookup_domain   (const gchar    *domain_name);

GList          * gimp_help_parse_locales   (const gchar    *help_locales);
