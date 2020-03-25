// Vertex shader
#include "TelescopeGL.hh"

const GLchar* TelescopeGL::vertexShaderSrc = R"glsl(
#version 150 core

in  int  l;
out int  nl;

void main(){
  nl = l;
}
)glsl";

// // Geometry shader
const GLchar* TelescopeGL::geometryShaderSrc = R"glsl(
#version 150 core

layout(points) in;
layout(line_strip, max_vertices = 16) out;

layout (std140) uniform UniformLayer{
  vec3  position; //
  vec3  color;    //
  vec3  pitch;    // pitch_x pitch_y pitch_z/thick_z
  uvec3 npixel;   // pixel_x pixel_y pixel_z/always1
  mat4  trans;    // 
} layers[10];

in int nl[];
out vec3 fColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main(){

  vec3   pos;
  vec3   color;
  vec3   pitch;
  uvec3  npixel;
  mat4   trans;

  switch(nl[0]){
  case 0:
    pos    = layers[0].position;
    color  = layers[0].color;
    pitch  = layers[0].pitch;
    npixel = layers[0].npixel;
    trans  = layers[0].trans;
    break;
  case 1:
    pos    = layers[1].position;
    color  = layers[1].color;
    pitch  = layers[1].pitch;
    npixel = layers[1].npixel;
    trans  = layers[1].trans;
    break;
  case 2:
    pos    = layers[2].position;
    color  = layers[2].color;
    pitch  = layers[2].pitch;
    npixel = layers[2].npixel;
    trans  = layers[2].trans;
    break;
  case 3:
    pos    = layers[3].position;
    color  = layers[3].color;
    pitch  = layers[3].pitch;
    npixel = layers[3].npixel;
    trans  = layers[3].trans;
    break;
  case 4:
    pos    = layers[4].position;
    color  = layers[4].color;
    pitch  = layers[4].pitch;
    npixel = layers[4].npixel;
    trans  = layers[4].trans;
    break;
  case 5:
    pos    = layers[5].position;
    color  = layers[5].color;
    pitch  = layers[5].pitch;
    npixel = layers[5].npixel;
    trans  = layers[5].trans;
    break;
  case 6:
    pos    = layers[6].position;
    color  = layers[6].color;
    pitch  = layers[6].pitch;
    npixel = layers[6].npixel;
    trans  = layers[6].trans;
    break;
  case 7:
    pos    = layers[7].position;
    color  = layers[7].color;
    pitch  = layers[7].pitch;
    npixel = layers[7].npixel;
    trans  = layers[7].trans;
    break;
  case 8:
    pos    = layers[8].position;
    color  = layers[8].color;
    pitch  = layers[8].pitch;
    npixel = layers[8].npixel;
    trans  = layers[8].trans;
    break;
  case 9:
    pos    = layers[9].position;
    color  = layers[9].color;
    pitch  = layers[9].pitch;
    npixel = layers[9].npixel;
    trans  = layers[9].trans;
    break;
  default:
    pos    = layers[0].position;
    color  = layers[0].color;
    pitch  = layers[0].pitch;
    npixel = layers[0].npixel;
    trans  = layers[0].trans;
    break;
  }
  fColor = color;

  mat4 pvmMatrix = proj * view * model * trans;
  vec4 gp = pvmMatrix * vec4(pos, 1.0);
  vec3 thick  = pitch.xyz * npixel.xyz;  
  //                                     N/S       U/D       E/W
  //                                     X         Y         Z
  vec4 NEU = pvmMatrix * vec4( thick.x,  thick.y,  thick.z, 0.0);
  vec4 NED = pvmMatrix * vec4( thick.x,  0,        thick.z, 0.0);
  vec4 NWU = pvmMatrix * vec4( thick.x,  thick.y,  0,       0.0);
  vec4 NWD = pvmMatrix * vec4( thick.x,  0,        0,       0.0);
  vec4 SEU = pvmMatrix * vec4( 0,        thick.y,  thick.z, 0.0);
  vec4 SED = pvmMatrix * vec4( 0,        0,        thick.z, 0.0);
  vec4 SWU = pvmMatrix * vec4( 0,        thick.y,  0,       0.0);
  vec4 SWD = pvmMatrix * vec4( 0,        0,        0,       0.0);
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


// Fragment shader
const GLchar* TelescopeGL::fragmentShaderSrc = R"glsl(
#version 150 core

in vec3 fColor;

out vec4 outColor;

void main(){
  outColor = vec4(fColor, 1.0);
}
)glsl";



