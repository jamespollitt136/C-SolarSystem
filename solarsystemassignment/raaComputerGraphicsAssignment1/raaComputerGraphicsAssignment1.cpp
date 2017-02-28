#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include <math.h>
#include <mmsystem.h>
#include "raaCamera/raaCamera.h"
#include "raaUtilities/raaUtilities.h"
#include "raaMaths/raaMaths.h"
#include "raaMaths/raaVector.h"
#include "raaMaths/raaMatrix.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <Windows.h>

// for the right click menu
#define SAVE 1
#define PLANETS 2
#define ADD 3
#define REMOVE 4
#define EXIT 5

// camera
raaCameraInput cameraInput;
raaCamera camera;

int g_aiLastMouse[2];
int g_aiStartMouse[2];

bool g_bExplore = false;
bool g_bFly = false;

void createMenu();
void writeFile();
bool collisionDetection(struct node *pNode, struct node *pNodeB, float distance[3]);

void display();
void idle();
void reshape(int iWidth, int iHeight);
void keyboard(unsigned char c, int iXPos, int iYPos);
void keyboardUp(unsigned char c, int iXPos, int iYPos);
void sKeyboard(int iC, int iXPos, int iYPos);
void sKeyboardUp(int iC, int iXPos, int iYPos);
void mouse(int iKey, int iEvent, int iXPos, int iYPos);
void mouseMotion();
void myInit();

unsigned int lastTime = 0;
const static unsigned int maxNumOfPlanets = 98; // number of 'planets'
float gravity = 0.98f;

struct node *head = NULL;
struct node *tail = NULL;
struct trail *trailHead = NULL;
struct trail *trailTail = NULL;

// the following variables are for statistics/file writing
float sunSize = 0.0f;
float sunMass = 80000.0f;
int collisions = 0;
SYSTEMTIME startTime; // Windows time, set on start up of the program
SYSTEMTIME finishTime; // Windows time, set on exit of the program
int numOfPlanets = 0;
int planetsRemoved = 0;

struct node {
	struct node *next;
	struct node *last;
	float acceleration[4];
	float colour[4];
	float force[4];
	float position[4];
	float rotationAngle;
	float velocity[4];
	float mass;
	float size;
	bool isSun;
};

/*
Push an element, pNode, to the head/front of the linked list.
*/
void pushHead(struct node *pNode) {
	if (!head && !tail) {
		head = pNode;
		tail = pNode;
	}
	else {
		head->last = pNode;
		pNode->next = head;
		head = pNode;
	}
}

/*
Push an element, pNode, to the tail/end of the linked list.
*/
void pushTail(struct node *pNode) {
	if (!head && !tail) {
		head = pNode;
		tail = pNode;
	}
	else {
		struct node *temp = tail;
		tail = pNode;
		tail->last;
		tail->next = NULL;
	}
}

/*
Pop the tail of the linked list
*/
node *popTail() {
	if (tail) {
		struct node *temp = tail;
		if (head == tail) { // if head and tail are the same
			head = 0;
			tail = 0;
		}
		else { // head and tail are not the same
			tail = tail->last;
			tail->next = 0;
			temp->last = 0;
		}
		return temp;
	}
}

