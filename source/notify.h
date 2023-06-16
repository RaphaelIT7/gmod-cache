#pragma once

#include "declarations.h"

class CMDLCacheNotify : public IMDLCacheNotify
{
public:
	IMDLCache* g_ModelLoader;
	IPhysicsCollision* g_PhysCollision = nullptr;

	virtual void OnDataLoaded(MDLCacheDataType_t type, MDLHandle_t handle);
	virtual void OnCombinerPreCache(MDLHandle_t OldHandle, MDLHandle_t NewHandle);
	virtual void OnDataUnloaded(MDLCacheDataType_t type, MDLHandle_t handle);
	virtual bool ShouldSupressLoadWarning(MDLHandle_t handle);

private:
	void ComputeModelFlags(model_t* mod, MDLHandle_t handle);

	// Sets the bounds from the studiohdr 
	void SetBoundsFromStudioHdr(model_t* pModel, MDLHandle_t handle);
};
static CMDLCacheNotify s_MDLCacheNotify;