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
GLfloat GRAY_RGB[] = {0.25, 0.25, 0.25};
GLfloat WHITE_RGB[] = {1., 1., 1.};

constexpr double PI = 3.141592653589793;

constexpr double NUMERICAL_ERROR = 1e-10;
constexpr double CAR_WIDTH = 50.;
constexpr double CAR_HEIGHT = 100.;
constexpr double MAXIMAL_STEERING_ANGLE = 40.;
constexpr double STEERING_SPEED = 30.;
constexpr double DELTA_T = 0.001;
constexpr double MAXIMAL_SPEED = 200.;
constexpr double ACCELERATION = 40.;
constexpr double DECELERATION_MINUS_ACCELERATION = 20.;
constexpr double RELATIVE_DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE =0.5; // to me 0.5 is natural
constexpr double TURNING_CONST_ANGLE = 0.; // Death rally should use it instead of speed / radius ( maybe calculating radius is too expensive ) - use 1.
constexpr double TURNING_DECELERATION = 10.;  // Not realistic physically
constexpr double MAXIMAL_TURNING_SPEED = 150.;

// Calculated constants

constexpr double CAR_HEIGHT_UPPER = ( 0.5 + RELATIVE_DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE ) * CAR_HEIGHT;
constexpr double CAR_HEIGHT_LOWER = ( 0.5 - RELATIVE_DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE ) * CAR_HEIGHT;
constexpr double DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE = RELATIVE_DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE * CAR_HEIGHT;
constexpr double DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE_2 = DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE * DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE;

constexpr double calculatingMagicNumberB( const double alpha ) {
   return ( CAR_HEIGHT_UPPER + CAR_HEIGHT_LOWER ) / ( std::tan( fabs(alpha) / 180. * PI ) + 1e-20 );
}

constexpr double turningBaseline( const double alpha ) {
   return ( CAR_WIDTH + calculatingMagicNumberB( alpha ) + sqrt( calculatingMagicNumberB( alpha ) * calculatingMagicNumberB( alpha ) + 4 * ( CAR_HEIGHT_UPPER * CAR_HEIGHT_LOWER ) ) ) / 2.; 
}

constexpr double turningRadius( const double alpha ) {
   return std::sqrt( DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE_2 + turningBaseline( alpha ) * turningBaseline( alpha ) );
}

const double MINIMAL_TURNING_RADIUS = turningRadius( MAXIMAL_STEERING_ANGLE );

double sign( const double number ) {
   if ( number > NUMERICAL_ERROR ) {
      return 1.;
   }
   if ( number < -NUMERICAL_ERROR ) {
      return -1.;
   }
   return 0.;
}

//-----------------------------------------------------------------------
// Classes of world objects
//-----------------------------------------------------------------------

class Drawable {
public:
   Drawable( double x = 0., double y = 0. ) : x_( x ), y_( y ) {}
   virtual ~Drawable() {}
   virtual void drawGL() const = 0;
   virtual void move( int passed_time_in_ms ) const = 0;
   virtual bool hasAttribute( const std::string& attribute, double x, double y ) const = 0;
   void setX( double x ) { x_ = x; }
   void setY( double y ) { y_ = y; }
   double getX() const { return x_;  }
   double getY() const { return y_;  }

protected:
   mutable double x_;
   mutable double y_;
};

class AsphaltRectangle : public Drawable {
public:
   AsphaltRectangle( float x = 0., float y = 0., float width = 100., float height = 100. )
     : Drawable( x, y ), width_( width ), height_( height ), horizontal_( height > width )  {}

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
   virtual bool hasAttribute( const std::string& attribute, double x, double y ) const override {
      for ( const auto& elem: static_cast< std::vector< const Drawable* > >( *this ) ) {
         if ( elem->hasAttribute( attribute, x, y ) ) {
            return true;
         }
      }
      return false;
   }

};

class Car : public Drawable {
public:
   Car( float x, float y, const DrawableContainer& world )
    : Drawable( x, y ), speed_( 0. ), drifting_( 0. ), angleOfCarOrientation_( 0. ), wheelOrientation_( 0. ),
      actionTurning_( 0 ), actionAccelerating_( 0 ), turningBaselineDistance_( 0. ), turningRadius_( 0. ) ,
      world_( world ) {}

