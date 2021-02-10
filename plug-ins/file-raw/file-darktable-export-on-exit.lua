--[[
  This file is part of GIMP,
  copyright (c) 2015-2017 Tobias Ellinghaus

  GIMP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  GIMP is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GIMP.  If not, see <https://www.gnu.org/licenses/>.
]]

--[[
EXPORT ON EXIT
exports all (but at most 1) images from the database to prefs setting "lua/export_on_exit/export_filename"
when darktable exits


USAGE
* require this file from your main lua config file
* or: use --luacmd "dofile('/path/to/this/file.lua')"
* and make sure to set the export filename

]]

local dt = require "darktable"

local orig_register_event = dt.register_event

function dt.register_event(name, event, func, label)
  if dt.configuration.api_version_string >= "6.2.1" then
    if label then
      orig_register_event(name, event, func, label)
    else
      orig_register_event(name, event, func)
    end
  else
    if label then
      orig_register_event(event, func, label)
    else
      orig_register_event(event, func)
    end
  end
end

local min_api_version = "2.1.0"
if dt.configuration.api_version_string < min_api_version then
  dt.print("the exit export script requires at least darktable version 1.7.0")
  dt.print_error("the exit export script requires at least darktable version 1.7.0")
  return
else
  dt.print("closing darktable will export the image and make GIMP load it")
end

local CURR_API_STRING = dt.configuration.api_version_string

local export_filename = dt.preferences.read("export_on_exit", "export_filename", "string")

dt.register_event("fileraw", "exit", function()
  -- safegurad against someone using this with their library containing 50k images
  if #dt.database > 1 then
    dt.print_error("too many images, only exporting the first")
--     return
  end

  -- change the view first to force writing of the history stack
  dt.gui.current_view(dt.gui.views.lighttable)
  -- now export
  local format = dt.new_format("exr")
  format.max_width = 0
  format.max_height = 0
  -- let's have the export in a loop so we could easily support > 1 images
  for _, image in ipairs(dt.database) do
    dt.print_error("exporting `"..tostring(image).."' to `"..export_filename.."'")
    format:write_image(image, export_filename)
    break -- only export one image. see above for the reason
  end
end)

--
-- vim: shiftwidth=2 expandtab tabstop=2 cindent syntax=lua
-- kate: hl Lua;
