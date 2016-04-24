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
GLfloat GREEN_RGB[] = {0.0, 1.0, 0.0};

const float CAR_WIDTH = 50.;
const float CAR_HEIGHT = 100.;

//-----------------------------------------------------------------------
// Classes of world objects
//-----------------------------------------------------------------------

class Drawable {
public:
   Drawable( float x = 0., float y = 0. ) : x_( x ), y_( y ) {}
   virtual ~Drawable() {}
   virtual void drawGL() const = 0;
   virtual void move( int passed_time_in_ms ) const = 0;
   void setX( float x ) { x_ = x; }
   void setY( float y ) { y_ = y; }
protected:
   mutable double x_;
   mutable double y_;
};

class BackgroundRectangle : public Drawable {
public:
   BackgroundRectangle( GLfloat* color, float x = 0., float y = 0., float width = 100., float height = 100. ) : Drawable( x, y ), color_( color ), width_( width ), height_( height ) {}
   virtual void drawGL() const override {
      glColor3fv( color_ );
      glRectf(x_, y_, x_ + width_, y_ + height_);
   }
   virtual void move( int passed_time_in_ms ) const override {}
protected:
   const GLfloat* const color_;
   float width_;
   float height_;
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
   virtual void move( int passed_time_in_ms ) const override {
       for ( const auto& elem: static_cast< std::vector< const Drawable* > >( *this ) ) {
         elem->move( passed_time_in_ms );
      }
   }
};

class Car : public Drawable {
public:
   Car( float x, float y ) : Drawable( x, y ), speedX_( 0.f ), speedY_( 0.f )  {}

   virtual void drawGL() const override {
      glPushMatrix();
      glTranslatef(x_, y_, 0.0f);
      glRotatef(orientation_, 0.0, 0.0, 1.0);
      glColor3fv(BLUE_RGB);
      const float w2 = CAR_WIDTH / 2.;
      const float h2 = CAR_HEIGHT / 2.;
      glRectf(- w2, - h2, + w2, + h2);
      glColor3fv(RED_RGB);
      glRectf(- w2, + h2 - h2 / 4., + w2 , + h2 );
      glPopMatrix();
   }

   // Moving in one ms
   void move_in_a_millisecond() const {
      x_ += speedX_ * 0.001;
      y_ += speedY_ * 0.001;
   }

   virtual void move( int passed_time_in_ms ) const override {
      for ( int i = 0; i < passed_time_in_ms; ++i ) {
         move_in_a_millisecond();
      }
   }
private:
   double speedX_;
   double speedY_;
   double orientation_; 
};

//-----------------------------------------------------------------------
//  Global variables
//-----------------------------------------------------------------------
static int Old_t = 0;

static Car myCar( 100., 100. );
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

void animate(int passed_time_in_ms, GLfloat* diamColor, GLfloat* rectColor) {
   World.move( passed_time_in_ms );
   World.drawGL();
}

void myDisplay(void) {                    // display callback
   const int t = glutGet( GLUT_ELAPSED_TIME );
   const int passed_time_in_ms = t - Old_t;
   Old_t = t;

   glClearColor(0.0, 0.0, 0.0, 1.0);         // background is black
   glClear(GL_COLOR_BUFFER_BIT);            // clear the window

   animate(passed_time_in_ms, RED_RGB, BLUE_RGB);

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
   BackgroundRectangle b1( GREEN_RGB, 0.f, 0.f, 1000.f, 1000.f );
   World.addChild( b1 );
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
