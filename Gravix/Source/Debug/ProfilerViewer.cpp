#include "pch.h"
#include "ProfilerViewer.h"

#ifdef ENGINE_DEBUG

#include <imgui.h>
#include <algorithm>
#include <numeric>

namespace Gravix
{
	ProfilerViewer::ProfilerViewer()
	{
		std::fill(std::begin(m_FrameTimeHistory), std::end(m_FrameTimeHistory), 0.0f);
	}

	ProfilerViewer::~ProfilerViewer()
	{
	}

	void ProfilerViewer::OnImGuiRender(float deltaTime)
	{
		if (!m_Visible)
			return;

		// Accumulate time
		m_TimeSinceLastUpdate += deltaTime;

		// Calculate actual FPS from deltaTime (matches RenderDoc)
		m_FrameTime = deltaTime * 1000.0f; // Convert seconds to milliseconds
		m_FPS = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;

		// Get profile results from instrumentor (always collect)
		auto results = Instrumentor::Get().GetFrameResults();

		// Calculate profiled frame statistics
		float totalProfiledTime = 0.0f;
		m_FunctionStats.clear();

		for (const auto& result : results)
		{
			float duration = result.GetDuration();
			totalProfiledTime += duration;

			// Update function statistics
			auto& stats = m_FunctionStats[result.Name];
			stats.TotalTime += duration;
			stats.MinTime = std::min(stats.MinTime, duration);
			stats.MaxTime = std::max(stats.MaxTime, duration);
			stats.CallCount++;
		}

		// Update frame time history
		m_FrameTimeHistory[m_HistoryOffset] = m_FrameTime;
		m_HistoryOffset = (m_HistoryOffset + 1) % HISTORY_SIZE;

		// Clear frame results for next frame
		Instrumentor::Get().ClearFrameResults();

		// Update display stats at specified interval
		if (m_TimeSinceLastUpdate >= m_UpdateInterval)
		{
			m_DisplayStats = m_FunctionStats;
			m_DisplayFrameTime = m_FrameTime;
			m_DisplayFPS = m_FPS;
			m_TimeSinceLastUpdate = 0.0f;
		}

		ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Performance Profiler", &m_Visible))
		{

			// Render UI sections
			RenderFrameStats();
			ImGui::Separator();

			if (m_ShowGraph)
			{
				RenderFrameGraph();
				ImGui::Separator();
			}

			if (m_ShowFunctions)
			{
				RenderFunctionTimings();
			}
		}
		ImGui::End();
	}

	void ProfilerViewer::RenderFrameStats()
	{
		ImGui::Text("Frame Time: %.3f ms", m_DisplayFrameTime);
		ImGui::SameLine();
		ImGui::Text("FPS: %.1f", m_DisplayFPS);

		// Calculate average frame time
		float avgFrameTime = std::accumulate(std::begin(m_FrameTimeHistory), std::end(m_FrameTimeHistory), 0.0f) / HISTORY_SIZE;
		ImGui::Text("Avg Frame Time: %.3f ms", avgFrameTime);

		// Display settings
		ImGui::Checkbox("Show Graph", &m_ShowGraph);
		ImGui::SameLine();
		ImGui::Checkbox("Show Functions", &m_ShowFunctions);

		// Capture control
		bool captureEnabled = Instrumentor::Get().IsCaptureEnabled();
		if (ImGui::Checkbox("Capture Enabled", &captureEnabled))
		{
			Instrumentor::Get().SetCaptureEnabled(captureEnabled);
		}
	}

	void ProfilerViewer::RenderFrameGraph()
	{
		// Find max value for scaling
		float maxTime = *std::max_element(std::begin(m_FrameTimeHistory), std::end(m_FrameTimeHistory));
		if (maxTime < 16.67f) // At least 60 FPS scale
			maxTime = 16.67f;

		ImGui::Text("Frame Time Graph");
		ImGui::SliderFloat("Graph Height", &m_GraphHeight, 50.0f, 200.0f);

		// Draw frame time graph
		ImGui::PlotLines("##FrameTime", m_FrameTimeHistory, HISTORY_SIZE, m_HistoryOffset,
			nullptr, 0.0f, maxTime, ImVec2(0, m_GraphHeight));

		// Draw reference lines
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "60 FPS (16.67ms)");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "30 FPS (33.33ms)");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "15 FPS (66.67ms)");
	}

	void ProfilerViewer::RenderFunctionTimings()
	{
		ImGui::Text("Function Timings (Updates every %.1fs)", m_UpdateInterval);
		ImGui::SameLine();
		ImGui::Text("Next update in: %.1fs", m_UpdateInterval - m_TimeSinceLastUpdate);

		// Filter and sort controls
		ImGui::InputText("Filter", m_FilterBuffer, sizeof(m_FilterBuffer));
		ImGui::SameLine();

		const char* sortModes[] = { "Total Time", "Average Time", "Max Time", "Call Count" };
		ImGui::Combo("Sort By", &m_SortMode, sortModes, IM_ARRAYSIZE(sortModes));

		// Update interval control
		ImGui::SliderFloat("Update Interval (s)", &m_UpdateInterval, 0.1f, 5.0f);

		// Create sorted vector of function stats
		std::vector<std::pair<std::string, FunctionStats>> sortedStats;
		sortedStats.reserve(m_DisplayStats.size());

		// Apply filter
		std::string filter(m_FilterBuffer);
		std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

		for (const auto& [name, stats] : m_DisplayStats)
		{
			if (filter.empty())
			{
				sortedStats.emplace_back(name, stats);
			}
			else
			{
				std::string lowerName = name;
				std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
				if (lowerName.find(filter) != std::string::npos)
				{
					sortedStats.emplace_back(name, stats);
				}
			}
		}

		// Sort based on selected mode
		switch (m_SortMode)
		{
		case 0: // Total Time
			std::sort(sortedStats.begin(), sortedStats.end(),
				[](const auto& a, const auto& b) { return a.second.TotalTime > b.second.TotalTime; });
			break;
		case 1: // Average Time
			std::sort(sortedStats.begin(), sortedStats.end(),
				[](const auto& a, const auto& b) { return a.second.GetAverage() > b.second.GetAverage(); });
			break;
		case 2: // Max Time
			std::sort(sortedStats.begin(), sortedStats.end(),
				[](const auto& a, const auto& b) { return a.second.MaxTime > b.second.MaxTime; });
			break;
		case 3: // Call Count
			std::sort(sortedStats.begin(), sortedStats.end(),
				[](const auto& a, const auto& b) { return a.second.CallCount > b.second.CallCount; });
			break;
		}

		// Display table
		if (ImGui::BeginTable("Functions", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Function", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Total (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Min (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
			ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableHeadersRow();

			for (const auto& [name, stats] : sortedStats)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(name.c_str());

				ImGui::TableNextColumn();
				ImGui::Text("%.3f", stats.TotalTime);

				ImGui::TableNextColumn();
				ImGui::Text("%.3f", stats.GetAverage());

				ImGui::TableNextColumn();
				ImGui::Text("%.3f", stats.MinTime);

				ImGui::TableNextColumn();
				ImGui::Text("%.3f", stats.MaxTime);

				ImGui::TableNextColumn();
				ImGui::Text("%u", stats.CallCount);
			}

			ImGui::EndTable();
		}
	}
}

#endif // ENGINE_DEBUG
