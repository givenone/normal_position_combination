/*
Szymon Rusinkiewicz
Princeton University

TriMesh_curvature.cc
Computation of per-vertex principal curvatures and directions.

Uses algorithm from
 Rusinkiewicz, Szymon.
 "Estimating Curvatures and Their Derivatives on Triangle Meshes,"
 Proc. 3DPVT, 2004.
*/

#include "TriMesh.h"
#include "TriMesh_algo.h"
#include "lineqn.h"
using namespace std;


namespace trimesh {

// Rotate a coordinate system to be perpendicular to the given normal
static void rot_coord_sys(const vec &old_u, const vec &old_v,
                          const vec &new_norm,
                          vec &new_u, vec &new_v)
{
	new_u = old_u;
	new_v = old_v;
	vec old_norm = old_u CROSS old_v;
	float ndot = old_norm DOT new_norm;
	if (unlikely(ndot <= -1.0f)) {
		new_u = -new_u;
		new_v = -new_v;
		return;
	}

	// Perpendicular to old_norm and in the plane of old_norm and new_norm
	vec perp_old = new_norm - ndot * old_norm;

	// Perpendicular to new_norm and in the plane of old_norm and new_norm
	// vec perp_new = ndot * new_norm - old_norm;

	// perp_old - perp_new, with normalization constants folded in
	vec dperp = 1.0f / (1 + ndot) * (old_norm + new_norm);

	// Subtracts component along perp_old, and adds the same amount along
	// perp_new.  Leaves unchanged the component perpendicular to the
	// plane containing old_norm and new_norm.
	new_u -= dperp * (new_u DOT perp_old);
	new_v -= dperp * (new_v DOT perp_old);
}


// Reproject a curvature tensor from the basis spanned by old_u and old_v
// (which are assumed to be unit-length and perpendicular) to the
// new_u, new_v basis.
void proj_curv(const vec &old_u, const vec &old_v,
               float old_ku, float old_kuv, float old_kv,
               const vec &new_u, const vec &new_v,
               float &new_ku, float &new_kuv, float &new_kv)
{
	vec r_new_u, r_new_v;
	rot_coord_sys(new_u, new_v, old_u CROSS old_v, r_new_u, r_new_v);

	float u1 = r_new_u DOT old_u;
	float v1 = r_new_u DOT old_v;
	float u2 = r_new_v DOT old_u;
	float v2 = r_new_v DOT old_v;
	new_ku  = old_ku * u1*u1 + old_kuv * (2.0f  * u1*v1) + old_kv * v1*v1;
	new_kuv = old_ku * u1*u2 + old_kuv * (u1*v2 + u2*v1) + old_kv * v1*v2;
	new_kv  = old_ku * u2*u2 + old_kuv * (2.0f  * u2*v2) + old_kv * v2*v2;
}


// Like the above, but for dcurv
void proj_dcurv(const vec &old_u, const vec &old_v,
		const Vec<4> old_dcurv,
		const vec &new_u, const vec &new_v,
		Vec<4> &new_dcurv)
{
	vec r_new_u, r_new_v;
	rot_coord_sys(new_u, new_v, old_u CROSS old_v, r_new_u, r_new_v);

	float u1 = r_new_u DOT old_u;
	float v1 = r_new_u DOT old_v;
	float u2 = r_new_v DOT old_u;
	float v2 = r_new_v DOT old_v;

	new_dcurv[0] = old_dcurv[0]*u1*u1*u1 +
	               old_dcurv[1]*3.0f*u1*u1*v1 +
	               old_dcurv[2]*3.0f*u1*v1*v1 +
	               old_dcurv[3]*v1*v1*v1;
	new_dcurv[1] = old_dcurv[0]*u1*u1*u2 +
	               old_dcurv[1]*(u1*u1*v2 + 2.0f*u2*u1*v1) +
	               old_dcurv[2]*(u2*v1*v1 + 2.0f*u1*v1*v2) +
	               old_dcurv[3]*v1*v1*v2;
	new_dcurv[2] = old_dcurv[0]*u1*u2*u2 +
	               old_dcurv[1]*(u2*u2*v1 + 2.0f*u1*u2*v2) +
	               old_dcurv[2]*(u1*v2*v2 + 2.0f*u2*v2*v1) +
	               old_dcurv[3]*v1*v2*v2;
	new_dcurv[3] = old_dcurv[0]*u2*u2*u2 +
	               old_dcurv[1]*3.0f*u2*u2*v2 +
	               old_dcurv[2]*3.0f*u2*v2*v2 +
	               old_dcurv[3]*v2*v2*v2;
}


// Given a curvature tensor, find principal directions and curvatures
// Makes sure that pdir1 and pdir2 are perpendicular to normal
void diagonalize_curv(const vec &old_u, const vec &old_v,
                      float ku, float kuv, float kv,
                      const vec &new_norm,
                      vec &pdir1, vec &pdir2, float &k1, float &k2)
{
	vec r_old_u, r_old_v;
	rot_coord_sys(old_u, old_v, new_norm, r_old_u, r_old_v);

	float c = 1, s = 0, tt = 0;
	if (likely(kuv != 0.0f)) {
		// Jacobi rotation to diagonalize
		float h = 0.5f * (kv - ku) / kuv;
		tt = (h < 0.0f) ?
			1.0f / (h - sqrt(1.0f + h*h)) :
			1.0f / (h + sqrt(1.0f + h*h));
		c = 1.0f / sqrt(1.0f + tt*tt);
		s = tt * c;
	}

	k1 = ku - tt * kuv;
	k2 = kv + tt * kuv;

	if (fabs(k1) >= fabs(k2)) {
		pdir1 = c*r_old_u - s*r_old_v;
	} else {
		swap(k1, k2);
		pdir1 = s*r_old_u + c*r_old_v;
	}
	pdir2 = new_norm CROSS pdir1;
}


// Compute principal curvatures and directions.
void TriMesh::need_curvatures()
{
	if (curv1.size() == vertices.size())
		return;
	need_faces();
	need_normals();
	need_pointareas();

	dprintf("Computing curvatures... ");

	// Resize the arrays we'll be using
	int nv = vertices.size(), nf = faces.size();
	curv1.clear(); curv1.resize(nv); curv2.clear(); curv2.resize(nv);
	pdir1.clear(); pdir1.resize(nv); pdir2.clear(); pdir2.resize(nv);
	vector<float> curv12(nv);

	// Set up an initial coordinate system per vertex
	for (int i = 0; i < nf; i++) {
		pdir1[faces[i][0]] = vertices[faces[i][1]] -
		                     vertices[faces[i][0]];
		pdir1[faces[i][1]] = vertices[faces[i][2]] -
		                     vertices[faces[i][1]];
		pdir1[faces[i][2]] = vertices[faces[i][0]] -
		                     vertices[faces[i][2]];
	}
#pragma omp parallel for
	for (int i = 0; i < nv; i++) {
		pdir1[i] = pdir1[i] CROSS normals[i];
		normalize(pdir1[i]);
		pdir2[i] = normals[i] CROSS pdir1[i];
	}

	// Compute curvature per-face
#pragma omp parallel for
	for (int i = 0; i < nf; i++) {
		// Edges
		vec e[3] = { vertices[faces[i][2]] - vertices[faces[i][1]],
		             vertices[faces[i][0]] - vertices[faces[i][2]],
		             vertices[faces[i][1]] - vertices[faces[i][0]] };

		// N-T-B coordinate system per face
		vec t = e[0];
		normalize(t);
		vec n = e[0] CROSS e[1];
		vec b = n CROSS t;
		normalize(b);

		// Estimate curvature based on variation of normals
		// along edges
		float m[3] = { 0, 0, 0 };
		float w[3][3] = { {0,0,0}, {0,0,0}, {0,0,0} };
		for (int j = 0; j < 3; j++) {
			float u = e[j] DOT t;
			float v = e[j] DOT b;
			w[0][0] += u*u;
			w[0][1] += u*v;
			w[2][2] += v*v;
			// The below are computed once at the end of the loop
			// w[1][1] += v*v + u*u;
			// w[1][2] += u*v;
			vec dn = normals[faces[i][PREV_MOD3(j)]] -
			         normals[faces[i][NEXT_MOD3(j)]];
			float dnu = dn DOT t;
			float dnv = dn DOT b;
			m[0] += dnu*u;
			m[1] += dnu*v + dnv*u;
			m[2] += dnv*v;
		}
		w[1][1] = w[0][0] + w[2][2];
		w[1][2] = w[0][1];

		// Least squares solution
		float diag[3];
		if (!ldltdc<float,3>(w, diag)) {
			//dprintf("ldltdc failed!\n");
			continue;
		}
		ldltsl<float,3>(w, diag, m, m);

		// Push it back out to the vertices
		for (int j = 0; j < 3; j++) {
			int vj = faces[i][j];
			float c1, c12, c2;
			proj_curv(t, b, m[0], m[1], m[2],
			          pdir1[vj], pdir2[vj], c1, c12, c2);
			float wt = cornerareas[i][j] / pointareas[vj];
#pragma omp atomic
			curv1[vj]  += wt * c1;
#pragma omp atomic
			curv12[vj] += wt * c12;
#pragma omp atomic
			curv2[vj]  += wt * c2;
		}
	}

	// Compute principal directions and curvatures at each vertex
#pragma omp parallel for
	for (int i = 0; i < nv; i++) {
		diagonalize_curv(pdir1[i], pdir2[i],
		                 curv1[i], curv12[i], curv2[i],
		                 normals[i], pdir1[i], pdir2[i],
		                 curv1[i], curv2[i]);
	}
	dprintf("Done.\n");
}


// Compute derivatives of curvature.
void TriMesh::need_dcurv()
{
	if (dcurv.size() == vertices.size())
		return;
	need_curvatures();

	dprintf("Computing dcurv... ");

	// Resize the arrays we'll be using
	int nv = vertices.size(), nf = faces.size();
	dcurv.clear(); dcurv.resize(nv);

	// Compute dcurv per-face
#pragma omp parallel for
	for (int i = 0; i < nf; i++) {
		// Edges
		vec e[3] = { vertices[faces[i][2]] - vertices[faces[i][1]],
		             vertices[faces[i][0]] - vertices[faces[i][2]],
		             vertices[faces[i][1]] - vertices[faces[i][0]] };

		// N-T-B coordinate system per face
		vec t = e[0];
		normalize(t);
		vec n = e[0] CROSS e[1];
		vec b = n CROSS t;
		normalize(b);

		// Project curvature tensor from each vertex into this
		// face's coordinate system
		vec fcurv[3];
		for (int j = 0; j < 3; j++) {
			int vj = faces[i][j];
			proj_curv(pdir1[vj], pdir2[vj], curv1[vj], 0, curv2[vj],
			          t, b, fcurv[j][0], fcurv[j][1], fcurv[j][2]);

		}

		// Estimate dcurv based on variation of curvature along edges
		float m[4] = { 0, 0, 0, 0 };
		float w[4][4] = { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} };
		for (int j = 0; j < 3; j++) {
			// Variation of curvature along each edge
			vec dfcurv = fcurv[PREV_MOD3(j)] - fcurv[NEXT_MOD3(j)];
			float u = e[j] DOT t;
			float v = e[j] DOT b;
			float u2 = u*u, v2 = v*v, uv = u*v;
			w[0][0] += u2;
			w[0][1] += uv;
			w[3][3] += v2;
			// All the below are computed at the end of the loop
			// w[1][1] += 2.0f*u2 + v2;
			// w[1][2] += 2.0f*uv;
			// w[2][2] += u2 + 2.0f*v2;
			// w[2][3] += uv;
			m[0] += u*dfcurv[0];
			m[1] += v*dfcurv[0] + 2.0f*u*dfcurv[1];
			m[2] += 2.0f*v*dfcurv[1] + u*dfcurv[2];
			m[3] += v*dfcurv[2];
		}
		w[1][1] = 2.0f * w[0][0] + w[3][3];
		w[1][2] = 2.0f * w[0][1];
		w[2][2] = w[0][0] + 2.0f * w[3][3];
		w[2][3] = w[0][1];

		// Least squares solution
		float d[4];
		if (!ldltdc<float,4>(w, d)) {
			//dprintf("ldltdc failed!\n");
			continue;
		}
		ldltsl<float,4>(w, d, m, m);
		Vec<4> face_dcurv(m);

		// Push it back out to each vertex
		for (int j = 0; j < 3; j++) {
			int vj = faces[i][j];
			Vec<4> this_vert_dcurv;
			proj_dcurv(t, b, face_dcurv,
			           pdir1[vj], pdir2[vj], this_vert_dcurv);
			float wt = cornerareas[i][j] / pointareas[vj];
			dcurv[vj] += wt * this_vert_dcurv;
		}
	}

	dprintf("Done.\n");
}

} // namespace trimesh
