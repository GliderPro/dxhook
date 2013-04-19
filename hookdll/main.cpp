#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#include <d3d9.h>

#include "detourxs.h"
#include "AntTweakBar.h"

extern "C" LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
extern "C" LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam);
extern "C" void InstallHook(HWND hWnd, const char *pName);
extern "C" void ReleaseHook();

// hooked function typedefs
typedef IDirect3D9 * (WINAPI *tDirect3DCreate9)(UINT SDKVersion);
typedef HRESULT (__stdcall *tCreateDevice)(IDirect3D9 *thisPtr, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, 
										   D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface);
typedef HRESULT (__stdcall *tEndScene)(IDirect3DDevice9 *thisPtr);

#pragma data_seg (".shared")
// only INITIALIZED variables in this block will actually end up in the shared section!!!
// http://abdelrahmanogail.wordpress.com/2010/12/28/sharing-variables-between-several-instances-from-the-same-exe-or-dll/
static HHOOK hHook = NULL;
static HINSTANCE hins = NULL;
static HWND hwnd = NULL;
static char targetName[MAX_PATH] = "";
#pragma data_seg ()

// non-shared globals
static bool inTarget = false;
static IDirect3D9 *pD3D9 = NULL;
static IDirect3DDevice9 *pD3D9Dev = NULL;
static DetourXS * dDirect3DCreate9 = NULL;
static DetourXS * dCreateDevice = NULL;
static DetourXS * dEndScene = NULL;
static TwBar *pBar = NULL;
static bool drawTwBar = false;
static float gColor[] = { 1, 0, 0 };
static HWND targetWindow = NULL;
static WNDPROC OldWindowProc = NULL;

// hooked functions
tEndScene oEndScene;
HRESULT __stdcall hEndScene(IDirect3DDevice9 *thisPtr)
{
	HRESULT rval = S_OK;
	static DWORD frame_count = 0;

	if(frame_count++ > 100)
	{
		OutputDebugStringW(L"hEndScene() called.\n");
		frame_count = 0;
	}

	if( drawTwBar )
	{
		TwDraw();
	}

	rval = oEndScene(thisPtr);

	return rval;
}

tCreateDevice oCreateDevice;
HRESULT __stdcall hCreateDevice(IDirect3D9 *thisPtr, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, 
								D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface)
{
	HRESULT rval = S_OK;

	OutputDebugStringW(L"hCreateDevice() called.\n");

	rval = oCreateDevice(thisPtr,Adapter,DeviceType,hFocusWindow,BehaviorFlags,pPresentationParameters,&pD3D9Dev);
	*ppReturnedDeviceInterface = pD3D9Dev;
	if( pD3D9Dev != NULL )
	{
		PDWORD_PTR vtable = reinterpret_cast<PDWORD_PTR>(*(DWORD_PTR*)pD3D9Dev);
		if( vtable != NULL )
		{
			dEndScene = new DetourXS((void*)vtable[42], hEndScene);
			oEndScene = (tEndScene) dEndScene->GetTrampoline();
		}
		
		TwInit(TW_DIRECT3D9, pD3D9Dev);
		pBar = TwNewBar("TESTBAR");
		TwDefine(" GLOBAL help='This example shows how to integrate AntTweakBar in a DirectX9 application.' "); // Message added to the help bar.
		TwDefine(" TESTBAR color='128 224 160' text=dark "); // Change TweakBar color and use dark text
		TwAddVarRW(pBar, "Color", TW_TYPE_COLOR3F, &gColor, " label='Strip color' ");
		drawTwBar = true;

		targetWindow = hFocusWindow != NULL ? hFocusWindow : pPresentationParameters->hDeviceWindow;
		if( targetWindow != NULL )
		{
			RECT rect;
			GetClientRect(targetWindow,&rect);
			TwWindowSize(rect.right-rect.left,rect.bottom-rect.top);
			OldWindowProc = (WNDPROC)SetWindowLongPtr(targetWindow,GWL_WNDPROC,(LONG_PTR)WindowProc);
		}
	}

	return rval;
}

tDirect3DCreate9 oDirect3DCreate9;
IDirect3D9 * WINAPI hDirect3DCreate9(UINT SDKVersion)
{
	OutputDebugStringW(L"hDirect3DCreate9() called.\n");
	pD3D9 = oDirect3DCreate9(SDKVersion);

	if( pD3D9 != NULL )
	{
		PDWORD_PTR vtable = reinterpret_cast<PDWORD_PTR>(*(DWORD_PTR*)pD3D9);
		if( vtable != NULL )
		{
			dCreateDevice = new DetourXS((void*)vtable[16], hCreateDevice);
			oCreateDevice = (tCreateDevice) dCreateDevice->GetTrampoline();
		}
	}

	return pD3D9;
}

