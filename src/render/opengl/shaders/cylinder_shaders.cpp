// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.

#include "polyscope/render/opengl/gl_shaders.h"

namespace polyscope {
namespace render {

// clang-format off

const ShaderStageSpecification PASSTHRU_CYLINDER_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position_tail", DataType::Vector3Float},
        {"a_position_tip", DataType::Vector3Float},
    },

    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        uniform mat4 u_modelView;

        out vec4 position_tip;
        
        void main()
        {
            gl_Position = u_modelView * vec4(a_position_tail,1.0);
            position_tip = u_modelView * vec4(a_position_tip, 1.0);
        }
    )
};

const ShaderStageSpecification CYLINDER_VALUE_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position_tail", DataType::Vector3Float},
        {"a_position_tip", DataType::Vector3Float},
        {"a_value", DataType::Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in float a_value;
        uniform mat4 u_modelView;

        out vec4 position_tip;
        out float edge_value;

        void main()
        {
            gl_Position = u_modelView * vec4(a_position_tail,1.0);
            position_tip = u_modelView * vec4(a_position_tip, 1.0);
            edge_value = a_value;
        }
    )
};

const ShaderStageSpecification CYLINDER_COLOR_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position_tail", DataType::Vector3Float},
        {"a_position_tip", DataType::Vector3Float},
        {"a_color", DataType::Vector3Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in vec3 a_color;
        uniform mat4 u_modelView;

        out vec4 position_tip;
        out vec3 edge_color;

        void main()
        {
            gl_Position = u_modelView * vec4(a_position_tail,1.0);
            position_tip = u_modelView * vec4(a_position_tip, 1.0);
            edge_color = a_color;
        }
    )
};

const ShaderStageSpecification CYLINDER_BLEND_VALUE_VERT_SHADER = {

    ShaderStageType::Vertex,

    // uniforms
    {
        {"u_modelView", DataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position_tail", DataType::Vector3Float},
        {"a_position_tip", DataType::Vector3Float},
        {"a_value_tail", DataType::Float},
        {"a_value_tip", DataType::Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in float a_value_tail;
        in float a_value_tip;
        uniform mat4 u_modelView;

        out vec4 position_tip;
        out float value_tail;
        out float value_tip;

        void main()
        {
            gl_Position = u_modelView * vec4(a_position_tail,1.0);
            position_tip = u_modelView * vec4(a_position_tip, 1.0);
            value_tail = a_value_tail;
            value_tip = a_value_tip;
        }
    )
};

const ShaderStageSpecification CYLINDER_BLEND_COLOR_VERT_SHADER = {
    
     ShaderStageType::Vertex,

    { // uniforms
        {"u_modelView", DataType::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position_tail", DataType::Vector3Float},
        {"a_position_tip", DataType::Vector3Float},
        {"a_color_tail", DataType::Vector3Float},
        {"a_color_tip", DataType::Vector3Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in vec3 a_color_tail;
        in vec3 a_color_tip;
        uniform mat4 u_modelView;

        out vec4 position_tip;
        out vec3 color_tail;
        out vec3 color_tip;

        void main()
        {
            gl_Position = u_modelView * vec4(a_position_tail,1.0);
            position_tip = u_modelView * vec4(a_position_tip, 1.0);
            color_tail = a_color_tail;
            color_tip = a_color_tip;
        }
    )
};

const ShaderStageSpecification CYLINDER_PICK_VERT_SHADER = {
    
    ShaderStageType::Vertex,

    { 
        {"u_modelView", DataType::Matrix44Float},
    }, // uniforms

    // attributes
    {
        {"a_position_tail", DataType::Vector3Float},
        {"a_position_tip", DataType::Vector3Float},
        {"a_color_tail", DataType::Vector3Float},
        {"a_color_tip", DataType::Vector3Float},
        {"a_color_edge", DataType::Vector3Float},
    },
    
    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position_tail;
        in vec3 a_position_tip;
        in vec3 a_color_tail;
        in vec3 a_color_tip;
        in vec3 a_color_edge;
        uniform mat4 u_modelView;

        out vec4 position_tip;
        out vec3 color_tail;
        out vec3 color_tip;
        out vec3 color_edge;

        void main()
        {
            gl_Position = u_modelView * vec4(a_position_tail,1.0);
            position_tip = u_modelView * vec4(a_position_tip, 1.0);
            color_tail = a_color_tail;
            color_tip = a_color_tip;
            color_edge = a_color_edge;
        }
    )
};



const ShaderStageSpecification CYLINDER_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_radius", DataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;
        in vec4 position_tip[];
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {

            // Build an orthogonal basis
            vec3 tailViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
            vec3 tipViewVal = position_tip[0].xyz / position_tip[0].w;
            vec3 cylDir = normalize(tipViewVal - tailViewVal);
            vec3 basisX; vec3 basisY; buildTangentBasis(cylDir, basisX, basisY);
  
            // Compute corners of cube
            vec4 tailProj = u_projMatrix * gl_in[0].gl_Position;
            vec4 tipProj = u_projMatrix * position_tip[0];
            vec4 dx = u_projMatrix * vec4(basisX * u_radius, 0.);
            vec4 dy = u_projMatrix * vec4(basisY * u_radius, 0.);

            vec4 p1 = tailProj - dx - dy;
            vec4 p2 = tailProj + dx - dy;
            vec4 p3 = tailProj - dx + dy;
            vec4 p4 = tailProj + dx + dy;
            vec4 p5 = tipProj - dx - dy;
            vec4 p6 = tipProj + dx - dy;
            vec4 p7 = tipProj - dx + dy;
            vec4 p8 = tipProj + dx + dy;
            
            // Other data to emit   
    
            // Emit the vertices as a triangle strip
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p6; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p1; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex();
    
            EndPrimitive();

        }

    )
};

const ShaderStageSpecification CYLINDER_VALUE_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_radius", DataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;
        in vec4 position_tip[];
        in float edge_value[];
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;
        out float value;

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {

            // Build an orthogonal basis
            vec3 tailViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
            vec3 tipViewVal = position_tip[0].xyz / position_tip[0].w;
            vec3 cylDir = normalize(tipViewVal - tailViewVal);
            vec3 basisX; vec3 basisY; buildTangentBasis(cylDir, basisX, basisY);
  
            // Compute corners of cube
            vec4 tailProj = u_projMatrix * gl_in[0].gl_Position;
            vec4 tipProj = u_projMatrix * position_tip[0];
            vec4 dx = u_projMatrix * vec4(basisX * u_radius, 0.);
            vec4 dy = u_projMatrix * vec4(basisY * u_radius, 0.);

            vec4 p1 = tailProj - dx - dy;
            vec4 p2 = tailProj + dx - dy;
            vec4 p3 = tailProj - dx + dy;
            vec4 p4 = tailProj + dx + dy;
            vec4 p5 = tipProj - dx - dy;
            vec4 p6 = tipProj + dx - dy;
            vec4 p7 = tipProj - dx + dy;
            vec4 p8 = tipProj + dx + dy;
            
            // Other data to emit   
    
            // Emit the vertices as a triangle strip
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p6; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p1; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            value = edge_value[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex();
    
            EndPrimitive();

        }

    )
};