/*
Create the planets.
*/
struct node *createElement() {
	struct node *pNode = new node;
	pNode->last = 0;
	pNode->next = 0;
	pNode->isSun = false;
	pNode->mass = randFloat(1.0f, 1000.0f); // values from primer
	pNode->size = pNode->mass / randFloat(80.0f, 100.0f); //size based on mass and a random float for diversity between planets;
	pNode->rotationAngle = randFloat(0.1f, 360.0f); // setting rotation angle 
	for (int i = 0; i <= 3; i++)
	{
		pNode->acceleration[i] = randFloat(-0.000001f, 0.000001f); // testing value
		pNode->force[i] = 0.0f;
		if (pNode->position) {
			pNode->position[0] = randFloat(-700.0f, 700.0f); //testing values
			pNode->position[1] = randFloat(-50.0f, 50.0f);
			pNode->position[2] = randFloat(-700.0f, 700.0f);
		}

		pNode->velocity[i] = randFloat(1.0f, 300.0f); // values from primer

		if (pNode->colour) {
			pNode->colour[0] = randFloat(0.0f, 1.0f);
			pNode->colour[1] = randFloat(0.0f, 1.0f);
			pNode->colour[2] = randFloat(0.0f, 1.0f);
		}

		float vUp[4];
		vecInitDVec(vUp);
		vUp[1] = 1.0;
		float sunPosition[4];
		vecSet(0.0f, 0.0f, 0.0f, sunPosition);

		// distance between the Sun and planets
		float vDir[4];
		vecInitDVec(vDir);
		vecSub(sunPosition, pNode->position, vDir);

		// tangential velocity
		vecCrossProduct(vUp, vDir, pNode->velocity);
		vecNormalise(pNode->velocity, pNode->velocity);
		vecScalarProduct(pNode->velocity, randFloat(1.0f, 300.0f), pNode->velocity);
	}
	pushHead(pNode);
	return pNode;
}

/*
Create a Sun for the system and put it at the head of the linked list.
*/
struct node *createSun() {
	struct node *sNode = new node;
	sNode->next = 0; // head of the linked list
	sNode->last = 0;
	sNode->isSun = true; // sets the bool value as this is a sun
	sNode->mass = 80000.0f; //mass value from primer
	sNode->size = sNode->mass / 100.0f; // size related to mass
	sunSize = sNode->size; // for the statistics in writeFile
	vecSet(255.0f, 184.0f, 0.0f, sNode->colour); // sets sun to a yellow colour
	for (int i = 0; i < 3; i++) { // neater than setting values individually
		sNode->position[i] = 0.0f; // from primer
	}
	pushHead(sNode);
	return sNode;
}

/*
Add a new planet to the system. This function will check that the current number of planets in the system is less than the max number
in order to maintain some control of the system. Currently the system will break when a planet is added and removed however when the
system does crash, you can see that a new planet has been drawn into the system.
*/
void addNewPlanet() {
	if (numOfPlanets < maxNumOfPlanets) {
		createElement();
	}
}

/*
Function for writing to a file. This function should be called on key press or exiting the program.
*/
void writeFile() {
	FILE *filePointer = fopen("solarsystem.txt", "w");
	GetSystemTime(&finishTime);
	if (filePointer == NULL) {
		printf("File I/O error occurred at: writeFile");
		exit(1); // unsuccessful termination
	}
	fprintf(filePointer, "Simulation started at: %d:%d:%d on %d/%d/%d\n", startTime.wHour, startTime.wMinute, startTime.wSecond, startTime.wDay, startTime.wMonth, startTime.wYear);
	fprintf(filePointer, "\n");
	fprintf(filePointer, "Sun mass: %f\n", sunMass);
	fprintf(filePointer, "Sun size: %f\n", sunSize);
	fprintf(filePointer, "Number of planets: %d\n", numOfPlanets);
	fprintf(filePointer, "Collisions occured: %d\n", collisions);
	fprintf(filePointer, "Planets removed: %d\n", planetsRemoved);
	fprintf(filePointer, "\n");
	fprintf(filePointer, "Simulation finished at: %d:%d:%d on %d/%d/%d\n", finishTime.wHour, finishTime.wMinute, finishTime.wSecond, finishTime.wDay, finishTime.wMonth, finishTime.wYear);

	fclose(filePointer);
}

/*
Function for controlling the selection from the right click menu
*/
void rightClick(int selection) {
	switch (selection) {
	case SAVE:
		writeFile(); // write the current stats to a file
		printf("File saved: solarsystem.txt"); // displayed in the window
		break;
	case ADD:
		addNewPlanet(); // add a new planet
		printf("Planet added");
		break;
	case REMOVE:
		popTail(); // remove tail node in linked list
		planetsRemoved + 1;
		printf("Planet removed from tail of linked list");
		break;
	case EXIT:
		writeFile();
		exit(0); // exit the program, successful termination
		break;
	}
}

