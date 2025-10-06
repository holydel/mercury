module;
#include <imgui.h>
#include <string>
#include <vector>
#include <memory>
#include "IconsFontAwesome7.h"

export module AssetsBrowserWindow;

import Project;
import Asset;
import EditorState;
import PropertyWindow;

#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

struct AssetsSelectionWithDeletion : ImGuiSelectionBasicStorage
{
	// Find which item should be Focused after deletion.
	// Call _before_ item submission. Retunr an index in the before-deletion item list, your item loop should call SetKeyboardFocusHere() on it.
	// The subsequent ApplyDeletionPostLoop() code will use it to apply Selection.
	// - We cannot provide this logic in core Dear ImGui because we don't have access to selection data.
	// - We don't actually manipulate the ImVector<> here, only in ApplyDeletionPostLoop(), but using similar API for consistency and flexibility.
	// - Important: Deletion only works if the underlying ImGuiID for your items are stable: aka not depend on their index, but on e.g. item id/ptr.
	// FIXME-MULTISELECT: Doesn't take account of the possibility focus target will be moved during deletion. Need refocus or scroll offset.
	int ApplyDeletionPreLoop(ImGuiMultiSelectIO* ms_io, int items_count)
	{
		if (Size == 0)
			return -1;

		// If focused item is not selected...
		const int focused_idx = (int)ms_io->NavIdItem;  // Index of currently focused item
		if (ms_io->NavIdSelected == false)  // This is merely a shortcut, == Contains(adapter->IndexToStorage(items, focused_idx))
		{
			ms_io->RangeSrcReset = true;    // Request to recover RangeSrc from NavId next frame. Would be ok to reset even when NavIdSelected==true, but it would take an extra frame to recover RangeSrc when deleting a selected item.
			return focused_idx;             // Request to focus same item after deletion.
		}

		// If focused item is selected: land on first unselected item after focused item.
		for (int idx = focused_idx + 1; idx < items_count; idx++)
			if (!Contains(GetStorageIdFromIndex(idx)))
				return idx;

		// If focused item is selected: otherwise return last unselected item before focused item.
		for (int idx = IM_MIN(focused_idx, items_count) - 1; idx >= 0; idx--)
			if (!Contains(GetStorageIdFromIndex(idx)))
				return idx;

		return -1;
	}

	// Rewrite item list (delete items) + update selection.
	// - Call after EndMultiSelect()
	// - We cannot provide this logic in core Dear ImGui because we don't have access to your items, nor to selection data.
	template<typename ITEM_TYPE>
	void ApplyDeletionPostLoop(ImGuiMultiSelectIO* ms_io, ImVector<ITEM_TYPE>& items, int item_curr_idx_to_select)
	{
		// Rewrite item list (delete items) + convert old selection index (before deletion) to new selection index (after selection).
		// If NavId was not part of selection, we will stay on same item.
		ImVector<ITEM_TYPE> new_items;
		new_items.reserve(items.Size - Size);
		int item_next_idx_to_select = -1;
		for (int idx = 0; idx < items.Size; idx++)
		{
			if (!Contains(GetStorageIdFromIndex(idx)))
				new_items.push_back(items[idx]);
			if (item_curr_idx_to_select == idx)
				item_next_idx_to_select = new_items.Size - 1;
		}
		items.swap(new_items);

		// Update selection
		Clear();
		if (item_next_idx_to_select != -1 && ms_io->NavIdSelected)
			SetItemSelected(GetStorageIdFromIndex(item_next_idx_to_select), true);
	}
};

export class AssetsBrowserWindow
{
public:
    void ProcessImgui();
	static AssetsBrowserWindow& Get() {
		static AssetsBrowserWindow instance;
		return instance;
	}
private:
    float leftPanelWidth = 256.0f;
    void RenderFolderSubfolders();
    void RenderFolder(FolderAsset* folder);

    void RenderFolderAssets();
    FolderAsset* selectedFolder = nullptr;

	// Options
	bool            ShowTypeOverlay = true;
	bool            AllowSorting = true;
	bool            AllowDragUnselected = false;
	bool            AllowBoxSelect = true;
	float           IconSize = 128.0f;
	int             IconSpacing = 8;
	int             IconHitSpacing = 4;         // Increase hit-spacing if you want to make it possible to clear or box-select from gaps. Some spacing is required to able to amend with Shift+box-select. Value is small in Explorer.
	bool            StretchSpacing = true;

