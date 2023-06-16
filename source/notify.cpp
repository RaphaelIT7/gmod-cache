#include "notify.h"
#include <istudiorender.h>

inline SurfaceHandle_t SurfaceHandleFromIndex(int surfaceIndex, worldbrushdata_t* pData)
{
	return &pData->surfaces2[surfaceIndex];
}

inline unsigned int& MSurf_Flags(SurfaceHandle_t surfID)
{
	return surfID->flags;
}

inline mtexinfo_t* MSurf_TexInfo(SurfaceHandle_t surfID, worldbrushdata_t* pData)
{
	return &pData->texinfo[surfID->texinfo];
}

//-----------------------------------------------------------------------------
// returns the first n materials.
//-----------------------------------------------------------------------------
int Mod_GetModelMaterials(model_t* pModel, int count, IMaterial** ppMaterials)
{
	studiohdr_t* pStudioHdr;
	int found = 0;
	int	i;

	switch (pModel->type)
	{
	case mod_brush:
	{
		for (i = 0; i < pModel->brush.nummodelsurfaces; ++i)
		{
			SurfaceHandle_t surfID = SurfaceHandleFromIndex(pModel->brush.firstmodelsurface + i, pModel->brush.pShared);
			if (MSurf_Flags(surfID) & SURFDRAW_NODRAW)
				continue;

			IMaterial* pMaterial = MSurf_TexInfo(surfID, pModel->brush.pShared)->material;

			// Try to find the material in the unique list of materials
			// if it's not there, then add it
			int j = found;
			while (--j >= 0)
			{
				if (ppMaterials[j] == pMaterial)
					break;
			}
			if (j < 0)
				ppMaterials[found++] = pMaterial;

			// Stop when we've gotten count materials
			if (found >= count)
				return found;
		}
	}
	break;

	case mod_studio:
		if (pModel->ppMaterials)
		{
			int nMaterials = ((intptr_t*)(pModel->ppMaterials))[-1];
			found = MIN(count, nMaterials);
			memcpy(ppMaterials, pModel->ppMaterials, found * sizeof(IMaterial*));
		}
		else
		{
			// Get the studiohdr into the cache
			pStudioHdr = g_pMDLCache->GetStudioHdr(pModel->studio);
			// Get the list of materials
			found = g_pStudioRender->GetMaterialList(pStudioHdr, count, ppMaterials);
		}
		break;

	default:
		// unimplemented
		//Assert(0);
		break;
	}

	return found;
}

//-----------------------------------------------------------------------------
// Computes model flags
//-----------------------------------------------------------------------------
void CMDLCacheNotify::ComputeModelFlags(model_t* pModel, MDLHandle_t handle)
{
	studiohdr_t* pStudioHdr = g_pMDLCache->GetStudioHdr(handle);

	// Clear out those flags we set...
	pModel->flags &= ~(MODELFLAG_TRANSLUCENT_TWOPASS | MODELFLAG_VERTEXLIT |
		MODELFLAG_TRANSLUCENT | MODELFLAG_MATERIALPROXY | MODELFLAG_FRAMEBUFFER_TEXTURE |
		MODELFLAG_STUDIOHDR_USES_FB_TEXTURE | MODELFLAG_STUDIOHDR_USES_BUMPMAPPING |
		MODELFLAG_STUDIOHDR_USES_ENV_CUBEMAP | MODELFLAG_STUDIOHDR_IS_STATIC_PROP | MODELFLAG_STUDIOHDR_BAKED_VERTEX_LIGHTING_IS_INDIRECT_ONLY);

	bool bForceOpaque = (pStudioHdr->flags & STUDIOHDR_FLAGS_FORCE_OPAQUE) != 0;

	if (pStudioHdr->flags & STUDIOHDR_FLAGS_TRANSLUCENT_TWOPASS)
	{
		pModel->flags |= MODELFLAG_TRANSLUCENT_TWOPASS;
	}
	if (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_FB_TEXTURE)
	{
		pModel->flags |= MODELFLAG_STUDIOHDR_USES_FB_TEXTURE;
	}
	if (pStudioHdr->flags & STUDIOHDR_FLAGS_STATIC_PROP)
	{
		pModel->flags |= MODELFLAG_STUDIOHDR_IS_STATIC_PROP;
	}
	if (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_BUMPMAPPING)
	{
		pModel->flags |= MODELFLAG_STUDIOHDR_USES_BUMPMAPPING;
	}
	if (pStudioHdr->flags & STUDIOHDR_BAKED_VERTEX_LIGHTING_IS_INDIRECT_ONLY)
	{
		pModel->flags |= MODELFLAG_STUDIOHDR_BAKED_VERTEX_LIGHTING_IS_INDIRECT_ONLY;
	}
	if (pStudioHdr->flags & STUDIOHDR_FLAGS_USES_ENV_CUBEMAP)
	{
		pModel->flags |= MODELFLAG_STUDIOHDR_USES_ENV_CUBEMAP;
	}
	if (pStudioHdr->flags & STUDIOHDR_FLAGS_AMBIENT_BOOST)
	{
		pModel->flags |= MODELFLAG_STUDIOHDR_AMBIENT_BOOST;
	}
	if (pStudioHdr->flags & STUDIOHDR_FLAGS_DO_NOT_CAST_SHADOWS)
	{
		pModel->flags |= MODELFLAG_STUDIOHDR_DO_NOT_CAST_SHADOWS;
	}

	IMaterial* materials[128];
	int materialCount = Mod_GetModelMaterials(pModel, ARRAYSIZE(materials), materials);

	for (int i = 0; i < materialCount; ++i)
	{
		IMaterial* pMaterial = materials[i];
		if (!pMaterial)
			continue;

		if (pMaterial->IsVertexLit())
		{
			pModel->flags |= MODELFLAG_VERTEXLIT;
		}

		if (!bForceOpaque && pMaterial->IsTranslucent())
		{
			//Msg("Translucent material %s for model %s\n", pLODData->ppMaterials[i]->GetName(), pModel->name );
			pModel->flags |= MODELFLAG_TRANSLUCENT;
		}

		if (pMaterial->HasProxy())
		{
			pModel->flags |= MODELFLAG_MATERIALPROXY;
		}

		if (pMaterial->NeedsPowerOfTwoFrameBufferTexture(false)) // The false checks if it will ever need the frame buffer, not just this frame
		{
			pModel->flags |= MODELFLAG_FRAMEBUFFER_TEXTURE;
		}
	}
}

