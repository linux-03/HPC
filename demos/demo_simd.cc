#include <iostream>
#include <sstream>


#include <simd.h>

using namespace ASC_HPC;
using std::cout, std::endl;

auto Func1 (SIMD<double> a, SIMD<double> b)
{
  return a+b;
}

auto Func2 (SIMD<double,4> a, SIMD<double,4> b)
{
  return a+3*b;
}

auto Func3 (SIMD<double,4> a, SIMD<double,4> b, SIMD<double,4> c)
{
  return FMA(a,b,c);
}


auto Load (double * p)
{
  return SIMD<double,2>(p);
}

auto LoadMask (double * p, SIMD<mask64,2> m)
{
  return SIMD<double,2>(p, m);
}

auto TestSelect (SIMD<mask64,2> m, SIMD<double,2> a, SIMD<double,2> b)
{
  return Select (m, a, b);
}

SIMD<double,2> TestHSum (SIMD<double,4> a, SIMD<double,4> b)
{
  return HSum(a,b);
}



int main()
{
  SIMD<double,4> a(1.,2.,3.,4.);
  SIMD<double,2> x(1.,2.);
  SIMD<double,2> y(1.,2.);
  SIMD<double,4> b(1.0);
  SIMD<double,4> c(1.,2.,3.,4.);
  cout << "a = " << a << endl;
  cout << "b = " << b << endl;
  cout << "a+b = " << a+b << endl;
  auto d =  a*b+c;
  cout << "a*b+c = " << d<< endl;
  cout << "HSum(a) = " << HSum(a) << endl;
  cout << "HSum(a,b) = " << HSum(a,b) << endl;

  
  auto sequ = SIMD<double, 4>(1.0,2.0,3.0,4.0);
  cout << "sequ = " << sequ << endl;
  auto mask = (2.0 >= sequ);
  cout << "2 >= " << sequ << " = " << mask << endl;


  auto simd1 = SIMD<double, 2>(2.,3.);
  cout << "simd1 = " << simd1 << endl;
  auto simd2 = SIMD<double, 2>(2.,4.);
  cout << "simd2 = " << simd2 << endl;
  SIMD<mask64, 2> comp = (simd1 >= simd2);
  cout << "(simd1 >= simd2) = " << comp << endl;

  {
    double a[] = { 10, 10, 10, 10 };
    SIMD<double,4> sa(&a[0], mask);
    cout << "sa = " << sa << endl;
  }

  cout << "Select(mask, a, b) = " << Select(mask, a,b) << endl;
  auto inseq = IndexSequence<int64_t,4>();
  auto mask2 = ((int64_t)2 >= inseq);
  cout <<inseq << endl<< mask2<<endl;
  //cout <<a/b<<endl;
  cout <<x<<endl;
  cout <<y<<endl;
  auto z = x+y;
  cout << z;
}
