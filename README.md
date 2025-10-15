# ComputerGraphics
# 🌟 Computer Graphics Projects (OpenGL/GLUT) 🎨

A collection of foundational and advanced projects created using **C/C++** and the **OpenGL (GLUT/GLU)** library. This repository explores core concepts in 2D and 3D computer graphics, offering practical implementations of rendering algorithms, geometric transformations, and interactive scene design.

---

## 🎯 Projects Highlights

This repository contains various examples categorized by topic:

### 1. Basic Primitives & Shapes (2D)
| Project Name | Description | Concepts Illustrated |
| :--- | :--- | :--- |
| `triangle.c` | Draws a simple filled red triangle. | `GL_TRIANGLES`, `glColor3f`, `gluOrtho2D`. |
| `square.c` | Draws a filled green square/quadrilateral. | `GL_POLYGON`, Cartesian coordinates (0 to 1). |
| `house_scene.c` | Combines a square and a triangle to draw a simple house. | Primitive combination, Z-ordering/drawing sequence. |
| `points_diagonal.c` | Draws two intersecting lines using consecutive points. | `GL_POINTS`, Iterative vertex generation (loops). |

### 2. Geometric Transformations & Viewing
* **[Example: `transform_rotate.c`]**: Demonstrates rotation of a square around its center using `glRotatef()`.
* **[Example: `view_ortho.c`]**: Detailed example of setting up different 2D viewing windows.
* **[Example: `view_perspective.c`]**: Project illustrating 3D projection and depth perception.

### 3. Interactive Graphics
* **[Example: `mouse_paint.c`]**: Draws points or lines based on mouse clicks.
* **[Example: `keyboard_move.c`]**: Moves an object on the screen using keyboard arrows (`glutSpecialFunc`).

---

## 💻 Technologies Used

| Tool/Library | Role in Project |
| :--- | :--- |
| **C/C++** | Primary programming language for implementation. |
| **OpenGL** | Core API used for hardware-accelerated rendering. |
| **GLUT (The OpenGL Utility Toolkit)** | Handles window management, keyboard/mouse input, and event looping. |
| **GLU (The OpenGL Utility Library)** | Provides helpful routines like matrix setup (`gluOrtho2D`, `gluPerspective`). |

---

## ▶️ Getting Started (How to Run a Project)

### Prerequisites

You need a C/C++ compiler (like GCC or Clang) and the necessary OpenGL, GLU, and GLUT development libraries installed on your system.

### 🚀 Compilation and Execution

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/YourUsername/YourRepoName.git](https://github.com/YourUsername/YourRepoName.git)
    cd YourRepoName
    ```

2.  **Compile the source file:**
    Use your compiler and link explicitly against the OpenGL libraries. Replace `[file_name].c` and `[executable_name]` with the names of the project you want to run.

    ```bash
    # Example for Linux/macOS (using GCC):
    g++ [file_name].c -o [executable_name] -lGL -lGLU -lglut

    # Example for Windows (using MinGW/g++):
    # Ensure you have the GLUT/GLU libs correctly set up in your environment
    g++ [file_name].c -o [executable_name].exe -lopengl32 -lglu32 -lfreeglut
    ```

3.  **Execute the program:**
    ```bash
    ./[executable_name]
    ```

---

## 🤝 Contribution

Feel free to fork this repository, submit issues, or create pull requests. Suggestions for new rendering techniques, optimizations, or additional examples are highly welcome!
