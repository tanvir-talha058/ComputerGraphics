#include <windows.h>
#include <GL/gl.h>
#include <iostream>
#include <cmath>

using namespace std;

// Global variables for circle parameters
int centerX, centerY, radius;
HWND hwnd;

void init() {
    glClearColor(1.0, 1.0, 1.0, 1.0);  // White background
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 640.0, 0.0, 480.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
}

void putPixel(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
}

// Function to draw all 8 symmetric points
void plotCirclePoints(int xc, int yc, int x, int y) {
    putPixel(xc + x, yc + y);
    putPixel(xc - x, yc + y);
    putPixel(xc + x, yc - y);
    putPixel(xc - x, yc - y);
    putPixel(xc + y, yc + x);
    putPixel(xc - y, yc + x);
    putPixel(xc + y, yc - x);
    putPixel(xc - y, yc - x);
}

// Midpoint Circle Algorithm
void midpointCircle(int xc, int yc, int r) {
    int x = 0;
    int y = r;
    int p = 1 - r;  // Initial decision parameter

    // Plot first set of points
    plotCirclePoints(xc, yc, x, y);

    while (x < y) {
        x++;

        if (p < 0) {
            // Select E (East) pixel
            p = p + 2 * x + 1;
        } else {
            // Select SE (South-East) pixel
            y--;
            p = p + 2 * (x - y) + 1;
        }

        // Plot the calculated points in all octants
        plotCirclePoints(xc, yc, x, y);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 0.0, 0.0);  // Red color for circle
    glPointSize(2.0);

    // Draw circle using midpoint algorithm
    midpointCircle(centerX, centerY, radius);

    glFlush();
    SwapBuffers(GetDC(hwnd));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_PAINT:
            display();
            ValidateRect(hwnd, NULL);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void EnableOpenGL(HWND hwnd, HDC* hDC, HGLRC* hRC) {
    PIXELFORMATDESCRIPTOR pfd;
    int iFormat;

    *hDC = GetDC(hwnd);
    ZeroMemory(&pfd, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;

    iFormat = ChoosePixelFormat(*hDC, &pfd);
    SetPixelFormat(*hDC, iFormat, &pfd);

    *hRC = wglCreateContext(*hDC);
    wglMakeCurrent(*hDC, *hRC);
}

void DisableOpenGL(HWND hwnd, HDC hDC, HGLRC hRC) {
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hRC);
    ReleaseDC(hwnd, hDC);
}

int main(int argc, char** argv) {
    cout << "=== Midpoint Circle Drawing Algorithm ===" << endl;
    cout << "Enter the center coordinates (x y): ";
    cin >> centerX >> centerY;

    cout << "Enter the radius: ";
    cin >> radius;

    cout << "\nDrawing circle with:" << endl;
    cout << "Center: (" << centerX << ", " << centerY << ")" << endl;
    cout << "Radius: " << radius << endl;

    WNDCLASS wc;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    HDC hDC;
    HGLRC hRC;
    MSG msg;

    // Register window class
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("MidpointCircle");
    RegisterClass(&wc);

    // Create window
    hwnd = CreateWindow(TEXT("MidpointCircle"), TEXT("Midpoint Circle Algorithm"),
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                        100, 100, 640, 480,
                        NULL, NULL, hInstance, NULL);

    // Enable OpenGL
    EnableOpenGL(hwnd, &hDC, &hRC);
    init();

    // Display the circle
    display();

    // Message loop
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Disable OpenGL
    DisableOpenGL(hwnd, hDC, hRC);
    DestroyWindow(hwnd);

    return msg.wParam;
}
