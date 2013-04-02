#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <glm/glm.hpp>
#include <SDL.h>
#include <bcm_host.h>
#include "graphics.hpp"
#include "evaluator.hpp"
#include "exceptions.hpp"
using namespace std;

static const int res_u_def = 64;
static const int res_v_def = 64;

void handle_sdl_error(const char *fname)
{
    cerr << "ERROR: " << fname << ": " << SDL_GetError() << "\n";
    SDL_Quit();
    exit(1);
}

void print_help(const char* progname)
{
    cout << "Usage: " << progname << " [options]\n"
         << "\nOptions:\n"
         << " -e <vardef>\n"
         << "   Define an auxiliary variable which can be used in following occurrences\n"
         << "   of -e, -x, -y and -z.  The definition may use u and v.  The option argument\n"
         << "   must be of the form \"<varchar>=<definition>\" with the '=' sign exactly at\n"
         << "   the second position (no whitespace before it!)\n"
         << " -x <xdef>, -y <ydef>, -z <zdef>\n"
         << "   Specify the parametric function to plot.  u, v and user-defined variables\n"
         << "   may be used.\n"
         << " -r <rdef>, -g <gdef>, -b <bdef>\n"
         << "   Specify the color of the surface.  u, v and user-defined variables\n"
         << "   may be used.\n"
         << " -u <u_res>, -v <v_res>\n"
         << "   Set the number of sampling points along the u and v coordinates.\n"
         << "   Their product must not be greater than 2^16.\n"
         << "   The default is " << res_u_def << ", " << res_v_def << ".\n"
         << "\nExamples:\n"
         << " Sphere:\n"
         << "   " << progname << " -e \"U=2*pi*u\" -e \"V=pi*v\" \\\n"
         << "     -x \"cos(U) * sin(V)\" -z \"sin(U) * sin(V)\" -y \"cos(V)\"\n";
}

// Calculates u,v resolution and all the positions from cmd args and loads the graphics object with them.
void gen_model(int argc, char **argv, Graphics& gfx);

int main(int argc, char **argv)
{
    bcm_host_init();

    // Initialize SDL:
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
        handle_sdl_error("SDL_Init");

    atexit(SDL_Quit);

    try
    {
        Graphics gfx;

        // Parse command-line options and generate model:
        gen_model(argc, argv, gfx);

        bool quitting = false;

        const int framecount_interval = 100;
        int frames = 0;
        Uint32 last_time = SDL_GetTicks();
        float v_phi = 0, v_theta = 0, v_roll = 0, v_z = 0;
        const float v_damp = 0.8;
        while(!quitting)
        {
            gfx.render();

            SDL_Event event;
            while(SDL_PollEvent(&event))
            {
                switch(event.type)
                {
                case SDL_MOUSEMOTION:
                    // Keep mouse centered:
                    {
                        int w, h;
                        gfx.get_screen_size(w, h);
                        if(event.motion.x != w/2 || event.motion.y != h/2)
                        {
                            SDL_WarpMouse(w/2, h/2);
                        }
                        else break;
                    }

                    // Camera rotation:
                    if(event.motion.state & SDL_BUTTON(1))
                    {
                        v_phi += 0.05 * event.motion.xrel;
                        v_theta += 0.05 * event.motion.yrel;
                    }

                    if(event.motion.state & SDL_BUTTON(3))
                    {
                        v_roll += 0.05 * event.motion.xrel;
                    }

                    // Camera movement:
                    if(event.motion.state & SDL_BUTTON(2))
                    {
                        v_z += 0.002 * event.motion.yrel;
                    }
                    break;

                case SDL_KEYDOWN:
                    switch(event.key.keysym.sym)
                    {
                    case SDLK_r:
                        gfx.reload_data();
                        break;

                    case SDLK_F1:
                        cout << "\nUsage:\n"
                             << "  F1:           Display this help.\n"
                             << "  F2:           Toggle VSync.\n"
                             << "  F3:           Toggle backface culling.\n"
                             << "  F4:           Toggle wireframe rendering.\n"
                             << "  Drag LMB:     Rotate camera.\n"
                             << "  Drag LMB+RMB: Roll camera.\n"
                             << "  Drag MMB:     Move camera.\n"
                             << "\n";
                        break;

                    case SDLK_F2:
                        {
                            const bool new_vsync = !gfx.get_vsync();
                            gfx.set_vsync(new_vsync);
                            cout << "VSync turned " << (new_vsync ? "on" : "off") << ".\n";
                        }
                        break;

                    case SDLK_F3:
                        {
                            const bool new_cull = !gfx.get_culling();
                            gfx.set_culling(new_cull);
                            cout << "Culling turned " << (new_cull ? "on" : "off") << ".\n";
                        }
                        break;

                    case SDLK_F4:
                        {
                            const bool new_wire = !gfx.get_wire_mode();
                            gfx.set_wire_mode(new_wire);
                            cout << "Wireframe turned " << (new_wire ? "on" : "off") << ".\n";
                        }
                        break;

                    case SDLK_ESCAPE:
                        quitting = true;
                        break;

                    default:
                        break;
                    }
                    break;

                case SDL_QUIT:
                    quitting = true;
                    break;

                default:
                    break;
                }
            }

            const Uint8 *keystate = SDL_GetKeyState(NULL);
            // Handle camera keys:
            if(keystate[SDLK_RIGHT]) v_phi   += 0.5;
            if(keystate[SDLK_LEFT])  v_phi   -= 0.5;
            if(keystate[SDLK_DOWN])  v_theta += 0.5;
            if(keystate[SDLK_UP])    v_theta -= 0.5;

            // Apply camera movements:
            gfx.rotate_cam(v_phi, v_theta, v_roll);
            gfx.move_cam(v_z);

            // Dampen movements:
            v_phi *= v_damp;
            v_theta *= v_damp;
            v_roll *= v_damp;
            v_z *= v_damp;

            //SDL_Delay(50);

            // Count framerate:
            if(++frames >= framecount_interval)
            {
                const Uint32 this_time = SDL_GetTicks();
                const Uint32 dt = this_time - last_time;
                cout << "* " << (1000.0 * frames / dt) << " FPS\n";
                frames = 0;
                last_time = this_time;
            }
        }
    }
    catch(const exception& e)
    {
        cerr << "ERROR: " << e.what() << "\n";
    }

    SDL_Quit();
    bcm_host_deinit();

    return 0;
}


