#include <gl_utils.h>
#include <shaders/common.h>

#include <stdexcept>

using std::cout; using std::endl;

GLuint GLProgram::commonShaderHandle = 0;

GLProgram::GLProgram(const VertShader* vShader, const FragShader* fShader, DrawMode dm) 
    : GLProgram(vShader, nullptr, nullptr, nullptr, fShader, dm, 0)
{    
}

GLProgram::GLProgram(const VertShader* vShader, const GeomShader* gShader, const FragShader* fShader, DrawMode dm) 
    : GLProgram(vShader, nullptr, nullptr, gShader, fShader, dm, 0)
{    
}

GLProgram::GLProgram(const VertShader* vShader, const TessShader* tShader, const FragShader* fShader, DrawMode dm, int patchVertices) 
    : GLProgram(vShader, tShader, nullptr, nullptr, fShader, dm, patchVertices)
{    
}

GLProgram::GLProgram(const VertShader* vShader, const EvalShader* eShader, const FragShader* fShader, DrawMode dm, int patchVertices) 
    : GLProgram(vShader, nullptr, eShader, nullptr, fShader, dm, patchVertices)
{    
}

GLProgram::GLProgram(const VertShader* vShader, const TessShader* tShader, const EvalShader* eShader, const FragShader* fShader, DrawMode dm, int patchVertices) 
    : GLProgram(vShader, tShader, eShader, nullptr, fShader, dm, patchVertices)
{    
}

GLProgram::GLProgram(const VertShader* vShader,
                     const TessShader* tShader,
                     const EvalShader* eShader,
                     const GeomShader* gShader,
                     const FragShader* fShader,
                     DrawMode dm,
                     int patchVertices) {

    vertShader = vShader;
    tessShader = tShader;
    evalShader = eShader;
    geomShader = gShader;
    fragShader = fShader;

    GLint maxPatchVertices;
    glGetIntegerv( GL_MAX_PATCH_VERTICES, &maxPatchVertices );
    if( patchVertices > maxPatchVertices )
    {
       throw std::invalid_argument( "Requested number of patch vertices (" +
                                    std::to_string(patchVertices) +
                                    ") is greater than the number supported by the tessellator (" +
                                    std::to_string(maxPatchVertices) );
    }

    nPatchVertices = patchVertices;

    drawMode = dm;
    if(dm == DrawMode::IndexedLines ||
       dm == DrawMode::IndexedLineStrip ||
       dm == DrawMode::IndexedLineStripAdjacency ||
       dm == DrawMode::IndexedTriangles) {
        useIndex = true;
    }

    // Collect attributes and uniforms from all of the shaders

    // Vertex shader
    for(ShaderUniform u : vertShader->uniforms) {
        addUniqueUniform(u);
    }
    for(ShaderAttribute a : vertShader->attributes) {
        addUniqueAttribute(a);
    }

    // Tessellation control shader
    if(tessShader != nullptr) {
        for(ShaderUniform u : tessShader->uniforms) {
            addUniqueUniform(u);
        }
        for(ShaderAttribute a : tessShader->attributes) {
            addUniqueAttribute(a);
        }
    }

    // Tessellation evaluation shader
    if(evalShader != nullptr) {
        for(ShaderUniform u : evalShader->uniforms) {
            addUniqueUniform(u);
        }
        for(ShaderAttribute a : evalShader->attributes) {
            addUniqueAttribute(a);
        }
    }

    // Geometry shader
    if(geomShader != nullptr) {
        for(ShaderUniform u : geomShader->uniforms) {
            addUniqueUniform(u);
        }
        for(ShaderAttribute a : geomShader->attributes) {
            addUniqueAttribute(a);
        }
    }

    // Fragment shader
    for(ShaderUniform u : fragShader->uniforms) {
        addUniqueUniform(u);
    }
    for(ShaderAttribute a : fragShader->attributes) {
        addUniqueAttribute(a);
    }
    for(ShaderTexture t : fragShader->textures) {
        addUniqueTexture(t);
    }


    if(attributes.size() == 0) {
        throw std::invalid_argument("Uh oh... GLProgram has no attributes");
    }


    // Perform setup tasks
    compileGLProgram();
    setDataLocations();
    createBuffers();
}

GLProgram::~GLProgram() {

    // Make sure that we free the buffers for all attributes
    for(GLAttribute a : attributes) {
        deleteAttributeBuffer(a);
    }
    for(GLTexture t : textures) {
        freeTexture(t);
    }

    // Free the program
    glDeleteProgram(programHandle);
}