const ShaderStageSpecification CYLINDER_COLOR_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_radius", DataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;
        in vec4 position_tip[];
        in vec3 edge_color[];
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;
        out vec3 color;

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {

            // Build an orthogonal basis
            vec3 tailViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
            vec3 tipViewVal = position_tip[0].xyz / position_tip[0].w;
            vec3 cylDir = normalize(tipViewVal - tailViewVal);
            vec3 basisX; vec3 basisY; buildTangentBasis(cylDir, basisX, basisY);
  
            // Compute corners of cube
            vec4 tailProj = u_projMatrix * gl_in[0].gl_Position;
            vec4 tipProj = u_projMatrix * position_tip[0];
            vec4 dx = u_projMatrix * vec4(basisX * u_radius, 0.);
            vec4 dy = u_projMatrix * vec4(basisY * u_radius, 0.);

            vec4 p1 = tailProj - dx - dy;
            vec4 p2 = tailProj + dx - dy;
            vec4 p3 = tailProj - dx + dy;
            vec4 p4 = tailProj + dx + dy;
            vec4 p5 = tipProj - dx - dy;
            vec4 p6 = tipProj + dx - dy;
            vec4 p7 = tipProj - dx + dy;
            vec4 p8 = tipProj + dx + dy;
            
            // Other data to emit   
    
            // Emit the vertices as a triangle strip
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p6; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p1; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            color = edge_color[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex();
    
            EndPrimitive();

        }

    )
};

