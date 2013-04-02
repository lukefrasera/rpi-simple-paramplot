#include <cassert>
#include <cinttypes>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "graphics.hpp"
#include "exceptions.hpp"
using namespace std;
using namespace glm;

Graphics::Graphics()
  : vsync(true), culling(false), wire_mode(false),
    cam_orient(quat(vec3(0.f, 0.f, 0.f))),
    cam_pos_z(7)
{
    sdl_screen = SDL_SetVideoMode(0, 0, 0, SDL_SWSURFACE | SDL_FULLSCREEN);
    if(!sdl_screen) throw SDLException("SDL_SetVideoMode");

    init_egl_context();
    init_gl();
    load_shaders();
}

Graphics::~Graphics()
{
}

void Graphics::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Create modelview matrix:
    glm::mat4 modelview(1);
    modelview = translate(modelview, vec3(0, 0, -cam_pos_z));
    modelview = modelview * mat4_cast(cam_orient);

    // Draw model:
    glUseProgram(prog_shiny);
    glUniformMatrix4fv(uni_shiny_modelmat, 1, GL_FALSE, value_ptr(modelview));
    glEnableVertexAttribArray(attr_shiny_pos);
    glEnableVertexAttribArray(attr_shiny_col);
    glEnableVertexAttribArray(attr_shiny_norm);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_model);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_model);

    if(wire_mode)
    {
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.0f, 1.0f);
    }

    glVertexAttribPointer(attr_shiny_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(attr_shiny_col, 3, GL_FLOAT, GL_FALSE, 0, (void *)(sizeof(float) * 3 * res_u * res_v));
    glVertexAttribPointer(attr_shiny_norm, 3, GL_FLOAT, GL_FALSE, 0, (void *)(sizeof(float) * 6 * res_u * res_v));
    glDrawElements(GL_TRIANGLE_STRIP, n_draw_elements, GL_UNSIGNED_SHORT, 0);

    if(wire_mode)
    {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(attr_shiny_pos);
    glDisableVertexAttribArray(attr_shiny_col);
    glDisableVertexAttribArray(attr_shiny_norm);
    glUseProgram(0);
    
    // Draw wireframe:
    if(wire_mode)
    {
        glUseProgram(prog_simple);
        glUniformMatrix4fv(uni_simple_modelmat, 1, GL_FALSE, value_ptr(modelview));
        glEnableVertexAttribArray(attr_simple_pos);
        glEnableVertexAttribArray(attr_simple_col);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_model);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_model_wire);

        glVertexAttribPointer(attr_simple_pos, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glVertexAttribPointer(attr_simple_col, 3, GL_FLOAT, GL_FALSE, 0, (void *)(sizeof(float) * 3 * res_u * res_v));
        glDrawElements(GL_LINES, n_draw_wire_elements, GL_UNSIGNED_SHORT, 0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(attr_simple_pos);
        glDisableVertexAttribArray(attr_simple_col);
        glUseProgram(0);
    }

    eglSwapBuffers(egl_display, egl_surface);
}

void Graphics::init_gl()
{
    glViewport(0, 0, screen_w, screen_h);

    glClearColor(0, 0, 0, 1);

    glClearDepthf(1);
    glEnable(GL_DEPTH_TEST);
    glDepthRangef(0, 1);
    glDepthFunc(GL_LESS);

    glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
}

void Graphics::load_shaders()
{
    // If the shader program is already loaded, delete it first:
    if(prog_simple != 0) glDeleteProgram(prog_simple);
    if(prog_shiny != 0) glDeleteProgram(prog_shiny);

    // Simple shader program:
    // Compile shaders:
    vector<GLuint> shaders;
    shaders.push_back(compile_shader(GL_VERTEX_SHADER, "data/shaders/simple.vs"));
    shaders.push_back(compile_shader(GL_FRAGMENT_SHADER, "data/shaders/simple.fs"));

    // Link program:
    prog_simple = link_program(shaders);

    // Delete shaders again:
    for_each(shaders.begin(), shaders.end(), glDeleteShader);

    // Get locations:
    attr_simple_pos  = glGetAttribLocation(prog_simple, "pos");
    attr_simple_col  = glGetAttribLocation(prog_simple, "col");
    uni_simple_modelmat = glGetUniformLocation(prog_simple, "mat_modelview");
    uni_simple_projmat  = glGetUniformLocation(prog_simple, "mat_projection");

    // Shiny shader program:
    // Compile shaders:
    shaders.clear();
    shaders.push_back(compile_shader(GL_VERTEX_SHADER, "data/shaders/shiny.vs"));
    shaders.push_back(compile_shader(GL_FRAGMENT_SHADER, "data/shaders/shiny.fs"));

    // Link program:
    prog_shiny = link_program(shaders);

    // Delete shaders again:
    for_each(shaders.begin(), shaders.end(), glDeleteShader);

    // Get locations:
    attr_shiny_pos  = glGetAttribLocation(prog_shiny, "pos");
    attr_shiny_col  = glGetAttribLocation(prog_shiny, "col");
    attr_shiny_norm = glGetAttribLocation(prog_shiny, "norm");
    uni_shiny_modelmat = glGetUniformLocation(prog_shiny, "mat_modelview");
    uni_shiny_projmat  = glGetUniformLocation(prog_shiny, "mat_projection");

    // Set constant uniforms:
    float ratio = 1.f * screen_w / screen_h;
    mat4 projection = perspective(45.f, ratio, .1f, 100.f);
    glUseProgram(prog_simple);
    glUniformMatrix4fv(uni_simple_projmat, 1, GL_FALSE, value_ptr(projection));
    glUseProgram(prog_shiny);
    glUniformMatrix4fv(uni_shiny_projmat, 1, GL_FALSE, value_ptr(projection));
    glUseProgram(0);
}

void Graphics::load_model(const glm::vec3* positions, const glm::vec3* colors, int res_u, int res_v)
{
    this->res_u = res_u;
    this->res_v = res_v;
    n_draw_elements = (res_v - 1) * 2 * res_u + (res_v - 2) * 2;
    n_draw_wire_elements = (res_v - 1) * 2 * res_u + (res_u - 1) * 2 * res_v;

    const size_t n_verts = res_u * res_v;

    // Calculate normals and fill buffer:
    float *verts = new float[3 * 3 * n_verts];
    for(int j=0; j<res_v; ++j)
    {
        for(int i=0; i<res_u; ++i)
        {
            const int pos_idx = (i + 1) + (res_u + 2) * (j + 1);
            const int col_idx = i + res_u * j;
            const int vert_idx = 3 * (i + res_u * j);

            // Approximate normal at current vertex with 4 neighboring vertices.
            // The normal is the cross product between the relative vectors:
            //
            //  .   .   .   .   .
            //  .   .  v_n  .   .
            //  .  v_w (*) v_e  .  ==> normal at (*): (v_e - v_w) x (v_n - v_s)
            //  .   .  v_s  .   .
            //  .   .   .   .   .
            //
            // If vertices on the grid fall together to make triangles (like at
            // the caps of a simple sphere), one of these relative vectors can
            // be zero which is bad.
            // If this occurs, these vertices have to be avoided, for example
            // like this:
            //
            //  .   .   .   .   .
            //  .   .  v_n  .   .
            //         (*)         <-- here a row of vertices is in one point
            //  .  v_w v_s v_e  .
            //  .   .   .   .   .

            vec3 v_w = positions[pos_idx - 1];
            vec3 v_e = positions[pos_idx + 1];
            vec3 v_s = positions[pos_idx + (res_u + 2)];
            vec3 v_n = positions[pos_idx - (res_u + 2)];

            const float tol = 1e-15;

            // Watch out for the "triangles":
            if(length(v_e - v_w) < tol)
            {
                v_w = positions[pos_idx - (res_u + 2) - 1];
                v_e = positions[pos_idx - (res_u + 2) + 1];

                if(j == 0) swap(v_w, v_e);  // this is a very hacky fix for simple spheres

                //cerr << "INFO: WE correction #1 at i=" << i << ", j=" << j << "\n";

                if(length(v_e - v_w) < tol)
                {
                    v_w = positions[pos_idx + (res_u + 2) - 1];
                    v_e = positions[pos_idx + (res_u + 2) + 1];

                    if(j == res_v - 1) swap(v_w, v_e);  // as above

                    //cerr << "INFO: WE correction #2 at i=" << i << ", j=" << j << "\n";
                }
            }

            if(length(v_n - v_s) < tol)
            {
                v_s = positions[pos_idx + (res_u + 2) - 1];
                v_n = positions[pos_idx - (res_u + 2) - 1];

                if(i == 0) swap(v_s, v_n);

                //cerr << "INFO: NS correction #1 at i=" << i << ", j=" << j << "\n";

                if(length(v_n - v_s) < tol)
                {
                    v_s = positions[pos_idx + (res_u + 2) + 1];
                    v_n = positions[pos_idx - (res_u + 2) + 1];

                    if(i == res_u - 1) swap(v_s, v_n);

                    //cerr << "INFO: NS correction #2 at i=" << i << ", j=" << j << "\n";
                }
            }

            // Do the cross product:
            vec3 norm = cross(v_e - v_w, v_n - v_s);
            const float norm_len = length(norm);

            if(norm_len < tol)
            {
                // Just give up at this point:
                norm = vec3(0, 0, 0);
                cerr << "WARNING: Null normal at i=" << i << ", j=" << j << "\n";
            }
            else
            {
                // Normal-ize!
                norm /= norm_len;
            }

            // Fill in GPU buffer data:
            verts[vert_idx + 0] = positions[pos_idx].x;
            verts[vert_idx + 1] = positions[pos_idx].y;
            verts[vert_idx + 2] = positions[pos_idx].z;

            verts[3 * n_verts + vert_idx + 0] = colors[col_idx].x;
            verts[3 * n_verts + vert_idx + 1] = colors[col_idx].y;
            verts[3 * n_verts + vert_idx + 2] = colors[col_idx].z;

            verts[6 * n_verts + vert_idx + 0] = norm.x;
            verts[6 * n_verts + vert_idx + 1] = norm.y;
            verts[6 * n_verts + vert_idx + 2] = norm.z;
        }
    }

    glGenBuffers(1, &vbo_model);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_model);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 3 * n_verts, verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    delete[] verts;

    uint16_t *elems;

    // Calculate element indices for filled model:
    elems = new uint16_t[n_draw_elements];
    {
        size_t idx = 0;
        for(int j=0; j < res_v - 1; ++j)
        {
            if(j > 0)
            {
                // Generate degenerate triangles:
                elems[idx ++] = res_u * (j + 1) - 1;
                elems[idx ++] = res_u * j;
            }

            for(int i=0; i<res_u; ++i)
            {
                elems[idx ++] = i + res_u * j;
                elems[idx ++] = i + res_u * (j + 1);
            }
        }
        assert(idx == n_draw_elements);
    }

    glGenBuffers(1, &ibo_model);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_model);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * n_draw_elements, elems, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    delete[] elems;

    // Calculate element indices for model wireframe:
    elems = new uint16_t[n_draw_wire_elements];
    {
        size_t idx = 0;
        for(int j=0; j < res_v; ++j)
        {
            for(int i=0; i < res_u - 1; ++i)
            {
                elems[idx ++] = i + res_u * j;
                elems[idx ++] = i + 1 + res_u * j;
            }
        }
        for(int i=0; i < res_u; ++i)
        {
            for(int j=0; j < res_v - 1; ++j)
            {
                elems[idx ++] = i + res_u * j;
                elems[idx ++] = i + res_u * (j + 1);
            }
        }
        assert(idx == n_draw_wire_elements);
    }

    glGenBuffers(1, &ibo_model_wire);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_model_wire);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * n_draw_wire_elements, elems, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    delete[] elems;
}

