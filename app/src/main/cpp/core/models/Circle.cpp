//
// Created by Roshan on 25/04/22.
//

#include "string"
#include <Circle.h>
#include <Renderer.h>

void Circle::CreateOnGlThread(AAssetManager *manager) {

    float unit = (2 * M_PI) / VERTEX_NUM;
    vertices.reserve(VERTEX_NUM * 3);
    for (int i = 0; i < VERTEX_NUM; i++) {
        vertices[i * 3] = (float) (centerX + radius * cos(unit * i));
        vertices[i * 3 + 1] = (float) (centerY + radius * sin(unit * i));
        vertices[i * 3 + 2] = 0;
    }

//    float ratio = width > height ? (1.0f * height / width) : (1.0f * width / height);
//    int start = width > height ? 0 : 1;
//    int num = vertices.size() / 3;
//    for (int i = 0; i < num; i++) {
//        vertices[start + i * 3] *= ratio;
//    }

    std::string vertexTextureShader = "attribute vec3 position;\n"
                                      "uniform mat4 u_MVP;\n"
                                      "void main()\n"
                                      "{\n"
                                      " gl_Position = u_MVP * vec4(position,1.0);\n"
                                      " gl_PointSize = 20.0;\n"
                                      "}";

    std::string fragmentTextureShader = "uniform vec4 color[2];\n" // Passing multiple rgba
                                        "void main()\n"
                                        "{\n"
                                        " gl_FragColor = vec4(1.0,0.0,0.0,1.0);\n" // assign it to texture color
                                        "}";

    GLCall(glLineWidth(5))
    GLCall(program = glCreateProgram())
    GLCall(meshShader = new Shader(program, vertexTextureShader, fragmentTextureShader))
    GLCall(glLinkProgram(program))
}

