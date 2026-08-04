// =============================================================================
// CST-310 · Project 9 (Advanced Shaders 1)
//
// What this shows
// ---------------
//     - A checkerboard ground plane
//     - A sphere with a basic Fresnel-rim shader
//     - A cylinder with a basic procedural bump-mapping shader
//     - A cube with a basic procedural panel-pattern shader
//     - A fractal-tree background prop
//     - A camera driven entirely by discrete keyboard events, matching the
//       Project 9 Resource Guide's control scheme exactly (see key_cb below)
//
//   Each object's shader is a lightweight version of the technique Project
//   10 (Advanced Shaders 2) will implement in full: the sphere's Fresnel rim
//   is a cheap stand-in for real environment/cube-map reflection, the
//   cylinder's procedural ridges are genuine bump mapping (already using the
//   analytic-derivative technique Project 10 would refine further), and the
//   cube's panel pattern hints at surface relief ahead of true parallax
//   (height-map UV displacement) mapping.
// =============================================================================

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

static const int   WW = 1200;
static const int   WH = 800;
static const char* TITLE = "CST-310 · Topic 9 · Checkerboard Scene (Project 9 starter)";

// -----------------------------------------------------------------------------
// Camera state — position plus yaw/pitch/roll (degrees), driven entirely by
// discrete key events (Resource Guide's keyboard scheme), not continuous
// polling or mouse-look. yaw=pitch=roll=0 faces -Z, matching the scene's
// object layout (all objects sit at negative Z ahead of the camera).
// -----------------------------------------------------------------------------
static const glm::vec3 DEFAULT_POS(0.0f, 1.5f, 6.0f);
static const float DEFAULT_YAW = 0.0f, DEFAULT_PITCH = 0.0f, DEFAULT_ROLL = 0.0f;
static glm::vec3 g_pos = DEFAULT_POS;
static float g_yaw = DEFAULT_YAW, g_pitch = DEFAULT_PITCH, g_roll = DEFAULT_ROLL;

// -----------------------------------------------------------------------------
// Mesh helpers — build a checkerboard plane, sphere, cylinder, cube once at
// startup. Each routine returns vertex+index counts via output params and
// fills the supplied vectors.
//
// Vertex layout for all meshes: position(3) + normal(3) + uv(2) = 8 floats.
// -----------------------------------------------------------------------------

static void build_plane(std::vector<float>& v, std::vector<unsigned int>& idx) {
    float size = 8.0f;
    // Four corners of a flat, axis-aligned quad at Y=0; normal points
    // straight up since the whole plane faces the same direction. UV runs
    // 0..8 per edge (not 0..1) so the checker pattern computed in the
    // fragment shader from floor(u)+floor(v) parity repeats 8x8 times
    // instead of drawing as one giant tile.
    v = {
        -size, 0.0f, -size,   0,1,0,  0.0f, 0.0f,
         size, 0.0f, -size,   0,1,0,  8.0f, 0.0f,
         size, 0.0f,  size,   0,1,0,  8.0f, 8.0f,
        -size, 0.0f,  size,   0,1,0,  0.0f, 8.0f
    };
    idx = { 0, 1, 2,  0, 2, 3 }; // two triangles covering the quad
}

static void build_sphere(std::vector<float>& v, std::vector<unsigned int>& idx,
                          int stacks, int slices) {
    v.clear(); idx.clear();
    const float PI = 3.14159265f;
    // Outer loop walks the polar angle phi pole-to-pole (i=0 is the top
    // pole, i=stacks is the bottom pole); inner loop sweeps the azimuthal
    // angle theta once around the equator for each stack.
    for(int i = 0; i <= stacks; ++i){
        float phi = PI * float(i) / stacks;
        float sphi = std::sin(phi), cphi = std::cos(phi);
        for(int j = 0; j <= slices; ++j){
            float theta = 2.0f * PI * float(j) / slices;
            float stheta = std::sin(theta), ctheta = std::cos(theta);
            float x = sphi * ctheta, y = cphi, z = sphi * stheta;
            v.push_back(x); v.push_back(y); v.push_back(z);
            // Normal = position: for a unit sphere centered at the origin
            // the outward normal is always parallel to the radius vector,
            // so no separate cross-product/normal computation is needed.
            v.push_back(x); v.push_back(y); v.push_back(z);
            v.push_back(float(j)/slices); v.push_back(1.0f - float(i)/stacks);
        }
    }
    // Stitch each (i,j) grid quad into two triangles using the four
    // neighboring vertices generated above.
    int cols = slices + 1;
    for(int i = 0; i < stacks; ++i){
        for(int j = 0; j < slices; ++j){
            unsigned int a = i*cols + j, b = (i+1)*cols + j, c = (i+1)*cols + j+1, d = i*cols + j+1;
            idx.push_back(a); idx.push_back(b); idx.push_back(c);
            idx.push_back(a); idx.push_back(c); idx.push_back(d);
        }
    }
}