void gen_model(int argc, char **argv, Graphics& gfx)
{
    int opt;
    extern char *optarg;

    Evaluator::varlist_t varlist;
    varlist.push_back("u");
    varlist.push_back("v");
    vector<double> vars(2);

    Evaluator::constmap_t constmap;
    constmap["pi"] = M_PI;
    constmap["e"] = M_E;

    vector<Evaluator> extra_etors;
    string x_str("2*u-1"), y_str("0"), z_str("2*v-1");
    string r_str("1"), g_str("1"), b_str("1");

    int res_u = res_u_def;
    int res_v = res_u_def;

    glm::vec3 *positions = NULL;
    glm::vec3 *colors    = NULL;

    try 
    {
        while((opt = getopt(argc, argv, "he:u:v:x:y:z:r:g:b:")) != -1)
        {
            switch(opt)
            {
            case 'h':
                print_help(argv[0]);
                exit(0);
                break;

            case 'e':
                {
                    assert(strlen(optarg) >= 3);
                    string varname;
                    varname += optarg[0];
                    varlist.push_back(varname);
                    extra_etors.push_back(Evaluator(optarg + 2, varlist, constmap));
                    vars.push_back(0.0);
                }
                break;

            case 'u':
                res_u = atoi(optarg);
                break;

            case 'v':
                res_v = atoi(optarg);
                break;

            case 'x':
                x_str = optarg;
                break;

            case 'y':
                y_str = optarg;
                break;

            case 'z':
                z_str = optarg;
                break;

            case 'r':
                r_str = optarg;
                break;

            case 'g':
                g_str = optarg;
                break;

            case 'b':
                b_str = optarg;
                break;
            }
        }

        assert(res_u > 1 && res_v > 1 && res_u * res_v <= 256 * 256);

        // Position evaluators:
        Evaluator x_eval(x_str, varlist, constmap),
                  y_eval(y_str, varlist, constmap),
                  z_eval(z_str, varlist, constmap);
        // Color evaluators:
        Evaluator r_eval(r_str, varlist, constmap),
                  g_eval(g_str, varlist, constmap),
                  b_eval(b_str, varlist, constmap);

        // Calculate vertex positions and colors:
        positions = new glm::vec3[(res_u + 2) * (res_v + 2)];
        colors    = new glm::vec3[res_u * res_v];
        for(int j=-1; j<res_v+1; ++j)
        {
            vars[1] = 1.0 * j / (res_v - 1);  // v
            for(int i=-1; i<res_u+1; ++i)
            {
                vars[0] = 1.0 * i / (res_u - 1);  // u

                // Calculate auxiliary variables:
                for(size_t k=0; k<extra_etors.size(); ++k)
                {
                    vars[2 + k] = extra_etors[k].evaluate(vars);
                }

                const int pos_idx = (i+1) + (res_u+2) * (j+1);

                // Evaluate position:
                positions[pos_idx] = glm::vec3(x_eval.evaluate(vars),
                                               y_eval.evaluate(vars),
                                               z_eval.evaluate(vars));

                // Evaluate colors:
                if(i >= 0 && i < res_u && j >= 0 && j < res_v)
                {
                    colors[i + res_u * j] = glm::vec3(r_eval.evaluate(vars),
                                                      g_eval.evaluate(vars),
                                                      b_eval.evaluate(vars));
                }
            }
        }
    }
    catch(const string& e)
    {
        cout << "PARSE ERROR: " << e << "\n";
        exit(1);
    }

    gfx.load_model(positions, colors, res_u, res_v);

    delete[] positions;
    delete[] colors;
}
