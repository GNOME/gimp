#!/usr/bin/env lua

-- GIMP - The GNU Image Manipulation Program
-- Copyright (C) 1995 Spencer Kimball and Peter Mattis
--
-- goat-exercise-lua.lua
-- Copyright (C) Jehan
--
-- This program is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <https://www.gnu.org/licenses/>.

local lgi = require 'lgi'

local GLib    = lgi.GLib
local GObject = lgi.GObject
local Gio     = lgi.Gio
local Gegl    = lgi.Gegl
local Gimp    = lgi.Gimp
local GimpUi  = lgi.GimpUi
local Gtk     = lgi.Gtk
local Gdk     = lgi.Gdk

local Goat = lgi.package 'Goat'
local Goat = lgi.Goat

function N_(message)
  return message;
end

function _(message)
  return GLib.dgettext(nil, message);
end

function run(procedure, run_mode, image, drawables, config, run_data)
  -- procedure:new_return_values() crashes LGI so we construct the
  -- GimpValueArray manually.
  local retval = Gimp.ValueArray(1)

  if table.getn(drawables) ~= 1 then
    local calling_err = GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.CALLING_ERROR)
    local msg         = "Procedure '%s' only works with one drawable."

    msg = string.format(msg, procedure:get_name())

    retval:append(calling_err)
    retval:append(GObject.Value(GObject.Type.STRING, msg))
    return retval
  end

  local drawable = drawables[1]

  -- Not sure why run_mode has become a string instead of testing
  -- against Gimp.RunMode.INTERACTIVE.
  if run_mode == "INTERACTIVE" then
    GimpUi.init("goat-exercise-lua");
    local dialog = GimpUi.Dialog {
      title          = _("Plug-In Example in Lua"),
      role           = "goat-exercise-Lua",
      use_header_bar = 1
    }
    dialog:add_button(_("_Cancel"), Gtk.ResponseType.CANCEL);
    dialog:add_button(_("_Source"), Gtk.ResponseType.APPLY);
    dialog:add_button(_("_OK"), Gtk.ResponseType.OK);

    local geometry = Gdk.Geometry()
    geometry.min_aspect = 0.5;
    geometry.max_aspect = 1.0;
    dialog:set_geometry_hints (nil, geometry, Gdk.WindowHints.ASPECT);

    local box = Gtk.Box {
      orientation = Gtk.Orientation.VERTICAL,
      spacing     = 2
    }
    dialog:get_content_area():add(box)
    box:show()

    local lang = "Lua"
    local head_text = _("This plug-in is an exercise in '%s' to demo plug-in creation.\n" ..
                      "Check out the last version of the source code online by clicking the \"Source\" button.")

    local label = Gtk.Label { label = string.format(head_text, lang) }
    box:pack_start(label, false, false, 1)
    label:show()

    local contents = GLib.file_get_contents(arg[0])
    if (contents) then
      local scrolled = Gtk.ScrolledWindow()
      scrolled:set_vexpand (true)
      box:pack_start(scrolled, true, true, 1)
      scrolled:show()

      local view = Gtk.TextView()
      view:set_wrap_mode(Gtk.WrapMode.WORD)
      view:set_editable(false)
      local buffer = view:get_buffer()
      buffer:set_text(contents, -1)
      scrolled:add(view)
      view:show()
    end

    while (true) do
      local response = dialog:run()
      local url = 'https://gitlab.gnome.org/GNOME/gimp/blob/master/extensions/goat-exercises/goat-exercise-lua.lua'

      if response == Gtk.ResponseType.OK then
        dialog:destroy()
        break
      elseif (response == Gtk.ResponseType.APPLY)  then
        Gio.app_info_launch_default_for_uri(url, nil);
      else -- CANCEL, CLOSE, DELETE_EVENT
        dialog:destroy()
        local cancel = GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.CANCEL)
        retval:append(cancel)
        return retval
      end
    end
  end

  local x, y, width, height = drawable:mask_intersect()
  if width ~= nill and height ~= nil and width > 0 and height > 0 then
    Gegl.init(nil)

    local buffer = drawable:get_buffer()
    local shadow_buffer = drawable:get_shadow_buffer()

    local graph = Gegl.Node()
    local input = graph:create_child("gegl:buffer-source")
    input:set_property("buffer", GObject.Value(Gegl.Buffer, buffer))
    local invert = graph:create_child("gegl:invert")
    local output = graph:create_child("gegl:write-buffer")
    output:set_property("buffer", GObject.Value(Gegl.Buffer, shadow_buffer))
    input:link(invert)
    invert:link(output)
    output:process()

    shadow_buffer:flush()

    drawable:merge_shadow(true)
    drawable:update(x, y, width, height)
    Gimp.displays_flush()
  end

  local success = GObject.Value(Gimp.PDBStatusType, Gimp.PDBStatusType.SUCCESS)
  retval:append(success)
  return retval
end

Goat:class('Exercise', Gimp.PlugIn)

function Goat.Exercise:do_query_procedures()
  return { 'plug-in-goat-exercise-lua' }
end

function Goat.Exercise:do_create_procedure(name)
  local procedure = Gimp.ImageProcedure.new(self, name,
                                            Gimp.PDBProcType.PLUGIN,
                                            run, nil)

  procedure:set_image_types("*");
  procedure:set_sensitivity_mask(Gimp.ProcedureSensitivityMask.DRAWABLE);

  procedure:set_menu_label(_("Plug-In Example in _Lua"));
  procedure:set_icon_name(GimpUi.ICON_GEGL);
  procedure:add_menu_path('<Image>/Filters/Development/Plug-In Examples/');

  procedure:set_documentation(_("Plug-in example in Lua"),
                              _("Plug-in example in Lua"),
                              name);
  procedure:set_attribution("Jehan", "Jehan", "2019");

  return procedure
end

-- 'arg' is a Lua table. When automatically converted to an array, the
-- value 0 is deleted (because Lua arrays start at 1!), which breaks
-- Gimp.main() call. So let's create our own array starting at 1.
argv = {}
for k, v in pairs(arg) do
  argv[k+1] = v
end

Gimp.main(GObject.Type.name(Goat.Exercise), argv)
