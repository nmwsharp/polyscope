// Copyright 2017-2019, Nicholas Sharp and the Polyscope contributors. http://polyscope.run.
#pragma once

#include "polyscope/gl/shaders.h"

namespace polyscope {
namespace gl {

// clang-format off

static const VertShader RAY_VERT_SHADER = {
    // uniforms
    {
       {"u_modelView", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
    }, 

    // attributes
    {
        {"a_position", GLData::Vector3Float},
        {"a_tp", GLData::Float},
        {"a_offset", GLData::Float},
    },

    // source
    POLYSCOPE_GLSL(150,
        in vec3 a_position;
        in float a_tp;
        in float a_offset;
      
        uniform mat4 u_modelView;
        uniform mat4 u_projMatrix;

        out float tp;
        out float offset;

        void main()
        {
            gl_Position = u_projMatrix * u_modelView * vec4(a_position,1.);
            tp = a_tp;
            offset = a_offset;
        }
    )
};

static const GeomShader RAY_GEOM_SHADER = {
    
    // uniforms
    {
        {"u_time", GLData::Float},
        {"u_timeMax", GLData::Float},
        {"u_streakWidth", GLData::Float},
    }, 

    // attributes
    {
    },

    // source
    POLYSCOPE_GLSL(150,
        layout(lines) in;
        layout(line_strip, max_vertices=2) out;
        
        in float tp[];
        in float offset[];

        uniform float u_time;
        uniform float u_timeMax;
        uniform float u_streakWidth;

        out float tp2;
        out float offset2;

        bool intervalsIntersect(float a_start, float a_end, float b_start, float b_end) {
            // a strictly after b || b strctly after a
            // --> no intersection
            if(a_start > b_end || b_start > a_end) {
                return false;
            }

            // otherwise, intersection
            return true;
        }

        void main()   {

            // == Test to see if some point in this segement will be active shaded
            bool willShade = false;

            float myTStart = mod(tp[0] + offset[0] * u_timeMax, u_timeMax);
            float myTEnd = mod(tp[1] + offset[1] * u_timeMax, u_timeMax);
            float myTLen = tp[1] - tp[0];

            // Always shade if more than one interval long
            if(myTLen > u_timeMax) {
                willShade = true;
            }

            // The usual streak
            float streakStart = u_time;
            float streakEnd = u_time + u_streakWidth;

            // A second streak "before" the main one, used to handle the
            // case where the streak wraps around
            float prestreakStart = u_time - u_timeMax;
            float prestreakEnd = prestreakStart + u_streakWidth;

            // The "standard" case, where the interval of this segment is contiguous
            if(myTEnd > myTStart) {
                willShade = willShade || intervalsIntersect(myTStart, myTEnd, streakStart, streakEnd);
                willShade = willShade || intervalsIntersect(myTStart, myTEnd, prestreakStart, prestreakEnd);
            } 
            // The case wehre the interval is split across the time mod boundary 
            else {
                willShade = willShade || intervalsIntersect(0.f, myTStart, streakStart, streakEnd);
                willShade = willShade || intervalsIntersect(0.f, myTStart, prestreakStart, prestreakEnd);
                willShade = willShade || intervalsIntersect(myTEnd, u_timeMax, streakStart, streakEnd);
                willShade = willShade || intervalsIntersect(myTEnd, u_timeMax, prestreakStart, prestreakEnd);
            }

            // Only pass the line through if some point will be shaded
            if(willShade) {
                
                gl_Position = gl_in[0].gl_Position;
                tp2 = tp[0];
                offset2 = offset[0];
                EmitVertex();
                
                gl_Position = gl_in[1].gl_Position;
                tp2 = tp[1];
                offset2 = offset[1];
                EmitVertex();

                EndPrimitive(); 
            }

        }
    )
};

static const FragShader RAY_FRAG_SHADER = {
    
    // uniforms
    {
        {"u_color", GLData::Vector3Float},
        {"u_time", GLData::Float},
        {"u_timeMax", GLData::Float},
        {"u_streakWidth", GLData::Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
    },
    
    // output location
    "outputF",
 
    // source
    POLYSCOPE_GLSL(150,
        in float tp2;
        in float offset2;
    
        uniform vec3 u_color;
        uniform float u_time;
        uniform float u_timeMax;
        uniform float u_streakWidth;

        out vec4 outputF;

        void main()
        {

            // Parameter for this point, after variable offset is applied
            float myT = mod(tp2 + offset2 * u_timeMax, u_timeMax);

            // Compute how far "ahead" of the current time this point is
            float leadTime = myT - u_time;
            if(leadTime < 0.f) {
                leadTime += u_timeMax;
            }

            // If we're outside the streak, don't draw anything
            if(leadTime > u_streakWidth) {
                discard;
            }

            // White at the tip, colored tail
            float leadT = leadTime / u_streakWidth;
            vec3 color = mix(u_color, vec3(1.f,1.f,1.f), leadT*leadT*leadT);

            outputF = vec4(color, 1.f);
        }
    )
};

// clang-format on

} // namespace gl
} // namespace polyscope
