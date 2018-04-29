#define _CRT_SECURE_NO_WARNINGS

#include <vector>
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <opencv.hpp>
#include <GL/glut.h>

#pragma warning(disable: 4800)
#include <mpirxx.h>
#pragma warning(default: 4800)

#include "type.h"

using namespace cv;
using namespace std;

extern double startX1;
extern double startX2;
extern double startY1;
extern double startY2;

extern mpf_class center_x;
extern mpf_class center_y;
extern int width, height;
extern int maxitr;
extern int movx, movy;
extern double bailout;

extern double radius;
extern double rotation;
extern DRAWMTD draw_mtd;
extern COLORMD c_mode;

extern int precision, depth;
extern vector <Color> pixels;

extern int movx, movy;

extern char j_form[];

char filename[MAX_PATH];
typedef enum {IMSAVE,VGSAVE,LOAD} FOPMODE;

bool selectDialog(const char* prompt, const FOPMODE fmode)
{
	OPENFILENAME ofn;
	ZeroMemory(&filename, sizeof(filename));
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
	
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = prompt;

	BOOL result;

	switch (fmode) {
	case IMSAVE:
		ofn.lpstrFilter = "JPG File (*.jpg)\0*.jpg\0BMP File (*.bmp)\0*.bmp\0PNG File (*.png)\0*.png\0\0";
		ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = "jpg";
		result = GetSaveFileNameA(&ofn);
		break;
	case VGSAVE:
		ofn.lpstrFilter = "Fractal File (*.frc)\0*.frc\0\0";
		ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = "frc";
		result = GetSaveFileNameA(&ofn);
		break;
	case LOAD:
		ofn.lpstrFilter = "Fractal File (*.frc)\0*.frc\0\0";
		ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;
		result = GetOpenFileNameA(&ofn);
		break;
	}

	if (result)
	{
		cout << "You chose the file \"" << filename << "\"\n";
		return true;
	}
	else
	{
		// All this stuff below is to tell you exactly how you messed up above. 
		// Once you've got that fixed, you can often (not always!) reduce it to a 'user cancelled' assumption.
		switch (CommDlgExtendedError())
		{
		case CDERR_DIALOGFAILURE: cout << "CDERR_DIALOGFAILURE\n";   break;
		case CDERR_FINDRESFAILURE: cout << "CDERR_FINDRESFAILURE\n";  break;
		case CDERR_INITIALIZATION: cout << "CDERR_INITIALIZATION\n";  break;
		case CDERR_LOADRESFAILURE: cout << "CDERR_LOADRESFAILURE\n";  break;
		case CDERR_LOADSTRFAILURE: cout << "CDERR_LOADSTRFAILURE\n";  break;
		case CDERR_LOCKRESFAILURE: cout << "CDERR_LOCKRESFAILURE\n";  break;
		case CDERR_MEMALLOCFAILURE: cout << "CDERR_MEMALLOCFAILURE\n"; break;
		case CDERR_MEMLOCKFAILURE: cout << "CDERR_MEMLOCKFAILURE\n";  break;
		case CDERR_NOHINSTANCE: cout << "CDERR_NOHINSTANCE\n";     break;
		case CDERR_NOHOOK: cout << "CDERR_NOHOOK\n";          break;
		case CDERR_NOTEMPLATE: cout << "CDERR_NOTEMPLATE\n";      break;
		case CDERR_STRUCTSIZE: cout << "CDERR_STRUCTSIZE\n";      break;
		case FNERR_BUFFERTOOSMALL: cout << "FNERR_BUFFERTOOSMALL\n";  break;
		case FNERR_INVALIDFILENAME: cout << "FNERR_INVALIDFILENAME\n"; break;
		case FNERR_SUBCLASSFAILURE: cout << "FNERR_SUBCLASSFAILURE\n"; break;
		default: cout << "You cancelled.\n";
		}
	}
	return false;
}