	// State
	//ImVector<ExampleAsset> Items;               // Our items
	AssetsSelectionWithDeletion Selection;     // Our selection (ImGuiSelectionBasicStorage + helper funcs to handle deletion)
	ImGuiID         NextItemId = 0;             // Unique identifier when creating new items
	bool            RequestDelete = false;      // Deferred deletion request
	bool            RequestSort = false;        // Deferred sort request
	float           ZoomWheelAccum = 0.0f;      // Mouse wheel accumulator to handle smooth wheels better

	// Calculated sizes for layout, output of UpdateLayoutSizes(). Could be locals but our code is simpler this way.
	ImVec2          LayoutItemSize;
	ImVec2          LayoutItemStep;             // == LayoutItemSize + LayoutItemSpacing
	float           LayoutItemSpacing = 4.0f;
	float           LayoutSelectableSpacing = 0.0f;
	float           LayoutOuterPadding = 0.0f;
	int             LayoutColumnCount = 0;
	int             LayoutLineCount = 0;

	void UpdateLayoutSizes(FolderAsset* folder, float avail_width)
	{
		// Layout: when not stretching: allow extending into right-most spacing.
		LayoutItemSpacing = (float)IconSpacing;
		if (StretchSpacing == false)
			avail_width += floorf(LayoutItemSpacing * 0.5f);

		// Layout: calculate number of icon per line and number of lines
		LayoutItemSize = ImVec2(floorf(IconSize), floorf(IconSize));
		LayoutColumnCount = IM_MAX((int)(avail_width / (LayoutItemSize.x + LayoutItemSpacing)), 1);
		LayoutLineCount = (folder->assets.size() + LayoutColumnCount - 1) / LayoutColumnCount;

		// Layout: when stretching: allocate remaining space to more spacing. Round before division, so item_spacing may be non-integer.
		if (StretchSpacing && LayoutColumnCount > 1)
			LayoutItemSpacing = floorf(avail_width - LayoutItemSize.x * LayoutColumnCount) / LayoutColumnCount;

		LayoutItemStep = ImVec2(LayoutItemSize.x + LayoutItemSpacing, LayoutItemSize.y + LayoutItemSpacing + 48);
		LayoutSelectableSpacing = IM_MAX(floorf(LayoutItemSpacing) - IconHitSpacing, 0.0f);
		LayoutOuterPadding = floorf(LayoutItemSpacing * 0.5f);
	}
};

void AssetsBrowserWindow::RenderFolder(FolderAsset* folder)
{
    if (!folder) return;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanFullWidth;
    if (selectedFolder == folder) flags |= ImGuiTreeNodeFlags_Selected;
    if (folder->subfolders.empty()) flags |= ImGuiTreeNodeFlags_Leaf;

    std::string iconString = folder->subfolders.empty() ? ICON_FA_FOLDER : ICON_FA_FOLDER_OPEN;
    if (folder->type == AssetType::ShadersSet) iconString = ICON_FA_SHAPES;

    bool isOpen = ImGui::TreeNodeEx(folder->path.filename().string().c_str(), flags, "%s %s",
        iconString.c_str(), folder->path.filename().string().c_str());

    if (ImGui::IsItemClicked()) {
        selectedFolder = folder;
		PropertyWindow::Get().SetObjectToEdit(folder);

		// Clear selection when changing folder
		Selection.Clear();
    }

    if (isOpen)
    {
        for (auto subfolder : folder->subfolders)
            RenderFolder(subfolder);
        ImGui::TreePop();
    }
}

void AssetsBrowserWindow::RenderFolderSubfolders()
{
    FolderAsset* rootAssetsFolder = EditorState::gCurrentProject.GetRootAssetsFolder();
    FolderAsset* rootShadersFolder = EditorState::gCurrentProject.GetRootShadersFolder();

    RenderFolder(rootAssetsFolder);
    RenderFolder(rootShadersFolder);
}

