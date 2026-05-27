/**************************************************************************/
/*  scene_templates_editor_plugin.cpp                                     */
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

#include "scene_templates_editor_plugin.h"

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/json.h"
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/math/math_funcs.h"
#include "core/templates/hash_set.h"
#include "editor/themes/editor_scale.h"
#include "editor/editor_node.h"
#include "editor/gui/editor_toaster.h"
#include "scene/gui/check_box.h"
#include "scene/gui/label.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/separator.h"
#include "scene/main/canvas_item.h"
#include "scene/scene_string_names.h"

namespace {

HashMap<ObjectID, HashMap<String, bool>> scene_templates_selection_state;

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
	Array values;
	values.push_back(_pose_number(p_scale.x));
	values.push_back(_pose_number(p_scale.y));
	return values;
}

String _get_resource_path(const Variant &p_variant) {
	if (p_variant.get_type() != Variant::OBJECT) {
		return String();
	}

	Ref<Resource> resource = p_variant;
	if (resource.is_null()) {
		return String();
	}

	return resource->get_path();
}

String _get_canvas_item_texture_path(CanvasItem *p_canvas_item) {
	ERR_FAIL_NULL_V(p_canvas_item, String());

	bool valid = false;
	String texture_path = _get_resource_path(p_canvas_item->get(SNAME("texture"), &valid));
	if (valid && !texture_path.is_empty()) {
		return texture_path;
	}

	List<PropertyInfo> property_list;
	p_canvas_item->get_property_list(&property_list, true);

	for (const PropertyInfo &property : property_list) {
		if (property.type != Variant::OBJECT || property.hint != PROPERTY_HINT_RESOURCE_TYPE) {
			continue;
		}
		if (!property.hint_string.contains("Texture")) {
			continue;
		}

		const String property_name = property.name;
		if (property_name != "icon" && !property_name.contains("texture")) {
			continue;
		}
		if (property_name.begins_with("theme_override_")) {
			continue;
		}

		texture_path = _get_resource_path(p_canvas_item->get(property.name, &valid));
		if (valid && !texture_path.is_empty()) {
			return texture_path;
		}
	}

	return String();
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

	if (const HashMap<String, bool> *scene_selection = scene_templates_selection_state.getptr(p_scene_root->get_instance_id())) {
		if (const bool *selected = scene_selection->getptr(p_node_path)) {
			return *selected;
		}
	}

	return true;
}

void _set_node_selected(Node *p_scene_root, const String &p_node_path, bool p_selected) {
	ERR_FAIL_NULL(p_scene_root);

	scene_templates_selection_state[p_scene_root->get_instance_id()].insert(p_node_path, p_selected);
}

String _template_path(Node *p_scene_root) {
	ERR_FAIL_NULL_V(p_scene_root, String("res://config/SceneTemplates/untitled.json"));

	String scene_path = p_scene_root->get_scene_file_path();
	if (scene_path.is_empty()) {
		return "res://config/SceneTemplates/untitled.json";
	}

	String scene_name = scene_path.get_file().get_basename();
	if (scene_name.is_empty()) {
		scene_name = "untitled";
	}

	return "res://config/SceneTemplates/" + scene_name + ".json";
}

String _template_relative_path(Node *p_scene_root) {
	String full_path = _template_path(p_scene_root);
	return full_path.trim_prefix("res://");
}

Dictionary _load_templates(Node *p_scene_root) {
	const String tmpl_path = _template_path(p_scene_root);
	if (!FileAccess::exists(tmpl_path)) {
		return Dictionary();
	}

	Error err = OK;
	const String text = FileAccess::get_file_as_string(tmpl_path, &err);
	if (err != OK || text.strip_edges().is_empty()) {
		return Dictionary();
	}

	const Variant parsed = JSON::parse_string(text);
	if (parsed.get_type() != Variant::DICTIONARY) {
		return Dictionary();
	}

	return parsed;
}

