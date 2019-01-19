#include <Win32/geMinWindows.h>
#include <gePrerequisitesUtil.h>
#include <geMatrix4.h>
#include <geVector2.h>
#include <geVector3.h>
#include <geRotator.h>
#include <geColor.h>

#include <geTime.h>
#include <geDynLibManager.h>

//#include <geD3D11Driver.h>
//#include <geD3D11DriverList.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <vld.h>

#include "GraphicsBuffer.h"

using namespace geEngineSDK;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
  Vector3 Pos;
  Vector2 Tex;
};

struct CBNeverChanges
{
  Matrix4 mView;
};

struct CBChangeOnResize
{
  Matrix4 mProjection;
};

struct CBChangesEveryFrame
{
  Matrix4 mWorld;
  LinearColor vMeshColor;
};

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = nullptr;
HWND                                g_hWnd = nullptr;
D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device*                       g_pd3dDevice = nullptr;
ID3D11DeviceContext*                g_pImmediateContext = nullptr;
IDXGISwapChain*                     g_pSwapChain = nullptr;
ID3D11RenderTargetView*             g_pRenderTargetView = nullptr;
ID3D11Texture2D*                    g_pDepthStencil = nullptr;
ID3D11DepthStencilView*             g_pDepthStencilView = nullptr;
ID3D11VertexShader*                 g_pVertexShader = nullptr;
ID3D11PixelShader*                  g_pPixelShader = nullptr;
ID3D11InputLayout*                  g_pVertexLayout = nullptr;
ID3D11Buffer*                       g_pVertexBuffer = nullptr;
ID3D11Buffer*                       g_pIndexBuffer = nullptr;
ID3D11Buffer*                       g_pCBNeverChanges = nullptr;
ID3D11Buffer*                       g_pCBChangeOnResize = nullptr;
ID3D11Buffer*                       g_pCBChangesEveryFrame = nullptr;
//ID3D11ShaderResourceView*           g_pTextureRV = nullptr;
ID3D11SamplerState*                 g_pSamplerLinear = nullptr;

Matrix4                             g_World;
Matrix4                             g_View;
Matrix4                             g_Projection;
LinearColor                         g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f);

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT
InitWindow(HINSTANCE hInstance, int nCmdShow);

HRESULT
InitDevice();

void
CleanupDevice();

LRESULT CALLBACK
WndProc(HWND, UINT, WPARAM, LPARAM);

void
Render();

//Tests
void
testLookupTable();

int
main() {
  CrashHandler::startUp();
  DynLibManager::startUp();
  Time::startUp();

  if (FAILED(InitWindow(GetModuleHandle(nullptr), SW_SHOW))) {
    return 0;
  }

  if (FAILED(InitDevice())) {
    CleanupDevice();
    return 0;
  }

  //Main message loop
  MSG msg;
  msg.message = WM_NULL;

  while (WM_QUIT != msg.message) {
    g_time()._update();

    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    Render();
  }

  CleanupDevice();

  DynLibManager::shutDown();
  Time::shutDown();
  CrashHandler::shutDown();

  return (int)msg.wParam;
}

//Called every time the application receives a message
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  HDC hdc;

  switch (message)
  {
    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }

  return 0;
}

//Register class and create window
HRESULT
InitWindow(HINSTANCE hInstance, int nCmdShow) {
  //Register class
  WNDCLASSEX wcex;
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc = WndProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = hInstance;
  wcex.hIcon = nullptr;
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName = nullptr;
  wcex.lpszClassName = "TutorialWindowClass";
  wcex.hIconSm = nullptr;

  if (!RegisterClassEx(&wcex)) {
    return E_FAIL;
  }

  //Create window
  g_hInst = hInstance;
  RECT rc = { 0, 0, 1920, 1080 };
  AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
  g_hWnd = CreateWindow("TutorialWindowClass",
                        "Direct3D 11 Test",
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        rc.right - rc.left,
                        rc.bottom - rc.top,
                        nullptr,
                        nullptr,
                        hInstance,
                        nullptr);

  if (!g_hWnd) {
    return E_FAIL;
  }

  ShowWindow(g_hWnd, nCmdShow);

  return S_OK;
}