void Graphics::rotate_cam(float dphi, float dtheta, float droll)
{
    dphi *= M_PI / 180.f;
    dtheta *= M_PI / 180.f;
    droll *= M_PI / 180.f;

    cam_orient = normalize(cross(quat(vec3(dtheta, dphi, -droll)), cam_orient));
}

void Graphics::init_egl_context()
{
    // Get an EGL display connection:
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(egl_display == EGL_NO_DISPLAY)
        throw EGLException("eglGetDisplay");
 
    // Initialize the EGL display connection:
    if(eglInitialize(egl_display, NULL, NULL) == EGL_FALSE)
        throw EGLException("eglInitialize");
 
    // Get an appropriate EGL frame buffer configuration:
    EGLConfig config;
    {
        EGLint num_config;
        static const EGLint attribute_list[] =
        {
           EGL_RED_SIZE, 8,
           EGL_GREEN_SIZE, 8,
           EGL_BLUE_SIZE, 8,
           EGL_ALPHA_SIZE, 8,
           EGL_DEPTH_SIZE, 8,
           EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
           EGL_NONE
        };
 
        if(eglChooseConfig(egl_display, attribute_list, &config, 1, &num_config) == EGL_FALSE)
            throw EGLException("eglChooseConfig");
    }
 
    // Get an appropriate EGL frame buffer configuration:
    if(eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE)
        throw EGLException("eglBindAPI");
 
    // Create an EGL rendering context:
    EGLContext context;
    {
        static const EGLint context_attributes[] = 
        {
           EGL_CONTEXT_CLIENT_VERSION, 2,
           EGL_NONE
        };
 
        context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attributes);
        if(context == EGL_NO_CONTEXT)
            throw EGLException("eglCreateContext");
    }
 
    // Create an EGL window surface:
    if(graphics_get_display_size(0 /* LCD */, &screen_w, &screen_h) < 0)
        throw EGLException("graphics_get_display_size");
 
    {
        static EGL_DISPMANX_WINDOW_T nativewindow;
 
        DISPMANX_ELEMENT_HANDLE_T dispman_element;
        DISPMANX_DISPLAY_HANDLE_T dispman_display;
        DISPMANX_UPDATE_HANDLE_T dispman_update;
        VC_RECT_T dst_rect;
        VC_RECT_T src_rect;
 
        dst_rect.x = 0;
        dst_rect.y = 0;
        dst_rect.width = screen_w;
        dst_rect.height = screen_h;
           
        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = screen_w << 16;
        src_rect.height = screen_h << 16;        
 
        dispman_display = vc_dispmanx_display_open(0 /* LCD */);
        dispman_update = vc_dispmanx_update_start(0);
              
        dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0 /*layer*/, &dst_rect, 0 /*src*/,
          &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0 /*clamp*/, DISPMANX_NO_ROTATE /*transform*/);
           
        nativewindow.element = dispman_element;
        nativewindow.width = screen_w;
        nativewindow.height = screen_h;
        vc_dispmanx_update_submit_sync( dispman_update );
           
        egl_surface = eglCreateWindowSurface(egl_display, config, &nativewindow, NULL);
        if(egl_surface == EGL_NO_SURFACE)
            throw EGLException("eglCreateWindowSurface (check your RAM split)");
    }
 
    // Connect the context to the surface:
    if(eglMakeCurrent(egl_display, egl_surface, egl_surface, context) == EGL_FALSE)
        throw EGLException("eglMakeCurrent");
 
    // Extra EGL settings:
    if(eglSwapInterval(egl_display, vsync) == EGL_FALSE)
        throw EGLException("eglSwapInterval");
}

