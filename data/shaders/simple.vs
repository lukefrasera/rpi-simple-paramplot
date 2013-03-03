attribute vec4 pos;
attribute vec3 col;
attribute vec3 norm;

uniform mat4 mat_modelview;
uniform mat4 mat_projection;

varying vec3 position;
varying vec3 color;
varying vec3 transf_normal;

void main()
{
    vec4 cam_pos = mat_modelview * pos;
    gl_Position = mat_projection * cam_pos;

    position = cam_pos.xyz;
    color = col;
    transf_normal = mat3(mat_modelview) * norm;
}
