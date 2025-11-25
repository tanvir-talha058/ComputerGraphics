#include <GL/glut.h>
#include <GL/glu.h>

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.0, 0.6, 0.0);
    glBegin(GL_POLYGON);
        glVertex2f(0.3, 0.2);
        glVertex2f(0.7, 0.2);
        glVertex2f(0.7, 0.6);
        glVertex2f(0.3, 0.6);
    glEnd();
    glColor3f(0.8, 0.0, 0.0);
    glBegin(GL_TRIANGLES);
        glVertex2f(0.25, 0.6);
        glVertex2f(0.75, 0.6);
        glVertex2f(0.5, 0.85);
    glEnd();
    glFlush();
}
void init() {
    glClearColor(0.6, 0.8, 1.0, 1.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
}
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(400, 400);
    glutInitWindowPosition(200, 200);
    glutCreateWindow("Simple House (Triangle + Square)");
    init();
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
