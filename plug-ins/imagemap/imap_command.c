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

#include "config.h"

#include <stdio.h>

#include <gtk/gtk.h>

#include "imap_command.h"

#define INFINITE_UNDO_LEVELS -1

static void command_destruct(Command_t *command);

static CommandList_t _command_list = {NULL, DEFAULT_UNDO_LEVELS};
static CommandList_t *_current_command_list = &_command_list;

static void
command_list_callback_add(CommandListCallback_t *list,
                          CommandListCallbackFunc_t func, gpointer data)
{
   CommandListCB_t *cb = g_new(CommandListCB_t, 1);
   cb->func = func;
   cb->data = data;
   list->list = g_list_append(list->list, cb);
}

static void
command_list_callback_call(CommandListCallback_t *list, Command_t *command)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      CommandListCB_t *cb = (CommandListCB_t*) p->data;
      cb->func(command, cb->data);
   }
}

CommandList_t*
command_list_new(gint undo_levels)
{
   CommandList_t *list = g_new(CommandList_t, 1);
   list->parent = NULL;
   list->undo_levels = undo_levels;
   list->list = NULL;
   list->undo = NULL;
   list->redo = NULL;
   list->update_cb.list = NULL;
   return list;
}

static void
command_list_clear(CommandList_t *list)
{
   GList *p;
   for (p = list->list; p; p = p->next)
      command_destruct((Command_t*) p->data);
   g_list_free(list->list);
   list->list = NULL;
   list->undo = NULL;
   list->redo = NULL;
   command_list_callback_call(&list->update_cb, NULL);
}

void
command_list_destruct(CommandList_t *list)
{
   command_list_clear(list);
   g_free(list);
}

void
command_list_remove_all(void)
{
   command_list_clear(&_command_list);
}

static void
_command_list_add(CommandList_t *list, Command_t *command)
{
   GList *p, *q;

   /* Remove rest */
   for (p = list->redo; p; p = q) {
      Command_t *curr = (Command_t*) p->data;
      q = p->next;
      command_destruct(curr);
      list->list = g_list_remove_link(list->list, p);
   }
   if (g_list_length(list->list) == list->undo_levels) {
      GList *first = g_list_first(list->list);
      Command_t *curr = (Command_t*) first->data;
      command_destruct(curr);
      list->list = g_list_remove_link(list->list, first);
   }
   list->list = g_list_append(list->list, (gpointer) command);
   list->undo = g_list_last(list->list);
   list->redo = NULL;

   command_list_callback_call(&list->update_cb, command);
}

void
command_list_add(Command_t *command)
{
   _command_list_add(_current_command_list, command);
}

/* Fix me! */
void
subcommand_list_add(CommandList_t *list, Command_t *command)
{
   _command_list_add(list, command);
}

static CommandClass_t parent_command_class = {
   NULL,                        /* parent_command_destruct */
   NULL,                        /* parent_command_execute */
   NULL,                        /* parent_command_undo */
   NULL                         /* parent_command_redo */
};

static Command_t*
command_list_start(CommandList_t *list, const gchar *name)
{
   Command_t *command = g_new(Command_t, 1);
   command_init(command, name, &parent_command_class);
   command->sub_commands = command_list_new(INFINITE_UNDO_LEVELS);

   command_list_add(command);
   command->sub_commands->parent = _current_command_list;
   _current_command_list = command->sub_commands;

   return command;
}

static void
command_list_end(CommandList_t *list)
{
   _current_command_list = list->parent;
}

Command_t*
subcommand_start(const gchar *name)
{
   return command_list_start(_current_command_list, name);
}

void
subcommand_end(void)
{
   command_list_end(_current_command_list);
}

static void
_command_list_set_undo_level(CommandList_t *list, gint level)
{
   gint diff = g_list_length(list->list) - level;
   if (diff > 0) {
      GList *p, *q;
      /* first remove data at the front */
      for (p = list->list; diff && p != list->undo; p = q, diff--) {
         Command_t *curr = (Command_t*) p->data;
         q = p->next;
         command_destruct(curr);
         list->list = g_list_remove_link(list->list, p);
      }

      /* If still to long start removing redo levels at the end */
      for (p = g_list_last(list->list); diff && p != list->undo; p = q,
              diff--) {
         Command_t *curr = (Command_t*) p->data;
         q = p->prev;
         command_destruct(curr);
         list->list = g_list_remove_link(list->list, p);
      }
      command_list_callback_call(&list->update_cb,
                                 (Command_t*) list->undo->data);
   }
   list->undo_levels = level;
}

