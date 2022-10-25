#include <d3d12.h>
#include <array>
#include <imgui\imgui.h>
#include <imgui\imgui_internal.h>
#include <imgui\imgui_impl_dx12.h>
#include <imgui\imgui_impl_win32.h>

#include "../../ModConfig.h"
#include "../Core/Application.h"
#include "../Core/CursorManager.h"
#include "../Core/PlayerGame.h"
#include "../Core/Mover.h"
#include "../PGraphics3D/RenderingDeviceDX12.h"
#include "../PGraphics3D/RenderingConfiguration.h"
#include "../PGraphics3D/SwapChainDX12.h"
#include "../PGraphics3D/HwRenderBuffer.h"

#include "DebugUI.h"
#include "DebugUIWindow.h"
#include "MainMenuBar.h"
#include "EntitySpawnerWindow.h"

#include "ankerl/unordered_dense.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace HRZ::DebugUI
{

	ankerl::unordered_dense::map<std::string, std::shared_ptr<Window>> m_Windows;

	std::array<ID3D12CommandAllocator*, SwapChainDX12::BackBufferCount> CommandAllocators;
	ID3D12GraphicsCommandList* CommandList;
	ID3D12DescriptorHeap* SrvDescHeap;

	WNDPROC OriginalWndProc;
	bool InterceptInput;

	LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	// sakura theme form : https://github.com/VelocityRa/pctation/commit/1df2e91812719e2976fda07c7a90333fcbe1de7d
	void init_theme() 
	{
		// cherry colors, 3 intensities
#define HI(v) ImVec4(0.502f, 0.075f, 0.256f, v)
#define MED(v) ImVec4(0.455f, 0.198f, 0.301f, v)
#define LOW(v) ImVec4(0.232f, 0.201f, 0.271f, v)
// backgrounds (@todo: complete with BG_MED, BG_LOW)
#define BG(v) ImVec4(0.200f, 0.220f, 0.270f, v)
// text
#define TXT(v) ImVec4(0.860f, 0.930f, 0.890f, v)

		auto& style = ImGui::GetStyle();

		style.Colors[ImGuiCol_Text] = TXT(0.78f);
		style.Colors[ImGuiCol_TextDisabled] = TXT(0.28f);
		style.Colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
		style.Colors[ImGuiCol_ChildBg] = BG(0.58f);
		style.Colors[ImGuiCol_PopupBg] = BG(0.9f);
		style.Colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 1.00f, 0.00f);
		style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style.Colors[ImGuiCol_FrameBg] = BG(1.00f);
		style.Colors[ImGuiCol_FrameBgHovered] = MED(0.78f);
		style.Colors[ImGuiCol_FrameBgActive] = MED(1.00f);
		style.Colors[ImGuiCol_TitleBg] = LOW(1.00f);
		style.Colors[ImGuiCol_TitleBgActive] = HI(1.00f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = BG(0.75f);
		style.Colors[ImGuiCol_MenuBarBg] = BG(0.47f);
		style.Colors[ImGuiCol_ScrollbarBg] = BG(1.00f);
		style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.09f, 0.15f, 0.16f, 1.00f);
		style.Colors[ImGuiCol_ScrollbarGrabHovered] = MED(0.78f);
		style.Colors[ImGuiCol_ScrollbarGrabActive] = MED(1.00f);
		style.Colors[ImGuiCol_CheckMark] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
		style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.71f, 0.22f, 0.27f, 1.00f);
		style.Colors[ImGuiCol_Button] = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
		style.Colors[ImGuiCol_ButtonHovered] = MED(0.86f);
		style.Colors[ImGuiCol_ButtonActive] = MED(1.00f);
		style.Colors[ImGuiCol_Header] = MED(0.76f);
		style.Colors[ImGuiCol_HeaderHovered] = MED(0.86f);
		style.Colors[ImGuiCol_HeaderActive] = HI(1.00f);
		style.Colors[ImGuiCol_Separator] = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
		style.Colors[ImGuiCol_SeparatorHovered] = MED(0.78f);
		style.Colors[ImGuiCol_SeparatorActive] = MED(1.00f);
		style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
		style.Colors[ImGuiCol_ResizeGripHovered] = MED(0.78f);
		style.Colors[ImGuiCol_ResizeGripActive] = MED(1.00f);
		style.Colors[ImGuiCol_PlotLines] = TXT(0.63f);
		style.Colors[ImGuiCol_PlotLinesHovered] = MED(1.00f);
		style.Colors[ImGuiCol_PlotHistogram] = TXT(0.63f);
		style.Colors[ImGuiCol_PlotHistogramHovered] = MED(1.00f);
		style.Colors[ImGuiCol_TextSelectedBg] = MED(0.43f);
		style.Colors[ImGuiCol_ModalWindowDimBg] = BG(0.73f);

		style.WindowPadding = ImVec2(6, 4);
		style.WindowRounding = 0.0f;
		style.FramePadding = ImVec2(5, 2);
		style.FrameRounding = 3.0f;
		style.ItemSpacing = ImVec2(7, 1);
		style.ItemInnerSpacing = ImVec2(1, 1);
		style.TouchExtraPadding = ImVec2(0, 0);
		style.IndentSpacing = 6.0f;
		style.ScrollbarSize = 12.0f;
		style.ScrollbarRounding = 16.0f; 
		style.GrabMinSize = 20.0f;
		style.GrabRounding = 2.0f;

		style.WindowTitleAlign.x = 0.50f;

		style.Colors[ImGuiCol_Border] = ImVec4(0.539f, 0.479f, 0.255f, 0.162f);
		style.FrameBorderSize = 0.0f; 
		style.WindowBorderSize = 1.0f;
	}



	void Initialize(const SwapChainDX12* SwapChain)
	{
		// D3D resources
		HRESULT hr = S_OK;
		auto device = RenderingDeviceDX12::Instance().m_Device;

		D3D12_DESCRIPTOR_HEAP_DESC desc
		{
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 10,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		};

		hr |= device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&SrvDescHeap));

		for (size_t i = 0; i < CommandAllocators.size(); i++)
			hr |= device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocators[i]));

		hr |= device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0], nullptr, IID_PPV_ARGS(&CommandList));
		hr |= CommandList->Close();

		if (FAILED(hr))
			__debugbreak();

		// ImGui resources
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		init_theme();

		auto& io = ImGui::GetIO();
		io.FontGlobalScale = ModConfiguration.DebugMenuFontScale;
		io.MouseDrawCursor = false;

		ImGui_ImplWin32_Init(SwapChain->m_NativeWindowHandle);
		ImGui_ImplDX12_Init(device, SwapChainDX12::BackBufferCount, SwapChain->m_BackBuffers[0].m_Resource->GetDesc().Format, SrvDescHeap, SrvDescHeap->GetCPUDescriptorHandleForHeapStart(), SrvDescHeap->GetGPUDescriptorHandleForHeapStart());
		OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(SwapChain->m_NativeWindowHandle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc)));

		DebugUI::AddWindow(std::make_shared<MainMenuBar>());
	}

	void AddWindow(std::shared_ptr<Window> Handle)
	{
		// Immediately discard duplicate window instances
		auto id = Handle->GetId();

		if (!m_Windows.contains(id))
			m_Windows.emplace(id, Handle);
	}

	void RenderUI()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		UpdateFreecam();

		// Clear window focus for every frame that isn't intercepting input
		if (!InterceptInput)
			ImGui::FocusWindow(nullptr);

		// A copy is required because Render() might create new instances and invalidate iterators
		auto currentWindows = m_Windows;

		for (auto& [name, window] : currentWindows)
		{
			// Detach windows that are pending close first
			if (window->Close())
				m_Windows.erase(name);
			else
				window->Render();
		}

		ImGui::Render();
	}

	void RenderUID3D(const SwapChainDX12* SwapChain)
	{
		uint32_t bufferIndex = (SwapChain->m_CurrentFrameIndex - 1) % SwapChainDX12::BackBufferCount;
		ID3D12CommandAllocator* allocator = CommandAllocators[bufferIndex];

		allocator->Reset();

		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = SwapChain->m_BackBuffers[bufferIndex].m_Resource;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

		CommandList->Reset(allocator, nullptr);
		CommandList->ResourceBarrier(1, &barrier);

		CommandList->OMSetRenderTargets(1, &SwapChain->m_BackBuffers[bufferIndex].m_RenderBuffer->m_CPUDescriptorHandle, false, nullptr);
		CommandList->SetDescriptorHeaps(1, &SrvDescHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), CommandList);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		CommandList->ResourceBarrier(1, &barrier);
		CommandList->Close();

		SwapChain->m_RenderingConfig->GetCommandQueue()->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&CommandList));
	}

	std::optional<LRESULT> HandleMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		switch (Msg)
		{
		case WM_KEYDOWN:
		{
			bool keyHandled = true;

			if (wParam == ModConfiguration.Hotkeys.ToggleDebugUI)
			{
				// Toggle input blocking and the main menu bar
				ToggleInputInterception();
				MainMenuBar::ToggleVisibility();

				CallWindowProcW(OriginalWndProc, hWnd, WM_ACTIVATEAPP, ShouldInterceptInput() ? 0 : 1, 0);
			}
			else if (wParam == ModConfiguration.Hotkeys.TogglePauseGameLogic)
				MainMenuBar::TogglePauseGameLogic();
			else if (wParam == ModConfiguration.Hotkeys.TogglePauseTimeOfDay)
				MainMenuBar::TogglePauseTimeOfDay();
			else if (wParam == ModConfiguration.Hotkeys.ToggleFreeflyCamera)
				MainMenuBar::ToggleFreeflyCamera();
			else if (wParam == ModConfiguration.Hotkeys.ToggleNoclip)
				MainMenuBar::ToggleNoclip();
			else if (wParam == ModConfiguration.Hotkeys.SaveQuicksave)
				MainMenuBar::SavePlayerGame(MainMenuBar::SaveType::Quick);
			else if (wParam == ModConfiguration.Hotkeys.LoadPreviousSave)
				MainMenuBar::LoadPreviousSave();
			else if (wParam == ModConfiguration.Hotkeys.SpawnEntity)
				EntitySpawnerWindow::ForceSpawnEntityClick();
			else if (wParam == ModConfiguration.Hotkeys.IncreaseTimescale)
				MainMenuBar::AdjustTimescale(0.25f);
			else if (wParam == ModConfiguration.Hotkeys.DecreaseTimescale)
				MainMenuBar::AdjustTimescale(-0.25f);
			else if (wParam == ModConfiguration.Hotkeys.IncreaseTimeOfDay)
				MainMenuBar::AdjustTimeOfDay(1.0f);
			else if (wParam == ModConfiguration.Hotkeys.DecreaseTimeOfDay)
				MainMenuBar::AdjustTimeOfDay(-1.0f);
			else
				keyHandled = false;

			if (keyHandled)
				return 1;
		}
		break;

		case WM_ACTIVATEAPP:
		{
			// Prevent alt-tab from interfering with input blocking
			if (ShouldInterceptInput())
				return 1;
		}
		break;
		}

		return std::nullopt;
	}

	void ToggleInputInterception()
	{
		InterceptInput = !InterceptInput;
		auto cursorManager = Application::Instance().m_CursorManager;

		if (InterceptInput)
		{
			ImGui::GetIO().MouseDrawCursor = true;
			cursorManager->m_UnlockCursorBounds = true;
			cursorManager->m_ForceHideCursor = true;
			ImGui::CaptureMouseFromApp(true);
		}
		else
		{
			ImGui::GetIO().MouseDrawCursor = false;
			cursorManager->m_UnlockCursorBounds = false;
			cursorManager->m_ForceHideCursor = false;
			ImGui::CaptureMouseFromApp(false);
		}

		// Apply freecam overrides (no visible cursor, but input still blocked)
		if (MainMenuBar::m_FreeCamMode == MainMenuBar::FreeCamMode::Free)
			cursorManager->m_UnlockCursorBounds = true;
	}

	bool ShouldInterceptInput()
	{
		return InterceptInput || MainMenuBar::m_FreeCamMode == MainMenuBar::FreeCamMode::Free;
	}

	void UpdateFreecam()
	{
		auto cameraMode = MainMenuBar::m_FreeCamMode;
		auto& cameraPosition = MainMenuBar::m_FreeCamPosition;

		if (cameraMode == MainMenuBar::FreeCamMode::Off)
			return;

		if (!Application::IsInGame())
			return;

		auto player = Player::GetLocalPlayer();

		if (!player)
			return;

		auto camera = player->GetLastActivatedCamera();

		if (!camera)
			return;

		auto& io = ImGui::GetIO();

		// Set up the camera's rotation matrix
		RotMatrix cameraMatrix;
		float yaw = 0.0f;
		float pitch = 0.0f;

		if (cameraMode == MainMenuBar::FreeCamMode::Free)
		{
			// Convert mouse X/Y to yaw/pitch angles
			static float currentCursorX = 0.0f;
			static float currentCursorY = 0.0f;
			static float targetCursorX = 0.0f;
			static float targetCursorY = 0.0f;

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
			{
				targetCursorX += io.MouseDelta.x * 0.5f;
				targetCursorY += io.MouseDelta.y * 0.5f;
			}

			// Exponential decay view angle smoothing (https://stackoverflow.com/a/10228863)
			const double springiness = 60.0;
			const float mult = static_cast<float>(1.0 - std::exp(std::log(0.5) * springiness * io.DeltaTime));

			currentCursorX += (targetCursorX - currentCursorX) * mult;
			currentCursorY += (targetCursorY - currentCursorY) * mult;

			float degreesX = std::fmodf(currentCursorX, 360.0f);
			if (degreesX < 0) degreesX += 360.0f;
			float degreesY = std::fmodf(currentCursorY, 360.0f);
			if (degreesY < 0) degreesY += 360.0f;

			// Degrees to radians
			yaw = degreesX * (3.14159f / 180.0f);
			pitch = degreesY * (3.14159f / 180.0f);

			cameraMatrix = RotMatrix(yaw, pitch, 0.0f);
		}
		else if (cameraMode == MainMenuBar::FreeCamMode::Noclip)
		{
			std::scoped_lock lock(camera->m_DataLock);

			// Convert matrix components to angles
			cameraMatrix = camera->m_Orientation.Orientation;
			cameraMatrix.Decompose(&yaw, &pitch, nullptr);
		}

		float speed = io.DeltaTime * 5.0f;

		if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
		{
			speed *= 10.0f;
		}
		else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
		{
			speed /= 5.0f;
		}	

		// WSAD keys for movement
		Vec3 moveDirection(sin(yaw) * cos(pitch), cos(yaw) * cos(pitch), -sin(pitch));

		if (ImGui::IsKeyDown(ImGuiKey_W))
		{
			cameraPosition += moveDirection * speed;
		}

		if (ImGui::IsKeyDown(ImGuiKey_S))
		{
			cameraPosition -= moveDirection * speed;
		}

		if (ImGui::IsKeyDown(ImGuiKey_A))
		{
			cameraPosition -= moveDirection.CrossProduct(Vec3(0, 0, 1)) * speed;
		}

		if (ImGui::IsKeyDown(ImGuiKey_D))
		{
			cameraPosition += moveDirection.CrossProduct(Vec3(0, 0, 1)) * speed;
		}

		WorldTransform newTransform
		{
			.Position = cameraPosition,
			.Orientation = cameraMatrix,
		};

		if (cameraMode == MainMenuBar::FreeCamMode::Free)
		{
			std::scoped_lock lock(camera->m_DataLock);

			camera->m_PreviousOrientation = newTransform;
			camera->m_Orientation = newTransform;
			camera->m_Flags |= Entity::WorldTransformChanged;
		}
		else if (cameraMode == MainMenuBar::FreeCamMode::Noclip)
		{
			player->m_Entity->m_Mover->MoveToWorldTransform(newTransform, 0.01f, false);
		}
	}

	LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		// Intercepted messages will prevent them from being forwarded to imgui and the game
		auto handled = HandleMessage(hWnd, Msg, wParam, lParam);

		if (handled)
			return handled.value();

		ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);
		return CallWindowProcW(OriginalWndProc, hWnd, Msg, wParam, lParam);
	}

}