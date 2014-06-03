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
    gamble

Description
    C++ programming tutorial - guess two numbers

Author 
    Christoph Goniva

\*---------------------------------------------------------------------------*/

//include OpenFoam basic routines
#include "fvCFD.H"

//include OpenFoam random class
#include "Random.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

//function main
int main(int argc, char *argv[])
{
    #include "setRootCase.H"                    //- create an argList object
    #include "createTime.H"                     //- create a time object

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
    //- PART 1 - create a random vector called "given"

    time_t cpuTime = Foam::clock().getTime();   //- Get the current clock time in seconds

    Random rand(cpuTime);                       //- Make a Random Object using cpuTime as seed

    vector given = rand.vector01();             //- Make a vector with random entries (between 0 and 1)
    // "given" is now a 3 component vector with entries between 0 and 1 (e.g. (0.23,0.89,0.123))
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
    //- PART 2 - Modify the vector to get 0 or 1 as entry

    int i = 0;
    while(i < 3)
    {
        given[i] = floor(given[i]+0.5);  //- change the entry "i" of the vector
        i++;
    }     
    // "given" is now a 3 component vector with entries 0 or 1 (e.g. (0,1,0) or (1,1,0) etc.)
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
    //- PART 3 - read a vector from the terminal
    Info << "LET US GAMBLE - guess a vector" << endl;

    vector input;         //- declare a vector called input

    for(int i = 0; i < 3; i++)
    {
        Info << "type 0 or 1 and then enter" << endl;
        cin >> input[i];  //- set the entry "i" of the vector with input data from terminal
    }
    
    Info << "\nyour guess:" << input << endl;
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
    //- PART 4 - compare the vectors "given" and "input"
    if(given == input)
    {
        Info << "\ncongratulations, you won!!" << endl;
    }
    else
    {
        Info << "\nSorry you lose!!!" << endl;
        Info << "\nThe correct vector is:" << given << endl;
    }
    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    // return of function main
    return 0;
}

// ************************************************************************* //
