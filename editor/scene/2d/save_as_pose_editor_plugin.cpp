/**************************************************************************/
/*  save_as_pose_editor_plugin.cpp                                        */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "save_as_pose_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/math/math_funcs.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/gui/editor_toaster.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/main/canvas_item.h"
#include "scene/scene_string_names.h"

namespace {

HashMap<ObjectID, HashMap<String, bool>> save_as_pose_selection_state;

Variant _pose_number(real_t p_value) {
	const real_t rounded_value = Math::round(p_value);
	if (Math::is_equal_approx(p_value, rounded_value)) {
		return int64_t(rounded_value);
	}

	return p_value;
}

Array _pose_vector2(const Vector2 &p_value) {
	Array values;
	values.push_back(_pose_number(p_value.x));
	values.push_back(_pose_number(p_value.y));
	return values;
}

Variant _pose_scale(const Size2 &p_scale) {
	return _pose_number(p_scale.x);
}

String _pose_node_key(const String &p_name) {
	String key = p_name.to_snake_case().to_lower();
	key = key.replace(" ", "_");

	String validated_key = key.validate_node_name().to_lower();
	if (!validated_key.is_empty()) {
		return validated_key;
	}

	return "node";
}

bool _is_node_selected(Node *p_scene_root, const String &p_node_path) {
	ERR_FAIL_NULL_V(p_scene_root, false);

	if (const HashMap<String, bool> *scene_selection = save_as_pose_selection_state.getptr(p_scene_root->get_instance_id())) {
		if (const bool *selected = scene_selection->getptr(p_node_path)) {
			return *selected;
		}
	}

	return true;
}

void _set_node_selected(Node *p_scene_root, const String &p_node_path, bool p_selected) {
	ERR_FAIL_NULL(p_scene_root);

	save_as_pose_selection_state[p_scene_root->get_instance_id()].insert(p_node_path, p_selected);
}

} // namespace

void SaveAsPoseEditor::_bind_methods() {
}

void SaveAsPoseEditor::_notification(int p_what) {
	if (p_what == NOTIFICATION_THEME_CHANGED && section) {
		section->set_bg_color(get_theme_color(SNAME("prop_subsection"), EditorStringName(Editor)));
	}
}

void SaveAsPoseEditor::set_scene_root(Node *p_scene_root) {
	scene_root = p_scene_root;
	section->setup("save_as_pose", TTR("SaveAsPose"), scene_root, Color(0.0f, 0.0f, 0.0f), true);
	section->unfold();
	section->set_bg_color(get_theme_color(SNAME("prop_subsection"), EditorStringName(Editor)));
	scene_root->connect(SNAME("child_order_changed"), callable_mp(this, &SaveAsPoseEditor::_rebuild_node_list), CONNECT_REFERENCE_COUNTED);
	_rebuild_node_list();
}

void SaveAsPoseEditor::_rebuild_node_list() {
	while (node_list_vbox->get_child_count() > 0) {
		memdelete(node_list_vbox->get_child(0));
	}

	bool has_canvas_item_child = false;
	if (scene_root) {
		const int child_count = scene_root->get_child_count(false);
		for (int i = 0; i < child_count; i++) {
			CanvasItem *canvas_item = Object::cast_to<CanvasItem>(scene_root->get_child(i, false));
			if (!canvas_item) {
				continue;
			}

			has_canvas_item_child = true;

			const String node_path = String(scene_root->get_path_to(canvas_item));
			CheckBox *checkbox = memnew(CheckBox);
			checkbox->set_text(String(canvas_item->get_name()));
			checkbox->set_pressed(_is_node_selected(scene_root, node_path));
			checkbox->connect(SceneStringName(toggled), callable_mp(this, &SaveAsPoseEditor::_node_toggled).bind(node_path));
			node_list_vbox->add_child(checkbox);
		}
	}

	if (!has_canvas_item_child) {
		empty_label = memnew(Label);
		empty_label->set_text(TTR("No CanvasItem children were found under the current scene root."));
		node_list_vbox->add_child(empty_label);
	} else {
		empty_label = nullptr;
	}

	_update_save_button_state();
}

void SaveAsPoseEditor::_update_save_button_state() {
	bool has_selected_node = false;

	if (scene_root) {
		const int child_count = scene_root->get_child_count(false);
		for (int i = 0; i < child_count; i++) {
			CanvasItem *canvas_item = Object::cast_to<CanvasItem>(scene_root->get_child(i, false));
			if (!canvas_item) {
				continue;
			}

			if (_is_node_selected(scene_root, String(scene_root->get_path_to(canvas_item)))) {
				has_selected_node = true;
				break;
			}
		}
	}

	save_button->set_disabled(!has_selected_node);
}

