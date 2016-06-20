#include "sign.h"

const double NUMERICAL_ERROR = 1e-10;

// http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
double sign( const double number ) { return ( number > NUMERICAL_ERROR ) - ( number < -NUMERICAL_ERROR ); }

