#ifndef _COLOR_H_
#define _COLOR_H_
#include <complex>
class Color {
public:
	unsigned char r, g, b, a;
public:
	Color(unsigned char rr = 0, unsigned char gg = 0, unsigned char bb = 0) {
		r = rr;
		g = gg;
		b = bb;
		a = 1;
	}
	Color& operator = (const Color& rhs) {
		r = rhs.r;
		g = rhs.g;
		b = rhs.b;
		return *this;
	}
	bool operator == (const Color& rhs) {
		return(r == rhs.r  && g == rhs.g && b == rhs.b);
	}
	bool operator != (const Color& rhs) {
		return !(*this == rhs);
	}
};

typedef enum { SIM_M, SIM_J, PTB_M } DRAWMTD;
typedef enum { RGB, YUV, HSL } COLORMD;

class Old {
public:
	int width, height;
	int maxitr;
	double bailout;

	double radius;
	double rotation;
	DRAWMTD draw_mtd;
	COLORMD c_mode;

	int precision, depth;

	char j_form[100];

	bool flag;
};
#endif // !_COLOR_H_

