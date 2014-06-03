/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2009 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

Application
    myFirstClass

Description
    C++ programming tutorial - classes and inheritance...

Author 
    Christoph Goniva

\*---------------------------------------------------------------------------*/

//- include standard IO routines
#include <iostream>

//- include my cPolygon class (base class)
#include "cPolygon.H"

//- include my derived cRectangle class (derived class)
#include "cRectangle.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

//- function main
main(int argc, char *argv[])
{
    //- define an object of type cRectangle (derived from cPolygon)
    cRectangle rect(10,2);

    //- use the member function contour of the cPolygon class
    std::cout << "the contour is: " << rect.contour() << "\n";

    //- use the member function contour of the cRectangle class
    std::cout << "the area is: " << rect.area() << "\n";
}

// ************************************************************************* //
