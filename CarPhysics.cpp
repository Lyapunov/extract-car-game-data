#include "CarPhysics.h"

#include <cmath>
#include <utility>

static const double PI = 3.141592653589793;
static const double NUMERICAL_ERROR = 1e-10;
static const double DELTA_T = 0.001;

namespace {
   double sign( const double number ) {
      if ( number > NUMERICAL_ERROR ) {
         return 1.;
      }
      if ( number < -NUMERICAL_ERROR ) {
         return -1.;
      }
      return 0.;
   }
}

double 
CarPhysicalParameters::getTurningBaseline( const double alpha ) const {
   return ( carWidth_ + calculatingMagicNumberB( alpha ) + sqrt( calculatingMagicNumberB( alpha ) * calculatingMagicNumberB( alpha ) + 4 * carHeightMagicProduct_ ) ) / 2.; 
}

double
CarPhysicalParameters::calculatingMagicNumberB( const double alpha ) const {
   return carHeight_ / ( std::tan( fabs(alpha) / 180. * PI ) + 1e-20 );
}

int
CarPhysics::wheelsOnAsphalt() const {
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

std::pair<double, double>
CarPhysics::wheelRelativePosition( int sx, int sy ) const {
   return std::pair<double, double>( sx * params_.getCarWidth() / 2., sy * params_.getCarHeight() / 2. );
}

std::pair<double, double>
CarPhysics::wheelPosition( int sx, int sy ) const {
   const std::pair<double, double> rp = wheelRelativePosition( sx, sy ); 

   const double angleOfCarOrientationInRad = angleOfCarOrientation_ / 180. * PI ;
   const double ox = -params_.getDistanceBetweenCenterAndTurningAxle() * std::sin( angleOfCarOrientationInRad );
   const double oy =  params_.getDistanceBetweenCenterAndTurningAxle() * std::cos( angleOfCarOrientationInRad );

   const double upx = -std::sin( angleOfCarOrientationInRad );
   const double upy =  std::cos( angleOfCarOrientationInRad );
   const double rightx = std::cos( angleOfCarOrientationInRad );
   const double righty = std::sin( angleOfCarOrientationInRad );

   return std::pair<double, double>(x_ + ox + rp.first * rightx + rp.second * upx, y_ + oy + rp.first * righty + rp.second * upy);
}

double
CarPhysics::wheelAngle( int sx, int sy ) const {
   const std::pair<double, double> rp = wheelRelativePosition( sx, sy ); 

   const double wheelAngleTan = fabs( ( sy == 1 ? params_.getCarHeightUpper() : params_.getCarHeightLower() ) / ( rp.first + turningBaselineDistance_ ) );
   const double signNullifier = ( fabs( sign( wheelOrientation_ ) )  > NUMERICAL_ERROR ? 1.0 : 0.0 );
   const double signOfAngle = 1. *sy * signNullifier * sign( rp.first + sign( wheelOrientation_ ) * turningBaselineDistance_ );
   return signOfAngle * std::atan( wheelAngleTan ) / PI * 180.;
}

void
CarPhysics::correctingWheelOrientation() const {
   // turning
   if ( !actionTurning_ ) {
      if ( fabs( wheelOrientation_ ) < DELTA_T * params_.getSteeringSpeed() ) {
         wheelOrientation_ = 0.;
         return;
      }
      wheelOrientation_ -= sign( wheelOrientation_ ) * DELTA_T * params_.getSteeringSpeed();
   } else {
      wheelOrientation_ += static_cast<double>( actionTurning_ ) * DELTA_T * params_.getSteeringSpeed();
   }
   if ( fabs( wheelOrientation_ ) > params_.getMaximalSteeringAngle() ) {
      wheelOrientation_ = params_.getMaximalSteeringAngle() * sign( wheelOrientation_ );
   }
}

void
CarPhysics::calculateTurningRadiusAndBaseline() const {
   if ( fabs( params_.getTurningConstAngle() ) ) {
      turningRadius_           = speed_ * DELTA_T / 2. / std::sin( params_.getTurningConstAngle() * DELTA_T / 2. );
      turningBaselineDistance_ = turningRadius_;
   } else {
      turningBaselineDistance_ = params_.getTurningBaseline( wheelOrientation_ );
      turningRadius_           = std::sqrt( params_.getDistanceBetweenCenterAndTurningAxle2() + turningBaselineDistance_ * turningBaselineDistance_ );
   }
}

// Moving in one ms
void
CarPhysics::move_in_a_millisecond() const {
   calculateTurningRadiusAndBaseline();

   if ( fabs( params_.getTurningConstAngle() ) )
   {
      angleOfCarOrientation_ += sign( wheelOrientation_ ) * params_.getTurningConstAngle() * 180. / PI * DELTA_T;
   } else {
      angleOfCarOrientation_ += sign( wheelOrientation_ ) * ( speed_ / turningRadius_ ) * 180. / PI * DELTA_T;
   }

   const double angleOfCarOrientationInRad = angleOfCarOrientation_ / 180. * PI ;

   x_ -= speed_ * std::sin( angleOfCarOrientationInRad ) * DELTA_T;
   y_ += speed_ * std::cos( angleOfCarOrientationInRad ) * DELTA_T;

   correctingWheelOrientation();

   // maximal speed correction
   const double currentMaximalSpeed = params_.getMaximalSpeed() * ( wheelsOnAsphalt() > 2 ? 1.0 : 0.35 );
   
   double acceleration = ( static_cast<double>( actionAccelerating_ ) * params_.getAcceleration()
                         - ( 1. - static_cast<double>( actionAccelerating_ ) ) * ( params_.getAcceleration() + params_.getDecelerationMinusAcceleration() ) ) * DELTA_T ;
   if ( speed_ > params_.getMaximalTurningSpeed() && sign( wheelOrientation_ ) != 0. && acceleration > -params_.getTurningDeceleration() * DELTA_T ) {
      acceleration = -params_.getTurningDeceleration()  * DELTA_T;
   }
   if ( speed_ > currentMaximalSpeed && acceleration > -( params_.getAcceleration() + params_.getDecelerationMinusAcceleration() ) * DELTA_T  ) {
     acceleration = -( params_.getAcceleration() + params_.getDecelerationMinusAcceleration() ) * DELTA_T;
   }

   speed_ += acceleration;

   if ( speed_ < 0. ) {
      speed_ = 0.;
   }
}

void
CarPhysics::move( int passed_time_in_ms ) const {
   for ( int i = 0; i < passed_time_in_ms; ++i ) {
      move_in_a_millisecond();
   }
}
