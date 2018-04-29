#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <GL/glut.h>
#include <AntTweakBar.h>
#include <vector>
#include <map>
using namespace std;

// Basic class for convenience
class TriangleIndices {
public:
	int ix[3];
	TriangleIndices(int i0, int i1, int i2){
		ix[0] = i0;
		ix[1] = i1;
		ix[2] = i2;
	}
};

class EdgeIndices {
	int ix1, ix2;
public:
	EdgeIndices(int i1, int i2) :ix1(i1), ix2(i2) {}
	EdgeIndices(){}
	inline bool operator < (const EdgeIndices rhs) const{
		if (ix1 != rhs.ix1)
			return ix1 < rhs.ix1;
		else
			return ix2 < rhs.ix2;
	}
};

class Vector3d {  // this is a pretty standard vector class
public:
	double x, y, z;
	Vector3d(){}
	Vector3d(double xx, double yy, double zz) :x(xx), y(yy), z(zz) {}
	Vector3d(Vector3d& v1, Vector3d& v2) {
		x = v2.x - v1.x;
		y = v2.y - v1.y;
		z = v2.z - v1.z;
	}
	inline Vector3d operator + (const Vector3d& rhs) const {
		return Vector3d(x + rhs.x, y + rhs.y, z + rhs.z);
	}
	inline Vector3d operator - (const Vector3d& rhs) const {
		return Vector3d(x - rhs.x, y - rhs.y, z - rhs.z);
	}
	inline Vector3d operator * (const Vector3d& rhs) const {
		double m = y*rhs.z - z*rhs.y;
		double n = z*rhs.x - x*rhs.z;
		double o = x*rhs.y - y*rhs.x;
		return Vector3d(m, n, o);
	}
	inline Vector3d operator * (const double factor) const {
		return Vector3d(x*factor, y*factor, z*factor);
	}
	inline Vector3d operator / (const double factor) const {
		return Vector3d(x/factor, y/factor, z/factor);
	}
	inline bool operator == (const Vector3d& rhs) const {
		if (x != rhs.x)return false;
		if (y != rhs.y)return false;
		if (z != rhs.z)return false;
		return true;
	}
	inline void set(double rx, double ry, double rz) {
		x = rx;
		y = ry;
		z = rz;
	}
	inline Vector3d norm() {
		double r = sqrt(x*x + y*y + z*z);
		if (r == 0)return Vector3d(0, 0, 0);
		return Vector3d(x / r, y / r, z / r);
	}
	inline double len() {
		return sqrt(x*x + y*y + z*z);
	}
	inline double square() {
		return x*x + y*y + z*z;
	}
};

// a 3d closed object's surface, used as basic class
class Shell {
public:
	vector<Vector3d> pts_cloud;
	vector<TriangleIndices> f_indices;
	int MAX_ITER, STEP_LIMIT, MAX_DEPTH;
	double EPS;
	double BAILOUT;
	Shell():MAX_ITER(50),STEP_LIMIT(80),MAX_DEPTH(7),EPS(0.00001),BAILOUT(100){}
	inline double DE(const Vector3d& v) {
		Vector3d z = v;
		double r = 0;
		double dr = 1;
		int power = 8;
		for (int i = 0; i < MAX_ITER; i++) {
			r = z.len();
			if (r > BAILOUT)break;

			double r2, r4, r8;
			r2 = r*r;
			r4 = r2*r2;// r4
			r8 = r4*r4;// fast power

			dr = r4*r2*r*power*dr + 1;

			double theta = asin(z.z / r);
			double phi = atan2(z.y, z.x);

			theta = 8 * theta;
			phi = 8 * phi;

			// convert to cartessian coordinate
			z = Vector3d(r8 * cos(theta)*cos(phi), r8 * cos(theta)*sin(phi), r8 * sin(theta));
			z = z + v;
		}
		return 0.5*log(r)*r / dr;
	}
	void saveXYZ() {
		FILE* fp = fopen("ptsCld.xyz", "w");
		for (int i = 0; i < pts_cloud.size(); i++) {
			fprintf(fp, "%f %f %f ", pts_cloud[i].x, pts_cloud[i].y, pts_cloud[i].z);
			fprintf(fp, "%f %f %f\n", 0.0, 0.0, 0.0);
		}
		fclose(fp);
	}
	void saveSTL() {
		FILE* fp = fopen("ptsCld.stl", "wb");
		// write header
		for (int i = 0; i < 80; i++)
			fwrite("\0", sizeof(char), 1, fp);
		int tragl_num = (int)f_indices.size();
		fwrite(&tragl_num, sizeof(int), 1, fp);

		for (int i = 0; i < tragl_num; i++) {
			//normal vec
			float buf[9] = { 0,0,0 };
			fwrite(buf, sizeof(float), 3, fp);

			//vertex 1 2 3, float32
			for (int j = 0; j < 3; j++) {
				buf[3 * j + 0] = (float)pts_cloud[f_indices[i].ix[j]].x * 30;
				buf[3 * j + 1] = (float)pts_cloud[f_indices[i].ix[j]].y * 30;
				buf[3 * j + 2] = (float)(pts_cloud[f_indices[i].ix[j]].z * 30 + 40);
			}
			fwrite(buf, sizeof(float), 9, fp);

			// attribute byte count: 16bit
			fwrite("\0\0", sizeof(char), 2, fp);
		}
		fclose(fp);
	}
};