/*
Function for creating a right-click menu which offers the same features as the window menu.
*/
void createMenu() {
	int menu = glutCreateMenu(rightClick); // create menu and add event handler
	glutAddMenuEntry("Add", ADD); // add entry to menu
	glutAddMenuEntry("Remove", REMOVE); // add entry to menu
	glutAddMenuEntry("Save", SAVE); // add entry to menu
	glutAddMenuEntry("Exit", EXIT); // add entry to menu
	glutAttachMenu(GLUT_RIGHT_BUTTON); // attaches the menu to a button so it appears when that button is clicked
}

/*
Draw and display the planets which are in the linked list
*/
void display() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	camApply(camera);
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glEnable(GL_LIGHTING);

	for (struct node *p = head; p != NULL; p = p->next) {
		colToMat(p->colour + (0)); // sets colour scheme for planets
		glPushMatrix();
		glRotatef(45.0, 1.0, 0.0, 0.0); // rotate on the x axis
		glTranslatef(p->position[0], p->position[1], p->position[2]);
		glutSolidSphere(powf(p->mass, 0.333f), 75, 75); // draw the planet, the arguments are: radius, slices and stacks
		glPopMatrix();
	}
	glPopAttrib();
	glFlush();
	glutSwapBuffers();
}

/*
Check for collisions between anything within the system
*/
bool collisionDetection(struct node *pNode, struct node *qNode, float distance[3]) {
	float colDistance[3];
	vecInit(colDistance);
	for (int i = 0; i < 3; i++) {
		if (distance[i] < 0.0f) {
			colDistance[i] = distance[i] * -1;
		}
		else {
			colDistance[i] = distance[i];
		}
	}

	bool xCollision = colDistance[0] < pNode->size;
	bool yCollision = colDistance[1] < pNode->size;
	bool zCollision = colDistance[2] < pNode->size;

	if (xCollision && yCollision && zCollision) {
		// check which body has the greater mass to determine which to remove
		if (pNode->mass >= qNode->mass) { // if first body has greater mass than the second
			pNode->size = pNode->size + (qNode->size / 10000); // first planet size to increase slightly
			pNode->mass = pNode->mass + qNode->mass; // update the first planet mass to add the colliding planets mass
			if (pNode->mass > 50000.0f) {
				pNode->mass = 20000.0f; // control the mass so it doesnt go close to the sun's attributes
			}
			if (pNode->size > 10000.0f) {
				pNode->size = pNode->size / 2; // control the size so a planet doesnt get as big as the sun
			}
			qNode->size = 0.0f;
			printf("Collision occured \n"); // show a collision has occured in the window
			collisions = collisions + 1;
			//printf("col: %d", collisions); // checking collisions
			qNode = 0;
		}
		else if (qNode->mass > pNode->mass) { // if second body has a greater mass than the first
			qNode->size = qNode->size + (pNode->size / 10000); // second planet size to increase slightly
			qNode->mass = qNode->mass + pNode->mass; // update the second planet mass to add the colliding planets mass
			if (qNode->mass > 50000.0f) {
				qNode->mass = 20000.0f; // control the mass so it doesnt go close to the sun's attributes
			}
			if (qNode->size > 10000.0f) {
				qNode->size = qNode->size / 2; // control the size so a planet doesnt get as big as the sun
			}
			pNode->size = 0.0f;
			printf("Collision occured \n"); // show a collision has occured in the window
			collisions = collisions + 1;
			//printf("col: %d", collisions); // checking collisions
			pNode = 0;
		}
	}
	return false;
}

