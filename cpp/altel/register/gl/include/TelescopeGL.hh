#ifndef TELESCOPE_GL_HH
#define TELESCOPE_GL_HH

#include <vector>
#include <memory>

#define GLEW_STATIC
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace sf{
  class Window;
}


struct UniformLayer
{
  GLfloat pos[4];   //vec3, pad
  GLfloat color[4]; //vec3, pad
  GLfloat pitch[4]; //pitch x,y, thick z, pad
  GLint   npixel[4];//pixe number x, y, z/1, pad
  GLfloat miss_alignment[16]; //mat4
};

class TelescopeGL{
public:
  glm::mat4 m_model;
  glm::mat4 m_view;
  glm::mat4 m_proj;
  std::unique_ptr<sf::Window> m_window;
  
  UniformLayer m_uniLayers[6];
  
  static const GLchar* vertexShaderSrc;
  static const GLchar* geometryShaderSrc;
  static const GLchar* fragmentShaderSrc;
  GLuint m_vertexShader{0};
  GLuint m_geometryShader{0};
  GLuint m_fragmentShader{0};
  GLuint m_shaderProgram{0};
  GLuint m_vao{0};
  GLuint m_vbo{0};
  GLuint m_vbo_tel{0};
  GLuint m_uboLayers[6]{0,0,0,0,0,0};
  GLint m_uniModel{0};
  GLint m_uniView{0};
  GLint m_uniProj{0};
  std::vector<GLfloat> m_points;
  std::vector<GLint> m_points_tel;
  
  static const GLchar* vertexShaderSrc_hit;
  static const GLchar* geometryShaderSrc_hit;
  static const GLchar* fragmentShaderSrc_hit;
  GLuint m_vertexShader_hit{0};
  GLuint m_geometryShader_hit{0};
  GLuint m_fragmentShader_hit{0};
  GLuint m_shaderProgram_hit{0};
  GLuint m_vao_hit{0};
  GLuint m_vbo_hit{0};
  GLint m_uniModel_hit{0};
  GLint m_uniView_hit{0};
  GLint m_uniProj_hit{0};  
  std::vector<GLfloat> m_points_hit;
  
  TelescopeGL();
  ~TelescopeGL();
  
  void initializeGL();
  void terminateGL();

  void addTelLayer(float hx, float hy, float hz, 
                   float px, float py, float pz,
                   float rx, float ry, float rz,
                   float cr, float cg, float cb );

  
  void buildProgramTel();
  void buildProgramHit();

  void clearFrame();
  void flushFrame();
  void drawTel();
  void drawHit();

  void addHit(float px, float py, float pz);
  void clearHit();  
  
  static GLuint createShader(GLenum type, const GLchar* src);  
};

#endif