   virtual void drawGL() const override {
      glPushMatrix();
      glTranslatef(x_, y_, 0.0f);
      glRotatef(angleOfCarOrientation_, 0.0, 0.0, 1.0);
      glTranslatef(0, DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE, 0.0f);
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
            const double mirroring = sign( wheelOrientation_ ) < 0 ? -1.0 : 1.0 ;
            glTranslatef( mirroring * sx * w2, sy * h2, 0.0f);
            const double myTan = ( sy == 1 ? CAR_HEIGHT_UPPER : CAR_HEIGHT_LOWER ) / ( sx * w2 + turningBaselineDistance_ );
            const double wheelAngle = sign( wheelOrientation_ ) * std::atan( myTan ) / PI * 180.;
            glRotatef( 1. *sy * wheelAngle, 0.0, 0.0, 1.0);
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

   virtual bool hasAttribute( const std::string& attribute, double x, double y ) const override { return false; }

   int numberOfWheelsOnAsphalt() const {
      int retval = 0;
      for ( int sx = -1; sx <= 1; sx += 2 ) {
         for ( int sy = -1; sy <= 1; sy += 2 ) {
            std::pair<double, double> pmi = wheelPosition( sx, sy );
            if ( world_.hasAttribute( "asphalt", pmi.first, pmi.second ) ) {
               retval += 1;
            }
         }
      }
      return retval;
   }

   std::pair<double, double> wheelPosition( int sx, int sy ) const {
      const double angleOfCarOrientationInRad = angleOfCarOrientation_ / 180. * PI ;
      const double ox = -DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE * std::sin( angleOfCarOrientationInRad );
      const double oy =  DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE * std::cos( angleOfCarOrientationInRad );

      const double upx = -CAR_HEIGHT / 2. * std::sin( angleOfCarOrientationInRad );
      const double upy =  CAR_HEIGHT / 2. * std::cos( angleOfCarOrientationInRad );

      const double rightx = CAR_WIDTH / 2. * std::cos( angleOfCarOrientationInRad );
      const double righty = CAR_WIDTH / 2. * std::sin( angleOfCarOrientationInRad );

      return std::pair<double, double>(x_ + ox + sx * upx + sy * rightx, y_ + oy + sx * upy + sy * righty);
   }

   void correctingWheelOrientation() const {
      // turning
      if ( !actionTurning_ ) {
         if ( fabs( wheelOrientation_ ) < DELTA_T * STEERING_SPEED ) {
            wheelOrientation_ = 0.;
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

   void calculateTurningRadiusAndBaseline() const {
      if ( fabs( TURNING_CONST_ANGLE ) ) {
         turningRadius_           = speed_ * DELTA_T / 2. / std::sin( TURNING_CONST_ANGLE * DELTA_T / 2. );
         turningBaselineDistance_ = turningRadius_;
      } else {
         turningBaselineDistance_ = turningBaseline( wheelOrientation_ );
         turningRadius_           = std::sqrt( DISTANCE_BETWEEN_CENTER_AND_TURNING_AXLE_2 + turningBaselineDistance_ * turningBaselineDistance_ );
      }
   }

   // Moving in one ms
   void move_in_a_millisecond() const {
      calculateTurningRadiusAndBaseline();

      if ( fabs( TURNING_CONST_ANGLE ) )
      {
         angleOfCarOrientation_ += sign( wheelOrientation_ ) * TURNING_CONST_ANGLE * 180. / PI * DELTA_T;
      } else {
         angleOfCarOrientation_ += sign( wheelOrientation_ ) * ( speed_ / turningRadius_ ) * 180. / PI * DELTA_T;
      }

      const double angleOfCarOrientationInRad = angleOfCarOrientation_ / 180. * PI ;

      x_ -= speed_ * std::sin( angleOfCarOrientationInRad ) * DELTA_T;
      y_ += speed_ * std::cos( angleOfCarOrientationInRad ) * DELTA_T;

      correctingWheelOrientation();

      // maximal speed correction
      const double currentMaximalSpeed = MAXIMAL_SPEED * ( numberOfWheelsOnAsphalt() > 2 ? 1.0 : 0.35 );
      
      double acceleration = ( static_cast<double>( actionAccelerating_ ) * ACCELERATION
                            - ( 1. - static_cast<double>( actionAccelerating_ ) ) * ( ACCELERATION + DECELERATION_MINUS_ACCELERATION ) ) * DELTA_T ;
      if ( speed_ > MAXIMAL_TURNING_SPEED && sign( wheelOrientation_ ) != 0. && acceleration > -TURNING_DECELERATION * DELTA_T ) {
         acceleration = -TURNING_DECELERATION  * DELTA_T;
      }
      if ( speed_ > currentMaximalSpeed && acceleration > -( ACCELERATION + DECELERATION_MINUS_ACCELERATION ) * DELTA_T  ) {
        acceleration = -( ACCELERATION + DECELERATION_MINUS_ACCELERATION ) * DELTA_T;
      }

      speed_ += acceleration;

      if ( speed_ < 0. ) {
         speed_ = 0.;
      }
   }

   virtual void move( int passed_time_in_ms ) const override {
      for ( int i = 0; i < passed_time_in_ms; ++i ) {
         move_in_a_millisecond();
      }
   }
private:
   mutable double speed_;
   mutable double drifting_;
   mutable double angleOfCarOrientation_; 
   mutable double wheelOrientation_;

   mutable int actionTurning_;
   mutable int actionAccelerating_;

   mutable double turningBaselineDistance_;
   mutable double turningRadius_;

   const DrawableContainer& world_;
};

//-----------------------------------------------------------------------
//  Global variables
//-----------------------------------------------------------------------
static int Old_t = 0;

static DrawableContainer World;
static Car myCar( 100., 100., World );
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
   World.drawGL();
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
   AsphaltRectangle b1( 100.f, 0.f, 300.f, 2000.f );
   AsphaltRectangle b2( 100.f, 0.f, 2000.f, 300.f );
   AsphaltRectangle b3( 1800.f, 0.f, 300.f, 2000.f );
   AsphaltRectangle b4( 100.f, 1700.f, 2000.f, 300.f );
   World.addChild( b1 );
   World.addChild( b2 );
   World.addChild( b3 );
   World.addChild( b4 );
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
