#!/usr/bin/env gjs

/* LIGMA - The GNU Image Manipulation Program
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

imports.gi.versions.Ligma = '3.0';
const Ligma = imports.gi.Ligma;
imports.gi.versions.LigmaUi = '3.0';
const LigmaUi = imports.gi.LigmaUi;
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

let url = "https://gitlab.gnome.org/GNOME/ligma/blob/master/extensions/goat-exercises/goat-exercise-gjs.js";

function _(message) { return GLib.dgettext(null, message); }

var Goat = GObject.registerClass({
    GTypeName: 'Goat',
}, class Goat extends Ligma.PlugIn {

    vfunc_query_procedures() {
        return ["plug-in-goat-exercise-gjs"];
    }

    vfunc_create_procedure(name) {
        let procedure = Ligma.ImageProcedure.new(this, name, Ligma.PDBProcType.PLUGIN, this.run);

        procedure.set_image_types("*");
        procedure.set_sensitivity_mask(Ligma.ProcedureSensitivityMask.DRAWABLE);

        procedure.set_menu_label(_("Exercise a JavaScript goat"));
        procedure.set_icon_name(LigmaUi.ICON_GEGL);
        procedure.add_menu_path ('<Image>/Filters/Development/Goat exercises/');

        procedure.set_documentation(_("Exercise a goat in the JavaScript language (GJS)"),
                                    _("Takes a goat for a walk in Javascript with the GJS interpreter"),
                                    name);
        procedure.set_attribution("Jehan", "Jehan", "2019");

        return procedure;
    }

    run(procedure, run_mode, image, drawables, args, run_data) {
        /* TODO: localization. */

        if (drawables.length != 1) {
            let msg = `Procedure '${procedure.get_name()}' only works with one drawable.`;
            let error = GLib.Error.new_literal(Ligma.PlugIn.error_quark(), 0, msg);
            return procedure.new_return_values(Ligma.PDBStatusType.CALLING_ERROR, error)
        }

        let drawable = drawables[0];

        if (run_mode == Ligma.RunMode.INTERACTIVE) {
            LigmaUi.init("goat-exercise-gjs");
            /* TODO: help function and ID. */
            let dialog = new LigmaUi.Dialog({
              title: _("Exercise a goat (JavaScript)"),
              role: "goat-exercise-JavaScript",
              use_header_bar: true,
            });
            dialog.add_button(_("_Cancel"), Gtk.ResponseType.CANCEL);
            dialog.add_button(_("_Source"), Gtk.ResponseType.APPLY);
            dialog.add_button(_("_OK"), Gtk.ResponseType.OK);

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

            let contents = imports.byteArray.toString(GLib.file_get_contents(System.programInvocationName)[1]);
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
                    return procedure.new_return_values(Ligma.PDBStatusType.CANCEL, null)
                }
            }
        }

        let [ intersect, x, y, width, height ] = drawable.mask_intersect();
        if (intersect) {
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
            Ligma.displays_flush();
        }

        return procedure.new_return_values(Ligma.PDBStatusType.SUCCESS, null);
    }
});

Ligma.main(Goat.$gtype, ARGV);