const ShaderStageSpecification CYLINDER_BLEND_VALUE_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_radius", DataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;
        in vec4 position_tip[];
        in float value_tail[];
        in float value_tip[];
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;
        out float valueTail;
        out float valueTip;

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {

            // Build an orthogonal basis
            vec3 tailViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
            vec3 tipViewVal = position_tip[0].xyz / position_tip[0].w;
            vec3 cylDir = normalize(tipViewVal - tailViewVal);
            vec3 basisX; vec3 basisY; buildTangentBasis(cylDir, basisX, basisY);
  
            // Compute corners of cube
            vec4 tailProj = u_projMatrix * gl_in[0].gl_Position;
            vec4 tipProj = u_projMatrix * position_tip[0];
            vec4 dx = u_projMatrix * vec4(basisX * u_radius, 0.);
            vec4 dy = u_projMatrix * vec4(basisY * u_radius, 0.);

            vec4 p1 = tailProj - dx - dy;
            vec4 p2 = tailProj + dx - dy;
            vec4 p3 = tailProj - dx + dy;
            vec4 p4 = tailProj + dx + dy;
            vec4 p5 = tipProj - dx - dy;
            vec4 p6 = tipProj + dx - dy;
            vec4 p7 = tipProj - dx + dy;
            vec4 p8 = tipProj + dx + dy;
            
            // Other data to emit   
    
            // Emit the vertices as a triangle strip
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p6; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p1; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            valueTail = value_tail[0]; valueTip = value_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex();
    
            EndPrimitive();

        }

    )
};

const ShaderStageSpecification CYLINDER_BLEND_COLOR_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_radius", DataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;
        in vec4 position_tip[];
        in vec3 color_tail[];
        in vec3 color_tip[];
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;
        out vec3 colorTail;
        out vec3 colorTip;

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {

            // Build an orthogonal basis
            vec3 tailViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
            vec3 tipViewVal = position_tip[0].xyz / position_tip[0].w;
            vec3 cylDir = normalize(tipViewVal - tailViewVal);
            vec3 basisX; vec3 basisY; buildTangentBasis(cylDir, basisX, basisY);
  
            // Compute corners of cube
            vec4 tailProj = u_projMatrix * gl_in[0].gl_Position;
            vec4 tipProj = u_projMatrix * position_tip[0];
            vec4 dx = u_projMatrix * vec4(basisX * u_radius, 0.);
            vec4 dy = u_projMatrix * vec4(basisY * u_radius, 0.);

            vec4 p1 = tailProj - dx - dy;
            vec4 p2 = tailProj + dx - dy;
            vec4 p3 = tailProj - dx + dy;
            vec4 p4 = tailProj + dx + dy;
            vec4 p5 = tipProj - dx - dy;
            vec4 p6 = tipProj + dx - dy;
            vec4 p7 = tipProj - dx + dy;
            vec4 p8 = tipProj + dx + dy;
            
            // Other data to emit   
    
            // Emit the vertices as a triangle strip
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p6; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p1; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex();
    
            EndPrimitive();

        }

    )
};

const ShaderStageSpecification CYLINDER_PICK_GEOM_SHADER = {
    
    ShaderStageType::Geometry,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_radius", DataType::Float},
    }, 

    // attributes
    {
    },

    {}, // textures

    // source
    POLYSCOPE_GLSL(150,
        layout(points) in;
        layout(triangle_strip, max_vertices=14) out;
        in vec4 position_tip[];
        in vec3 color_tip[];
        in vec3 color_tail[];
        in vec3 color_edge[];
        uniform mat4 u_projMatrix;
        uniform float u_radius;
        out vec3 tipView;
        out vec3 tailView;
        flat out vec3 colorTail;
        flat out vec3 colorTip;
        flat out vec3 colorEdge;

        void buildTangentBasis(vec3 unitNormal, out vec3 basisX, out vec3 basisY);

        void main() {

            // Build an orthogonal basis
            vec3 tailViewVal = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;
            vec3 tipViewVal = position_tip[0].xyz / position_tip[0].w;
            vec3 cylDir = normalize(tipViewVal - tailViewVal);
            vec3 basisX; vec3 basisY; buildTangentBasis(cylDir, basisX, basisY);
  
            // Compute corners of cube
            vec4 tailProj = u_projMatrix * gl_in[0].gl_Position;
            vec4 tipProj = u_projMatrix * position_tip[0];
            vec4 dx = u_projMatrix * vec4(basisX * u_radius, 0.);
            vec4 dy = u_projMatrix * vec4(basisY * u_radius, 0.);

            vec4 p1 = tailProj - dx - dy;
            vec4 p2 = tailProj + dx - dy;
            vec4 p3 = tailProj - dx + dy;
            vec4 p4 = tailProj + dx + dy;
            vec4 p5 = tipProj - dx - dy;
            vec4 p6 = tipProj + dx - dy;
            vec4 p7 = tipProj - dx + dy;
            vec4 p8 = tipProj + dx + dy;
            
            // Other data to emit   
    
            // Emit the vertices as a triangle strip
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p6; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p8; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p7; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p5; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p1; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p2; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p3; EmitVertex(); 
            colorTail = color_tail[0]; colorTip = color_tip[0]; colorEdge = color_edge[0]; tailView = tailViewVal; tipView = tipViewVal; gl_Position = p4; EmitVertex();
    
            EndPrimitive();

        }

    )
};


