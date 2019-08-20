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
imports.gi.versions.Gegl = '0.4';
const Gegl = imports.gi.Gegl;
imports.gi.versions.Gtk = '3.0';
const Gtk = imports.gi.Gtk;
imports.gi.versions.Gdk = '3.0';
const Gdk = imports.gi.Gdk;

const GLib = imports.gi.GLib;
const GObject = imports.gi.GObject;
const Gio = imports.gi.Gio;

/* gjs's ARGV is not C-style. We must add the program name as first
 * value.
 */
ARGV.unshift(System.programInvocationName);

let url = "https://gitlab.gnome.org/GNOME/gimp/blob/master/plug-ins/goat-exercises/goat-exercise-gjs.js";

var Goat = GObject.registerClass({
    GTypeName: 'Goat',
}, class Goat extends Gimp.PlugIn {

    vfunc_query_procedures() {
        return ["plug-in-goat-exercise-gjs"];
    }

    vfunc_create_procedure(name) {
        let procedure = Gimp.Procedure.new(this, name, Gimp.PDBProcType.PLUGIN, this.run);

        procedure.set_image_types("RGB*, INDEXED*, GRAY*");
        procedure.set_menu_label("Exercise a JavaScript goat");
        procedure.set_documentation("Exercise a goat in the JavaScript language (GJS)",
                                    "Takes a goat for a walk in Javascript with the GJS interpreter",
                                    "");
        procedure.add_menu_path ('<Image>/Filters/Development/Goat exercises/');
        procedure.set_attribution("Jehan", "Jehan", "2019");
        procedure.set_icon_name(Gimp.ICON_GEGL);


        procedure.add_argument(GObject.param_spec_enum("run-mode",
                                                       "Run mode",
                                                       "The run mode",
                                                       Gimp.RunMode.$gtype,
                                                       Gimp.RunMode.NONINTERACTIVE,
                                                       GObject.ParamFlags.READWRITE));
        procedure.add_argument(GObject.param_spec_object ("image",
                                                          "Image",
                                                          "The input image",
                                                          Gimp.Image.$gtype,
                                                          GObject.ParamFlags.READWRITE));
        procedure.add_argument(GObject.param_spec_object ("drawable",
                                                          "Drawable",
                                                          "The input drawable",
                                                          Gimp.Drawable.$gtype,
                                                          GObject.ParamFlags.READWRITE));

        return procedure;
    }

    run(procedure, args, data) {
        Gimp.get_plug_in().extension_enable();

        /* TODO: localization. */
        let run_mode = args.index(0);
        let image    = args.index(1);

        image.connect("destroyed", (image) => {
            Gimp.quit();
        });

        if (run_mode == Gimp.RunMode.INTERACTIVE) {
            Gimp.ui_init("goat-exercise-gjs", false);
            /* TODO: help function and ID. */
            let dialog = new Gimp.Dialog({
              title: "Exercise a goat (JavaScript)",
              role: "goat-exercise-JavaScript",
              use_header_bar: true,
            });
            dialog.add_button("_Cancel", Gtk.ResponseType.CANCEL);
            dialog.add_button("_Source", Gtk.ResponseType.APPLY);
            dialog.add_button("_OK", Gtk.ResponseType.OK);

            let geometry = new Gdk.Geometry();
            geometry.min_aspect = 0.5;
            geometry.max_aspect = 1.0;
            dialog.set_geometry_hints (null, geometry, Gdk.WindowHints.ASPECT);

            let box = new Gtk.Box({
              orientation: Gtk.Orientation.VERTICAL,
              spacing: 2
            });
            dialog.get_content_area().add(box);
            box.show();

            let lang = "JavaScript (GJS)";
            /* XXX Can't we have nicer-looking multiline strings and
             * also in printf format like in C for sharing the same
             * string in localization?
             */
            let head_text = `This plug-in is an exercise in '${lang}' to demo plug-in creation.\n` +
                            `Check out the last version of the source code online by clicking the \"Source\" button.`;

            let label = new Gtk.Label({label:head_text});
            box.pack_start(label, false, false, 1);
            label.show();

            let contents = String(GLib.file_get_contents(System.programInvocationName)[1]);
            if (contents) {
                let scrolled = new Gtk.ScrolledWindow();
                scrolled.set_vexpand (true);
                box.pack_start(scrolled, true, true, 1);
                scrolled.show();

                let view = new Gtk.TextView();
                view.set_wrap_mode(Gtk.WrapMode.WORD);
                view.set_editable(false);
                let buffer = view.get_buffer();
                buffer.set_text(contents, -1);
                scrolled.add(view);
                view.show();
            }

            while (true) {
                let response = dialog.run();

                if (response == Gtk.ResponseType.OK) {
                    dialog.destroy();
                    break;
                }
                else if (response == Gtk.ResponseType.APPLY) {
                    Gio.app_info_launch_default_for_uri(url, null);
                    continue;
                }
                else { /* CANCEL, CLOSE, DELETE_EVENT */
                    dialog.destroy();
                    return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, null)
                }
            }
        }

        let drawable = args.index(2);
        let [ success, x, y, width, height ] = drawable.mask_intersect();
        if (success) {
            Gegl.init(null);

            let buffer = drawable.get_buffer();
            let shadow_buffer = drawable.get_shadow_buffer();

            let graph = new Gegl.Node();
            let input = graph.create_child("gegl:buffer-source");
            input.set_property("buffer", buffer);
            let invert = graph.create_child("gegl:invert");
            let output = graph.create_child("gegl:write-buffer");
            output.set_property("buffer", shadow_buffer);
            input.link(invert);
            invert.link(output);
            output.process();

            /* This is extremely important in bindings, since we don't
             * unref buffers. If we don't explicitly flush a buffer, we
             * may left hanging forever. This step is usually done
             * during an unref().
             */
            shadow_buffer.flush();

            drawable.merge_shadow(true);
            drawable.update(x, y, width, height);
            Gimp.displays_flush();
        }
        else {
            let error = GLib.Error.new_literal(GLib.quark_from_string("goat-error-quark"), 0,
                                               "No pixels to process in the selected area.");
            return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR, error);
        }

        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, null);
    }
});

Gimp.main(Goat.$gtype, ARGV);
