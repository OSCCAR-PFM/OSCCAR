
class cPolygon{
protected:  //- allow derived classes acces 
  //- members
    int width_;
    int height_;

public:   

  // member functions  
    //- set members
    inline void setValues(int a, int b);

    //- calculations
    int contour();
};

#include "cPolygonI.H" // include the inline functions
