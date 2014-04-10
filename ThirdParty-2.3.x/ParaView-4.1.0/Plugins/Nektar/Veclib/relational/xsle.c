/*
 * scalar less than or equal to vector
 */

void dsle(int n, double *ap, double *x, int incx, int *y, int incy)
{
  register double alpha = *ap;

  while( n-- ) {
    *y = alpha <= (*x);
    x += incx;
    y += incy;
  }
  return;
}
