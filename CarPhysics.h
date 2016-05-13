#ifndef CARPHYSICS_H
#define CARPHYSICS_H

#include "Positioned.h"

class CarPhysicalParameters {
public:
   CarPhysicalParameters(  double carWidth = 50.,
                           double carHeight = 100.,
                           double maximalSteeringAngle = 40.,
                           double steeringSpeed = 60.,
                           double maximalSpeed = 200.,
                           double maximalTurningSpeed = 150.,
                           double acceleration = 40.,
                           double decelerationMinusAcceleration = 20.,
                           double relativeDistanceBetweenCenterAndTurningAxle =0.5,
                           double turningConstAngle = 0.,
                           double turningDeceleration = 20. )
    : carWidth_(carWidth),
      carHeight_(carHeight),
      maximalSteeringAngle_(maximalSteeringAngle),
      steeringSpeed_(steeringSpeed),
      maximalSpeed_(maximalSpeed),
      maximalTurningSpeed_(maximalTurningSpeed),
      acceleration_(acceleration),
      decelerationMinusAcceleration_(decelerationMinusAcceleration),
      relativeDistanceBetweenCenterAndTurningAxle_(relativeDistanceBetweenCenterAndTurningAxle), // to me 0.5 is natural
      turningConstAngle_(turningConstAngle), // death rally should use it instead of speed / radius ( maybe calculating radius is too expensive ) - use 1.
      turningDeceleration_(turningDeceleration), // not realistic physically

      // simply calculated values
      distanceBetweenCenterAndTurningAxle_( relativeDistanceBetweenCenterAndTurningAxle_ * carHeight_ ),
      distanceBetweenCenterAndTurningAxle2_( distanceBetweenCenterAndTurningAxle_ * distanceBetweenCenterAndTurningAxle_ ),
      carHeightUpper_( ( 0.5 + relativeDistanceBetweenCenterAndTurningAxle_ ) * carHeight_ ), 
      carHeightLower_( ( 0.5 - relativeDistanceBetweenCenterAndTurningAxle_ ) * carHeight_ ),
      carHeightMagicProduct_ ( carHeightUpper_ * carHeightLower_ )
   {}

   double getCarWidth() const { return carWidth_; }
   double getCarHeight() const { return carHeight_; }
   double getMaximalSteeringAngle() const { return maximalSteeringAngle_; }
   double getSteeringSpeed() const { return steeringSpeed_; }
   double getMaximalSpeed() const { return maximalSpeed_; }
   double getMaximalTurningSpeed() const { return maximalTurningSpeed_; }
   double getAcceleration() const { return acceleration_; }
   double getDecelerationMinusAcceleration() const { return decelerationMinusAcceleration_; }
   double getRelativeDistanceBetweenCenterAndTurningAxle() const { return relativeDistanceBetweenCenterAndTurningAxle_; }
   double getTurningConstAngle() const { return turningConstAngle_; }
   double getTurningDeceleration() const { return turningDeceleration_; }
   double getDistanceBetweenCenterAndTurningAxle() const { return distanceBetweenCenterAndTurningAxle_; }
   double getDistanceBetweenCenterAndTurningAxle2() const { return distanceBetweenCenterAndTurningAxle2_; }
   double getCarHeightUpper() const { return carHeightUpper_; }
   double getCarHeightLower() const { return carHeightLower_; }

   double getTurningBaseline( const double alpha ) const;
private:
   double calculatingMagicNumberB( const double alpha ) const;

   const double carWidth_;
   const double carHeight_;
   const double maximalSteeringAngle_;
   const double steeringSpeed_;
   const double maximalSpeed_;
   const double maximalTurningSpeed_;
   const double acceleration_;
   const double decelerationMinusAcceleration_;
   const double relativeDistanceBetweenCenterAndTurningAxle_; // to me 0.5 is natural
   const double turningConstAngle_; // death rally should use it instead of speed / radius ( maybe calculating radius is too expensive ) - use 1.
   const double turningDeceleration_;  // not realistic physically
   const double distanceBetweenCenterAndTurningAxle_;
   const double distanceBetweenCenterAndTurningAxle2_;
   const double carHeightUpper_;
   const double carHeightLower_;
   const double carHeightMagicProduct_;
};


class CarPhysics : public Positioned {
public:
   CarPhysics( double x, double y, const PositionedContainer& world )
    : Positioned( x, y ), params_(),  speed_( 0. ), drifting_( 0. ), angleOfCarOrientation_( 0. ), wheelOrientation_( 0. ),
      actionTurning_( 0 ), actionAccelerating_( 0 ), turningBaselineDistance_( 0. ), turningRadius_( 0. ) ,
      world_( world ) {}

   virtual bool hasAttribute( const std::string& attribute, double x, double y ) const override { return false; }
   virtual void move( int passed_time_in_ms ) const override;
   void move_in_a_millisecond() const; // Moving in one ms

   void stopTurning() const { actionTurning_ =  0; }
   void turnLeft()    const { actionTurning_ = +1; }
   void turnRight()   const { actionTurning_ = -1; }
   void stopAccelerating() const  { actionAccelerating_ = +0; }
   void accelerate() const  { actionAccelerating_ = +1; }

   double                    wheelAngle( int sx, int sy ) const;
   std::pair<double, double> wheelRelativePosition( int sx, int sy ) const;
   std::pair<double, double> wheelPosition( int sx, int sy ) const;
   std::pair<double, double> carCenterPosition( int sx, int sy ) const { return wheelPosition( sx, sy ); }

   double getSpeed() const { return speed_; }
   double getAngleOfCarOrientation() const { return angleOfCarOrientation_; }
   double getWheelOrientation() const { return wheelOrientation_; }
   double getTurningBaselineDistance() const { return turningBaselineDistance_; }
   double getTurningRadius() const { return turningRadius_; }

   const CarPhysicalParameters& getParams() const { return params_; } // good for drawing

private:
   int  wheelsOnAsphalt() const; // Check wheter the car is out of the race track
   void correctingWheelOrientation() const;
   void calculateTurningRadiusAndBaseline() const;

   const CarPhysicalParameters params_;

   mutable double speed_;
   mutable double drifting_;
   mutable double angleOfCarOrientation_; 
   mutable double wheelOrientation_;

   mutable int actionTurning_;
   mutable int actionAccelerating_;

   mutable double turningBaselineDistance_;
   mutable double turningRadius_;

   const PositionedContainer& world_;
};

#endif /* CARPHYSICS_H */
