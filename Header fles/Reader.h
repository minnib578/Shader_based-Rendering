#pragma once
#include "GL\glew.h"
#include "GL\freeglut.h"
#include <vector>
#include <string>
using namespace std;
class Reader {
public:
	Reader(string filename);
	void Draw_POINT();
	void Draw_LINE();
	void Draw_TRIANGLE();
	float bounding;
	vector<GLfloat> vertexs;
	vector<GLint> indices;
private:
	vector<vector<GLfloat>>Vertexs;
	vector<vector<GLint>>Face;

};