/*
This is the function which controls the simulation. The planets should rotate counter clockwise in an orbit whilst also being affected
by mass of the sun and also other planets.
*/
void idle() {
	mouseMotion();
	// elements in the linked list
	for (struct node *pNode = head; pNode != NULL; pNode = pNode->next) { // this line causes around 50% of planets to not move
		if (pNode != NULL) {
			float iForce[4];
			vecInitDVec(iForce);
			for (struct node *sNode = head; sNode != NULL; sNode = sNode->next) {
				float distance[3]; //x, y, z
				if (pNode != sNode) {
					vecSub(pNode->position, sNode->position, distance); //get distance

					float sForce = gravity * (pNode->mass * sNode->mass) / ((vecLength(distance))*(vecLength(distance))); //sim primer slide 6
					float normaliseInput[4];
					float normaliseOutput[4];
					vecSub(sNode->position, pNode->position, normaliseInput);
					vecInit(normaliseOutput);
					vecNormalise(normaliseInput, normaliseOutput);
					vecScalarProduct(normaliseOutput, sForce, normaliseOutput); // normalise output needs to be scaled by sForce

					for (int i = 0; i < 3; i++)
					{
						iForce[i] = (iForce[i] + normaliseOutput[i]);
					}
				}

				if (pNode->mass > sNode->mass) {
					if (collisionDetection(pNode, sNode, distance)) { //collision detection
						break;
					}
				}
			}

			for (int i = 0; i < 3; i++) {
				pNode->acceleration[i] = iForce[i] / pNode->mass; //calculate acceleration primer
			}

			pNode->position[0] = pNode->position[0] + (pNode->velocity[0] * 0.001f); //update position primer
			pNode->position[1] = pNode->position[1] + (pNode->velocity[1] * 0.001f); //0.001f is okay for speed of movement
			pNode->position[2] = pNode->position[2] + (pNode->velocity[2] * 0.001f);

			pNode->velocity[0] = pNode->velocity[0] + pNode->acceleration[0]; //update velocity primer
			pNode->velocity[1] = pNode->velocity[1] + pNode->acceleration[1];
			pNode->velocity[2] = pNode->velocity[2] + pNode->acceleration[2];

			pNode->acceleration[0] = pNode->acceleration[0] * 0.98f; //dampening from primer
			pNode->acceleration[1] = pNode->acceleration[1] * 0.98f;
			pNode->acceleration[2] = pNode->acceleration[2] * 0.98f;

			if (pNode->rotationAngle > 359.0f) { // rotation should cause the planet to spin
				pNode->rotationAngle = 0.0f;
			}
			else {
				pNode->rotationAngle = pNode->rotationAngle += 1.0f;
			}

			pNode = pNode->next;
		}
	}
	glutPostRedisplay();
}

void reshape(int iWidth, int iHeight) {
	glViewport(0, 0, iWidth, iHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30.0f, ((float)iWidth) / ((float)iHeight), 0.1f, 10000.0f);
	glMatrixMode(GL_MODELVIEW);
	glutPostRedisplay();
}

/*
This function controls the users input. Switch controls the functions relating to the key entered by the user.
*/
void keyboard(unsigned char c, int iXPos, int iYPos) {
	switch (c) {
	case 'a': // save to file
		writeFile(); // call to writeFile function, will create a txt file with current statistics
		printf("File saved to solarsystem.txt \n"); // testing
		break;
	case 'n':
		addNewPlanet(); // call to addNewPlanet function, will add new planets.
		printf("New planet added \n");
		break;
	case 'r': // remove a planet
		popTail();
		planetsRemoved + 1;
		printf("planet removed \n"); // testing
		break;
	case 's': // zoom out
		camInputTravel(cameraInput, tri_neg);
		break;
	case 'w': //zoom in
		camInputTravel(cameraInput, tri_pos);
		break;
	case 'x': // exit
		writeFile(); // call to writeFile function, will create a txt file with end statistics
		exit(0); // successful termination
		break;
	}
}

