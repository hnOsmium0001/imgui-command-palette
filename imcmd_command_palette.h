#pragma once

#include <imgui.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

// TODO support std::string_view
// TODO support function pointer callback in addition to std::function

namespace ImGuiCommandPalette
{

// Forward declaration
struct Command;
class CommandRegistry;
class CommandExecutionContext;
class CommandPalette;

struct Command
{
    const char* Name;
    std::function<void(CommandExecutionContext& ctx)> InitialCallback;
    std::function<void(CommandExecutionContext& ctx, int selected_option)> SubsequentCallback;
    std::function<void()> TerminatingCallback;
};

class CommandRegistry
{
private:
    std::vector<Command> m_Commands;

public:
    void AddCommand(Command command);
    bool RemoveCommand(const char* name);

    size_t GetCommandCount() const;
    const Command& GetCommand(size_t idx) const;
};

class CommandExecutionContext
{
    friend class CommandPalette;

private:
    const Command* m_Command = nullptr;
    std::vector<std::string> m_CurrentOptions;
    int m_Depth = 0;

public:
    const Command* GetCurrentCommand() const;
    bool IsInitiated() const;
    void Initiate(const Command& command);

    void Prompt(std::vector<std::string> options);
    void Finish();

    /// Return the number of prompts that the user is currently completing. For example, when the user opens command
    /// palette fresh and selects a command, 0 is returned. If the command asks some prompt, and then the user selects
    /// again, 1 is returned.
    int GetExecutionDepth() const;
};

class CommandPalette
{
public:
    ImFont* RegularFont = nullptr;
    ImFont* HighlightFont = nullptr;

private:
    struct SearchResult;
    struct Item;

    CommandRegistry* m_Registry;
    std::vector<SearchResult> m_SearchResults;
    std::vector<Item> m_Items;
    CommandExecutionContext m_ExecutionCtx;
    int m_FocusedItemId = 0;
    char m_SearchText[std::numeric_limits<uint8_t>::max() + 1];
    bool m_FocusSearchBox = false;
    bool m_WindowVisible = false;

public:
    CommandPalette(CommandRegistry& registry);
    ~CommandPalette();

    void SelectFocusedItem();

    bool IsVisible() const;
    void SetVisible(bool visible);

    void Show(const char* name, float search_result_window_height = 400.0f);
};

} // namespace ImGuiCommandPalette