//-----------------------------------------------------------------------------
// Sets the bounds from the studiohdr 
//-----------------------------------------------------------------------------
void CMDLCacheNotify::SetBoundsFromStudioHdr(model_t* pModel, MDLHandle_t handle)
{
	studiohdr_t* pStudioHdr = g_pMDLCache->GetStudioHdr(handle);
	VectorCopy(pStudioHdr->hull_min, pModel->mins);
	VectorCopy(pStudioHdr->hull_max, pModel->maxs);
	pModel->radius = 0.0f;
	for (int i = 0; i < 3; i++)
	{
		if (fabs(pModel->mins[i]) > pModel->radius)
		{
			pModel->radius = fabs(pModel->mins[i]);
		}

		if (fabs(pModel->maxs[i]) > pModel->radius)
		{
			pModel->radius = fabs(pModel->maxs[i]);
		}
	}
}

//-----------------------------------------------------------------------------
// Callbacks to get called when various data is loaded or unloaded 
//-----------------------------------------------------------------------------
void CMDLCacheNotify::OnDataLoaded(MDLCacheDataType_t type, MDLHandle_t handle)
{
	model_t* pModel = (model_t*)g_pMDLCache->GetUserData(handle);
	Msg(g_pMDLCache->GetModelName(handle));
	Msg("\n");
	// NOTE: A NULL model can occur for dependent MDLHandle_ts (like .ani files)
	if (!pModel)
		return;

	switch (type)
	{
	case MDLCACHE_STUDIOHDR:
	{
		// FIXME: This code only works because it assumes StudioHdr
		// is loaded before VCollide.
		SetBoundsFromStudioHdr(pModel, handle);
	}
	break;

	case MDLCACHE_VCOLLIDE:
	{
		SetBoundsFromStudioHdr(pModel, handle);

		// Expand the model bounds to enclose the collision model (should be done in studiomdl)
		vcollide_t* pCollide = g_pMDLCache->GetVCollide(handle);
		if (pCollide)
		{
			Vector mins, maxs;
			g_PhysCollision->CollideGetAABB(&mins, &maxs, pCollide->solids[0], vec3_origin, vec3_angle);
			AddPointToBounds(mins, pModel->mins, pModel->maxs);
			AddPointToBounds(maxs, pModel->mins, pModel->maxs);
		}
	}
	break;

	case MDLCACHE_STUDIOHWDATA:
	{
		ComputeModelFlags(pModel, handle);

/*#if !defined( DEDICATED )
		if (g_ModelLoader.m_bAllowWeaponModelCache)
		{
			int nMapIndex = g_ModelLoader.m_WeaponModelCache.Find(pModel);
			if (nMapIndex != g_ModelLoader.m_WeaponModelCache.InvalidIndex())
			{
				g_ModelLoader.m_WeaponModelCache[nMapIndex]->m_bStudioHWDataResident = true;
			}
		}
#endif*/
	}
	break;
	}
}

void CMDLCacheNotify::OnCombinerPreCache(MDLHandle_t OldHandle, MDLHandle_t NewHandle)
{
	model_t* pModel = (model_t*)g_pMDLCache->GetUserData(OldHandle);
	if (!pModel)
	{
		//Assert(0);
		return;
	}

	//pModel.studio = NewHandle;
	g_pMDLCache->SetUserData(OldHandle, NULL);
	g_pMDLCache->SetUserData(NewHandle, pModel);
}


void CMDLCacheNotify::OnDataUnloaded(MDLCacheDataType_t type, MDLHandle_t handle)
{
#if defined( PLATFORM_WINDOWS_PC ) || defined( DEDICATED )
	return;
#endif

/* !defined(DEDICATED)
	if (g_ModelLoader.m_bAllowWeaponModelCache && type == MDLCACHE_STUDIOHWDATA)
	{
		model_t* pModel = (model_t*)g_pMDLCache->GetUserData(handle);
		if (pModel)
		{
			int nMapIndex = g_ModelLoader.m_WeaponModelCache.Find(pModel);
			if (nMapIndex != g_ModelLoader.m_WeaponModelCache.InvalidIndex())
			{
				g_ModelLoader.m_WeaponModelCache[nMapIndex]->m_bStudioHWDataResident = false;
			}
		}
	}
#endif*/
}

bool CMDLCacheNotify::ShouldSupressLoadWarning(MDLHandle_t handle)
{
/*#if !defined( DEDICATED )
	if (g_ModelLoader.m_bAllowWeaponModelCache)
	{
		// the QL wants to warn about loading data outside its awareness
		// weapon models are explicitly prevented from the QL path
		model_t* pModel = (model_t*)g_pMDLCache->GetUserData(handle);
		if (pModel)
		{
			int nMapIndex = g_ModelLoader.m_WeaponModelCache.Find(pModel);
			if (nMapIndex != g_ModelLoader.m_WeaponModelCache.InvalidIndex())
			{
				// model is part of weapon model cache
				// any QL load warnings are benign and should be suppressed
				return true;
			}
		}
	}
#endif*/

	return false;
}