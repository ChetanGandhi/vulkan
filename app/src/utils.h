#pragma once

#include <xRenderer/model.h>
#include <xRenderer/context.h>

#include "lib/stb/stb_image.h"
#include "lib/tinyobj/tiny_obj_loader.h"

VkBool32 xrLoadModal(XrContext *context, const char *baseDir, const char *modelFile, XrModel *model);
void xrLoadTexture(XrContext *context, const char *textureFilePath, XrTexture *texture, stbi_uc **pixels);