int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	if (NULL == hins) hins = hInstance;

	if( DLL_PROCESS_DETACH == fdwReason && inTarget )
	{
		if( NULL != OldWindowProc && NULL != targetWindow )
		{
			SetWindowLongPtr(targetWindow,GWL_WNDPROC,(LONG_PTR)OldWindowProc);	// not doing this could be bad so do it...
		}
	}
	return TRUE;
}

extern "C" LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int handled = 0;

	if(WM_CLOSE == message)
	{
		drawTwBar = false;
		TwTerminate();
	}
	else
	{
		handled = TwEventWin(hWnd,message,wParam,lParam);
	}

	return handled ? 0 : CallWindowProc(OldWindowProc, hWnd, message, wParam, lParam);
}

extern "C" LRESULT CALLBACK HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if( HCBT_CREATEWND == nCode )
	{
		if( inTarget == false )
		{
			char szPath[MAX_PATH];

			if( GetModuleFileNameA( NULL, szPath, MAX_PATH ) )
			{
				if( strstr(szPath,targetName) )
				{
					OutputDebugString(_T("Found target process.  Hooking DirectX...\n"));
					tDirect3DCreate9 _Direct3DCreate9 = (tDirect3DCreate9)GetProcAddress(GetModuleHandle(TEXT("d3d9.dll")),"Direct3DCreate9");
					if( _Direct3DCreate9 != NULL )
					{
						dDirect3DCreate9 = new DetourXS(_Direct3DCreate9, hDirect3DCreate9);
						oDirect3DCreate9 = (tDirect3DCreate9) dDirect3DCreate9->GetTrampoline();
						inTarget = true;
						SendMessage(hwnd,0xBEEF,GetCurrentProcessId(),0);
					}
				}
			}
		}
	}

	return CallNextHookEx(hHook, nCode, wParam, lParam);
}

extern "C" void InstallHook(HWND hWnd, const char *pName)
{
	hwnd = hWnd;
	targetName[0] = 0;
	if( pName )
	{
		strcpy_s(targetName,pName);
	}
	TCHAR dbgBuf[256];
	_stprintf_s(dbgBuf,"hwnd = 0x%08X\n", hwnd);
	OutputDebugString(dbgBuf);
	_stprintf_s(dbgBuf,"targetName = %hs\n", targetName);
	OutputDebugString(dbgBuf);

	hHook = SetWindowsHookEx(WH_CBT, (HOOKPROC)HookProc, hins, 0);
	if (NULL == hHook) 
	{
		TCHAR msg[256];
		wsprintf(msg, TEXT("Cannot install hook, code: %d"), GetLastError());
		MessageBox(hwnd, msg, TEXT("error"), MB_ICONERROR);
	}
}

extern "C" void ReleaseHook()
{
	if (hHook != NULL) 
	{
		BOOL bRes = UnhookWindowsHookEx(hHook);
		if (!bRes) MessageBox(hwnd, TEXT("Cannot remove hook."), TEXT("error"), MB_ICONERROR);
	}
}


#if 0
DECLARE_INTERFACE_(IDirect3D9, IUnknown)
{
	/*** IUnknown methods ***/
000	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
001	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
002	STDMETHOD_(ULONG,Release)(THIS) PURE;

/*** IDirect3D9 methods ***/
003	STDMETHOD(RegisterSoftwareDevice)(THIS_ void* pInitializeFunction) PURE;
004	STDMETHOD_(UINT, GetAdapterCount)(THIS) PURE;
005	STDMETHOD(GetAdapterIdentifier)(THIS_ UINT Adapter,DWORD Flags,D3DADAPTER_IDENTIFIER9* pIdentifier) PURE;
006	STDMETHOD_(UINT, GetAdapterModeCount)(THIS_ UINT Adapter,D3DFORMAT Format) PURE;
007	STDMETHOD(EnumAdapterModes)(THIS_ UINT Adapter,D3DFORMAT Format,UINT Mode,D3DDISPLAYMODE* pMode) PURE;
008	STDMETHOD(GetAdapterDisplayMode)(THIS_ UINT Adapter,D3DDISPLAYMODE* pMode) PURE;
009	STDMETHOD(CheckDeviceType)(THIS_ UINT Adapter,D3DDEVTYPE DevType,D3DFORMAT AdapterFormat,D3DFORMAT BackBufferFormat,BOOL bWindowed) PURE;
010	STDMETHOD(CheckDeviceFormat)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,DWORD Usage,D3DRESOURCETYPE RType,D3DFORMAT CheckFormat) PURE;
011	STDMETHOD(CheckDeviceMultiSampleType)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SurfaceFormat,BOOL Windowed,D3DMULTISAMPLE_TYPE MultiSampleType,DWORD* pQualityLevels) PURE;
012	STDMETHOD(CheckDepthStencilMatch)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT AdapterFormat,D3DFORMAT RenderTargetFormat,D3DFORMAT DepthStencilFormat) PURE;
013	STDMETHOD(CheckDeviceFormatConversion)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT SourceFormat,D3DFORMAT TargetFormat) PURE;
014	STDMETHOD(GetDeviceCaps)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,D3DCAPS9* pCaps) PURE;
015	STDMETHOD_(HMONITOR, GetAdapterMonitor)(THIS_ UINT Adapter) PURE;
016	STDMETHOD(CreateDevice)(THIS_ UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface) PURE;

