#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <string>
#include <atomic>
#include <thread>
#include <array>
#include <malloc.h>
#include <cine/simulation.h>
#include <glsl/debug.h>
#include "AppWin.h"


#define breakpoint_guard std::lock_guard<std::mutex> ____(breakpoint_mutex_)


namespace cinema { 
  

  namespace {

    std::atomic<bool> ctrl_c_triggered = false;
    std::atomic<DWORD> gui_thread_native_handle = 0;


    BOOL WINAPI CtrlHandlerRoutine(_In_ DWORD dwCtrlType) 
    {
      using namespace std::chrono_literals;
      switch (dwCtrlType)
      {
      case CTRL_C_EVENT:
      case CTRL_CLOSE_EVENT:
        ctrl_c_triggered = true;
        ::PostThreadMessage(gui_thread_native_handle, WM_CLOSE, 0, 0);
        return TRUE;
      default:
        // Pass signal on to the next handler
        return FALSE;
      }
    }


    template <typename T>
    inline T atomic_load(std::atomic<T>& atom)
    {
      return atom.load(std::memory_order_acquire);
    }


    template <typename T>
    inline void atomic_store(std::atomic<T>& atom, T val)
    {
      atom.store(val, std::memory_order_release);
    }

  }


  AppWin::AppWin()
  : SimulationHost(),
    accel_(0), tracking_(false)
  {
    application_terminated = false;
    application_finished = false;
    application_paused = true;
    ::SetConsoleCtrlHandler(CtrlHandlerRoutine, TRUE);
  }


  AppWin::~AppWin()
  {
  }


  // Observer interface
  bool AppWin::notify(void* userdata, long long msg)
  {
    using msg_type = cine2::Simulation::msg_type;
    using namespace std::chrono_literals;

    if (atomic_load(application_terminated)) return false;

    switch (msg) {
    case msg_type::INITIALIZED:
      atomic_store(application_initialized,false);
      ::SendMessage(m_hWnd, CINEMA_WM_NOTIFY, MAKEPARAM64(msg, 0), MAKEPARAM64(simulation()->generation(), simulation()->timestep()));
      while (!atomic_load(application_initialized)) {
        std::this_thread::sleep_for(10ms);
      }
    case msg_type::POST_TIMESTEP:
    case msg_type::NEW_GENERATION:
    case msg_type::GENERATION:
    case msg_type::FINISHED:
      sim_state_->flush_async(*simulation(), msg);
      LandscapeWin_->flush_async(msg);
      AnnWin_->flush_async(msg);
      TimelineWin_->flush_async(msg);
      if (msg != msg_type::INITIALIZED) {
        // avoid second call
        ::PostMessage(m_hWnd, CINEMA_WM_NOTIFY, MAKEPARAM64(msg, 0), MAKEPARAM64(simulation()->generation(), simulation()->timestep()));
      }
      if (msg == msg_type::FINISHED && !simulation()->param().gui.wait_for_close) {
        ::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
        break;
      }
    }
    bool ret = notify_next(userdata, msg);
    test_breakpoint();
    return ret;
  }