static void build_cylinder(std::vector<float>& v, std::vector<unsigned int>& idx, int slices) {
    v.clear(); idx.clear();
    const float PI = 3.14159265f;
    for(int j = 0; j <= slices; ++j){
        float theta = 2.0f * PI * float(j) / slices;
        float c = std::cos(theta), s = std::sin(theta);
        // top ring
        v.push_back(c); v.push_back(1.0f); v.push_back(s);
        v.push_back(c); v.push_back(0); v.push_back(s);
        v.push_back(float(j)/slices); v.push_back(1);
        // bottom ring
        v.push_back(c); v.push_back(-1.0f); v.push_back(s);
        v.push_back(c); v.push_back(0); v.push_back(s);
        v.push_back(float(j)/slices); v.push_back(0);
    }
    for(int j = 0; j < slices; ++j){
        unsigned int a = j*2, b = j*2+1, c = (j+1)*2+1, d = (j+1)*2;
        idx.push_back(a); idx.push_back(b); idx.push_back(c);
        idx.push_back(a); idx.push_back(c); idx.push_back(d);
    }
}

static void build_cube(std::vector<float>& v, std::vector<unsigned int>& idx) {
    v.clear(); idx.clear();
    // 24 verts (4 per face), not 8 shared corners, so each face can carry
    // its own constant normal and its own 0-1 UV square. A shared-corner
    // cube would average adjacent faces' normals together at each corner,
    // producing incorrect, smeared lighting on what should be flat faces.
    struct F { glm::vec3 n; glm::vec3 c[4]; };
    F faces[6] = {
        // Each entry: outward face normal, then that face's four +/-1
        // corners wound counter-clockwise as seen from outside the cube.
        { glm::vec3( 0, 0, 1), {glm::vec3(-1,-1,1), glm::vec3(1,-1,1), glm::vec3(1,1,1), glm::vec3(-1,1,1)} },
        { glm::vec3( 0, 0,-1), {glm::vec3(1,-1,-1), glm::vec3(-1,-1,-1), glm::vec3(-1,1,-1), glm::vec3(1,1,-1)} },
        { glm::vec3( 1, 0, 0), {glm::vec3(1,-1,1), glm::vec3(1,-1,-1), glm::vec3(1,1,-1), glm::vec3(1,1,1)} },
        { glm::vec3(-1, 0, 0), {glm::vec3(-1,-1,-1), glm::vec3(-1,-1,1), glm::vec3(-1,1,1), glm::vec3(-1,1,-1)} },
        { glm::vec3( 0, 1, 0), {glm::vec3(-1,1,1), glm::vec3(1,1,1), glm::vec3(1,1,-1), glm::vec3(-1,1,-1)} },
        { glm::vec3( 0,-1, 0), {glm::vec3(-1,-1,-1), glm::vec3(1,-1,-1), glm::vec3(1,-1,1), glm::vec3(-1,-1,1)} },
    };
    float uv[8] = {0,0, 1,0, 1,1, 0,1}; // same 0-1 square reused on every face
    for(int f = 0; f < 6; ++f) {
        for(int i = 0; i < 4; ++i) {
            v.push_back(faces[f].c[i].x); v.push_back(faces[f].c[i].y); v.push_back(faces[f].c[i].z);
            v.push_back(faces[f].n.x);    v.push_back(faces[f].n.y);    v.push_back(faces[f].n.z);
            v.push_back(uv[i*2]); v.push_back(uv[i*2 + 1]);
        }
        // Two triangles per face, indexed relative to this face's own
        // 4-vertex block (b = first vertex of face f).
        unsigned int b = f * 4;
        idx.push_back(b); idx.push_back(b+1); idx.push_back(b+2);
        idx.push_back(b); idx.push_back(b+2); idx.push_back(b+3);
    }
}

