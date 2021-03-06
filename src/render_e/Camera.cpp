/*
 *  RenderE
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergnesen.com/ ) 
 *  License: LGPL 3.0 ( http://www.gnu.org/licenses/lgpl-3.0.txt )
 */

#include "Camera.h"

#include <glm/gtc/type_ptr.hpp>
#include "GL/glew.h"
#include "math/Mathf.h"
#include "textures/Texture2D.h"
#include "OpenGLHelper.h"
#include "Log.h"

namespace render_e {

Camera::Camera()
: Component(CameraType), cameraMode(ORTHOGRAPHIC),
nearPlane(-1), farPlane(1),
left(-1), right(1),
bottom(-1), top(1), clearColor(0, 0, 0, 1), renderToTexture(false) {
    SetClearMask(COLOR_BUFFER | DEPTH_BUFFER);
    SetClearColor(glm::vec4(0, 0, 0, 1));
}

Camera::~Camera() {
    SetRenderToTexture(false, COLOR_BUFFER, NULL);
}

void Camera::SetProjection(float fieldOfView, float aspect, float nearPlane, float farPlane) {
    assert(nearPlane != 0.0f); // See http://www.opengl.org/sdk/docs/man/xhtml/gluPerspective.xml#notes
    this->cameraMode = PERSPECTIVE;
    this->fieldOfView = fieldOfView;
    this->aspect = aspect;
    this->nearPlane = nearPlane;
    this->farPlane = farPlane;

    // inspired by
    // Source code from
    // http://gpwiki.org/index.php/OpenGL_Tutorial_Framework:First_Polygon
    top = nearPlane * tan(fieldOfView * Mathf::DEGREE_TO_RADIAN);
    bottom = -top;
    left = bottom * aspect;
    right = top * aspect;

}

void Camera::SetOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    this->cameraMode = ORTHOGRAPHIC;
    this->left = left;
    this->right = right;
    this->bottom = bottom;
    this->top = top;
    this->nearPlane = nearPlane;
    this->farPlane = farPlane;
}

void Camera::SetClearMask(int clearBitMask) {
    clearMaskNative = 0;
    if (clearBitMask & COLOR_BUFFER) {
        clearMaskNative |= GL_COLOR_BUFFER_BIT;
    }
    if (clearBitMask & DEPTH_BUFFER) {
        clearMaskNative |= GL_DEPTH_BUFFER_BIT;
    }
    if (clearBitMask & STENCIL_BUFFER) {
        clearMaskNative |= GL_STENCIL_BUFFER_BIT;
    }
    clearMask = clearBitMask;
}

int Camera::GetClearMask() {
    return clearMask;
}

void Camera::Setup(int viewportWidth, int viewportHeight) {
    if (renderToTexture) {
        BindFrameBufferObject();
        glViewport(0, 0, fboWidth, fboHeight);

        // currently this should only be true on depth rendering
        //Disable color rendering, we only want to write to the Z-Buffer
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    } else {
        glViewport(0, 0, viewportWidth, viewportHeight);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (renderToTexture && framebufferTextureType == GL_TEXTURE_CUBE_MAP) {
        glFrustum(left, right, bottom, top, nearPlane, farPlane);
    } else if (cameraMode == PERSPECTIVE) {
        glFrustum(left, right, bottom, top, nearPlane, farPlane);
    } else {
        glOrtho(left, right, bottom, top, nearPlane, farPlane);
    }
    glMatrixMode(GL_MODELVIEW);

    SceneObject *sceneObject = GetOwner();
    assert(sceneObject != NULL); // Cannot setup camera with it being owned by a scene object
    glm::mat4 cameraMatrix = sceneObject->GetTransform()->GetLocalTransformInverse();
    
    glLoadMatrixf(glm::value_ptr(cameraMatrix));

    if (renderToTexture) {
        //		float modelView[16];
        //		float projection[16];
        glm::mat4 pj;
        glm::mat4 mv;
        glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(mv));
        glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(pj));

        // Moving from unit cube [-1,1] to [0,1]  
        glm::mat4 mat = glm::mat4(
                0.5f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.5f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.5f, 0.0f,
                0.5f, 0.5f, 0.5f, 1.0f);

        shadowMatrix = mat * pj*mv;
    }
    glClear(clearMaskNative);
}

void Camera::TearDown() {
    if (renderToTexture) {
        UnBindFrameBufferObject();
    }
}

void Camera::SetClearColor(glm::vec4 clearColor) {
    this->clearColor = clearColor;
}

void Camera::SetRenderToTexture(bool doRenderToTexture, CameraBuffer framebufferTargetType, TextureBase *texture) {
    if (renderToTexture == doRenderToTexture) {
        return;
    }

    renderToTexture = doRenderToTexture;
    if (renderToTexture) {
        glGenFramebuffers(1, &framebufferId);
        framebufferTextureId = texture->GetTextureId();

        fboWidth = texture->GetWidth();
        fboHeight = texture->GetHeight();

        framebufferTextureType = texture->GetTextureType();
        switch (framebufferTargetType) {
            case COLOR_BUFFER:
                this->framebufferTargetType = GL_COLOR_ATTACHMENT0;
                break;
            case DEPTH_BUFFER:
                this->framebufferTargetType = GL_DEPTH_ATTACHMENT;
                break;
            case STENCIL_BUFFER:
                this->framebufferTargetType = GL_STENCIL_ATTACHMENT;
                break;
            default:
                this->framebufferTargetType = framebufferTargetType;
        }

        if (framebufferTargetType == DEPTH_BUFFER) {


            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferId);

            // glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBufferId);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, framebufferTextureId, 0);

            // create color buffer (not used though)

        } else {
            // create render buffer
            glGenRenderbuffers(1, &renderBufferId);
            glBindRenderbuffer(GL_RENDERBUFFER, renderBufferId);
            glRenderbufferStorage(GL_RENDERBUFFER, /* internalformat */GL_DEPTH_COMPONENT24, fboWidth, fboHeight);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferId);
            if (framebufferTextureType == GL_TEXTURE_2D) {
                glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTextureId, 0);
            } else {
                for (int i = 0; i < 6; i++) {
                    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, framebufferTextureId, 0);
                }
            }
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBufferId);
        }
        GLenum frameBufferRes = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

        if (frameBufferRes != GL_FRAMEBUFFER_COMPLETE) {
            ERROR(OpenGLHelper::GetFrameBufferStatusString(frameBufferRes));
        }

    } else {
        glDeleteFramebuffers(1, &framebufferId);
        glDeleteRenderbuffers(1, &renderBufferId);
    }

}

void Camera::BindFrameBufferObject() {
    assert(renderToTexture);

    // target parameter : GL_FRAMEBUFFER = read/write, GL_DRAW_FRAMEBUFFER = write only
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferId);

}

void Camera::UnBindFrameBufferObject() {
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

float *Camera::GetShadowMatrix(glm::mat4 &modelTransform) {
    shadowMatrixMultiplied = shadowMatrix*modelTransform;
    return glm::value_ptr(shadowMatrixMultiplied);
}
}