  LRESULT AppWin::OnNotify(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
  {
    using msg_type = cine2::Simulation::msg_type;

    int msg = LOINT32(wParam);
    int g = (int)LOINT32(lParam);
    int t = (int)HIINT32(lParam);
    char buf[128];
    switch (msg) {
    case msg_type::INITIALIZED: {
      sim_state_.reset(new GLSimState(m_hWnd, sim_.get()));
      LandscapeWin_.reset(new GLLandscapeWin(sim_state_.get()));
      AnnWin_.reset(new GLAnnWin(sim_state_.get()));
      TimelineWin_.reset(new GLTimeLineWin(sim_state_.get()));
      LandscapeWin_->Create(LandscapePane_);
      AnnWin_->Create(AnnPane_);
      TimelineWin_->Create(TimelinePane_);
      LandscapePane_.SetClient(*LandscapeWin_);
      AnnPane_.SetClient(*AnnWin_);
      TimelinePane_.SetClient(*TimelineWin_);
      breakpoints_ = sim_->param().gui.breakpoints; 
      atomic_store(application_initialized, true);   // signal completion
    }
    case msg_type::NEW_GENERATION:
        snprintf(buf, 128, "Generation %d%s", g, (simulation()->fixed() ? "*" : ""));
        StatusBar_.SetPaneText(ID_PANE_GENERATION, CA2T(buf));
    case msg_type::POST_TIMESTEP:
        snprintf(buf, 128, "Time %d", t);
        StatusBar_.SetPaneText(ID_PANE_TIMESTEP, CA2T(buf));
        break;
    case msg_type::FINISHED:
        if (!simulation()->param().gui.wait_for_close) {
          PostQuitMessage(0);
        }
        StatusBar_.SetPaneText(ID_DEFAULT_PANE, _T("Finished"));
        break;
    }
    return 0;
  }


  bool AppWin::run(Observer* next, const cine2::Param& param)
  {
    bool simres = false;

    // Create window and message loop
    HINSTANCE hInstance = GetModuleHandle(NULL);
    CMessageLoop theLoop;
    AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls
    auto hRes = _Module.Init(NULL, hInstance);
    if (FAILED(hRes)) return false;
    _Module.AddMessageLoop(&theLoop);
    if(CreateEx() == NULL) return false;
    ShowWindow(SW_SHOWDEFAULT);

    // create simulation thread
    chain_back(next);   // chain observer
    auto sim_thread = std::thread([&simres, &param, this](Observer* obs) { 
      simres = SimulationHost::run(obs, param); 
    }, this);

    // run message loop
    theLoop.Run();

    // wait for completion
    if (sim_thread.joinable()) {
      sim_thread.join();
    }
    return simres;
  }


  BOOL AppWin::PreTranslateMessage(MSG* pMsg)
  {
    if(accel_ != NULL)
    {
        if(::TranslateAccelerator(m_hWnd, accel_, pMsg))
            return TRUE;
    }
    if (CFrameWindowImpl<AppWin>::PreTranslateMessage(pMsg)) {
      return TRUE;
    }
    return FALSE;
  }


  LRESULT AppWin::OnCreate(UINT, WPARAM, LPARAM, BOOL&)
  {
    gui_thread_native_handle = ::GetCurrentThreadId();

    const double phi = 1.6180339887;
    ResizeClient(1024, int(1024. / phi));

                     // status bar
    m_hWndStatusBar = StatusBar_.Create(*this);
    int arrPanes[] = { ID_DEFAULT_PANE, ID_PANE_GENERATION, ID_PANE_TIMESTEP, ID_PANE_MS };
    StatusBar_.SetPanes(arrPanes, sizeof(arrPanes) / sizeof(int), false);
    StatusBar_.SetPaneText(ID_DEFAULT_PANE, _T("Running"));

    // Creating splitter and panes
    CRect rc;
    GetClientRect(&rc);
    hSplit_.Create(m_hWnd, rc);

    m_hWndClient = hSplit_;
    TimelinePane_.Create(hSplit_, _T("Timeline"));

    hSplit_.GetClientRect(&rc);
    vSplit_.Create(hSplit_, rc);
    LandscapePane_.Create(vSplit_, _T("Landscape"));
    AnnPane_.Create(vSplit_, _T("Neural network weights"));

    vSplit_.SetSplitterPanes(LandscapePane_, AnnPane_);
    vSplit_.SetSplitterPos(int(rc.Width() / phi), true);
    vSplit_.m_cxyMin = 100;
    hSplit_.SetSplitterPanes(vSplit_, TimelinePane_);
    hSplit_.SetSplitterPos(int(rc.Height() / phi), true);

    // Initial UI settings
    UISetCheck(ID_VIEW_LANDSCAPE, 1);
    UISetCheck(ID_VIEW_ANN, 1);
    UISetCheck(ID_VIEW_TIMELINE, 1);

    accel_ = ::AtlLoadAccelerators(IDR_CINEMA);
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);

    // signal creation done
    atomic_store(application_paused, false);

    hSplit_.SetSinglePaneMode(SPLIT_PANE_TOP);
    UISetCheck(ID_VIEW_TIMELINE, 0);
    return 0;
  }


  LRESULT AppWin::OnClose(UINT, WPARAM, LPARAM, BOOL &bHandled)
  {
    atomic_store(application_finished, true);
    LandscapeWin_->on_close();
    AnnWin_->on_close();
    TimelineWin_->on_close();
    bHandled = FALSE;
    return 1;
  }


  LRESULT AppWin::OnDestroy(UINT, WPARAM, LPARAM, BOOL &bHandled)
  {
    atomic_store(application_terminated, true);
    LandscapeWin_->DestroyWindow();
    AnnWin_->DestroyWindow();
    sim_state_.reset(nullptr);
    bHandled = FALSE;
    return 1;
  }


  LRESULT AppWin::OnEraseBkgnd(UINT, WPARAM, LPARAM, BOOL& bHandled)
  {
    bHandled = TRUE;
    return 1;
  }


  LRESULT AppWin::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
  {
    winW_ = GET_X_LPARAM(lParam);
    winH_ = GET_Y_LPARAM(lParam);
    bHandled = FALSE;
    return 0;
  }


  LRESULT AppWin::OnLButtonDown(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    SetCapture();
    tracking_ = true;
    mouseX_ = GET_X_LPARAM(lParam);
    mouseY_ = GET_Y_LPARAM(lParam);
    return 0;
  }


  LRESULT AppWin::OnLButtonUp(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    ReleaseCapture();
    tracking_ = false;
    return 0;
  }


  LRESULT AppWin::OnLButtonDblClick(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    return 0;
  }


  LRESULT AppWin::OnRButtonDown(UINT, WPARAM, LPARAM lParam, BOOL&)
  {
    return 0;
  }


