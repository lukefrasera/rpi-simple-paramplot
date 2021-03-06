attribute vec4 pos;
attribute vec3 col;

uniform mat4 mat_modelview;
uniform mat4 mat_projection;

varying vec3 color;

void main()
{
    vec4 cam_pos = mat_modelview * pos;
    gl_Position = mat_projection * cam_pos;

    color = col;
}