void Circle::drawTriangle(glm::mat4 projection, glm::mat4 camera) {
    int CHORDS_COLOR_PER_VERTEX = 4;
    int BYTES_PER_FLOAT = 4;
    float vertices[] = {
            //    X    Y     Z     R     G      B    A
            0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};

    float color[] = {1.0f, 0.5f, 0.0f, 1.0f};

    // varying field uses to transfer data between vertex and fragment shader
    std::string vertexShaderCode = "attribute vec4 position;\n"
                                   "attribute vec4 v_color;\n"
                                   "uniform mat4 uMVPMatrix;\n"
                                   "varying vec4 f_color;\n" // Passes to fragment shader
                                   "void main()\n"
                                   "{\n"
                                   " f_color = v_color;\n"
                                   " gl_Position =  position;\n"
                                   "}";
    std::string fragmentShaderCode = "uniform vec4 color;\n" // Get using uniform location
                                     "varying vec4 f_color;\n" // Get from vertex shader
                                     "void main()\n"
                                     "{\n"
                                     " gl_FragColor = f_color;\n"
                                     //                                " gl_FragColor = color;\n"
                                     "}";

    GLCall(GLuint program = glCreateProgram())
    GLCall(Shader shader(program, vertexShaderCode, fragmentShaderCode))
    GLCall(glLinkProgram(program))
    GLCall(glUseProgram(program))

    GLCall(VertexBuffer vb(vertices, sizeof(vertices)))

    GLCall(GLint positionHandle = shader.getAttributeLocation("position"))
    GLCall(shader.vertexAttribPointer(positionHandle, GL_FLOAT, 3, 7 * sizeof(float),
                                      (GLvoid *) nullptr))
    GLCall(shader.enableVertexAttribArray(positionHandle))

    GLCall(GLuint colorPositionHandle = shader.getAttributeLocation("v_color"))
    GLCall(shader.vertexAttribPointer(colorPositionHandle, GL_FLOAT, 4, 7 * sizeof(float),
                                      (GLvoid *) (3 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(colorPositionHandle))

    GLCall(GLuint mvpLocation = shader.getUniformLocation("uMVPMatrix"))
    glm::mat4 model = glm::mat4(1.0);
    // model = glm::translate(model, glm::vec3(0.0, 0.0, -1.0));
    // model = glm::scale(model, glm::vec3(10.0, 10.0, 10.0));
    glm::mat4 mvp = projection * camera * model;
    GLCall(shader.setUniformMatrix4fv(mvpLocation, 1, glm::value_ptr(mvp)))

    GLCall(int colorHandle = shader.getUniformLocation("color"))
    GLCall(shader.setUniform4fv(colorHandle,
                                sizeof(color) / CHORDS_COLOR_PER_VERTEX / BYTES_PER_FLOAT, color))

    GLCall(glDrawArrays(GL_TRIANGLES, 0, 3))
    GLCall(shader.disableVertexAttribPointer(positionHandle))
    GLCall(shader.disableVertexAttribPointer(colorPositionHandle))
    GLCall(shader.unBind())
    GLCall(vb.unBind())
}

void Circle::drawSquare(glm::mat4 projection, glm::mat4 camera) {
    float vertices[] = {
            // x     y     z    tc1   tc2
            -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, // to map texture bottom left
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // to map texture bottom right
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // to map texture top right
            0.5f, 0.5f, 0.0f, 1.0f, 1.0f, // to map texture top left
    };

    // Text chords before texture at 4 points(direction)
    // values are fixed 0.0f to 1.0f
    float texChords[] = {
            0.0f, 1.0f, // to map texture bottom left
            0.0f, 0.0f, // to map texture bottom right
            1.0f, 0.0f, // to map texture top right
            1.0f, 1.0f, // to map texture top left
    };

    // order floats for drawing triangles
    // Re use number for anti clockwise
    // This order will create two triangles which makes it square
    unsigned int order[] = {0, 1, 2, 0, 2, 3};

    // RGBA color format
    float color[] = {0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f};

    // Works from bottom
//    glm::mat4 ortho = glm::ortho(-1.0f, (float) 1.0, -1.0f, (float) 1.0, -1.0f,
//                                 100.0f);
//    glm::mat4 perspective = glm::perspective(glm::radians(
//            90.0f), // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
//                                             (float) windowWidth /
//                                             (float) windowHeight,       // Aspect Ratio. Depends on the size of your previewWindow. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
//                                             0.1f,              // Near clipping plane. Keep as big as possible, or you'll get precision issues.
//                                             100.0f             // Far clipping plane. Keep as little as possible.
//    );
    // Translation
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    // Scaling
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
    // Rotation
    glm::mat4 rotationMatrixX = rotate(glm::mat4(1.0f), glm::radians(5.0f),
                                       glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationMatrixY = rotate(glm::mat4(1.0f), glm::radians(45.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));
    // Model matrix
    glm::mat4 modelMatrix = translation;
    // final projection
    glm::mat4 proj = projection * camera * modelMatrix;

    std::string vertexTextureShader = "attribute vec3 position;\n"
                                      "attribute vec2 texChords;\n"
                                      "uniform mat4 u_MVP;\n"
                                      "varying vec2 chords;\n"
                                      "void main()\n"
                                      "{\n"
                                      " gl_Position = u_MVP * vec4(position,1.0);\n"
                                      " chords = texChords;\n"
                                      "}";

    std::string fragmentTextureShader = "uniform vec4 color[2];\n" // Passing multiple rgba
                                        "varying vec2 chords;\n"
                                        "void main()\n"
                                        "{\n"
                                        " gl_FragColor = vec4(1.0,0.0,0.0,1.0);\n" // assign it to texture color
                                        // " gl_FragColor = color[0];\n" // select which color is selected from array
                                        "}";

    GLCall(GLuint program = glCreateProgram())
    GLCall(Shader shader(program, vertexTextureShader, fragmentTextureShader))
    GLCall(glLinkProgram(program))

    // Core OpenGL requires that we use a VAO so it knows what to do with our vertex inputs. If we fail to bind a VAO, OpenGL will most likely refuse to draw anything.

    GLCall(VertexBuffer vb(vertices, sizeof(vertices)))
    GLCall(IndexBuffer ib(order, sizeof(order)))

    GLCall(glUseProgram(program))

    GLCall(GLint positionHandle = shader.getAttributeLocation("position"))
    GLCall(shader.vertexAttribPointer(positionHandle, GL_FLOAT, 3, 5 * sizeof(float),
                                      (GLvoid *) nullptr))
    GLCall(shader.enableVertexAttribArray(positionHandle))

    GLCall(GLint texChordsHandle = shader.getAttributeLocation("texChords"))
    GLCall(shader.vertexAttribPointer(texChordsHandle, GL_FLOAT, 2, 5 * sizeof(float),
                                      (GLvoid *) (3 * sizeof(float))))
    GLCall(shader.enableVertexAttribArray(texChordsHandle))

    GLCall(GLint mvpHandle = shader.getUniformLocation("u_MVP"))
    GLCall(shader.setUniformMatrix4fv(mvpHandle, 1 /* No. of matrix array in our case only 1*/,
                                      glm::value_ptr(
                                              proj) /* pass reference at 0 0 position other work will be handled by opengl itself Basically opengl will take other values automatically */))

//    GLCall(Texture texture("/storage/emulated/0/Pictures/UHD Wallpapers/_uhdminimal34.jpg", 0, 4,
//                   GL_RGBA))
//    GLCall(GLint textureChords = shader.getUniformLocation("u_texture"))
//    GLCall(shader.setUniform1i(textureChords, texture.getSlot()))

    GLCall(GLint colorHandle = shader.getUniformLocation("color"))
    GLCall(shader.setUniform4fv(colorHandle, 2, color)) // Passing two rgba in float array

    GLCall(glDrawElements(GL_TRIANGLES, ib.getCount(), GL_UNSIGNED_INT,
                          nullptr))

    GLCall(shader.disableVertexAttribPointer(positionHandle))
    GLCall(shader.disableVertexAttribPointer(texChordsHandle))

    // GLCall(texture.~Texture())
    GLCall(shader.unBind())
    GLCall(vb.unBind())
    GLCall(ib.unBind())
}

void Circle::drawCircle(glm::mat4 projection, glm::mat4 camera) {

    // Translation
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -1.0f));
    // Scaling
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f));
    // Rotation
    glm::mat4 rotationMatrixX = rotate(glm::mat4(1.0f), glm::radians(90.0f),
                                       glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationMatrixY = rotate(glm::mat4(1.0f), glm::radians(90.0f),
                                       glm::vec3(0.0f, 1.0f, 0.0f));

    // Model matrix
    glm::mat4 modelMatrix = translation * rotationMatrixX;
    // final projection
    glm::mat4 proj = projection * camera * modelMatrix;


    // Core OpenGL requires that we use a VAO so it knows what to do with our vertex inputs. If we fail to bind a VAO, OpenGL will most likely refuse to draw anything.
    GLCall(glUseProgram(program))

    GLCall(GLint positionHandle = meshShader->getAttributeLocation("position"))
    GLCall(meshShader->vertexAttribPointer(positionHandle, GL_FLOAT, 3, 0, &vertices[0]))
    GLCall(meshShader->enableVertexAttribArray(positionHandle))

    GLCall(GLint mvpHandle = meshShader->getUniformLocation("u_MVP"))
    GLCall(meshShader->setUniformMatrix4fv(mvpHandle, 1 /* No. of matrix array in our case only 1*/,
                                      glm::value_ptr(
                                              proj) /* pass reference at 0 0 position other work will be handled by opengl itself Basically opengl will take other values automatically */))

    // Draw circle with only stroke
    // GLCall(glDrawArrays(GL_LINE_LOOP, 0, VERTEX_NUM))
    // Draw circle with stroke points
    // GLCall(glDrawArrays(GL_POINTS, 0, VERTEX_NUM))
    // Draw fill circle
    GLCall(glDrawArrays(GL_TRIANGLE_FAN, 0, VERTEX_NUM))

    GLCall(meshShader->disableVertexAttribPointer(positionHandle))

    GLCall(meshShader->unBind())

}


