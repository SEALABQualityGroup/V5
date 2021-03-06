/*  -*- c++ -*-
 * $HeadURL: http://rads-svn.sce.carleton.ca:8080/svn/lqn/trunk-V5/lqns/unit-test/disttest.cc $
 *
 * Distribution function tests.
 * ------------------------------------------------------------------------
 *
 * $Id: disttest.cc 13676 2020-07-10 15:46:20Z greg $
 */

#include "testmva.h"
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include "randomvar.h"
#include "vector.h"

char * myName;

static void usage ()
{
    cerr << myName << " " << endl;
    exit( 1 );
}

int main ( int argc, char * argv[] )
{
    int c;

    myName = argv[0];
	
    while (( c = getopt( argc, argv, "" )) != EOF) {
	switch ( c ) {
	default:
	    cerr << "Unkown option." << endl;
	    usage();
	}
    }

    VectorMath<double> t1(6);
    VectorMath<double> A1(6);
    const double u1 = 1.0 / 6.0;;

    VectorMath<double> t2(4);
    VectorMath<double> A2(4);
    const double u2 = 1.0 / 4.0;;

    for ( unsigned i = 1; i <= 6; ++i ) {
	t1[i] = i;
	A1[i] = u1 * static_cast<double>(i);
    }
	
    for ( unsigned i = 1; i <= 4; ++i ) {
	t2[i] = i;
	A2[i] = u2 * static_cast<double>(i);
    }
	
    DiscretePoints cdf1;
    cdf1.setCDF( t1, A1 );
    cdf1.meanVar();
    cout << cdf1 << endl;
    DiscretePoints cdf2;
    cdf2.setCDF( t2, A2 );
    cdf2.meanVar();
    cout << cdf2 << endl;
    DiscretePoints cdf3( cdf2 );

//    cdf2.pointByPointAdd( cdf1 );
    cdf2.pointByPointMul( cdf1 );
    cdf3 *= cdf1;

    cdf2.meanVar();
    cout << cdf2 << endl;
    cdf3.meanVar();
    cout << cdf3 << endl;
//    cdf1 += 2;
//    cdf1.meanVar();
//    cout << cdf1 << endl;
    
    return 0;
}

#include <vector.cc>

template class Vector<double>;
template class Vector<unsigned int>;
template class Vector<unsigned long>;
template class Vector<Server *>;
template class VectorMath<unsigned int>;
template class VectorMath<double>;
template class Vector<Vector<unsigned> >;
template class Vector<VectorMath<double> >;
template class Vector<VectorMath<unsigned> >;