//Helper for compiling shaders with D3DX11
HRESULT
CompileShaderFromFile(Path fileName,
                      String szEntryPoint,
                      String szShaderModel,
                      ID3DBlob** ppBlobOut) {
  HRESULT hr = S_OK;
  int32 dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if GE_DEBUG_MODE
  dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

  ID3DBlob* pErrorBlob = nullptr;
  hr = D3DCompileFromFile(fileName.toPlatformString().c_str(),
                          nullptr,
                          nullptr,
                          szEntryPoint.c_str(),
                          szShaderModel.c_str(),
                          dwShaderFlags,
                          0,
                          ppBlobOut,
                          &pErrorBlob);

  if (FAILED(hr)) {
    if (nullptr != pErrorBlob) {
      OutputDebugStringA(reinterpret_cast<char*>(pErrorBlob->GetBufferPointer()));
    }

    if (pErrorBlob) {
      pErrorBlob->Release();
    }

    return hr;
  }

  if (pErrorBlob) {
    pErrorBlob->Release();
  }

  return S_OK;
}

//using namespace geCoreThread;

//Create Direct3D device and swap chain
HRESULT
InitDevice() {
  HRESULT hr = S_OK;

  IDXGIFactory* dxgiFactory = nullptr;
  hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&dxgiFactory);
  if (FAILED(hr)) {
    GE_EXCEPT(RenderingAPIException,
              "Failed to create Direct3D11 DXGIFactory");
  }

  /*
  D3D11DriverList* driverList = ge_new<D3D11DriverList>(dxgiFactory);
  D3D11Driver* activeD3DDriver = driverList->item(0); //TODO: Always get first driver, for now
  SPtr<VideoModeInfo> videoModeInfo = activeD3DDriver->getVideoModeInfo();

  GPUInfo gpuInfo;
  gpuInfo.numGPUs = Math::min(5U, driverList->count());

  for (uint32 i = 0; i < gpuInfo.numGPUs; ++i) {
    gpuInfo.names[i] = driverList->item(i)->getDriverName();
  }

  PlatformUtility::_setGPUInfo(gpuInfo);
  */

  RECT rc;
  GetClientRect(g_hWnd, &rc);
  uint32 width = rc.right - rc.left;
  uint32 height = rc.bottom - rc.top;

  uint32 createDeviceFlags = 0;
