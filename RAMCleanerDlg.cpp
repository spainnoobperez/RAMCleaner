#include "stdafx.h"
#include "RAMCleaner.h"
#include "RAMCleanerDlg.h"
#include "afxdialogex.h"
#include <commctrl.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include "aerosubc.h"

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"AeroGlassGDI.lib")

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialogEx {
public:
    CAboutDlg();

    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange *pDX);

protected:
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD) {
}

void CAboutDlg::DoDataExchange(CDataExchange *pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
    ON_WM_TIMER()
END_MESSAGE_MAP()

CRAMCleanerDlg::CRAMCleanerDlg(CWnd *pParent )
    : CDialogEx(CRAMCleanerDlg::IDD, pParent) {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRAMCleanerDlg::DoDataExchange(CDataExchange *pDX) {
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CRAMCleanerDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON1, &CRAMCleanerDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CRAMCleanerDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON3, &CRAMCleanerDlg::OnBnClickedButton3)
    ON_BN_CLICKED(IDC_BUTTON4, &CRAMCleanerDlg::OnBnClickedButton4)
    ON_MESSAGE(WM_SHOWTASK, onShowTask)
    ON_BN_CLICKED(IDC_CHECK1, &CRAMCleanerDlg::OnBnClickedCheck1)
END_MESSAGE_MAP()

BOOL CRAMCleanerDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    CMenu *pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL) {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);
    AllAero(m_hWnd);
    return TRUE;
}

void CRAMCleanerDlg::OnSysCommand(UINT nID, LPARAM lParam) {
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else
        CDialogEx::OnSysCommand(nID, lParam);
}

void CRAMCleanerDlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this);
        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        dc.DrawIcon(x, y, m_hIcon);
    } else
        CDialogEx::OnPaint();
}

HCURSOR CRAMCleanerDlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(m_hIcon);
}

struct Info {
    BOOL isOk;
    int processess;
    int TotalMem;
    int AvaiMem;
};

void AdjustTokenPrivilegesForNT() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    OpenProcessToken(GetCurrentProcess(),
                     TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid);
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
}

Info EmptyAllSet() {
    HANDLE SnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (SnapShot == NULL) return {FALSE, 0, 0, 0};
    PROCESSENTRY32 ProcessInfo;
    ProcessInfo.dwSize = sizeof(ProcessInfo);
    int count = 0;
    BOOL Status = Process32First(SnapShot, &ProcessInfo);
    while(Status) {
        count ++;
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE,
                                      ProcessInfo.th32ProcessID);
        if(hProcess) {
            SetProcessWorkingSetSize(hProcess, -1, -1);
            EmptyWorkingSet(hProcess);
            CloseHandle(hProcess);
        }
        Status = Process32Next(SnapShot, &ProcessInfo);
    }
    MEMORYSTATUSEX memStatus;
    typedef  void(WINAPI * FunctionGlobalMemoryStatusEx)(LPMEMORYSTATUSEX);
    memStatus.dwLength = sizeof(memStatus);
    FunctionGlobalMemoryStatusEx MyGlobalMemoryStatusEx;
    MyGlobalMemoryStatusEx = (FunctionGlobalMemoryStatusEx)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GlobalMemoryStatusEx");
    if (NULL == MyGlobalMemoryStatusEx) return{ FALSE, 0, 0, 0 };
    MyGlobalMemoryStatusEx(&memStatus);
    return { TRUE, count, (int)(memStatus.ullTotalPhys / 1024 / 1024), (int)(memStatus.ullAvailPhys / 1024 / 1024) };
}

bool isStarted = false, backNotify = false;

void CRAMCleanerDlg::OnBnClickedButton1() {
    AdjustTokenPrivilegesForNT();
    Info ii = EmptyAllSet();
    if(ii.isOk) {
        CTime time = CTime::GetCurrentTime();
        WCHAR str[10240];
        CString date = time.Format("%Y%m%d-%H:%M:%S");
        wsprintf(str, L"Time:%s\nTotal number of processes:%d\nTotal RAM Size:%dMB\nAvailable RAM size after cleaning:%dMB\n", date, ii.processess, ii.TotalMem, ii.AvaiMem);
        TaskDialog(NULL, NULL,
                   _T("Tips"),
                   _T("Success"),
                   str,
                   TDCBF_OK_BUTTON,
                   TD_INFORMATION_ICON,
                   NULL);
    } else {
        TaskDialog(NULL, NULL,
                   _T("Error"),
                   _T("Error occurred while cleaning"),
                   NULL,
                   TDCBF_OK_BUTTON,
                   TD_ERROR_ICON,
                   NULL);
    }
}

