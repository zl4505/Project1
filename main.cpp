#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl";
const char F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const char TEXTURE_FILEPATH_0[] = "assets/t0.png";
const char TEXTURE_FILEPATH_1[] = "assets/t1.jpeg";

const float BG_RED = 0.0f;
const float BG_GREEN = 0.0f;
const float BG_BLUE = 0.0f;
const float BG_OPACITY = 1.0f;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

// Triangle properties
glm::mat4 g_triangle_matrix;
GLuint g_triangle_texture;
float g_triangle_x = 0.0f;
float g_triangle_speed = 1.0f;

// Rectangle properties
glm::mat4 g_rectangle_matrix;
GLuint g_rectangle_texture;
float g_rotation_angle = 0.0f;

constexpr int NUMBER_OF_TEXTURES = 1; // to be generated, that is
constexpr GLint LEVEL_OF_DETAIL = 0; // base image level; Level n is the nth mipmap reduction image
constexpr GLint TEXTURE_BORDER = 0; // this value MUST be zero
GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}

void initialise() {
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("2D Scene",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    // Load textures
    g_triangle_texture = load_texture(TEXTURE_FILEPATH_0);
    g_rectangle_texture = load_texture(TEXTURE_FILEPATH_1);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            g_game_is_running = false;
        }
    }
}

void update() {
    static float prev_ticks = (float)SDL_GetTicks() / 1000.0f;
    float current_ticks = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = current_ticks - prev_ticks;
    prev_ticks = current_ticks;

    // Update triangle position (horizontal movement)
    g_triangle_x += g_triangle_speed * delta_time;
    if (g_triangle_x > 3.0f || g_triangle_x < -3.0f) {
        g_triangle_speed *= -1;
    }

    // Update rectangle rotation
    g_rotation_angle += glm::radians(90.0f) * delta_time;

    // Reset matrices
    g_triangle_matrix = glm::mat4(1.0f);
    g_rectangle_matrix = glm::mat4(1.0f);

    // Transform triangle
    g_triangle_matrix = glm::translate(g_triangle_matrix, glm::vec3(g_triangle_x, 0.0f, 0.0f));

    // Transform rectangle (follow triangle with offset)
    g_rectangle_matrix = glm::translate(g_rectangle_matrix,
        glm::vec3(g_triangle_x + 1.5f, 0.0f, 0.0f));
    g_rectangle_matrix = glm::rotate(g_rectangle_matrix, g_rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));
}

void draw_object(glm::mat4& model_matrix, GLuint texture, float* vertices, float* tex_coords) {
    g_shader_program.set_model_matrix(model_matrix);

    glBindTexture(GL_TEXTURE_2D, texture);

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Triangle vertices and texture coordinates
    float triangle_vertices[] = {
         0.0f,  0.5f,
        -0.5f, -0.5f,
         0.5f, -0.5f
    };

    float triangle_tex_coords[] = {
        0.5f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    // Rectangle vertices and texture coordinates
    float rectangle_vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f
    };

    float rectangle_tex_coords[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f
    };

    // Draw objects
    draw_object(g_triangle_matrix, g_triangle_texture, triangle_vertices, triangle_tex_coords);
    draw_object(g_rectangle_matrix, g_rectangle_texture, rectangle_vertices, rectangle_tex_coords);

    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

int main(int argc, char* argv[]) {
    initialise();

    while (g_game_is_running) {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}

