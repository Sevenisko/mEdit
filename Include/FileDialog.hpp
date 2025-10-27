#pragma once
#include <IO/FileSystem.h>
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <algorithm>
#include <Uxtheme.h>

// Ensure modern common controls (for visual styles)
#pragma comment( \
    linker,      \
    "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

class FileDialog {
  public:
    enum class Mode { OpenFile, SaveFile, SelectDirectory };

    struct Filter {
        std::string description; // e.g., "Text Files (*.txt)"
        std::string pattern; // e.g., "*.txt"
    };

    struct SelectionResult {
        std::string path;
        bool isFile;
        bool valid;
    };

    FileDialog(Mode mode,
               const std::string& title = "File Dialog",
               const std::vector<Filter>& filters = {},
               int width = 600,
               int height = 400,
               int x = 100,
               int y = 100)
        : m_Mode(mode),
          m_Title(title),
          m_Filters(filters),
          m_Width(width),
          m_Height(height),
          m_X(x),
          m_Y(y),
          m_CurrentDir(FileSystem::GetRootDir()),
          m_SelectedFilterIndex(0),
          m_FileNameInput(mode == Mode::SaveFile ? "newfile.txt" : ""),
          m_Result({"", mode != Mode::SelectDirectory, false}),
          m_IsUpdatingAddressBar(false),
          m_IsTextboxEdited(false),
          m_hFont(nullptr) {
        if(filters.empty()) { m_Filters.push_back({"All Files (*.*)", "*.*"}); }
    }

    ~FileDialog() {
        if(m_hFont) DeleteObject(m_hFont);
    }

    void SetFilters(const std::vector<Filter>& newFilters) {
        m_Filters = newFilters;
        if(m_Filters.empty()) { m_Filters.push_back({"All Files (*.*)", "*.*"}); }
        m_SelectedFilterIndex = 0;
    }

    SelectionResult GetResult() const { return m_Result; }

