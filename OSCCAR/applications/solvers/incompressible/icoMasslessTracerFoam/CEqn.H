// Solve the transport equation for massless tracer C
solve
(
    fvm::ddt(C)
  + fvm::div(phi, C)
  - fvm::laplacian(DC, C)
);
