#include "tier2/renderutils.h"
void InitialiseDebugOverlayRenderUtils(CModule module)
{
	RenderLine = module.Offset(0x192A70).RCast<decltype(RenderLine)>();
	RenderBox = module.Offset(0x192520).RCast<decltype(RenderBox)>();
	RenderWireframeBox = module.Offset(0x193DA0).RCast<decltype(RenderWireframeBox)>();
	RenderWireframeSweptBox = module.Offset(0x1945A0).RCast<decltype(RenderWireframeSweptBox)>();
	RenderTriangle = module.Offset(0x193940).RCast<decltype(RenderTriangle)>();
	RenderAxis = module.Offset(0x1924D0).RCast<decltype(RenderAxis)>();
	RenderSphere = module.Offset(0x194170).RCast<decltype(RenderSphere)>();
	RenderUnknown = module.Offset(0x1924E0).RCast<decltype(RenderUnknown)>();
}
