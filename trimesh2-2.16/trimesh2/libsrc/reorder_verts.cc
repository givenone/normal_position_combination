/*
Szymon Rusinkiewicz
Princeton University

reorder_verts.cc
Reorder and remap vertices.
*/

#include "TriMesh.h"
#include "TriMesh_algo.h"
using namespace std;
#define dprintf TriMesh::dprintf
#define eprintf TriMesh::eprintf


namespace trimesh {

// Remap vertices according to the given table
//
// Faces are renumbered to reflect the new numbering of vertices, and any
// faces that included a vertex that went away will also be removed.
//
// Any per-vertex properties are renumbered along with the vertices.
void remap_verts(TriMesh *mesh, const std::vector<int> &remap_table)
{
	if (remap_table.size() != mesh->vertices.size()) {
		eprintf("remap_verts called with wrong table size!\n");
		return;
	}

	// Check what we're doing
	bool removing_verts = false, any_left = false;
	int last = -1;
	int nv = mesh->vertices.size();
	for (int i = 0; i < nv; i++) {
		if (remap_table[i] < 0) {
			removing_verts = true;
		} else {
			any_left = true;
			if (remap_table[i] > last)
				last = remap_table[i];
		}
	}

	if (!any_left) {
		mesh->clear();
		return;
	}

	// Figure out what we have sitting around, so we can remap/recompute
	bool have_faces = !mesh->faces.empty();
	bool have_tstrips = !mesh->tstrips.empty();
	bool have_grid = !mesh->grid.empty();
	if (removing_verts) {
		if (have_grid) {
			// Remap the grid, recompute faces/tstrips if necessary
			mesh->faces.clear();
			mesh->tstrips.clear();
		} else if (have_tstrips) {
			// Remap faces, will recompute tstrips
			mesh->need_faces();
			mesh->tstrips.clear();
		}
	}

	bool have_col = !mesh->colors.empty();
	bool have_conf = !mesh->confidences.empty();
	bool have_flags = !mesh->flags.empty();
	bool have_normals = !mesh->normals.empty();
	bool have_pdir1 = !mesh->pdir1.empty();
	bool have_pdir2 = !mesh->pdir2.empty();
	bool have_curv1 = !mesh->curv1.empty();
	bool have_curv2 = !mesh->curv2.empty();
	bool have_dcurv = !mesh->dcurv.empty();

	// Remap the vertices and per-vertex properties
	TriMesh *oldmesh = new TriMesh;
	*oldmesh = *mesh;

#define REMAP(property) mesh->property[remap_table[i]] = oldmesh->property[i]

	for (int i = 0; i < nv; i++) {
		if (remap_table[i] < 0 || remap_table[i] == i)
			continue;
		REMAP(vertices);
		if (have_col) REMAP(colors);
		if (have_conf) REMAP(confidences);
		if (have_flags) REMAP(flags);
		if (have_normals) REMAP(normals);
		if (have_pdir1) REMAP(pdir1);
		if (have_pdir2) REMAP(pdir2);
		if (have_curv1) REMAP(curv1);
		if (have_curv2) REMAP(curv2);
		if (have_dcurv) REMAP(dcurv);
	}

#define ERASE(property) mesh->property.erase(mesh->property.begin()+last+1, \
                                             mesh->property.end())
	ERASE(vertices);
	if (have_col) ERASE(colors);
	if (have_conf) ERASE(confidences);
	if (have_flags) ERASE(flags);
	if (have_normals) ERASE(normals);
	if (have_pdir1) ERASE(pdir1);
	if (have_pdir2) ERASE(pdir2);
	if (have_curv1) ERASE(curv1);
	if (have_curv2) ERASE(curv2);
	if (have_dcurv) ERASE(dcurv);

