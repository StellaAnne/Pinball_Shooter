#include <GLFW\glfw3.h>
#include "linmath.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <vector>
#include <windows.h>
#include <time.h>

using namespace std;


/*

=== HOW TO PLAY ===

Goal: Destroy all destructable blocks, which are initially green.

Instructions:
- Hold spacebar until the firing stick poitns to your desired direction.
- Release spacebar to shoot the ball

Features:
- You can bounce off of the walls (sides of screen), or the brown bricks.
- Targets require three hits to be destroyed.
- The target is initially green
- Upon first hit, the target will turn yellow
- Upon second hit, the target will turn red
- Upon third hit, the target will disappear.
- When all targets are hit, the game will exit

*/


// SETTINGS
const double MAX_HOLD_TIME = 2.0; // stick speed and hold time
const double stickLength = 0.2; // length of the firing stick
const double MAX_BALL_LIFETIME = 2.0; // maximum life of the ball
const double BALL_SHOOT_COOLDOWN = 1.0;  // 1 second cooldown
int ballHitLimit = 2;  // maximum collisions before ball is destroyed. 0 for infinity.
double ballSpeed = 0.03; // Speed of ball
int destructibleBrickCount = 2; // program will exit after this number of destructable bricks are destroyed
const bool EXIT_AFTER_DESTRUCTION = true; // ... unless this is false

// other variables

const float DEG2RAD = 3.14159 / 180;

void processInput(GLFWwindow* window);

enum BRICKTYPE { REFLECTIVE, DESTRUCTABLE };
enum ONOFF { ON, OFF };
double lastBallShotTime = 0.0;
double spaceBarPressTime = -1;
double angleStick;

class Brick
{
public:
	float red, green, blue;
	float x, y, width;
	BRICKTYPE brick_type;
	ONOFF onoff;
	int hitCount = 0;

	Brick(BRICKTYPE bt, float xx, float yy, float ww, float rr, float gg, float bb)
	{
		brick_type = bt; x = xx; y = yy, width = ww; red = rr, green = gg, blue = bb;
		onoff = ON;
	};

	void drawBrick()
	{
		if (onoff == ON)
		{
			double halfside = width / 2;

			glColor3d(red, green, blue);
			glBegin(GL_POLYGON);

			glVertex2d(x + halfside, y + halfside);
			glVertex2d(x + halfside, y - halfside);
			glVertex2d(x - halfside, y - halfside);
			glVertex2d(x - halfside, y + halfside);

			glEnd();
		}
	}
};


class Circle
{
public:
	float red, green, blue;
	float radius;
	float x;
	float y;
	float speed = 0.03;
	int direction; // 1=up 2=right 3=down 4=left 5 = up right   6 = up left  7 = down right  8= down left
	bool alive = true;
	float dirX, dirY;
	int edgeHitCount = 0;
	double creationTime;

	Circle(double xx, double yy, double rr, int dir, float rad, float r, float g, float b)
	{
		x = xx;
		y = yy;
		radius = rr;
		red = r;
		green = g;
		blue = b;
		radius = rad;
		direction = dir;
		creationTime = glfwGetTime(); // set ball creation time to current time
	}

	void CheckCollision(Brick* brk)
	{
		float testX = x;
		float testY = y;

		if (x < brk->x - brk->width / 2) testX = brk->x - brk->width / 2;
		if (x > brk->x + brk->width / 2) testX = brk->x + brk->width / 2;
		if (y < brk->y - brk->width / 2) testY = brk->y - brk->width / 2;
		if (y > brk->y + brk->width / 2) testY = brk->y + brk->width / 2;

		float distX = x - testX;
		float distY = y - testY;
		float distance = sqrt((distX * distX) + (distY * distY));

		if (distance <= radius) {

			// Reflect ball direction based on where it hit.
			if (testX != x) dirX = -dirX;
			if (testY != y) dirY = -dirY;

			edgeHitCount++;

			if (brk->brick_type == REFLECTIVE)
			{
				direction = GetRandomDirection();
			}
			else if (brk->brick_type == DESTRUCTABLE)
			{
				brk->hitCount++;
				if (brk->hitCount == 1)
				{
					brk->red = 1.0;   // Change to yellow
					brk->green = 1.0;
					brk->blue = 0.0;
				}
				else if (brk->hitCount == 2)
				{
					brk->red = 1.0;   // Change to red
					brk->green = 0.0;
					brk->blue = 0.0;
				}
				else if (brk->hitCount == 3)
				{
					brk->onoff = OFF;  // Destroy the brick after 3 hits
					destructibleBrickCount--; // fewer bricks to destroy
				}
				alive = false;  // destroy the circle if there's a collision with a brick
			}

			if (ballHitLimit != 0 && edgeHitCount > ballHitLimit) {
				alive = false;
			}
		}
	}

	int GetRandomDirection()
	{
		return (rand() % 8) + 1;
	}