// using cylindrical coordinates
class Bucket : public Shell {
	int z_den;
	double d_theta;
	double TwoPI;
public:
	Bucket() :z_den(800), d_theta(1 / 200.0), TwoPI(2 * 3.1415926) {
		double voxel_size = 2.7 / z_den;
		printf("[");
		int len = 0;
		for (int i = 0; i < z_den; i++) {
			if (i % (z_den / 20) == 0)printf(">");
			double z = (i - z_den / 2)*voxel_size;
			for (double theta = 0; theta < TwoPI; theta += d_theta) {
				double r = 4;
				double dist;
				Vector3d vt;
				for (int k = 0; k < MAX_ITER; k++) {
					double x = r*sin(theta);
					double y = r*cos(theta);
					vt.set(x, y, z);
					dist = DE(vt);
					r -= dist;
					if (dist < EPS) {
						break;
					}
				}
				if (dist > 0.5) {
					break;
				}
				pts_cloud.push_back(vt);
			}
			if (len == 0)len = pts_cloud.size();
		}
		for (int i = 0; i < pts_cloud.size() / len - 1; i++) {
			if (i % (z_den / 20) == 0)printf(">");
			for (int j = 0; j < len; j++) {
				int base = i*len;
				int right = (j == len - 1) ? base : base + j + 1;
				f_indices.push_back(TriangleIndices(j + base, right + len, right));
				f_indices.push_back(TriangleIndices(j + base, j + base + len, right + len));
			}
		}
		printf("]\n");
	}
};

// using spherical coordinates
class Geodisc: public Shell {
public:
	map<EdgeIndices, int>indexCache;
	int max_depth;
	int inline getMiddlePoint(int p1, int p2) {
		EdgeIndices key;
		if (p1 < p2) {
			key = EdgeIndices(p1, p2);
		}
		else {
			key = EdgeIndices(p2, p1);
		}

		map<EdgeIndices, int>::iterator it = indexCache.find(key);
		if (it != indexCache.end()) {
			return it->second;
		}
		
		const Vector3d v1 = pts_cloud[p1];
		const Vector3d v2 = pts_cloud[p2];
		const Vector3d v12 = (v1 + v2).norm();

		// update cache
		int idx = pts_cloud.size();
		indexCache.insert(pair<EdgeIndices, int>(key, idx));
		pts_cloud.push_back(v12);
		return idx;
	}
	void subdivide(int ix1, int ix2, int ix3, const unsigned int depth) {
		if (depth == 0) {
			f_indices.push_back(TriangleIndices(ix1, ix3, ix2));
			return;
		}

		int ix12 = getMiddlePoint(ix1, ix2);
		int ix23 = getMiddlePoint(ix2, ix3);
		int ix31 = getMiddlePoint(ix3, ix1);

		subdivide(ix1, ix12, ix31, depth - 1);
		subdivide(ix2, ix23, ix12, depth - 1);
		subdivide(ix3, ix31, ix23, depth - 1);
		subdivide(ix12, ix23, ix31, depth - 1);
	}
	// straight forward method, bad result
	void transform() {
		printf("[");
		for (int i = 0; i < pts_cloud.size(); i++) {
			if (i % (pts_cloud.size() / 20) == 0)printf(">");
			Vector3d n(pts_cloud[i]);
			double dist, r = 2.5;
			pts_cloud[i] = n * r;
			for (int j = 0; j < STEP_LIMIT; j++) {
				dist = DE(pts_cloud[i]);
				r -= dist;
				pts_cloud[i] = n * r;
			}
		}
		printf("]\n");
	}

