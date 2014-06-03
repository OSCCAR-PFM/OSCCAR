//this function reads three integer from terminal and stores them in a vector
//this vector is then returned

vector readFromTerminal()
{
    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<//
    //- function body -


    vector input;         //- declare a vector called input

    for(int i = 0; i < 3; i++)
    {
        Info << "type 0 or 1 and then enter" << endl;
        cin >> input[i];  //- set the entry "i" of the vector with input data from terminal
    }



    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<//

    return input;
}