// -----------------------------------------------------------------------------
// Background prop: a recursive fractal tree (ported from topic_09_fractal_tree),
// generated once at startup as a static GL_LINES prop rather than rebuilt every
// frame — it's set dressing here, not the interactive subject.
// -----------------------------------------------------------------------------
static void grow_tree(std::vector<float>& v, glm::vec3 base, glm::vec3 dir, float len,
                       int depth, int branches, float angleDeg, float shrink) {
    glm::vec3 tip = base + dir * len;
    // position(3) + normal(3, unused for lines — reuse as a dummy) + uv(2, unused)
    v.push_back(base.x); v.push_back(base.y); v.push_back(base.z);
    v.push_back(0); v.push_back(1); v.push_back(0);
    v.push_back(0); v.push_back(0);
    v.push_back(tip.x); v.push_back(tip.y); v.push_back(tip.z);
    v.push_back(0); v.push_back(1); v.push_back(0);
    v.push_back(0); v.push_back(0);

    if (depth <= 0) return;

    glm::vec3 up(0, 1, 0);
    if (std::abs(glm::dot(up, dir)) > 0.95f) up = glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(dir, up));
    up = glm::cross(right, dir);

    float angleRad = angleDeg * 3.14159265f / 180.0f;
    for (int i = 0; i < branches; ++i) {
        float t = 2.0f * 3.14159265f * i / branches;
        glm::vec3 sideways = std::cos(t) * right + std::sin(t) * up;
        glm::vec3 newDir = glm::normalize(std::cos(angleRad) * dir + std::sin(angleRad) * sideways);
        grow_tree(v, tip, newDir, len * shrink, depth - 1, branches, angleDeg, shrink);
    }
}

struct Mesh { GLuint vao, vbo, ebo; GLsizei n; };

static Mesh upload(std::vector<float>& v, std::vector<unsigned int>& idx) {
    Mesh m;
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    m.n = static_cast<GLsizei>(idx.size());
    return m;
}

// Same 8-float layout as `upload()`, but no index buffer and drawn with
// GL_LINES instead of GL_TRIANGLES.
static Mesh upload_lines(std::vector<float>& v) {
    Mesh m; m.ebo = 0;
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);
    m.n = static_cast<GLsizei>(v.size() / 8);
    return m;
}

static std::string read_file(const std::string& p){ std::ifstream f(p); std::stringstream s; s<<f.rdbuf(); return s.str(); }
static GLuint compile(GLenum k,const std::string& src){ GLuint id=glCreateShader(k); const char* c=src.c_str(); glShaderSource(id,1,&c,nullptr); glCompileShader(id); GLint ok; glGetShaderiv(id,GL_COMPILE_STATUS,&ok); if(!ok){char L[2048]; glGetShaderInfoLog(id,2048,nullptr,L); std::cerr<<L<<"\n"; return 0;} return id; }
static GLuint link_prog(GLuint a,GLuint b){ GLuint p=glCreateProgram(); glAttachShader(p,a); glAttachShader(p,b); glLinkProgram(p); GLint ok; glGetProgramiv(p,GL_LINK_STATUS,&ok); if(!ok){char L[2048]; glGetProgramInfoLog(p,2048,nullptr,L); std::cerr<<L<<"\n"; return 0;} return p; }
static void fb_cb(GLFWwindow*,int w,int h){ glViewport(0,0,w,h); }