Dictionary _load_landmark_names() {
	Dictionary mapping;
	const String path = "res://config/photo_mode.json";
	if (!FileAccess::exists(path)) {
		return mapping;
	}

	Error err = OK;
	const String text = FileAccess::get_file_as_string(path, &err);
	if (err != OK) {
		return mapping;
	}

	const Variant parsed = JSON::parse_string(text);
	if (parsed.get_type() != Variant::DICTIONARY) {
		return mapping;
	}

	Dictionary config = parsed;
	Dictionary landmarks = config.get("landmarks", Dictionary());
	for (const Variant &key : landmarks.get_key_list()) {
		const String lm_name = key;
		Dictionary lm_data = landmarks[key];
		const String pid = lm_data.get("pose_id", Variant());
		if (!pid.is_empty()) {
			mapping[pid] = lm_name;
		}
	}
	return mapping;
}

void _extract_pose_prefixes(const Dictionary &p_pose_data, HashSet<String> &r_prefixes) {
	for (const Variant &key_var : p_pose_data.get_key_list()) {
		String key = key_var;

		if (key.ends_with("_rotation_degrees")) {
			r_prefixes.insert(key.substr(0, key.length() - 17));
		} else if (key.ends_with("_texture_path")) {
			r_prefixes.insert(key.substr(0, key.length() - 13));
		} else if (key.ends_with("_position")) {
			r_prefixes.insert(key.substr(0, key.length() - 9));
		} else if (key.ends_with("_scale")) {
			r_prefixes.insert(key.substr(0, key.length() - 6));
		}
	}
}

} // namespace

void SceneTemplatesEditor::_bind_methods() {
}

void SceneTemplatesEditor::set_scene_root(Node *p_scene_root) {
	scene_root = p_scene_root;
	category->set_property_info(PropertyInfo(Variant::NIL, "SceneTemplates"));
	category->set_doc_class_name("CanvasItem");
	category->set_tooltip_text("property|CanvasItem|scene_templates");
	scene_root->connect(SNAME("child_order_changed"), callable_mp(this, &SceneTemplatesEditor::_rebuild_node_list), CONNECT_REFERENCE_COUNTED);
	_rebuild_node_list();
}

void SceneTemplatesEditor::_rebuild_node_list() {
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
			checkbox->connect(SceneStringName(toggled), callable_mp(this, &SceneTemplatesEditor::_node_toggled).bind(node_path));
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
	_rebuild_template_buttons();
}

