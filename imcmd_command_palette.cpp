#include "imcmd_command_palette.h"

#include "imcmd_fuzzy_search.h"

#include <imgui.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

namespace ImGuiCommandPalette
{
void CommandRegistry::AddCommand(Command command)
{
    auto location = std::lower_bound(
        m_Commands.begin(),
        m_Commands.end(),
        command,
        [](const Command& a, const Command& b) -> bool {
            return strcmp(a.Name, b.Name) < 0;
        });
    m_Commands.insert(location, std::move(command));

    // TODO invalidate search results
}

bool CommandRegistry::RemoveCommand(const char* name)
{
    struct Comparator
    {
        bool operator()(const Command& command, const char* str) const
        {
            return strcmp(command.Name, str) < 0;
        }

        bool operator()(const char* str, const Command& command) const
        {
            return strcmp(str, command.Name) < 0;
        }
    };

    auto range = std::equal_range(m_Commands.begin(), m_Commands.end(), name, Comparator{});
    m_Commands.erase(range.first, range.second);

    // TODO invalidate search results
    return range.first != range.second;
}

size_t CommandRegistry::GetCommandCount() const
{
    return m_Commands.size();
}

const Command& CommandRegistry::GetCommand(size_t idx) const
{
    return m_Commands[idx];
}

bool CommandExecutionContext::IsInitiated() const
{
    return m_Command != nullptr;
}

const Command* CommandExecutionContext::GetCurrentCommand() const
{
    return m_Command;
}

void CommandExecutionContext::Initiate(const Command& command)
{
    if (m_Command == nullptr) {
        m_Command = &command;
    }
}

void CommandExecutionContext::Prompt(std::vector<std::string> options)
{
    IM_ASSERT(m_Command != nullptr);
    m_CurrentOptions = std::move(options);
    ++m_Depth;
}

void CommandExecutionContext::Finish()
{
    IM_ASSERT(m_Command != nullptr);
    m_Command = nullptr;
    m_CurrentOptions.clear();
    m_Depth = 0;
}

int CommandExecutionContext::GetExecutionDepth() const
{
    return m_Depth;
}

struct CommandPalette::SearchResult
{
    int ItemIndex;
    int Score;
    int MatchCount;
    uint8_t Matches[32];
};

struct CommandPalette::Item
{
    bool hovered = false;
    bool held = false;

    // Helpers

    enum ItemType
    {
        CommandItem,
        CommandOptionItem,
    };

    enum IndexType
    {
        DirectIndex,
        SearchResultIndex,
    };

    struct ItemInfo
    {
        const char* Text;
        const Command* AssociatedCommand;
        int ItemId;
        ItemType ItemType;
        IndexType IndexType;
    };

    static size_t GetItemCount(const CommandPalette& self)
    {
        int depth = self.m_ExecutionCtx.GetExecutionDepth();
        if (depth == 0) {
            if (self.m_SearchText[0] == '\0') {
                return self.m_Registry->GetCommandCount();
            } else {
                return self.m_SearchResults.size();
            }
        } else {
            if (self.m_SearchText[0] == '\0') {
                return self.m_ExecutionCtx.m_CurrentOptions.size();
            } else {
                return self.m_SearchResults.size();
            }
        }
    }