void
command_list_set_undo_level(gint level)
{
   _command_list_set_undo_level(&_command_list, level);
}

Command_t*
command_list_get_redo_command(void)
{
   return (_command_list.redo) ? (Command_t*) _command_list.redo->data : NULL;
}

void
command_list_add_update_cb(CommandListCallbackFunc_t func, gpointer data)
{
   command_list_callback_add(&_command_list.update_cb, func, data);
}

static void
command_destruct(Command_t *command)
{
   if (command->sub_commands)
      command_list_destruct(command->sub_commands);
   if (command->class->destruct)
      command->class->destruct(command);
}

static void
command_list_execute(CommandList_t *list)
{
   GList *p;
   for (p = list->list; p; p = p->next) {
      Command_t *command = (Command_t*) p->data;
      if (command->sub_commands)
         command_list_execute(command->sub_commands);
      if (command->class->execute)
         (void) command->class->execute(command);
   }
}

void
command_execute(Command_t *command)
{
   if (command->locked) {
      command->locked = FALSE;
   } else {
      if (command->sub_commands)
         command_list_execute(command->sub_commands);
      if (command->class->execute) {
         CmdExecuteValue_t value = command->class->execute(command);
         if (value == CMD_APPEND)
            command_list_add(command);
         else if (value == CMD_DESTRUCT)
            command_destruct(command);
      }
   }
}

void
command_redo(Command_t *command)
{
   if (command->sub_commands)
      command_list_redo_all(command->sub_commands);
   if (command->class->redo)
      command->class->redo(command);
   else if (command->class->execute)
      (void) command->class->execute(command);
}

void
command_undo(Command_t *command)
{
   if (command->sub_commands)
      command_list_undo_all(command->sub_commands);
   if (command->class->undo)
      command->class->undo(command);
}

void
command_set_name(Command_t *command, const gchar *name)
{
   command->name = name;
   command_list_callback_call(&_command_list.update_cb, command);
}

void
command_list_undo(CommandList_t *list)
{
   Command_t *command = (Command_t*) list->undo->data;
   command_undo(command);

   list->redo = list->undo;
   list->undo = list->undo->prev;
   if (list->undo)
      command = (Command_t*) list->undo->data;
   else
      command = NULL;
   command_list_callback_call(&list->update_cb, command);
}

void
command_list_undo_all(CommandList_t *list)
{
   while (list->undo)
      command_list_undo(list);
}

void
last_command_undo(void)
{
   command_list_undo(&_command_list);
}

void
command_list_redo(CommandList_t *list)
{
   Command_t *command = (Command_t*) list->redo->data;
   command_redo(command);

   list->undo = list->redo;
   list->redo = list->redo->next;
   command_list_callback_call(&list->update_cb, command);
}

void
command_list_redo_all(CommandList_t *list)
{
   while (list->redo)
      command_list_redo(list);
}

void
last_command_redo(void)
{
   command_list_redo(&_command_list);
}

Command_t*
command_init(Command_t *command, const gchar *name, CommandClass_t *class)
{
   command->sub_commands = NULL;
   command->name = name;
   command->class = class;
   command->locked = FALSE;
   return command;
}

void
command_add_subcommand(Command_t *command, Command_t *sub_command)
{
   if (!command->sub_commands)
      command->sub_commands = command_list_new(INFINITE_UNDO_LEVELS);
   subcommand_list_add(command->sub_commands, sub_command);
}

static CmdExecuteValue_t basic_command_execute(Command_t *command);

static CommandClass_t basic_command_class = {
   NULL,                        /* basic_command_destruct */
   basic_command_execute,
   NULL,
   NULL                         /* basic_command_redo */
};

typedef struct {
   Command_t parent;
   void (*func)(void);
} BasicCommand_t;

Command_t*
command_new(void (*func)(void))
{
   BasicCommand_t *command = g_new(BasicCommand_t, 1);
   command->func = func;
   return command_init(&command->parent, "Unknown", &basic_command_class);
}

static CmdExecuteValue_t
basic_command_execute(Command_t *command)
{
   ((BasicCommand_t*) command)->func();
   return CMD_DESTRUCT;
}
