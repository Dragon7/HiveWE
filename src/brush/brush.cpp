#include "brush.h"

//#include "Globals.h"
#include <glad/glad.h>

#include <map_global.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

import Camera;
import OpenGLUtilities;

Brush::Brush() {
	glCreateTextures(GL_TEXTURE_2D, 1, &brush_texture);
	glTextureParameteri(brush_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(brush_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(brush_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(brush_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	set_size(size);

	selection_shader = resource_manager.load<Shader>({ "Data/Shaders/selection.vs", "Data/Shaders/selection.fs" });
	selection_circle_shader = resource_manager.load<Shader>({ "Data/Shaders/selection_circle.vs", "Data/Shaders/selection_circle.fs" });
	brush_shader = resource_manager.load<Shader>({ "Data/Shaders/brush.vs", "Data/Shaders/brush.fs" });
}

void Brush::set_position(const glm::vec2& new_position) {
	const glm::vec2 center_position = new_position - size / 2.f * 0.25f + brush_offset;

	position = glm::floor(center_position);
	if (!uv_offset_locked) {
		glm::vec2 decimals = center_position - glm::vec2(position);

		switch (uv_offset_granularity) {
			case 1:
				uv_offset = { 0.f, 0.f };
				break;
			case 2:
				decimals *= 2.f;
				decimals = glm::floor(decimals);
				decimals *= 2.f;
				uv_offset = glm::abs(decimals);
				break;
			case 4:
				uv_offset = glm::abs(decimals * 4.f);
				break;
		}
	}
}

glm::vec2 Brush::get_position() const {
	return glm::vec2(position);
}

void Brush::set_size(const int new_size) {
	glm::vec2 center = glm::vec2(position) + size / 8.f + glm::vec2(uv_offset) * 0.25f;

	size = std::clamp(new_size * size_granularity, 1, 999);

	brush.clear();
	brush.resize(size * size, { 0, 0, 0, 0 });

	set_shape(shape);

	set_position(center);
}

void Brush::set_shape(const Shape new_shape) {
	shape = new_shape;

	for (int i = 0; i < size; i++) {
		for (int j = 0; j < size; j++) {
			if (contains(i / size_granularity, j / size_granularity)) {
				brush[j * size + i] = brush_color;
			} else {
				brush[j * size + i] = { 0, 0, 0, 0 };
			}
		}
	}

	glBindTexture(GL_TEXTURE_2D, brush_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_BGRA, GL_UNSIGNED_BYTE, brush.data());
}

/// Whether the brush shape contains the point, Arguments in brush coordinates
bool Brush::contains(const int x, const int y) const {
	switch (shape) {
		case Shape::square:
			return true;
		case Shape::circle: {
			int half_size = (size / 2) / size_granularity;
			int distance = (x - half_size) * (x - half_size);
			distance += (y - half_size) * (y - half_size);
			return distance <= half_size * half_size;
		}
		case Shape::diamond:
			int half_size = (size / 2) / size_granularity;

			return std::abs(x - half_size) + std::abs(y - half_size) <= half_size;
	}
	return true;
}

void Brush::increase_size(const int new_size) {
	set_size(size / size_granularity + new_size);
}
void Brush::decrease_size(const int new_size) {
	set_size(size / size_granularity - new_size);
}

void Brush::switch_mode() {
	if (mode != Mode::placement && mode != Mode::selection) {
		mode = return_mode;
	}
	mode = (mode == Mode::placement) ? Mode::selection : Mode::placement;

	clear_selection();
	selection_started = false;
}

void Brush::key_press_event(QKeyEvent* event) {
	switch (event->key()) {
		case Qt::Key_Escape:
			clear_selection();
			break;
		case Qt::Key_Equal:
			increase_size(2);
			break;
		case Qt::Key_Minus:
			decrease_size(2);
			break;
		case Qt::Key_Delete:
			delete_selection();
			break;
		case Qt::Key_X:
			if (event->modifiers() & Qt::ControlModifier) {
				cut_selection();
			}
			break;
		case Qt::Key_C:
			if (event->modifiers() & Qt::ControlModifier) {
				copy_selection();
			}
			break;
		case Qt::Key_V:
			if (event->modifiers() & Qt::ControlModifier) {
				return_mode = mode;
				mode = Mode::pasting;
			}
			break;
	}
}

void Brush::mouse_move_event(QMouseEvent* event, double frame_delta) {
	set_position(input_handler.mouse_world);

	if (event->buttons() == Qt::LeftButton) {
		if (mode == Mode::placement && (can_place() || event->modifiers() & Qt::ShiftModifier)) {
			apply(frame_delta);
		}
	}
}

void Brush::mouse_press_event(QMouseEvent* event, double frame_delta) {
	if (event->button() != Qt::LeftButton) {
		return;
	}

	if (!event->modifiers()) {
		clear_selection();
	}

	if (mode == Mode::selection && !(event->modifiers() & Qt::ControlModifier)) {
		if (!selection_started) {
			selection_started = true;
			selection_start = input_handler.mouse_world;
		}
	} else if (mode == Mode::placement) {
		// Check if ellegible for placement
		if (event->button() == Qt::LeftButton) {
			apply_begin();
			if (can_place() || event->modifiers() & Qt::ShiftModifier) {
				apply(0.5);
			}
		}
	} else if (mode == Mode::pasting && (can_place() || event->modifiers() & Qt::ShiftModifier)) {
		clear_selection();
		place_clipboard();
		mode = Mode::selection;
	}
}

void Brush::mouse_release_event(QMouseEvent* event) {
	if (mode == Mode::selection) {
		selection_started = false;
	} else if (mode == Mode::placement) {
		if (event->button() == Qt::LeftButton) {
			apply_end();
		}
	}
}

void Brush::render() {
	if (mode == Mode::selection) {
		render_selector();
	} 
	if (mode == Mode::placement) {
		render_brush();
	}
	if (mode == Mode::pasting) {
		render_clipboard();
	}
	render_selection();
}

void Brush::render_selector() const {
	if (selection_started) {
		glDisable(GL_DEPTH_TEST);

		selection_shader->use();

		glm::mat4 model(1.f);
		model = glm::translate(model, glm::vec3(selection_start, map->terrain.interpolated_height(selection_start.x, selection_start.y)));
		model = glm::scale(model, glm::vec3(glm::vec2(input_handler.mouse_world), 1.f) - glm::vec3(selection_start, 1.f));
		model = camera.projection_view * model;
		glUniformMatrix4fv(1, 1, GL_FALSE, &model[0][0]);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

		glDrawArrays(GL_LINE_LOOP, 0, 4);

		glDisableVertexAttribArray(0);
		glEnable(GL_DEPTH_TEST);
	}
}

void Brush::render_brush() {
	glDisable(GL_DEPTH_TEST);

	brush_shader->use();

	const int cells = std::ceil(size / 4.f) + 1;

	glUniformMatrix4fv(1, 1, GL_FALSE, &camera.projection_view[0][0]);
	glUniform2f(2, position.x, position.y);
	glUniform2f(3, uv_offset.x, uv_offset.y);
	glUniform1i(4, cells);

	glBindTextureUnit(0, map->terrain.ground_corner_height);
	glBindTextureUnit(1, brush_texture);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, shapes.vertex_buffer);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shapes.index_buffer);
	glDrawElementsInstanced(GL_TRIANGLES, shapes.quad_indices.size() * 3, GL_UNSIGNED_INT, nullptr, cells * cells);

	glDisableVertexAttribArray(0);
	glEnable(GL_DEPTH_TEST);
}