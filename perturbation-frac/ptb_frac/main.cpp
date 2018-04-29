#include "global.h"
double cx, cy;

double inline hue2RGB(double v1, double v2, double vh) {
	if (vh < 0)vh += 1;
	if (vh > 1)vh -= 1;
	if (6 * vh < 1)return v1 + (v2 - v1)*6.0*vh;
	if (2 * vh < 1)return v2;
	if (3 * vh < 2)return v1 + (v2 - v1)*((2 / 3.0) - vh) * 6;
	return v1;
}

// determined pixel color according to iteration counts and escape distance
void inline bepaint(Color &pix, int cnt, double dist, int px = 0, int py = 0) {
	double r, g, b;
	double p, q;
	double ch, cs, cl;
	double y, u, v; // 0~1

	switch (c_mode) {
	case RGB:
		if (cnt == actu_itr) {
			pix = Color(0, 50, 50);
		}
		else {
			pix = Color((bailout/dist)*200+55, 200.0*cnt / actu_itr + 55, 0);
		}
		break;
	case HSL: // smoothly color
		// h:0~1, s:0~1, L:0~1
		if (cnt == actu_itr) {
			cl = cs = ch = 0;
		}
		else {
			cl = 0.6;
			cs = 0.9;
			//double log_zn = log(dist) / 2;
			//double nu = log(log_zn / log(2)) / log(2);// distance estimation
			double log_zn = log(dist);
			double nu = log(log_zn) / log(2);
			ch = (double)(cnt + 1 - nu) / actu_itr;
		}

		if (cs <= 0) {
			r = g = b = cl;
			pix = Color(r * 255, g * 255, b * 255);
			break;
		}

		q = cl < 0.5 ? cl*(cl + cs) : cl + cs - cl*cs;
		p = 2 * cl - q;
		r = hue2RGB(p, q, ch + 1 / 3.0);
		g = hue2RGB(p, q, ch);
		b = hue2RGB(p, q, ch - 1 / 3.0);

		pix = Color(r*255, g*255, b*255);
		break;
	case YUV:
		if (cnt == actu_itr) {
			v = 0;
			u = 0;
			y = 0;
		}
		else {
			u = cnt / (double)actu_itr - 0.5;
			v = ((int)(dist*cnt)%3)/3.0 - 0.5;
			y = 0.4;
		}
		
		r = y + 1.13983*v;
		g = y - 0.39465*u - 0.5806*v;
		b = y + 2.032*u;
		
		r *= 255;
		g *= 255;
		b *= 255;

		pix = Color(r, g, b);
		break;
	default:break;
	}
}

// get the local coordinate of pixels[i], according to center
complex<double> inline getLocalPos(int i) {
	const double PI = 3.1415926;
	double x = radius*(2 * (i % width) - width);
	double y = radius*(2 * (i / width) - height);
	// rotation transform
	double rx = cos(rotation*PI / 180.0)*x - sin(rotation*PI / 180.0)*y;
	double ry = sin(rotation*PI / 180.0)*x + cos(rotation*PI / 180.0)*y;
	return complex<double>(rx, ry);
}

// store 2 * Xn sequence, perturbation
void calcCenter() {
	mpf_class re(center_x, precision);
	mpf_class im(center_y, precision);
	mpf_class tmp(0, precision);
	int i;
	for (i = 0; i < depth; i++) {
		tmp = re + re;
		cpx_list[i] = complex<double>(tmp.get_d(), 2 * im.get_d());
		if (re > 1024 || im > 1024 || re < -1024 || im < -1024) break; // if too large, >1024
		re = re*re - im*im + center_x;
		im = tmp * im + center_y;
	}
	actu_itr = i > maxitr ? maxitr : i;
}

// perturbation using stored result
void calcNeighbor() {
	for (int i = 0; i < height*width; i++) {
		complex<double> d0 = getLocalPos(i);
		complex<double> dn = d0;
		int cnt;
		double dist;
		for (cnt = 0; cnt < actu_itr; cnt++) {
			dn *= cpx_list[cnt] + dn;
			dn += d0;
			dist = norm(cpx_list[cnt + 1] * 0.5 + dn);
			if (dist > 4) break;
		}
		bepaint(pixels[i], cnt, dist);
	}
}

// simple escape time method
void simpleMSet() {
	complex<double> center(center_x.get_d(), center_y.get_d());
	actu_itr = maxitr;
	for (int i = 0; i < height*width; i++) {
		complex<double> c = getLocalPos(i);
		c += center;
		complex<double> z = c;
		int cnt;
		double dist;
		for (cnt = 0; cnt < actu_itr; cnt++) {
			z = z*z + c;
			dist = norm(z);
			if (dist > 4) break;
		}
		bepaint(pixels[i], cnt, dist);
	}
}

