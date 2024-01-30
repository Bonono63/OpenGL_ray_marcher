#version 460 core
in vec2 uv;
out vec4 FragColor;

#define SURFACE_DISTANCE 0.01

uniform float TIME;
uniform vec2 RESOLUTION;
uniform sampler3D VOXELS;
uniform float yaw;
uniform float pitch;
uniform float roll;
uniform vec3 camera_forward;
uniform vec3 camera_up;
uniform vec3 camera_right;
uniform vec3 camera_position;
uniform float fov;
uniform float near;
uniform float far;
uniform vec3 light;


vec3 rotate_3d(vec3 position, vec3 axis, float angle)
{
        return mix(dot(axis, position) * axis, position, cos(angle))
                 + cross(axis, position) * sin(angle);
}


mat2 rotate_2d(float angle)
{
        float s = sin(angle);
        float c = cos(angle);
        return mat2(c, -s, s, c);
}


float sdf_sphere(vec3 position, float size)
{
        return length(position) - size;
}


/*float sdf_texture()
{
        return length(position);
}*/


float map(vec3 position)
{
        // TODO add voxel sdf function or something
        float sphere = sdf_sphere(position, 1.0f);
        
        return min(position.y + 0.75, sphere);
}

float ray_march(vec3 ray_origin, vec3 ray_direction)
{
        float total_distance = 0.0f;
        for(int i = 0; i < 200 ; i++)
        {
                vec3 position = ray_origin + ray_direction * total_distance;
        
                float distance = map(position);
        
                total_distance += distance;
        
                // stop the ray steps from getting too small and the total distance from too far
                if (distance < 0.001f || total_distance > far ) break;
        }
        return total_distance;
}

vec3 get_normal(vec3 position)
{
        float distance = map(position);
        vec3 normal = distance - vec3(
                map(position-vec3(0.01f,0.0f,0.0f)),
                map(position-vec3(0.0f,0.01f,0.0f)),
                map(position-vec3(0.0f,0.0f,0.01f))
        );
        return normalize(normal);
}


float get_light(vec3 position)
{
        vec3 _light = normalize(light-position);
        vec3 normal = get_normal(position);
        float diffusion = clamp(dot(normal, _light), 0.0f, 1.0f);
        float distance = ray_march(position+normal*SURFACE_DISTANCE*2.0f, _light);
        if (distance < length(light-position)) diffusion *= 0.1;
        return diffusion;
}


void main()
{
        // Initialization
        vec2 UV = (gl_FragCoord.xy * 2.0 - RESOLUTION.xy) / RESOLUTION.y;

        vec3 ray_origin = camera_position.zyx;
        vec3 ray_direction = normalize(vec3(UV * fov, 1.0));
        // apply pitch
        ray_direction.zy *= rotate_2d(pitch);
        // apply yaw
        ray_direction.xz *= rotate_2d(-yaw);
        vec3 color = vec3(0);

        // Raymarching
        float total_distance = ray_march(ray_origin, ray_direction);
                
        float diffuse_color = get_light(ray_origin + ray_direction * total_distance);

        // DEPTH BUFFER?!!?!??!?
        //color = vec3(total_distance * 0.2f);

        //normal buffer?
        //color = get_normal(ray_origin+ray_direction*total_distance);

        color = vec3(diffuse_color);

        FragColor = vec4(color, 1.0f);
}
