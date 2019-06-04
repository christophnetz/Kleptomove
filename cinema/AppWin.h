#ifndef APPWIN_H_INCLUDED
#define APPWIN_H_INCLUDED

#ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
#endif

#include <memory>
#include <mutex>
#include <future>
#include <atomic>
#include <string>
#include <deque>
#include "stdafx.h"
#include "resource.h"
#include "bridge.h"
#include "GLAnnWin.h"
#include "GLLandscapeWin.h"
#include "GLTimelineWin.h"
#include <cine/simulation.h>
#include <cine/parameter.h>
#include <glsl/context.h>


// WPARAM: msg
// LPARAM: generation | timestep
#define CINEMA_WM_NOTIFY (WM_USER + 1)


#define MAKEPARAM64(l, h) ((std::uint64_t(h) << 32) | std::uint64_t(l))
#define LOINT32(x) int(x & 0xFFFFFFFF)
#define HIINT32(x) int(x >> 32)


namespace cinema {


  class AppWin : 
    public CFrameWindowImpl<AppWin>, 
    public CUpdateUI<AppWin>,
    public CMessageFilter,
    public cine2::SimulationHost,
    public cine2::Observer
  {
  public:
    AppWin();
    virtual ~AppWin();

    // Observer interface
    bool notify(void* userdata, long long msg) override;
    
    // SimulationHost interface
    bool run(Observer* next, const cine2::Param&) override;

    // CFrameWindowImpl stuff
    virtual BOOL PreTranslateMessage(MSG* pMsg);

    DECLARE_FRAME_WND_CLASS(NULL, IDR_CINEMA)

    CMultiPaneStatusBarCtrl StatusBar_;
    CSplitterWindow vSplit_;
    CHorSplitterWindow hSplit_;
    CPaneContainer LandscapePane_;
    CPaneContainer AnnPane_;
    CPaneContainer TimelinePane_;
    std::unique_ptr<GLLandscapeWin> LandscapeWin_;
    std::unique_ptr<GLAnnWin> AnnWin_;
    std::unique_ptr<GLTimeLineWin> TimelineWin_;
    
    BEGIN_UPDATE_UI_MAP(AppWin)
      UPDATE_ELEMENT(ID_VIEW_LANDSCAPE, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_ANN, UPDUI_MENUPOPUP)
      UPDATE_ELEMENT(ID_VIEW_TIMELINE, UPDUI_MENUPOPUP)
      //UPDATE_ELEMENT(ID_FILE_SAVE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR)
      //UPDATE_ELEMENT(ID_FILE_SAVE_AS, UPDUI_MENUPOPUP)
      //UPDATE_ELEMENT(ID_SPEEDDOWN, UPDUI_TOOLBAR)
      //UPDATE_ELEMENT(ID_SPEEDREAL, UPDUI_TOOLBAR)
      //UPDATE_ELEMENT(ID_SPEEDUP, UPDUI_TOOLBAR)
    END_UPDATE_UI_MAP()
  
    BEGIN_MSG_MAP(AppWin)
      MESSAGE_HANDLER(WM_SIZE, OnSize);
      MESSAGE_HANDLER(WM_CREATE, OnCreate);
      MESSAGE_HANDLER(WM_CLOSE, OnClose);
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy);
      MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd);
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown);
      MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp);
      MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClick);
      MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown);
      MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove);
      MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel);
      MESSAGE_HANDLER(CINEMA_WM_NOTIFY, OnNotify);
    
      COMMAND_ID_HANDLER(ID_TOGGLE_PAUSE, OnTogglePause);
      COMMAND_ID_HANDLER(ID_SINGLE_STEP, OnSingleStep);
      COMMAND_ID_HANDLER(ID_SINGLE_GEN, OnSingleGen);

      COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit);
      COMMAND_ID_HANDLER(ID_VIEW_PARAM, OnViewParam);
      COMMAND_RANGE_HANDLER(ID_VIEW_LANDSCAPE, ID_VIEW_ANN, OnViewTopPane);
      COMMAND_ID_HANDLER(ID_VIEW_TIMELINE, OnViewBottomPane);
      COMMAND_ID_HANDLER(ID_PANE_CLOSE, OnPaneClose);

      CHAIN_MSG_MAP(CUpdateUI<AppWin>);
      CHAIN_MSG_MAP(CFrameWindowImpl<AppWin>);
    END_MSG_MAP()

    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnLButtonDown(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnLButtonUp(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnLButtonDblClick(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnRButtonDown(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMouseMove(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnMouseWheel(UINT, WPARAM, LPARAM, BOOL&);

    LRESULT OnTogglePause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSingleStep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSingleGen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnViewParam(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnViewTopPane(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnViewBottomPane(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnPaneClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled);

    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL&);
 
  private:
    void test_breakpoint();
    void emulate_breakpoint();
    void break_at_next_frame();
  
    HACCEL accel_;
    int winW_, winH_;
    bool tracking_;
    int mouseX_, mouseY_;
    std::string titleFmt_;

    mutable std::mutex breakpoint_mutex_;
    mutable decltype(cine2::Param::gui.breakpoints) breakpoints_;
    mutable std::atomic<bool> application_terminated;
    mutable std::atomic<bool> application_finished;
    mutable std::atomic<bool> application_paused;
    mutable std::atomic<bool> application_initialized;

    std::unique_ptr<GLSimState> sim_state_;
  };

}

#endif