NOTIFYICONDATA nid;
bool minize = false;

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime ) {
    AdjustTokenPrivilegesForNT();
    Info ii = EmptyAllSet();
    if(ii.isOk) {
        CTime time = CTime::GetCurrentTime();
        WCHAR str[10240];
        CString date = time.Format("%Y%m%d-%H:%M:%S");
        wsprintf(str, L"Time:%s\nTotal number of processes:%d\nTotal RAM Size:%dMB\nAvailable RAM size after cleaning:%dMB\n", date, ii.processess, ii.TotalMem, ii.AvaiMem);
        if (!minize) {
            TaskDialog(NULL, NULL,
                       _T("Tips"),
                       _T("Success"),
                       str,
                       TDCBF_OK_BUTTON,
                       TD_INFORMATION_ICON,
                       NULL);
        } else {
            if (backNotify) {
                Shell_NotifyIcon(NIM_DELETE, &nid);
                nid.cbSize = sizeof(NOTIFYICONDATA);
                nid.uID = IDR_MAINFRAME;
                nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
                nid.hWnd = hwnd;
                nid.uCallbackMessage = WM_SHOWTASK;
                nid.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
                wsprintf(nid.szTip, L"Click here to show the mainframe");
                wsprintf(nid.szInfo, str);
                wsprintf(nid.szInfoTitle, L"Autocleaning completed!");
                Shell_NotifyIcon(NIM_ADD, &nid);
            }
        }
    } else {
        if (!minize) {
            TaskDialog(NULL, NULL,
                       _T("Error"),
                       _T("Error occurred while cleaning"),
                       NULL,
                       TDCBF_OK_BUTTON,
                       TD_ERROR_ICON,
                       NULL);
        } else {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.uID = IDR_MAINFRAME;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
            nid.hWnd = hwnd;
            nid.uCallbackMessage = WM_SHOWTASK;
            nid.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
            wsprintf(nid.szTip, L"Click here to show the mainframe");
            wsprintf(nid.szInfo, L"Please restart the program");
            wsprintf(nid.szInfoTitle, L"Error occurred while autocleaning");
            Shell_NotifyIcon(NIM_ADD, &nid);
        }
    }
}

void CRAMCleanerDlg::OnBnClickedButton2() {
    if(isStarted) {
        TaskDialog(NULL, NULL,
                   _T("Tips"),
                   _T("There's already a timing autoclean schedule"),
                   NULL,
                   TDCBF_OK_BUTTON,
                   TD_WARNING_ICON,
                   NULL);
        return;
    }
    TaskDialog(NULL, NULL,
               _T("Tips"),
               _T("Timing autoclean schedule has been started, and it will autoclean per 30 seconds."),
               NULL,
               TDCBF_OK_BUTTON,
               TD_INFORMATION_ICON,
               NULL);
    SetTimer(1, 30 * 1000, (TIMERPROC)TimerProc);
    isStarted = true;
}

void CAboutDlg::OnTimer(UINT_PTR nIDEvent) {
    CDialogEx::OnTimer(nIDEvent);
}

void CRAMCleanerDlg::OnBnClickedButton3() {
    if(!isStarted) {
        TaskDialog(NULL, NULL,
                   _T("Tips"),
                   _T("No autoclean schedule started."),
                   NULL,
                   TDCBF_OK_BUTTON,
                   TD_WARNING_ICON,
                   NULL);
        return;
    }
    TaskDialog(NULL, NULL,
               _T("Tips"),
               _T("Autoclean schedule stopped."),
               NULL,
               TDCBF_OK_BUTTON,
               TD_INFORMATION_ICON,
               NULL);
    KillTimer(1);
    isStarted = false;
}

void CRAMCleanerDlg::OnBnClickedButton4() {
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = this-> m_hWnd;
    nid.uID = IDR_MAINFRAME;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    nid.uCallbackMessage = WM_SHOWTASK;
    nid.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
    wsprintf(nid.szTip, L"Click here to show the mainframe.");
    wsprintf(nid.szInfo, L"Click this tray icon to display the main interface of the program.");
    wsprintf(nid.szInfoTitle, L"The program is running in minimized mode.");
    Shell_NotifyIcon(NIM_ADD, &nid);
    ShowWindow(SW_HIDE);
    minize = true;
}

LRESULT CRAMCleanerDlg::onShowTask(WPARAM wParam, LPARAM lParam) {
    if(wParam != IDR_MAINFRAME) return 1;
    switch(lParam) {
    case WM_RBUTTONUP: {
        LPPOINT lpoint = new tagPOINT;
        ::GetCursorPos(lpoint);
        CMenu   menu;
        menu.CreatePopupMenu();
        menu.AppendMenu(MF_STRING, WM_DESTROY, L"Close");
        menu.TrackPopupMenu(TPM_LEFTALIGN, lpoint->x, lpoint->y, this);
        HMENU   hmenu = menu.Detach();
        menu.DestroyMenu();
        delete   lpoint;
    }
    break;
    case WM_LBUTTONDOWN: {
        this->ShowWindow(SW_SHOW);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        minize = false;
    }
    break;
    }
    return 0;
}

void CRAMCleanerDlg::OnBnClickedCheck1() {
    CButton *pBtn = (CButton *)GetDlgItem(IDC_CHECK1);
    int state = pBtn->GetCheck();
    if (state == 0)
        backNotify = false;
    else if (state == 1)
        backNotify = true;
}
