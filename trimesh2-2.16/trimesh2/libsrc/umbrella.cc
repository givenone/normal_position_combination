/*
Szymon Rusinkiewicz
Princeton University

umbrella.cc
Umbrella and Taubin lambda/mu mesh smoothing
*/

#include "TriMesh.h"
#include "TriMesh_algo.h"
#define dprintf TriMesh::dprintf


namespace trimesh {

// One iteration of umbrella-operator smoothing
void umbrella(TriMesh *mesh, float stepsize, bool tangent /* = false */)
{
	mesh->need_neighbors();
	mesh->need_adjacentfaces();
	if (tangent)
		mesh->need_normals();
	int nv = mesh->vertices.size();
	std::vector<vec> disp(nv);
#pragma omp parallel for
	for (int i = 0; i < nv; i++) {
		if (mesh->is_bdy(i)) {
			// Change to #if 1 to smooth boundaries.
			// This way, we leave boundaries alone.
#if 0
			int nn = mesh->neighbors[i].size();
			int nnused = 0;
			if (!nn)
				continue;
			for (int j = 0; j < nn; j++) {
				if (!mesh->is_bdy(mesh->neighbors[i][j]))
					continue;
				disp[i] += mesh->vertices[mesh->neighbors[i][j]];
				nnused++;
			}
			disp[i] /= nnused;
			disp[i] -= mesh->vertices[i];
#else
			disp[i].clear();
#endif
		} else {
			int nn = mesh->neighbors[i].size();
			if (!nn)
				continue;
			for (int j = 0; j < nn; j++)
				disp[i] += mesh->vertices[mesh->neighbors[i][j]];
			disp[i] /= nn;
			disp[i] -= mesh->vertices[i];
		}
	}
	if (tangent) {
#pragma omp parallel for
		for (int i = 0; i < nv; i++) {
			const vec &norm = mesh->normals[i];
			mesh->vertices[i] += stepsize * (disp[i] -
				norm * (disp[i] DOT norm));
		}
	} else {
#pragma omp parallel for
		for (int i = 0; i < nv; i++)
			mesh->vertices[i] += stepsize * disp[i];
	}

	mesh->bbox.valid = false;
	mesh->bsphere.valid = false;
}


// Several iterations of Taubin lambda/mu
void lmsmooth(TriMesh *mesh, int niters)
{
	mesh->need_neighbors();
	mesh->need_adjacentfaces();
	dprintf("Smoothing mesh... ");
	for (int i = 0; i < niters; i++) {
		umbrella(mesh, 0.330f);
		umbrella(mesh, -0.331f);
	}
	dprintf("Done.\n");

	mesh->bbox.valid = false;
	mesh->bsphere.valid = false;
}


// One iteration of umbrella-operator smoothing on the normals
void numbrella(TriMesh *mesh, float stepsize)
{
	mesh->need_neighbors();
	mesh->need_normals();
	int nv = mesh->normals.size();
	std::vector<vec> disp(nv);
#pragma omp parallel for
	for (int i = 0; i < nv; i++) {
		int nn = mesh->neighbors[i].size();
		if (!nn)
			continue;
		for (int j = 0; j < nn; j++)
			disp[i] += mesh->normals[mesh->neighbors[i][j]];
		disp[i] /= nn;
		disp[i] -= mesh->normals[i];
	}

#pragma omp parallel for
	for (int i = 0; i < nv; i++) {
		mesh->normals[i] += stepsize * disp[i];
		normalize(mesh->normals[i]);
	}
}

} // namespace trimesh
