// Vertex shader
#include "TelescopeGL.hh"

const GLchar* TelescopeGL::vertexShaderSrc = R"glsl(
    #version 150 core

    in vec3 color;
    //in vec3 thick;

    in uint id;
    in uint amount;

    in vec3 pos;

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




////////////////////////


const GLchar* TelescopeGL::vertexShaderSrc_hit = R"glsl(
    #version 150 core
    in vec3 pos;
    out vec3 vColor;

    void main()
    {
        gl_Position = vec4(pos, 1.0);
        vColor = vec3(0, 1, 0);
    }
)glsl";



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