    static ItemInfo GetItem(const CommandPalette& self, size_t idx)
    {
        ItemInfo option;

        int depth = self.m_ExecutionCtx.GetExecutionDepth();
        if (depth == 0) {
            if (self.m_SearchText[0] == '\0') {
                auto& command = self.m_Registry->GetCommand(idx);
                option.Text = command.Name;
                option.AssociatedCommand = &command;
                option.ItemId = idx;
                option.IndexType = DirectIndex;
            } else {
                auto id = self.m_SearchResults[idx].ItemIndex;
                auto& command = self.m_Registry->GetCommand(id);
                option.Text = command.Name;
                option.AssociatedCommand = &command;
                option.ItemId = id;
                option.IndexType = SearchResultIndex;
            }
            option.ItemType = CommandItem;
        } else {
            IM_ASSERT(self.m_ExecutionCtx.GetCurrentCommand() != nullptr);
            if (self.m_SearchText[0] == '\0') {
                option.Text = self.m_ExecutionCtx.m_CurrentOptions[idx].c_str();
                option.AssociatedCommand = self.m_ExecutionCtx.GetCurrentCommand();
                option.ItemId = idx;
                option.IndexType = DirectIndex;
            } else {
                auto id = self.m_SearchResults[idx].ItemIndex;
                option.Text = self.m_ExecutionCtx.m_CurrentOptions[id].c_str();
                option.AssociatedCommand = self.m_ExecutionCtx.GetCurrentCommand();
                option.ItemId = id;
                option.IndexType = SearchResultIndex;
            }
            option.ItemType = CommandOptionItem;
        }

        return option;
    }
};

CommandPalette::CommandPalette(CommandRegistry& registry)
    : m_Registry{ &registry }
{
    memset(m_SearchText, 0, IM_ARRAYSIZE(m_SearchText));
}

CommandPalette::~CommandPalette() = default;

void CommandPalette::SelectFocusedItem()
{
    if (m_FocusedItemId < 0 || m_FocusedItemId >= Item::GetItemCount(*this)) {
        return;
    }

    auto selected_item = Item::GetItem(*this, m_FocusedItemId);
    auto& command = *selected_item.AssociatedCommand;

    auto InvalidateSearchResults = [&]() -> void {
        memset(m_SearchText, 0, IM_ARRAYSIZE(m_SearchText));
        m_SearchResults.clear();
        m_FocusedItemId = 0;
    };

    int depth = m_ExecutionCtx.GetExecutionDepth();
    if (depth == 0) {
        IM_ASSERT(!m_ExecutionCtx.IsInitiated());

        m_ExecutionCtx.Initiate(*selected_item.AssociatedCommand);
        if (command.InitialCallback) {
            command.InitialCallback(m_ExecutionCtx);
            // TODO if command adds new commands in the callback, this invalidates `command`, breaking the code later

            m_FocusSearchBox = true;
            // Don't invalidate search results if no further actions have been requested (returning to global list of commands)
            if (m_ExecutionCtx.IsInitiated()) {
                InvalidateSearchResults();
            }
        } else {
            m_ExecutionCtx.Finish();
        }
    } else {
        IM_ASSERT(m_ExecutionCtx.IsInitiated());
        IM_ASSERT(command.SubsequentCallback);
        command.SubsequentCallback(m_ExecutionCtx, selected_item.ItemId);
        // TODO see above note about adding new commands

        m_FocusSearchBox = true;
        InvalidateSearchResults();
    }

    // This action terminated execution, close command palette window
    if (!m_ExecutionCtx.IsInitiated()) {
        if (command.TerminatingCallback) {
            command.TerminatingCallback();
        }
        m_WindowVisible = false;
    }
}

bool CommandPalette::IsVisible() const
{
    return m_WindowVisible;
}

void CommandPalette::SetVisible(bool visible)
{
    // Rising edge
    if (m_WindowVisible == false && visible == true) { // NOLINT Simplify
        m_FocusSearchBox = true;
    }

    m_WindowVisible = visible;
}

void CommandPalette::Show(const char* name, float search_result_window_height)
{
    if (m_WindowVisible) {
        auto viewport = ImGui::GetMainViewport()->Size;

        // Center window horizontally, align top vertically
        ImGui::SetNextWindowPos(ImVec2(viewport.x / 2, 0), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(viewport.x * 0.3f, viewport.y * 0.0f), ImGuiCond_Always);

        ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
        float width = ImGui::GetWindowContentRegionWidth();

        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
            // Close popup when user unfocused the command palette window (clicking elsewhere)
            m_WindowVisible = false;
        }

        if (m_FocusSearchBox) {
            m_FocusSearchBox = false;
            // Focus the search box when user first brings command palette window up
            // Note: this only affects the next frame
            ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::SetNextItemWidth(width);
        if (ImGui::InputText("##SearchBox", m_SearchText, IM_ARRAYSIZE(m_SearchText))) {
            // Search string updated, update search results

            m_FocusedItemId = 0;
            m_SearchResults.clear();

            size_t item_count;
            if (m_ExecutionCtx.GetExecutionDepth() == 0) {
                item_count = m_Registry->GetCommandCount();
            } else {
                item_count = m_ExecutionCtx.m_CurrentOptions.size();
            }

            for (size_t i = 0; i < item_count; ++i) {
                const char* text;
                if (m_ExecutionCtx.GetExecutionDepth() == 0) {
                    text = m_Registry->GetCommand(i).Name;
                } else {
                    text = m_ExecutionCtx.m_CurrentOptions[i].c_str();
                }

                SearchResult result{
                    .ItemIndex = (int)i,
                };
                if (FuzzySearch(m_SearchText, text, result.Score, result.Matches, IM_ARRAYSIZE(result.Matches), result.MatchCount)) {
                    m_SearchResults.push_back(result);
                }
            }

            std::sort(
                m_SearchResults.begin(),
                m_SearchResults.end(),
                [](const SearchResult& a, const SearchResult& b) -> bool {
                    // We want the biggest element first
                    return a.Score > b.Score;
                });
        }

        ImGui::BeginChild("SearchResults", ImVec2(width, search_result_window_height));

        auto window = ImGui::GetCurrentWindow();
        auto& io = ImGui::GetIO();

        auto text_color = ImGui::GetColorU32(ImGuiCol_Text);
        auto item_hovered_color = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
        auto item_active_color = ImGui::GetColorU32(ImGuiCol_HeaderActive);
        auto item_selected_color = ImGui::GetColorU32(ImGuiCol_Header);

        int item_count = Item::GetItemCount(*this);
        if (m_Items.size() < item_count) {
            m_Items.resize(item_count);
        }

        if (!RegularFont) {
            RegularFont = ImGui::GetDrawListSharedData()->Font;
        }
        if (!HighlightFont) {
            HighlightFont = ImGui::GetDrawListSharedData()->Font;
        }

        // Flag used to delay item selection until after the loop ends
        bool select_focused_item = false;
        for (size_t i = 0; i < item_count; ++i) {
            auto id = window->GetID(static_cast<int>(i));

            ImVec2 size{
                ImGui::GetContentRegionAvailWidth(),
                ImMax(RegularFont->FontSize, HighlightFont->FontSize),
            };
            ImRect rect{
                window->DC.CursorPos,
                window->DC.CursorPos + ImGui::CalcItemSize(size, 0.0f, 0.0f),
            };

            bool& hovered = m_Items[i].hovered;
            bool& held = m_Items[i].held;
            if (held && hovered) {
                window->DrawList->AddRectFilled(rect.Min, rect.Max, item_active_color);
            } else if (hovered) {
                window->DrawList->AddRectFilled(rect.Min, rect.Max, item_hovered_color);
            } else if (m_FocusedItemId == i) {
                window->DrawList->AddRectFilled(rect.Min, rect.Max, item_selected_color);
            }

            auto item = Item::GetItem(*this, i);
            if (item.IndexType == Item::SearchResultIndex) {
                // Iterating search results: draw text with highlights at matched chars

                auto& search_result = m_SearchResults[i];
                auto text_pos = window->DC.CursorPos;
                int range_begin;
                int range_end;
                int last_range_end = 0;

                auto DrawCurrentRange = [&]() -> void {
                    if (range_begin != last_range_end) {
                        // Draw normal text between last highlighted range end and current highlighted range start
                        auto begin = item.Text + last_range_end;
                        auto end = item.Text + range_begin;
                        window->DrawList->AddText(text_pos, text_color, begin, end);

                        auto segment_size = RegularFont->CalcTextSizeA(RegularFont->FontSize, std::numeric_limits<float>::max(), 0.0f, begin, end);
                        text_pos.x += segment_size.x;
                    }

                    auto begin = item.Text + range_begin;
                    auto end = item.Text + range_end;
                    window->DrawList->AddText(HighlightFont, HighlightFont->FontSize, text_pos, text_color, begin, end);

                    auto segment_size = HighlightFont->CalcTextSizeA(HighlightFont->FontSize, std::numeric_limits<float>::max(), 0.0f, begin, end);
                    text_pos.x += segment_size.x;
                };

                IM_ASSERT(search_result.MatchCount >= 1);
                range_begin = search_result.Matches[0];
                range_end = range_begin;

                int last_char_idx = -1;
                for (int j = 0; j < search_result.MatchCount; ++j) {
                    int char_idx = search_result.Matches[j];

                    if (char_idx == last_char_idx + 1) {
                        // These 2 indices are equal, extend our current range by 1
                        ++range_end;
                    } else {
                        DrawCurrentRange();
                        last_range_end = range_end;
                        range_begin = char_idx;
                        range_end = char_idx + 1;
                    }

                    last_char_idx = char_idx;
                }

                // Draw the remaining range (if any)
                if (range_begin != range_end) {
                    DrawCurrentRange();
                }

                // Draw the text after the last range (if any)
                window->DrawList->AddText(text_pos, text_color, item.Text + range_end); // Draw until \0
            } else {
                // Iterating everything else: draw text as-is, there is no highlights

                window->DrawList->AddText(window->DC.CursorPos, text_color, item.Text);
            }

            ImGui::ItemSize(rect);
            if (!ImGui::ItemAdd(rect, id)) {
                continue;
            }
            if (ImGui::ButtonBehavior(rect, id, &hovered, &held)) {
                m_FocusedItemId = i;
                select_focused_item = true;
            }
        }

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow))) {
            m_FocusedItemId = ImMax(m_FocusedItemId - 1, 0);
        } else if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow))) {
            m_FocusedItemId = ImMin(m_FocusedItemId + 1, item_count - 1);
        }
        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) || select_focused_item) {
            SelectFocusedItem();
        }

        ImGui::EndChild();

        ImGui::End();
    }
}
} // namespace ImGuiCommandPalette
