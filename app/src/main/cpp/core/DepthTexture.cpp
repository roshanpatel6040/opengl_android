//
// Created by Roshan on 23/07/22.
//

#include "DepthTexture.h"

void DepthTexture::CreateOnGlThread() {
    GLCall(glGenTextures(1, &texture_id_))
    GLCall(glBindTexture(GL_TEXTURE_2D, texture_id_))
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void DepthTexture::UpdateWithDepthImageOnGlThread(const ArSession &session, const ArFrame &frame) {
    ArImage *depth_image = nullptr;
    ArStatus depthStatus = ArFrame_acquireDepthImage(&session, &frame, &depth_image);
    if (depthStatus == AR_SUCCESS) {
        if (!depth_image) {
            return;
        }
        ArImage_getTimestamp(&session, depth_image, &depthImageTime);
        if (previousDepthImageTime != depthImageTime) {
            previousDepthImageTime = depthImageTime;

            ArImageFormat format;
            ArImage_getFormat(&session, depth_image, &format);
            // Valid depth format
            if (format == AR_IMAGE_FORMAT_DEPTH16) {
                const uint8_t *depthData = nullptr;
                int32_t planeSize;
                ArImage_getPlaneData(&session, depth_image, 0, &depthData, &planeSize);

                if (depthData) {
                    int image_width = 0;
                    int image_height = 0;
                    int image_pixel_stride = 0;
                    int image_row_stride = 0;
                    ArImage_getWidth(&session, depth_image, &image_width);
                    ArImage_getHeight(&session, depth_image, &image_height);
                    ArImage_getPlanePixelStride(&session, depth_image, 0, &image_pixel_stride);
                    ArImage_getPlaneRowStride(&session, depth_image, 0, &image_row_stride);

                    GLCall(glBindTexture(GL_TEXTURE_2D, texture_id_))
                    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, image_width, image_height, 0, GL_RG, GL_UNSIGNED_BYTE, depthData))
                    width_ = image_width;
                    height_ = image_height;
                }
            }
        }
        ArImage_release(depth_image);
    }
}