	// new method
	void squeeze() {
		// random shuffle
		int size = f_indices.size();
		int* shf = new int[size];
		Vector3d* init_nv = new Vector3d[f_indices.size()];

		for (int i = 0; i < size; i++)
			shf[i] = i;

		srand(time(NULL));
		for (int i = size - 1; i > 0; i--) {
			int rn = rand() % (i + 1);
			int tmp = shf[i];
			shf[i] = shf[rn];
			shf[rn] = tmp;
		}

		printf("#");
		for (int k = 0; k < 1; k++) {
			for (int i = 0; i < size; i++) {
				int idx[3];
				idx[0] = f_indices[i].ix[0];
				idx[1] = f_indices[i].ix[1];
				idx[2] = f_indices[i].ix[2];

				Vector3d v1 = pts_cloud[idx[1]] - pts_cloud[idx[0]];
				Vector3d v2 = pts_cloud[idx[2]] - pts_cloud[idx[0]];
				Vector3d nv = v2*v1;// cross multiple
				nv = nv.norm();
				init_nv[i] = nv;
			}
			printf(">");
			for (int m = 0; m < 2; m++) {
				for (int i = 0; i < size; i++) {
					int idx[3];
					idx[0] = f_indices[shf[i]].ix[0];
					idx[1] = f_indices[shf[i]].ix[1];
					idx[2] = f_indices[shf[i]].ix[2];
					Vector3d nv = init_nv[shf[i]];
					for (int j = 0; j < 3; j++) {// 3 cannot bechanged
						double dist = DE(pts_cloud[idx[j]]);
						Vector3d offset = nv*dist;
						pts_cloud[idx[j]] = pts_cloud[idx[j]] + offset;
					}
				}
			}
		}
		delete[] init_nv;
		delete[] shf;
	}
	void manipulate() {
		printf("[");
		
		// basic model
		for (int i = 0; i < pts_cloud.size(); i++) {
			if (i % (pts_cloud.size() / 20) == 0)printf(">");
			Vector3d n(pts_cloud[i]);
			double r = 3;
			pts_cloud[i] = n*r;
			// 4 is first move without adjust norm
			for (int j = 0; j < 4; j++) {
				double dist = DE(pts_cloud[i]);
				r -= dist;
				pts_cloud[i] = n*r;
			}
		}
		squeeze();
		printf("]\n");
	}
	Geodisc(int dep=3):max_depth(dep) {
		const double X = 0.525731112119133606;
		const double Z = 0.850650808352039932;
		const Vector3d vdata[12] = {
			{ -X, 0.0, Z },{ X, 0.0, Z },{ -X, 0.0, -Z },{ X, 0.0, -Z },
			{ 0.0, Z, X },{ 0.0, Z, -X },{ 0.0, -Z, X },{ 0.0, -Z, -X },
			{ Z, X, 0.0 },{ -Z, X, 0.0 },{ Z, -X, 0.0 },{ -Z, -X, 0.0 }
		};
		int findices[20][3] = {
			{ 0, 4, 1 },{ 0, 9, 4 },{ 9, 5, 4 },{ 4, 5, 8 },{ 4, 8, 1 },
			{ 8, 10, 1 },{ 8, 3, 10 },{ 5, 3, 8 },{ 5, 2, 3 },{ 2, 7, 3 },
			{ 7, 10, 3 },{ 7, 6, 10 },{ 7, 11, 6 },{ 11, 0, 6 },{ 0, 1, 6 },
			{ 6, 1, 10 },{ 9, 0, 11 },{ 9, 11, 2 },{ 9, 2, 5 },{ 7, 2, 11 }
		};
		// adding first 20 points
		for (int i = 0; i < 12; i++) {
			pts_cloud.push_back(vdata[i]);
		}
		for (int i = 0; i < 20; i++) {
			subdivide(findices[i][0], findices[i][1], findices[i][2], max_depth);
		}
		manipulate();
	}
};

