#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <complex>
#include <vector>
#include <string>
#include <map>

// user's cpp
#include "fio.h"

// MPFR: arbitrary float arithematic
#pragma warning(disable: 4800)
#include <mpirxx.h>
#pragma warning(default: 4800)
// Toolbar
#include <AntTweakBar.h>
// User defined class
#include "type.h"

using namespace std;

// most fractal lie in this field
double startX1 = -2.2;
double startX2 =  2.2;
double startY1 = -1.4;
double startY2 =  1.4;

int precision = 128;

// canvas size
int height;
int width;

// iteration control variable
int depth;      // store list depth in peturbation method
int maxitr;     // "const" maximum iteration limit
int actu_itr;   // actual iteration number
double radius;  // radius of every pixel
double bailout; // use for julia set escape

mpf_class center_x;
mpf_class center_y;

// color && position
vector <Color> pixels;
vector <complex<double>> cpx_list;

DRAWMTD draw_mtd;
COLORMD c_mode;

// formula
char j_form[100];
double rotation;

bool flag;



