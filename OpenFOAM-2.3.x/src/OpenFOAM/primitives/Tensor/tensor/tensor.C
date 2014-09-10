/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2014 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "tensor.H"
#include "mathematicalConstants.H"

using namespace Foam::constant::mathematical;

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    template<>
    const char* const tensor::typeName = "tensor";

    template<>
    const char* tensor::componentNames[] =
    {
        "xx", "xy", "xz",
        "yx", "yy", "yz",
        "zx", "zy", "zz"
    };

    template<>
    const tensor tensor::zero
    (
        0, 0, 0,
        0, 0, 0,
        0, 0, 0
    );

    template<>
    const tensor tensor::one
    (
        1, 1, 1,
        1, 1, 1,
        1, 1, 1
    );

    template<>
    const tensor tensor::max
    (
        VGREAT, VGREAT, VGREAT,
        VGREAT, VGREAT, VGREAT,
        VGREAT, VGREAT, VGREAT
    );

    template<>
    const tensor tensor::min
    (
        -VGREAT, -VGREAT, -VGREAT,
        -VGREAT, -VGREAT, -VGREAT,
        -VGREAT, -VGREAT, -VGREAT
    );

    template<>
    const tensor tensor::I
    (
        1, 0, 0,
        0, 1, 0,
        0, 0, 1
    );
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Foam::vector Foam::eigenValues(const tensor& t)
{
<<<<<<< HEAD
<<<<<<< HEAD
    scalar i = 0;
    scalar ii = 0;
    scalar iii = 0;

=======
    // The eigenvalues
    scalar i, ii, iii;

    // diagonal matrix
>>>>>>> OpenFOAM/master
=======
    // The eigenvalues
    scalar i, ii, iii;

    // diagonal matrix
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    if
    (
        (
            mag(t.xy()) + mag(t.xz()) + mag(t.yx())
<<<<<<< HEAD
<<<<<<< HEAD
          + mag(t.yz()) + mag(t.zx()) + mag(t.zy())
=======
            + mag(t.yz()) + mag(t.zx()) + mag(t.zy())
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
        )
        < SMALL
    )
    {
<<<<<<< HEAD
        // diagonal matrix
=======
            + mag(t.yz()) + mag(t.zx()) + mag(t.zy())
        )
        < SMALL
    )
    {
>>>>>>> OpenFOAM/master
=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
        i = t.xx();
        ii = t.yy();
        iii = t.zz();
    }
<<<<<<< HEAD
<<<<<<< HEAD
=======

    // non-diagonal matrix
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    else
    {
        // Coefficients of the characteristic polynmial
        // x^3 + a*x^2 + b*x + c = 0
        scalar a =
           - t.xx() - t.yy() - t.zz();

        scalar b =
            t.xx()*t.yy() + t.xx()*t.zz() + t.yy()*t.zz()
          - t.xy()*t.yx() - t.yz()*t.zy() - t.zx()*t.xz();

        scalar c =
          - t.xx()*t.yy()*t.zz()
          - t.xy()*t.yz()*t.zx() - t.xz()*t.zy()*t.yx()
          + t.xx()*t.yz()*t.zy() + t.yy()*t.zx()*t.xz() + t.zz()*t.xy()*t.yx();

        // Auxillary variables
        scalar aBy3 = a/3;

        scalar P = (a*a - 3*b)/9; // == -p_wikipedia/3
        scalar PPP = P*P*P;

        scalar Q = (2*a*a*a - 9*a*b + 27*c)/54; // == q_wikipedia/2
        scalar QQ = Q*Q;

        // Three identical roots
        if (mag(P) < SMALL && mag(Q) < SMALL)
        {
            return vector(- aBy3, - aBy3, - aBy3);
        }

        // Two identical roots and one distinct root
        else if (mag(PPP/QQ - 1) < SMALL)
        {
            scalar sqrtP = sqrt(P);
            scalar signQ = sign(Q);

            i = ii = signQ*sqrtP - aBy3;
            iii = - 2*signQ*sqrtP - aBy3;
        }

        // Three distinct roots
        else if (PPP > QQ)
        {
            scalar sqrtP = sqrt(P);
            scalar value = cos(acos(Q/sqrt(PPP))/3);
            scalar delta = sqrt(3 - 3*value*value);

            i = - 2*sqrtP*value - aBy3;
            ii = sqrtP*(value + delta) - aBy3;
            iii = sqrtP*(value - delta) - aBy3;
        }

        // One real root, two imaginary roots
        // based on the above logic, PPP must be less than QQ
        else
        {
            WarningIn("eigenValues(const tensor&)")
                << "complex eigenvalues detected for tensor: " << t
                << endl;

            if (mag(P) < SMALL)
            {
                i = cbrt(QQ/2);
            }
            else
            {
                scalar w = cbrt(- Q - sqrt(QQ - PPP));
                i = w + P/w - aBy3;
            }

            return vector(-VGREAT, i, VGREAT);
        }
    }

