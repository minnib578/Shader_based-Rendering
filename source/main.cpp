#include <GL/glew.h>
#include <GL/freeglut.h>
#include<Wingdi.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream> 
#include <sstream>
#include"Reader.h"
#include <windows.h>
#include "utils.h"
#include"camera.h"
#include"MathHelper.h"
#include"normal.h"
#include<Windows.h>
#include <intrin.h>
HDC    hdc;
HGLRC  hglrc;
using namespace std;
bool reset_shader = true;
bool reset_fix = true;
bool stop = false;
//string file_name = "cactus.obj";
//string Cull = "Front";
//string plot = "Polygon";
static float near_cutplane = 0.1;
static float far_cutplane = 100.0;
static int old_x = 1;                                               // mouse initial coordinate 
static int old_y = 1;
static float window_w = 500;
static float window_h = 500;
static float mouse_sensitivity = 800;
static float Ini_offset = -3;

string filePath = "cactus.obj";                             // Initial file path
//string filePath = "bunny.obj";                             // Initial file path
Reader Model = Reader(filePath);
string primitive = "TRIANGLE";                                 // initial primitive rendering 
string colors = "R";                                        // initial color
GLenum mode = GL_CCW;                                       // initial rendering model dirction
Camera CAM;



int n_vertexs = Model.vertexs.size();
int n_indices = Model.indices.size();

GLuint vsID, fsID, pID, VBO, VAO, EBO;
GLfloat projectionMat[16], modelViewMat[16];

GLfloat red[4] = { 1.0f, 0.0f, 0.0f,1.0f };
GLfloat green[4] = { 0.0f, 1.0f, 0.0f,1.0f };
GLfloat blue[4] = { 0.0f, 0.0f, 1.0f,1.0f };
GLfloat white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat random[4] = { (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX, 1 };

GLfloat* transMatrix;	// Matrix for changing the position of the object
GLfloat* Ori_C;         // Origin of camera
GLfloat* M;				// The final model matrix M to change into world coordinates
GLfloat* V;				// The camera matrix (for position/rotation) to change into camera coordinates
GLfloat* P;				// The perspective matrix for the camera (to give the scene depth); initialize this ONLY ONCE!

bool light = true;
bool shade_model = true;
GLfloat Global_amb[4] = { 0.0,0.0, 0.0, 1.0 };
GLfloat am[4] = { 0.1,0.1,0.1, 1.0 };
GLfloat df[4] = { 1.0,1.0, 1.0, 1.0 };
GLfloat sp[4] = { 1.0,1.0, 1.0, 1.0 };
GLfloat position[4] = { 10.0, 10.0, 10.0, 1.0 };

void initMatrices() {
	// Allocate memory for the matrices and initialize them to the Identity matrix
	transMatrix = new GLfloat[16];	MathHelper::makeIdentity(transMatrix);
	M = new GLfloat[16];			MathHelper::makeIdentity(M);
	V = new GLfloat[16];			MathHelper::makeIdentity(V);
	P = new GLfloat[16];			MathHelper::makeIdentity(P);
	Ori_C = new GLfloat[3];			MathHelper::makeZero(Ori_C);
	float scale = Model.bounding;
	MathHelper::OC_Translate(Ori_C, 0.0f, 0.0f, -scale);
	MathHelper::makeTranslate(V, 0.0f, 0.0f, -scale);
}


void NEW() {
	string path = New_path();                                 	// get new file path 
	Model = Reader(path);
	n_vertexs = Model.vertexs.size();
	n_indices = Model.indices.size();
}


void readShaderFile(const GLchar* shaderPath, std::string& shaderCode)
{

	std::ifstream shaderFile;

	// ensures ifstream objects can throw exceptions:
	shaderFile.exceptions(std::ifstream::badbit);
	try
	{
		// Open files
		shaderFile.open(shaderPath);
		std::stringstream shaderStream;

		// Read file's buffer contents into streams
		shaderStream << shaderFile.rdbuf();

		// close file handlers
		shaderFile.close();

		// Convert stream into GLchar array
		shaderCode = shaderStream.str();

	}
	catch (std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

}


void initBufferObject()
{
	n_vertexs = Model.vertexs.size();
	n_indices = Model.indices.size();
	GLfloat* vertData = new GLfloat[n_vertexs];
	GLint* indices = new GLint[n_indices];

	for (int i = 0; i < n_vertexs; i++) {
		vertData[i] = Model.vertexs[i];
	}
	for (int i = 0; i < n_indices; i++) {
		indices[i] = Model.indices[i];
	}

	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, 2 * Model.vertexs.size() * sizeof(GLfloat), NULL, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, Model.vertexs.size() * sizeof(GLfloat), vertData);
	//glBufferSubData(GL_ARRAY_BUFFER, Model.vertexs.size() * sizeof(GLfloat), Model.vertexs.size() * sizeof(GLfloat), vertex_n_cactus);   //-------------------------------------------------------------------- change data


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, Model.indices.size() * sizeof(GLint), indices, GL_STATIC_DRAW);


	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)(Model.vertexs.size() * sizeof(GLfloat)));


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}