void SceneTemplatesEditor::_update_save_button_state() {
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

void SceneTemplatesEditor::_node_toggled(bool p_pressed, const String &p_node_path) {
	if (!scene_root) {
		return;
	}

	_set_node_selected(scene_root, p_node_path, p_pressed);
	_update_save_button_state();
}

void SceneTemplatesEditor::_save_pressed() {
	if (!scene_root) {
		EditorNode::get_singleton()->show_warning(TTR("There is no edited scene root to save."));
		return;
	}

	Dictionary tmpl_entry;
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

		tmpl_entry[node_key + "_position"] = _pose_vector2(canvas_item->_edit_get_position());
		tmpl_entry[node_key + "_scale"] = _pose_scale(canvas_item->_edit_get_scale());
		tmpl_entry[node_key + "_rotation_degrees"] = _pose_number(Math::rad_to_deg(canvas_item->_edit_get_rotation()));
		tmpl_entry[node_key + "_texture_path"] = _get_canvas_item_texture_path(canvas_item);
	}

	if (tmpl_entry.is_empty()) {
		EditorNode::get_singleton()->show_warning(TTR("Select at least one CanvasItem child to save a template."));
		return;
	}

	const String tmpl_path = _template_path(scene_root);
	const String rel_path = _template_relative_path(scene_root);
	const Error dir_error = DirAccess::make_dir_recursive_absolute(ProjectSettings::get_singleton()->globalize_path(tmpl_path.get_base_dir()));
	if (dir_error != OK) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("Could not create directory for \"%s\"."), rel_path));
		return;
	}

	Dictionary templates;
	if (FileAccess::exists(tmpl_path)) {
		Error read_error = OK;
		const String file_text = FileAccess::get_file_as_string(tmpl_path, &read_error);
		if (read_error != OK) {
			EditorNode::get_singleton()->show_warning(vformat(TTR("Could not read template file \"%s\"."), rel_path));
			return;
		}

		if (!file_text.strip_edges().is_empty()) {
			const Variant parsed = JSON::parse_string(file_text);
			if (parsed.get_type() != Variant::DICTIONARY) {
				EditorNode::get_singleton()->show_warning(vformat(TTR("Template file \"%s\" is not a valid JSON object."), rel_path));
				return;
			}
			templates = parsed;
		}
	}

	int next_index = 1;
	for (const Variant &key : templates.get_key_list()) {
		const int index = String(key).to_int();
		if (index >= next_index) {
			next_index = index + 1;
		}
	}

	templates[itos(next_index)] = tmpl_entry;

	Error write_error = OK;
	Ref<FileAccess> file = FileAccess::open(tmpl_path, FileAccess::WRITE, &write_error);
	if (file.is_null()) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("Could not open template file \"%s\" for writing."), rel_path));
		return;
	}

	file->store_string(JSON::stringify(templates, "\t", false, true) + "\n");
	file.unref(); // 确保写入并关闭，_rebuild_template_buttons 才能读到最新数据

	EditorToaster::get_singleton()->popup_str(vformat(TTR("Scene template saved to %s."), rel_path), EditorToaster::SEVERITY_INFO);

	_rebuild_template_buttons();
}

void SceneTemplatesEditor::_rebuild_template_buttons() {
	while (tmpl_buttons_vbox->get_child_count() > 0) {
		memdelete(tmpl_buttons_vbox->get_child(0));
	}

	Dictionary templates = _load_templates(scene_root);
	Dictionary landmark_names = _load_landmark_names();

	if (templates.is_empty()) {
		tmpl_separator->hide();
		tmpl_buttons_vbox->hide();
		return;
	}

	List<int> sorted_ids;
	for (const Variant &key : templates.get_key_list()) {
		sorted_ids.push_back(String(key).to_int());
	}
	sorted_ids.sort();

	for (int id : sorted_ids) {
		const String tmpl_id = itos(id);

		Button *btn = memnew(Button);
		String btn_text = "Template " + tmpl_id;
		if (landmark_names.has(tmpl_id)) {
			btn_text += "  " + String(landmark_names[tmpl_id]);
		}
		btn->set_text(btn_text);
		btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		btn->set_h_size_flags(SIZE_FILL);
		btn->connect(SceneStringName(pressed), callable_mp(this, &SceneTemplatesEditor::_template_selected).bind(tmpl_id));
		tmpl_buttons_vbox->add_child(btn);
	}

	tmpl_separator->show();
	tmpl_buttons_vbox->show();
}