<<<<<<< HEAD

=======

    // non-diagonal matrix
    else
    {
        // Coefficients of the characteristic polynmial
        // x^3 + a*x^2 + b*x + c = 0
        scalar a =
           - t.xx() - t.yy() - t.zz();

        scalar b =
            t.xx()*t.yy() + t.xx()*t.zz() + t.yy()*t.zz()
          - t.xy()*t.yx() - t.yz()*t.zy() - t.zx()*t.xz();

        scalar c =
          - t.xx()*t.yy()*t.zz()
          - t.xy()*t.yz()*t.zx() - t.xz()*t.zy()*t.yx()
          + t.xx()*t.yz()*t.zy() + t.yy()*t.zx()*t.xz() + t.zz()*t.xy()*t.yx();

        // Auxillary variables
        scalar aBy3 = a/3;

        scalar P = (a*a - 3*b)/9; // == -p_wikipedia/3
        scalar PPP = P*P*P;

        scalar Q = (2*a*a*a - 9*a*b + 27*c)/54; // == q_wikipedia/2
        scalar QQ = Q*Q;

        // Three identical roots
        if (mag(P) < SMALL && mag(Q) < SMALL)
        {
            return vector(- aBy3, - aBy3, - aBy3);
        }

        // Two identical roots and one distinct root
        else if (mag(PPP/QQ - 1) < SMALL)
        {
            scalar sqrtP = sqrt(P);
            scalar signQ = sign(Q);

            i = ii = signQ*sqrtP - aBy3;
            iii = - 2*signQ*sqrtP - aBy3;
        }

        // Three distinct roots
        else if (PPP > QQ)
        {
            scalar sqrtP = sqrt(P);
            scalar value = cos(acos(Q/sqrt(PPP))/3);
            scalar delta = sqrt(3 - 3*value*value);

            i = - 2*sqrtP*value - aBy3;
            ii = sqrtP*(value + delta) - aBy3;
            iii = sqrtP*(value - delta) - aBy3;
        }

        // One real root, two imaginary roots
        // based on the above logic, PPP must be less than QQ
        else
        {
            WarningIn("eigenValues(const tensor&)")
                << "complex eigenvalues detected for tensor: " << t
                << endl;

            if (mag(P) < SMALL)
            {
                i = cbrt(QQ/2);
            }
            else
            {
                scalar w = cbrt(- Q - sqrt(QQ - PPP));
                i = w + P/w - aBy3;
            }

            return vector(-VGREAT, i, VGREAT);
        }
    }

>>>>>>> OpenFOAM/master
=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    // Sort the eigenvalues into ascending order
    if (i > ii)
    {
        Swap(i, ii);
    }

    if (ii > iii)
    {
        Swap(ii, iii);
    }

    if (i > ii)
    {
        Swap(i, ii);
    }

    return vector(i, ii, iii);
}


