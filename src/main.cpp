#include <windows.h>
#include <shellapi.h>

#include <string>

namespace {

constexpr wchar_t kWindowClassName[] = L"MacAltWinSwapWindow";
constexpr wchar_t kAppName[] = L"MacAltWinSwap";
constexpr wchar_t kStartupValueName[] = L"MacAltWinSwap";
constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayCallbackMessage = WM_APP + 1;
constexpr UINT kMenuToggleEnabled = 1001;
constexpr UINT kMenuToggleStartup = 1002;
constexpr UINT kMenuExit = 1003;

HWND g_window = nullptr;
HHOOK g_keyboardHook = nullptr;
NOTIFYICONDATAW g_trayIcon{};
bool g_enabled = true;

WORD ScanCodeFor(UINT virtualKey) {
    return static_cast<WORD>(MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC_EX));
}

bool IsExtendedScanCode(WORD scanCode) {
    return (scanCode & 0xFF00) != 0;
}

void SendKey(UINT virtualKey, bool keyUp) {
    const WORD scanCode = ScanCodeFor(virtualKey);

    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = static_cast<WORD>(scanCode & 0x00FF);
    input.ki.dwFlags = KEYEVENTF_SCANCODE;

    if (IsExtendedScanCode(scanCode)) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    if (keyUp) {
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    }

    SendInput(1, &input, sizeof(input));
}

UINT SwapVirtualKey(DWORD virtualKey, DWORD flags) {
    switch (virtualKey) {
    case VK_LWIN:
        return VK_LMENU;
    case VK_RWIN:
        return VK_RMENU;
    case VK_LMENU:
        return VK_LWIN;
    case VK_RMENU:
        return VK_RWIN;
    case VK_MENU:
        return (flags & LLKHF_EXTENDED) ? VK_RWIN : VK_LWIN;
    default:
        return 0;
    }
}

LRESULT CALLBACK KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && g_enabled) {
        const auto* event = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if ((event->flags & LLKHF_INJECTED) == 0) {
            const UINT swappedKey = SwapVirtualKey(event->vkCode, event->flags);

            if (swappedKey != 0) {
                const bool keyUp = (event->flags & LLKHF_UP) != 0 ||
                    wParam == WM_KEYUP || wParam == WM_SYSKEYUP;
                SendKey(swappedKey, keyUp);
                return 1;
            }
        }
    }

    return CallNextHookEx(g_keyboardHook, code, wParam, lParam);
}

std::wstring GetExecutablePath() {
    std::wstring path(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));

    while (length == path.size() && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        path.resize(path.size() * 2);
        length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    }

    path.resize(length);
    return path;
}

bool IsStartupEnabled() {
    DWORD type = 0;
    wchar_t value[MAX_PATH]{};
    DWORD valueSize = sizeof(value);

    const LSTATUS status = RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        kStartupValueName,
        RRF_RT_REG_SZ,
        &type,
        value,
        &valueSize);

    return status == ERROR_SUCCESS && type == REG_SZ;
}

bool SetStartupEnabled(bool enabled) {
    HKEY key = nullptr;
    const LSTATUS openStatus = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_SET_VALUE,
        &key);

    if (openStatus != ERROR_SUCCESS) {
        return false;
    }

    LSTATUS status = ERROR_SUCCESS;
    if (enabled) {
        const std::wstring command = L"\"" + GetExecutablePath() + L"\"";
        status = RegSetValueExW(
            key,
            kStartupValueName,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        status = RegDeleteValueW(key, kStartupValueName);
        if (status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
        }
    }

    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

void UpdateTrayIcon() {
    const std::wstring tooltip = std::wstring(kAppName) +
        (g_enabled ? L": Enabled" : L": Disabled");
    wcsncpy_s(g_trayIcon.szTip, tooltip.c_str(), _TRUNCATE);

    Shell_NotifyIconW(NIM_MODIFY, &g_trayIcon);
}

bool AddTrayIcon(HWND window) {
    g_trayIcon = {};
    g_trayIcon.cbSize = sizeof(g_trayIcon);
    g_trayIcon.hWnd = window;
    g_trayIcon.uID = kTrayIconId;
    g_trayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_trayIcon.uCallbackMessage = kTrayCallbackMessage;
    g_trayIcon.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    const std::wstring tooltip = std::wstring(kAppName) + L": Enabled";
    wcsncpy_s(g_trayIcon.szTip, tooltip.c_str(), _TRUNCATE);

    if (!Shell_NotifyIconW(NIM_ADD, &g_trayIcon)) {
        return false;
    }

    g_trayIcon.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &g_trayIcon);
    return true;
}