void setShaders()
{
	char* vs = NULL, * fs = NULL;

	std::string vertShaderString;
	std::string fragShaderString;
	vsID = glCreateShader(GL_VERTEX_SHADER);
	fsID = glCreateShader(GL_FRAGMENT_SHADER);
	readShaderFile("vertexshader.txt", vertShaderString);
	readShaderFile("fragshader.txt", fragShaderString);
	const GLchar* pVertShaderSource = vertShaderString.c_str();
	const GLchar* pFragShaderSource = fragShaderString.c_str();
	glShaderSource(vsID, 1, &pVertShaderSource, NULL);
	glShaderSource(fsID, 1, &pFragShaderSource, NULL);
	glCompileShader(vsID);
	glCompileShader(fsID);
	pID = glCreateProgram();
	glAttachShader(pID, vsID);
	glAttachShader(pID, fsID);
	glLinkProgram(pID);
	glUseProgram(pID);



}


void display_shader(void)
{
	float scale = Model.bounding;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(pID); 
	if (reset_shader == true) {
		MathHelper::makeIdentity(V);
		MathHelper::makeZero(Ori_C);
		MathHelper::OC_Translate(Ori_C, 0.0f, 0.0f, -scale);
		MathHelper::makeTranslate(V, 0.0f, 0.0f, -scale);
		near_cutplane = 0.1;
		far_cutplane = 50;
		reset_shader = false;
	}
	MathHelper::makeRotateCX(V, Ori_C, 0);
	MathHelper::makePerspectiveMatrix(P, 60.0f, window_w / window_h, near_cutplane, far_cutplane);
	GLuint modeViewMatLocation = glGetUniformLocation(pID, "modelviewMatrix");
	GLuint projectionMatLocation = glGetUniformLocation(pID, "projectionMatrix");
	GLuint transformMatLocation = glGetUniformLocation(pID, "transformationMatrix");
	GLuint colorID = glGetUniformLocation(pID, "objectcolor");

	GLuint am_ID = glGetUniformLocation(pID, "am");
	GLuint df_ID = glGetUniformLocation(pID, "df");
	GLuint sp_ID = glGetUniformLocation(pID, "sp");
	glUniform4fv(am_ID, 1, am);
	glUniform4fv(df_ID, 1, df);
	glUniform4fv(sp_ID, 1, sp);
	glUniformMatrix4fv(modeViewMatLocation, 1, GL_TRUE, V);
	glUniformMatrix4fv(projectionMatLocation, 1, GL_TRUE, P);

	//change color
	if (colors == "R") {
		glUniform4fv(colorID, 1, red);
	}
	else if (colors == "G") {
		glUniform4fv(colorID, 1, green);
	}
	else if (colors == "B") {
		glUniform4fv(colorID, 1, blue);
	}
	else if (colors == "W") {
		glUniform4fv(colorID, 1, white);
	}
	else if (colors == "RandomC") {
		glUniform4fv(colorID, 1, random);
	}

	glDeleteShader(vsID);
	glDeleteShader(fsID);
	glValidateProgram(pID);

	GLint validate = 0;
	glGetProgramiv(pID, GL_VALIDATE_STATUS, &validate);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(VAO);
	if (primitive == "POINT") {
		glDrawElements(GL_POINTS, 3*Model.vertexs.size(), GL_UNSIGNED_INT, 0);
	}
	else if (primitive == "LINE") {
		glDrawElements(GL_LINES, 3*Model.vertexs.size(), GL_UNSIGNED_INT, 0);
	}
	else {
		glDrawElements(GL_TRIANGLES, 3*Model.vertexs.size(), GL_UNSIGNED_INT, 0);
	}

	glBindVertexArray(0);
	glEnable(GL_CULL_FACE);                                                         // rendering objects whose polygon vertices were defined using CW (clockwise) and CCW (counter-clockwise) orientation
	glCullFace(GL_BACK);
	glPolygonMode(GL_FRONT, GL_FILL);
	glFrontFace(mode);
	glFlush();
	glutSwapBuffers();
}