void saveImage() {
	if (!selectDialog("Saving your fractal", IMSAVE))return;
	Mat mat(height, width, CV_8UC3);
	for (int i = 0; i < mat.rows; ++i) {
		for (int j = 0; j < mat.cols; ++j) {
			Vec3b& rgb = mat.at<Vec3b>(i, j);
			rgb[0] = pixels[i*width + j].b;
			rgb[1] = pixels[i*width + j].g;
			rgb[2] = pixels[i*width + j].r;
		}
	}
	try {
		imwrite(filename, mat);
	}
	catch (runtime_error& ex) {
		cout << ex.what() << endl;
	}
}

template<typename T>
int saving(T& x, FILE* fp) {
	return fwrite(&x, sizeof(x), 1, fp);
}

int saving(char s[], FILE* fp, int len) {
	return fwrite(s, len, 1, fp);
}

template<typename T>
int loading(T& x, FILE* fp) {
	return fread(&x, sizeof(x), 1, fp);
}

int loading(char s[], FILE* fp, int len) {
	return fread(s, len, 1, fp);
}

void saveVecgram() {
	if (!selectDialog("Saving your fractal", VGSAVE))return;
	FILE *fp = fopen(filename, "wb");
	int len = strlen(j_form);
	saving(len, fp);
	saving(j_form, fp, len);
	saving(startX1, fp);
	saving(startX2, fp);
	saving(startY1, fp);
	saving(startY2, fp);

	saving(precision, fp);
	saving(depth, fp);

	saving(width, fp);
	saving(height, fp);

	saving(depth, fp);
	saving(maxitr, fp);
	saving(bailout, fp);

	saving(radius, fp);

	saving(c_mode, fp);
	saving(draw_mtd, fp);
	
	saving(rotation, fp);

	// hack mpf, dirty code
	__mpf_struct X(*center_x.get_mpf_t());
	saving(X._mp_prec, fp);
	saving(X._mp_size, fp);
	saving(X._mp_exp, fp);
	for (int i = 0; i < abs(X._mp_size); i++) {
		saving(X._mp_d[i], fp);
	}
	X = *center_y.get_mpf_t();
	saving(X._mp_prec, fp);
	saving(X._mp_size, fp);
	saving(X._mp_exp, fp);
	for (int i = 0; i < abs(X._mp_size); i++) {
		saving(X._mp_d[i], fp);
	}
	// end hack

	fclose(fp);
}

void loadVecgram() {
	if (!selectDialog("Loading your fractal", LOAD))return;
	FILE *fp = fopen(filename, "rb");
	int len;
	loading(len, fp);
	loading(j_form, fp, len);
	loading(startX1, fp);
	loading(startX2, fp);
	loading(startY1, fp);
	loading(startY2, fp);

	loading(precision, fp);
	loading(depth, fp);

	loading(width, fp);
	loading(height, fp);

	loading(depth, fp);
	loading(maxitr, fp);
	loading(bailout, fp);

	loading(radius, fp);

	loading(c_mode, fp);
	loading(draw_mtd, fp);

	loading(rotation, fp);

	// hack mpf, dirty code
	// initial center x
	__mpf_struct X;
	loading(X._mp_prec, fp);
	loading(X._mp_size, fp);
	loading(X._mp_exp, fp);
	X._mp_d = new mp_limb_t[abs(X._mp_size)];
	for (int i = 0; i < abs(X._mp_size); i++) {
		loading(X._mp_d[i], fp);
	}
	mpf_t Z = { X };
	center_x = mpf_class(Z);
	delete [] X._mp_d;

	// initial center y
	loading(X._mp_prec, fp);
	loading(X._mp_size, fp);
	loading(X._mp_exp, fp);
	X._mp_d = new mp_limb_t[abs(X._mp_size)];
	for (int i = 0; i < abs(X._mp_size); i++) {
		loading(X._mp_d[i], fp);
	}
	Z[0] = X;
	center_y = mpf_class(Z);
	delete[] X._mp_d;

	// end hack

	fclose(fp);
	glutReshapeWindow(width,height);
}