class Donkey: public Shell {
public:
	const static int den = 100;
	const double diameter = 2*2.0/den;
	Vector3d up_face[den][den],dn_face[den][den];

	inline double DE(Vector3d& v) {
		Vector3d z = v;
		double r = 0;
		double dr = 1;
		int power = 8;
		for (int i = 0; i < MAX_ITER; i++) {
			r = z.len();
			if (r > BAILOUT)break;

			double r2, r4, r8;
			r2 = r*r;
			r4 = r2*r2;// r4
			r8 = r4*r4;// fast power

			//dr = r4*r2*r*power*dr + 1;
			dr = r4*r2*r*power*dr;

			double theta = asin(z.z / r);
			double phi = atan2(z.y, z.x);

			theta = 8 * theta;
			phi = 8 * phi;

			// convert to cartessian coordinate
			z = Vector3d(r8 * cos(theta)*cos(phi), r8 * cos(theta)*sin(phi), r8 * sin(theta));
			z = z + v;
		}
		return 0.5*log(r)*r / dr;
	}
	Donkey() {
		printf("[");
		for (int i = 0; i < den; i++) {
			if (i % 20 == 0)printf(">");
			for (int j = 0; j < den; j++) {
				//upper face
				up_face[i][j].set(diameter*(i-den/2), diameter*(j-den/2),-2.5);
				double dist;
				for (int k = 0; k < STEP_LIMIT; k++) {
					dist = DE(up_face[i][j]);
					if (dist < EPS||up_face[i][j].z>0)break;
					up_face[i][j].z += dist;
				}
				if (dist < 0.1) {
					pts_cloud.push_back(up_face[i][j]);
				}
			}
		}
		for (int i = 0; i < den; i++) {
			if (i % 20 == 0)printf(">");
			for (int j = 0; j < den; j++) {
				//down face
				up_face[i][j].set(diameter*(i - den / 2), diameter*(j - den / 2), 2.5);
				double dist;
				for (int k = 0; k < STEP_LIMIT; k++) {
					dist = DE(up_face[i][j]);
					if (dist < EPS||up_face[i][j].z<0)break;
					up_face[i][j].z -= dist;
				}
				if (dist < 0.1) {
					pts_cloud.push_back(up_face[i][j]);
				}
			}
		}
		printf("]\n");
	}
};

// Changable Variable
bool cull_face = false;
int display_mode = 0;
Shell* pts_ptr;

// Shapes scale
float g_Zoom = 2.0f;
// Shape orientation (stored as a quaternion)
float g_Rotation[] = { 0.0f, 0.0f, 0.0f, 1.0f };
// Auto rotate
int g_AutoRotate = 0;
int g_RotateTime = 0;
float g_RotateStart[] = { 0.0f, 0.0f, 0.0f, 1.0f };
// Shapes material
float g_MatAmbient[] = { 0.5f, 0.0f, 0.0f, 1.0f };
float g_MatDiffuse[] = { 1.0f, 1.0f, 0.0f, 1.0f };
// Light parameter
float g_LightMultiplier = 4.0f;
float g_LightDirection[] = { -0.57735f, -0.57735f, -0.57735f };

// Routine to set a quaternion from a rotation axis and angle
// ( input axis = float[3] angle = float  output: quat = float[4] )
void SetQuaternionFromAxisAngle(const float *axis, float angle, float *quat)
{
	float sina2, norm;
	sina2 = (float)sin(0.5f * angle);
	norm = (float)sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
	quat[0] = sina2 * axis[0] / norm;
	quat[1] = sina2 * axis[1] / norm;
	quat[2] = sina2 * axis[2] / norm;
	quat[3] = (float)cos(0.5f * angle);
}

