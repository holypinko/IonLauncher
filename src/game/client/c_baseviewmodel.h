#pragma once

#include "game/client/c_baseanimatingoverlay.h"

class C_BaseViewModel : public C_BaseAnimatingOverlay
{
  public:
    void UpdateRenderPresentation(float cameraTime);
    void RestoreRenderPresentation();
};