void keyboardUp(unsigned char c, int iXPos, int iYPos) {
	switch (c) {
	case 'w':
		camInputTravel(cameraInput, tri_pos);
		break;
	case 's':
		camInputTravel(cameraInput, tri_null);
		break;
	case 'f':
		camInputFly(cameraInput, !cameraInput.m_bFly);
		break;
	}
}

void sKeyboard(int iC, int iXPos, int iYPos) {
	switch (iC) {
	case GLUT_KEY_UP:
		camInputTravel(cameraInput, tri_pos);
		break;
	case GLUT_KEY_DOWN:
		camInputTravel(cameraInput, tri_neg);
		break;
	}
}

void sKeyboardUp(int iC, int iXPos, int iYPos) {
	switch (iC) {
	case GLUT_KEY_UP:
	case GLUT_KEY_DOWN:
		camInputTravel(cameraInput, tri_null);
		break;
	}
}

/*
This function controls the view dispayed when the user clicks and moves the mouse within the system.
*/
void mouse(int iKey, int iEvent, int iXPos, int iYPos) {
	if (iKey == GLUT_LEFT_BUTTON) {
		camInputMouse(cameraInput, (iEvent == GLUT_DOWN) ? true : false);
		if (iEvent == GLUT_DOWN)camInputSetMouseStart(cameraInput, iXPos, iYPos);
	}
}

void motion(int iXPos, int iYPos) {
	if (cameraInput.m_bMouse) {
		camInputSetMouseLast(cameraInput, iXPos, iYPos);
	}
}

void mouseMotion() {
	camProcessInput(cameraInput, camera);
	glutPostRedisplay();
}

void myInit() {
	initMaths();
	for (unsigned int i = 0; i < 48; i++) { // will create 42 planets on start up
		if (i) {
			node *pNode = createElement(); // push is inside createElement
		}
		else {
			node *sNode = createSun(); // push is inside createSun
		}
		numOfPlanets = i; // allows for checking of number of planets when adding a new planet
	}
	camInit(camera);
	camInputInit(cameraInput);
	camInputExplore(cameraInput, true);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // sets the background to black (space)
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

/*
This function is used to display a command list on start up of the simulation in order to allow the user to control some things such as adding
new planets and also removing.
*/
void startup() {
	printf("James Pollitt: Solar System for Computer Graphics\n");
	printf("simulation started at: %d:%d:%d on %d/%d/%d", startTime.wHour, startTime.wMinute, startTime.wSecond, startTime.wDay, startTime.wMonth, startTime.wYear); // Windows time
	printf("\n");
	printf("\tCommand Menu\n");
	printf("\n");
	printf("\tKey \t Action\n");
	printf("\ta \t Save progress to file\n");
	printf("\tn \t Add planet (WILL CRASH THE SYSTEM BUT DOES ADD NEW PLANET)\n");
	printf("\tr \t Delete planet (WILL CRASH THE SYSTEM BUT DOES REMOVE)\n");
	printf("\ts \t Zoom out\n");
	printf("\tw \t Zoom in\n");
	printf("\tx \t Exit (exiting will save your current statistics to a txt file)\n");
	printf("\tdown arrow \t Zoom out \n");
	printf("\tup arrow \t Zoom in \n");
	printf("\tmouse \t Look left or right\n");
	printf("\nExecute commands within the Solar System Simulation Window\n");
	printf("\n");
	printf("Your simulation: \n");
}

int main(int argc, char* argv[]) {
	glutInit(&argc, (char**)argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA); //inits display mode with a depth buffer, double buffering and that shows colour
	GetSystemTime(&startTime);
	glutInitWindowPosition(0, 0); // set the window x/y pixels position from top left
	glutInitWindowSize(1000, 800);  // set the window size
	glutCreateWindow("James Pollitt: Solar System for Computer Graphics"); // create the window with the title given
	startup(); // create the menu list in the window created above

	myInit();

	createMenu();

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutSpecialFunc(sKeyboard);
	glutSpecialUpFunc(sKeyboardUp);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop(); //starts the glut event loop
	return 0;
}