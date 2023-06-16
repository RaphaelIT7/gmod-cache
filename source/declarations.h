#pragma once

#include "vphysics_interface.h"
#include "model_types.h"
#include "datacache/imdlcache.h"
#include <engine/ivmodelinfo.h>
#include <cmodel.h>

class ITexture;
struct mtexinfo_t
{
	//Vector4D	textureVecsTexelsPerWorldUnits[2];	// [s/t] unit vectors in world space. 
	// [i][3] is the s/t offset relative to the origin.
	//Vector4D	lightmapVecsLuxelsPerWorldUnits[2];
	float		luxelsPerWorldUnit;
	float		worldUnitsPerLuxel;
	unsigned short flags;		// SURF_ flags.
	unsigned short texinfoFlags;// TEXINFO_ flags.
	IMaterial* material;

	mtexinfo_t(mtexinfo_t const& src)
	{
		// copy constructor needed since Vector4D has no copy constructor
		memcpy(this, &src, sizeof(mtexinfo_t));
	}
};

struct mnode_t
{
	// common with leaf
	int			contents;		// <0 to differentiate from leafs
	// -1 means check the node for visibility
	// -2 means don't check the node for visibility

	int			visframe;		// node needs to be traversed if current

	mnode_t* parent;
	short		area;			// If all leaves below this node are in the same area, then
	// this is the area index. If not, this is -1.
	short		flags;

	VectorAligned		m_vecCenter;
	VectorAligned		m_vecHalfDiagonal;

	// node specific
	cplane_t* plane;
	mnode_t* children[2];

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
};

struct mleaf_t
{
public:

	// common with node
	int			contents;		// contents mask
	int			visframe;		// node needs to be traversed if current

	mnode_t* parent;

	short		area;
	short		flags;
	VectorAligned		m_vecCenter;
	VectorAligned		m_vecHalfDiagonal;


	// leaf specific
	short		cluster;
	short		leafWaterDataID;

	unsigned short		firstmarksurface;
	unsigned short		nummarksurfaces;

	short		nummarknodesurfaces;
	short		unused;

	unsigned short	dispListStart;			// index into displist of first displacement
	unsigned short	dispCount;				// number of displacements in the list for this leaf
};

struct mleafwaterdata_t
{
	float	surfaceZ;
	float	minZ;
	short	surfaceTexInfoID;
	short	firstLeafIndex;
};

struct mcubemapsample_t
{
	Vector origin;
	ITexture* pTexture;
	unsigned char size; // default (mat_envmaptgasize) if 0, 1<<(size-1) otherwise.
};

typedef struct mportal_s
{
	unsigned short* vertList;
	int				numportalverts;
	cplane_t* plane;
	unsigned short	cluster[2];
	//	int				visframe;
} mportal_t;

typedef struct mcluster_s
{
	unsigned short* portalList;
	int				numportals;
} mcluster_t;

struct mmodel_t
{
	Vector		mins, maxs;
	Vector		origin;		// for sounds or lights
	float		radius;
	int			headnode;
	int			firstface, numfaces;
};

struct mprimitive_t
{
	int	type;
	unsigned short	firstIndex;
	unsigned short	indexCount;
	unsigned short	firstVert;
	unsigned short	vertCount;
};

struct mprimvert_t
{
	Vector		pos;
	float		texCoord[2];
	float		lightCoord[2];
};

struct LightShadowZBufferSample_t
{
	float m_flTraceDistance;								// how far we traced. 0 = invalid
	float m_flHitDistance;									// where we hit
};

typedef unsigned short WorldDecalHandle_t;

typedef unsigned short ShadowDecalHandle_t;

enum
{
	SHADOW_DECAL_HANDLE_INVALID = (ShadowDecalHandle_t)~0
};

typedef unsigned short OverlayFragmentHandle_t;

enum
{
	OVERLAY_FRAGMENT_INVALID = (OverlayFragmentHandle_t)~0
};

