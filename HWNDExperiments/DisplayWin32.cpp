#include "DisplayWin32.h"
#include <Windows.h>


#include <iostream>
#include "winuser.h"
#include <wrl.h>
#include <chrono>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")

#include <d3d.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <DirectXColors.h>
#include <thread>

#define ZCHECK(exp) if(FAILED(exp)) { printf("Check failed at file: %s at line %i", __FILE__, __LINE__); return 0; }

int winPosX = 0;
int winPosY = 0;
int screenWidth = 800;
int screenHeight = 600;

struct Vertex
{
	float x;
	float y;
};

std::chrono::time_point<std::chrono::steady_clock> PrevTime;
float totalTime = 0;
unsigned int frameCount = 0;

MSG msg{};
HWND hWnd{};

Microsoft::WRL::ComPtr<ID3D11Device> device;
ID3D11DeviceContext* context;
IDXGISwapChain* swapChain;
ID3D11RenderTargetView* rTargetView = nullptr;


void FreeD3D()
{
	if (swapChain != nullptr) {
		swapChain->Release();
	}
	if (rTargetView != nullptr) {
		rTargetView->Release();
	}
	if (context != nullptr) {
		context->Release();
	}
	if (device != nullptr) {
		device->Release();
	}
}

//void clearBuffer(float red, float green, float blue)
//{
//	const float color[] = { red, green, blue, 1.0f };
//	context->ClearRenderTargetView(rTargetView, color);
//}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR szCmdLine, int nCmdShow)
{
//Window Initialization
	WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	wc.hInstance = hInstance;
	wc.lpfnWndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)->LRESULT
	{
		switch (uMsg)
		{
			case WM_QUIT:
			case WM_DESTROY:
			{
				FreeD3D();
				PostQuitMessage(EXIT_SUCCESS);
				return 0;
			}
			default:
			{
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}
		}
		
	};
	wc.lpszClassName = L"AppClass";
	wc.lpszMenuName = nullptr;
	wc.style = CS_VREDRAW | CS_HREDRAW;
	
	if (!RegisterClassEx(&wc))
		return EXIT_FAILURE;

	hWnd = CreateWindow(wc.lpszClassName, L"MainWindow", WS_OVERLAPPEDWINDOW, winPosX, winPosY, screenWidth, screenHeight, nullptr, nullptr, wc.hInstance, nullptr);
	if (hWnd == INVALID_HANDLE_VALUE)
		return EXIT_FAILURE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


//Swap Chain Initialization
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	ZeroMemory(&swapDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapDesc.BufferCount = 2;
	swapDesc.BufferDesc.Width = screenWidth;
	swapDesc.BufferDesc.Height = screenHeight;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 0;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = hWnd;
	swapDesc.Windowed = true;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;

	D3D_FEATURE_LEVEL featureLevel[] = { D3D_FEATURE_LEVEL_11_1 };

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		D3D11_CREATE_DEVICE_DEBUG,
		featureLevel,
		1,
		D3D11_SDK_VERSION,
		&swapDesc,
		&swapChain,
		&device,
		nullptr,
		&context);
	if(device == nullptr){ MessageBox(hWnd, LPCWSTR("1"), NULL, MB_OK); }
	if (swapChain == nullptr) { MessageBox(hWnd, LPCWSTR("2"), NULL, MB_OK); }

	D3D11_TEXTURE2D_DESC backBufferDesc;
	ZeroMemory(&backBufferDesc, sizeof(backBufferDesc));
	backBufferDesc.ArraySize = 1;
	backBufferDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	backBufferDesc.CPUAccessFlags = 0;
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	backBufferDesc.Height = screenHeight;
	backBufferDesc.Width = screenWidth;
	backBufferDesc.MipLevels = 1;
	backBufferDesc.MiscFlags = 0;
	backBufferDesc.SampleDesc.Count = 1;
	backBufferDesc.SampleDesc.Quality = 0;
	backBufferDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D* backBufferTexture;
	device->CreateTexture2D(&backBufferDesc, 0, &backBufferTexture);
	swapChain->GetBuffer(0, __uuidof(ID3D11Resource), (LPVOID*)(&backBufferTexture)); 

	device->CreateRenderTargetView(
		backBufferTexture,
		nullptr, //&rTargetViewDesc,
		&rTargetView
	);
	backBufferTexture->Release();


