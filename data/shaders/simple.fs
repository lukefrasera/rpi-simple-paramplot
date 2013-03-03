varying vec3 position;
varying vec3 color;
varying vec3 transf_normal;

void main()
{
    vec3 normal = normalize(transf_normal);
    const vec3 light_pos = vec3(0, 0, 0);
    //const vec3 spec_col = vec3(1, 1, 1);
    vec3 to_light = light_pos - position;
    float dist_to_light = length(to_light);
    vec3 to_light_norm = to_light / dist_to_light;
    vec3 half_vec = normalize(normalize(-position) + to_light_norm);

    vec3 end_color = color;
    // Ambient, diffuse and specular:
    end_color *= 0.01 + 0.3 * abs(dot(normal, to_light_norm))
                      + 0.7 * pow(abs(dot(normal, half_vec)), 10.0);
    //end_color += 0.5 * spec_col * pow(abs(dot(normal, half_vec)), 10.0);

    // Attenuation:
    //end_col *= 1.0 / (1.0 + 0.005 * dist_to_light * dist_to_light);

    gl_FragColor = vec4(end_color, 1);
}
