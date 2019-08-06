#!/usr/bin/gjs

/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * hello-world.js
 * Copyright (C) Jehan
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

const System = imports.system

imports.gi.versions.Gimp = '3.0';
const Gimp = imports.gi.Gimp;
imports.gi.versions.Gtk = '3.0';
const Gtk = imports.gi.Gtk;

const GLib = imports.gi.GLib;
const GObject = imports.gi.GObject;

/* gjs's ARGV is not C-style. We must add the program name as first
 * value.
 */
ARGV.unshift(System.programInvocationName);

var MyPlugIn = GObject.registerClass({
    GTypeName: 'GimpMyPlugin',
}, class MyPlugIn extends Gimp.PlugIn {

    vfunc_query_procedures() {
        return ["javascript-fu-hello-world"];
    }

    vfunc_create_procedure(name) {
        let procedure = Gimp.Procedure.new(this, name, Gimp.PDBProcType.PLUGIN, this.run);
        procedure.set_menu_label("Hello World in GJS")
        procedure.set_documentation("Hello World in GJS",
                                    "Create a new image to demonstrate a javascript plug-in.",
                                    "")
        procedure.set_attribution("Jehan", "Jehan", "2019")
        /*procedure.add_menu_path ('<Image>/Filters/Hello World')*/

        return procedure;
    }

    run(procedure, args, data) {
        let image = Gimp.image_new(1000, 1000, Gimp.ImageBaseType.RGB);
        let layer = Gimp.layer_new(image, "Hello World",
                                   1000, 1000, Gimp.ImageType.RGB_IMAGE,
                                   100.0, Gimp.LayerMode.NORMAL);
        Gimp.image_insert_layer(image, layer, null, 0);
        Gimp.display_new(image);
        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, null)
    }
});

Gimp.main(MyPlugIn.$gtype, ARGV)