#ifdef GE_DEBUG_MODE
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  Vector<D3D_DRIVER_TYPE> driverTypes;
  driverTypes.push_back(D3D_DRIVER_TYPE_HARDWARE);
  driverTypes.push_back(D3D_DRIVER_TYPE_WARP);
  driverTypes.push_back(D3D_DRIVER_TYPE_REFERENCE);

  Vector<D3D_FEATURE_LEVEL> featureLevels;
  featureLevels.push_back(D3D_FEATURE_LEVEL_11_0);
  featureLevels.push_back(D3D_FEATURE_LEVEL_10_1);
  featureLevels.push_back(D3D_FEATURE_LEVEL_10_0);

  DXGI_SWAP_CHAIN_DESC sd;
  memset(&sd, 0, sizeof(sd));
  sd.BufferCount = 1;
  sd.BufferDesc.Width = width;
  sd.BufferDesc.Height = height;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferDesc.RefreshRate.Numerator = 60;
  sd.BufferDesc.RefreshRate.Denominator = 1;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = g_hWnd;
  sd.SampleDesc.Count = 1;
  sd.SampleDesc.Quality = 0;
  sd.Windowed = TRUE;

  for (auto & driverType : driverTypes) {
    g_driverType = driverType;
    hr = D3D11CreateDeviceAndSwapChain(nullptr,
                                       g_driverType,
                                       nullptr,
                                       createDeviceFlags,
                                       &featureLevels[0],
                                       static_cast<uint32>(featureLevels.size()),
                                       D3D11_SDK_VERSION,
                                       &sd,
                                       &g_pSwapChain,
                                       &g_pd3dDevice,
                                       &g_featureLevel,
                                       &g_pImmediateContext);
    if (SUCCEEDED(hr)) {
      break;
    }
  }

  if (FAILED(hr)) {
    return hr;
  }

  //Create a render target view
  ID3D11Texture2D* pBackBuffer = nullptr;
  hr = g_pSwapChain->GetBuffer(0,
                               __uuidof(ID3D11Texture2D),
                               reinterpret_cast<LPVOID*>(&pBackBuffer));
  if (FAILED(hr)) {
    return hr;
  }

  hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer,
                                            nullptr,
                                            &g_pRenderTargetView);
  pBackBuffer->Release();
  if (FAILED(hr)) {
    return hr;
  }

  //Create depth stencil texture
  D3D11_TEXTURE2D_DESC descDepth;
  ZeroMemory(&descDepth, sizeof(descDepth));
  descDepth.Width = width;
  descDepth.Height = height;
  descDepth.MipLevels = 1;
  descDepth.ArraySize = 1;
  descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  descDepth.SampleDesc.Count = 1;
  descDepth.SampleDesc.Quality = 0;
  descDepth.Usage = D3D11_USAGE_DEFAULT;
  descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  descDepth.CPUAccessFlags = 0;
  descDepth.MiscFlags = 0;

  hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
  if (FAILED(hr)) {
    return hr;
  }

  // Create the depth stencil view
  D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
  ZeroMemory(&descDSV, sizeof(descDSV));

  descDSV.Format = descDepth.Format;
  descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
  descDSV.Texture2D.MipSlice = 0;

  hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil,
                                            &descDSV,
                                            &g_pDepthStencilView);
  if (FAILED(hr)) {
    return hr;
  }

  g_pImmediateContext->OMSetRenderTargets(1,
                                          &g_pRenderTargetView,
                                          g_pDepthStencilView);

  // Setup the viewport
  D3D11_VIEWPORT vp;
  vp.Width = (FLOAT)width;
  vp.Height = (FLOAT)height;
  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;
  g_pImmediateContext->RSSetViewports(1, &vp);

  // Compile the vertex shader
  ID3DBlob* pVSBlob = nullptr;
  hr = CompileShaderFromFile("Test/Tutorial07.fx", "VS", "vs_5_0", &pVSBlob);
  if (FAILED(hr))
  {
    MessageBox(nullptr,
               "The FX file cannot be compiled. "
                 "Please run this executable from the directory that contains "
                 "the FX file.",
               "Error",
               MB_OK);
    return hr;
  }

  // Create the vertex shader
  hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(),
                                        pVSBlob->GetBufferSize(),
                                        nullptr,
                                        &g_pVertexShader);
  if (FAILED(hr))
  {
    pVSBlob->Release();
    return hr;
  }

  // Define the input layout
  Vector<D3D11_INPUT_ELEMENT_DESC> layout;
  layout.push_back({ "POSITION",
                     0,
                     DXGI_FORMAT_R32G32B32_FLOAT,
                     0,
                     0,
                     D3D11_INPUT_PER_VERTEX_DATA,
                     0 });

  layout.push_back({ "TEXCOORD",
                     0,
                     DXGI_FORMAT_R32G32_FLOAT,
                     0,
                     12,
                     D3D11_INPUT_PER_VERTEX_DATA,
                     0 });

  // Create the input layout
  hr = g_pd3dDevice->CreateInputLayout(&layout[0],
                                       static_cast<uint32>(layout.size()),
                                       pVSBlob->GetBufferPointer(),
                                       pVSBlob->GetBufferSize(),
                                       &g_pVertexLayout);
  pVSBlob->Release();
  if (FAILED(hr)) {
    return hr;
  }

  // Set the input layout
  g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

  // Compile the pixel shader
  ID3DBlob* pPSBlob = nullptr;
  hr = CompileShaderFromFile("Test\\Tutorial07.fx", "PS", "ps_5_0", &pPSBlob);
  if (FAILED(hr)) {
    MessageBox(nullptr,
               "The FX file cannot be compiled. "
                 "Please run this executable from the directory that "
                 "contains the FX file.",
               "Error",
               MB_OK);
    return hr;
  }

  // Create the pixel shader
  hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(),
                                       pPSBlob->GetBufferSize(),
                                       nullptr,
                                       &g_pPixelShader);
  pPSBlob->Release();
  if (FAILED(hr)) {
    return hr;
  }

  // Create vertex buffer
  Vector<SimpleVertex> vertices;
  vertices.reserve(24);

  vertices.push_back({ Vector3(-1.0f, 1.0f, -1.0f), Vector2(0.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, 1.0f, -1.0f), Vector2(1.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, 1.0f, 1.0f), Vector2(1.0f, 1.0f) });
  vertices.push_back({ Vector3(-1.0f, 1.0f, 1.0f), Vector2(0.0f, 1.0f) });

  vertices.push_back({ Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, -1.0f, -1.0f), Vector2(1.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, -1.0f, 1.0f), Vector2(1.0f, 1.0f) });
  vertices.push_back({ Vector3(-1.0f, -1.0f, 1.0f), Vector2(0.0f, 1.0f) });

  vertices.push_back({ Vector3(-1.0f, -1.0f, 1.0f), Vector2(0.0f, 0.0f) });
  vertices.push_back({ Vector3(-1.0f, -1.0f, -1.0f), Vector2(1.0f, 0.0f) });
  vertices.push_back({ Vector3(-1.0f, 1.0f, -1.0f), Vector2(1.0f, 1.0f) });
  vertices.push_back({ Vector3(-1.0f, 1.0f, 1.0f), Vector2(0.0f, 1.0f) });

  vertices.push_back({ Vector3(1.0f, -1.0f, 1.0f), Vector2(0.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, -1.0f, -1.0f), Vector2(1.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, 1.0f, -1.0f), Vector2(1.0f, 1.0f) });
  vertices.push_back({ Vector3(1.0f, 1.0f, 1.0f), Vector2(0.0f, 1.0f) });

  vertices.push_back({ Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, -1.0f, -1.0f), Vector2(1.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, 1.0f, -1.0f), Vector2(1.0f, 1.0f) });
  vertices.push_back({ Vector3(-1.0f, 1.0f, -1.0f), Vector2(0.0f, 1.0f) });

  vertices.push_back({ Vector3(-1.0f, -1.0f, 1.0f), Vector2(0.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, -1.0f, 1.0f), Vector2(1.0f, 0.0f) });
  vertices.push_back({ Vector3(1.0f, 1.0f, 1.0f), Vector2(1.0f, 1.0f) });
  vertices.push_back({ Vector3(-1.0f, 1.0f, 1.0f), Vector2(0.0f, 1.0f) });

  D3D11_BUFFER_DESC bd;
  ZeroMemory(&bd, sizeof(bd));
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = static_cast<uint32>(sizeof(SimpleVertex) * vertices.size());
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bd.CPUAccessFlags = 0;
  D3D11_SUBRESOURCE_DATA InitData;
  ZeroMemory(&InitData, sizeof(InitData));
  InitData.pSysMem = &vertices[0];
  hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
  if (FAILED(hr)) {
    return hr;
  }

  //Set vertex buffer
  uint32 stride = sizeof(SimpleVertex);
  uint32 offset = 0;
  g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

  //Create index buffer
  //Create vertex buffer
  Vector<uint16> indices;
  indices.reserve(36);

  indices.push_back(3);
  indices.push_back(1);
  indices.push_back(0);

  indices.push_back(2);
  indices.push_back(1);
  indices.push_back(3);

  indices.push_back(6);
  indices.push_back(4);
  indices.push_back(5);

  indices.push_back(7);
  indices.push_back(4);
  indices.push_back(6);

  indices.push_back(11);
  indices.push_back(9);
  indices.push_back(8);

  indices.push_back(10);
  indices.push_back(9);
  indices.push_back(11);

  indices.push_back(14);
  indices.push_back(12);
  indices.push_back(13);

  indices.push_back(15);
  indices.push_back(12);
  indices.push_back(14);

  indices.push_back(19);
  indices.push_back(17);
  indices.push_back(16);

  indices.push_back(18);
  indices.push_back(17);
  indices.push_back(19);

  indices.push_back(22);
  indices.push_back(20);
  indices.push_back(21);

  indices.push_back(23);
  indices.push_back(20);
  indices.push_back(22);

  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = static_cast<uint32>(sizeof(uint16) * indices.size());
  bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  bd.CPUAccessFlags = 0;
  InitData.pSysMem = &indices[0];
  hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
  if (FAILED(hr)) {
    return hr;
  }

  //Set index buffer
  g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

  //Set primitive topology
  g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  // Create the constant buffers
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = sizeof(CBNeverChanges);
  bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bd.CPUAccessFlags = 0;
  hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBNeverChanges);
  if (FAILED(hr)) {
    return hr;
  }

  bd.ByteWidth = sizeof(CBChangeOnResize);
  hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangeOnResize);
  if (FAILED(hr)) {
    return hr;
  }

  bd.ByteWidth = sizeof(CBChangesEveryFrame);
  hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangesEveryFrame);
  if (FAILED(hr)) {
    return hr;
  }

  // Load the Texture
  //hr = D3DCreateShaderResourceViewFromFile(g_pd3dDevice,
  //                                         L"seafloor.dds",
  //                                         NULL,
  //                                         NULL,
  //                                         &g_pTextureRV,
  //                                         NULL);
  //if (FAILED(hr))
  //  return hr;

  // Create the sample state
  D3D11_SAMPLER_DESC sampDesc;
  ZeroMemory(&sampDesc, sizeof(sampDesc));
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD = 0;
  sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

  hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
  if (FAILED(hr)) {
    return hr;
  }

  //Initialize the world matrices
  g_World.setIdentity();

  //Initialize the view matrix
  Vector3 Eye = (Vector3::FORWARD * -6.0f) + (Vector3::UP * 3.0f);
  Vector3 At(0.0f, 0.0f, 0.0f);
  g_View = LookAtMatrix(Eye, At, Vector3::UP);

  CBNeverChanges cbNeverChanges;
  cbNeverChanges.mView = g_View.getTransposed();
  g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges,
                                         0,
                                         nullptr,
                                         &cbNeverChanges,
                                         0,
                                         0);

  // Initialize the projection matrix
  g_Projection = PerspectiveMatrix(Math::HALF_PI * 0.5f,
                                   static_cast<float>(width),
                                   static_cast<float>(height),
                                   0.01f,
                                   100.0f);

  CBChangeOnResize cbChangesOnResize;
  cbChangesOnResize.mProjection = g_Projection.getTransposed();
  g_pImmediateContext->UpdateSubresource(g_pCBChangeOnResize,
                                         0,
                                         nullptr,
                                         &cbChangesOnResize,
                                         0,
                                         0);

  srand(GetTickCount());
  g_vMeshColor = LinearColor::makeRandomColor();

  return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void
