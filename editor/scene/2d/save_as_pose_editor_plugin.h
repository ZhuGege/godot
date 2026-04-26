/**************************************************************************/
/*  save_as_pose_editor_plugin.h                                          */
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

class EditorInspectorCategory;
class Label;
class MarginContainer;
class Node;

class SaveAsPoseEditor : public VBoxContainer {
	GDCLASS(SaveAsPoseEditor, VBoxContainer);

	EditorInspectorCategory *category = nullptr;
	MarginContainer *content_margin = nullptr;
	VBoxContainer *content_vbox = nullptr;
	VBoxContainer *node_list_vbox = nullptr;
	Label *empty_label = nullptr;
	EditorInspectorActionButton *save_button = nullptr;
	Node *scene_root = nullptr;

	void _rebuild_node_list();
	void _update_save_button_state();
	void _node_toggled(bool p_pressed, const String &p_node_path);
	void _save_pressed();

protected:
	static void _bind_methods();

public:
	void set_scene_root(Node *p_scene_root);

	SaveAsPoseEditor();
};

class EditorInspectorPluginSaveAsPose : public EditorInspectorPlugin {
	GDCLASS(EditorInspectorPluginSaveAsPose, EditorInspectorPlugin);

public:
	virtual bool can_handle(Object *p_object) override;
	virtual void parse_end(Object *p_object) override;
};
