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
#include <vector>

#include <GL/glut.h>               // GLUT
#include <GL/glu.h>                // GLU
#include <GL/gl.h>                 // OpenGL

//-----------------------------------------------------------------------
// Global data
//-----------------------------------------------------------------------

GLint TIMER_DELAY = 30;                 // timer delay (10 seconds)
GLfloat RED_RGB[] = {1.0, 0.0, 0.0};       // drawing colors
GLfloat BLUE_RGB[] = {0.0, 0.0, 1.0};
GLfloat GREEN_RGB[] = {0.0, 0.5, 0.0};
GLfloat BLACK_RGB[] = {0.0, 0.0, 0.0};
GLfloat YELLOW_RGB[] = {1.0, 1.0, 0.0};

const double PI = 3.141592653589793;

const double NUMERICAL_ERROR = 1e-10;
const double CAR_WIDTH = 50.;
const double CAR_HEIGHT = 100.;
const double MAXIMAL_STEERING_ANGLE = 40.;
const double STEERING_SPEED = 30.;
const double DELTA_T = 0.001;
const double MAXIMAL_SPEED = 80.;
const double ACCELERATION = 20.;
const double DECELERATION = 30.;
const double RELATIVE_DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE = 0.5; // to me 0.5 is natural
const double TURNING_CONST_ANGLE = 0.; // Death rally should use it instead of speed / radius ( maybe calculating radius is too expensive )

// Calculated constants

const double CAR_HEIGHT_UPPER = ( 0.5 + RELATIVE_DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE ) * CAR_HEIGHT;
const double CAR_HEIGHT_LOWER = ( 0.5 - RELATIVE_DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE ) * CAR_HEIGHT;
const double DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE = RELATIVE_DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE * CAR_HEIGHT;
const double DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE_2 = DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE * DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE;

double sign( const double number ) {
   if ( number > NUMERICAL_ERROR ) {
      return 1.;
   }
   if ( number < -NUMERICAL_ERROR ) {
      return -1.;
   }
   return 0.;
}

