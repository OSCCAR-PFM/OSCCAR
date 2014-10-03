fvScalarMatrix CEqn
(
    fvm::ddt(C)
  + fvm::div(phi, C)
  - fvm::laplacian(DC, C)
);

solve(CEqn);

interpolateC = fvc::interpolate(C);