CleanupDevice() {
  if (g_pImmediateContext) g_pImmediateContext->ClearState();

  if (g_pSamplerLinear) g_pSamplerLinear->Release();
  //if (g_pTextureRV) g_pTextureRV->Release();
  if (g_pCBNeverChanges) g_pCBNeverChanges->Release();
  if (g_pCBChangeOnResize) g_pCBChangeOnResize->Release();
  if (g_pCBChangesEveryFrame) g_pCBChangesEveryFrame->Release();
  if (g_pVertexBuffer) g_pVertexBuffer->Release();
  if (g_pIndexBuffer) g_pIndexBuffer->Release();
  if (g_pVertexLayout) g_pVertexLayout->Release();
  if (g_pVertexShader) g_pVertexShader->Release();
  if (g_pPixelShader) g_pPixelShader->Release();
  if (g_pDepthStencil) g_pDepthStencil->Release();
  if (g_pDepthStencilView) g_pDepthStencilView->Release();
  if (g_pRenderTargetView) g_pRenderTargetView->Release();
  if (g_pSwapChain) g_pSwapChain->Release();
  if (g_pImmediateContext) g_pImmediateContext->Release();
  if (g_pd3dDevice) g_pd3dDevice->Release();
}


//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void
Render() {
  // Rotate cube around the origin
  Rotator cubeRotation(0, Radian(g_time().getTime()).valueDegrees(), 0);
  g_World = RotationMatrix(cubeRotation);

  // Modify the color
  //g_vMeshColor = LinearColor::makeRandomColor();

  // Clear the back buffer
  LinearColor ClearColor(0.0f, 0.125f, 0.3f, 1.0f); // red, green, blue, alpha
  g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor.ptr());

  // Clear the depth buffer to 1.0 (max depth)
  g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

  // Update variables that change once per frame
  CBChangesEveryFrame cb;
  cb.mWorld = g_World.getTransposed();
  cb.vMeshColor = g_vMeshColor;
  g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

  // Render the cube
  g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
  g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBNeverChanges);
  g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCBChangeOnResize);
  g_pImmediateContext->VSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
  g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
  g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
  //g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
  g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
  g_pImmediateContext->DrawIndexed(36, 0, 0);

  //
  // Present our back buffer to our front buffer
  //
  g_pSwapChain->Present(DXGI_SWAP_EFFECT_DISCARD, DXGI_PRESENT_DO_NOT_WAIT);
}