double turningBaseline( const double alpha ) {
   const double higherRad = fabs(alpha) / 180. * PI;
   if ( higherRad < NUMERICAL_ERROR ) {
      return 10.e40;
   }
   // if heightLower == 0, then 
   // const double d = heightUpper / std::tan( higherRad );

   // b and c, because it is from a quadratic equation
   const double b = - ( CAR_HEIGHT_UPPER + CAR_HEIGHT_LOWER ) / std::tan( higherRad );
   const double c = - ( CAR_HEIGHT_UPPER * CAR_HEIGHT_LOWER );
  
   const double d = ( -b + sqrt( b * b - 4 * c ) ) / 2.; 
   return d + CAR_WIDTH / 2.;
}

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
   Car( float x, float y ) : Drawable( x, y ), speed_( 0. ), angleOfCarOrientation_( 0. ), wheelOrientation_( 0. ), actionTurning_( 0 ), actionAccelerating_( 0 ), turningBaselineDistance_( 0. ) {}

   virtual void drawGL() const override {
      glPushMatrix();
      glTranslatef(x_, y_, 0.0f);
      glRotatef(angleOfCarOrientation_, 0.0, 0.0, 1.0);
      glColor3fv(BLUE_RGB);
      const float w2 = CAR_WIDTH / 2.;
      const float h2 = CAR_HEIGHT / 2.;
      glRectf(- w2, - h2, + w2, + h2);
      glColor3fv(RED_RGB);
      glRectf(- w2, + h2 - h2 / 4., + w2 , + h2 );
      glColor3fv(BLACK_RGB);
      for ( int sx = -1; sx <= 1; sx += 2 ) {
         for ( int sy = -1; sy <= 1; sy += 2 ) {
            glPushMatrix();
            glTranslatef( sx * w2, sy * h2, 0.0f);
            const double wheelAngle = std::atan( ( sy == 1 ? CAR_HEIGHT_UPPER : CAR_HEIGHT_LOWER ) / ( sx * w2 + turningBaselineDistance_ ) ) / PI * 180.;
            glRotatef( sy * sign( wheelOrientation_ ) * wheelAngle, 0.0, 0.0, 1.0);
            glRectf( - w2 /4., - h2 / 4. , + w2 / 4.,  + h2 / 4. );
            glPopMatrix();
         }
      }
      // debug info
      glColor3fv(YELLOW_RGB);
      if ( fabs( wheelOrientation_ ) > 1. ) {
         glBegin(GL_LINES);
         glVertex2f( 0, -h2 + CAR_HEIGHT_LOWER );
         glVertex2f( -sign( wheelOrientation_ ) * turningBaselineDistance_, -h2 + CAR_HEIGHT_LOWER );
         glEnd();
      }

      glColor3fv(RED_RGB);
      glPopMatrix();
   }

   void stopTurning() const { actionTurning_ =  0; }
   void turnLeft()    const { actionTurning_ = +1; }
   void turnRight()   const { actionTurning_ = -1; }
   void stopAccelerating() const  { actionAccelerating_ = +0; }
   void accelerate() const  { actionAccelerating_ = +1; }

   void correctingWheelOrientation() const {
      // turning
      if ( !actionTurning_ ) {
         if ( fabs( wheelOrientation_ ) < NUMERICAL_ERROR ) {
            return;
         }
         wheelOrientation_ -= sign( wheelOrientation_ ) * DELTA_T * STEERING_SPEED;
      } else {
         wheelOrientation_ += static_cast<double>( actionTurning_ ) * DELTA_T * STEERING_SPEED;
      }
      if ( fabs( wheelOrientation_ ) > MAXIMAL_STEERING_ANGLE ) {
         wheelOrientation_ = MAXIMAL_STEERING_ANGLE * sign( wheelOrientation_ );
      }
   }

   // Moving in one ms
   void move_in_a_millisecond() const {
      turningBaselineDistance_ = turningBaseline( wheelOrientation_ );
      const double radius = std::sqrt( DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE_2 + turningBaselineDistance_ * turningBaselineDistance_ );
      const double turningDeviationAngleInRad = sign( wheelOrientation_ ) * std::asin( DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE / radius );
      const double deltaAngleOfCarOrientation = sign( wheelOrientation_ ) * ( ( fabs( TURNING_CONST_ANGLE ) > NUMERICAL_ERROR ? TURNING_CONST_ANGLE : speed_ ) / radius ) * 180. / PI * DELTA_T;
      angleOfCarOrientation_ += deltaAngleOfCarOrientation;

      x_ -= speed_ * std::sin( angleOfCarOrientation_ / 180. * PI + turningDeviationAngleInRad ) * DELTA_T;
      y_ += speed_ * std::cos( angleOfCarOrientation_ / 180. * PI + turningDeviationAngleInRad ) * DELTA_T;

      correctingWheelOrientation();

      const double acceleration = ( static_cast<double>( actionAccelerating_ ) * ACCELERATION - ( 1. - static_cast<double>( actionAccelerating_ ) ) * DECELERATION ) * DELTA_T ;

      speed_ += acceleration;
      if ( speed_ < 0. ) {
         speed_ = 0.;
      }
      if ( speed_ > MAXIMAL_SPEED ) {
         speed_ = MAXIMAL_SPEED;
      }
   }

   virtual void move( int passed_time_in_ms ) const override {
      for ( int i = 0; i < passed_time_in_ms; ++i ) {
         move_in_a_millisecond();
      }
   }
private:
   mutable double speed_;
   mutable double angleOfCarOrientation_; 
   mutable double wheelOrientation_;

   mutable int actionTurning_;
   mutable int actionAccelerating_;

   mutable double turningBaselineDistance_;
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
   glutSpecialUpFunc(myKeyboardSpecialKeysUp);
   glutTimerFunc(TIMER_DELAY, myTimer, 0);
   glutMainLoop();
   return 0;
}