const ShaderStageSpecification CYLINDER_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_invProjMatrix", DataType::Matrix44Float},
        {"u_viewport", DataType::Vector4Float},
        {"u_radius", DataType::Float},
        {"u_baseColor", DataType::Vector3Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
        uniform mat4 u_projMatrix;
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        uniform float u_radius;
        uniform vec3 u_baseColor;
        in vec3 tailView;
        in vec3 tipView;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler2D t_mat_k;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           // Raycast to the sphere 
           float tHit;
           vec3 pHit;
           vec3 nHit;
           rayCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, tipView, u_radius, tHit, pHit, nHit);
           if(tHit >= LARGE_FLOAT()) {
              discard;
           }

           // Lighting
           outputF = vec4(lightSurfaceMat(nHit, u_baseColor, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);

           // Set depth (expensive!)
           float depth = fragDepthFromView(u_projMatrix, depthRange, pHit);
           gl_FragDepth = depth;
        }
    )
};

const ShaderStageSpecification CYLINDER_VALUE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_invProjMatrix", DataType::Matrix44Float},
        {"u_viewport", DataType::Vector4Float},
        {"u_radius", DataType::Float},
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
        uniform mat4 u_projMatrix;
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        uniform float u_radius;
        uniform float u_rangeHigh;
        uniform float u_rangeLow;
        in vec3 tailView;
        in vec3 tipView;
        in float value;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler2D t_mat_k;
        uniform sampler1D t_colormap;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);
        
        vec3 surfaceColor() {
          float t = (value - u_rangeLow) / (u_rangeHigh - u_rangeLow);
          t = clamp(t, 0.f, 1.f);
          return texture(t_colormap, t).rgb;
        }

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           // Raycast to the sphere 
           float tHit;
           vec3 pHit;
           vec3 nHit;
           rayCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, tipView, u_radius, tHit, pHit, nHit);
           if(tHit >= LARGE_FLOAT()) {
              discard;
           }

           // Lighting
           outputF = vec4(lightSurfaceMat(nHit, surfaceColor(), t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);

           // Set depth (expensive!)
           float depth = fragDepthFromView(u_projMatrix, depthRange, pHit);
           gl_FragDepth = depth;
        }
    )
};

const ShaderStageSpecification CYLINDER_COLOR_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_invProjMatrix", DataType::Matrix44Float},
        {"u_viewport", DataType::Vector4Float},
        {"u_radius", DataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
        uniform mat4 u_projMatrix;
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        uniform float u_radius;
        in vec3 tailView;
        in vec3 tipView;
        in vec3 color;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler2D t_mat_k;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           // Raycast to the sphere 
           float tHit;
           vec3 pHit;
           vec3 nHit;
           rayCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, tipView, u_radius, tHit, pHit, nHit);
           if(tHit >= LARGE_FLOAT()) {
              discard;
           }

           // Lighting
           outputF = vec4(lightSurfaceMat(nHit, color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);

           // Set depth (expensive!)
           float depth = fragDepthFromView(u_projMatrix, depthRange, pHit);
           gl_FragDepth = depth;
        }
    )
};


const ShaderStageSpecification CYLINDER_BLEND_VALUE_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_invProjMatrix", DataType::Matrix44Float},
        {"u_viewport", DataType::Vector4Float},
        {"u_radius", DataType::Float},
        {"u_rangeLow", DataType::Float},
        {"u_rangeHigh", DataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
        {"t_colormap", 1}
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
        uniform mat4 u_projMatrix;
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        uniform float u_radius;
        uniform float u_rangeHigh;
        uniform float u_rangeLow;
        in vec3 tailView;
        in vec3 tipView;
        in float valueTail;
        in float valueTip;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler2D t_mat_k;
        uniform sampler1D t_colormap;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);
        float length2(vec3 x);
        
        vec3 surfaceColor(float value) {
          float t = (value - u_rangeLow) / (u_rangeHigh - u_rangeLow);
          t = clamp(t, 0.f, 1.f);
          return texture(t_colormap, t).rgb;
        }

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           // Raycast to the sphere 
           float tHit;
           vec3 pHit;
           vec3 nHit;
           rayCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, tipView, u_radius, tHit, pHit, nHit);
           if(tHit >= LARGE_FLOAT()) {
              discard;
           }
           
           // Compute distance along edge
           float tEdge = dot(pHit - tailView, tipView - tailView) / length2(tipView - tailView);
           float value = mix(valueTail, valueTip, tEdge);

           // Lighting
           outputF = vec4(lightSurfaceMat(nHit, surfaceColor(value), t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);

           // Set depth (expensive!)
           float depth = fragDepthFromView(u_projMatrix, depthRange, pHit);
           gl_FragDepth = depth;
        }
    )
};