<<<<<<< HEAD
<<<<<<< HEAD
Foam::vector Foam::eigenVector(const tensor& t, const scalar lambda)
=======
Foam::vector Foam::eigenVector
(
    const tensor& t,
    const scalar lambda
)
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
{
    // Constantly rotating direction ensures different eigenvectors are
    // generated when called sequentially with a multiple eigenvalue
    static vector direction(1,0,0);
    vector oldDirection(direction);
    scalar temp = direction[2];
    direction[2] = direction[1];
    direction[1] = direction[0];
    direction[0] = temp;

    // Construct the linear system for this eigenvalue
    tensor A(t - lambda*I);

    // Determinants of the 2x2 sub-matrices used to find the eigenvectors
    scalar sd0, sd1, sd2;
    scalar magSd0, magSd1, magSd2;

<<<<<<< HEAD
    scalar magSd0 = mag(sd0);
    scalar magSd1 = mag(sd1);
    scalar magSd2 = mag(sd2);
=======
Foam::vector Foam::eigenVector
(
    const tensor& t,
    const scalar lambda
)
{
    // Constantly rotating direction ensures different eigenvectors are
    // generated when called sequentially with a multiple eigenvalue
    static vector direction(1,0,0);
    vector oldDirection(direction);
    scalar temp = direction[2];
    direction[2] = direction[1];
    direction[1] = direction[0];
    direction[0] = temp;

    // Construct the linear system for this eigenvalue
    tensor A(t - lambda*I);

    // Determinants of the 2x2 sub-matrices used to find the eigenvectors
    scalar sd0, sd1, sd2;
    scalar magSd0, magSd1, magSd2;

=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    // Sub-determinants for a unique eivenvalue
    sd0 = A.yy()*A.zz() - A.yz()*A.zy();
    sd1 = A.zz()*A.xx() - A.zx()*A.xz();
    sd2 = A.xx()*A.yy() - A.xy()*A.yx();
    magSd0 = mag(sd0);
    magSd1 = mag(sd1);
    magSd2 = mag(sd2);
<<<<<<< HEAD
>>>>>>> OpenFOAM/master
=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a

    // Evaluate the eigenvector using the largest sub-determinant
    if (magSd0 >= magSd1 && magSd0 >= magSd2 && magSd0 > SMALL)
    {
        vector ev
        (
            1,
            (A.yz()*A.zx() - A.zz()*A.yx())/sd0,
            (A.zy()*A.yx() - A.yy()*A.zx())/sd0
        );
<<<<<<< HEAD
<<<<<<< HEAD
        ev /= mag(ev);

        return ev;
=======

        return ev/mag(ev);
>>>>>>> OpenFOAM/master
=======

        return ev/mag(ev);
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    }
    else if (magSd1 >= magSd2 && magSd1 > SMALL)
    {
        vector ev
        (
            (A.xz()*A.zy() - A.zz()*A.xy())/sd1,
            1,
            (A.zx()*A.xy() - A.xx()*A.zy())/sd1
        );
<<<<<<< HEAD
<<<<<<< HEAD
        ev /= mag(ev);

        return ev;
=======

        return ev/mag(ev);
>>>>>>> OpenFOAM/master
=======

        return ev/mag(ev);
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    }
    else if (magSd2 > SMALL)
    {
        vector ev
        (
            (A.xy()*A.yz() - A.yy()*A.xz())/sd2,
            (A.yx()*A.xz() - A.xx()*A.yz())/sd2,
            1
        );
<<<<<<< HEAD
<<<<<<< HEAD
        ev /= mag(ev);

        return ev;
    }
    else
    {
        return vector::zero;
    }
}


Foam::tensor Foam::eigenVectors(const tensor& t)
{
    vector evals(eigenValues(t));
=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a

        return ev/mag(ev);
    }

<<<<<<< HEAD
        scalar b = t.xx()*t.yy() + t.xx()*t.zz() + t.yy()*t.zz()
            - t.xy()*t.xy() - t.xz()*t.xz() - t.yz()*t.yz();

        scalar c = - t.xx()*t.yy()*t.zz() - t.xy()*t.yz()*t.xz()
            - t.xz()*t.xy()*t.yz() + t.xz()*t.yy()*t.xz()
            + t.xy()*t.xy()*t.zz() + t.xx()*t.yz()*t.yz();

        // If there is a zero root
        if (mag(c) < 1e-100)
        {
            scalar disc = sqr(a) - 4*b;

            if (disc >= -SMALL)
            {
                scalar q = -0.5*sqrt(max(0.0, disc));

                i = 0;
                ii = -0.5*a + q;
                iii = -0.5*a - q;
            }
            else
            {
                FatalErrorIn("eigenValues(const tensor&)")
                    << "zero and complex eigenvalues in tensor: " << t
                    << abort(FatalError);
            }
        }
        else
        {
            scalar Q = (a*a - 3*b)/9;
            scalar R = (2*a*a*a - 9*a*b + 27*c)/54;

            scalar R2 = sqr(R);
            scalar Q3 = pow3(Q);

            // Three different real roots
            if (R2 < Q3)
            {
                scalar sqrtQ = sqrt(Q);
                scalar theta = acos(min(1.0, max(-1.0, R/(Q*sqrtQ))));

                scalar m2SqrtQ = -2*sqrtQ;
                scalar aBy3 = a/3;

                i = m2SqrtQ*cos(theta/3) - aBy3;
                ii = m2SqrtQ*cos((theta + twoPi)/3) - aBy3;
                iii = m2SqrtQ*cos((theta - twoPi)/3) - aBy3;
            }
            else
            {
                scalar A = cbrt(R + sqrt(R2 - Q3));

                // Three equal real roots
                if (A < SMALL)
                {
                    scalar root = -a/3;
                    return vector(root, root, root);
                }
                else
                {
                    // Complex roots
                    WarningIn("eigenValues(const symmTensor&)")
                        << "complex eigenvalues detected for symmTensor: " << t
                        << endl;

                    return vector::zero;
                }
            }
        }
    }


    // Sort the eigenvalues into ascending order
    if (i > ii)
    {
        Swap(i, ii);
    }

    if (ii > iii)
    {
        Swap(ii, iii);
    }

    if (i > ii)
    {
        Swap(i, ii);
    }

    return vector(i, ii, iii);
}