struct msurface2_t
{
	unsigned int			flags;			// see SURFDRAW_ #defines (only 22-bits right now)
	// These are packed in to flags now
	//unsigned char			vertCount;		// number of verts for this surface
	//unsigned char			sortGroup;		// only uses 2 bits, subdivide?
	cplane_t* plane;			// pointer to shared plane	
	int						firstvertindex;	// look up in model->vertindices[] (only uses 17-18 bits?)
	WorldDecalHandle_t		decals;
	ShadowDecalHandle_t		m_ShadowDecals; // unsigned short
	OverlayFragmentHandle_t m_nFirstOverlayFragment;	// First overlay fragment on the surface (short)
	short					materialSortID;
	unsigned short			vertBufferIndex;

	unsigned short			m_bDynamicShadowsEnabled : 1;	// Can this surface receive dynamic shadows?
	unsigned short			texinfo : 15;

	//IDispInfo* pDispInfo;         // displacement map information
	int						visframe;		// should be drawn when node is crossed
};

typedef msurface2_t* SurfaceHandle_t;

struct worldbrushdata_t
{
	int			numsubmodels;

	int			numplanes;
	cplane_t* planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t* leafs;

	int			numleafwaterdata;
	mleafwaterdata_t* leafwaterdata;

	int			numvertexes;
	//mvertex_t* vertexes;

	int			numoccluders;
	//doccluderdata_t* occluders;

	int			numoccluderpolys;
	//doccluderpolydata_t* occluderpolys;

	int			numoccludervertindices;
	int* occludervertindices;

	int				numvertnormalindices;	// These index vertnormals.
	unsigned short* vertnormalindices;

	int			numvertnormals;
	Vector* vertnormals;

	int			numnodes;
	mnode_t* nodes;
	unsigned short* m_LeafMinDistToWater;

	int			numtexinfo;
	mtexinfo_t* texinfo;

	int			numtexdata;
	csurface_t* texdata;

	int         numDispInfos;
	//HDISPINFOARRAY	hDispInfos;	// Use DispInfo_Index to get IDispInfos..

	/*
	int         numOrigSurfaces;
	msurface_t  *pOrigSurfaces;
	*/

	int			numsurfaces;
	//msurface1_t* surfaces1;
	msurface2_t* surfaces2;
	//msurfacelighting_t* surfacelighting;
	//msurfacenormal_t* surfacenormals;

	bool		unloadedlightmaps;

	int			numvertindices;
	unsigned short* vertindices;

	int nummarksurfaces;
	SurfaceHandle_t* marksurfaces;

	ColorRGBExp32* lightdata;

	int			numworldlights;
	//dworldlight_t* worldlights;

	//lightzbuffer_t* shadowzbuffers;

	// non-polygon primitives (strips and lists)
	int			numprimitives;
	mprimitive_t* primitives;

	int			numprimverts;
	mprimvert_t* primverts;

	int			numprimindices;
	unsigned short* primindices;

	int				m_nAreas;
	//darea_t* m_pAreas;

	int				m_nAreaPortals;
	//dareaportal_t* m_pAreaPortals;

	int				m_nClipPortalVerts;
	Vector* m_pClipPortalVerts;

	mcubemapsample_t* m_pCubemapSamples;
	int				   m_nCubemapSamples;

	int				m_nDispInfoReferences;
	unsigned short* m_pDispInfoReferences;

	//mleafambientindex_t* m_pLeafAmbient;
	//mleafambientlighting_t* m_pAmbientSamples;
#if 0
	int			numportals;
	mportal_t* portals;

	int			numclusters;
	mcluster_t* clusters;

	int			numportalverts;
	unsigned short* portalverts;

	int			numclusterportals;
	unsigned short* clusterportals;
#endif
};

struct brushdata_t
{
	worldbrushdata_t* pShared;
	int			firstmodelsurface, nummodelsurfaces;

	unsigned short	renderHandle;
	unsigned short	firstnode;
};

// only models with type "mod_sprite" have this data
struct spritedata_t
{
	int				numframes;
	int				width;
	int				height;
	//CEngineSprite* sprite;
};

struct model_t
{
	FileNameHandle_t	fnHandle;
	CUtlString			strName;

	int					nLoadFlags;		// mark loaded/not loaded
	int					nServerCount;	// marked at load
	IMaterial** ppMaterials;	// null-terminated runtime material cache; ((intptr_t*)(ppMaterials))[-1] == nMaterials

	modtype_t			type;
	int					flags;			// MODELFLAG_???

