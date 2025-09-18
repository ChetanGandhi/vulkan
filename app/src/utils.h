#pragma once

#include <xRenderer/model.h>
#include <xRenderer/texture.h>
#include <xRenderer/vertex.h>
#include <xRenderer/context.h>

#include "lib/stb/stb_image.h"
#include "lib/tinyobj/tiny_obj_loader.h"

bool xrLoadModal(XrContext *context, const char *modelFilePath, XrModel *model);
void xrLoadTexture(XrContext *context, const char *textureFilePath, XrTexture *texture, stbi_uc **pixels);