//Shader Thumba Jumba
	ID3DBlob* vertexBC;
	ID3DBlob* pixelBC;
	ID3D11VertexShader* vertexSh;
	ID3D11PixelShader* pixelSh;

	D3DReadFileToBlob(L"VertexShader.cso", &vertexBC);
	device->CreateVertexShader(
		vertexBC->GetBufferPointer(),
		vertexBC->GetBufferSize(),
		nullptr, &vertexSh);
	D3DReadFileToBlob(L"PixelShader.cso", &pixelBC);
	device->CreatePixelShader(
		pixelBC->GetBufferPointer(),
		pixelBC->GetBufferSize(),
		nullptr, &pixelSh);

	D3D11_INPUT_ELEMENT_DESC inputElements[] = {
	D3D11_INPUT_ELEMENT_DESC {"Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	context->VSSetShader(vertexSh, nullptr, 0);
	context->PSSetShader(pixelSh, nullptr, 0);
//EndShader

	ID3D11InputLayout* layout;
	device->CreateInputLayout(inputElements, 1, vertexBC->GetBufferPointer(), vertexBC->GetBufferSize(), &layout);
	
	D3D11_VIEWPORT viewport = {};
	viewport.Width = (float)screenWidth;
	viewport.Height = (float)screenHeight;
	viewport.MinDepth = 0;
	viewport.MaxDepth = 1;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	context->RSSetViewports(1, &viewport);

//Triangle Vertices List
	Vertex vertices[] = {
		{0.0f, 0.5f},
		{0.5f, -0.5f},
		{-0.5f, -0.5f},
	};

//Buffer Initialization
	UINT stride = sizeof(Vertex);
	UINT offset = (UINT)0;

	ID3D11Buffer* vertexBuffer;
	D3D11_BUFFER_DESC vbd = {};
	vbd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.ByteWidth = sizeof(vertices);
	vbd.StructureByteStride = sizeof(Vertex);
	D3D11_SUBRESOURCE_DATA vsubdata = {};
	vsubdata.pSysMem = vertices;
	device->CreateBuffer(&vbd, &vsubdata, &vertexBuffer);

//Input Assembler Stage
	context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
	context->IASetInputLayout(layout);
	context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	context->OMSetRenderTargets(1, &rTargetView, nullptr);
	
//Main Render Loop
	bool isExitRequested = false;
	while (!isExitRequested) {
	
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {		//if case in constant loop seems horrible but I haven't realized how to make it better yet
				isExitRequested = true;
			}
		}
		
	//Frame & Time Counters
		auto	curTime = std::chrono::steady_clock::now();
		float	deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(curTime - PrevTime).count() / 1000000.0f;
		PrevTime = curTime;

		totalTime += deltaTime;
		frameCount++;

		if (totalTime > 1.0f) {
			float fps = frameCount / totalTime;

			totalTime = 0.0f;

			WCHAR text[256];
			swprintf_s(text, TEXT("FPS: %f"), fps);
			SetWindowText(hWnd, text);

			frameCount = 0;
		}

	//Clear-Draw-Swap Part
		float color[] = { totalTime, 0.3f, 0.4f, 1.0f };
		context->OMSetRenderTargets(1, &rTargetView, nullptr);
		context->ClearRenderTargetView(rTargetView, color);
		context->Draw((UINT)std::size(vertices), 0);
		swapChain->Present(1,0);
	}
	vertexSh->Release();
	pixelSh->Release();
	return 0;
}