	// Renumber faces
	int nf = mesh->faces.size(), nextface = 0;
	for (int i = 0; i < nf; i++) {
		int n0 = (mesh->faces[nextface][0] = remap_table[oldmesh->faces[i][0]]);
		int n1 = (mesh->faces[nextface][1] = remap_table[oldmesh->faces[i][1]]);
		int n2 = (mesh->faces[nextface][2] = remap_table[oldmesh->faces[i][2]]);
		if ((n0 >= 0) && (n1 >= 0) && (n2 >= 0))
			nextface++;
	}
	mesh->faces.erase(mesh->faces.begin() + nextface, mesh->faces.end());

	// Renumber grid
	if (have_grid) {
		int ng = mesh->grid.size();
		for (int i = 0; i < ng; i++) {
			if (mesh->grid[i] >= 0)
				mesh->grid[i] = remap_table[oldmesh->grid[i]];
		}
		if (have_faces && mesh->faces.empty())
			mesh->need_faces();
	}

	// Renumber tstrips if we're keeping (vs. recomputing) them.
	if (!mesh->tstrips.empty()) {
		oldmesh->convert_strips(TriMesh::TSTRIP_TERM);
		int ns = mesh->tstrips.size();
		for (int i = 0; i < ns; i++) {
			if (oldmesh->tstrips[i] < 0)
				mesh->tstrips[i] = -1;
			else
				mesh->tstrips[i] = remap_table[oldmesh->tstrips[i]];
		}
		mesh->convert_strips(TriMesh::TSTRIP_LENGTH);
	}

	// Recompute whatever needs recomputing...
	if (!mesh->pointareas.empty() || !mesh->cornerareas.empty()) {
		mesh->pointareas.clear();
		mesh->cornerareas.clear();
		mesh->need_pointareas();
	}
	if (mesh->bbox.valid) {
		mesh->bbox.valid = false;
		mesh->need_bbox();
	}
	if (mesh->bsphere.valid) {
		mesh->bsphere.valid = false;
		mesh->need_bsphere();
	}
	if (!mesh->neighbors.empty()) {
		mesh->neighbors.clear();
		mesh->need_neighbors();
	}
	if (!mesh->adjacentfaces.empty()) {
		mesh->adjacentfaces.clear();
		mesh->need_adjacentfaces();
	}
	if (!mesh->across_edge.empty()) {
		mesh->across_edge.clear();
		mesh->need_across_edge();
	}

	// Must recompute tstrips after connectivity is recomputed...
	if (have_tstrips)
		mesh->need_tstrips();

	if (!have_faces)
		mesh->faces.clear();

	delete oldmesh;
}


// Reorder vertices in a mesh according to the order in which
// they are referenced by the grid, tstrips, or faces.
void reorder_verts(TriMesh *mesh)
{
	if (mesh->grid.empty() && mesh->tstrips.empty() && mesh->faces.empty())
		return;

	dprintf("Reordering vertices... ");

	int nv = mesh->vertices.size();
	vector<int> remap(nv, -1);
	int next = 0;
	if (!mesh->grid.empty()) {
		for (size_t i = 0; i < mesh->grid.size(); i++) {
			int v = mesh->grid[i];
			if (v == -1)
				continue;
			if (remap[v] == -1)
				remap[v] = next++;
		}
	} else if (!mesh->tstrips.empty()) {
		mesh->convert_strips(TriMesh::TSTRIP_TERM);
		for (size_t i = 0; i < mesh->tstrips.size(); i++) {
			int v = mesh->tstrips[i];
			if (v == -1)
				continue;
			if (remap[v] == -1)
				remap[v] = next++;
		}
		mesh->convert_strips(TriMesh::TSTRIP_LENGTH);
	} else {
		for (size_t i = 0; i < mesh->faces.size(); i++) {
			for (int j = 0; j < 3; j++) {
				int v = mesh->faces[i][j];
				if (remap[v] == -1)
					remap[v] = next++;
			}
		}
	}

	if (next != nv) {
		// Unreferenced vertices...  Just stick them at the end.
		for (int i = 0; i < nv; i++)
			if (remap[i] == -1)
				remap[i] = next++;
	}

	remap_verts(mesh, remap);

	dprintf("Done.\n");
}

} // namespace trimesh