// Routine to convert a quaternion to a 4x4 matrix
// ( input: quat = float[4]  output: mat = float[4*4] )
void ConvertQuaternionToMatrix(const float *quat, float *mat)
{
	float yy2 = 2.0f * quat[1] * quat[1];
	float xy2 = 2.0f * quat[0] * quat[1];
	float xz2 = 2.0f * quat[0] * quat[2];
	float yz2 = 2.0f * quat[1] * quat[2];
	float zz2 = 2.0f * quat[2] * quat[2];
	float wz2 = 2.0f * quat[3] * quat[2];
	float wy2 = 2.0f * quat[3] * quat[1];
	float wx2 = 2.0f * quat[3] * quat[0];
	float xx2 = 2.0f * quat[0] * quat[0];
	mat[0 * 4 + 0] = -yy2 - zz2 + 1.0f;
	mat[0 * 4 + 1] = xy2 + wz2;
	mat[0 * 4 + 2] = xz2 - wy2;
	mat[0 * 4 + 3] = 0;
	mat[1 * 4 + 0] = xy2 - wz2;
	mat[1 * 4 + 1] = -xx2 - zz2 + 1.0f;
	mat[1 * 4 + 2] = yz2 + wx2;
	mat[1 * 4 + 3] = 0;
	mat[2 * 4 + 0] = xz2 + wy2;
	mat[2 * 4 + 1] = yz2 - wx2;
	mat[2 * 4 + 2] = -xx2 - yy2 + 1.0f;
	mat[2 * 4 + 3] = 0;
	mat[3 * 4 + 0] = mat[3 * 4 + 1] = mat[3 * 4 + 2] = 0;
	mat[3 * 4 + 3] = 1;
}


// Routine to multiply 2 quaternions (ie, compose rotations)
// ( input q1 = float[4] q2 = float[4]  output: qout = float[4] )
void MultiplyQuaternions(const float *q1, const float *q2, float *qout)
{
	float qr[4];
	qr[0] = q1[3] * q2[0] + q1[0] * q2[3] + q1[1] * q2[2] - q1[2] * q2[1];
	qr[1] = q1[3] * q2[1] + q1[1] * q2[3] + q1[2] * q2[0] - q1[0] * q2[2];
	qr[2] = q1[3] * q2[2] + q1[2] * q2[3] + q1[0] * q2[1] - q1[1] * q2[0];
	qr[3] = q1[3] * q2[3] - (q1[0] * q2[0] + q1[1] * q2[1] + q1[2] * q2[2]);
	qout[0] = qr[0]; qout[1] = qr[1]; qout[2] = qr[2]; qout[3] = qr[3];
}

// Return elapsed time in milliseconds
int GetTimeMs()
{
	return glutGet(GLUT_ELAPSED_TIME);
}

void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);				// Select The Projection Matrix
	glLoadIdentity();
	gluPerspective(40, (double)w / h, 1, 10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);
	glTranslatef(0, 0.6f, -1);

	// Send the new window size to AntTweakBar
	TwWindowSize(w, h);
}

// Function called at exit
void myTerminate(void)
{
	TwTerminate();
}

//  Callback function called when the 'AutoRotate' variable value of the tweak bar has changed
void TW_CALL SetAutoRotateCB(const void *value, void *clientData)
{
	(void)clientData; // unused

	g_AutoRotate = *(const int *)value; // copy value to g_AutoRotate
	if (g_AutoRotate != 0)
	{
		// init rotation
		g_RotateTime = GetTimeMs();
		g_RotateStart[0] = g_Rotation[0];
		g_RotateStart[1] = g_Rotation[1];
		g_RotateStart[2] = g_Rotation[2];
		g_RotateStart[3] = g_Rotation[3];

		// make Rotation variable read-only
		TwDefine(" TweakBar/ObjRotation readonly ");
	}
	else
		// make Rotation variable read-write
		TwDefine(" TweakBar/ObjRotation readwrite ");
}

//  Callback function called by the tweak bar to get the 'AutoRotate' value
void TW_CALL GetAutoRotateCB(void *value, void *clientData)
{
	(void)clientData; // unused
	*(int *)value = g_AutoRotate; // copy g_AutoRotate to value
}


static const float vertex_list[][3] =
{
	-0.5f, -0.5f, -0.5f,
	0.5f, -0.5f, -0.5f,
	-0.5f, 0.5f, -0.5f,
	0.5f, 0.5f, -0.5f,
	-0.5f, -0.5f, 0.5f,
	0.5f, -0.5f, 0.5f,
	-0.5f, 0.5f, 0.5f,
	0.5f, 0.5f, 0.5f,
};

