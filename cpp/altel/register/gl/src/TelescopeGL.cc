#include "TelescopeGL.hh"

#include <chrono>
#include <thread>
#include <iostream>

#include <SFML/Window.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint TelescopeGL::createShader(GLenum type, const GLchar* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint IsCompiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &IsCompiled);
    if(IsCompiled == GL_FALSE){
      GLint maxLength;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);      
      std::vector<GLchar> infoLog(maxLength, 0);
      glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());
      std::fprintf(stderr, "ERROR, unable to create shader.\n%s\n", infoLog.data());
      throw;
    }
    return shader;
}


void TelescopeGL::addTelLayer(float hx, float hy, float hz, 
                              float px, float py, float pz,
                              float rx, float ry, float rz,
                              float cr, float cg, float cb 
                              ){

  ///////////////////////position , color
  std::vector<GLfloat> l{px, py, pz, cr, cg, cb};
  m_points.insert(m_points.end(), l.begin(), l.end());  
}


TelescopeGL::TelescopeGL(){
  
  GLfloat win_width = 1200;
  GLfloat win_high  = 400;
  
  m_model = glm::mat4(1.0f);
    // auto t_now = std::chrono::high_resolution_clock::now();
    // float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_now - t_start).count();
    // model = glm::rotate(model,
    //                     0.25f * time * glm::radians(180.0f),
    //                     glm::vec3(0.0f, 0.0f, 1.0f));
    
  m_view = glm::lookAt(glm::vec3(-300.0f, 0.0f, 90.0f),
                       glm::vec3(0.0f, 0.0f, 90.0f),
                       glm::vec3(0.0f, 1.0f, 0.0f));
    
  m_proj = glm::perspective(glm::radians(15.0f), win_width/win_high, 1.0f, 500.0f);
  
  initializeGL();
}


void TelescopeGL::initializeGL(){
  
  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.majorVersion = 4;
  settings.minorVersion = 0;

  GLfloat win_width = 1200;
  GLfloat win_high  = 400;
  m_window = std::make_unique<sf::Window>(sf::VideoMode(win_width, win_high, 32), "Telescope", sf::Style::Titlebar | sf::Style::Close, settings);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK){
    std::fprintf(stderr, "glew error\n");
    throw;
  }
}


TelescopeGL::~TelescopeGL(){
  terminateGL();
  
}

void TelescopeGL::buildProgramTel(){
  m_vertexShader = createShader(GL_VERTEX_SHADER, vertexShaderSrc);
  m_geometryShader = createShader(GL_GEOMETRY_SHADER, geometryShaderSrc);
  m_fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

  m_shaderProgram = glCreateProgram();
  glAttachShader(m_shaderProgram, m_vertexShader);
  glAttachShader(m_shaderProgram, m_geometryShader);
  glAttachShader(m_shaderProgram, m_fragmentShader);
  glLinkProgram(m_shaderProgram);
  
  glUseProgram(m_shaderProgram);
  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  glGenBuffers(1, &m_vbo);
  
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glNamedBufferData(m_vbo, sizeof(GLfloat)*m_points.size(), m_points.data(), GL_STATIC_DRAW);
  
  GLint posAttrib = glGetAttribLocation(m_shaderProgram, "pos");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

  GLint colAttrib = glGetAttribLocation(m_shaderProgram, "color");
  glEnableVertexAttribArray(colAttrib);
  glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

  m_uniModel = glGetUniformLocation(m_shaderProgram, "model");
  m_uniView = glGetUniformLocation(m_shaderProgram, "view");
  m_uniProj = glGetUniformLocation(m_shaderProgram, "proj");

  glUniformMatrix4fv(m_uniModel, 1, GL_FALSE, glm::value_ptr(m_model));
  glUniformMatrix4fv(m_uniView, 1, GL_FALSE, glm::value_ptr(m_view));
  glUniformMatrix4fv(m_uniProj, 1, GL_FALSE, glm::value_ptr(m_proj));
}


void TelescopeGL::buildProgramHit(){
  m_vertexShader_hit = createShader(GL_VERTEX_SHADER, vertexShaderSrc_hit);
  m_geometryShader_hit = createShader(GL_GEOMETRY_SHADER, geometryShaderSrc_hit);
  m_fragmentShader_hit = createShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

  m_shaderProgram_hit = glCreateProgram();
  glAttachShader(m_shaderProgram_hit, m_vertexShader_hit);
  glAttachShader(m_shaderProgram_hit, m_geometryShader_hit);
  glAttachShader(m_shaderProgram_hit, m_fragmentShader_hit);
  glLinkProgram(m_shaderProgram_hit);

  glUseProgram(m_shaderProgram_hit);
  glGenVertexArrays(1, &m_vao_hit);
  glBindVertexArray(m_vao_hit);

  glGenBuffers(1, &m_vbo_hit);
    
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo_hit);
  glNamedBufferData(m_vbo_hit, sizeof(GLfloat)*m_points_hit.size(), m_points_hit.data(), GL_STATIC_DRAW); //hit
  
  GLint posAttrib_hit = glGetAttribLocation(m_shaderProgram_hit, "pos");
  glEnableVertexAttribArray(posAttrib_hit);
  glVertexAttribPointer(posAttrib_hit, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);


  m_uniModel_hit = glGetUniformLocation(m_shaderProgram_hit, "model");
  m_uniView_hit = glGetUniformLocation(m_shaderProgram_hit, "view");
  m_uniProj_hit = glGetUniformLocation(m_shaderProgram_hit, "proj");

  glUniformMatrix4fv(m_uniModel_hit, 1, GL_FALSE, glm::value_ptr(m_model));
  glUniformMatrix4fv(m_uniView_hit, 1, GL_FALSE, glm::value_ptr(m_view));
  glUniformMatrix4fv(m_uniProj_hit, 1, GL_FALSE, glm::value_ptr(m_proj));
}

