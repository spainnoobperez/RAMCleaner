#pragma once

#define WM_SHOWTASK (WM_USER+10)

class CRAMCleanerDlg : public CDialogEx {

public:
    CRAMCleanerDlg(CWnd *pParent = NULL);

    enum { IDD = IDD_RAMCLEANER_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange *pDX);

protected:
    HICON m_hIcon;

    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();
    afx_msg void OnBnClickedButton3();
    afx_msg void OnBnClickedButton4();
    afx_msg  LRESULT  onShowTask(WPARAM   wParam, LPARAM   lParam);
    afx_msg void OnBnClickedCheck1();
};
