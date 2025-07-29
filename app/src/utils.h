#pragma once

#include <xRenderer/model.h>
#include <xRenderer/vertex.h>

bool loadModal(const char *modelFilePath, xr::Model *model);
stbi_uc *loadTexture(const char *textureFilePath, xr::Texture *texture);