#ifdef D3D_DEBUG_INFO
	LPCWSTR Version;
#endif
};

DECLARE_INTERFACE_(IDirect3DDevice9, IUnknown)
{
	/*** IUnknown methods ***/
000	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void** ppvObj) PURE;
001	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
002	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/*** IDirect3DDevice9 methods ***/
003	STDMETHOD(TestCooperativeLevel)(THIS) PURE;
004	STDMETHOD_(UINT, GetAvailableTextureMem)(THIS) PURE;
005	STDMETHOD(EvictManagedResources)(THIS) PURE;
006	STDMETHOD(GetDirect3D)(THIS_ IDirect3D9** ppD3D9) PURE;
007	STDMETHOD(GetDeviceCaps)(THIS_ D3DCAPS9* pCaps) PURE;
008	STDMETHOD(GetDisplayMode)(THIS_ UINT iSwapChain,D3DDISPLAYMODE* pMode) PURE;
009	STDMETHOD(GetCreationParameters)(THIS_ D3DDEVICE_CREATION_PARAMETERS *pParameters) PURE;
010	STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot,UINT YHotSpot,IDirect3DSurface9* pCursorBitmap) PURE;
011	STDMETHOD_(void, SetCursorPosition)(THIS_ int X,int Y,DWORD Flags) PURE;
012	STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow) PURE;
013	STDMETHOD(CreateAdditionalSwapChain)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) PURE;
014	STDMETHOD(GetSwapChain)(THIS_ UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) PURE;
015	STDMETHOD_(UINT, GetNumberOfSwapChains)(THIS) PURE;
016	STDMETHOD(Reset)(THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters) PURE;
017	STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) PURE;
018	STDMETHOD(GetBackBuffer)(THIS_ UINT iSwapChain,UINT iBackBuffer,D3DBACKBUFFER_TYPE Type,IDirect3DSurface9** ppBackBuffer) PURE;
019	STDMETHOD(GetRasterStatus)(THIS_ UINT iSwapChain,D3DRASTER_STATUS* pRasterStatus) PURE;
020	STDMETHOD(SetDialogBoxMode)(THIS_ BOOL bEnableDialogs) PURE;
021	STDMETHOD_(void, SetGammaRamp)(THIS_ UINT iSwapChain,DWORD Flags,CONST D3DGAMMARAMP* pRamp) PURE;
022	STDMETHOD_(void, GetGammaRamp)(THIS_ UINT iSwapChain,D3DGAMMARAMP* pRamp) PURE;
023	STDMETHOD(CreateTexture)(THIS_ UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) PURE;
024	STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width,UINT Height,UINT Depth,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DVolumeTexture9** ppVolumeTexture,HANDLE* pSharedHandle) PURE;
025	STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DCubeTexture9** ppCubeTexture,HANDLE* pSharedHandle) PURE;
026	STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length,DWORD Usage,DWORD FVF,D3DPOOL Pool,IDirect3DVertexBuffer9** ppVertexBuffer,HANDLE* pSharedHandle) PURE;
027	STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DIndexBuffer9** ppIndexBuffer,HANDLE* pSharedHandle) PURE;
028	STDMETHOD(CreateRenderTarget)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) PURE;
029	STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) PURE;
030	STDMETHOD(UpdateSurface)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint) PURE;
031	STDMETHOD(UpdateTexture)(THIS_ IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture) PURE;
032	STDMETHOD(GetRenderTargetData)(THIS_ IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface) PURE;
033	STDMETHOD(GetFrontBufferData)(THIS_ UINT iSwapChain,IDirect3DSurface9* pDestSurface) PURE;
034	STDMETHOD(StretchRect)(THIS_ IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter) PURE;
035	STDMETHOD(ColorFill)(THIS_ IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color) PURE;
036	STDMETHOD(CreateOffscreenPlainSurface)(THIS_ UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) PURE;
037	STDMETHOD(SetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) PURE;
038	STDMETHOD(GetRenderTarget)(THIS_ DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) PURE;
039	STDMETHOD(SetDepthStencilSurface)(THIS_ IDirect3DSurface9* pNewZStencil) PURE;
040	STDMETHOD(GetDepthStencilSurface)(THIS_ IDirect3DSurface9** ppZStencilSurface) PURE;
041	STDMETHOD(BeginScene)(THIS) PURE;
042	STDMETHOD(EndScene)(THIS) PURE;
043	STDMETHOD(Clear)(THIS_ DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) PURE;
044	STDMETHOD(SetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix) PURE;
045	STDMETHOD(GetTransform)(THIS_ D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) PURE;
046	STDMETHOD(MultiplyTransform)(THIS_ D3DTRANSFORMSTATETYPE,CONST D3DMATRIX*) PURE;
047	STDMETHOD(SetViewport)(THIS_ CONST D3DVIEWPORT9* pViewport) PURE;
048	STDMETHOD(GetViewport)(THIS_ D3DVIEWPORT9* pViewport) PURE;
049	STDMETHOD(SetMaterial)(THIS_ CONST D3DMATERIAL9* pMaterial) PURE;
050	STDMETHOD(GetMaterial)(THIS_ D3DMATERIAL9* pMaterial) PURE;
051	STDMETHOD(SetLight)(THIS_ DWORD Index,CONST D3DLIGHT9*) PURE;
052	STDMETHOD(GetLight)(THIS_ DWORD Index,D3DLIGHT9*) PURE;
053	STDMETHOD(LightEnable)(THIS_ DWORD Index,BOOL Enable) PURE;
054	STDMETHOD(GetLightEnable)(THIS_ DWORD Index,BOOL* pEnable) PURE;
055	STDMETHOD(SetClipPlane)(THIS_ DWORD Index,CONST float* pPlane) PURE;
056	STDMETHOD(GetClipPlane)(THIS_ DWORD Index,float* pPlane) PURE;
057	STDMETHOD(SetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD Value) PURE;
058	STDMETHOD(GetRenderState)(THIS_ D3DRENDERSTATETYPE State,DWORD* pValue) PURE;
059	STDMETHOD(CreateStateBlock)(THIS_ D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB) PURE;
060	STDMETHOD(BeginStateBlock)(THIS) PURE;
061	STDMETHOD(EndStateBlock)(THIS_ IDirect3DStateBlock9** ppSB) PURE;
062	STDMETHOD(SetClipStatus)(THIS_ CONST D3DCLIPSTATUS9* pClipStatus) PURE;
063	STDMETHOD(GetClipStatus)(THIS_ D3DCLIPSTATUS9* pClipStatus) PURE;
064	STDMETHOD(GetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9** ppTexture) PURE;
065	STDMETHOD(SetTexture)(THIS_ DWORD Stage,IDirect3DBaseTexture9* pTexture) PURE;
066	STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) PURE;
067	STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value) PURE;
068	STDMETHOD(GetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue) PURE;
069	STDMETHOD(SetSamplerState)(THIS_ DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value) PURE;
070	STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses) PURE;
071	STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber,CONST PALETTEENTRY* pEntries) PURE;
072	STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber,PALETTEENTRY* pEntries) PURE;
073	STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber) PURE;
074	STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT *PaletteNumber) PURE;
075	STDMETHOD(SetScissorRect)(THIS_ CONST RECT* pRect) PURE;
076	STDMETHOD(GetScissorRect)(THIS_ RECT* pRect) PURE;
077	STDMETHOD(SetSoftwareVertexProcessing)(THIS_ BOOL bSoftware) PURE;
078	STDMETHOD_(BOOL, GetSoftwareVertexProcessing)(THIS) PURE;
079	STDMETHOD(SetNPatchMode)(THIS_ float nSegments) PURE;
080	STDMETHOD_(float, GetNPatchMode)(THIS) PURE;
081	STDMETHOD(DrawPrimitive)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) PURE;
082	STDMETHOD(DrawIndexedPrimitive)(THIS_ D3DPRIMITIVETYPE,INT BaseVertexIndex,UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) PURE;
083	STDMETHOD(DrawPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) PURE;
084	STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) PURE;
085	STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags) PURE;
086	STDMETHOD(CreateVertexDeclaration)(THIS_ CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl) PURE;
087	STDMETHOD(SetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9* pDecl) PURE;
088	STDMETHOD(GetVertexDeclaration)(THIS_ IDirect3DVertexDeclaration9** ppDecl) PURE;
089	STDMETHOD(SetFVF)(THIS_ DWORD FVF) PURE;
090	STDMETHOD(GetFVF)(THIS_ DWORD* pFVF) PURE;
091	STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader) PURE;
092	STDMETHOD(SetVertexShader)(THIS_ IDirect3DVertexShader9* pShader) PURE;
093	STDMETHOD(GetVertexShader)(THIS_ IDirect3DVertexShader9** ppShader) PURE;
094	STDMETHOD(SetVertexShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) PURE;
095	STDMETHOD(GetVertexShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount) PURE;
096	STDMETHOD(SetVertexShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) PURE;
097	STDMETHOD(GetVertexShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount) PURE;
098	STDMETHOD(SetVertexShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) PURE;
099	STDMETHOD(GetVertexShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount) PURE;
100	STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride) PURE;
101	STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* pOffsetInBytes,UINT* pStride) PURE;
102	STDMETHOD(SetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT Setting) PURE;
103	STDMETHOD(GetStreamSourceFreq)(THIS_ UINT StreamNumber,UINT* pSetting) PURE;
104	STDMETHOD(SetIndices)(THIS_ IDirect3DIndexBuffer9* pIndexData) PURE;
105	STDMETHOD(GetIndices)(THIS_ IDirect3DIndexBuffer9** ppIndexData) PURE;
106	STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader) PURE;
107	STDMETHOD(SetPixelShader)(THIS_ IDirect3DPixelShader9* pShader) PURE;
108	STDMETHOD(GetPixelShader)(THIS_ IDirect3DPixelShader9** ppShader) PURE;
109	STDMETHOD(SetPixelShaderConstantF)(THIS_ UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount) PURE;
110	STDMETHOD(GetPixelShaderConstantF)(THIS_ UINT StartRegister,float* pConstantData,UINT Vector4fCount) PURE;
111	STDMETHOD(SetPixelShaderConstantI)(THIS_ UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount) PURE;
112	STDMETHOD(GetPixelShaderConstantI)(THIS_ UINT StartRegister,int* pConstantData,UINT Vector4iCount) PURE;
113	STDMETHOD(SetPixelShaderConstantB)(THIS_ UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount) PURE;
114	STDMETHOD(GetPixelShaderConstantB)(THIS_ UINT StartRegister,BOOL* pConstantData,UINT BoolCount) PURE;
115	STDMETHOD(DrawRectPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) PURE;
116	STDMETHOD(DrawTriPatch)(THIS_ UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) PURE;
117	STDMETHOD(DeletePatch)(THIS_ UINT Handle) PURE;
118	STDMETHOD(CreateQuery)(THIS_ D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery) PURE;

#ifdef D3D_DEBUG_INFO
	D3DDEVICE_CREATION_PARAMETERS CreationParameters;
	D3DPRESENT_PARAMETERS PresentParameters;
	D3DDISPLAYMODE DisplayMode;
	D3DCAPS9 Caps;

	UINT AvailableTextureMem;
	UINT SwapChains;
	UINT Textures;
	UINT VertexBuffers;
	UINT IndexBuffers;
	UINT VertexShaders;
	UINT PixelShaders;

	D3DVIEWPORT9 Viewport;
	D3DMATRIX ProjectionMatrix;
	D3DMATRIX ViewMatrix;
	D3DMATRIX WorldMatrix;
	D3DMATRIX TextureMatrices[8];

	DWORD FVF;
	UINT VertexSize;
	DWORD VertexShaderVersion;
	DWORD PixelShaderVersion;
	BOOL SoftwareVertexProcessing;

	D3DMATERIAL9 Material;
	D3DLIGHT9 Lights[16];
	BOOL LightsEnabled[16];

	D3DGAMMARAMP GammaRamp;
	RECT ScissorRect;
	BOOL DialogBoxMode;
#endif
};

#endif