void AssetsBrowserWindow::RenderFolderAssets()
{
    if (!selectedFolder) {
        ImGui::Text("No folder selected...");
        return;
    }

	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	const float avail_width = ImGui::GetContentRegionAvail().x;
	UpdateLayoutSizes(selectedFolder, avail_width);

	// Calculate and store start position.
	ImVec2 start_pos = ImGui::GetCursorScreenPos();
	start_pos = ImVec2(start_pos.x + LayoutOuterPadding, start_pos.y + LayoutOuterPadding);
	ImGui::SetCursorScreenPos(start_pos);

	// Multi-select
	ImGuiMultiSelectFlags ms_flags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_ClearOnClickVoid;

	// - Enable box-select (in 2D mode, so that changing box-select rectangle X1/X2 boundaries will affect clipped items)
	if (AllowBoxSelect)
		ms_flags |= ImGuiMultiSelectFlags_BoxSelect2d;

	// - This feature allows dragging an unselected item without selecting it (rarely used)
	if (AllowDragUnselected)
		ms_flags |= ImGuiMultiSelectFlags_SelectOnClickRelease;

	// - Enable keyboard wrapping on X axis
	// (FIXME-MULTISELECT: We haven't designed/exposed a general nav wrapping api yet, so this flag is provided as a courtesy to avoid doing:
	//    ImGui::NavMoveRequestTryWrapping(ImGui::GetCurrentWindow(), ImGuiNavMoveFlags_WrapX);
	// When we finish implementing a more general API for this, we will obsolete this flag in favor of the new system)
	ms_flags |= ImGuiMultiSelectFlags_NavWrapX;

	ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(ms_flags, Selection.Size, selectedFolder->assets.size());

	// Use custom selection adapter: store ID in selection (recommended)
	// Selection.UserData = selectedFolder;
	// Selection.AdapterIndexToStorageId = [this](ImGuiSelectionBasicStorage* self_, int idx) { return (ImGuiID)(selectedFolder->assets[idx]->GetID()); };

    Selection.UserData = selectedFolder;
    Selection.AdapterIndexToStorageId = [](ImGuiSelectionBasicStorage* self_, int idx) -> ImGuiID
    {
        auto folder = static_cast<FolderAsset*>(self_->UserData);
        if (!folder) return 0;
        if (idx < 0 || idx >= (int)folder->assets.size()) return 0;
        return (ImGuiID)folder->assets[idx]->GetID();
    };

	Selection.ApplyRequests(ms_io);

	const bool want_delete = (ImGui::Shortcut(ImGuiKey_Delete, ImGuiInputFlags_Repeat) && (Selection.Size > 0)) || RequestDelete;
	const int item_curr_idx_to_focus = want_delete ? Selection.ApplyDeletionPreLoop(ms_io, selectedFolder->assets.size()) : -1;
	RequestDelete = false;

	// Push LayoutSelectableSpacing (which is LayoutItemSpacing minus hit-spacing, if we decide to have hit gaps between items)
	// Altering style ItemSpacing may seem unnecessary as we position every items using SetCursorScreenPos()...
	// But it is necessary for two reasons:
	// - Selectables uses it by default to visually fill the space between two items.
	// - The vertical spacing would be measured by Clipper to calculate line height if we didn't provide it explicitly (here we do).
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(LayoutSelectableSpacing, LayoutSelectableSpacing));

	// Rendering parameters
	const ImU32 icon_type_overlay_colors[3] = { 0, IM_COL32(200, 70, 70, 255), IM_COL32(70, 170, 70, 255) };
	const ImU32 icon_bg_color = ImGui::GetColorU32(IM_COL32(35, 35, 35, 220));
	const ImVec2 icon_type_overlay_size = ImVec2(4.0f, 4.0f);
	const bool display_label = (LayoutItemSize.x >= ImGui::CalcTextSize("999").x);

	const int column_count = LayoutColumnCount;
	ImGuiListClipper clipper;
	clipper.Begin(LayoutLineCount, LayoutItemStep.y);
	if (item_curr_idx_to_focus != -1)
		clipper.IncludeItemByIndex(item_curr_idx_to_focus / column_count); // Ensure focused item line is not clipped.
	if (ms_io->RangeSrcItem != -1)
		clipper.IncludeItemByIndex((int)ms_io->RangeSrcItem / column_count); // Ensure RangeSrc item line is not clipped.
	while (clipper.Step())
	{
		for (int line_idx = clipper.DisplayStart; line_idx < clipper.DisplayEnd; line_idx++)
		{
			const int item_min_idx_for_current_line = line_idx * column_count;
			const int item_max_idx_for_current_line = IM_MIN((line_idx + 1) * column_count, selectedFolder->assets.size());
			for (int item_idx = item_min_idx_for_current_line; item_idx < item_max_idx_for_current_line; ++item_idx)
			{
				if(item_idx < 0 || item_idx >= (int)selectedFolder->assets.size())
					continue;

				Asset* item_data = selectedFolder->assets[item_idx];
				ImGui::PushID((int)item_data->GetID());

				// Position item
				ImVec2 pos = ImVec2(start_pos.x + (item_idx % column_count) * LayoutItemStep.x, start_pos.y + line_idx * LayoutItemStep.y);
				ImGui::SetCursorScreenPos(pos);

				ImGui::SetNextItemSelectionUserData(item_idx);
				bool item_is_selected = Selection.Contains((ImGuiID)item_data->GetID());
				bool item_is_visible = ImGui::IsRectVisible(LayoutItemSize);
				ImGui::Selectable("", item_is_selected, ImGuiSelectableFlags_None, LayoutItemSize);

				if (item_is_selected)
				{
				//	if (!item_data->GetData())
				//		item_data->Import();

					//PropertyEditorScreen::Instance()->SetProps(item_data);

				//	if (item_data->GetData())
				//	{
					//	PreviewRenderer::Instance().SetPreviewProps(item_data->GetData());
					//}

					//if (item_data->GetType() == AssetType::Folder && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					//{
					//	gSelectedFolder = static_cast<AssetFolder*>(item_data);
					//
					//}
					if(item_data->type == AssetType::Folder && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						selectedFolder = static_cast<FolderAsset*>(item_data);
					}

					PropertyWindow::Get().SetObjectToEdit(item_data);
				}
				// Update our selection state immediately (without waiting for EndMultiSelect() requests)
				// because we use this to alter the color of our text/icon.
				if (ImGui::IsItemToggledSelection())
					item_is_selected = !item_is_selected;

				// Focus (for after deletion)
				if (item_curr_idx_to_focus == item_idx)
					ImGui::SetKeyboardFocusHere(-1);

				// Drag and drop
				if (ImGui::BeginDragDropSource())
				{
					// Create payload with full selection OR single unselected item.
					// (the later is only possible when using ImGuiMultiSelectFlags_SelectOnClickRelease)
					if (ImGui::GetDragDropPayload() == NULL)
					{
						ImVector<ImGuiID> payload_items;
						void* it = NULL;
						ImGuiID id = 0;
						if (!item_is_selected)
							payload_items.push_back(item_data->GetID());
						else
							while (Selection.GetNextSelectedItem(&it, &id))
								payload_items.push_back(id);
						ImGui::SetDragDropPayload("ASSETS_BROWSER_ITEMS", payload_items.Data, (size_t)payload_items.size_in_bytes());
					}

					// Display payload content in tooltip, by extracting it from the payload data
					// (we could read from selection, but it is more correct and reusable to read from payload)
					const ImGuiPayload* payload = ImGui::GetDragDropPayload();
					const int payload_count = (int)payload->DataSize / (int)sizeof(ImGuiID);
					ImGui::Text("%d assets", payload_count);

					ImGui::EndDragDropSource();
				}

				// Render icon (a real app would likely display an image/thumbnail here)
				// Because we use ImGuiMultiSelectFlags_BoxSelect2d, clipping vertical may occasionally be larger, so we coarse-clip our rendering as well.
				if (item_is_visible)
				{
					int bottomSpace = 4;
					if (display_label)
					{
						//bottomSpace = (item_data->GetAssetNameRows() + 1) * ImGui::GetFontSize();
						bottomSpace = (1 + 1) * ImGui::GetFontSize();
					}

					ImVec2 box_min(pos.x - 1, pos.y - 1);
					ImVec2 box_max(box_min.x + LayoutItemSize.x + 2, box_min.y + LayoutItemSize.y + 4); // Dubious
					draw_list->AddRectFilled(box_min, box_max, icon_bg_color); // Background color


					//ImTextureID previewID = item_data->GetAssetPreviewStatic();
					//if (previewID)
					//{
					//	draw_list->AddImage(previewID, box_min, box_max);
					//}


					//if (ShowTypeOverlay && item_data->Type != 0)
					//{
					//    ImU32 type_col = icon_type_overlay_colors[item_data->Type % IM_ARRAYSIZE(icon_type_overlay_colors)];
					//    draw_list->AddRectFilled(ImVec2(box_max.x - 2 - icon_type_overlay_size.x, box_min.y + 2), ImVec2(box_max.x - 2, box_min.y + 2 + icon_type_overlay_size.y), type_col);
					//}
					if (display_label)
					{
						ImU32 label_col = ImGui::GetColorU32(item_is_selected ? ImGuiCol_Text : ImGuiCol_TextDisabled);

						//draw_list->AddText(ImVec2(box_min.x, box_max.y + 4), label_col, item_data->AssetNameCStr());
						draw_list->AddText(ImVec2(box_min.x, box_max.y + 4), label_col, item_data->GetAssetNameCStr());
					}
				}

				ImGui::PopID();
			}
		}
	}
	clipper.End();
	ImGui::PopStyleVar(); // ImGuiStyleVar_ItemSpacing

	// Context menu
	if (ImGui::BeginPopupContextWindow())
	{
		ImGui::Text("Selection: %d items", Selection.Size);
		ImGui::Separator();
		if (ImGui::MenuItem("Delete", "Del", false, Selection.Size > 0))
			RequestDelete = true;
		ImGui::EndPopup();
	}

	ms_io = ImGui::EndMultiSelect();
	Selection.ApplyRequests(ms_io);
	//if (want_delete)
	//    Selection.ApplyDeletionPostLoop(ms_io, Items, item_curr_idx_to_focus);

	// Zooming with CTRL+Wheel
	if (ImGui::IsWindowAppearing())
		ZoomWheelAccum = 0.0f;
	if (ImGui::IsWindowHovered() && io.MouseWheel != 0.0f && ImGui::IsKeyDown(ImGuiMod_Ctrl) && ImGui::IsAnyItemActive() == false)
	{
		ZoomWheelAccum += io.MouseWheel;
		if (fabsf(ZoomWheelAccum) >= 1.0f)
		{
			// Calculate hovered item index from mouse location
			// FIXME: Locking aiming on 'hovered_item_idx' (with a cool-down timer) would ensure zoom keeps on it.
			const float hovered_item_nx = (io.MousePos.x - start_pos.x + LayoutItemSpacing * 0.5f) / LayoutItemStep.x;
			const float hovered_item_ny = (io.MousePos.y - start_pos.y + LayoutItemSpacing * 0.5f) / LayoutItemStep.y;
			const int hovered_item_idx = ((int)hovered_item_ny * LayoutColumnCount) + (int)hovered_item_nx;
			ImGui::SetTooltip("%f,%f -> item %d", hovered_item_nx, hovered_item_ny, hovered_item_idx); // Move those 4 lines in block above for easy debugging

			// Zoom
			IconSize *= powf(1.1f, (float)(int)ZoomWheelAccum);
			IconSize = IM_CLAMP(IconSize, 16.0f, 128.0f);
			ZoomWheelAccum -= (int)ZoomWheelAccum;
			UpdateLayoutSizes(selectedFolder, avail_width);

			// Manipulate scroll to that we will land at the same Y location of currently hovered item.
			// - Calculate next frame position of item under mouse
			// - Set new scroll position to be used in next ImGui::BeginChild() call.
			float hovered_item_rel_pos_y = ((float)(hovered_item_idx / LayoutColumnCount) + fmodf(hovered_item_ny, 1.0f)) * LayoutItemStep.y;
			hovered_item_rel_pos_y += ImGui::GetStyle().WindowPadding.y;
			float mouse_local_y = io.MousePos.y - ImGui::GetWindowPos().y;
			ImGui::SetScrollY(hovered_item_rel_pos_y - mouse_local_y);
		}
	}
}

void AssetsBrowserWindow::ProcessImgui()
{
    ImGui::Begin("Assets Browser");

    // LEFT PANEL (folders)
    ImGui::BeginChild("left_panel", ImVec2(leftPanelWidth, 0), true);
    RenderFolderSubfolders();

    // Click on empty space in left panel -> clear both selections
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_None) &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
        !ImGui::IsAnyItemHovered())
    {
        selectedFolder = nullptr;
    }
    ImGui::EndChild();

    // Splitter
    ImGui::SameLine();
    ImGui::InvisibleButton("vsplitter", ImVec2(8.0f, -1));
    if (ImGui::IsItemActive())
        leftPanelWidth += ImGui::GetIO().MouseDelta.x;
    ImGui::SameLine();

    // RIGHT PANEL (assets)
    ImGui::BeginChild("right_panel", ImVec2(0, 0), true);
    RenderFolderAssets();
    ImGui::EndChild();

    ImGui::End();
}
