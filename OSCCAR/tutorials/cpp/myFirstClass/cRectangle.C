
#include "cPolygon.H"
#include "cRectangle.H"

//- constructor
cRectangle::cRectangle(int a, int b)
{
    setValues(a,b);
}

//- member functions
int cRectangle::area()
{
    return (width_*height_);
}