void GLProgram::addUniqueAttribute(ShaderAttribute newAttribute) {
    for(GLAttribute& a : attributes) {
        if(a.name == newAttribute.name && a.type == newAttribute.type) {
            return;
        }
    }
    attributes.push_back(GLAttribute{newAttribute.name, newAttribute.type, 777, 777, -1});
}

void GLProgram::addUniqueUniform(ShaderUniform newUniform) {
    for(GLUniform& u : uniforms) {
        if(u.name == newUniform.name && u.type == newUniform.type) {
            return;
        }
    }
    uniforms.push_back(GLUniform{newUniform.name, newUniform.type, 777, false});
}

void GLProgram::addUniqueTexture(ShaderTexture newTexture) {
    for(GLTexture& t : textures) {
        if(t.name == newTexture.name && t.dim == newTexture.dim) {
            return;
        }
    }
    textures.push_back(GLTexture{newTexture.name, newTexture.dim, 777, 777, 777, false});
}

void GLProgram::deleteAttributeBuffer(GLAttribute attribute) {
    glUseProgram(programHandle);
    glBindVertexArray(vaoHandle);
    glDeleteBuffers(1, &attribute.VBOLoc); 
}

void GLProgram::freeTexture(GLTexture t) {
    glDeleteTextures(1, &(t.bufferLoc)); 
}

void GLProgram::compileGLProgram() {

    // Compile the vertex shader
    vertShaderHandle = glCreateShader(GL_VERTEX_SHADER);
    const char *vertShaderTmp = vertShader->src.c_str();
    glShaderSource(vertShaderHandle, 1, &vertShaderTmp, nullptr);
    glCompileShader(vertShaderHandle);
    printShaderInfoLog(vertShaderHandle);

    // Compile the tessellation control shader
    if(tessShader != nullptr) {
        tessShaderHandle = glCreateShader(GL_TESS_CONTROL_SHADER);
        const char *tessShaderTmp = tessShader->src.c_str();
        glShaderSource(tessShaderHandle, 1, &tessShaderTmp, nullptr);
        glCompileShader(tessShaderHandle);
        printShaderInfoLog(tessShaderHandle);
    }

    // Compile the tessellation evaluation shader
    if(evalShader != nullptr) {
        evalShaderHandle = glCreateShader(GL_TESS_EVALUATION_SHADER);
        const char *evalShaderTmp = evalShader->src.c_str();
        glShaderSource(evalShaderHandle, 1, &evalShaderTmp, nullptr);
        glCompileShader(evalShaderHandle);
        printShaderInfoLog(evalShaderHandle);
    }

    // Compile the geometry shader
    if(geomShader != nullptr) {
        geomShaderHandle = glCreateShader(GL_GEOMETRY_SHADER);
        const char *geomShaderTmp = geomShader->src.c_str();
        glShaderSource(geomShaderHandle, 1, &geomShaderTmp, nullptr);
        glCompileShader(geomShaderHandle);
        printShaderInfoLog(geomShaderHandle);
    }

    // Compile the fragment shader
    fragShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragShaderTmp = fragShader->src.c_str();
    glShaderSource(fragShaderHandle, 1, &fragShaderTmp, nullptr);
    glCompileShader(fragShaderHandle);
    printShaderInfoLog(fragShaderHandle);


    // Create the program and attach the shaders
    programHandle = glCreateProgram();
    glAttachShader(programHandle, vertShaderHandle);
    if(tessShader != nullptr) {
        glAttachShader(programHandle, tessShaderHandle);
    }
    if(evalShader != nullptr) {
        glAttachShader(programHandle, evalShaderHandle);
    }
    if(geomShader != nullptr) {
        glAttachShader(programHandle, geomShaderHandle);
    }
    glAttachShader(programHandle, commonShaderHandle);
    glAttachShader(programHandle, fragShaderHandle);

    
    // Set the output data location
    glBindFragDataLocation(programHandle, 0, fragShader->outputLoc.c_str()); 


    // Link the program
    glLinkProgram(programHandle);
    printProgramInfoLog(programHandle);

    // Delete the shaders we just compiled, they aren't used after link 
    glDeleteShader(vertShaderHandle);
    if(tessShader != nullptr) {
        glDeleteShader(tessShaderHandle);
    }
    if(evalShader != nullptr) {
        glDeleteShader(evalShaderHandle);
    }
    if(geomShader != nullptr) {
        glDeleteShader(geomShaderHandle);
    }
    glDeleteShader(fragShaderHandle);
}

