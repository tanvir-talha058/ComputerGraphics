#include <GL/glut.h>
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace std;

// Global variables to store line coordinates
int startX, startY, endX, endY;

void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);  // White background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 800, 0, 600);
    glMatrixMode(GL_MODELVIEW);
}

void setPixel(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

void ddaLine(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;
    
    // Calculate steps required for generating pixels
    int steps = max(abs(dx), abs(dy));
    
    // Calculate increment in x & y for each step
    float xIncrement = float(dx) / float(steps);
    float yIncrement = float(dy) / float(steps);
    
    // Put pixel for each step
    float x = x1;
    float y = y1;
    
    for (int i = 0; i <= steps; i++) {
        setPixel(round(x), round(y));
        x += xIncrement;
        y += yIncrement;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Set line color to red
    glColor3f(1.0, 0.0, 0.0);
    glPointSize(2.0);
    
    // Draw the line using DDA algorithm
    ddaLine(startX, startY, endX, endY);
    
    // Draw coordinate axes for reference
    glColor3f(0.5, 0.5, 0.5);
    glPointSize(1.0);
    
    // Draw X-axis
    ddaLine(0, 300, 800, 300);
    
    // Draw Y-axis
    ddaLine(400, 0, 400, 600);
    
    glFlush();
}

void getUserInput() {
    cout << "\n=== DDA Line Drawing Algorithm ===" << endl;
    cout << "Enter coordinates for the line:" << endl;
    
    cout << "Enter starting point (x1, y1): ";
    cin >> startX >> startY;
    
    cout << "Enter ending point (x2, y2): ";
    cin >> endX >> endY;
    
    cout << "\nLine will be drawn from (" << startX << ", " << startY << ") to (" << endX << ", " << endY << ")" << endl;
    cout << "Close the graphics window to draw another line or exit." << endl;
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { // ESC key
        exit(0);
    } else if (key == 'n' || key == 'N') {
        getUserInput();
        glutPostRedisplay();
    }
}

void displayMenu() {
    cout << "\nControls:" << endl;
    cout << "Press 'N' to draw a new line" << endl;
    cout << "Press 'ESC' to exit" << endl;
}

int main(int argc, char** argv) {
    // Get user input for line coordinates
    getUserInput();
    
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("DDA Line Drawing Algorithm");
    
    // Initialize OpenGL
    init();
    
    // Set callback functions
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    
    // Display menu
    displayMenu();
    
    // Start the main loop
    glutMainLoop();
    
    return 0;
}
