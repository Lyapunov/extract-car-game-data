#ifndef DRAWABLE_H
#define DRAWABLE_H

#include "Containers.h"

class Drawable {
public:
   Drawable() {}
   virtual ~Drawable() {}
   virtual void drawGL() const = 0;
};

class DrawableContainer : public Drawable, public Container<Drawable> {
public:
   virtual ~DrawableContainer() {}
   virtual void drawGL() const override;
};

#endif /* DRAWABLE_H */