////////////////////////
const GLchar* TelescopeGL::vertexShaderSrc_hit = R"glsl(
#version 150 core

in ivec3 pos;

out vec3 vColor;

void main()
{
  gl_Position = vec4(float(pos.x), float(pos.y), float(pos.z), 1.0);
  vColor = vec3(0, 1, 0);
}

)glsl";


const GLchar* TelescopeGL::geometryShaderSrc_hit = R"glsl(
#version 150 core

layout(points) in;
in vec3 vColor[];

layout(line_strip, max_vertices = 2) out;
out vec3 fColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

layout (std140) uniform UniformLayer{
  vec3  position; //
  vec3  color;    //
  vec3  pitch;    // pitch_x pitch_y pitch_z/thick_z
  uvec3 npixel;   // pixel_x pixel_y pixel_z/always1
  mat4  trans;    // 
} layers[10];

void main(){
  int    nlx = int(gl_in[0].gl_Position.z+0.1);
  vec3   pos;
  vec3   color;
  vec3   pitch;
  uvec3  npixel;
  mat4   trans;
  switch(nlx){
  case 1:
    pos    = layers[0].position;
    color  = layers[0].color;
    pitch  = layers[0].pitch;
    npixel = layers[0].npixel;
    trans  = layers[0].trans;
    break;
  case 0:
    pos    = layers[1].position;
    color  = layers[1].color;
    pitch  = layers[1].pitch;
    npixel = layers[1].npixel;
    trans  = layers[1].trans;
    break;
  case 2:
    pos    = layers[2].position;
    color  = layers[2].color;
    pitch  = layers[2].pitch;
    npixel = layers[2].npixel;
    trans  = layers[2].trans;
    break;
  case 3:
    pos    = layers[3].position;
    color  = layers[3].color;
    pitch  = layers[3].pitch;
    npixel = layers[3].npixel;
    trans  = layers[3].trans;
    break;
  case 4:
    pos    = layers[4].position;
    color  = layers[4].color;
    pitch  = layers[4].pitch;
    npixel = layers[4].npixel;
    trans  = layers[4].trans;
    break;
  case 5:
    pos    = layers[5].position;
    color  = layers[5].color;
    pitch  = layers[5].pitch;
    npixel = layers[5].npixel;
    trans  = layers[5].trans;
    break;
  case 6:
    pos    = layers[6].position;
    color  = layers[6].color;
    pitch  = layers[6].pitch;
    npixel = layers[6].npixel;
    trans  = layers[6].trans;
    break;
  case 7:
    pos    = layers[7].position;
    color  = layers[7].color;
    pitch  = layers[7].pitch;
    npixel = layers[7].npixel;
    trans  = layers[7].trans;
    break;
  case 8:
    pos    = layers[8].position;
    color  = layers[8].color;
    pitch  = layers[8].pitch;
    npixel = layers[8].npixel;
    trans  = layers[8].trans;
    break;
  case 9:
    pos    = layers[9].position;
    color  = layers[9].color;
    pitch  = layers[9].pitch;
    npixel = layers[9].npixel;
    trans  = layers[9].trans;
    break;
  default:
    pos    = layers[5].position;
    color  = layers[5].color;
    pitch  = layers[5].pitch;
    npixel = layers[5].npixel;
    trans  = layers[5].trans;
    break;
  }


  vec3 pos_hit = pos.xyz + vec3(pitch.xy * gl_in[0].gl_Position.xy , 0); 
  //fColor = vColor[0];
  fColor = color;

  mat4 pvmMatrix = proj * view * model * trans;
  vec4 gp = pvmMatrix * vec4(pos_hit, 1.0);
  vec4 offset = pvmMatrix * vec4( 0.0,  0.0,  1.0, 0.0);

  gl_Position = gp + offset;
  EmitVertex();

  gl_Position = gp - offset;
  EmitVertex();

  EndPrimitive();
}
)glsl";