// 将要使用的顶点的序号保存到一个数组里面 

static const GLint index_list[][2] =
{
	{ 0, 1 },
	{ 2, 3 },
	{ 4, 5 },
	{ 6, 7 },
	{ 0, 2 },
	{ 1, 3 },
	{ 4, 6 },
	{ 5, 7 },
	{ 0, 4 },
	{ 1, 5 },
	{ 7, 3 },
	{ 2, 6 }
};

// 绘制立方体

void DrawCube(void)
{
	int i, j;

	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	for (i = 0; i<12; ++i) // 12 条线段

	{
		for (j = 0; j<2; ++j) // 每条线段 2个顶点

		{
			glVertex3fv(vertex_list[index_list[i][j]]);
		}
	}
	glEnd();
}

void drawScene() {
	float v[4]; // will be used to set light parameters
	float mat[4 * 4]; // rotation matrix

	// settings
	switch (display_mode) {
	case 2:glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);  break;
	case 1:glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
	default:glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
	}

	if (cull_face) {
		glEnable(GL_CULL_FACE);
	}
	else {
		glDisable(GL_CULL_FACE);
	}

	// Clear frame buffer
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);

	// Set light
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	v[0] = v[1] = v[2] = g_LightMultiplier*0.4f; v[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_AMBIENT, v);
	v[0] = v[1] = v[2] = g_LightMultiplier*0.8f; v[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_DIFFUSE, v);
	v[0] = -g_LightDirection[0]; v[1] = -g_LightDirection[1]; v[2] = -g_LightDirection[2]; v[3] = 0.0f;
	glLightfv(GL_LIGHT0, GL_POSITION, v);

	// Set material
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, g_MatAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, g_MatDiffuse);

	// Rotate and draw shape
	glPushMatrix();
	glTranslatef(0.5f, -0.3f, 0.0f);
	if (g_AutoRotate)
	{
		float axis[3] = { 0, 1, 0 };
		float angle = (float)(GetTimeMs() - g_RotateTime) / 1000.0f;
		float quat[4];
		SetQuaternionFromAxisAngle(axis, angle, quat);
		MultiplyQuaternions(g_RotateStart, quat, g_Rotation);
	}
	ConvertQuaternionToMatrix(g_Rotation, mat);
	glMultMatrixf(mat);
	glScalef(g_Zoom, g_Zoom, g_Zoom);


	// display
	vector<Vector3d> &points = pts_ptr->pts_cloud;
	vector<TriangleIndices> &facets = pts_ptr->f_indices;

#define DRAWTRIANGLE 1

#if(DRAWTRIANGLE)
	glBegin(GL_TRIANGLES);
	for (int i = 0; i < facets.size(); i++) {
		int ix[3] = { facets[i].ix[0],facets[i].ix[1],facets[i].ix[2] };
		Vector3d v1(points[ix[0]], points[ix[1]]);
		Vector3d v2(points[ix[1]], points[ix[2]]);
		Vector3d norm_v = v1*v2;
		glNormal3dv(&(norm_v.x));
		for (int j = 0; j < 3; j++) {
			glVertex3dv(&(points[ix[j]].x));
		}
	}
	glEnd();
#else
	glBegin(GL_POINTS);
	for (int i = 0; i < points.size(); i++) {
		glVertex3dv(&(points[i].x));
	}
	glEnd();
#endif

	glPopMatrix();

	// Draw tweak bars
	TwDraw();

	// Present frame buffer
	glutSwapBuffers();

	// Recall Display at next frame
	glutPostRedisplay();
}

void keyboardHandler(unsigned char key, int x, int y) {
	if (!TwEventKeyboardGLUT(key, x, y)) {
		switch (key) {
		case 27:exit(0); break;
		case 'a':case 'A':display_mode = (display_mode + 1) % 3; break;
		case 'b':case 'B':cull_face = !cull_face; break;
		}
	}
}

