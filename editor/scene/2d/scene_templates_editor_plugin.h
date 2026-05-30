/**************************************************************************/
/*  scene_templates_editor_plugin.h                                       */
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

#pragma once

#include "editor/inspector/editor_inspector.h"
#include "scene/gui/box_container.h"

class Button;
class EditorInspectorCategory;
class HSeparator;
class Label;
class LineEdit;
class MarginContainer;
class Node;

// ── 单个模板条目控件 ─────────────────────────────

class SceneTemplateEntry : public HBoxContainer {
	GDCLASS(SceneTemplateEntry, HBoxContainer);

	Label *display_label = nullptr;
	LineEdit *name_edit = nullptr;
	String entry_id;
	bool _highlighted = false;
	bool _editing = false;

	void gui_input(const Ref<InputEvent> &p_event) override;
	void _on_text_submitted(const String &p_new_text);
	void _on_focus_exited();
	void _notification(int p_what);

protected:
	static void _bind_methods();

public:
	void set_entry_id(const String &p_id);
	String get_entry_id() const;
	void set_entry_name(const String &p_name);
	String get_entry_name() const;
	void set_highlighted(bool p_highlighted);

	SceneTemplateEntry();
};

// ── 场景模板编辑器面板 ──────────────────────────

class SceneTemplatesEditor : public VBoxContainer {
	GDCLASS(SceneTemplatesEditor, VBoxContainer);

	EditorInspectorCategory *category = nullptr;
	MarginContainer *content_margin = nullptr;
	VBoxContainer *content_vbox = nullptr;
	VBoxContainer *tmpl_list_vbox = nullptr;
	Button *add_button = nullptr;
	HSeparator *list_separator = nullptr;
	VBoxContainer *node_list_vbox = nullptr;
	Label *empty_label = nullptr;
	EditorInspectorActionButton *save_button = nullptr;
	Node *scene_root = nullptr;

	String _selected_tmpl_id;

	void _rebuild_node_list();
	void _rebuild_template_list();
	void _update_save_button_state();
	void _node_toggled(bool p_pressed, const String &p_node_path);
	void _save_pressed();
	void _on_template_selected(const String &p_tmpl_id);
	void _on_entry_name_changed(const String &p_tmpl_id, const String &p_new_name);
	void _on_add_template();

protected:
	static void _bind_methods();

public:
	void set_scene_root(Node *p_scene_root);

	SceneTemplatesEditor();
};

// ── Inspector 插件 ──────────────────────────────

class EditorInspectorPluginSceneTemplates : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginSceneTemplates, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_end(Object *p_object) override;
};