GLuint Graphics::compile_shader(GLenum type, const std::string& filename)
{
    char *shader_text = NULL;
    int shader_length = 0;

    // Read file contents:
    //   Open file:
    ifstream file_stream(filename.c_str());

    //   Get file length:
    file_stream.seekg(0, ios::end);
    shader_length = file_stream.tellg();
    file_stream.seekg(0, ios::beg);

    //   Read into buffer:
    shader_text = new char[shader_length];
    file_stream.read(shader_text, shader_length);

    //    Close file:
    file_stream.close();


    // Create OpenGL shader:
    const GLuint handle = glCreateShader(type);
    const char *const_shader_text = shader_text;
    glShaderSource(handle, 1, &const_shader_text, &shader_length);
    delete[] shader_text;
    glCompileShader(handle);

    // Check for errors:
    GLint status;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        // Get info log:
        GLint log_length;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        GLchar *log_text = new GLchar[log_length];
        glGetShaderInfoLog(handle, log_length, NULL, log_text);

        // Print log:
        cerr << "Compilation of \"" << filename << "\" failed:\n" << log_text << "\n";
        delete[] log_text;

        throw GLException("shader compilation failed");
    }

    return handle;
}

GLuint Graphics::link_program(const vector<GLuint>& shaders)
{
    typedef vector<GLuint>::const_iterator IT;

    // Create OpenGL program:
    const GLuint handle = glCreateProgram();

    // Attach the shaders:
    for(IT it = shaders.begin(); it != shaders.end(); ++it)
        glAttachShader(handle, *it);

    // Link:
    glLinkProgram(handle);

    // Detach the shaders:
    for(IT it = shaders.begin(); it != shaders.end(); ++it)
        glDetachShader(handle, *it);

    // Check for errors:
    GLint status;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    if(status == GL_FALSE)
    {
        // Get info log:
        GLint log_length;
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        GLchar *log_text = new GLchar[log_length];
        glGetProgramInfoLog(handle, log_length, NULL, log_text);

        // Print log:
        cerr << "Linking of shader program failed:\n" << log_text << "\n";
        delete[] log_text;

        throw GLException("program linking failed");
    }

    return handle;
}
