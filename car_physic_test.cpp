// car_physic_test
// ---------------
// Used the following opengl template as a starting pooint:
//   https://www.cs.umd.edu/class/spring2013/cmsc425/OpenGL/OpenGLSamples/OpenGL-2D-Sample/opengl-2D-sample.cpp
//   Author: Dave Mount
//
// + extended with
//   http://stackoverflow.com/questions/2182675/how-do-you-make-sure-the-speed-of-opengl-animation-is-consistent-on-different-ma 
//   http://gamedev.stackexchange.com/questions/103334/how-can-i-set-up-a-pixel-perfect-camera-using-opengl-and-orthographic-projection
//
// How to compile in ubuntu:
// -------------------------
//   sudo apt-get install freeglut3-dev

#include <iostream>
#include <cmath>
#include <vector>

#include <GL/glut.h>               // GLUT
#include <GL/glu.h>                // GLU
#include <GL/gl.h>                 // OpenGL

//-----------------------------------------------------------------------
// Global data
//-----------------------------------------------------------------------

GLint TIMER_DELAY = 10000;                 // timer delay (10 seconds)
GLfloat RED_RGB[] = {1.0, 0.0, 0.0};       // drawing colors
GLfloat BLUE_RGB[] = {0.0, 0.0, 1.0};

//-----------------------------------------------------------------------
// Classes of world objects
//-----------------------------------------------------------------------

class Drawable {
public:
   Drawable( float x = 0., float y = 0. ) : x_( x ), y_( y ) {}
   virtual ~Drawable() {}
   virtual void drawGL() const = 0;
   void setX( float x ) { x_ = x; }
   void setY( float y ) { y_ = y; }
protected:
   float x_;
   float y_;
};

class DrawableContainer : public Drawable, public std::vector< const Drawable* > {
public:
   virtual ~DrawableContainer() {}
   void addChild( const Drawable& child ) { push_back( &child ); }
   virtual void drawGL() const override {
      for ( const auto& elem: static_cast< std::vector< const Drawable* > >( *this ) ) {
         elem->drawGL();
      }
   }
};

class Car : public Drawable {
public:
   Car( float x, float y ) : Drawable( x, y ) {}

   virtual void drawGL() const override {
      glColor3fv(BLUE_RGB);
      glRectf(10, 10, 60, 110);
   }
};

//-----------------------------------------------------------------------
//  Global variables
//-----------------------------------------------------------------------
static int old_t = 0;

static Car myCar( 10., 10. );
static DrawableContainer World;

//-----------------------------------------------------------------------
//  Callbacks
//-----------------------------------------------------------------------

void myReshape(int screenWidth, int screenHeight) {
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glViewport(0, 0, screenWidth, screenHeight);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glOrtho(0, screenWidth, 0, screenHeight, 1, -1);
   glutPostRedisplay();
}

void animate(int passed_time, GLfloat* diamColor, GLfloat* rectColor) {
   World.drawGL();
}

void myDisplay(void) {                    // display callback
   const int t = glutGet( GLUT_ELAPSED_TIME );
   const int passed_time = t - old_t;
   old_t = t;

   glClearColor(0.0, 0.0, 0.0, 1.0);         // background is black
   glClear(GL_COLOR_BUFFER_BIT);            // clear the window

   animate(passed_time, RED_RGB, BLUE_RGB);

   glutSwapBuffers();                    // swap buffers
}

void myTimer(int id) {                    // timer callback
   glutPostRedisplay();                  // request redraw
   glutTimerFunc(TIMER_DELAY, myTimer, 0);    // reset timer for 10 seconds
}

void myMouse(int b, int s, int x, int y) {     // mouse click callback
   if (s == GLUT_DOWN) {
      if (b == GLUT_LEFT_BUTTON) {
         glutPostRedisplay();
      }
   }
}

void myKeyboard(unsigned char key, int x, int y) {
   switch (key) {
      case 'q':                        // 'q' means quit
         exit(0);
         break;
      default:
         break;
   }
}

void myKeyboardSpecialKeys(int key, int x, int y) {
   switch (key) {
      case GLUT_KEY_UP:
      case GLUT_KEY_DOWN:
         exit(0);
         break;
      default:
         break;
   }
}

//-----------------------------------------------------------------------
//  Main program
//-----------------------------------------------------------------------

int main(int argc, char** argv)
{
   // building the world
   World.addChild( myCar );

   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
   glutInitWindowSize(400, 400);
   glutInitWindowPosition(0, 0);
   glutCreateWindow(argv[0]);

   glutDisplayFunc(myDisplay);
   glutReshapeFunc(myReshape);
   glutMouseFunc(myMouse);
   glutKeyboardFunc(myKeyboard);
   glutSpecialFunc(myKeyboardSpecialKeys);
   glutTimerFunc(TIMER_DELAY, myTimer, 0);
   glutMainLoop();
   return 0;
}