void SceneTemplatesEditor::_template_selected(const String &p_tmpl_id) {
	Dictionary templates = _load_templates(scene_root);
	if (!templates.has(p_tmpl_id)) {
		EditorToaster::get_singleton()->popup_str(vformat(TTR("Template %s not found."), p_tmpl_id), EditorToaster::SEVERITY_WARNING);
		return;
	}

	Dictionary tmpl_data = templates[p_tmpl_id];
	if (!scene_root) {
		return;
	}

	HashSet<String> prefixes;
	_extract_pose_prefixes(tmpl_data, prefixes);

	const int child_count = scene_root->get_child_count(false);
	for (int i = 0; i < child_count; i++) {
		CanvasItem *canvas_item = Object::cast_to<CanvasItem>(scene_root->get_child(i, false));
		if (!canvas_item) {
			continue;
		}

		const String node_key = _pose_node_key(String(canvas_item->get_name()));
		if (!prefixes.has(node_key)) {
			continue;
		}

		const Variant pos_var = tmpl_data.get(node_key + "_position", Variant());
		if (pos_var.get_type() == Variant::ARRAY) {
			Array pos_arr = pos_var;
			if (pos_arr.size() >= 2) {
				canvas_item->_edit_set_position(Vector2(pos_arr[0], pos_arr[1]));
			}
		}

		const Variant scale_var = tmpl_data.get(node_key + "_scale", Variant());
		if (scale_var.get_type() == Variant::ARRAY) {
			Array scale_arr = scale_var;
			if (scale_arr.size() >= 2) {
				canvas_item->_edit_set_scale(Vector2(scale_arr[0], scale_arr[1]));
			}
		}

		if (tmpl_data.has(node_key + "_rotation_degrees")) {
			real_t new_rot = tmpl_data[node_key + "_rotation_degrees"];
			canvas_item->_edit_set_rotation(Math::deg_to_rad(new_rot));
		}

		if (tmpl_data.has(node_key + "_texture_path")) {
			const String tex_path = tmpl_data[node_key + "_texture_path"];
			if (!tex_path.is_empty() && ResourceLoader::exists(tex_path)) {
				Ref<Texture2D> tex = ResourceLoader::load(tex_path);
				if (tex.is_valid()) {
					canvas_item->set(SNAME("texture"), tex);
				}
			}
		}
	}

	Dictionary landmark_names = _load_landmark_names();
	String msg = vformat("Loaded Template %s", p_tmpl_id);
	if (landmark_names.has(p_tmpl_id)) {
		msg += " (" + String(landmark_names[p_tmpl_id]) + ")";
	}
	EditorToaster::get_singleton()->popup_str(msg, EditorToaster::SEVERITY_INFO);
}

SceneTemplatesEditor::SceneTemplatesEditor() {
	category = memnew(EditorInspectorCategory);
	add_child(category);

	content_margin = memnew(MarginContainer);
	content_margin->add_theme_constant_override(SNAME("margin_left"), Math::round(14 * EDSCALE));
	content_margin->add_theme_constant_override(SNAME("margin_right"), Math::round(4 * EDSCALE));
	content_margin->add_theme_constant_override(SNAME("margin_bottom"), Math::round(8 * EDSCALE));
	add_child(content_margin);

	content_vbox = memnew(VBoxContainer);
	content_vbox->set_theme_type_variation(SNAME("EditorPropertyContainer"));
	content_vbox->add_theme_constant_override(SNAME("separation"), Math::round(4 * EDSCALE));
	content_margin->add_child(content_vbox);

	tmpl_buttons_vbox = memnew(VBoxContainer);
	tmpl_buttons_vbox->add_theme_constant_override(SNAME("separation"), Math::round(2 * EDSCALE));
	content_vbox->add_child(tmpl_buttons_vbox);

	tmpl_separator = memnew(HSeparator);
	content_vbox->add_child(tmpl_separator);

	node_list_vbox = memnew(VBoxContainer);
	content_vbox->add_child(node_list_vbox);

	save_button = memnew(EditorInspectorActionButton(TTRC("Save"), SNAME("Save")));
	save_button->connect(SceneStringName(pressed), callable_mp(this, &SceneTemplatesEditor::_save_pressed));
	content_vbox->add_child(save_button);
}

bool EditorInspectorPluginSceneTemplates::can_handle(Object *p_object) {
	return Object::cast_to<Node>(p_object) != nullptr;
}

void EditorInspectorPluginSceneTemplates::parse_end(Object *p_object) {
	Node *node = Object::cast_to<Node>(p_object);
	if (!node || node != EditorNode::get_singleton()->get_edited_scene()) {
		return;
	}

	SceneTemplatesEditor *editor = memnew(SceneTemplatesEditor);
	editor->set_scene_root(node);
	add_custom_control(editor);
}
