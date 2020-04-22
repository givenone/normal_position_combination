/*
Szymon Rusinkiewicz
Princeton University

TriMesh_pointareas.cc
Compute the area "belonging" to each vertex or each corner
of a triangle (defined as Voronoi area restricted to the 1-ring of
a vertex, or to the triangle).
*/

#include "TriMesh.h"


namespace trimesh {

// Compute per-vertex point areas
void TriMesh::need_pointareas()
{
	if (pointareas.size() == vertices.size())
		return;
	need_faces();

	dprintf("Computing point areas... ");

	int nf = faces.size(), nv = vertices.size();
	pointareas.clear();
	pointareas.resize(nv);
	cornerareas.clear();
	cornerareas.resize(nf);

#pragma omp parallel for
	for (int i = 0; i < nf; i++) {
		// Edges
		vec e[3] = { vertices[faces[i][2]] - vertices[faces[i][1]],
		             vertices[faces[i][0]] - vertices[faces[i][2]],
		             vertices[faces[i][1]] - vertices[faces[i][0]] };

		// Compute corner weights
		float area = 0.5f * len(e[0] CROSS e[1]);
		float l2[3] = { len2(e[0]), len2(e[1]), len2(e[2]) };

		// Barycentric weights of circumcenter
		float bcw[3] = { l2[0] * (l2[1] + l2[2] - l2[0]),
		                 l2[1] * (l2[2] + l2[0] - l2[1]),
		                 l2[2] * (l2[0] + l2[1] - l2[2]) };
		if (bcw[0] <= 0.0f) {
			cornerareas[i][1] = -0.25f * l2[2] * area /
			                    (e[0] DOT e[2]);
			cornerareas[i][2] = -0.25f * l2[1] * area /
			                    (e[0] DOT e[1]);
			cornerareas[i][0] = area - cornerareas[i][1] -
			                    cornerareas[i][2];
		} else if (bcw[1] <= 0.0f) {
			cornerareas[i][2] = -0.25f * l2[0] * area /
			                    (e[1] DOT e[0]);
			cornerareas[i][0] = -0.25f * l2[2] * area /
			                    (e[1] DOT e[2]);
			cornerareas[i][1] = area - cornerareas[i][2] -
			                    cornerareas[i][0];
		} else if (bcw[2] <= 0.0f) {
			cornerareas[i][0] = -0.25f * l2[1] * area /
			                    (e[2] DOT e[1]);
			cornerareas[i][1] = -0.25f * l2[0] * area /
			                    (e[2] DOT e[0]);
			cornerareas[i][2] = area - cornerareas[i][0] -
			                    cornerareas[i][1];
		} else {
			float scale = 0.5f * area / (bcw[0] + bcw[1] + bcw[2]);
			for (int j = 0; j < 3; j++)
				cornerareas[i][j] = scale * (bcw[NEXT_MOD3(j)] +
				                             bcw[PREV_MOD3(j)]);
		}
#pragma omp atomic
		pointareas[faces[i][0]] += cornerareas[i][0];
#pragma omp atomic
		pointareas[faces[i][1]] += cornerareas[i][1];
#pragma omp atomic
		pointareas[faces[i][2]] += cornerareas[i][2];
	}

	dprintf("Done.\n");
}

} // namespace trimesh
