/*
Szymon Rusinkiewicz
Princeton University

TriMesh_tstrips.cc
Code for dealing with triangle strips
*/

#include "TriMesh.h"
using namespace std;


namespace trimesh {

// Forward declarations
static void tstrip_build(TriMesh &mesh, int f, vector<signed char> &face_avail,
                         vector<int> &todo);
//static void collect_tris_in_strips(vector<int> &tstrips);


// Convert faces to tstrips
void TriMesh::need_tstrips(TstripRep rep)
{
	if (!tstrips.empty()) {
		convert_strips(rep);
		return;
	}

	if (faces.empty() && !grid.empty())
		need_faces();

	if (faces.empty())
		return;

	need_across_edge();

	dprintf("Building triangle strips... ");
	int nf = faces.size();

	vector<int> todo;
	vector<signed char> face_avail(nf);
	for (int i = 0; i < nf; i++) {
		face_avail[i] = (across_edge[i][0] != -1) +
				(across_edge[i][1] != -1) +
				(across_edge[i][2] != -1);
		if (face_avail[i] == 1)
			todo.push_back(i);
	}

	tstrips.reserve(nf * 2);

	int nstrips = 0;
	int i = 0;
	while (i < nf) {
		int next;
		if (todo.empty()) {
			next = i++;
		} else {
			next = todo.back();
			todo.pop_back();
		}
		if (face_avail[next] < 0)
			continue;
		tstrip_build(*this, next, face_avail, todo);
		nstrips++;
	}

	convert_strips(rep);

	dprintf("Done.\n  %d strips (Avg. length %.1f)\n",
		nstrips, (float) faces.size() / nstrips);
}


// Build a triangle strip starting with the given face
static void tstrip_build(TriMesh &mesh, int f, vector<signed char> &face_avail,
	vector<int> &todo)
{
	TriMesh::Face &v = mesh.faces[f];
	if (face_avail[f] == 0) {
		mesh.tstrips.push_back(v[0]);
		mesh.tstrips.push_back(v[1]);
		mesh.tstrips.push_back(v[2]);
		mesh.tstrips.push_back(-1);
		face_avail[f] = -1;
		return;
	}

	int score[3];
	for (int i = 0; i < 3; i++) {
		score[i] = 0;
		int ae = mesh.across_edge[f][i];
		if (ae == -1 || face_avail[ae] < 0)
			continue;
		score[i]++;
		int next_edge = mesh.faces[ae].indexof(v[NEXT_MOD3(i)]);
		int nae = mesh.across_edge[ae][next_edge];
		if (nae == -1 || face_avail[nae] < 0)
			continue;
		score[i]++;
		if (face_avail[ae] == 2)
			score[i]++;
	}

	int best_score = max(max(score[0], score[1]), score[2]);
	int best = (score[0] == best_score) ? 0 :
	           (score[1] == best_score) ? 1 : 2;

	int vlast2 = v[          best ];
	int vlast1 = v[NEXT_MOD3(best)];
	int vnext  = v[PREV_MOD3(best)];
	int dir = 1;
	mesh.tstrips.push_back(vlast2);
	mesh.tstrips.push_back(vlast1);

	while (1) {
		mesh.tstrips.push_back(vnext);
		face_avail[f] = -1;
		for (int j = 0; j < 3; j++) {
			int ae = mesh.across_edge[f][j];
			if (ae == -1)
				continue;
			if (face_avail[ae] > 0)
				face_avail[ae]--;
			if (face_avail[ae] == 1)
				todo.push_back(ae);
		}

		int faceindex = mesh.faces[f].indexof(vlast2);
		if (faceindex < 0)
			break;
		f = mesh.across_edge[f][faceindex];
		if (f == -1 || face_avail[f] < 0)
			break;
		vlast2 = vlast1;
		vlast1 = vnext;
		vnext = mesh.faces[f][(mesh.faces[f].indexof(vlast2)+3+dir)%3];
		dir = -dir;
	}

	mesh.tstrips.push_back(-1);
}


// Convert triangle strips to faces
void TriMesh::unpack_tstrips()
{
	if (tstrips.empty() || !faces.empty())
		return;

	convert_strips(TSTRIP_LENGTH);

	dprintf("Unpacking triangle strips... ");
	int nstrips = tstrips.size();
	int nfaces = 0;
	int i = 0;
	while (i < nstrips) {
		nfaces += tstrips[i] - 2;
		i += tstrips[i] + 1;
	}

	faces.reserve(nfaces);

	int len = 0;
	bool flip = false;
	for (i = 0; i < nstrips; i++) {
		if (len == 0) {
			len = tstrips[i] - 2;
			flip = false;
			i += 2;
			continue;
		}
		if (flip)
			faces.push_back(Face(tstrips[i-1],
			                     tstrips[i-2],
			                     tstrips[i]));
		else
			faces.push_back(Face(tstrips[i-2],
			                     tstrips[i-1],
			                     tstrips[i]));
		flip = !flip;
		len--;
	}
	dprintf("Done.\n  %d triangles\n", nfaces);
}


// Convert between "length preceding strip" and "-1 following strip"
// representations
void TriMesh::convert_strips(TstripRep rep)
{
	if (tstrips.empty())
		return;
	if (rep == TSTRIP_TERM && tstrips.back() == -1)
		return;
	if (rep == TSTRIP_LENGTH && tstrips.back() != -1) {
		//collect_tris_in_strips(tstrips);
		return;
	}
	int nstrips = tstrips.size();

	if (rep == TSTRIP_TERM) {
		int len = tstrips[0];
		for (int i = 1; i < nstrips; i++) {
			if (len) {
				tstrips[i-1] = tstrips[i];
				len--;
			} else {
				tstrips[i-1] = -1;
				len = tstrips[i];
			}
		}
		tstrips.back() = -1;
	} else {
		int len = 0;
		for (int i = nstrips - 2; i >= 0; i--) {
			if (tstrips[i] == -1) {
				tstrips[i+1] = len;
				len = 0;
			} else {
				tstrips[i+1] = tstrips[i];
				len++;
			}
		}
		tstrips[0] = len;
		//collect_tris_in_strips(tstrips);
	}
}


#if 0
// Collect all single triangles to be at the end of the list of tstrips
static void collect_tris_in_strips(vector<int> &tstrips)
{
	if (tstrips.empty())
		return;
	vector<int> tris;
	int nstrips = tstrips.size();

	int n = 0, offset = 0;
	bool have_tri = false, bad_strip = false;
	for (int i = 0; i < nstrips; i++) {
		if (n == 0) {
			n = tstrips[i];
			bad_strip = (n < 3);
			have_tri = (n == 3);
			n++;
		}
		if (bad_strip) {
			offset++;
		} else if (have_tri) {
			tris.push_back(tstrips[i]);
			offset++;
		} else if (offset > 0) {
			tstrips[i-offset] = tstrips[i];
		}
		n--;
	}
	if (offset == 0)
		return;
	tstrips.erase(tstrips.end() - offset, tstrips.end());
	tstrips.insert(tstrips.end(), tris.begin(), tris.end());

}
#endif

} // namespace trimesh