void GLProgram::setDataLocations() {

    glUseProgram(programHandle);

    // Uniforms
    for(GLUniform& u : uniforms) {
        u.location = glGetUniformLocation(programHandle, u.name.c_str());
    }

    // Attributes
    for(GLAttribute& a : attributes) {
        a.location = glGetAttribLocation(programHandle, a.name.c_str());
    }
    
    // Textures 
    for(GLTexture& t : textures) {
        t.location = glGetUniformLocation(programHandle, t.name.c_str());
    }

}

void GLProgram::createBuffers() {
    
    // Create a VAO
    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    // Create buffers for each attributes
    for(GLAttribute& a : attributes) {
        glGenBuffers(1, &a.VBOLoc);
        glBindBuffer(GL_ARRAY_BUFFER, a.VBOLoc);
        glEnableVertexAttribArray(a.location);

        // Choose the correct type for the buffer
        switch(a.type) {
            case GLData::Float:
                glVertexAttribPointer(a.location, 1, GL_FLOAT, 0, 0, 0);
                break;
            case GLData::Int:
                glVertexAttribPointer(a.location, 1, GL_INT, 0, 0, 0);
                break;
            case GLData::Vector2Float:
                glVertexAttribPointer(a.location, 2, GL_FLOAT, 0, 0, 0);
                break;
            case GLData::Vector3Float:
                glVertexAttribPointer(a.location, 3, GL_FLOAT, 0, 0, 0);
                break;
            default:
                throw std::invalid_argument("Unrecognized GLAttribute type");
                break;
        }
    }

    // Create an index buffer, if we're using one
    if(useIndex) {
        glGenBuffers(1, &indexVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
    }

    // === Generate textures

    // Verify we have enough texture units
    GLint nAvailTextureUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &nAvailTextureUnits);
    if((int)textures.size() > nAvailTextureUnits) {
        throw std::invalid_argument("Attempted to load more textures than the number of available texture units (" + std::to_string(nAvailTextureUnits) + ").");
    }

    // Create texture buffers for each
    for(unsigned int iTexture = 0; iTexture < textures.size(); iTexture++) {
        GLTexture& t = textures[iTexture];
        glGenTextures(1, &(t.bufferLoc));
        t.index = iTexture;
    }
}

// Set an integer
void GLProgram::setUniform(std::string name, int val) {
    
    glUseProgram(programHandle);

    for(GLUniform& u : uniforms) {
        if(u.name == name) {
            if(u.type == GLData::Int) {
                glUniform1i(u.location, val);
                u.isSet = true;
            } else {
                throw std::invalid_argument("Tried to set GLUniform with wrong type");
            }
            return;
        }
    }
    throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}


