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
// + about turning:
//   https://en.wikipedia.org/wiki/Ackermann_steering_geometry
//
// How to compile in ubuntu:
// -------------------------
//   sudo apt-get install freeglut3-dev

#include <iostream>
#include <cmath>

#include <GL/glut.h>               // GLUT
#include <GL/glu.h>                // GLU
#include <GL/gl.h>                 // OpenGL

#include "sign.h"
#include "Drawable.h"
#include "Positioned.h"
#include "CarPhysics.h"

// From:
// http://slabode.exofire.net/circle_draw.shtml
void DrawCircle(float cx, float cy, float r, int num_segments) 
{ 
   float theta = 2 * 3.1415926 / float(num_segments); 
   float c = cosf(theta);//precalculate the sine and cosine
   float s = sinf(theta);
   float t;

   float x = r;//we start at angle = 0 
   float y = 0; 
       
   glBegin(GL_LINE_LOOP); 
   for(int ii = 0; ii < num_segments; ii++) 
   { 
      glVertex2f(x + cx, y + cy);//output vertex 
      //apply the rotation matrix
      t = x;
      x = c * x - s * y;
      y = s * t + c * y;
   } 
   glEnd(); 
}
   

//-----------------------------------------------------------------------
// Global data
//-----------------------------------------------------------------------

GLint TIMER_DELAY = 30;                 // timer delay (10 seconds)
GLfloat RED_RGB[] = {1.0, 0.0, 0.0};       // drawing colors
GLfloat BLUE_RGB[] = {0.0, 0.0, 1.0};
GLfloat GREEN_RGB[] = {0.0, 0.5, 0.0};
GLfloat BLACK_RGB[] = {0.0, 0.0, 0.0};
GLfloat YELLOW_RGB[] = {1.0, 1.0, 0.0};
GLfloat MAGENTA_RGB[] = {1.0, 0.0, 1.0};
GLfloat GRAY_RGB[] = {0.25, 0.25, 0.25};
GLfloat WHITE_RGB[] = {1., 1., 1.};

//-----------------------------------------------------------------------
// Classes of world objects
//-----------------------------------------------------------------------

class AsphaltRectangle : public Drawable, public Positioned {
public:
   AsphaltRectangle( double x = 0., double y = 0., double width = 300., double height = 1000. )
     : Positioned( x, y ), width_( width ), height_( height ), horizontal_( height > width )  {}

   virtual void drawGL() const override {
      glColor3fv( GRAY_RGB );
      glPushMatrix();
      glTranslatef(x_, y_, 0.0f);
      glRectf(0., 0., width_, height_);
      const double pace = 100.0;
      const double swidth = 5.0;
      glColor3fv( WHITE_RGB );
      if ( horizontal_ ) {
         for ( double starty = width_; starty + pace <= height_ - width_; starty += pace  ) {
            glRectf(width_ /2. - swidth, starty, width_ /2. + swidth, starty + pace /2.);
         }
      } else {
         for ( double startx = height_; startx + pace <= width_ - height_; startx += pace  ) {
            glRectf(startx, height_ /2. - swidth, startx + pace /2., height_ /2. + swidth );
         }
      }
      glPopMatrix();
   }
   virtual void move( int passed_time_in_ms ) const override {}
   virtual bool hasAttribute( const std::string& attribute, double x, double y ) const override {
      if ( std::string("asphalt") == attribute && x_ <= x && y_ <= y && x <= x_ + width_ && y <= y_ + height_ ) {
         return true;
      }
      return false;
   }
protected:
   float width_;
   float height_;
   bool horizontal_;
};

class Car: public CarPhysics, public Drawable {
public:
   Car( double x, double y, const PositionedContainer& world )
    : CarPhysics( x, y, world ) {}

   virtual void drawGL() const override {
      glPushMatrix();
      glTranslatef(this->getX(), this->getY(), 0.0f);
      glRotatef( this->getAngleOfCarOrientation(), 0.0, 0.0, 1.0);
      glTranslatef(0, this->getParams().getDistanceBetweenCenterAndTurningAxle(), 0.0f);

      // debug info
      glColor3fv(YELLOW_RGB);
      if ( fabs( this->getWheelOrientation() ) > 1. ) {
         DrawCircle( -sign( this->getWheelOrientation() ) * this->getTurningBaselineDistance(), -this->getParams().getCarHeight() / 2. + this->getParams().getCarHeightLower(), this->getTurningRadius(), 200 );
      }

      // car visualization
      glColor3fv(BLUE_RGB);
      const float w2 = this->getParams().getCarWidth() / 2.;
      const float h2 = this->getParams().getCarHeight() / 2.;
      glRectf(- w2, - h2, + w2, + h2);
      glColor3fv(RED_RGB);
      glRectf(- w2, + h2 - h2 / 4., + w2 , + h2 );
      glColor3fv(BLACK_RGB);
      for ( int sx = -1; sx <= 1; sx += 2 ) {
         for ( int sy = -1; sy <= 1; sy += 2 ) {
            const std::pair<double, double> rp = this->wheelRelativePosition( sx, sy );
            const double ra = this->wheelAngle( sx, sy );

            glPushMatrix();
            glTranslatef( rp.first, rp.second, 0.0f);
            glRotatef( ra, 0.0, 0.0, 1.0 );
            glRectf( - w2 /4., - h2 / 4. , + w2 / 4.,  + h2 / 4. );
            glPopMatrix();
         }
      }
      // debug info
      glColor3fv(YELLOW_RGB);
      if ( fabs( this->getWheelOrientation() ) > 1. ) {
         glBegin(GL_LINES);
         glVertex2f( 0, -h2 + this->getParams().getCarHeightLower() );
         glVertex2f( -sign( this->getWheelOrientation() ) * this->getTurningBaselineDistance(), -h2 + this->getParams().getCarHeightLower() );
         glEnd();
      }
      glColor3fv(MAGENTA_RGB);
      if ( fabs( this->getSpeed() ) > 1. ) {
         glBegin(GL_LINES);
         glVertex2f( 0, -h2 + this->getParams().getCarHeightLower() );
         glVertex2f( 0, -h2 + this->getParams().getCarHeightLower() + this->getSpeed() );
         glEnd();
      }

      glPopMatrix();
      for ( int sx = -1; sx <= 1; sx += 1 ) {
         for ( int sy = -1; sy <= 1; sy += 1 ) {
            std::pair<double, double> pmi = wheelPosition( sx, sy );
            glRectf( pmi.first - 1., pmi.second - 1. , pmi.first + 1.,  pmi.second + 1. );
         }
      }
   }
};

