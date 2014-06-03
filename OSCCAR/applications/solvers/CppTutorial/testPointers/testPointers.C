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
    test pointer notation

Description
    C++ programming tutorial

Author 
    Christoph Goniva

\*---------------------------------------------------------------------------*/

//include standard IO routines
#include <iostream>

#include "fvCFD.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

//function main
int main(int argc, char *argv[])
{
    int a,b;
    int* pointer;

    // example 1 - set values of a and b using a pointer

    pointer = & a;           //pointerA stores address of a (using reference operator &)
    *pointer = 10;           // set value 10 at memory address of a (using dereference operator *)

    pointer = & b;           //pointerA stores address of a
    *pointer = 20;           // set value 20 at memory address of b (using dereference operator *)

    Info << "a= " << a << endl;  // a=10
    Info << "b= " << b << endl;  // b=20


    // example 2 - copy value a to variable b

    a = 22;                 // declare and define a variable a
    pointer = &a;           // define pointer to the memory of variable a (using reference operator &)
    b = *pointer;           // get value of a and write to b (using dereference operator *)
    Info << "a= " << a << endl;  // a=22
    Info << "b= " << b << endl;  // b=22

    // return of function main
    return 0;
}

// ************************************************************************* //