void SaveAsPoseEditor::_node_toggled(bool p_pressed, const String &p_node_path) {
	if (!scene_root) {
		return;
	}

	_set_node_selected(scene_root, p_node_path, p_pressed);
	_update_save_button_state();
}

void SaveAsPoseEditor::_save_pressed() {
	if (!scene_root) {
		EditorNode::get_singleton()->show_warning(TTR("There is no edited scene root to save."));
		return;
	}

	Dictionary pose_entry;
	HashMap<String, int> used_node_keys;

	const int child_count = scene_root->get_child_count(false);
	for (int i = 0; i < child_count; i++) {
		CanvasItem *canvas_item = Object::cast_to<CanvasItem>(scene_root->get_child(i, false));
		if (!canvas_item) {
			continue;
		}

		const String node_path = String(scene_root->get_path_to(canvas_item));
		if (!_is_node_selected(scene_root, node_path)) {
			continue;
		}

		String node_key = _pose_node_key(String(canvas_item->get_name()));
		if (int *existing_count = used_node_keys.getptr(node_key)) {
			(*existing_count)++;
			node_key += "_" + itos(*existing_count);
		} else {
			used_node_keys.insert(node_key, 1);
		}

		pose_entry[node_key + "_position"] = _pose_vector2(canvas_item->_edit_get_position());
		pose_entry[node_key + "_scale"] = _pose_scale(canvas_item->_edit_get_scale());
		pose_entry[node_key + "_rotation_degrees"] = _pose_number(Math::rad_to_deg(canvas_item->_edit_get_rotation()));
	}

	if (pose_entry.is_empty()) {
		EditorNode::get_singleton()->show_warning(TTR("Select at least one CanvasItem child to save a pose."));
		return;
	}

	const String pose_relative_path = "config/pose.json";
	const String pose_path = "res://" + pose_relative_path;
	const Error dir_error = DirAccess::make_dir_recursive_absolute(ProjectSettings::get_singleton()->globalize_path(pose_path.get_base_dir()));
	if (dir_error != OK) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("Could not create pose directory for \"%s\"."), pose_relative_path));
		return;
	}

	Dictionary poses;
	if (FileAccess::exists(pose_path)) {
		Error read_error = OK;
		const String file_text = FileAccess::get_file_as_string(pose_path, &read_error);
		if (read_error != OK) {
			EditorNode::get_singleton()->show_warning(vformat(TTR("Could not read pose file \"%s\"."), pose_relative_path));
			return;
		}

		if (!file_text.strip_edges().is_empty()) {
			const Variant parsed = JSON::parse_string(file_text);
			if (parsed.get_type() != Variant::DICTIONARY) {
				EditorNode::get_singleton()->show_warning(vformat(TTR("Pose file \"%s\" is not a valid JSON object."), pose_relative_path));
				return;
			}
			poses = parsed;
		}
	}

	int next_index = 1;
	for (const Variant &key : poses.get_key_list()) {
		const int index = String(key).to_int();
		if (index >= next_index) {
			next_index = index + 1;
		}
	}

	poses[itos(next_index)] = pose_entry;

	Error write_error = OK;
	Ref<FileAccess> file = FileAccess::open(pose_path, FileAccess::WRITE, &write_error);
	if (file.is_null()) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("Could not open pose file \"%s\" for writing."), pose_relative_path));
		return;
	}

	file->store_string(JSON::stringify(poses, "\t", false, true) + "\n");
	EditorToaster::get_singleton()->popup_str(vformat(TTR("Pose saved to %s."), pose_relative_path), EditorToaster::SEVERITY_INFO);
}

SaveAsPoseEditor::SaveAsPoseEditor() {
	section = memnew(EditorInspectorSection);
	add_child(section);

	node_list_vbox = memnew(VBoxContainer);
	section->get_vbox()->add_child(node_list_vbox);

	save_button = memnew(EditorInspectorActionButton(TTRC("Save"), SNAME("Save")));
	save_button->connect(SceneStringName(pressed), callable_mp(this, &SaveAsPoseEditor::_save_pressed));
	section->get_vbox()->add_child(save_button);
}

bool EditorInspectorPluginSaveAsPose::can_handle(Object *p_object) {
	return Object::cast_to<Node>(p_object) != nullptr;
}

void EditorInspectorPluginSaveAsPose::parse_end(Object *p_object) {
	Node *node = Object::cast_to<Node>(p_object);
	if (!node || node != EditorNode::get_singleton()->get_edited_scene()) {
		return;
	}

	SaveAsPoseEditor *save_as_pose_editor = memnew(SaveAsPoseEditor);
	save_as_pose_editor->set_scene_root(node);
	add_custom_control(save_as_pose_editor);
}