Foam::vector Foam::eigenVector(const symmTensor& t, const scalar lambda)
{
    if (mag(lambda) < SMALL)
    {
        return vector::zero;
    }

    // Construct the matrix for the eigenvector problem
    symmTensor A(t - lambda*I);

    // Calculate the sub-determinants of the 3 components
    scalar sd0 = A.yy()*A.zz() - A.yz()*A.yz();
    scalar sd1 = A.xx()*A.zz() - A.xz()*A.xz();
    scalar sd2 = A.xx()*A.yy() - A.xy()*A.xy();

    scalar magSd0 = mag(sd0);
    scalar magSd1 = mag(sd1);
    scalar magSd2 = mag(sd2);
=======

        return ev/mag(ev);
    }

=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    // Sub-determinants for a repeated eigenvalue
    sd0 = A.yy()*direction.z() - A.yz()*direction.y();
    sd1 = A.zz()*direction.x() - A.zx()*direction.z();
    sd2 = A.xx()*direction.y() - A.xy()*direction.x();
    magSd0 = mag(sd0);
    magSd1 = mag(sd1);
    magSd2 = mag(sd2);
<<<<<<< HEAD
>>>>>>> OpenFOAM/master
=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a

    // Evaluate the eigenvector using the largest sub-determinant
    if (magSd0 >= magSd1 && magSd0 >= magSd2 && magSd0 > SMALL)
    {
        vector ev
        (
            1,
<<<<<<< HEAD
<<<<<<< HEAD
            (A.yz()*A.xz() - A.zz()*A.xy())/sd0,
            (A.yz()*A.xy() - A.yy()*A.xz())/sd0
=======
            (A.yz()*direction.x() - direction.z()*A.yx())/sd0,
            (direction.y()*A.yx() - A.yy()*direction.x())/sd0
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
        );

<<<<<<< HEAD
        return ev;
=======
            (A.yz()*direction.x() - direction.z()*A.yx())/sd0,
            (direction.y()*A.yx() - A.yy()*direction.x())/sd0
        );

        return ev/mag(ev);
>>>>>>> OpenFOAM/master
=======
        return ev/mag(ev);
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    }
    else if (magSd1 >= magSd2 && magSd1 > SMALL)
    {
        vector ev
        (
<<<<<<< HEAD
<<<<<<< HEAD
            (A.xz()*A.yz() - A.zz()*A.xy())/sd1,
=======
            (direction.z()*A.zy() - A.zz()*direction.y())/sd1,
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
            1,
            (A.zx()*direction.y() - direction.x()*A.zy())/sd1
        );

<<<<<<< HEAD
        return ev;
=======
            (direction.z()*A.zy() - A.zz()*direction.y())/sd1,
            1,
            (A.zx()*direction.y() - direction.x()*A.zy())/sd1
        );

        return ev/mag(ev);
>>>>>>> OpenFOAM/master
=======
        return ev/mag(ev);
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
    }
    else if (magSd2 > SMALL)
    {
        vector ev
        (
<<<<<<< HEAD
<<<<<<< HEAD
            (A.xy()*A.yz() - A.yy()*A.xz())/sd2,
            (A.xy()*A.xz() - A.xx()*A.yz())/sd2,
=======
            (A.xy()*direction.z() - direction.y()*A.xz())/sd2,
            (direction.x()*A.xz() - A.xx()*direction.z())/sd2,
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
            1
        );

        return ev/mag(ev);
    }

    // Triple eigenvalue
    return oldDirection;
}


<<<<<<< HEAD
Foam::tensor Foam::eigenVectors(const symmTensor& t)
=======
            (A.xy()*direction.z() - direction.y()*A.xz())/sd2,
            (direction.x()*A.xz() - A.xx()*direction.z())/sd2,
            1
        );

        return ev/mag(ev);
    }

    // Triple eigenvalue
    return oldDirection;
}


Foam::tensor Foam::eigenVectors(const tensor& t)
>>>>>>> OpenFOAM/master
=======
Foam::tensor Foam::eigenVectors(const tensor& t)
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
{
    vector evals(eigenValues(t));

    tensor evs
    (
        eigenVector(t, evals.x()),
        eigenVector(t, evals.y()),
        eigenVector(t, evals.z())
    );

    return evs;
}


<<<<<<< HEAD
<<<<<<< HEAD
=======
=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
Foam::vector Foam::eigenValues(const symmTensor& t)
{
    return eigenValues(tensor(t));
}


Foam::vector Foam::eigenVector(const symmTensor& t, const scalar lambda)
{
    return eigenVector(tensor(t), lambda);
}


Foam::tensor Foam::eigenVectors(const symmTensor& t)
{
    return eigenVectors(tensor(t));
}


<<<<<<< HEAD
>>>>>>> OpenFOAM/master
=======
>>>>>>> e703b792933da4136816a0d0616d2757672e396a
// ************************************************************************* //
