#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <vector>

template <class T>
class Container : public std::vector< const T* > {
public:
   void addChild( const T& child ) { container_.push_back( &child ); }
   const std::vector< const T* >& getChildren() const { return container_; } 

private:
   std::vector< const T* > container_;
};

#endif /* CONTAINERS_H */