void RemoveTrayIcon() {
    if (g_trayIcon.cbSize != 0) {
        Shell_NotifyIconW(NIM_DELETE, &g_trayIcon);
        g_trayIcon = {};
    }
}

void ReleaseModifierKeys() {
    constexpr UINT keys[] = { VK_LMENU, VK_RMENU, VK_LWIN, VK_RWIN };
    for (const UINT key : keys) {
        SendKey(key, true);
    }
}

void ShowTrayMenu(HWND window) {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }

    AppendMenuW(
        menu,
        MF_STRING | (g_enabled ? MF_CHECKED : MF_UNCHECKED),
        kMenuToggleEnabled,
        L"Enabled");
    AppendMenuW(
        menu,
        MF_STRING | (IsStartupEnabled() ? MF_CHECKED : MF_UNCHECKED),
        kMenuToggleStartup,
        L"Start with Windows");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kMenuExit, L"Exit");

    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
        cursor.x, cursor.y, 0, window, nullptr);
    DestroyMenu(menu);
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        if (!AddTrayIcon(window)) {
            return -1;
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case kMenuToggleEnabled:
            g_enabled = !g_enabled;
            ReleaseModifierKeys();
            UpdateTrayIcon();
            return 0;
        case kMenuToggleStartup:
            SetStartupEnabled(!IsStartupEnabled());
            return 0;
        case kMenuExit:
            DestroyWindow(window);
            return 0;
        default:
            return 0;
        }

    case kTrayCallbackMessage:
        if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
            ShowTrayMenu(window);
        } else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) {
            g_enabled = !g_enabled;
            ReleaseModifierKeys();
            UpdateTrayIcon();
        }
        return 0;

    case WM_DESTROY:
        ReleaseModifierKeys();
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

bool RegisterMainWindowClass(HINSTANCE instance) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kWindowClassName;

    return RegisterClassExW(&windowClass) != 0;
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    HANDLE singleInstanceMutex = CreateMutexW(nullptr, TRUE, L"Local\\MacAltWinSwapInstance");
    if (singleInstanceMutex == nullptr) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"MacAltWinSwap is already running.", kAppName, MB_OK | MB_ICONINFORMATION);
        CloseHandle(singleInstanceMutex);
        return 0;
    }

    if (!RegisterMainWindowClass(instance)) {
        MessageBoxW(nullptr, L"Failed to register window class.", kAppName, MB_OK | MB_ICONERROR);
        CloseHandle(singleInstanceMutex);
        return 1;
    }

    g_window = CreateWindowExW(
        0,
        kWindowClassName,
        kAppName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (g_window == nullptr) {
        MessageBoxW(nullptr, L"Failed to create window.", kAppName, MB_OK | MB_ICONERROR);
        CloseHandle(singleInstanceMutex);
        return 1;
    }

    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, instance, 0);
    if (g_keyboardHook == nullptr) {
        MessageBoxW(nullptr, L"Failed to install keyboard hook.", kAppName, MB_OK | MB_ICONERROR);
        DestroyWindow(g_window);
        CloseHandle(singleInstanceMutex);
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (g_keyboardHook != nullptr) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }

    CloseHandle(singleInstanceMutex);
    return 0;
}