void TelescopeGL::clearFrame(){
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}


void TelescopeGL::drawTel(){
  glUseProgram(m_shaderProgram);
  glBindVertexArray(m_vao);
  glDrawArrays(GL_POINTS, 0, 6);
}


void TelescopeGL::addHit(float px, float py, float pz){  
  std::vector<GLfloat> h{px, py, pz};
  m_points_hit.insert(m_points_hit.end(), h.begin(), h.end());
}

void TelescopeGL::clearHit(){
  m_points_hit.clear();
}

void TelescopeGL::drawHit(){
  glUseProgram(m_shaderProgram_hit);
  glBindVertexArray(m_vao_hit);
  glNamedBufferData(m_vbo_hit, sizeof(GLfloat)*m_points_hit.size(), m_points_hit.data(), GL_STATIC_DRAW);
  glDrawArrays(GL_POINTS, 0, m_points_hit.size()/3);  
}

void TelescopeGL::flushFrame(){
  if(m_window){
    m_window->display();
  }
}

void TelescopeGL::terminateGL(){
  if(m_shaderProgram) {glDeleteProgram(m_shaderProgram); m_shaderProgram = 0;}
  if(m_fragmentShader){glDeleteShader(m_fragmentShader); m_fragmentShader = 0;}
  if(m_geometryShader){glDeleteShader(m_geometryShader); m_geometryShader = 0;}
  if(m_vertexShader){glDeleteShader(m_vertexShader); m_vertexShader = 0;}
  if(m_vbo){glDeleteBuffers(1, &m_vbo); m_vbo = 0;}
  if(m_vao){glDeleteVertexArrays(1, &m_vao); m_vao = 0;}
  if(m_shaderProgram_hit){glDeleteProgram(m_shaderProgram_hit); m_shaderProgram_hit = 0;}
  if(m_fragmentShader_hit){glDeleteShader(m_fragmentShader_hit); m_fragmentShader_hit = 0;}
  if(m_geometryShader_hit){glDeleteShader(m_geometryShader_hit); m_geometryShader_hit = 0;}
  if(m_vertexShader_hit){glDeleteShader(m_vertexShader_hit); m_vertexShader_hit = 0;}
  if(m_vbo_hit){glDeleteBuffers(1, &m_vbo_hit); m_vbo_hit = 0;}
  if(m_vao_hit){glDeleteVertexArrays(1, &m_vao_hit); m_vao_hit = 0;}
  if(m_window){m_window->close(); m_window.reset();}  
}


int main(){
  TelescopeGL tel;
  tel.addTelLayer(30, 15, 0.1, 0, 0, 0, 0, 0, 0, 1, 0, 0);
  tel.addTelLayer(30, 15, 0.1, 0, 0, 30, 0, 0, 0, 0, 1, 0);
  tel.addTelLayer(30, 15, 0.1, 0, 0, 60, 0, 0, 0, 0, 0, 1);
  tel.addTelLayer(30, 15, 0.1, 0, 0, 120, 0, 0, 0, 1, 1, 0);
  tel.addTelLayer(30, 15, 0.1, 0, 0, 150, 0, 0, 0, 0, 1, 1);
  tel.addTelLayer(30, 15, 0.1, 0, 0, 180, 0, 0, 0, 1, 0, 1);

  tel.buildProgramTel();
  tel.buildProgramHit();
  
  bool running = true;
  while (running)
  {
    sf::Event windowEvent;
    while (tel.m_window->pollEvent(windowEvent))
    {
      switch (windowEvent.type)
      {
      case sf::Event::Closed:
        running = false;
        break;
      }  
    }
    tel.clearHit();
    tel.addHit(0, 0, 0);
    tel.addHit(0, 0, 30);
    tel.addHit(0, 0, 60);
    tel.addHit(0, 0, 120);
    tel.addHit(0, 0, 150);
    tel.addHit(0, 0, 180);

    
    tel.clearFrame();
    tel.drawTel();
    tel.drawHit();
    tel.flushFrame();    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}
