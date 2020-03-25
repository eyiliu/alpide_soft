
// Programmer: Mihalis Tsoukalos
// Date: Wednesday 04 June 2014
//
// A simple OpenGL program that draws a triangle
// and automatically rotates it.
//
// g++ rotateCube.cc -lm -lglut -lGL -lGLU -o rotateCube

#include <iostream>
#include <stdlib.h>

// the GLUT and OpenGL libraries have to be linked correctly
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif


#include <random>
#include <cmath>

using namespace std;

// The coordinates for the vertices of the cube
double sensor_x_size = 30;
double sensor_y_size = 15;
double sensor_z_size = 1;

double lpz[6] = {0, 30, 60, 120, 150, 180};

// Rotate X
double rX=0;
// Rotate Y
double rY=270;

struct Hit{
  double x;
  double y;
  unsigned l;
};

std::vector<Hit> hits(6);

void drawTelescope(){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Reset transformations
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glTranslatef(0.0, 0.0, -200);
  glRotatef( rX, 1.0, 0.0, 0.0 );
  glRotatef( rY, 0.0, 1.0, 0.0 );
  
  // Add an ambient light
  GLfloat ambientColor[] = {0.2, 0.2, 0.2, 1.0};
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

  // Add a positioned light
  GLfloat lightColor0[] = {0.5, 0.5, 0.5, 1.0};
  GLfloat lightPos0[] = {1000, 1000, -1000, 0};
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
  glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);

  glLineWidth(10.0);
  glBegin(GL_LINES);
  glColor3f( 1.0f, 0.0f, 0.0f);
  glVertex3f(0, 0, 0 );
  glVertex3f(1, 0, 0 );
  glEnd();

  glLineWidth(10.0);
  glBegin(GL_LINES);
  glColor3f( 0.0f, 1.0f, 0.0f);
  glVertex3f(0, 0, 0 );
  glVertex3f(0, 1, 0 );
  glEnd();

  glLineWidth(10.0);
  glBegin(GL_LINES);
  glColor3f( 0.0f, 0.0f, 1.0f);
  glVertex3f(0, 0, 0 );
  glVertex3f(0, 0, 1 );
  glEnd();

  // glTranslatef(0.5, 1.0, 0.0);
  // glRotatef(angle, 1.0, 1.0, 1.0);
  // glRotatef( angle, 1.0, 0.0, 1.0 );
  // glRotatef( angle, 0.0, 1.0, 1.0 );
  // glTranslatef(-0.5, -1.0, 0.0);

  // Create the 3D cube

  for(int i=0; i< 6; i++){
    // BACK

    double x = sensor_x_size/2;
    double y = sensor_y_size/2;
    double z_min = lpz[i];
    double z_max = z_min + sensor_z_size;
    
    glBegin(GL_POLYGON);
    glColor3f(1, 1, 0);
    glVertex3f(x, -y, z_max);
    glVertex3f(x, y, z_max);
    glVertex3f(-x, y, z_max);
    glVertex3f(-x, -y, z_max);
    glEnd();

    // FRONT
    glBegin(GL_POLYGON);
    glColor3f(1, 1, 0);
    glVertex3f(-x, y, z_min);
    glVertex3f(-x, -y, z_min);
    glVertex3f(x, -y, z_min);
    glVertex3f(x, y, z_min);
    glEnd();

    // LEFT
    glBegin(GL_POLYGON);
    glColor3f(1, 1, 0);
    glVertex3f(-x, -y, z_min);
    glVertex3f(-x, -y, z_max);
    glVertex3f(-x, y, z_max);
    glVertex3f(-x, y, z_min);
    glEnd();

    // RIGHT
    glBegin(GL_POLYGON); 
    glColor3f(1, 1, 0);
    glVertex3f(x, -y, z_min);
    glVertex3f(x, -y, z_max);
    glVertex3f(x, y, z_max);
    glVertex3f(x, y, z_min);
    glEnd();

    // TOP
    glBegin(GL_POLYGON);
    glColor3f(1, 1, 0);
    glVertex3f(x, y, z_max);
    glVertex3f(-x, y, z_max);
    glVertex3f(-x, y, z_min);
    glVertex3f(x, y, z_min);
    glEnd();

    // BOTTOM
    glBegin(GL_POLYGON);
    glColor3f(1, 1, 0);
    glVertex3f(-x, -y, z_min);
    glVertex3f(-x, -y, z_max);
    glVertex3f(x, -y, z_max);
    glVertex3f(x, -y, z_min);
    glEnd();
  }

  glBegin(GL_POINTS);
  glColor3f(1,0,0);
  glPointSize(5);
  for(auto& hit: hits){
    glVertex3f(hit.x,hit.y,lpz[hit.l]);
  }
  glEnd();
  glFlush();
  glutSwapBuffers();
}

// Function for increasing the angle variable smoothly, 
// keeps it <=360
// It can also be implemented using the modulo operator.
void update(int value)
{
  //angle += 1.0f;
  //if (angle > 360)
  //{
  //  angle -= 360;
  //}
  
  glutPostRedisplay();
  glutTimerFunc(25, update, 0);
}

// Initializes 3D rendering
void initRendering()
{
  
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_COLOR_MATERIAL);

  // Set the color of the background
  glClearColor(0.7f, 0.8f, 1.0f, 1.0f);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_NORMALIZE);
}


// Called when the window is resized
void handleResize(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, (double)w / (double)h, 1.0, 200.0);
}


void SpecialKeyboard(int key, int x, int y)
{
    if (key == GLUT_KEY_RIGHT)
        {
                rY += 15;
        }
    else if (key == GLUT_KEY_LEFT)
        {
                rY -= 15;
        }
    else if (key == GLUT_KEY_DOWN)
        {
                rX -= 15;
        }
    else if (key == GLUT_KEY_UP)
        {
                rX += 15;
        }
    
    // Request display update
    std::cout<< rX <<" "<< rY <<std::endl;
    glutPostRedisplay();
}


void Keyboard(unsigned char key, int x, int y)
{

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 6);
  std::normal_distribution<> d{0, 5};
  //hits.clear();
  for(unsigned i = 0; i<6; i++){
    Hit h= {d(gen), d(gen), i};
    hits.push_back(h);
  }

  // Request display update
  // std::cout<< rX <<" "<< rY <<std::endl;  
  
  glutPostRedisplay();
}



int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(1000, 400);
  glutInitWindowPosition(100, 100);
  glutCreateWindow("OpenGL - Telescope ALPIDE");
  initRendering();
  
  glutDisplayFunc(drawTelescope);
  glutReshapeFunc(handleResize);
  glutSpecialFunc(SpecialKeyboard);
  glutKeyboardFunc(Keyboard);
  // Add a timer for the update(...) function
  glutTimerFunc(25, update, 0);

  glutMainLoop();
  return 0;
}
