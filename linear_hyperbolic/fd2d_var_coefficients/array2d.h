#ifndef __ARRAY2D_H__
#define __ARRAY2D_H__

#include <vector>
#include <iostream>
#include <iomanip> //Used to define setw
//https://stdcxx.apache.org/doc/stdlibref/iomanip-h.html#:~:text=The%20header%20is%20part,the%20state%20of%20iostream%20objects.
#include <cassert>

using namespace std;

class Array2D
{
public:
   Array2D(); //Empty constructor declared
   Array2D(const int nx, const int ny,
           const int ng = 0/*Default value*/); 
   void resize (const int nx, const int ny);
   void resize (const int nx, const int ny, const int ng);
   int a,b; //a,b are chosen such that
   //A(i,j) = u[a + i + j*b].
   //Actually, a = (2ng+nx+1)*ng, b = nx+2*ng
   //It is for optimizaiton that we are storing a,b separately.

   int sizex() const;
   int sizey() const;
   double  operator()(const int i, const int j) const;
   double& operator()(const int i, const int j);
   Array2D& operator= (const double scalar);
   Array2D& operator= (const Array2D& u);
   //Overloads '<<', combining it with cout prints array without ghost cells
   //Question - Why is it inside the class?
   //'<<' overloaded as a friend so that it can access class variables.
   //The output is std::ostream format. When you call cout, it probably creates
   //and ostream object os which is fed to the arrows.
   //http://www.cplusplus.com/forum/beginner/99332/#msg534235
   //https://stackoverflow.com/questions/5508857/how-does-cout-actually-work
   //https://www.learncpp.com/cpp-tutorial/introduction-to-iostream-cout-cin-and-endl/
    friend std::ostream& operator<< (std::ostream&  os,
                                     const Array2D& A) 
    {
      for(int i=0; i<A.sizex(); ++i)
      {
        for(int j=0; j<A.sizey(); ++j)
          os << std::setw(10) << A(i,j); //Sets width to be 10
        os << std::endl;
      }
      return os;
    }
    void print_all(bool label = false);
   
private:
  //We put ghost layers, which are extra columns/rows in our array
  //ng gives the number of ghost layers to be put on sides. If we put ng = 1,
  //we put one ghost layer each on left, right, top and bottom.
   int nx, ny, n, ng;
   std::vector<double> u;
};


// Empty constructor
Array2D::Array2D ()
:
nx (0),
ny (0),
ng (0)
{
  n = 0;
  a = (2*ng+nx+1)*ng;
  b = nx+2*ng;
}
//We can specify a layer of ghost cells 
//Adding a print function.
// Constructor based on size
Array2D::Array2D (const int nx, const int ny,
                  const int ng)
:
//Even though the actual size of array will be more, from the user's
//point of view, we'd like the size to be nx,ny as before.
//We will put the ghost cells in places like -1 and nx + 1
nx (nx),
ny (ny),
ng (ng)
{
  n  = (nx+2*ng)*(ny+2*ng);
  u.resize((nx+2*ng)*(ny+2*ng));
  a = (2*ng+nx+1)*ng;
  b = nx+2*ng;
}

//Overload constructor.



// Change size of array, keeping same ghost cell sizes.
void Array2D::resize(const int nx1, const int ny1)
{
  //Remember that nx, ny are sizes without ghost cells.
  nx = nx1;
  ny = ny1;
  n  = (nx1+2*ng) * (ny1+2*ng);
  u.resize (n);
  a = (2*ng+nx+1)*ng;
  b = nx+2*ng;
}
void Array2D::resize(const int nx1, const int ny1, const int ng1)
{
  //Remember that nx, ny are sizes without ghost cells.
  nx = nx1;
  ny = ny1;
  ng = ng1;
  n  = (nx1+2*ng) * (ny1+2*ng);
  u.resize (n);
  a = (2*ng+nx+1)*ng;
  b = nx+2*ng;
}

// return number of rows, size of first index
int Array2D::sizex() const
{
   return nx;
}

// return number of columns, size of second index
int Array2D::sizey() const
{
   return ny;
}

// Return value at (i,j), this is read only(Note the absence of &)
double Array2D::operator() (const int i, const int j) const
{
#ifdef DEBUG /* g++ -o output main.cc -DDEBUG*/
   if (i>=nx + ng || j>=ny + ng || i < -ng || j < -ng ) //Move to debug. Debugging option in makefile.
   {
   cout << "Attempt to access non-existent array entries"<<endl;
   cout << "Array has rows, columns of sizes " <<  nx <<","<< ny << endl;
   cout << "Ghost layer is of size " << ng<<endl;
   cout << "Tried to access row, column position " << i <<"," <<j << endl;
   assert(false);
   }
#endif
   return u[a + i + j*b];
}

// Return reference to (i,j), this can modify the value
double& Array2D::operator() (const int i, const int j)
{
#ifdef DEBUG /* g++ -o output main.cc -DDEBUG*/
   if (i>=nx + ng || j>=ny + ng || i < -ng || j < -ng ) //Move to debug. Debugging option in makefile.
   {
   cout << "Attempt to access non-existent array entries"<<endl;
   cout << "Array has rows, columns of sizes " <<  nx <<","<< ny << endl;
   cout << "Ghost layer is of size " << ng<<endl;
   cout << "Tried to access row, column position " << i <<"," <<j << endl;
   assert(false);
   }
#endif
   return u[a + i + j*b];
}

// Set all elements to scalar value
Array2D& Array2D::operator= (const double scalar)
{
   for (int i=0; i<n; ++i)
      u[i] = scalar;
   return *this; //'this' is just the pointer to the class object that is 
   //automatically created within class functions so that the compiler
   //knows which object the class function is working on.
   //https://www.learncpp.com/cpp-tutorial/8-8-the-hidden-this-pointer/
}

// Copy array a into this one
Array2D& Array2D::operator= (const Array2D& a)
{
   u = a.u; //The u on left is the local variable of LHS array
   //while a.u prints the local variable of RHS array.
   //C++ is allowing us to extract the private variable of 'a' because this is
   //a class function
   return *this;
}

void Array2D::print_all(bool label)
{
  if (label == true)
  {
    for (int j = -ng; j<ny+ng; j++)
    {
      for (int i = -ng; i<nx+ng;i++)
        {
          cout << "A["<<i<<","<<j<<"]="<< u[a + i + j*b] << "   ";
        }
      cout << endl;
    }
  }
  else
  {
    for (int j = -ng; j<ny+ng; j++)
    {
      for (int i = -ng; i<nx+ng;i++)
        {
          cout << u[a + i + j*b] << " ";
        }
      cout << endl;
    }
  }
}

#endif
