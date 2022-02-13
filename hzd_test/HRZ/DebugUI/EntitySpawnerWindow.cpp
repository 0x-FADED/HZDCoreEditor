#include <mutex>
#include <robin_hood_hashing/include/robin_hood.h>
#include <algorithm>
#include <imgui.h>

#include "../../ModConfig.h"
#include "../Core/Application.h"
#include "../Core/Resource.h"
#include "../Core/Player.h"
#include "../Core/StreamingManager.h"
#include "../Core/PrefetchList.h"

#include "DebugUI.h"
#include "EntitySpawnerWindow.h"

namespace HRZ
{
DECL_RTTI(Spawnpoint);
DECL_RTTI(SpawnSetupBase);
}

namespace HRZ::DebugUI
{

robin_hood::unordered_map<GGUUID, StreamingRef<Resource>> CurrentlyLoadedSpawnSetups;

void DebugUI::EntitySpawnerLoaderCallback::OnStreamingRefLoad(RTTIRefObject *Object)
{
	if (!Object)
		return;

	uint32_t count = m_NextSpawnCount.exchange(0);

	if (count <= 0)
		return;

	Application::RunOnMainThread([count, transform = m_NextSpawnTransform, refObj = Ref<RTTIRefObject>(Object)]()
	{
		for (uint32_t i = 0; i < count; i++)
		{
			auto spawnpoint = Offsets::CallID<"RTTI::CreateObject", RTTIRefObject *(*)(const RTTI *)>(RTTI_Spawnpoint);
			auto rtti = spawnpoint->GetRTTI()->AsClass();

			spawnpoint->IncRef();
			rtti->SetMemberValue<GGUUID>(spawnpoint, "ObjectUUID", GGUUID::Generate());
			rtti->SetMemberValue<WorldTransform>(spawnpoint, "Orientation", transform);
			rtti->SetMemberValue<String>(spawnpoint, "Name", "DebugUI_Manually_Spawned_Entity");
			rtti->SetMemberValue<Ref<Resource>>(spawnpoint, "SpawnSetup", RTTI::Cast<Resource>(refObj.get()));
			rtti->SetMemberValue<float>(spawnpoint, "Radius", 1.0f);
			rtti->SetMemberValue<float>(spawnpoint, "DespawnRadius", 0.0f);
			// WeakPtr<Entity> @ Spawnpoint+0x188

			Offsets::CallID<"NodeGraph::ExportedSpawnpointSpawn", void(*)(RTTIRefObject *)>(spawnpoint);
			spawnpoint->DecRef();
		}
	});
}

void DebugUI::EntitySpawnerLoaderCallback::OnStreamingRefUnload()
{
}

void DebugUI::EntitySpawnerLoaderCallback::DoSpawn(const std::string_view CorePath, const std::string_view UUID)
{
	auto uuid = GGUUID::Parse(UUID);

	if (auto itr = CurrentlyLoadedSpawnSetups.find(uuid); itr != CurrentlyLoadedSpawnSetups.end())
	{
		// Instance already loaded. Spawn a copy.
		OnStreamingRefLoad(itr->second.get());
	}
	else
	{
		// Instance has to be loaded
		StreamingRef<Resource> resourceHandle;
		StreamingManager::Instance()->CreateHandleFromPath(resourceHandle, AssetPath{ CorePath.data() }, uuid);

		// Set the loader callback
		m_NextSpawnSetup = resourceHandle;
		StreamingManager::Instance()->IStreamingManagerUnknown05(m_NextSpawnSetup, 1, this, nullptr);
		StreamingManager::Instance()->UpdateLoadState(m_NextSpawnSetup, 5);

		CurrentlyLoadedSpawnSetups.emplace(uuid, m_NextSpawnSetup);
	}
}

void EntitySpawnerWindow::Render()
{
	ImGui::SetNextWindowSize(ImVec2(500, 600), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin(GetId().c_str(), &m_WindowOpen))
	{
		ImGui::End();
		return;
	}

	// Draw list
	m_SpawnerNameFilter.Draw();

	if (ImGui::BeginListBox("##SpawnSetupSelector", ImVec2(-FLT_MIN, -200)))
	{
		for (size_t i = 0; i < ModConfiguration.CachedSpawnSetups.size(); i++)
		{
			auto& spawnSetup = ModConfiguration.CachedSpawnSetups[i];

			char fullName[2048];
			sprintf_s(fullName, "%s, %s##%p", spawnSetup.Name.c_str(), spawnSetup.CorePath.c_str(), &spawnSetup);

			if (!m_SpawnerNameFilter.PassFilter(fullName))
				continue;

			if (ImGui::Selectable(fullName, m_LastSelectedSetupIndex == i))
				m_LastSelectedSetupIndex = i;
		}

		ImGui::EndListBox();
	}

	// Draw settings
	ImGui::Separator();

	bool allowSpawn = m_LastSelectedSetupIndex != -1 && m_LoaderCallback.m_NextSpawnCount == 0;

	if (!allowSpawn)
		ImGui::BeginDisabled(true);

	static int spawnCount = 1;
	static int spawnLocationType = 0;
	static WorldPosition customSpawnPosition;

	ImGui::SetNextItemWidth(200); ImGui::InputInt("Entity count", &spawnCount);
	ImGui::Spacing();
	ImGui::RadioButton("Spawn at player position", &spawnLocationType, 0);
	ImGui::RadioButton("Spawn at crosshair position", &spawnLocationType, 1);
	ImGui::RadioButton("Spawn at custom position", &spawnLocationType, 2);
	ImGui::Spacing();

	if (spawnLocationType == 2)
	{
		ImGui::PushItemWidth(200);
		ImGui::InputDouble("X", &customSpawnPosition.X, 1.0, 20.0, "%.3f");
		ImGui::InputDouble("Y", &customSpawnPosition.Y, 1.0, 20.0, "%.3f");
		ImGui::InputDouble("Z", &customSpawnPosition.Z, 1.0, 20.0, "%.3f");
		ImGui::PopItemWidth();
		ImGui::Spacing();
	}

	auto getSpawnTransform = []()
	{
		auto player = Player::GetLocalPlayer();
		auto currentTransform = player->m_Entity->m_Orientation;

		if (spawnLocationType == 0)
		{
			// Player position
			currentTransform.Position = player->m_Entity->m_Orientation.Position;
		}
		else if (spawnLocationType == 1)
		{
			// Crosshair position
			auto camera = player->GetLastActivatedCamera();
			auto cameraMatrix = camera->m_Orientation.Orientation;

			float yaw;
			float pitch;
			cameraMatrix.Decompose(&yaw, &pitch, nullptr);

			// Project forwards
			Vec3 moveDirection(sin(yaw) * cos(pitch), cos(yaw) * cos(pitch), -sin(pitch));
			moveDirection = moveDirection * 200.0f;

			currentTransform.Position.X = camera->m_Orientation.Position.X + moveDirection.X;
			currentTransform.Position.Y = camera->m_Orientation.Position.Y + moveDirection.Y;
			currentTransform.Position.Z = camera->m_Orientation.Position.Z + moveDirection.Z;

			// Raycast
			WorldPosition rayHitPosition;
			float unknownFloat;
			uint16_t materialType;
			Entity *unknownEntity;
			Vec3 normal;

			Offsets::CallID<"NodeGraph::ExportedIntersectLine", void(*)(WorldPosition&, WorldPosition&, int, const Entity *, bool, WorldPosition *, Vec3 *, float *, Entity **, uint16_t *)>
				(camera->m_Orientation.Position, currentTransform.Position, 47, nullptr, false, &rayHitPosition, &normal, &unknownFloat, &unknownEntity, &materialType);

			currentTransform.Position = rayHitPosition;
		}
		else if (spawnLocationType == 2)
		{
			// Custom position
			currentTransform.Position = customSpawnPosition;
		}

		return currentTransform;
	};

	// Spawn button
	if (ImGui::Button("Spawn") || (m_DoSpawnOnNextFrame && allowSpawn))
	{
		m_LoaderCallback.m_NextSpawnCount = spawnCount;
		m_LoaderCallback.m_NextSpawnTransform = getSpawnTransform();
		m_LoaderCallback.DoSpawn(ModConfiguration.CachedSpawnSetups[m_LastSelectedSetupIndex].CorePath, ModConfiguration.CachedSpawnSetups[m_LastSelectedSetupIndex].UUID);
	}

	ImGui::Spacing();
	ImGui::TextDisabled("Note that many humanoid and scripted entities will crash the game upon taking damage.");

	if (!allowSpawn)
		ImGui::EndDisabled();

	ImGui::End();

	m_DoSpawnOnNextFrame = false;
}

bool EntitySpawnerWindow::Close()
{
	return !m_WindowOpen || !Application::IsInGame();
}

std::string DebugUI::EntitySpawnerWindow::GetId() const
{
	return "Entity Spawner";
}

void EntitySpawnerWindow::ForceSpawnEntityClick()
{
	m_DoSpawnOnNextFrame = true;
}

}