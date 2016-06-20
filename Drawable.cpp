#include "Drawable.h"

void 
DrawableContainer::drawGL() const  {
   for ( const auto& elem: this->getChildren() ) {
      elem->drawGL();
   }
}
