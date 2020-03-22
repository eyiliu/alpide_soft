#include "TelescopeGL.hh"
#include <SFML/Window.hpp>
#include <chrono>
#include <thread>
#include <iostream>

// Vertex shader
const GLchar* TelescopeGL::vertexShaderSrc = R"glsl(
    #version 150 core

    in vec3 pos;
    in vec3 color;

    out vec3 vColor;

    void main()
    {
        gl_Position = vec4(pos, 1.0);
        vColor = color;
    }
)glsl";

// Geometry shader
const GLchar* TelescopeGL::geometryShaderSrc = R"glsl(
    #version 150 core

    layout(points) in;
    layout(line_strip, max_vertices = 16) out;

    in vec3 vColor[];
    out vec3 fColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;

    void main()
    {
        fColor = vColor[0];

        vec4 gp = proj * view * model * gl_in[0].gl_Position;


        // +X direction is "North", -X direction is "South"
        // +Y direction is "Up",    -Y direction is "Down"
        // +Z direction is "East",  -Z direction is "West"
        //                                     N/S   U/D   E/W
        //                                     X     Y     Z
        vec4 NEU = proj * view * model * vec4( 15.0,  7.5,  1.1, 0.0);
        vec4 NED = proj * view * model * vec4( 15.0, -7.5,  1.1, 0.0);
        vec4 NWU = proj * view * model * vec4( 15.0,  7.5, -1.1, 0.0);
        vec4 NWD = proj * view * model * vec4( 15.0, -7.5, -1.1, 0.0);
        vec4 SEU = proj * view * model * vec4(-15.0,  7.5,  1.1, 0.0);
        vec4 SED = proj * view * model * vec4(-15.0, -7.5,  1.1, 0.0);
        vec4 SWU = proj * view * model * vec4(-15.0,  7.5, -1.1, 0.0);
        vec4 SWD = proj * view * model * vec4(-15.0, -7.5, -1.1, 0.0);

        // Create a cube centered on the given point.
        gl_Position = gp + NED;
        EmitVertex();

        gl_Position = gp + NWD;
        EmitVertex();

        gl_Position = gp + SWD;
        EmitVertex();

        gl_Position = gp + SED;
        EmitVertex();

        gl_Position = gp + SEU;
        EmitVertex();

        gl_Position = gp + SWU;
        EmitVertex();

        gl_Position = gp + NWU;
        EmitVertex();

        gl_Position = gp + NEU;
        EmitVertex();

        gl_Position = gp + NED;
        EmitVertex();

        gl_Position = gp + SED;
        EmitVertex();

        gl_Position = gp + SEU;
        EmitVertex();

        gl_Position = gp + NEU;
        EmitVertex();

        gl_Position = gp + NWU;
        EmitVertex();

        gl_Position = gp + NWD;
        EmitVertex();

        gl_Position = gp + SWD;
        EmitVertex();

        gl_Position = gp + SWU;
        EmitVertex();

        EndPrimitive();
    }
)glsl";


// Geometry shader
const GLchar* TelescopeGL::geometryShaderSrc_hit = R"glsl(
    #version 150 core

    layout(points) in;
    layout(line_strip, max_vertices = 2) out;

    in vec3 vColor[];
    out vec3 fColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;

    void main()
    {
        fColor = vColor[0];

        vec4 gp = proj * view * model * gl_in[0].gl_Position;
        vec4 offset = proj * view * model * vec4( 0.0,  0.0,  1.0, 0.0);

        gl_Position = gp + offset;
        EmitVertex();

        gl_Position = gp - offset;
        EmitVertex();

        EndPrimitive();
    }
)glsl";




// Fragment shader
const GLchar* TelescopeGL::fragmentShaderSrc = R"glsl(
    #version 150 core

    in vec3 fColor;

    out vec4 outColor;

    void main()
    {
        outColor = vec4(fColor, 1.0);
    }
)glsl";


GLuint createShader(GLenum type, const GLchar* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint IsCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &IsCompiled);
    if(IsCompiled == GL_FALSE){
      GLint maxLength;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
      
      /* The maxLength includes the NULL character */
      char* infoLog = (char *)malloc(maxLength);

      glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog);
      
      /* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
      /* In this simple program, we'll just leave */
      std::cout<< infoLog<<std::endl;
      free(infoLog);
      throw;
    }
    return shader;
}