const ShaderStageSpecification CYLINDER_BLEND_COLOR_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_invProjMatrix", DataType::Matrix44Float},
        {"u_viewport", DataType::Vector4Float},
        {"u_radius", DataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
        {"t_mat_r", 2},
        {"t_mat_g", 2},
        {"t_mat_b", 2},
        {"t_mat_k", 2},
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
        uniform mat4 u_projMatrix;
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        uniform float u_radius;
        in vec3 tailView;
        in vec3 tipView;
        in vec3 colorTail;
        in vec3 colorTip;
        uniform sampler2D t_mat_r;
        uniform sampler2D t_mat_g;
        uniform sampler2D t_mat_b;
        uniform sampler2D t_mat_k;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b, sampler2D t_mat_k);
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);
        float length2(vec3 x);
        
        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           // Raycast to the sphere 
           float tHit;
           vec3 pHit;
           vec3 nHit;
           rayCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, tipView, u_radius, tHit, pHit, nHit);
           if(tHit >= LARGE_FLOAT()) {
              discard;
           }
           
           // Compute distance along edge
           float tEdge = dot(pHit - tailView, tipView - tailView) / length2(tipView - tailView);
           vec3 color = mix(colorTail, colorTip, tEdge);

           // Lighting
           outputF = vec4(lightSurfaceMat(nHit, color, t_mat_r, t_mat_g, t_mat_b, t_mat_k), 1.);

           // Set depth (expensive!)
           float depth = fragDepthFromView(u_projMatrix, depthRange, pHit);
           gl_FragDepth = depth;
        }
    )
};


const ShaderStageSpecification CYLINDER_PICK_FRAG_SHADER = {
    
    ShaderStageType::Fragment,
    
    // uniforms
    {
        {"u_projMatrix", DataType::Matrix44Float},
        {"u_invProjMatrix", DataType::Matrix44Float},
        {"u_viewport", DataType::Vector4Float},
        {"u_radius", DataType::Float},
    }, 

    { }, // attributes
    
    // textures 
    {
    },
 
    // source
    POLYSCOPE_GLSL(330 core,
        uniform mat4 u_projMatrix;
        uniform mat4 u_invProjMatrix;
        uniform vec4 u_viewport;
        uniform float u_radius;
        in vec3 tailView;
        in vec3 tipView;
        flat in vec3 colorTail;
        flat in vec3 colorTip;
        flat in vec3 colorEdge;
        layout(location = 0) out vec4 outputF;

        float LARGE_FLOAT();
        vec3 fragmentViewPosition(vec4 viewport, vec2 depthRange, mat4 invProjMat, vec4 fragCoord);
        bool rayCylinderIntersection(vec3 rayStart, vec3 rayDir, vec3 cylTail, vec3 cylTip, float cylRad, out float tHit, out vec3 pHit, out vec3 nHit);
        float fragDepthFromView(mat4 projMat, vec2 depthRange, vec3 viewPoint);
        float length2(vec3 x);

        void main()
        {
           // Build a ray corresponding to this fragment
           vec2 depthRange = vec2(gl_DepthRange.near, gl_DepthRange.far);
           vec3 viewRay = fragmentViewPosition(u_viewport, depthRange, u_invProjMatrix, gl_FragCoord);

           // Raycast to the sphere 
           float tHit;
           vec3 pHit;
           vec3 nHit;
           rayCylinderIntersection(vec3(0., 0., 0), viewRay, tailView, tipView, u_radius, tHit, pHit, nHit);
           if(tHit >= LARGE_FLOAT()) {
              discard;
           }

           // Compute distance along edge
           float tEdge = dot(pHit - tailView, tipView - tailView) / length2(tipView - tailView);

           float endWidth = 0.2;
           vec3 myColor;
           if(tEdge < endWidth) {
             myColor = colorTail;
           } else if (tEdge < (1.0f - endWidth)) {
             myColor = colorEdge;
           } else {
             myColor = colorTip;
           }
           outputF = vec4(myColor, 1.f);

           // Set depth (expensive!)
           float depth = fragDepthFromView(u_projMatrix, depthRange, pHit);
           gl_FragDepth = depth;
        }
    )
};



// clang-format on

} // namespace render
} // namespace polyscope