// -----------------------------------------------------------------------------
// Camera control — implements the Project 9 Resource Guide's keyboard table
// exactly. Arrow keys mean three different things depending on the modifier
// held (plain = X/Y slide, Shift = Z slide, Ctrl = pitch/yaw), so the plain
// arrow keys and their modifiers have to be disambiguated in one callback
// rather than polled independently. GLFW_REPEAT is handled the same as
// GLFW_PRESS so holding a key keeps applying the same discrete step via the
// OS's own key-repeat timing, without needing a separate per-frame poll.
// -----------------------------------------------------------------------------
static void key_cb(GLFWwindow* w, int key, int, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    bool shift = (mods & GLFW_MOD_SHIFT) != 0;
    bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;

    if (key == GLFW_KEY_ESCAPE) { glfwSetWindowShouldClose(w, GLFW_TRUE); return; }
    if (key == GLFW_KEY_R) { g_pos = DEFAULT_POS; g_yaw = DEFAULT_YAW; g_pitch = DEFAULT_PITCH; g_roll = DEFAULT_ROLL; return; }
    if (key == GLFW_KEY_COMMA)  { g_roll += 2.0f; return; } // '<' — change camera roll by 2 degrees
    if (key == GLFW_KEY_PERIOD) { g_roll -= 2.0f; return; } // '>' — change camera roll by -2 degrees

    if (key == GLFW_KEY_RIGHT) {
        if (ctrl) g_yaw += 2.0f;              // Control Right Arrow: yaw += 2 deg
        else if (!shift) g_pos.x += 1.0f;     // Right Arrow: slide +X
        return;
    }
    if (key == GLFW_KEY_LEFT) {
        if (ctrl) g_yaw -= 2.0f;              // Control Left Arrow: yaw -= 2 deg
        else if (!shift) g_pos.x -= 1.0f;     // Left Arrow: slide -X
        return;
    }
    if (key == GLFW_KEY_UP) {
        if (ctrl) g_pitch -= 2.0f;            // Control Up Arrow: pitch -= 2 deg
        else if (shift) g_pos.z += 1.0f;      // Shift Up Arrow: slide +Z ("in")
        else g_pos.y += 1.0f;                 // Up Arrow: slide +Y
        return;
    }
    if (key == GLFW_KEY_DOWN) {
        if (ctrl) g_pitch += 2.0f;            // Control Down Arrow: pitch += 2 deg
        else if (shift) g_pos.z -= 1.0f;      // Shift Down Arrow: slide -Z ("out")
        else g_pos.y -= 1.0f;                 // Down Arrow: slide -Y
        return;
    }
}

