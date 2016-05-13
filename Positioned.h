#ifndef POSITIONED_H
#define POSITIONED_H

#include <string>

#include "Containers.h"

class Positioned {
public:
   Positioned( double x = 0., double y = 0. ) : x_( x ), y_( y ) {}
   virtual ~Positioned() {}

   void setX( double x ) { x_ = x; }
   void setY( double y ) { y_ = y; }
   double getX() const { return x_;  }
   double getY() const { return y_;  }
   virtual void move( int passed_time_in_ms ) const = 0;
   virtual bool hasAttribute( const std::string& attribute, double x, double y ) const = 0;

protected:
   mutable double x_;
   mutable double y_;
};

class PositionedContainer : public Positioned, public Container<Positioned> {
public:
   virtual void move( int passed_time_in_ms ) const override;
   virtual bool hasAttribute( const std::string& attribute, double x, double y ) const override;
};

#endif /* POSITIONED_H */
