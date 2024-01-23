#include <stdio.h>
#include <stdlib.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string.h>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/gtc/type_ptr.hpp"

#include <fstream>
#include <filesystem>

typedef struct Camera
{
        const float fov = 70.0f;
        float speed = 0.25f;
        const float sensitivity = 0.05f;
        float yaw=90.f,pitch;
        glm::vec3 position = glm::vec3(0.0f,0.0f,-1.0f);
        glm::vec3 direction;
        glm::vec3 front;
        glm::vec3 up;
        glm::vec3 right;
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection;
}Camera;


// free out!
int read_file(const char * path, char** out)
{
        std::ifstream file (path);
        if (file.is_open())
        {
                long long file_size = std::filesystem::file_size(path);
                
                *out = (char*) malloc((file_size+1)*sizeof(char));
                for (long long i = 0 ; i < file_size; i++)
                {
                        char c;
                        file.get(c);
                        *(*out+i) = c;
                }
                // terminating byte because final character isn't one?
                *(*out+file_size) = '\0';
                file.close();
        }
        else
        {
                printf("Unable to read file at: %s\n",path);
                file.close();
                return -1;
        }
        return 0;
}


void set_shader_value_float(const char * loc, float value, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                return;//printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform1f(location, value);
}


void set_shader_value_vec2(const char * loc, glm::vec2 value, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                return;//printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform2f(location, value.x, value.y);
}


void set_shader_value_float_array(const char * loc, float* value, int size, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                return;//printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniform1fv(location,size,value);
}


void set_shader_value_matrix4(const char * loc, glm::mat4 value, unsigned int shader_program)
{
        int location = glGetUniformLocation(shader_program, loc);
        if (location == -1)
                return;//printf("Unable to locate uniform %s in shader %d\n",loc,shader_program);
        else
                glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}


unsigned int load_shader(const char* vertex_shaderPath, const char* fragment_shaderPath)
{
        // VERTEX
        char * vertex_source;
        int vertex_file = read_file(vertex_shaderPath, &vertex_source);
        if (vertex_file != 0)
        {
                printf("unable to compile shader. vertex shader couldn't be found.\n");
                return -1;
        }
	
	    unsigned int vertex_shader;
	    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	    glShaderSource(vertex_shader, 1, (const char* const *)&vertex_source, NULL);
	    glCompileShader(vertex_shader);

	    int vertex_success;
	    char vertex_info_log[512];
	    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_success);
	    if(!vertex_success)
	    {
	    	glGetShaderInfoLog(vertex_shader, 512, NULL, vertex_info_log);
	    	printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED: %s\n",vertex_info_log);
	    }
	    free(vertex_source);

        // FRAGMENT

        char * fragment_source;
        int fragment_file = read_file(fragment_shaderPath, &fragment_source);
        if (fragment_file != 0)
        {
                printf("unable to compile shader. fragment shader couldn't be found.\n");
                return -1;
        }

	    unsigned int fragment_shader;
	    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	    glShaderSource(fragment_shader, 1, (const char* const *)&fragment_source, NULL);
	    glCompileShader(fragment_shader);
	    
	    int fragment_success;
	    char fragment_info_log[512];
	    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_success);
	    if(!fragment_success)
	    {
	    	glGetShaderInfoLog(fragment_shader, 512, NULL, fragment_info_log);
	    	printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED: %s\n",fragment_info_log);
	    }
	    free(fragment_source);

	    //SHADER PROGRAM
	    unsigned int shader = glCreateProgram();
	    glAttachShader(shader, vertex_shader);
	    glAttachShader(shader, fragment_shader);
	    glLinkProgram(shader);

	    int shader_success;
	    char shader_info_log[512];
	    glGetProgramiv(shader, GL_LINK_STATUS, &shader_success);
	    if(!shader_success)
	    {
	    	glGetProgramInfoLog(shader, 512, NULL, shader_info_log);
	    	printf("ERROR::SHADER::PROGRAM::COMPILATION_FAILED: %s\n",shader_info_log);
	    }
	    
	    glDeleteShader(vertex_shader);
	    glDeleteShader(fragment_shader);

	    return shader;
}


float vertex_data[] = {
        // VERTEX           UV
        -1.0f, -1.0f, 0.0f, 1.0f,1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,0.0f,
         1.0f,  1.0f, 0.0f, 0.0f,0.0f,

         1.0f,  1.0f, 0.0f, 0.0f,0.0f,
         1.0f, -1.0f, 0.0f, 0.0f,1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,1.0f
};


// Update the framebuffer size to the window size 
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
        glViewport(0,0,width,height);
}