TelescopeGL::TelescopeGL(){
  points =
  {//  Coordinates             Color
   0.0f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f,
   0.0f,  0.0f,  30.0f, 0.0f, 1.0f, 0.0f,
   0.0f,  0.0f,  60.0f, 0.0f, 0.0f, 1.0f,
   0.0f,  0.0f,  120.0f, 1.0f, 1.0f, 0.0f,
   0.0f,  0.0f,  150.0f, 0.0f, 1.0f, 1.0f,
   0.0f,  0.0f,  180.0f, 1.0f, 0.0f, 1.0f,
   0.0f,  0.0f,  210.0f, 1.0f, 0.5f, 0.5f,
   0.0f,  0.0f,  240.0f, 0.5f, 1.0f, 0.5f,
  };

  points_hit =
  {//  Coordinates             Color
   0.0f,  0.0f,  0.0f, 1.0f, 0.0f, 0.0f,
   0.0f,  0.0f,  30.0f, 0.0f, 0.0f, 1.0f,
   0.0f,  0.0f,  60.0f, 0.0f, 1.0f, 0.0f,
   0.0f,  0.0f,  120.0f, 1.0f, 1.0f, 0.0f,
   0.0f,  0.0f,  150.0f, 0.0f, 1.0f, 1.0f,
   0.0f,  0.0f,  180.0f, 1.0f, 0.0f, 1.0f,
   0.0f,  0.0f,  210.0f, 1.0f, 0.5f, 0.5f,
   0.0f,  0.0f,  240.0f, 0.5f, 1.0f, 0.5f,
  };

  
  GLfloat win_width = 1200;
  GLfloat win_high  = 400;
  
  model = glm::mat4(1.0f);
    // auto t_now = std::chrono::high_resolution_clock::now();
    // float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();
    // model = glm::rotate(model,
    //                     0.25f * time * glm::radians(180.0f),
    //                     glm::vec3(0.0f, 0.0f, 1.0f));
    
  view = glm::lookAt(glm::vec3(-200.0f, 0.0f, 90.0f),
                               glm::vec3(0.0f, 0.0f, 90.0f),
                               glm::vec3(0.0f, 1.0f, 0.0f));
    
  proj = glm::perspective(glm::radians(30.0f), win_width/win_high, 1.0f, 500.0f);
  
  InitializeGL();
}


void TelescopeGL::InitializeGL(){
  
  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.majorVersion = 4;
  settings.minorVersion = 0;

  GLfloat win_width = 1200;
  GLfloat win_high  = 400;
  window = std::make_unique<sf::Window>(sf::VideoMode(win_width, win_high, 32), "Telescope", sf::Style::Titlebar | sf::Style::Close, settings);

  glewExperimental = GL_TRUE;
  GLenum err = glewInit();
  if (err != GLEW_OK){
    std::cerr<< "glew error\n";
    throw;
  }
  
}


TelescopeGL::~TelescopeGL(){
  TerminateGL();
  
}

void TelescopeGL::BuildProgramTel(){
  vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSrc);
  geometryShader = createShader(GL_GEOMETRY_SHADER, geometryShaderSrc);
  fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, geometryShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  
  glUseProgram(shaderProgram);
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glNamedBufferData(vbo, sizeof(GLfloat)*points.size(), points.data(), GL_STATIC_DRAW);
  
  GLint posAttrib = glGetAttribLocation(shaderProgram, "pos");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

  GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
  glEnableVertexAttribArray(colAttrib);
  glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

  uniModel = glGetUniformLocation(shaderProgram, "model");
  uniView = glGetUniformLocation(shaderProgram, "view");
  uniProj = glGetUniformLocation(shaderProgram, "proj");

  glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
  glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

}