void display_fixedpipe()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (colors == "R") { glColor3f(red[0], red[1], red[2]); }                                      // change model rendering color
	else if (colors == "G") { glColor3f(green[0], green[1], green[2]); }
	else if (colors == "B") { glColor3f(blue[0], blue[1], blue[2]); }
	else if (colors == "W") { glColor3f(white[0], white[1], white[2]); }
	else if (colors == "RandomC") { glColor3f(random[0], random[1], random[2]); }

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	float scale = Model.bounding;                                                      // choose propertive camera coordinate according to the new model dimension
	if (reset_fix == true) {                                                   // reset the camera to original point when change file and reset 
		CAM.eye = vector3f(0, 0, scale);
		CAM.at = vector3f(0, 0, -1);
		CAM.up = vector3f(0, 1, 0);
		near_cutplane = 0.1;
		far_cutplane = 50;
		reset_fix = false;
	}
	gluLookAt(CAM.eye.x, CAM.eye.y, CAM.eye.z, CAM.eye.x + CAM.at.x, CAM.eye.y + CAM.at.y, CAM.eye.z + CAM.at.z, CAM.up.x, CAM.up.y, CAM.up.z);

	if (primitive == "POINT") {                                                      // rendering the object using different kinds of primitives
		Model.Draw_POINT();
	}
	else if (primitive == "LINE") {
		Model.Draw_LINE();
	}
	else {
			Model.Draw_TRIANGLE();
		}
	
	glEnable(GL_CULL_FACE);                                                         // rendering objects whose polygon vertices were defined using CW (clockwise) and CCW (counter-clockwise) orientation
	glCullFace(GL_BACK);
	glPolygonMode(GL_FRONT, GL_FILL);
	glFrontFace(mode);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, window_w / window_h, near_cutplane, far_cutplane);
	glMatrixMode(GL_MODELVIEW);

	glFlush();
	glutSwapBuffers();
}


void MyReshape(int width, int height)
{
	glViewport(0, 0, width, height);
	window_w = width;
	window_h = height;
}


void myIdlex()
{
	glutPostRedisplay();
}


