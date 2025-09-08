#pragma once

#include <xRenderer/model.h>
#include <xRenderer/vertex.h>

bool xrLoadModal(const char *modelFilePath, XRModel *model);
void xrLoadTexture(const char *textureFilePath, XRTexture *texture, stbi_uc **pixels);