// Set an unsigned integer
void GLProgram::setUniform(std::string name, unsigned int val) {
    
    glUseProgram(programHandle);

    for(GLUniform& u : uniforms) {
        if(u.name == name) {
            if(u.type == GLData::UInt) {
                glUniform1ui(u.location, val);
                u.isSet = true;
            } else {
                throw std::invalid_argument("Tried to set GLUniform with wrong type");
            }
            return;
        }
    }
    throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a float
void GLProgram::setUniform(std::string name, float val) {
    
    glUseProgram(programHandle);

    for(GLUniform& u : uniforms) {
        if(u.name == name) {
            if(u.type == GLData::Float) {
                glUniform1f(u.location, val);
                u.isSet = true;
            } else {
                throw std::invalid_argument("Tried to set GLUniform with wrong type");
            }
            return;
        }
    }
    throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a double --- WARNING casts down to float
void GLProgram::setUniform(std::string name, double val) {
    
    glUseProgram(programHandle);

    for(GLUniform& u : uniforms) {
        if(u.name == name) {
            if(u.type == GLData::Float) {
                glUniform1f(u.location, static_cast<float>(val));
                u.isSet = true;
            } else {
                throw std::invalid_argument("Tried to set GLUniform with wrong type");
            }
            return;
        }
    }
    throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a 4x4 uniform matrix
void GLProgram::setUniform(std::string name, float* val) {
    
    glUseProgram(programHandle);

    for(GLUniform& u : uniforms) {
        if(u.name == name) {
            if(u.type == GLData::Matrix44Float) {
                glUniformMatrix4fv(u.location, 1, false, val);
                u.isSet = true;
            } else {
                throw std::invalid_argument("Tried to set GLUniform with wrong type");
            }
            return;
        }
    }
    throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}


// Set a vector2 uniform
void GLProgram::setUniform(std::string name, Vector2 val) {
    
    glUseProgram(programHandle);
    
    for(GLUniform& u : uniforms) {
        if(u.name == name) {
            if(u.type == GLData::Vector2Float) {
                glUniform2f(u.location, val.x, val.y);
                u.isSet = true;
            } else {
                throw std::invalid_argument("Tried to set GLUniform with wrong type");
            }
            return;
        }
    }
    throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a vector3 uniform
void GLProgram::setUniform(std::string name, Vector3 val) {
    
    glUseProgram(programHandle);
   
    for(GLUniform& u : uniforms) {
        if(u.name == name) {
            if(u.type == GLData::Vector3Float) {
                glUniform3f(u.location, val.x, val.y, val.z);
                u.isSet = true;
            } else {
                throw std::invalid_argument("Tried to set GLUniform with wrong type");
            }
            return;
        }
    }
    throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

// Set a vec4 uniform
void GLProgram::setUniform(std::string name, float x, float y, float z, float w) {
    
    glUseProgram(programHandle);
    
    for(GLUniform& u : uniforms) {
        if(u.name == name) {
            if(u.type == GLData::Vector4Float) {
                glUniform4f(u.location, x, y, z, w);
                u.isSet = true;
            } else {
                throw std::invalid_argument("Tried to set GLUniform with wrong type");
            }
            return;
        }
    }
    throw std::invalid_argument("Tried to set nonexistent uniform with name " + name);
}

void GLProgram::setAttribute(std::string name, const std::vector<Vector2> &data, bool update,
                             int offset, int size) {

    // Reshape the vector
    // Right now, the data is probably laid out in this form already... but let's not be overly clever
    // and just reshape it.
   std::vector<float> rawData(2*data.size());
    for(unsigned int i = 0; i < data.size(); i++) {
       rawData[2*i + 0] = static_cast<float>(data[i].x);
       rawData[2*i + 1] = static_cast<float>(data[i].y);
    }

    for(GLAttribute& a : attributes) {
        if(a.name == name) {
            if(a.type == GLData::Vector2Float) {
                glBindVertexArray(vaoHandle);
                glBindBuffer(GL_ARRAY_BUFFER, a.VBOLoc);
                if( update ) {
                   // TODO: Allow modifications to non-contiguous memory
                   offset *= 2 * sizeof(float);
                   if (size == -1) size = 2 * a.dataSize * sizeof(float);
                   else size *= 2 * sizeof(float);
                    
                   glBufferSubData(GL_ARRAY_BUFFER, offset, size, &rawData[0]);
                }
                else {
                   glBufferData(GL_ARRAY_BUFFER, 2*data.size()*sizeof(float), &rawData[0], GL_STATIC_DRAW);
                   a.dataSize = data.size();
                }
            } else {
                throw std::invalid_argument("Tried to set GLAttribute named " + name + " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) + "  Attempted type: " + std::to_string(static_cast<int>(GLData::Vector2Float)));
            }
            return;
        }
    }

    throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}

void GLProgram::setAttribute(std::string name, const std::vector<Vector3> &data, bool update,
                             int offset, int size) {

    // Reshape the vector
    // Right now, the data is probably laid out in this form already... but let's not be overly clever
    // and just reshape it.
   std::vector<float> rawData(3*data.size());
    for(unsigned int i = 0; i < data.size(); i++) {
        rawData[3*i + 0] = static_cast<float>(data[i].x);
        rawData[3*i + 1] = static_cast<float>(data[i].y);
        rawData[3*i + 2] = static_cast<float>(data[i].z);
    }

    for(GLAttribute& a : attributes) {
        if(a.name == name) {
            if(a.type == GLData::Vector3Float) {
                glBindVertexArray(vaoHandle);
                glBindBuffer(GL_ARRAY_BUFFER, a.VBOLoc);
                if( update ) {
                   // TODO: Allow modifications to non-contiguous memory
                   offset *= 3 * sizeof(float);
                   if (size == -1) size = 3 * a.dataSize * sizeof(float);
                   else size *= 3 * sizeof(float);
                        
                   glBufferSubData(GL_ARRAY_BUFFER, offset, size, &rawData[0]);
                }
                else {
                   glBufferData(GL_ARRAY_BUFFER, 3*data.size()*sizeof(float), &rawData[0], GL_STATIC_DRAW);
                   a.dataSize = data.size();
                }
            } else {
                throw std::invalid_argument("Tried to set GLAttribute named " + name + " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) + "  Attempted type: " + std::to_string(static_cast<int>(GLData::Vector3Float)));
            }
            return;
        }
    }

    throw std::invalid_argument("Tried to set nonexistent attribute with name " + name);
}


void GLProgram::setAttribute(std::string name, const std::vector<double> &data, bool update,
                             int offset, int size) {
    
    // Convert input data to floats
    std::vector<float> floatData(data.size());
    for(unsigned int i = 0; i < data.size(); i++) {
        floatData[i] = static_cast<float>(data[i]);
    }

    for(GLAttribute& a : attributes) {
        if(a.name == name) {
            if(a.type == GLData::Float) {
                glBindVertexArray(vaoHandle);
                glBindBuffer(GL_ARRAY_BUFFER, a.VBOLoc);
                if( update ) {
                   // TODO: Allow modifications to non-contiguous memory
                   offset *= sizeof(float);
                   if (size == -1) size = a.dataSize * sizeof(float);
                   else size *= sizeof(float);
                    
                   glBufferSubData(GL_ARRAY_BUFFER, offset, size, &floatData[0]);
                }
                else {
                   glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float), &floatData[0], GL_STATIC_DRAW);
                   a.dataSize = data.size();
                }
            } else {
                throw std::invalid_argument("Tried to set GLAttribute named " + name + " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) + "  Attempted type: " + std::to_string(static_cast<float>(GLData::Float)));
            }
            return;
        }
    }

    throw std::invalid_argument("No attribute with name " + name);
}

void GLProgram::setAttribute(std::string name, const std::vector<int> &data, bool update,
                             int offset, int size) {
   
    // FIXME I've seen strange bugs when using int's in shaders. Need to figure out it it's my shaders or something wrong with this function
 
    // Convert data to GL_INT (probably does nothing)
    std::vector<GLint> intData(data.size());
    for(unsigned int i = 0; i < data.size(); i++) {
        intData[i] = static_cast<GLint>(data[i]);
    }
    
    for(GLAttribute& a : attributes) {
        if(a.name == name) {
            if(a.type == GLData::Int) {
                glBindVertexArray(vaoHandle);
                glBindBuffer(GL_ARRAY_BUFFER, a.VBOLoc);
                if( update ) {
                   // TODO: Allow modifications to non-contiguous memory 
                   offset *= sizeof(GLint);
                   if (size == -1) size = a.dataSize * sizeof(GLint);
                   else size *= sizeof(GLint);
                    
                   glBufferSubData(GL_ARRAY_BUFFER, offset, size, &intData[0]);
                }
                else {
                   glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(GLint), &intData[0], GL_STATIC_DRAW);
                   a.dataSize = data.size();
                }
            } else {
                throw std::invalid_argument("Tried to set GLAttribute named " + name + " with wrong type. Actual type: " + std::to_string(static_cast<int>(a.type)) + "  Attempted type: " + std::to_string(static_cast<int>(GLData::Int)));
            }
            return;
        }
    }

    throw std::invalid_argument("No attribute with name " + name);
}

void GLProgram::setTexture1D(std::string name, unsigned char* texData, unsigned int length) {
    
    throw std::invalid_argument("This code hasn't been testded yet.");

    // Find the right texture
    for(GLTexture& t : textures) {

        if(t.name != name) continue;

        if(t.isSet) {
            throw std::invalid_argument("Attempted to set texture twice");
        }

        glActiveTexture(GL_TEXTURE0 + t.index);
        
        if(t.dim != 1) {
            throw std::invalid_argument("Tried to use texture with mismatched dimension " + std::to_string(t.dim));
        }

        glBindTexture(GL_TEXTURE_1D, t.bufferLoc); 
        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, length, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);
 
        // Set policies
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glUniform1i(t.location, t.index);
        t.isSet = true;
        return;
    }

    throw std::invalid_argument("No texture with name " + name);
}

void GLProgram::setTexture2D(std::string name, unsigned char* texData, unsigned int width, unsigned int height) {

    throw std::invalid_argument("This code hasn't been testded yet.");

    // Find the right texture
    for(GLTexture& t : textures) {

        if(t.name != name) continue;
        
        if(t.isSet) {
            throw std::invalid_argument("Attempted to set texture twice");
        }

        glActiveTexture(GL_TEXTURE0 + t.index);
        
        if(t.dim != 2) {
            throw std::invalid_argument("Tried to use texture with mismatched dimension " + std::to_string(t.dim));
        }

        glBindTexture(GL_TEXTURE_2D, t.bufferLoc); 
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texData);

        // Set policies
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glUniform1i(t.location, t.index); 
        t.isSet = true;
        return;
    }

    throw std::invalid_argument("No texture with name " + name);
}

void GLProgram::setTextureFromColormap(std::string name, Colormap colormap) {

    // Find the right texture
    for(GLTexture& t : textures) {

        if(t.name != name) continue;
        
        if(t.isSet) {
            throw std::invalid_argument("Attempted to set texture twice");
        }

        glActiveTexture(GL_TEXTURE0 + t.index);
        
        if(t.dim != 1) {
            throw std::invalid_argument("Tried to use texture with mismatched dimension " + std::to_string(t.dim));
        }

        glBindTexture(GL_TEXTURE_1D, t.bufferLoc); 

        // Fill a buffer with the data
        unsigned int dataLength = colormap.values.size() * 3;
        std::vector<float> colorBuffer(dataLength);
        for(unsigned int i = 0; i < colormap.values.size(); i++) {
            colorBuffer[3*i + 0] = static_cast<float>(colormap.values[i].x);
            colorBuffer[3*i + 1] = static_cast<float>(colormap.values[i].y);
            colorBuffer[3*i + 2] = static_cast<float>(colormap.values[i].z);
        }

        glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, colormap.values.size(), 0, GL_RGB, GL_FLOAT, &(colorBuffer[0]));

        // Set policies
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        glUniform1i(t.location, t.index);
        t.isSet = true;
        return;
    }

    throw std::invalid_argument("No texture with name " + name);
}

void GLProgram::setIndex(std::vector<uint3> indices) {
    if(!useIndex) {
        throw std::invalid_argument("Tried to setIndex() when program drawMode does not use indexed drawing");
    }
    
    // Reshape the vector
    // Right now, the data is probably laid out in this form already... but let's not be overly clever
    // and just reshape it.
    unsigned int* rawData = new unsigned int[3*indices.size()];
    indexSize = 3*indices.size();
    for(unsigned int i = 0; i < indices.size(); i++) {
        rawData[3*i + 0] = static_cast<float>(indices[i].first);
        rawData[3*i + 1] = static_cast<float>(indices[i].second);
        rawData[3*i + 2] = static_cast<float>(indices[i].third);
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*indices.size()*sizeof(unsigned int), rawData, GL_STATIC_DRAW);

    delete[] rawData;
}

void GLProgram::setIndex(std::vector<unsigned int> indices) {
   // (This version is typically used for indexed lines)

    if(!useIndex) {
        throw std::invalid_argument("Tried to setIndex() when program drawMode does not use indexed drawing");
    }
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    indexSize = indices.size();
}



// Check that uniforms and attributes are all set and of consistent size
void GLProgram::validateData() {

    // Check uniforms
    for(GLUniform& u : uniforms) {
        if(!u.isSet) {
            throw std::invalid_argument("Uniform " + u.name + " has not been set");
        }
    }
    
    // Check attributes
    long int attributeSize = -1;
    for(GLAttribute a : attributes) {
        if(a.dataSize < 0) {
            throw std::invalid_argument("Attribute " + a.name + " has not been set");
        }
        if(attributeSize == -1) { // first one we've seen
            attributeSize = a.dataSize;
        } else { // not the first one we've seen
            if(a.dataSize != attributeSize) {
                throw std::invalid_argument("Attributes have inconsistent size. One attribute has size " + std::to_string(attributeSize) + " and " + a.name + " has size " + std::to_string(a.dataSize));
            }
        }
    }
    drawDataLength = static_cast<unsigned int>(attributeSize);


    // Check textures
    for(GLTexture& t : textures) {
        if(!t.isSet) {
            throw std::invalid_argument("Texture " + t.name + " has not been set");
        }
    }

    // Check index (if applicable)
    if(useIndex) {
        if(indexSize == -1) {
            throw std::invalid_argument("Index buffer has not been filled");
        }
        drawDataLength = static_cast<unsigned int>(indexSize);
    }

}

void GLProgram::initCommonShaders() {
    // Compile functions accessible to all shaders
    commonShaderHandle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(commonShaderHandle, 1, &shaderCommonSource, nullptr);
    glCompileShader(commonShaderHandle);
    printShaderInfoLog(commonShaderHandle);
}

void GLProgram::draw() {

    validateData();

    glUseProgram(programHandle);
    glBindVertexArray(vaoHandle);

    switch(drawMode) {
        case DrawMode::Points:
            glDrawArrays(GL_POINTS, 0, drawDataLength); 
            break;
        case DrawMode::Triangles:
            glDrawArrays(GL_TRIANGLES, 0, drawDataLength); 
            break;
        case DrawMode::Lines:
            glDrawArrays(GL_LINES, 0, drawDataLength); 
            break;
        case DrawMode::TrianglesAdjacency:
            glDrawArrays(GL_TRIANGLES_ADJACENCY, 0, drawDataLength);
            break;
        case DrawMode::Patches:
            glPatchParameteri(GL_PATCH_VERTICES, nPatchVertices);
            glDrawArrays(GL_PATCHES, 0, drawDataLength); 
            break;
        case DrawMode::LinesAdjacency:
            glDrawArrays(GL_LINES_ADJACENCY, 0, drawDataLength);
            break;
        case DrawMode::IndexedLines:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
            glDrawElements(GL_LINES, drawDataLength, GL_UNSIGNED_INT, 0);
            break;
        case DrawMode::IndexedLineStrip:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
            glDrawElements(GL_LINE_STRIP, drawDataLength, GL_UNSIGNED_INT, 0);
            break;
        case DrawMode::IndexedLinesAdjacency:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
            glDrawElements(GL_LINES_ADJACENCY, drawDataLength, GL_UNSIGNED_INT, 0);
            break;
        case DrawMode::IndexedLineStripAdjacency:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
            glDrawElements(GL_LINE_STRIP_ADJACENCY, drawDataLength, GL_UNSIGNED_INT, 0);
            break;
        case DrawMode::IndexedTriangles:
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVBO);
            glDrawElements(GL_TRIANGLES, drawDataLength, GL_UNSIGNED_INT, 0);
            break;
    }


    checkGLError();
}



// Helper function to print compile logs
void printShaderInfoLog(GLuint shaderHandle) {
    int logLen = 0;
    int chars = 0;
    char *log;

    glGetShaderiv(shaderHandle, GL_INFO_LOG_LENGTH, &logLen);

    if (logLen > 1) { // for some reason we often get logs of length 1 with no visible characters
        log = (char *)malloc(logLen);
        glGetShaderInfoLog(shaderHandle, logLen, &chars, log);
        printf("Shader info log:\n%s\n", log);
        free(log);
        //exit(EXIT_FAILURE);
    }
}
void printProgramInfoLog(GLuint handle) {
    int logLen = 0;
    int chars = 0;
    char *log;

    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLen);

     if (logLen > 1)  { // for some reason we often get logs of length 1 with no visible characters
         log = (char *)malloc(logLen);
         glGetProgramInfoLog(handle, logLen, &chars, log);
         printf("Program info log:\n%s\n", log);
         free(log);
         //exit(EXIT_FAILURE);
     }
}


void checkGLError(bool fatal) {

#ifndef NDEBUG
    GLenum err = GL_NO_ERROR;
    while((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error!  Type: ";
        switch (err)
        {
        case GL_NO_ERROR:          std::cerr << "No error"; break;
        case GL_INVALID_ENUM:      std::cerr << "Invalid enum"; break;
        case GL_INVALID_VALUE:     std::cerr << "Invalid value"; break;
        case GL_INVALID_OPERATION: std::cerr << "Invalid operation"; break;
        //case GL_STACK_OVERFLOW:    std::cerr << "Stack overflow"; break;
        //case GL_STACK_UNDERFLOW:   std::cerr << "Stack underflow"; break;
        case GL_OUT_OF_MEMORY:     std::cerr << "Out of memory"; break;
        default:                   std::cerr << "Unknown error";
        }
        std::cerr << std::endl;

        if(fatal) {
            throw std::runtime_error("OpenGl error occurred");
        }
    }
#endif
}
