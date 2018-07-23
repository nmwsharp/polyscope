#pragma once

static const VertShader GROUND_PLANE_VERT_SHADER =  {
    
    // uniforms
    {
       {"u_viewMatrix", GLData::Matrix44Float},
       {"u_projMatrix", GLData::Matrix44Float},
       {"u_groundHeight", GLData::Float},
    },

    // attributes
    {
        {"a_position", GLData::Vector4Float},
    },

    // source
    GLSL(150,
      uniform mat4 u_viewMatrix;
      uniform mat4 u_projMatrix;
      uniform float u_groundHeight;
      in vec4 a_position;
      out vec3 Normal;
      out vec4 PositionWorldHomog;

      void main()
      {
          vec4 adjustedPosition = a_position + vec4(0., u_groundHeight * a_position.w, 0., 0.);
          Normal = mat3(u_viewMatrix) * vec3(0., 1., 0.);
          gl_Position = u_projMatrix * u_viewMatrix * adjustedPosition;
          PositionWorldHomog = adjustedPosition;
      }
    )
};

static const FragShader GROUND_PLANE_FRAG_SHADER = {
    
    // uniforms
    {
      {"u_lengthScale", GLData::Float},
      {"u_centerXZ", GLData::Vector2Float},
      {"u_viewportDim", GLData::Vector2Float},
    }, 

    // attributes
    {
    },
    
    // textures 
    {
        {"t_ground", 2},
        {"t_mirrorImage", 2},
    },
    
    // output location
    "outputF",
    
    // source 
    GLSL(150,

      uniform sampler2D t_ground;
      uniform sampler2D t_mirrorImage;
      uniform mat4 u_viewMatrix;
      uniform float u_lengthScale;
      uniform vec2 u_centerXZ;
      uniform vec2 u_viewportDim;
      in vec3 Normal;
      in vec4 PositionWorldHomog;
      out vec4 outputF;

      vec4 lightSurfaceMat(vec3 normal, vec3 color, sampler2D t_mat_r, sampler2D t_mat_g, sampler2D t_mat_b);
      float orenNayarDiffuse( vec3 lightDirection, vec3 viewDirection, vec3 surfaceNormal, float roughness, float albedo);
      float specular( vec3 N, vec3 L, vec3 E, float shininess );
      float getEdgeFactor(vec2 UV);

      vec4 blurMirrorSample() {
        //vec2 screenCoords = vec2(gl_FragCoord.x / u_viewportDim.x, gl_FragCoord.y / u_viewportDim.y);
        vec2 screenCoords = vec2(gl_FragCoord.x, gl_FragCoord.y);

        vec4 mirrorImage =
          texture(t_mirrorImage, screenCoords / u_viewportDim) * .6 + 
          texture(t_mirrorImage, (screenCoords + vec2(+1.0, +1.0)) / u_viewportDim) * .1 + 
          texture(t_mirrorImage, (screenCoords + vec2(+1.0, -1.0)) / u_viewportDim) * .1 + 
          texture(t_mirrorImage, (screenCoords + vec2(-1.0, +1.0)) / u_viewportDim) * .1 + 
          texture(t_mirrorImage, (screenCoords + vec2(-1.0, -1.0)) / u_viewportDim) * .1;

        return mirrorImage;
      }

      void main()
      {
        vec2 coordXZ = PositionWorldHomog.xz / PositionWorldHomog.w - u_centerXZ;
        coordXZ /= u_lengthScale * .5;

        // Checker stripes
        float modDist = min(min(mod(coordXZ.x, 1.0), mod(coordXZ.y, 1.0)), min(mod(-coordXZ.x, 1.0), mod(-coordXZ.y, 1.0)));
        float stripeBlendFac = smoothstep(0.005, .01, modDist);
        vec4 baseColor = mix(texture(t_ground, coordXZ), vec4(.88, .88, .88, 1.), .6); 
        vec4 groundColor = mix( vec4(baseColor.xyz * .2, 1.0), baseColor, stripeBlendFac);

        // Mirror image
        //vec2 screenCoords = vec2(gl_FragCoord.x / u_viewportDim.x, gl_FragCoord.y / u_viewportDim.y);
        //vec4 mirrorImage = texture(t_mirrorImage, screenCoords);
        vec4 mirrorImage = blurMirrorSample();

        // Ground color
        vec3 color3 = mix(groundColor.rgb, mirrorImage.rgb * mirrorImage.w, .2 * mirrorImage.w);

        // Fade off far away
        float distFromCenter = length(coordXZ);
        float fadeFactor = 1.0 - smoothstep(8.0, 8.5, distFromCenter);
        vec4 color = vec4(color3, fadeFactor);
      
        // Lighting stuff
        vec4 posCameraSpace4 = u_viewMatrix * PositionWorldHomog;
        vec3 posCameraSpace = posCameraSpace4.xyz / posCameraSpace4.w;
        vec3 normalCameraSpace = mat3(u_viewMatrix) * vec3(0., 1., 0.);
        vec3 eyeCameraSpace = vec3(0., 0., 0.);
        vec3 lightPosCameraSpace = vec3(5., 5., -5.) * u_lengthScale;
        vec3 lightDir = normalize(lightPosCameraSpace - posCameraSpace);
        vec3 eyeDir = normalize(eyeCameraSpace - posCameraSpace);

        float coloredBrightness = 1.2 *orenNayarDiffuse(lightDir, eyeDir, normalCameraSpace, .2, 1.0) + .5;
        float whiteBrightness = .25 * specular(normalCameraSpace, lightDir, eyeDir, 12.);

        vec4 lightColor = vec4(color.xyz * coloredBrightness + vec3(1., 1., 1.) * whiteBrightness, color.w);
        
        outputF = lightColor;
      }

    )
};
