#ifndef __ARRAY2D_H__
#define __ARRAY2D_H__

#include <vector>
#include <iomanip>
//https://stdcxx.apache.org/doc/stdlibref/iomanip-h.html#:~:text=The%20header%20is%20part,the%20state%20of%20iostream%20objects.
#include <cassert>
#include <iostream>

using namespace std;

class Array2D
{
public:
   Array2D();
   Array2D(const unsigned int nx, const unsigned int ny);
   void resize (const unsigned int nx, const unsigned int ny);
   unsigned int sizex() const;
   unsigned int sizey() const;
   double  operator()(const unsigned int i, const unsigned int j) const;
   double& operator()(const unsigned int i, const unsigned int j);
   Array2D& operator= (const double scalar);
   Array2D& operator= (const Array2D& u);

  friend std::ostream& operator<< (std::ostream&          os,
                                    const Array2D& A) //Overload.
  {
      for(unsigned int i=0; i<A.sizex(); ++i)
      {
        for(unsigned int j=0; j<A.sizey(); ++j)
            os << std::setw(10) << A(i,j);
        os << std::endl;
      }
      return os;
  }

private:
   unsigned int nx, ny, n;
   std::vector<double> u;
};


// Empty constructor
Array2D::Array2D ()
:
nx (0),
ny (0),
n  (0)
{
}

// Constructor based on size
Array2D::Array2D (const unsigned int nx, const unsigned int ny)
:
nx (nx),
ny (ny),
n  (nx*ny),
u  (nx*ny)
{
}

// Change size of array
void Array2D::resize(const unsigned int nx1, const unsigned int ny1)
{
   nx = nx1;
   ny = ny1;
   n  = nx1 * ny1;
   u.resize (n);
}

// return number of rows, size of first index
unsigned int Array2D::sizex() const
{
   return nx;
}

// return number of columns, size of second index
unsigned int Array2D::sizey() const
{
   return ny;
}

// Return value at (i,j), this is read only
double Array2D::operator() (const unsigned int i, const unsigned int j) const
{
  if ((i>=nx) || (j>=ny))
   {
   cout << "Attempt to access non-existent array entries"<<endl;
   cout << "Array has rows,columns of sizes " <<  nx <<","<< ny << endl;
   cout << "Tried to access row, column position " << i <<"," <<j << endl;
   assert(false);
   }
   return u[i + j*nx];
}

// Return reference to (i,j), this can modify the value
double& Array2D::operator() (const unsigned int i, const unsigned int j)
{
   if ((i>=nx) || (j>=ny))
   {
   cout << "Attempt to access non-existent array entries"<<endl;
   cout << "Array has rows,columns of sizes " <<  nx <<","<< ny << endl;
   cout << "Tried to access row, column position " << i <<"," <<j << endl;
   assert(false);
   }
   return u[i + j*nx];
}

// Set all elements to scalar value
Array2D& Array2D::operator= (const double scalar)
{
   for (unsigned int i=0; i<n; ++i)
      u[i] = scalar;
   return *this;
}

// Copy array a into this one
Array2D& Array2D::operator= (const Array2D& a)
{
   u = a.u;
   return *this;
}


#endif