int main(int argc, char** argv) {
	TwBar *bar; // Pointer to the tweak bar
	float axis[] = { 0.7f, 0.7f, 0.0f }; // initial model rotation
	float angle = 0.8f;

	int width = 800;
	int height = 600;
	
	Geodisc mandelbulb;
	pts_ptr = &mandelbulb;
	mandelbulb.saveSTL();
	

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(width, height);
	glutCreateWindow("OpenGL: Fractal");

	glutDisplayFunc(drawScene);
	glutReshapeFunc(reshape);
	atexit(myTerminate);

	// Initialize AntTweakBar
	TwInit(TW_OPENGL, NULL);

	// Set GLUT event callbacks
	// - Directly redirect GLUT mouse button events to AntTweakBar
	glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
	// - Directly redirect GLUT mouse motion events to AntTweakBar
	glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	// - Directly redirect GLUT mouse "passive" motion events to AntTweakBar (same as MouseMotion)
	glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	// - Directly redirect GLUT key events to AntTweakBar
	glutKeyboardFunc(keyboardHandler);
	// - Directly redirect GLUT special key events to AntTweakBar
	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
	// - Send 'glutGetModifers' function pointer to AntTweakBar;
	//   required because the GLUT key event functions do not report key modifiers states.
	TwGLUTModifiersFunc(glutGetModifiers);

	// Create a tweak bar
	bar = TwNewBar("TweakBar");
	TwDefine(" GLOBAL help='This example shows how to integrate AntTweakBar with GLUT and OpenGL.' "); // Message added to the help bar.
	TwDefine(" TweakBar size='200 400' color='96 216 224' "); // change default tweak bar size and color

															  // Add 'g_Zoom' to 'bar': this is a modifable (RW) variable of type TW_TYPE_FLOAT. Its key shortcuts are [z] and [Z].
	TwAddVarRW(bar, "Zoom", TW_TYPE_FLOAT, &g_Zoom,
		" min=0.01 max=4.0 step=0.01 keyIncr=z keyDecr=Z help='Scale the object (1=original size).' ");

	// Add 'g_Rotation' to 'bar': this is a variable of type TW_TYPE_QUAT4F which defines the object's orientation
	TwAddVarRW(bar, "ObjRotation", TW_TYPE_QUAT4F, &g_Rotation,
		" label='Object rotation' opened=true help='Change the object orientation.' ");

	// Add callback to toggle auto-rotate mode (callback functions are defined above).
	TwAddVarCB(bar, "AutoRotate", TW_TYPE_BOOL32, SetAutoRotateCB, GetAutoRotateCB, NULL,
		" label='Auto-rotate' key=space help='Toggle auto-rotate mode.' ");

	// Add 'g_LightMultiplier' to 'bar': this is a variable of type TW_TYPE_FLOAT. Its key shortcuts are [+] and [-].
	TwAddVarRW(bar, "Multiplier", TW_TYPE_FLOAT, &g_LightMultiplier,
		" label='Light booster' min=0.1 max=4 step=0.02 keyIncr='+' keyDecr='-' help='Increase/decrease the light power.' ");

	// Add 'g_LightDirection' to 'bar': this is a variable of type TW_TYPE_DIR3F which defines the light direction
	TwAddVarRW(bar, "LightDir", TW_TYPE_DIR3F, &g_LightDirection,
		" label='Light direction' opened=true help='Change the light direction.' ");

	// Add 'g_MatAmbient' to 'bar': this is a variable of type TW_TYPE_COLOR3F (3 floats color, alpha is ignored)
	// and is inserted into a group named 'Material'.
	TwAddVarRW(bar, "Ambient", TW_TYPE_COLOR3F, &g_MatAmbient, " group='Material' ");

	// Add 'g_MatDiffuse' to 'bar': this is a variable of type TW_TYPE_COLOR3F (3 floats color, alpha is ignored)
	// and is inserted into group 'Material'.
	TwAddVarRW(bar, "Diffuse", TW_TYPE_COLOR3F, &g_MatDiffuse, " group='Material' ");

	// Store time
	g_RotateTime = GetTimeMs();
	// Init rotation
	SetQuaternionFromAxisAngle(axis, angle, g_Rotation);
	SetQuaternionFromAxisAngle(axis, angle, g_RotateStart);

	glutMainLoop();
	return 0;
}