//-----------------------------------------------------------------------
//  Global variables
//-----------------------------------------------------------------------
static int Old_t = 0;

static PositionedContainer World;
static Car myCar( 200., 200., World );
static DrawableContainer View;
static double GlobalCenterX = 0.;
static double GlobalCenterY = 0.;

static double ScreenWidth = 0.;
static double ScreenHeight = 0.;


//-----------------------------------------------------------------------
//  Callbacks
//-----------------------------------------------------------------------

void myReshape(int screenWidth, int screenHeight) {
   ScreenWidth  = screenWidth;
   ScreenHeight = screenHeight;

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
   glPushMatrix();
   glTranslatef( -GlobalCenterX + ScreenWidth / 2, -GlobalCenterY + ScreenHeight / 2, 0.0f);
   View.drawGL();
   glPopMatrix();
}

void setVariableToLimitIfOutOfRadius( double& variable, const double target, const double radius ) {
  if ( variable < target - radius ) {
     variable = target - radius;
  }
  if ( variable > target +radius ) {
     variable = target + radius;
  }
}

void myDisplay(void) {                    // display callback
   const int t = glutGet( GLUT_ELAPSED_TIME );
   const int passed_time_in_ms = t - Old_t;
   Old_t = t;

   glClearColor(0.0, 0.5, 0.0, 1.0);         // background is green
   glClear(GL_COLOR_BUFFER_BIT);            // clear the window

   setVariableToLimitIfOutOfRadius( GlobalCenterX, myCar.getX(), ScreenWidth / 8. );
   setVariableToLimitIfOutOfRadius( GlobalCenterY, myCar.getY(), ScreenHeight / 8. );

   animate(passed_time_in_ms, RED_RGB, BLUE_RGB);

   glutSwapBuffers();                    // swap buffers
}

void myTimer(int id) {                        // timer callback
   glutPostRedisplay()     ;                  // request redraw
   glutTimerFunc(TIMER_DELAY, myTimer, 0);
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
      case GLUT_KEY_LEFT:
         myCar.turnLeft();
         break;
      case GLUT_KEY_RIGHT:
         myCar.turnRight();
         break;
      case GLUT_KEY_UP:
         myCar.accelerate();
         break;
      case GLUT_KEY_DOWN:
         break;
      default:
         break;
   }
}

void myKeyboardSpecialKeysUp(int key, int x, int y) {
   switch (key) {
      case GLUT_KEY_LEFT:
         myCar.stopTurning();
         break;
      case GLUT_KEY_RIGHT:
         myCar.stopTurning();
         break;
      case GLUT_KEY_UP:
         myCar.stopAccelerating();
         break;
      case GLUT_KEY_DOWN:
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
   double roadWidth = 300.;
   double roadLength = 3000.;
   double roadStartX = 100.;
   double roadStartY = 100.;
   AsphaltRectangle b1( roadStartX, roadStartY, roadWidth,  roadLength );
   AsphaltRectangle b2( roadStartX, roadStartY, roadLength, roadWidth );
   AsphaltRectangle b3( roadStartX + roadLength - roadWidth, roadStartY, roadWidth, roadLength );
   AsphaltRectangle b4( roadStartX, roadStartY + roadLength - roadWidth, roadLength, roadWidth );
   World.addChild( b1 );
   World.addChild( b2 );
   World.addChild( b3 );
   World.addChild( b4 );
   World.addChild( myCar );

   View.addChild( b1 );
   View.addChild( b2 );
   View.addChild( b3 );
   View.addChild( b4 );
   View.addChild( myCar );

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
   glutSpecialUpFunc(myKeyboardSpecialKeysUp);
   glutTimerFunc(TIMER_DELAY, myTimer, 0);
   glutMainLoop();
   return 0;
}