    bool Show(HWND hwndParent) {
        HICON icon = LoadIconA(GetModuleHandleA(NULL), MAKEINTRESOURCE(101));

        WNDCLASSEXA wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.lpfnWndProc = DialogProcStatic;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = "FileDialogClass";
        wc.hbrBackground = CreateSolidBrush(RGB(45, 45, 45)); // Dark background
        wc.hIcon = icon;
        wc.hIconSm = icon;
        if(!RegisterClassExA(&wc)) { return false; }

        m_hFont = CreateFontA(16,
                              0,
                              0,
                              0,
                              FW_NORMAL,
                              FALSE,
                              FALSE,
                              FALSE,
                              ANSI_CHARSET,
                              OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS,
                              DEFAULT_QUALITY,
                              DEFAULT_PITCH | FF_SWISS,
                              "Segoe UI");

        HWND hwnd = CreateWindowExA(0,
                                    "FileDialogClass",
                                    m_Title.c_str(),
                                    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                    m_X,
                                    m_Y,
                                    m_Width,
                                    m_Height,
                                    hwndParent,
                                    nullptr,
                                    GetModuleHandleA(nullptr),
                                    this);

        if(!hwnd) { return false; }

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg;
        while(GetMessageA(&msg, nullptr, 0, 0) > 0) {
            if(!IsDialogMessageA(hwnd, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            if(!IsWindow(hwnd)) { break; }
        }

        UnregisterClassA("FileDialogClass", GetModuleHandleA(nullptr));
        return m_Result.valid;
    }

  private:
    Mode m_Mode;
    std::string m_Title;
    std::vector<Filter> m_Filters;
    int m_SelectedFilterIndex;
    const FileSystem::Directory* m_CurrentDir;
    FileSystem::Directory* m_SelectedDir = nullptr;
    std::vector<std::string> m_PathSegments;
    std::string m_FileNameInput;
    SelectionResult m_Result;
    int m_Width, m_Height, m_X, m_Y;
    bool m_IsUpdatingAddressBar;
    bool m_IsTextboxEdited;
    HFONT m_hFont;
    HWND m_hwndListView = nullptr;
    HWND m_hwndEdit = nullptr;
    HWND m_hwndCombo = nullptr;
    HWND m_hwndOK = nullptr;
    HWND m_hwndCancel = nullptr;
    HWND m_hwndAddressBar = nullptr;
    HWND m_hwndParentButton = nullptr;
    WNDPROC m_origHeaderProc = nullptr;
    WNDPROC m_origComboProc = nullptr;

    std::string FormatFileSize(uint64_t bytes) const {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        double size = static_cast<double>(bytes);
        int unitIndex = 0;
        while(size >= 1024 && unitIndex < 4) {
            size /= 1024;
            unitIndex++;
        }
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0) << size << " " << units[unitIndex];
        return ss.str();
    }

    std::string GetFileType(const std::string& fileName) const {
        size_t dotPos = fileName.find_last_of('.');
        if(dotPos == std::string::npos || dotPos == fileName.length() - 1) { return "File"; }
        std::string ext = fileName.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
        return ext + " File";
    }

    std::regex BuildFilterRegex(const std::string& pattern) const {
        try {
            if(pattern == "*.*") { return std::regex(".*", std::regex::icase); }
            std::vector<std::string> extensions;
            std::string current;
            for(char c: pattern) {
                if(c == ';') {
                    if(!current.empty()) extensions.push_back(current);
                    current.clear();
                } else {
                    current += c;
                }
            }
            if(!current.empty()) extensions.push_back(current);

            std::string regexStr = ".*\\.(";
            for(size_t i = 0; i < extensions.size(); ++i) {
                std::string ext = extensions[i];
                if(ext.substr(0, 2) == "*.") { ext = ext.substr(2); }
                std::string escaped;
                for(char c: ext) {
                    if(c == '.' || c == '*' || c == '+' || c == '?' || c == '|' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
                       c == '^' || c == '$') {
                        escaped += '\\';
                    }
                    escaped += c;
                }
                regexStr += escaped;
                if(i < extensions.size() - 1) regexStr += "|";
            }
            regexStr += ")$";

            return std::regex(regexStr, std::regex::icase);
        } catch(...) {
            return std::regex(".*", std::regex::icase); // Fallback to match all
        }
    }

    void PopulateListView() {
        SendMessageA(m_hwndListView, LVM_DELETEALLITEMS, 0, 0);
        ListView_SetBkColor(m_hwndListView, RGB(30, 30, 30)); // Dark background
        std::regex regexPattern = BuildFilterRegex(m_Filters[m_SelectedFilterIndex].pattern);

        for(const auto& dir: m_CurrentDir->directories) {
            if(dir.name.empty()) continue;
            LVITEMA item = {0};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = ListView_GetItemCount(m_hwndListView);
            std::string dirName = dir.name + "\\";
            item.pszText = const_cast<LPSTR>(dirName.c_str());
            item.lParam = reinterpret_cast<LPARAM>(&dir);
            ListView_InsertItem(m_hwndListView, &item);
            ListView_SetItemText(m_hwndListView, item.iItem, 1, const_cast<LPSTR>("File folder"));
            ListView_SetItemText(m_hwndListView, item.iItem, 2, const_cast<LPSTR>(""));
        }

        if(m_Mode != Mode::SelectDirectory) {
            for(const auto& file: m_CurrentDir->files) {
                if(file.name.empty()) continue;
                bool matches = std::regex_match(file.name, regexPattern);
                if(matches || m_Filters[m_SelectedFilterIndex].pattern == "*.*") {
                    LVITEMA item = {0};
                    item.mask = LVIF_TEXT | LVIF_PARAM;
                    item.iItem = ListView_GetItemCount(m_hwndListView);
                    item.pszText = const_cast<LPSTR>(file.name.c_str());
                    item.lParam = reinterpret_cast<LPARAM>(&file);
                    ListView_InsertItem(m_hwndListView, &item);
                    std::string type = GetFileType(file.name);
                    ListView_SetItemText(m_hwndListView, item.iItem, 1, const_cast<LPSTR>(type.c_str()));
                    std::string sizeStr = FormatFileSize(file.size);
                    ListView_SetItemText(m_hwndListView, item.iItem, 2, const_cast<LPSTR>(sizeStr.c_str()));
                }
            }
        }
        InvalidateRect(m_hwndListView, nullptr, TRUE);
        UpdateWindow(m_hwndListView);
    }

    void PopulateComboBox() {
        SendMessageA(m_hwndCombo, CB_RESETCONTENT, 0, 0);
        for(const auto& filter: m_Filters) {
            SendMessageA(m_hwndCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(filter.description.c_str()));
        }
        SendMessageA(m_hwndCombo, CB_SETCURSEL, m_SelectedFilterIndex, 0);
    }

    void UpdatePathDisplay(HWND hwnd) {
        if(m_IsUpdatingAddressBar) return;
        m_IsUpdatingAddressBar = true;
        std::string pathDisplay;
        for(size_t i = 0; i < m_PathSegments.size(); ++i) {
            if(i > 0) pathDisplay += "\\";
            pathDisplay += m_PathSegments[i];
        }
        SetWindowTextA(m_hwndAddressBar, pathDisplay.empty() ? "\\" : pathDisplay.c_str());
        m_IsUpdatingAddressBar = false;
    }

    bool NavigateToPath(const std::string& path) {
        std::vector<std::string> segments;
        std::string current;
        for(char c: path) {
            if(c == '\\' || c == '/') {
                if(!current.empty()) segments.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
        if(!current.empty()) segments.push_back(current);

        const FileSystem::Directory* dir = FileSystem::GetRootDir();
        std::vector<std::string> newSegments = {dir->name};
        for(size_t i = 1; i < segments.size(); ++i) {
            bool found = false;
            for(const auto& subDir: dir->directories) {
                if(subDir.name == segments[i]) {
                    dir = &subDir;
                    newSegments.push_back(subDir.name);
                    found = true;
                    break;
                }
            }
            if(!found) return false;
        }

        m_CurrentDir = dir;
        m_PathSegments = newSegments;
        m_SelectedDir = (m_Mode == Mode::SelectDirectory) ? const_cast<FileSystem::Directory*>(dir) : nullptr;
        return true;
    }

    void handleOK(HWND hwnd) {
        if(m_hwndEdit) {
            char buffer[1024];
            GetWindowTextA(m_hwndEdit, buffer, 1024);
            m_FileNameInput = buffer;
        }

        if(m_Mode == Mode::SelectDirectory) {
            int selected = ListView_GetNextItem(m_hwndListView, -1, LVNI_SELECTED);
            if(selected != -1) {
                LVITEMA item = {0};
                item.iItem = selected;
                item.mask = LVIF_PARAM;
                ListView_GetItem(m_hwndListView, &item);
                if(item.lParam >= reinterpret_cast<LPARAM>(m_CurrentDir->directories.data()) &&
                   item.lParam < reinterpret_cast<LPARAM>(m_CurrentDir->directories.data() + m_CurrentDir->directories.size())) {
                    m_SelectedDir = const_cast<FileSystem::Directory*>(reinterpret_cast<FileSystem::Directory*>(item.lParam));
                } else {
                    m_SelectedDir = const_cast<FileSystem::Directory*>(m_CurrentDir);
                }
            } else {
                m_SelectedDir = const_cast<FileSystem::Directory*>(m_CurrentDir);
            }

            std::string basePath = BuildSelectedDirPath();
            m_Result.path = basePath;
            if(m_IsTextboxEdited && !m_FileNameInput.empty() && m_FileNameInput != m_SelectedDir->name) {
                if(!basePath.empty() && basePath.back() != '\\') basePath += "\\";
                m_Result.path = basePath + m_FileNameInput;
                m_Result.isFile = true;
            } else {
                m_Result.isFile = false;
            }
            m_Result.valid = !m_Result.path.empty();
        } else if(!m_FileNameInput.empty()) {
            std::string basePath = BuildPath();
            if(!basePath.empty() && basePath.back() != '\\') basePath += "\\";
            m_Result.path = basePath + m_FileNameInput;
            m_Result.isFile = true;
            m_Result.valid = true;
        } else {
            m_Result.valid = false;
        }
        DestroyWindow(hwnd);
    }

    std::string BuildPath() const {
        std::string path;
        for(size_t i = 1; i < m_PathSegments.size(); ++i) {
            path += m_PathSegments[i];
            if(i < m_PathSegments.size() - 1) path += "\\";
        }
        return path;
    }

    std::string BuildSelectedDirPath() const {
        if(!m_SelectedDir) return BuildPath();
        std::vector<std::string> segments;
        auto dir = m_SelectedDir;
        while(dir) {
            segments.insert(segments.begin(), dir->name);
            dir = const_cast<FileSystem::Directory*>(FindParent(dir));
        }
        std::string path;
        for(size_t i = 1; i < segments.size(); ++i) {
            path += segments[i];
            if(i < segments.size() - 1) path += "\\";
        }
        return path;
    }

    const FileSystem::Directory* FindParent(const FileSystem::Directory* dir) const { return FindParentRecursive(FileSystem::GetRootDir(), dir); }

    const FileSystem::Directory* FindParentRecursive(const FileSystem::Directory* current, const FileSystem::Directory* target) const {
        if(!current || !target) return nullptr;
        for(const auto& subDir: current->directories) {
            if(&subDir == target) return current;
            auto result = FindParentRecursive(&subDir, target);
            if(result) return result;
        }
        return nullptr;
    }

    static LRESULT CALLBACK HeaderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        FileDialog* pThis = reinterpret_cast<FileDialog*>(dwRefData);
        switch(msg) {
        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            if(nmhdr->code == NM_CUSTOMDRAW) {
                LPNMCUSTOMDRAW cd = (LPNMCUSTOMDRAW)lParam;
                switch(cd->dwDrawStage) {
                case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
                case CDDS_ITEMPREPAINT: {
                    SetTextColor(cd->hdc, RGB(200, 200, 200));
                    SetBkColor(cd->hdc, RGB(45, 45, 45));
                    FillRect(cd->hdc, &cd->rc, CreateSolidBrush(RGB(45, 45, 45)));
                    return CDRF_DODEFAULT;
                }
                case CDDS_PREERASE: {
                    SetBkColor(cd->hdc, RGB(45, 45, 45));
                    FillRect(cd->hdc, &cd->rc, CreateSolidBrush(RGB(45, 45, 45)));
                    return CDRF_DODEFAULT;
                }
                }
            }
            break;
        }
        case WM_DESTROY: RemoveWindowSubclass(hwnd, HeaderProc, uIdSubclass); break;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    static LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        FileDialog* pThis = reinterpret_cast<FileDialog*>(dwRefData);
        switch(msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            SetTextColor(hdc, RGB(200, 200, 200));
            SetBkColor(hdc, RGB(30, 30, 30));
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CTLCOLORLISTBOX:
            SetTextColor((HDC)wParam, RGB(200, 200, 200));
            SetBkColor((HDC)wParam, RGB(30, 30, 30));
            return (LRESULT)CreateSolidBrush(RGB(30, 30, 30));
        case WM_DESTROY: RemoveWindowSubclass(hwnd, ComboProc, uIdSubclass); break;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    void InitControls(HWND hwnd) {
        INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES};
        if(!InitCommonControlsEx(&icc)) { return; }

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int clientWidth = clientRect.right - clientRect.left;
        int clientHeight = clientRect.bottom - clientRect.top;

        m_hwndParentButton = CreateWindowA("BUTTON", "<", WS_CHILD | WS_VISIBLE, 10, 10, 30, 24, hwnd, (HMENU)IDC_BUTTON_PARENT, nullptr, nullptr);
        if(m_hwndParentButton) {
            SetWindowTheme(m_hwndParentButton, L"DarkMode_Explorer", nullptr);
            SendMessageA(m_hwndParentButton, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        }

        m_hwndAddressBar = CreateWindowA("EDIT",
                                         "\\",
                                         WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                         50,
                                         10,
                                         clientWidth - 70,
                                         24,
                                         hwnd,
                                         (HMENU)IDC_ADDRESS_BAR,
                                         nullptr,
                                         nullptr);
        if(m_hwndAddressBar) {
            SetWindowTheme(m_hwndAddressBar, L"Explorer", nullptr);
            SendMessageA(m_hwndAddressBar, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        }

        m_hwndListView = CreateWindowA(WC_LISTVIEWA,
                                       "",
                                       WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                       10,
                                       40,
                                       clientWidth - 20,
                                       clientHeight - 160,
                                       hwnd,
                                       (HMENU)IDC_LISTVIEW,
                                       nullptr,
                                       nullptr);
        if(m_hwndListView) {
            ListView_SetExtendedListViewStyle(m_hwndListView, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
            ListView_SetBkColor(m_hwndListView, RGB(30, 30, 30));
            SetWindowTheme(m_hwndListView, L"DarkMode_Explorer", nullptr);
            SendMessageA(m_hwndListView, WM_SETFONT, (WPARAM)m_hFont, TRUE);

            HWND hwndHeader = ListView_GetHeader(m_hwndListView);
            if(hwndHeader) {
                SetWindowTheme(hwndHeader, L"DarkMode_Explorer", nullptr);
                SetWindowSubclass(hwndHeader, HeaderProc, 0, reinterpret_cast<DWORD_PTR>(this));
            }
        }

        LVCOLUMNA col = {0};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.pszText = const_cast<LPSTR>("Name");
        col.cx = clientWidth - 220;
        ListView_InsertColumn(m_hwndListView, 0, &col);
        col.pszText = const_cast<LPSTR>("Type");
        col.cx = 100;
        ListView_InsertColumn(m_hwndListView, 1, &col);
        col.pszText = const_cast<LPSTR>("Size");
        col.cx = 100;
        ListView_InsertColumn(m_hwndListView, 2, &col);

        if(m_Mode == Mode::SelectDirectory) {
            HWND hwndStaticFileName =
                CreateWindowA("STATIC", "Directory name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, clientHeight - 106, 120, 24, hwnd, nullptr, nullptr, nullptr);
            if(hwndStaticFileName) { SendMessageA(hwndStaticFileName, WM_SETFONT, (WPARAM)m_hFont, TRUE); }
            m_hwndEdit = CreateWindowA("EDIT",
                                       m_FileNameInput.c_str(),
                                       WS_CHILD | WS_VISIBLE | WS_BORDER,
                                       100,
                                       clientHeight - 110,
                                       clientWidth - 120,
                                       24,
                                       hwnd,
                                       (HMENU)IDC_EDIT_FILENAME,
                                       nullptr,
                                       nullptr);
            if(m_hwndEdit) {
                SetWindowTheme(m_hwndEdit, L"Explorer", nullptr);
                SendMessageA(m_hwndEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);
            }
        } else {
            HWND hwndStaticFileName =
                CreateWindowA("STATIC", "File name:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, clientHeight - 110, 80, 24, hwnd, nullptr, nullptr, nullptr);
            if(hwndStaticFileName) { SendMessageA(hwndStaticFileName, WM_SETFONT, (WPARAM)m_hFont, TRUE); }
            m_hwndEdit = CreateWindowA("EDIT",
                                       m_FileNameInput.c_str(),
                                       WS_CHILD | WS_VISIBLE | WS_BORDER,
                                       90,
                                       clientHeight - 110,
                                       clientWidth - 80,
                                       24,
                                       hwnd,
                                       (HMENU)IDC_EDIT_FILENAME,
                                       nullptr,
                                       nullptr);
            if(m_hwndEdit) {
                SetWindowTheme(m_hwndEdit, L"Explorer", nullptr);
                SendMessageA(m_hwndEdit, WM_SETFONT, (WPARAM)m_hFont, TRUE);
            }
        }

        if(m_Mode != Mode::SelectDirectory) {
            HWND hwndStaticFilter =
                CreateWindowA("STATIC", "Files of type:", WS_CHILD | WS_VISIBLE | SS_LEFT, 10, clientHeight - 80, 80, 24, hwnd, nullptr, nullptr, nullptr);
            if(hwndStaticFilter) { SendMessageA(hwndStaticFilter, WM_SETFONT, (WPARAM)m_hFont, TRUE); }
            m_hwndCombo = CreateWindowA("COMBOBOX",
                                        "",
                                        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                                        90,
                                        clientHeight - 80,
                                        200,
                                        100,
                                        hwnd,
                                        (HMENU)IDC_COMBO_FILTER,
                                        nullptr,
                                        nullptr);
            if(m_hwndCombo) {
                SetWindowTheme(m_hwndCombo, L"DarkMode_Explorer", nullptr);
                SendMessageA(m_hwndCombo, WM_SETFONT, (WPARAM)m_hFont, TRUE);
                SetWindowSubclass(m_hwndCombo, ComboProc, 0, reinterpret_cast<DWORD_PTR>(this));
            }
        }

        m_hwndOK = CreateWindowA("BUTTON",
                                 m_Mode == Mode::SaveFile ? "Save" : "Open",
                                 WS_CHILD | WS_VISIBLE,
                                 clientWidth - 180,
                                 clientHeight - 50,
                                 80,
                                 30,
                                 hwnd,
                                 (HMENU)IDOK,
                                 nullptr,
                                 nullptr);
        if(m_hwndOK) {
            SetWindowTheme(m_hwndOK, L"DarkMode_Explorer", nullptr);
            SendMessageA(m_hwndOK, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        }
        m_hwndCancel =
            CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE, clientWidth - 90, clientHeight - 50, 80, 30, hwnd, (HMENU)IDCANCEL, nullptr, nullptr);
        if(m_hwndCancel) {
            SetWindowTheme(m_hwndCancel, L"DarkMode_Explorer", nullptr);
            SendMessageA(m_hwndCancel, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        }

        m_PathSegments.push_back(m_CurrentDir->name);
        UpdatePathDisplay(hwnd);
        PopulateListView();
        if(m_hwndCombo) { PopulateComboBox(); }
    }

    static LRESULT CALLBACK DialogProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        FileDialog* pThis = nullptr;
        if(msg == WM_CREATE) {
            pThis = reinterpret_cast<FileDialog*>(((CREATESTRUCTA*)lParam)->lpCreateParams);
            SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        } else {
            pThis = reinterpret_cast<FileDialog*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
        }
        return pThis ? pThis->DialogProc(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    LRESULT DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch(msg) {
        case WM_CREATE: InitControls(hwnd); return 0;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORLISTBOX:
            SetTextColor((HDC)wParam, RGB(200, 200, 200));
            SetBkColor((HDC)wParam, RGB(45, 45, 45));
            return (LRESULT)CreateSolidBrush(RGB(45, 45, 45));

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
            if(pdis->CtlID == IDOK || pdis->CtlID == IDCANCEL || pdis->CtlID == IDC_BUTTON_PARENT) {
                HDC hdc = pdis->hDC;
                SetTextColor(hdc, RGB(200, 200, 200));
                SetBkColor(hdc, RGB(45, 45, 45));
                char text[256];
                GetWindowTextA(pdis->hwndItem, text, 256);
                DrawTextA(hdc, text, -1, &pdis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                if(pdis->itemState & ODS_SELECTED) {
                    FrameRect(hdc, &pdis->rcItem, CreateSolidBrush(RGB(100, 100, 100)));
                } else if(pdis->itemState & ODS_FOCUS) {
                    FrameRect(hdc, &pdis->rcItem, CreateSolidBrush(RGB(80, 80, 80)));
                }
                return TRUE;
            }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            if(nmhdr->idFrom == IDC_LISTVIEW) {
                if(nmhdr->code == NM_CLICK) {
                    LPNMITEMACTIVATE nmia = (LPNMITEMACTIVATE)lParam;
                    int selected = nmia->iItem;
                    if(selected != -1) {
                        LVITEMA item = {0};
                        item.iItem = selected;
                        item.mask = LVIF_PARAM;
                        ListView_GetItem(m_hwndListView, &item);
                        if(m_Mode != Mode::SelectDirectory) {
                            // Check if the selected item is a file
                            if(item.lParam < reinterpret_cast<LPARAM>(m_CurrentDir->directories.data()) ||
                               item.lParam >= reinterpret_cast<LPARAM>(m_CurrentDir->directories.data() + m_CurrentDir->directories.size())) {
                                const FileSystem::File* file = reinterpret_cast<FileSystem::File*>(item.lParam);
                                m_FileNameInput = file->name;
                                m_IsTextboxEdited = false;
                                SetWindowTextA(m_hwndEdit, m_FileNameInput.c_str());
                            }
                        } else {
                            // Existing behavior for SelectDirectory mode
                            if(item.lParam >= reinterpret_cast<LPARAM>(m_CurrentDir->directories.data()) &&
                               item.lParam < reinterpret_cast<LPARAM>(m_CurrentDir->directories.data() + m_CurrentDir->directories.size())) {
                                const FileSystem::Directory* dir = reinterpret_cast<FileSystem::Directory*>(item.lParam);
                                m_FileNameInput = dir->name;
                                m_IsTextboxEdited = false;
                                SetWindowTextA(m_hwndEdit, m_FileNameInput.c_str());
                                m_SelectedDir = const_cast<FileSystem::Directory*>(dir);
                            }
                        }
                    }
                    return 0;
                } else if(nmhdr->code == NM_DBLCLK) {
                    int selected = ListView_GetNextItem(m_hwndListView, -1, LVNI_SELECTED);
                    if(selected != -1) {
                        LVITEMA item = {0};
                        item.iItem = selected;
                        item.mask = LVIF_PARAM;
                        ListView_GetItem(m_hwndListView, &item);
                        if(item.lParam >= reinterpret_cast<LPARAM>(m_CurrentDir->directories.data()) &&
                           item.lParam < reinterpret_cast<LPARAM>(m_CurrentDir->directories.data() + m_CurrentDir->directories.size())) {
                            // Directory double-clicked
                            m_CurrentDir = reinterpret_cast<FileSystem::Directory*>(item.lParam);
                            m_PathSegments.push_back(m_CurrentDir->name);
                            if(m_Mode == Mode::SelectDirectory) {
                                m_SelectedDir = const_cast<FileSystem::Directory*>(m_CurrentDir);
                                m_FileNameInput = m_CurrentDir->name;
                                m_IsTextboxEdited = false;
                                SetWindowTextA(m_hwndEdit, m_FileNameInput.c_str());
                            }
                            UpdatePathDisplay(hwnd);
                            PopulateListView();
                        } else if(m_Mode != Mode::SelectDirectory) {
                            // File double-clicked in OpenFile or SaveFile mode
                            const FileSystem::File* file = reinterpret_cast<FileSystem::File*>(item.lParam);
                            m_FileNameInput = file->name;
                            m_IsTextboxEdited = false;
                            SetWindowTextA(m_hwndEdit, m_FileNameInput.c_str());
                            handleOK(hwnd);
                        }
                    }
                    return 0;
                } else if(nmhdr->code == NM_CUSTOMDRAW) {
                    LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lParam;
                    switch(cd->nmcd.dwDrawStage) {
                    case CDDS_PREPAINT: return CDRF_NOTIFYITEMDRAW;
                    case CDDS_ITEMPREPAINT:
                        cd->clrText = RGB(200, 200, 200);
                        cd->clrTextBk = RGB(45, 45, 45);
                        return CDRF_DODEFAULT;
                    case CDDS_PREERASE: SetBkColor(cd->nmcd.hdc, RGB(45, 45, 45)); return CDRF_DODEFAULT;
                    }
                }
            }
            break;
        }

        case WM_COMMAND:
            if(LOWORD(wParam) == IDOK) {
                handleOK(hwnd);
                return 0;
            } else if(LOWORD(wParam) == IDCANCEL) {
                m_Result.valid = false;
                DestroyWindow(hwnd);
                return 0;
            } else if(HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_COMBO_FILTER) {
                m_SelectedFilterIndex = SendMessageA(m_hwndCombo, CB_GETCURSEL, 0, 0);
                PopulateListView();
                return 0;
            } else if(LOWORD(wParam) == IDC_BUTTON_PARENT) {
                const FileSystem::Directory* parent = FindParent(m_CurrentDir);
                if(parent) {
                    m_CurrentDir = parent;
                    m_PathSegments.pop_back();
                    if(m_Mode == Mode::SelectDirectory) {
                        m_SelectedDir = const_cast<FileSystem::Directory*>(m_CurrentDir);
                        m_FileNameInput = m_CurrentDir->name;
                        m_IsTextboxEdited = false;
                        SetWindowTextA(m_hwndEdit, m_FileNameInput.c_str());
                    }
                    UpdatePathDisplay(hwnd);
                    PopulateListView();
                }
                return 0;
            } else if(HIWORD(wParam) == EN_UPDATE && LOWORD(wParam) == IDC_ADDRESS_BAR) {
                if(GetKeyState(VK_RETURN) & 0x8000) {
                    char buffer[1024];
                    GetWindowTextA(m_hwndAddressBar, buffer, 1024);
                    std::string path = buffer;
                    if(!path.empty() && NavigateToPath(path)) {
                        if(m_Mode == Mode::SelectDirectory) {
                            m_FileNameInput = m_CurrentDir->name;
                            m_IsTextboxEdited = false;
                            SetWindowTextA(m_hwndEdit, m_FileNameInput.c_str());
                        }
                        UpdatePathDisplay(hwnd);
                        PopulateListView();
                    }
                }
                return 0;
            } else if(HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_EDIT_FILENAME) {
                m_IsTextboxEdited = true;
                return 0;
            }
            break;

        case WM_DESTROY:
            DeleteObject((HGDIOBJ)GetClassLongPtrA(hwnd, GCLP_HBRBACKGROUND));
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

    enum { IDC_LISTVIEW = 100, IDC_EDIT_FILENAME, IDC_COMBO_FILTER, IDC_ADDRESS_BAR, IDC_BUTTON_PARENT, IDC_STATIC_PATH_LABEL };
};