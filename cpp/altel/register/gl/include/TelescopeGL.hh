#ifndef TELESCOPE_GL_HH
#define TELESCOPE_GL_HH

#define GLEW_STATIC

#include <vector>
#include <memory>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace sf{
  class Window;
}

class TelescopeGL{
public:
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
  std::unique_ptr<sf::Window> window;
  

  static const GLchar* vertexShaderSrc;
  static const GLchar* geometryShaderSrc;
  static const GLchar* fragmentShaderSrc;
  GLuint vertexShader{0};
  GLuint geometryShader{0};
  GLuint fragmentShader{0};
  GLuint shaderProgram{0};
  GLuint vao{0};
  GLuint vbo{0};
  GLint uniModel{0};
  GLint uniView{0};
  GLint uniProj{0};
  std::vector<GLfloat> points;



  static const GLchar* vertexShaderSrc_hit;
  static const GLchar* geometryShaderSrc_hit;
  static const GLchar* fragmentShaderSrc_hit;
  GLuint vertexShader_hit{0};
  GLuint geometryShader_hit{0};
  GLuint fragmentShader_hit{0};
  GLuint shaderProgram_hit{0};
  GLuint vao_hit{0};
  GLuint vbo_hit{0};
  GLint uniModel_hit{0};
  GLint uniView_hit{0};
  GLint uniProj_hit{0};  
  std::vector<GLfloat> points_hit;
  
  TelescopeGL();
  ~TelescopeGL();
  
  void InitializeGL();
  void TerminateGL();
  void BuildProgramTel();
  void BuildProgramHit();
  
  void ClearFrame();
  void FlushFrame();
  void DrawTel();
  void DrawHit();
  
  //static GLuint createShader(GLenum type, const GLchar* src);  
};

#endif
