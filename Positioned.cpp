#include "Positioned.h"

void
PositionedContainer::move( int passed_time_in_ms ) const {
      for ( const auto& elem: this->getChildren() ) {
         elem->move( passed_time_in_ms );
      }
}

bool
PositionedContainer::hasAttribute( const std::string& attribute, double x, double y ) const {
   for ( const auto& elem: this->getChildren() ) {
      if ( elem->hasAttribute( attribute, x, y ) ) {
         return true;
      }
   }
   return false;
}
