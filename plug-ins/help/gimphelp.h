/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The LIGMA Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
 *                         Henrik Brix Andersen <brix@ligma.org>
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

#ifndef __LIGMA_HELP_H__
#define __LIGMA_HELP_H__


#include "ligmahelptypes.h"

#include "ligmahelpdomain.h"
#include "ligmahelpitem.h"
#include "ligmahelplocale.h"
#include "ligmahelpprogress.h"


#define LIGMA_HELP_DEFAULT_DOMAIN  "https://www.ligma.org/help"
#define LIGMA_HELP_DEFAULT_ID      "ligma-main"
#define LIGMA_HELP_DEFAULT_LOCALE  "en"

#define LIGMA_HELP_PREFIX          "help"
#define LIGMA_HELP_ENV_URI         "LIGMA2_HELP_URI"

/* #define LIGMA_HELP_DEBUG */


gboolean         ligma_help_init            (const gchar   **domain_names,
                                            const gchar   **domain_uris);
void             ligma_help_exit            (void);

void             ligma_help_register_domain (const gchar    *domain_name,
                                            const gchar    *domain_uri);
LigmaHelpDomain * ligma_help_lookup_domain   (const gchar    *domain_name);

GList          * ligma_help_parse_locales   (const gchar    *help_locales);


#endif /* ! __LIGMA_HELP_H__ */
