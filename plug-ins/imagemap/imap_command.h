/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#ifndef _IMAP_COMMAND_H
#define _IMAP_COMMAND_H

#include "imap_object.h"

#define DEFAULT_UNDO_LEVELS 10

typedef struct CommandClass_t CommandClass_t;
typedef struct Command_t Command_t;
typedef struct CommandList_t CommandList_t;

typedef enum {CMD_APPEND, CMD_DESTRUCT, CMD_IGNORE} CmdExecuteValue_t;

struct CommandClass_t {
   void (*destruct)(Command_t*);
   CmdExecuteValue_t (*execute)(Command_t*);
   void (*undo)(Command_t*);
   void (*redo)(Command_t*);
};

struct Command_t {
   CommandClass_t      *class;
   CommandList_t       *sub_commands;
   const gchar         *name;
   gboolean             locked;
};


typedef Command_t* (*CommandFactory_t)(void);

typedef void (*CommandListCallbackFunc_t)(Command_t*, gpointer);

typedef struct {
   CommandListCallbackFunc_t func;
   gpointer data;
} CommandListCB_t;

typedef struct {
   GList *list;
} CommandListCallback_t;

struct CommandList_t {
   CommandList_t *parent;
   gint undo_levels;
   GList *list;
   GList *undo;                 /* Pointer to current undo command */
   GList *redo;                 /* Pointer to current redo command */
   CommandListCallback_t update_cb;
};

CommandList_t *command_list_new(gint undo_levels);
void command_list_destruct(CommandList_t *list);
void command_list_set_undo_level(gint level);
void command_list_add(Command_t *command);
void command_list_remove_all(void);
void command_list_undo(CommandList_t *list);
void command_list_undo_all(CommandList_t *list);
void command_list_redo(CommandList_t *list);
void command_list_redo_all(CommandList_t *list);
void command_list_add_update_cb(CommandListCallbackFunc_t func, gpointer data);
Command_t *command_list_get_redo_command(void);

Command_t *command_new(void (*func)(void));
Command_t *command_init(Command_t *command, const gchar *name,
                        CommandClass_t *class);
void command_execute(Command_t *command);
void command_undo(Command_t *command);
void command_redo(Command_t *command);
void command_set_name(Command_t *command, const gchar *name);
void command_add_subcommand(Command_t *command, Command_t *sub_command);

void last_command_undo(void);
void last_command_redo(void);

void subcommand_list_add(CommandList_t *list, Command_t *command);
Command_t *subcommand_start(const gchar *name);
void subcommand_end(void);

#define command_lock(command) ((command)->locked = TRUE)

#endif /* _IMAP_COMMAND_H */