  LRESULT AppWin::OnMouseMove(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
  {
    if (tracking_)
    {
      auto mouseX = GET_X_LPARAM(lParam);
      auto mouseY = GET_Y_LPARAM(lParam);
      mouseX_ = mouseX;
      mouseY_ = mouseY;
    }
    return 0;
  }


  LRESULT AppWin::OnMouseWheel(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
  {
    int zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
    return 0;
  }


  LRESULT AppWin::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
  {
    PostMessage(WM_CLOSE);
    return 0;
  }


  LRESULT AppWin::OnViewParam(WORD, WORD, HWND hWndCtl, BOOL & bHandled)
  {
    return LRESULT();
  }


  LRESULT AppWin::OnViewTopPane(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    bool hide = (UIGetState(wID) & UPDUI_CHECKED) ? true : false;
    int mode = vSplit_.GetSinglePaneMode();
    if (hide && (mode == SPLIT_PANE_NONE)) {
      // both panes are visible, hide one
      vSplit_.SetSinglePaneMode(wID == ID_VIEW_LANDSCAPE ? SPLIT_PANE_RIGHT : SPLIT_PANE_LEFT);
      UISetCheck(wID, 0);
      // disable alternative
      UIEnable(wID == ID_VIEW_LANDSCAPE ? ID_VIEW_ANN : ID_VIEW_LANDSCAPE, 0);
    }
    else if (!hide) {
      // one pane visible, show both
      vSplit_.SetSinglePaneMode(SPLIT_PANE_NONE);
      UISetCheck(wID, 1);
      // enable both
      UIEnable(ID_VIEW_LANDSCAPE, 1);
      UIEnable(ID_VIEW_ANN, 1);
    }
    return 0;
  }


  LRESULT AppWin::OnViewBottomPane(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    bool hide = (UIGetState(wID) & UPDUI_CHECKED) ? true : false;
    int mode = hSplit_.GetSinglePaneMode();
    if (hide && (mode == SPLIT_PANE_NONE)) {
      // both panes are visible, hide bottom one
      hSplit_.SetSinglePaneMode(SPLIT_PANE_TOP);
      UISetCheck(wID, 0);
    }
    else if (!hide) {
      // one pane visible, show both
      hSplit_.SetSinglePaneMode(SPLIT_PANE_NONE);
      UISetCheck(wID, 1);
    }
    return 0;
  }


  LRESULT AppWin::OnPaneClose(WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& bHandled)
  {
    if (hWndCtl == LandscapePane_ || hWndCtl == AnnPane_) {
      WORD ID = (hWndCtl == LandscapePane_) ? ID_VIEW_LANDSCAPE : ID_VIEW_ANN;
      return OnViewTopPane(0, ID, hWndCtl, bHandled);
    }
    else {
      OnViewBottomPane(0, ID_VIEW_TIMELINE, hWndCtl, bHandled);
    }
    return 0;
  }


  LRESULT AppWin::OnTogglePause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
  {
    if (atomic_load(application_paused)) {
      atomic_store(application_paused, false);
      StatusBar_.SetPaneText(ID_DEFAULT_PANE, _T("Running"));
    }
    else {
      breakpoint_guard;
      break_at_next_frame();
    }
    return 0;
  }


  LRESULT AppWin::OnSingleStep(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled)
  {
    if (atomic_load(application_paused)) {
      breakpoint_guard;
      break_at_next_frame();
      atomic_store(application_paused, false);
    }
    return 0;
  }


  LRESULT AppWin::OnSingleGen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled)
  {
    if (atomic_load(application_paused)) {
      breakpoint_guard;
      if (simulation()->timestep() == simulation()->param().T) {
        breakpoints_.emplace_front(simulation()->generation() + 1, simulation()->param().T);
      }
      else {
        breakpoints_.emplace_front(simulation()->generation(), simulation()->param().T);
      }
      atomic_store(application_paused, false);
    }
    return 0;
  }


  void AppWin::test_breakpoint()
  {
    using namespace std::chrono_literals;
    {
      breakpoint_guard;
      while (!breakpoints_.empty() &&  breakpoints_.front().first < simulation()->generation()) {
        breakpoints_.pop_front();   // remove breakpoint that are in the past
      }
      bool hit = !breakpoints_.empty()
                && breakpoints_.front() == std::pair<int, int>(simulation()->generation(), simulation()->timestep());
    
      if (hit) {
        StatusBar_.SetPaneText(ID_DEFAULT_PANE, _T("Breakpoint hit"));
        breakpoints_.pop_front();
        application_paused = true;
      }
    }
    // emulate breakpoint
    while (!atomic_load(application_finished) && atomic_load(application_paused)) {
      std::this_thread::sleep_for(50ms);
    }
    atomic_store(application_paused, false);
  }


  void AppWin::emulate_breakpoint()
  {
    using namespace std::chrono_literals;
    while (!atomic_load(application_finished) && atomic_load(application_paused)) {
      std::this_thread::sleep_for(50ms);
    }
  }


  void AppWin::break_at_next_frame()
  {
    (simulation()->timestep() == simulation()->param().T) 
      ? breakpoints_.emplace_front(simulation()->generation() + 1, 0)
      : breakpoints_.emplace_front(simulation()->generation(), simulation()->timestep() + 1);
  }

}


#undef breakpoint_guard