	void MoveOneStep() {
		x += speed * dirX;
		y += speed * dirY;

		if (x < -1 + radius) {
			x = -1 + radius;
			dirX = -dirX;
			edgeHitCount++;
		}
		if (x > 1 - radius) {
			x = 1 - radius;
			dirX = -dirX;
			edgeHitCount++;
		}
		if (y < -1 + radius) {
			y = -1 + radius;
			dirY = -dirY;
			edgeHitCount++;
		}
		if (y > 1 - radius) {
			y = 1 - radius;
			dirY = -dirY;
			edgeHitCount++;
		}


		if (ballHitLimit != 0 && edgeHitCount > ballHitLimit) {
			alive = false; // destroy the ball if it hits the edge more than the limit
		}

		if (glfwGetTime() - creationTime > MAX_BALL_LIFETIME) {
			alive = false; // destroy the ball if it has exceeded its maximum lifetime
		}
	}

	void DrawCircle()
	{
		glColor3f(red, green, blue);
		glBegin(GL_POLYGON);
		for (int i = 0; i < 360; i++) {
			float degInRad = i * DEG2RAD;
			glVertex2f((cos(degInRad)*radius) + x, (sin(degInRad)*radius) + y);
		}
		glEnd();
	}
};


vector<Circle> world;


int main(void) {
	srand(time(NULL));

	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	GLFWwindow* window = glfwCreateWindow(480, 480, "Pinball Shooter", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	// Bottom bricks, avoid them!
	Brick brick(REFLECTIVE, 0.5, -0.33, 0.3, 0.6, 0.32, 0.17);
	Brick brick3(REFLECTIVE, -0.5, -0.33, 0.3, 0.6, 0.32, 0.17);

	// Top bricks, destroy them!
	Brick brick2(DESTRUCTABLE, -0.7, 0.33, 0.15, 0.2, 0.8, 0.2);
	Brick brick4(DESTRUCTABLE, 0.7, 0.33, 0.15, 0.2, 0.8, 0.2);

	while (!glfwWindowShouldClose(window)) {
		//Setup View
		float ratio;
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		processInput(window);

		//Movement
		for (int i = 0; i < world.size(); i++) {
			if (world[i].alive) {
				world[i].CheckCollision(&brick);
				world[i].CheckCollision(&brick2);
				world[i].CheckCollision(&brick3);
				world[i].CheckCollision(&brick4);
				world[i].MoveOneStep();
				world[i].DrawCircle();
			}
			
		}

		brick.drawBrick();
		brick2.drawBrick();
		brick3.drawBrick();
		brick4.drawBrick();

		// Draw a white stick representing angle
		glColor3f(1.0f, 1.0f, 1.0f);  // white color

		// Compute the angleStick based on elapsed time
		if (spaceBarPressTime == -1) {
			angleStick = 90;  // default position 90 degrees
		}
		else {
			double elapsedTime = glfwGetTime() - spaceBarPressTime;
			double oscillation = 180.0 * (elapsedTime / MAX_HOLD_TIME);  // begin to oscilate starting at the left
			if (oscillation <= 180) {
				angleStick = 180 - oscillation;  // left to right
			}
			else {
				angleStick = oscillation - 180;  // right to left
			}
		}

		double stickEndX = 0 + stickLength * cos(angleStick * DEG2RAD);
		double stickEndY = -1 + stickLength * sin(angleStick * DEG2RAD);

		glBegin(GL_LINES);
		glVertex2f(0.0f, -1.0f);
		glVertex2f(stickEndX, stickEndY);
		glEnd();

		if (destructibleBrickCount == 0 && EXIT_AFTER_DESTRUCTION) {
			// nothing else to do. goodbye!
			glfwSetWindowShouldClose(window, true); 
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate;
	exit(EXIT_SUCCESS);
}


void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && spaceBarPressTime == -1
		&& (glfwGetTime() - lastBallShotTime > BALL_SHOOT_COOLDOWN))
	{
		spaceBarPressTime = glfwGetTime();
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE && spaceBarPressTime != -1) {
		double elapsedTime = glfwGetTime() - spaceBarPressTime;
		spaceBarPressTime = -1;

		double angle = 45 + (elapsedTime / MAX_HOLD_TIME) * 90.0;

		double dx = cos(angleStick * DEG2RAD);
		double dy = sin(angleStick * DEG2RAD);

		// maintain constant speed
		double magnitude = sqrt(dx * dx + dy * dy);
		dx /= magnitude;
		dy /= magnitude;

		double r, g, b;
		r = ((double)rand() / RAND_MAX);
		g = ((double)rand() / RAND_MAX);
		b = ((double)rand() / RAND_MAX);

		Circle B(0, -0.95, 0.05, 1, 0.05, r, g, b);
		B.speed = ballSpeed;
		B.dirX = dx;
		B.dirY = dy;

		lastBallShotTime = glfwGetTime();

		world.push_back(B);
	}
}