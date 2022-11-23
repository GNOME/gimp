/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaprocedureconfig-private.h
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_PROCEDURE_CONFIG_PRIVATE_H__
#define __LIGMA_PROCEDURE_CONFIG_PRIVATE_H__


gboolean   ligma_procedure_config_has_default   (LigmaProcedureConfig  *config);

gboolean   ligma_procedure_config_load_default  (LigmaProcedureConfig  *config,
                                                GError              **error);
gboolean   ligma_procedure_config_save_default  (LigmaProcedureConfig  *config,
                                                GError              **error);

gboolean   ligma_procedure_config_load_last     (LigmaProcedureConfig  *config,
                                                GError              **error);
gboolean   ligma_procedure_config_save_last     (LigmaProcedureConfig  *config,
                                                GError              **error);

gboolean   ligma_procedure_config_load_parasite (LigmaProcedureConfig  *config,
                                                LigmaImage            *image,
                                                GError              **error);
gboolean   ligma_procedure_config_save_parasite (LigmaProcedureConfig  *config,
                                                LigmaImage            *image,
                                                GError              **error);


#endif /* __LIGMA_PROCEDURE_CONFIG_PRIVATE_H__ */