void ptbMSet() {
	calcCenter();
	calcNeighbor();
}

inline void juliaIter(complex<double> &z, map<int, complex<double>> &all, int max = 0) {
	if (all.empty()) {
		z = z*z + complex<double>(-0.1, 0.651);
	}
	else {
		complex<double> res(0);
		for (int i = max; i > 0; i--) {
			if (all.count(i) == 1) {
				res = res + all[i];
			}
			res *= z;
		}
		if (all.count(0) == 1)res += all[0];
		z = res;
	}
}

void simpleJulia() {
	complex<double> center(center_x.get_d(), center_y.get_d());
	actu_itr = maxitr;

	// parsing julia formula
	char* pch;
	char jcpy[100];
	strcpy(jcpy, j_form);
	map<int, complex<double>>all;
	complex<double> cof;
	int exp, max = 0;
	pch = strtok(jcpy, "+^z*() "); // seperator
	for (int i = 0; pch != NULL; i++) {
		if (i % 3 == 0)cof.real(atof(pch));
		else if (i % 3 == 1)cof.imag(atof(pch));
		else {
			exp = atoi(pch);
			if (max < exp) max = exp;
			all[exp] = cof;
		}
		pch = strtok(NULL, "+^z*() ");
	}

	for (int i = 0; i < height*width; i++) {
		complex<double> z = getLocalPos(i);
		z += center; // transfer to global cord
		int cnt;
		double dist;
		for (cnt = 0; cnt < actu_itr; cnt++) {
			juliaIter(z,all,max);
			dist = norm(z);
			if (dist > bailout) break;
		}
		bepaint(pixels[i], cnt, dist);
	}
}

bool genFractal() {
	static Old old;
	while (1) { // if nothing change return
		if (old.flag != flag)break;
		if (old.radius != radius)break;
		if (old.width != width)break;
		if (old.height != height)break;
		if (old.maxitr != maxitr)break;
		if (old.bailout != bailout)break;
		if (old.rotation != rotation)break;
		if (old.draw_mtd != draw_mtd)break;
		if (old.c_mode != c_mode)break;
		if (!strcmp(j_form, old.j_form))break;
		return false;
	}
	// copying...
	old.width = width;
	old.height = height;
	old.maxitr = maxitr;
	old.bailout = bailout;
	old.radius = radius;
	old.rotation = rotation;
	old.draw_mtd = draw_mtd;
	old.c_mode = c_mode;
	old.flag = flag;
	strcpy(old.j_form, j_form);
	// end copy

	switch (draw_mtd)
	{
	case SIM_M:simpleMSet(); break;
	case SIM_J:simpleJulia(); break;
	case PTB_M:ptbMSet(); break;
	}

#define DEBUG 0
#if(DEBUG)
	int mid;
	Color fc = Color(255, 255, 255);

	mid = height/2 * width + width / 2;
	pixels[mid + 1] = pixels[mid] = fc;

	mid = (height / 2 + 1) * width + width / 2;
	pixels[mid + 1] = pixels[mid] = fc;

	mid = height / 4 * width + width / 4;
	for (int i = 0; i < width / 2; i++)
		pixels[mid + i] = fc;

	mid = 3 * height / 4 * width + width / 4;
	for (int i = 0; i < width / 2; i++)
		pixels[mid + i] = fc;

	mid = height / 4 * width + width / 4;
	for (int i = 0; i < height / 2; i++)
		pixels[mid + i*width] = fc;

	mid = height / 4 * width + 3 * width / 4;
	for (int i = 0; i < height / 2; i++)
		pixels[mid + i*width] = fc;
#endif
	return true;
}

void zoomReset() {
	center_x = (startX1 + startX2) / 2.0;
	center_y = (startY1 + startY2) / 2.0;
	radius = (startY2 - startY1) / height / 2.0;
	rotation = 0;
	flag = !flag;
}

void zoomMove(int x, int y) {
	center_x += (2 * x - width)*radius;
	center_y -= (2 * y - height)*radius;
	cx = center_x.get_d();
	cy = center_y.get_d();

}

void zoomIn() {
	radius /= 2;
}

void zoomOut() {
	radius *= 2;
}

// display function
void drawScene(void)
{
	if(genFractal()) // determined whether to redraw it
		glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
	TwDraw(); // Draw tweak bars
	glutSwapBuffers();
	glutPostRedisplay();
}

void reshape(int wid, int hei)
{
	radius = radius * wid / width;

	width = wid;
	height = hei;

	pixels.resize(height*width);

	genFractal();

	// Send the new window size to AntTweakBar
	TwWindowSize(wid, hei);
}

// Function called at exit
void Terminate(void)
{
	TwTerminate();
}

