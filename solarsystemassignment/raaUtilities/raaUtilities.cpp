#include "StdAfx.h"

#include <windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/GLUT.h>
#include "raaMaths/raaVector.h"
#include "raaUtilities.h"

float g_afSphereAmbient[]={0.1f, 0.3f, 0.1f, 1.0f};
float g_afSphereDiffuse[]={0.1f, 0.8f, 0.1f, 1.0f};
float g_afSphereSpecular[]={1.0f, 1.0f, 1.0f, 1.0f};
float g_fSphereShininess=5.0f;

void gridInit(unsigned long &ulGrid, float *pafColour, int iMin, int iMax, float fCellSize)
{
	float fMin=(float)iMin;
	float fMax=(float)iMax;

	if(!ulGrid) ulGrid=glGenLists(1);

	glNewList(ulGrid, GL_COMPILE);

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glDisable(GL_LIGHTING);

	glColor4fv(pafColour);

	glBegin(GL_LINES);
	for(int i=iMin;i<=iMax;i++)
	{
		glVertex3f(((float)i)*fCellSize, 0.0f, fMin*fCellSize);			
		glVertex3f(((float)i)*fCellSize, 0.0f, fMax*fCellSize);			
		glVertex3f(fMin*fCellSize, 0.0f, ((float)i)*fCellSize);			
		glVertex3f(fMax*fCellSize, 0.0f, ((float)i)*fCellSize);			
	}
	glEnd();

	glPopAttrib();

	glEndList();
}

void gridDraw( unsigned long &ulGrid )
{
	glCallList(ulGrid);
}

void drawSphere( float fRadius, int iSlices, int iSegments )
{
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glEnable(GL_LIGHTING);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, g_afSphereAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, g_afSphereDiffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, g_afSphereSpecular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, g_fSphereShininess);

	glutSolidSphere(fRadius, iSlices, iSegments);
	glPopAttrib();
	glPopMatrix();
}

void colToMat(float* pafCol, float fAlpha)
{
	if (pafCol)
	{
		float afAmb[4], afDif[4], afSpec[4];
		vecScalarProduct(pafCol, 0.25f, afAmb);
		vecScalarProduct(pafCol, 0.8f, afDif);
		vecScalarProduct(pafCol, 1.0f, afSpec);
		afAmb[3] = afDif[3] = afSpec[3] = fAlpha;

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, afAmb);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, afDif);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, afSpec);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 3.0f);
	}
}