#include "RenderManager.h"

#include "ResourceManager.h"
#include "Camera.h"

RenderManager::RenderManager() {
	instance_static_mesh_shader = resource_manager.load<Shader>({ "Data/Shaders/static_mesh_instanced.vs", "Data/Shaders/static_mesh_instanced.fs" });
	static_mesh_shader = resource_manager.load<Shader>({ "Data/Shaders/static_mesh.vs", "Data/Shaders/static_mesh.fs" });
}


void RenderManager::render() {
	instance_static_mesh_shader->use();
	gl->glUniformMatrix4fv(4, 1, false, &camera->projection_view[0][0]);

	gl->glEnableVertexAttribArray(0);
	gl->glEnableVertexAttribArray(1);
	gl->glEnableVertexAttribArray(2);

	// Render opaque meshes
	// These don't have to be sorted and can thus be drawn instanced (one draw call per type of mesh)
	gl->glDisable(GL_BLEND);

	for (const auto& i : meshes) {
		i->render_opaque();
	}

	// Render transparent meshes
	std::sort(transparent_instances.begin(), transparent_instances.end(), [](Inst& left, Inst& right) { return left.distance > right.distance; });
	static_mesh_shader->use();

	gl->glEnable(GL_BLEND);
	gl->glDepthMask(false);
	gl->glUniform1f(3, -1.f);
	for (const auto& i : transparent_instances) {
		meshes[i.mesh_id]->render_transparent(i.instance_id);
	}
	gl->glDepthMask(true);

	for (const auto& i : meshes) {
		i->render_jobs.clear();
	}

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
	gl->glDisableVertexAttribArray(2);

	meshes.clear();
	transparent_instances.clear();
}