void globalInit() {
	// initial window size 
	width = 880;
	height = 560;

	depth = 1000;
	maxitr = 200;
	actu_itr = 200;
	bailout = 4;

	center_x = mpf_class((startX1 + startX2) / 2.0, precision);
	center_y = mpf_class((startY1 + startY2) / 2.0, precision);

	pixels.resize(height*width);
	radius = (startX2 - startX1) / (width * 2);
	cpx_list.resize(depth);

	c_mode = HSL;
	draw_mtd = SIM_M;

	rotation = 0;
}

void TW_CALL saveImg(const void *value, void *clientData) {
	saveImage();
}

void TW_CALL savePara(const void *value, void *clientData) {
	saveVecgram();
}

void TW_CALL loadPara(const void *value, void *clientData) {
	loadVecgram();
}

void TW_CALL nullFunc(void *value, void *clientData) {

}

void clickHandler(int button, int state, int x, int y) {
	if (!TwEventMouseButtonGLUT(button, state, x, y)) {
		if (state == GLUT_DOWN) {
			flag = !flag;
			switch (button) {
			case GLUT_LEFT_BUTTON:
				zoomMove(x,y);
				break;
			case GLUT_RIGHT_BUTTON:
				zoomIn();
				break;
			case GLUT_MIDDLE_BUTTON:
				zoomOut();
				break;
			}
		}
	}
}

void pressHandler(unsigned char key, int x, int y) {
	if (!TwEventKeyboardGLUT(key, x, y)) {
		switch (key) {
		case 27:exit(0); break;
		case 'q':zoomReset(); break;
		}
	}
}


int main(int argc, char *argv[])
{
	globalInit();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutInitWindowPosition(50, 50);
	glutInitWindowSize(width, height);
	glutCreateWindow("OpenGL: Fractal");
	glutDisplayFunc(drawScene);
	glutReshapeFunc(reshape);
	atexit(Terminate);  // Called after glutMainLoop ends

	TwInit(TW_OPENGL, NULL);
	glutMouseFunc(clickHandler);
	glutKeyboardFunc(pressHandler);
	glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
	TwGLUTModifiersFunc(glutGetModifiers);

	// create tweak bar
	TwBar *bar = TwNewBar("Parameter");
	TwDefine(" Parameter size='220 250' color='96 216 224' ");
	TwDefine(" GLOBAL help='ZJU CAD&LAB 416: Fractal' ");

	TwAddVarRW(bar, "Iteration Num", TW_TYPE_INT32, &maxitr,
		"min=10 max=1000 step=10 help='Change iteration number' ");

	TwAddVarRW(bar, "Bailout", TW_TYPE_DOUBLE, &bailout,
		"min=0.1 max=16.0 help='Change escape bailout' ");

	TwAddVarRW(bar, "Formula", TW_TYPE_CSSTRING(sizeof(j_form)), j_form,
		"help='Julia set generation formula' ");

	TwAddVarRW(bar, "Rotation", TW_TYPE_DOUBLE, &rotation,
		"help='set rotaion theta in degree' ");
	
	TwAddSeparator(bar, "", NULL);

	// color mode
	{
		TwEnumVal colorVal[3] = { { RGB, "RGB" },{ YUV, "YUV" },{ HSL, "HSL" } };
		TwType colorType = TwDefineEnum("Color Mode", colorVal, 3);
		TwAddVarRW(bar, "Color Mode", colorType, &c_mode, NULL);
	}

	// render type
	{
		TwEnumVal fracVal[3] = { { SIM_M, "Mandelbrot(sim)" },{ SIM_J, "Julia" },{ PTB_M, "Peturbation" } };
		TwType fracType = TwDefineEnum("Fractal Type", fracVal, 3);
		TwAddVarRW(bar, "Fractal Type", fracType, &draw_mtd, NULL);
	}

	TwAddSeparator(bar, "", NULL);

	TwAddVarRO(bar, "Width", TW_TYPE_INT32, &width, NULL);
	TwAddVarRO(bar, "Height", TW_TYPE_INT32, &height, NULL);
	TwAddVarRO(bar, "Radius", TW_TYPE_DOUBLE, &radius, NULL);

	TwAddVarRO(bar, "Center_X", TW_TYPE_DOUBLE, &cx, NULL);
	TwAddVarRO(bar, "Center_Y", TW_TYPE_DOUBLE, &cy, NULL);


	TwAddSeparator(bar, "", NULL);
	TwAddVarCB(bar, "Save image", TW_TYPE_BOOL32, saveImg, nullFunc, NULL, "key=i help='Save the current image'");
	TwAddVarCB(bar, "Save vgram", TW_TYPE_BOOL32, savePara, nullFunc, NULL, "key=o help='Save the parameter'");
	TwAddVarCB(bar, "Load vgram", TW_TYPE_BOOL32, loadPara, nullFunc, NULL, "key=p help='Load the parameter'");

	glutMainLoop();
	return 0;
}

