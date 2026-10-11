#include "game/shared/navmesh_debug_draw.h"
#include "tier2/renderutils.h"
#include "Detour/Include/DetourNavMesh.h"
#include "game/client/cdll_client_int.h"
#include "mathlib/vector.h"
#include "mathlib/vplane.h"
#include "game/server/ai_navmesh.h"
#include "tier1/cvar.h"

#include <cmath>
#include <cstdint>
#include <optional>
#include <unordered_set>
#include <utility>

static ConVar* Cvar_navmesh_debug_hull;
static ConVar* Cvar_navmesh_debug_camera_radius;
static ConVar* Cvar_navmesh_debug_lossy_optimization;

static uint64_t PackNavmeshOutline(const Vector3D& v1, const Vector3D& v2)
{
    int16_t x1 = static_cast<int16_t>(v1.x);
    int16_t x2 = static_cast<int16_t>(v2.x);
    int16_t y1 = static_cast<int16_t>(v1.y);
    int16_t y2 = static_cast<int16_t>(v2.y);
    if (x1 < x2)
        std::swap(x1, x2);
    if (y1 < y2)
        std::swap(y1, y2);

    return (static_cast<uint64_t>(static_cast<uint16_t>(x1)) << 48) | (static_cast<uint64_t>(static_cast<uint16_t>(x2)) << 32) |
           (static_cast<uint64_t>(static_cast<uint16_t>(y1)) << 16) | static_cast<uint16_t>(y2);
}

void DrawNavmeshPolys()
{
    if (!Cvar_navmesh_debug_hull || !Cvar_navmesh_debug_camera_radius || !Cvar_navmesh_debug_lossy_optimization || !g_pClientTools || !RenderLine ||
        !RenderTriangle)
        return;

    const int nHull = Cvar_navmesh_debug_hull->GetInt();
    if (nHull < 1 || nHull > 4)
        return;

    const float fCamRadius = Cvar_navmesh_debug_camera_radius->GetFloat();
    const float fCamRadiusSquared = fCamRadius * fCamRadius;
    if (!std::isfinite(fCamRadius) || fCamRadius <= 0.0f || !std::isfinite(fCamRadiusSquared))
        return;

    const dtNavMesh* pNavMesh = GetNavMeshForHull(nHull);
    if (!pNavMesh)
        return;

    Vector3D vCamera;
    QAngle aCamera;
    float fFov;
    if (!g_pClientTools->GetLocalPlayerEyePosition(vCamera, aCamera, fFov) || !vCamera.IsValid())
        return;

    VPlane CullPlane;
    CullPlane.Init(vCamera - aCamera.GetNormal() * 256.0f, aCamera);
    const bool bOptimize = Cvar_navmesh_debug_lossy_optimization->GetBool();

	std::optional<std::unordered_set<uint64_t>> Outlines;

    const int nMaxTiles = pNavMesh->getMaxTiles();
    for (int i = 0; i < nMaxTiles; ++i)
    {
        const dtMeshTile* pTile = pNavMesh->getTile(i);
        if (!pTile || !pTile->header)
            continue;

        for (int j = 0; j < pTile->header->polyCount; ++j)
        {
            const dtPoly* pPoly = &pTile->polys[j];
            const Vector3D vCenter(pPoly->center.x, pPoly->center.y, pPoly->center.z);
            const Vector3D vDelta = vCenter - vCamera;
            if (!(vDelta.Dot(vDelta) <= fCamRadiusSquared) || CullPlane.GetPointSide(vCenter) != SIDE_FRONT)
                continue;

            if (pPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
            {
                const dtOffMeshConnection* pCon = &pTile->offMeshCons[j - pTile->header->offMeshBase];
                RenderLine(Vector3D(pCon->posa.x, pCon->posa.y, pCon->posa.z), Vector3D(pCon->posb.x, pCon->posb.y, pCon->posb.z),
                           Color(255, 250, 50, 255), true);
                continue;
            }

            const dtPolyDetail* pDetail = &pTile->detailMeshes[j];
            for (int k = 0; k < pDetail->triCount; ++k)
            {
                const unsigned char* pTriangle = &pTile->detailTris[(pDetail->triBase + k) * 4];
                Vector3D v[3];
                for (int l = 0; l < 3; ++l)
                {
                    const rdVec3D& Vertex = pTriangle[l] < pPoly->vertCount ? pTile->verts[pPoly->verts[pTriangle[l]]]
                                                                            : pTile->detailVerts[pDetail->vertBase + pTriangle[l] - pPoly->vertCount];
                    v[l] = Vector3D(Vertex.x, Vertex.y, Vertex.z);
                }

                RenderTriangle(v[0], v[1], v[2], Color(110, 200, 220, 160), true);
                if (bOptimize && !Outlines)
                    Outlines.emplace();

                for (int l = 0; l < 3; ++l)
                {
                    const Vector3D& vStart = v[l];
                    const Vector3D& vEnd = v[(l + 1) % 3];
                    if (!bOptimize || Outlines->insert(PackNavmeshOutline(vStart, vEnd)).second)
                        RenderLine(vStart, vEnd, Color(0, 0, 150), true);
                }
            }
        }
    }
}

void InitialiseNavmeshDebugDraw()
{
    Cvar_navmesh_debug_hull =
        new ConVar("navmesh_debug_hull", "0", FCVAR_RELEASE, "0 = off, 1 = small/Human, 2 = med_short/Prowler, 3 = medium/Reaper, 4 = large/Titan");
    Cvar_navmesh_debug_camera_radius = new ConVar("navmesh_debug_camera_radius", "1000", FCVAR_RELEASE, "Radius in which to draw navmeshes");
    Cvar_navmesh_debug_lossy_optimization =
        new ConVar("navmesh_debug_lossy_optimization", "1", FCVAR_RELEASE, "Whether to enable lossy navmesh debug draw optimizations");
}