void TelescopeGL::BuildProgramHit(){
  vertexShader_hit = createShader(GL_VERTEX_SHADER, vertexShaderSrc);
  geometryShader_hit = createShader(GL_GEOMETRY_SHADER, geometryShaderSrc_hit);
  fragmentShader_hit = createShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

  shaderProgram_hit = glCreateProgram();
  glAttachShader(shaderProgram_hit, vertexShader_hit);
  glAttachShader(shaderProgram_hit, geometryShader_hit);
  glAttachShader(shaderProgram_hit, fragmentShader_hit);
  glLinkProgram(shaderProgram_hit);


  glUseProgram(shaderProgram_hit);
  glGenVertexArrays(1, &vao_hit);
  glBindVertexArray(vao_hit);

  glGenBuffers(1, &vbo_hit);
    
  glBindBuffer(GL_ARRAY_BUFFER, vbo_hit);
  glNamedBufferData(vbo_hit, sizeof(GLfloat)*points_hit.size(), points_hit.data(), GL_STATIC_DRAW); //hit
  
  GLint posAttrib_hit = glGetAttribLocation(shaderProgram_hit, "pos");
  glEnableVertexAttribArray(posAttrib_hit);
  glVertexAttribPointer(posAttrib_hit, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

  GLint colAttrib_hit = glGetAttribLocation(shaderProgram_hit, "color");
  glEnableVertexAttribArray(colAttrib_hit);
  glVertexAttribPointer(colAttrib_hit, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

  GLint uniModel_hit = glGetUniformLocation(shaderProgram_hit, "model");
  GLint uniView_hit = glGetUniformLocation(shaderProgram_hit, "view");
  GLint uniProj_hit = glGetUniformLocation(shaderProgram_hit, "proj");

  glUniformMatrix4fv(uniModel_hit, 1, GL_FALSE, glm::value_ptr(model));
  glUniformMatrix4fv(uniView_hit, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(uniProj_hit, 1, GL_FALSE, glm::value_ptr(proj));
}

void TelescopeGL::ClearFrame(){
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

void TelescopeGL::DrawTel(){
  glBindVertexArray(vao);
  glUseProgram(shaderProgram);
  glDrawArrays(GL_POINTS, 0, 6);
}

void TelescopeGL::DrawHit(){
  points_hit[4] += 0.1;
  if(points_hit[4] > 1.0){
    points_hit[4] = 0;
  }
  glNamedBufferSubData(vbo_hit, 0, sizeof(GLfloat)*points_hit.size(), points_hit.data());
  glBindVertexArray(vao_hit);
  glUseProgram(shaderProgram_hit);
  glDrawArrays(GL_POINTS, 0, 6);
}

void TelescopeGL::FlushFrame(){
  if(window){
    window->display();
  }
}

void TelescopeGL::TerminateGL(){
  if(shaderProgram) {glDeleteProgram(shaderProgram); shaderProgram = 0;}
  if(fragmentShader){glDeleteShader(fragmentShader); fragmentShader = 0;}
  if(geometryShader){glDeleteShader(geometryShader); geometryShader = 0;}
  if(vertexShader){glDeleteShader(vertexShader); vertexShader = 0;}
  if(vbo){glDeleteBuffers(1, &vbo); vbo = 0;}
  if(vao){glDeleteVertexArrays(1, &vao); vao = 0;}
  if(shaderProgram_hit){glDeleteProgram(shaderProgram_hit); shaderProgram_hit = 0;}
  if(fragmentShader_hit){glDeleteShader(fragmentShader_hit); fragmentShader_hit = 0;}
  if(geometryShader_hit){glDeleteShader(geometryShader_hit); geometryShader_hit = 0;}
  if(vertexShader_hit){glDeleteShader(vertexShader_hit); vertexShader_hit = 0;}
  if(vbo_hit){glDeleteBuffers(1, &vbo_hit); vbo_hit = 0;}
  if(vao_hit){glDeleteVertexArrays(1, &vao_hit); vao_hit = 0;}
  if(window){window->close(); window.reset();}
  
}


int main(){
  TelescopeGL tel;
  tel.BuildProgramTel();
  tel.BuildProgramHit();
  
  bool running = true;
  while (running)
  {
    sf::Event windowEvent;
    while (tel.window->pollEvent(windowEvent))
    {
      switch (windowEvent.type)
      {
      case sf::Event::Closed:
        running = false;
        break;
      }

    }

    tel.ClearFrame();
    tel.DrawTel();
    tel.DrawHit();
    tel.FlushFrame();    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
