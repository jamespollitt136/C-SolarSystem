#include "StdAfx.h"
#include <stdlib.h>
#include <time.h>
#include "raaMaths.h"

float degToRad( float f )
{
	return (f/180.0f)*cs_fPi;
}

float redToDeg( float f )
{
	return (f/cs_fPi)*180.0f;
}

void initMaths()
{
	if(!s_bMathsInit)
	{
		srand(time(0));
		s_bMathsInit=true;
	}
}

float randFloat( float fMin, float fMax)
{
	initMaths();
	return (((float)rand() / (float)RAND_MAX)*(fMax - fMin)) + fMin;
}