void keyboard(unsigned char key, int x, int y)
{
	if (key == 'Q') exit(0);
	if (key == 'r' || key == 'R') { reset_shader = true; reset_fix = true; }
	if (key == 'n') { NEW(); initBufferObject(); }


	if (key == '7') { colors = "R";}
	if (key == '8') { colors = "G";}
	if (key == '9') { colors = "B";}
	if (key == '0') { colors = "W";}
	if (key == 'c') {
		random[0] = (float)rand() / RAND_MAX;
		random[1] = (float)rand() / RAND_MAX;
		random[2] = (float)rand() / RAND_MAX;
		random[3] = (float)rand() / RAND_MAX;
		colors = "RandomC";
		cout << "Current Color:" << colors << endl;
	}
	if (key == 'p' && primitive == "TRIANGLE") { primitive = "LINE"; cout << "Rendering with Line" << endl; }
	else if (key == 'p' && primitive == "LINE") { primitive = "POINT"; cout << "Rendering with Point" << endl; }
	else if (key == 'p' && primitive == "POINT") { primitive = "TRIANGLE"; cout << "Rendering with Polygon" << endl; }


	float scale = Model.bounding/3;
	if (key == 'z' || key == 'Z') {
		CAM.F_B(scale * 0.1f );      
		MathHelper::OriC_TranslateZ(Ori_C, V, scale*0.1f);
	}
	if (key == 'w' || key == 'W') {
		CAM.F_B(scale * -0.1f );    
		MathHelper::OriC_TranslateZ(Ori_C, V, scale * -0.1f );
	}
	if (key == 'a' || key == 'A') {
		CAM.R_L(scale * 0.1f);     
		MathHelper::OriC_TranslateX(Ori_C, V, scale * -0.1f);
	}
	if (key == 's' || key == 'S') {
		CAM.R_L(scale * -0.1f );    
		MathHelper::OriC_TranslateX(Ori_C, V, scale * 0.1f);
	}
	if (key == 'U' || key == 'u') {
		CAM.U_D(scale * 0.1f); 
		MathHelper::OriC_TranslateY(Ori_C, V, scale * -0.1f);
	}
	if (key == 'd' || key == 'D') {
		CAM.U_D(scale * -0.1f);        
		MathHelper::OriC_TranslateY(Ori_C, V, scale * 0.1f);
	}

	
	if (key == 'n') {
		near_cutplane +=  0.1;
		cout << "The near cutplane is at " << near_cutplane << endl;
	}
	if (key == 'N') {
		near_cutplane -= 0.1;
		if (near_cutplane <= 0.1) near_cutplane =   0.1;
		cout << "The near cutplane is at " << near_cutplane << endl;
	}
	if (key == 'f') {
		far_cutplane += 1;
		cout << "The far cutplane is at " << far_cutplane << endl;
	}
	if (key == 'F') {
		far_cutplane -= 1;
		if (far_cutplane <= 0.1) far_cutplane =  0.1;
		cout << "The far cutplane is at " << far_cutplane << endl;
	}

	if (key == ' ') stop = !stop;
	if (stop)
		glutIdleFunc(NULL);
	else
		glutIdleFunc(myIdlex);
}


void MouseWheel(int wheel, int direction, int x, int y)
{
	float scale = Model.bounding;
	wheel = 0;
	if (direction == -1) {
		CAM.F_B(scale*0.2f);      //near
		MathHelper::OriC_TranslateZ(Ori_C, V, scale*0.2f);
	}
	else if (direction == +1) {
		CAM.F_B(scale *-0.2f);      //near
		MathHelper::OriC_TranslateZ(Ori_C, V, scale *-0.2f);
	}
	glutIdleFunc(myIdlex);
}


void Mouse(int button, int state, int x, int y)
{
	if (state == GLUT_LEFT_BUTTON) {
		old_x = x; old_y = y;
	}
}


void changeViewPoint(int x, int y)
{
	float dx = x - old_x;
	float dy = y - old_y;
	CAM.Y_axis(-dx / mouse_sensitivity);
	CAM.X_axis(-dy / mouse_sensitivity);
	MathHelper::makeRotateCX(V, Ori_C, -dy / mouse_sensitivity);
	MathHelper::makeRotateCY(V, Ori_C, -dx / mouse_sensitivity);
	old_x = x;
	old_y = y; 
}


int main(int argc, char** argv)
{
	
	
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(window_w , window_h);

	GLint window_1 = glutCreateWindow(argv[0]);
	glutSetWindowTitle("Fixed Pipeline");
	glutPositionWindow(1000, 250);
	glutReshapeFunc(MyReshape);
	glutDisplayFunc(display_fixedpipe);
	glutIdleFunc(myIdlex);
	glutMouseWheelFunc(MouseWheel);
	glutMouseFunc(Mouse);
	glutMotionFunc(changeViewPoint);
	glutKeyboardFunc(keyboard);

	GLint window_2 = glutCreateWindow(argv[0]);
	glutPositionWindow(300, 250);
	glutSetWindowTitle("Shader");
	glewInit();
	initMatrices();

	setShaders();
	initBufferObject(); 
	glutReshapeFunc(MyReshape);
	glutDisplayFunc(display_shader);
	glutIdleFunc(myIdlex);
	glutMouseWheelFunc(MouseWheel);
	glutMouseFunc(Mouse);
	glutMotionFunc(changeViewPoint);
	glutKeyboardFunc(keyboard);
	glutMainLoop();
	return 0;

}