int main(){
    if(!glfwInit()){ return 1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* win = glfwCreateWindow(WW, WH, TITLE, nullptr, nullptr);
    if(!win){ glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, fb_cb);
    glfwSetKeyCallback(win, key_cb);
    glfwSwapInterval(1);

    if(!gladLoadGL((GLADloadfunc)glfwGetProcAddress)){ return 1; }
    std::cout<<"OpenGL "<<glGetString(GL_VERSION)<<"\nGPU:    "<<glGetString(GL_RENDERER)<<"\n";
    std::cout<<"Arrows=slide X/Y  Shift+Up/Down=slide Z  Ctrl+Arrows=pitch/yaw  ,/.=roll  r=reset  Esc=close\n";

    int fbw,fbh; glfwGetFramebufferSize(win,&fbw,&fbh); glViewport(0,0,fbw,fbh);
    glEnable(GL_DEPTH_TEST);

    auto vs = read_file("shaders/scene.vert");
    auto fs = read_file("shaders/scene.frag");
    GLuint v = compile(GL_VERTEX_SHADER, vs);
    GLuint f = compile(GL_FRAGMENT_SHADER, fs);
    GLuint prog = link_prog(v, f);
    glDeleteShader(v); glDeleteShader(f);
    if(!prog){ glfwTerminate(); return 1; }

    GLint loc_mvp  = glGetUniformLocation(prog, "uMVP");
    GLint loc_m    = glGetUniformLocation(prog, "uModel");
    GLint loc_nm   = glGetUniformLocation(prog, "uNormalMatrix");
    GLint loc_ld   = glGetUniformLocation(prog, "uLightDir");
    GLint loc_eye  = glGetUniformLocation(prog, "uEyePos");
    GLint loc_col  = glGetUniformLocation(prog, "uColor");
    GLint loc_mode = glGetUniformLocation(prog, "uShaderMode");

    std::vector<float> v_buf; std::vector<unsigned int> i_buf;
    build_plane(v_buf, i_buf);     Mesh plane    = upload(v_buf, i_buf);
    build_sphere(v_buf, i_buf, 24, 36);  Mesh sphere   = upload(v_buf, i_buf);
    build_cylinder(v_buf, i_buf, 32);    Mesh cylinder = upload(v_buf, i_buf);
    build_cube(v_buf, i_buf);            Mesh cube     = upload(v_buf, i_buf);

    std::vector<float> tree_buf;
    grow_tree(tree_buf, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 1.2f, 5, 3, 28.0f, 0.72f);
    Mesh tree = upload_lines(tree_buf);

    while(!glfwWindowShouldClose(win)){
        // Camera orientation: build a rotation matrix from yaw (world Y),
        // pitch (local X), and roll (local Z) — applied in that order so
        // roll tilts the already-yawed-and-pitched view, not the world.
        // forward/up are that rotation applied to the default -Z/+Y axes;
        // passing the *rotated* up (not world up) into lookAt is what makes
        // roll visible, since lookAt's up vector defines the view's "top".
        glm::mat4 R = glm::mat4(1.0f);
        R = glm::rotate(R, glm::radians(g_yaw),   glm::vec3(0, 1, 0));
        R = glm::rotate(R, glm::radians(g_pitch), glm::vec3(1, 0, 0));
        R = glm::rotate(R, glm::radians(g_roll),  glm::vec3(0, 0, 1));
        glm::vec3 fwd = glm::normalize(glm::vec3(R * glm::vec4(0, 0, -1, 0)));
        glm::vec3 up  = glm::normalize(glm::vec3(R * glm::vec4(0, 1,  0, 0)));

        glm::mat4 view = glm::lookAt(g_pos, g_pos + fwd, up);
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), static_cast<float>(fbw)/fbh, 0.1f, 100.0f);

        glClearColor(0.5f, 0.62f, 0.78f, 1.0f);   // sky-ish
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);
        glUniform3f(loc_ld, 0.4f, 0.85f, 0.5f);
        glUniform3fv(loc_eye, 1, glm::value_ptr(g_pos));

        auto draw = [&](const Mesh& m, const glm::mat4& model, const glm::vec3& color, int shaderMode) {
            glm::mat3 nm = glm::transpose(glm::inverse(glm::mat3(model)));
            glm::mat4 mvp = proj * view * model;
            glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(loc_m, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix3fv(loc_nm, 1, GL_FALSE, glm::value_ptr(nm));
            glUniform3fv(loc_col, 1, glm::value_ptr(color));
            glUniform1i(loc_mode, shaderMode);
            glBindVertexArray(m.vao);
            glDrawElements(GL_TRIANGLES, m.n, GL_UNSIGNED_INT, 0);
        };

        // ground plane (checkerboard, mode 0)
        draw(plane,    glm::mat4(1.0f), glm::vec3(0.9f), 0);

        // Composition: cylinder sits directly on the camera's initial straight-
        // ahead path (x=0); the sphere is staged nearer and to the left, the
        // cube farther back, off to the right, and rotated so it doesn't read
        // as an axis-aligned box dropped in place. The fractal tree anchors the
        // background well behind all three, giving the flythrough a sense of
        // depth instead of one row of objects.

        // sphere — near, left — mode 1: basic Fresnel-rim shader
        draw(sphere,   glm::translate(glm::mat4(1.0f), glm::vec3(-2.2f, 1.0f, 0.5f)), glm::vec3(0.88f, 0.72f, 0.36f), 1);

        // cylinder — centered on the camera's initial path — mode 2: basic bump mapping
        glm::mat4 cylM = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, -2.0f));
        cylM = glm::scale(cylM, glm::vec3(0.6f, 1.0f, 0.6f));
        draw(cylinder, cylM, glm::vec3(0.40f, 0.78f, 0.55f), 2);

        // cube — far, right, rotated for visual interest — mode 3: basic panel pattern
        glm::mat4 cubeM = glm::translate(glm::mat4(1.0f), glm::vec3(2.6f, 0.9f, -3.5f));
        cubeM = glm::rotate(cubeM, glm::radians(25.0f), glm::vec3(0, 1, 0));
        cubeM = glm::scale(cubeM, glm::vec3(0.9f));
        draw(cube,     cubeM, glm::vec3(0.40f, 0.55f, 0.85f), 3);

        // fractal tree — background prop, further back and slightly off-axis
        // so it doesn't block the cylinder from the camera's starting view
        glm::mat4 treeM = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.0f, -6.5f));
        glUniformMatrix4fv(loc_mvp, 1, GL_FALSE, glm::value_ptr(proj * view * treeM));
        glUniformMatrix4fv(loc_m, 1, GL_FALSE, glm::value_ptr(treeM));
        glm::mat3 treeNm = glm::transpose(glm::inverse(glm::mat3(treeM)));
        glUniformMatrix3fv(loc_nm, 1, GL_FALSE, glm::value_ptr(treeNm));
        glUniform3f(loc_col, 0.36f, 0.27f, 0.16f);
        glUniform1i(loc_mode, 0);
        glBindVertexArray(tree.vao);
        glLineWidth(1.5f);
        glDrawArrays(GL_LINES, 0, tree.n);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glDeleteProgram(prog);
    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
