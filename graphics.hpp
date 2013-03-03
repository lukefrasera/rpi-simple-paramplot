#ifndef GRAPHICS_HPP
#define GRAPHICS_HPP

#include <cinttypes>
#include <string>
#include <vector>
#include <bcm_host.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <SDL.h>

class Graphics
{
public:
    Graphics();
    ~Graphics();

    // Calculates normals from given positions and loads both into GPU memory.
    // Around a grid of size res_u*res_v, positions must have a frame of
    // sentinel vertices for the calculation of normals (therefore the positions
    // array has to have (res_u+2)*(res_v+2) entries).
    void load_model(const glm::vec3* positions, const glm::vec3* colors, int res_u, int res_v);

    void render();
    void rotate_cam(float dphi, float dtheta, float droll);
    void move_cam(float dz) { cam_pos_z = glm::clamp(cam_pos_z + dz, 0.f, 100.f); }
    void reload_data() { load_shaders(); }
    void get_screen_size(int& w, int& h) const { w = sdl_screen->w;  h = sdl_screen->h; }
    void set_vsync(bool vsync) { this->vsync = vsync;  eglSwapInterval(egl_display, vsync); }
    bool get_vsync() const { return vsync; }
    void set_culling(bool culling) { if((this->culling = culling)) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE); }
    bool get_culling() const { return culling; }

private:
    void init_egl_context();
    void init_gl();
    void load_shaders();

    static GLuint compile_shader(GLenum type, const std::string& filename);
    static GLuint link_program(const std::vector<GLuint>& shaders);

    int res_u;
    int res_v;
    size_t n_draw_elements;

    SDL_Surface *sdl_screen;
    uint32_t screen_w, screen_h;
    EGLDisplay egl_display;
    EGLSurface egl_surface;
    bool vsync, culling;

    GLuint vao;
    GLuint vbo_model;
    GLuint ibo_model;
    GLuint prog_simple;
    GLuint attr_pos, attr_col, attr_norm;
    GLuint uni_modelmat, uni_projmat;

    glm::quat cam_orient;
    float cam_pos_z;
};

#endif  // GRAPHICS_HPP