	// volume occupied by the model graphics	
	Vector				mins, maxs;
	float				radius;

	union
	{
		brushdata_t		brush;
		MDLHandle_t		studio;
		spritedata_t	sprite;
	};
};

#define MODELFLAG_MATERIALPROXY					0x0001	// we've got a material proxy
#define MODELFLAG_TRANSLUCENT					0x0002	// we've got a translucent model
#define MODELFLAG_VERTEXLIT						0x0004	// we've got a vertex-lit model
#define MODELFLAG_TRANSLUCENT_TWOPASS			0x0008	// render opaque part in opaque pass
#define MODELFLAG_FRAMEBUFFER_TEXTURE			0x0010	// we need the framebuffer as a texture
#define MODELFLAG_HAS_DLIGHT					0x0020	// need to check dlights
#define MODELFLAG_VIEW_WEAPON_MODEL				0x0040	// monitored by weapon model cache
#define MODELFLAG_RENDER_DISABLED				0x0080	// excluded for compliance with government regulations
#define MODELFLAG_STUDIOHDR_USES_FB_TEXTURE		0x0100	// persisted from studiohdr
#define MODELFLAG_STUDIOHDR_USES_BUMPMAPPING	0x0200	// persisted from studiohdr
#define MODELFLAG_STUDIOHDR_USES_ENV_CUBEMAP	0x0400	// persisted from studiohdr
#define MODELFLAG_STUDIOHDR_AMBIENT_BOOST		0x0800	// persisted from studiohdr
#define MODELFLAG_STUDIOHDR_DO_NOT_CAST_SHADOWS	0x1000	// persisted from studiohdr
#define MODELFLAG_STUDIOHDR_IS_STATIC_PROP		0x2000	// persisted from studiohdr
#define MODELFLAG_STUDIOHDR_BAKED_VERTEX_LIGHTING_IS_INDIRECT_ONLY	0x4000	// persisted from studiohdr

#define STUDIOHDR_BAKED_VERTEX_LIGHTING_IS_INDIRECT_ONLY	( 1 << 27 )

#define SURFDRAW_NOLIGHT		0x00000001		// no lightmap
#define	SURFDRAW_NODE			0x00000002		// This surface is on a node
#define	SURFDRAW_SKY			0x00000004		// portal to sky
#define SURFDRAW_BUMPLIGHT		0x00000008		// Has multiple lightmaps for bump-mapping
#define SURFDRAW_NODRAW			0x00000010		// don't draw this surface, not really visible
#define SURFDRAW_TRANS			0x00000020		// sort this surface from back to front
#define SURFDRAW_PLANEBACK		0x00000040		// faces away from plane of the node that stores this face
#define SURFDRAW_DYNAMIC		0x00000080		// Don't use a static buffer for this face
#define SURFDRAW_TANGENTSPACE	0x00000100		// This surface needs a tangent space
#define SURFDRAW_NOCULL			0x00000200		// Don't bother backface culling these
#define SURFDRAW_HASLIGHTSYTLES 0x00000400		// has a lightstyle other than 0
#define SURFDRAW_HAS_DISP		0x00000800		// has a dispinfo
#define SURFDRAW_ALPHATEST		0x00001000		// Is alphstested
#define SURFDRAW_NOSHADOWS		0x00002000		// No shadows baby
#define SURFDRAW_NODECALS		0x00004000		// No decals baby
#define SURFDRAW_HAS_PRIMS		0x00008000		// has a list of prims
#define SURFDRAW_WATERSURFACE	0x00010000	// This is a water surface
#define SURFDRAW_UNDERWATER		0x00020000
#define SURFDRAW_ABOVEWATER		0x00040000
#define SURFDRAW_HASDLIGHT		0x00080000	// Has some kind of dynamic light that must be checked
#define SURFDRAW_DLIGHTPASS		0x00100000	// Must be drawn in the dlight pass
#define SURFDRAW_UNUSED2		0x00200000	// unused
#define SURFDRAW_VERTCOUNT_MASK	0xFF000000	// 8 bits of vertex count
#define SURFDRAW_SORTGROUP_MASK	0x00C00000	// 2 bits of sortgroup

#define SURFDRAW_VERTCOUNT_SHIFT	24
#define SURFDRAW_SORTGROUP_SHIFT	22