// global client camera
struct Camera camera;


void camera_process(struct Camera* camera)
{
        camera->front = glm::normalize(camera->direction);

        camera->right = glm::normalize(glm::cross(glm::vec3(0.0f,1.0f,0.0f), camera->direction));
        camera->up = glm::cross(camera->direction, camera->right);

        camera->view = glm::lookAt(camera->position, camera->position+camera->front, camera->up);
}


double last_x,last_y;
bool first_mouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
        if (first_mouse)
        {
                last_x = xpos;
                last_y = ypos;
                first_mouse = false;
        }

        float x_offset = xpos - last_x;
        float y_offset = ypos - last_y;
        last_x = xpos;
        last_y = ypos;

        x_offset *= camera.sensitivity;
        y_offset *= camera.sensitivity;

        camera.yaw += x_offset;
        camera.pitch -= y_offset;

        if (camera.pitch > 89.0f)
                camera.pitch = 89.0f;
        if (camera.pitch < -89.0f)
                camera.pitch = -89.0f;
        
        camera.direction.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
        camera.direction.y = sin(glm::radians(camera.pitch));
        camera.direction.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
}


void input_process(GLFWwindow* window, struct Camera* camera, float frame_delta)
{
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);
        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                camera->position += camera->speed * frame_delta * camera->front;
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                camera->position -= camera->speed * frame_delta * camera->front;
        if(glfwGetKey(window, GLFW_KEY_A))
                camera->position -= glm::normalize(glm::cross(camera->front, camera->up)) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_D))
                camera->position += glm::normalize(glm::cross(camera->front, camera->up)) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT))
                camera->position -= glm::vec3(0.0,1.0,0.0) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_SPACE))
                camera->position += glm::vec3(0.0,1.0,0.0) * camera->speed * frame_delta;
        if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL))
                camera->speed = 2.0;
        else
                camera->speed = 1.0f;
}


int main(int argc, char* argv[])
{
	    if (!glfwInit())
		        return -1;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* window;
	    int width = 800,height = 600;

	    window = glfwCreateWindow(width,height, "global lattice test", NULL, NULL);
	    if(!window)
	    {
	    	glfwTerminate();
	    	printf("unable to open a window context.");
	    	return -1;
	    }

	    glfwMakeContextCurrent(window);
	    gladLoadGL(glfwGetProcAddress);

        printf("OpenGL version: %s\n",glGetString(GL_VERSION));

        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
        glfwSetCursorPosCallback(window, mouse_callback);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // viewport thingy
        
        unsigned int vao=0,vbo=0,shader=0;
        size_t vbo_size = sizeof(vertex_data);

        printf("vao: %d vbo: %d shader: %d\n",vao,vbo,shader);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vbo_size, vertex_data, GL_STATIC_DRAW);

        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        shader = load_shader("resources/genericVertex.glsl","resources/genericFragment.glsl");

        if (shader == -1 || shader == 0)
            return -1;

        int size = 10;
        int chunk_data_size = size*size*size;
        printf("chunk data size: %d\n",chunk_data_size);
        int* chunk_data = (int*) malloc(chunk_data_size*sizeof(int));
        printf("size of chunk_data: %zu\n",chunk_data_size*sizeof(int));

        for (int i = 0 ; i < chunk_data_size ; i++)
        {
                *(chunk_data+i) = rand()%2;
        }


        double previous_frame_time,current_frame_time,frame_delta = 0.0f;
        unsigned int frame_count = 0;

        glfwSwapInterval(0);

        printf("vao: %d vbo: %d shader: %d vbo_size: %zu\n",vao,vbo,shader,vbo_size);

        while(!glfwWindowShouldClose(window))
        {
                // calculate FPS
                current_frame_time = glfwGetTime();
                frame_delta = current_frame_time - previous_frame_time;
                frame_count++;
                if (frame_delta >= 1.0 / 30.0)
                {
                        char title[30];
                        sprintf(title, "time: %f fps: %f", (frame_delta/frame_count) * 1000, (1.0f/frame_delta) * frame_count);
                        glfwSetWindowTitle(window, title);
                        previous_frame_time = current_frame_time;
                        frame_count = 0;
                }
                
                glClearColor(0.4f,0.5f,0.6f,1.0f);
		        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
                input_process(window, &camera, frame_delta);

                //camera_process(&camera);

                glUseProgram(shader);
                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLES, 0, vbo_size);
                
                glfwSwapBuffers(window);

		        glfwPollEvents();
	    }

        free(chunk_data);

        glfwTerminate();

        return 0;
}
