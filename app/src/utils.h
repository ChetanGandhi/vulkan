#pragma once

#include <xRenderer/model.h>
#include <xRenderer/vertex.h>

bool loadModal(const char *modelFilePath, xr::Model *model);
void loadTexture(const char *textureFilePath, xr::Texture *texture, stbi_uc **pixels);
