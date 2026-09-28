// CurveSnapView.cpp : implementation of the CCurveSnapView class
//

#include "stdafx.h"
#include "CurveSnap.h"

#include "CurveSnapDoc.h"
#include "CurveSnapView.h"
#include "ChoosedView.h"
#include "CurveDlg.h"
#include "PlotView.h"
#include "MainFrm.h"

#include "DialogCoord.h"
#include "ScopedPhysicalDpi.h"

#define WM_CHOOSE_CONNECT_DONE WM_USER + 1
#define WM_EXTRACT_DONE WM_USER + 2

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

UINT ExtractThreadFunc(LPVOID lpParam);


// CCurveSnapView

IMPLEMENT_DYNCREATE(CCurveSnapView, CView)

BEGIN_MESSAGE_MAP(CCurveSnapView, CView)
  ON_MESSAGE(WM_CHOOSE_CONNECT_DONE, OnChooseConnectCurveDone)
  ON_MESSAGE(WM_EXTRACT_DONE, OnExtractDone)
  // Standard printing commands
  ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
  ON_WM_DROPFILES()
  //    ON_WM_SETCURSOR()
  ON_COMMAND(ID_CALIBRATE_SETPOINT1, &CCurveSnapView::OnCalibrateSetpoint1)
  ON_COMMAND(ID_CALIBRATE_SETPOINT2, &CCurveSnapView::OnCalibrateSetpoint2)
  ON_UPDATE_COMMAND_UI(ID_CALIBRATE_SETPOINT1, &CCurveSnapView::OnUpdateCalibrateSetpoint1)
  ON_UPDATE_COMMAND_UI(ID_CALIBRATE_SETPOINT2, &CCurveSnapView::OnUpdateCalibrateSetpoint2)
  ON_WM_LBUTTONUP()
  //ON_WM_LBUTTONDOWN()
  ON_WM_MOUSEMOVE()
  ON_COMMAND(ID_EDIT_ERASER, &CCurveSnapView::OnEditEraser)
  ON_UPDATE_COMMAND_UI(ID_EDIT_ERASER, &CCurveSnapView::OnUpdateEditEraser)
  ON_WM_LBUTTONDOWN()
  ON_COMMAND(ID_CURVE_CHOOSECURVE, &CCurveSnapView::OnCurveChooseConnectCurve)
  ON_UPDATE_COMMAND_UI(ID_CURVE_CHOOSECURVE, &CCurveSnapView::OnUpdateCurveChooseConnectCurve)
  ON_COMMAND(ID_EDIT_PEN, &CCurveSnapView::OnEditPen)
  ON_UPDATE_COMMAND_UI(ID_EDIT_PEN, &CCurveSnapView::OnUpdateEditPen)
  ON_COMMAND(ID_CURVE_CHOOSERECTANGLE, &CCurveSnapView::OnCurveChooserectangle)
  ON_COMMAND(ID_CURVE_CHOOSECOLOR, &CCurveSnapView::OnCurveChoosecolor)
  ON_UPDATE_COMMAND_UI(ID_CURVE_CHOOSECOLOR, &CCurveSnapView::OnUpdateCurveChoosecolor)
  ON_UPDATE_COMMAND_UI(ID_CURVE_CHOOSERECTANGLE, &CCurveSnapView::OnUpdateCurveChooserectangle)
  ON_WM_SETCURSOR()
  ON_COMMAND(ID_CURVE_CHOOSEPOINTS, &CCurveSnapView::OnCurveChoosepoints)
  ON_UPDATE_COMMAND_UI(ID_CURVE_CHOOSEPOINTS, &CCurveSnapView::OnUpdateCurveChoosepoints)
END_MESSAGE_MAP()

// CCurveSnapView construction/destruction

CCurveSnapView::CCurveSnapView()
{
  operation = NONE;

  _tempPt[0] = CPoint(-1, -1);
  _tempPt[1] = CPoint(-1, -1);

  crossColors[0] = RGB(255,0,0);
  crossColors[1] = RGB(0,255,0);

  nPointCrossRadius = 7;

  _pChooseConnectCurveThread = NULL;
  _pExtractThread = NULL;

  _cursorEraserShape = NULL;
  _eraserCursorHalf = 0;
  _eraserCursorSquare = false;
}

CCurveSnapView::~CCurveSnapView()
{
  if (_cursorEraserShape)
    DestroyCursor(_cursorEraserShape);
}

BOOL CCurveSnapView::PreCreateWindow(CREATESTRUCT& cs)
{
  // TODO: Modify the Window class or styles here by modifying
  //  the CREATESTRUCT cs


  return CView::PreCreateWindow(cs);
}

// CCurveSnapView drawing

void CCurveSnapView::OnDraw(CDC* /*pDC*/)
{
  CCurveSnapDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (!pDoc)
    return;

  if (!pDoc->extractor.shower.IsEmpty())
  {
    // note that the shower will get DC from the WND, so don't use pdc above
    // using more than one dc from the same wnd is not stable
    // it may cause the latter DrawCross bugging.
    pDoc->extractor.shower.Show(this->m_hWnd);

    CDC* pdc = this->GetDC();
    //pdc->SetBkMode(TRANSPARENT);
    //RECT rc;
    //this->GetClientRect(&rc);

    //pdc->TextOut(rc.left, rc.bottom - 15, pDoc->extractor.GetInputSizeString());

    if (!pDoc->extractor.singlePoints.empty())
      for (list<cv::Point2d>::iterator it = pDoc->extractor.singlePoints.begin();
        it != pDoc->extractor.singlePoints.end(); ++it)
        DrawCross(pdc, pDoc->extractor.shower.Original2Show(*it), 3, RGB(255,0,0));

    for (int k=0; k<2; k++)
    {
      if (pDoc->extractor.pcCvter.IsPixelCoordsSet(k))
      {
        double x,y;
        pDoc->extractor.pcCvter.GetPixel(x, y, k);

        this->DrawCross(pdc, pDoc->extractor.shower.Original2Show(cv::Point2d(x,y)),
          nPointCrossRadius, crossColors[k]);

        pdc->SetTextColor(crossColors[k]);
        pdc->SetBkMode(TRANSPARENT);
        CString str = pDoc->extractor.pcCvter.GetCoordString(k);
        pdc->TextOut(0, k*20, str, str.GetLength());
      }

      if (_tempPt[k].x >=0)
      {
        this->DrawCross(pdc, _tempPt[k],
          nPointCrossRadius, crossColors[k]);
      }
    }

    this->ReleaseDC(pdc);
  }

}


// CCurveSnapView printing

BOOL CCurveSnapView::OnPreparePrinting(CPrintInfo* pInfo)
{
  // default preparation
  return DoPreparePrinting(pInfo);
}

void CCurveSnapView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add extra initialization before printing
}

void CCurveSnapView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
  // TODO: add cleanup after printing
}


// CCurveSnapView diagnostics

#ifdef _DEBUG
void CCurveSnapView::AssertValid() const
{
  CView::AssertValid();
}

void CCurveSnapView::Dump(CDumpContext& dc) const
{
  CView::Dump(dc);
}

CCurveSnapDoc* CCurveSnapView::GetDocument() const // non-debug version is inline
{
  ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCurveSnapDoc)));
  return (CCurveSnapDoc*)m_pDocument;
}
#endif //_DEBUG


// CCurveSnapView message handlers

void CCurveSnapView::OnDropFiles(HDROP hDropInfo)
{
  // TODO: Add your message handler code here and/or call default

  int nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);

  if (nFiles > 0)
    nFiles = 1;

  for (int i = 0; i < nFiles; i++)
  {
    CString s;
    LPTSTR pFilename = s.GetBufferSetLength(MAX_PATH);
    ::DragQueryFile(hDropInfo, i, pFilename, MAX_PATH);
    s.ReleaseBuffer();

    //GetDocument()->OnOpenDocument(s);		// won't create document

    if (AfxGetApp()->m_pDocManager->GetOpenDocumentCount() > 0)
    {
      POSITION pos = AfxGetApp()->m_pDocManager->GetFirstDocTemplatePosition();

      CDocTemplate* pTemplate = AfxGetApp()->m_pDocManager->GetNextDocTemplate(pos);

      if (pTemplate)
        pTemplate->OpenDocumentFile(s);

      //m_pDocManager->OpenDocumentFile(newName);	// won't reopen
    }

    Invalidate();
  }
  ::DragFinish(hDropInfo);


  CView::OnDropFiles(hDropInfo);
}

void CCurveSnapView::OnInitialUpdate()
{
  CView::OnInitialUpdate();
  _cursorCross = LoadCursor(NULL, IDC_CROSS);
  _cursorWait = LoadCursor(NULL, IDC_WAIT);
  _cursorEraser = AfxGetApp()->LoadCursor(IDC_CURSOR_ERASER);
  _cursorPen = AfxGetApp()->LoadCursor(IDC_CURSOR_PEN);
  _cursorChoose = AfxGetApp()->LoadCursor(IDC_CURSOR_CHOOSE); 

  this->DragAcceptFiles();
}


void CCurveSnapView::OnCalibrateSetpoint1()
{
  operation = (operation == SETPOINT1)? NONE : SETPOINT1;
}

void CCurveSnapView::OnCalibrateSetpoint2()
{
  operation = (operation == SETPOINT2)? NONE : SETPOINT2;
}

void CCurveSnapView::OnUpdateCalibrateSetpoint1(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(operation == SETPOINT1);
}

void CCurveSnapView::OnUpdateCalibrateSetpoint2(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(operation == SETPOINT2);
}

LONG CCurveSnapView::OnChooseConnectCurveDone(WPARAM wParam, LPARAM)
{
  _pChooseConnectCurveThread = NULL;
  GetDocument()->UpdateAllViews(this);

  if (!GetDocument()->extractor.IsExtracted())
    _pExtractThread = AfxBeginThread(ExtractThreadFunc, this);

  return 0;
}

LONG CCurveSnapView::OnExtractDone(WPARAM wParam, LPARAM)
{
  _pExtractThread = NULL;

  CPlotView* pView = (CPlotView*)GetDocument()->GetView(RUNTIME_CLASS(CPlotView));

  if (pView)
    pView->OnUpdate(NULL, 0,0);

  return 1;
}

UINT ChooseCurveThreadFunc(LPVOID lpParam)
{
  CCurveSnapView* pView = (CCurveSnapView*) lpParam;

  CCurveSnapDoc* pDoc = pView->GetDocument();
  //ASSERT_VALID(pDoc);
  if (!pDoc)
    return 0;

  pDoc->extractor.ChooseConnectCurve();

  //CView* pChoosedView = pDoc->GetView(RUNTIME_CLASS(CChoosedView));

  //if (pChoosedView && pChoosedView->m_hWnd)
  //	::InvalidateRect(pChoosedView->m_hWnd, NULL, false);

  ::SendMessage(pView->GetSafeHwnd(), WM_CHOOSE_CONNECT_DONE, 0, 0);

  return 0;
}

UINT ExtractThreadFunc(LPVOID lpParam)
{
  CCurveSnapView* pView = (CCurveSnapView*) lpParam;

  CCurveSnapDoc* pDoc = pView->GetDocument();
  //ASSERT_VALID(pDoc);
  if (!pDoc)
    return 0;

  pDoc->extractor.Extract();
  ::SendMessage(pView->GetSafeHwnd(), WM_EXTRACT_DONE, 0, 0);

  return 0;
}


void CCurveSnapView::OnLButtonUp(UINT nFlags, CPoint point)
{

  // TODO: Add your message handler code here and/or call default
  CCurveSnapDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (!pDoc)
    return;

  switch (operation)
  {
  case SETPOINT1:
  case SETPOINT2:
    break;
  case ERASING:
  case PEN:
    pDoc->extractor.CheckCurve();
    pDoc->extractor.UpdateShowers();
    pDoc->UpdateAllViews(this);
    if (!pDoc->extractor.IsExtracted())
      _pExtractThread = AfxBeginThread(ExtractThreadFunc, this);
    break;
  case CHOOSING_CURVE:
    break;
  }

  CView::OnLButtonUp(nFlags, point);
}

void CCurveSnapView::OnUpdate(CView* pSender, LPARAM /*lHint*/, CObject* /*pHint*/)
{
  if (pSender == this)
    return;

  this->Invalidate();

  CMainFrame *pFrmWnd = (CMainFrame*)AfxGetMainWnd();
  pFrmWnd->m_wndStatusBar.SetPaneText(2, GetDocument()->extractor.GetInputSizeString());
}

void CCurveSnapView::DrawCross(CDC* pDC, CPoint pt, int r, COLORREF color)
{
  CPen pen(PS_SOLID, 1, color);//1是笔的宽度, RGB是宏, 用于生成颜色
  CPen* oldPen = pDC->SelectObject(&pen);

  pDC->MoveTo(pt.x-r, pt.y);
  pDC->LineTo(pt.x+r+1, pt.y);

  pDC->MoveTo(pt.x, pt.y-r);
  pDC->LineTo(pt.x, pt.y+r+1);

  pDC->SelectObject(oldPen);
}

void CCurveSnapView::OnMouseMove(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default

  CCurveSnapDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (!pDoc)
    return;

  if (pDoc->extractor.pcCvter.IsCalibrated())
  {
    cv::Vec2d v = pDoc->extractor.shower.Show2Original(point);

    CMainFrame *pFrmWnd = (CMainFrame*)AfxGetMainWnd();
    pFrmWnd->m_wndStatusBar.SetPaneText(1, pDoc->extractor.pcCvter.Pixel2CoordString(v[0], v[1]));
  }

  if (operation == ERASING || operation == PEN)
  {
    if( nFlags & MK_LBUTTON )
    {
      DrawAt(point);
      this->Invalidate(false);
    }
  }

  CView::OnMouseMove(nFlags, point);
}

void CCurveSnapView::DrawAt(CPoint point)
{
  CCurveSnapDoc* pDoc = GetDocument();

  if (operation == ERASING)
  {
    CMainFrame *pFrmWnd = (CMainFrame*)AfxGetMainWnd();
    pDoc->extractor.Draw(point, pFrmWnd->m_wndToolOptions.GetEraserSize()/2.0,
                         pFrmWnd->m_wndToolOptions.IsEraserSquare());
  }
  else
    pDoc->extractor.Draw(point, operation-100);
}

static bool InsideShape(int dx, int dy, double half, bool square)
{
  if (square)
    return abs(dx) <= half && abs(dy) <= half;
  else
    return dx*dx + dy*dy <= half*half;
}

// Monochrome cursor with the 1-pixel outline of a circle/square of the given
// half size (screen pixels), inverting the screen so it shows on any background.
static HCURSOR CreateShapeCursor(double half, bool square)
{
  int c = (int)ceil(half) + 1;
  int n = 2*c + 1;
  int stride = (n + 15) / 16 * 2;		// rows are WORD aligned

  // top half: AND mask (1 = keep screen), bottom half: XOR mask (1 = invert)
  std::vector<BYTE> bits(stride * n * 2, 0);
  memset(&bits[0], 0xFF, stride * n);
  BYTE* xorMask = &bits[stride * n];

  for (int y = 0; y < n; y++)
    for (int x = 0; x < n; x++)
    {
      int dx = x - c, dy = y - c;
      bool edge = InsideShape(dx, dy, half, square) &&
        (!InsideShape(dx-1, dy, half, square) || !InsideShape(dx+1, dy, half, square) ||
         !InsideShape(dx, dy-1, half, square) || !InsideShape(dx, dy+1, half, square));
      bool center = half >= 4 && dx == 0 && dy == 0;
      if (edge || center)
        xorMask[y*stride + x/8] |= 0x80 >> (x%8);
    }

  HBITMAP hMask = CreateBitmap(n, 2*n, 1, 1, &bits[0]);
  if (!hMask)
    return NULL;

  ICONINFO ii = { FALSE, (DWORD)c, (DWORD)c, hMask, NULL };
  HCURSOR hCursor = (HCURSOR)CreateIconIndirect(&ii);
  DeleteObject(hMask);
  return hCursor;
}

// The process is DPI-unaware: Windows stretches our windows at >100% scaling,
// but not a cursor built at runtime. Returns physical pixels per window pixel.
static double GetDpiStretch(HWND hwnd)
{
  CRect logical, physical;
  ::GetWindowRect(hwnd, &logical);
  {
    ScopedPhysicalDpi physical_dpi;
    ::GetWindowRect(hwnd, &physical);
  }

  if (logical.Width() <= 0 || physical.Width() <= 0)
    return 1.0;
  return double(physical.Width()) / logical.Width();
}

void CCurveSnapView::SetEraserCursor()
{
  CMainFrame *pFrmWnd = (CMainFrame*)AfxGetMainWnd();
  double radius = pFrmWnd->m_wndToolOptions.GetEraserSize()/2.0;
  bool square = pFrmWnd->m_wndToolOptions.IsEraserSquare();

  // ImageCurveExtractor::Draw erases r image pixels around the clicked pixel,
  // so the erased area reaches (r+0.5) image pixels from its center.
  cvx::HwndShower& shower = GetDocument()->extractor.shower;
  double scale = shower.IsEmpty() ? 1.0 : shower.GetScale();
  if (scale <= 0)
    scale = 1.0;
  double half = (cvx::round(radius/scale) + 0.5) * scale;
  half *= GetDpiStretch(m_hWnd);

  if (!_cursorEraserShape || half != _eraserCursorHalf || square != _eraserCursorSquare)
  {
    HCURSOR hNew = CreateShapeCursor(half, square);
    if (!hNew)
    {
      ::SetCursor(_cursorEraser);
      return;
    }

    ::SetCursor(hNew);	// before destroying the old one, which may be in use
    if (_cursorEraserShape)
      DestroyCursor(_cursorEraserShape);

    _cursorEraserShape = hNew;
    _eraserCursorHalf = half;
    _eraserCursorSquare = square;
  }
  else
    ::SetCursor(_cursorEraserShape);
}

void CCurveSnapView::OnEditEraser()
{
  operation = (operation == ERASING)? NONE : ERASING;
}

void CCurveSnapView::OnUpdateEditEraser(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(operation == ERASING);
}

void CCurveSnapView::OnEditPen()
{
  operation = (operation == PEN)? NONE : PEN;
}

void CCurveSnapView::OnUpdateEditPen(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(operation == PEN);
}

void CCurveSnapView::OnLButtonDown(UINT nFlags, CPoint point)
{
  // TODO: Add your message handler code here and/or call default
  CCurveSnapDoc* pDoc = GetDocument();
  ASSERT_VALID(pDoc);
  if (!pDoc)
    return;

  if (!pDoc->extractor.shower.IsPointInShowArea(point))
    return;

  CPoint pt(point);
  CSize r(nPointCrossRadius, nPointCrossRadius);

  CRectTracker* pTracker;
  RECT rect;

  CDialogCoord* pdlg;
  int k = 0;

  cv::Point2d ptOriginal;
  switch (operation)
  {
  case SETPOINT1:
  case SETPOINT2:
    k = operation;
    _tempPt[k] = point;
    // show the temp point out before open the dialog for coordinates
    InvalidateRect(CRect(pt - r, pt + r));

    ptOriginal = pDoc->extractor.shower.Show2Original(point);
    pdlg = new CDialogCoord();
    pdlg->px_ = cvRound(ptOriginal.x);
    pdlg->py_ = cvRound(ptOriginal.y);

    pdlg->px_ = std::min(pdlg->px_, pDoc->extractor.imgProcess.cols-1);
    pdlg->px_ = std::max(pdlg->px_, 0);
    pdlg->py_ = std::min(pdlg->py_, pDoc->extractor.imgProcess.rows-1);
    pdlg->py_ = std::max(pdlg->py_, 0);

    pdlg->SetImage(pDoc->extractor.imgProcess);
    if (pdlg->DoModal() != IDCANCEL)
    {
      pDoc->extractor.pcCvter.SetPixel(pdlg->px_, pdlg->py_, k);
      pDoc->extractor.pcCvter.SetCoord(pdlg->x, pdlg->y, k);
      InvalidateRect(CRect(0,0, 200, 50));

      CString str;
      str.LoadString(ID_INDICATOR_CALIBRATE);
      if (!pDoc->extractor.pcCvter.IsCalibrated())
      {
        CMainFrame *pFrmWnd = (CMainFrame*)AfxGetMainWnd();
        pFrmWnd->m_wndStatusBar.SetPaneText(1, str);

      }
      else // calibrated
        _pExtractThread = AfxBeginThread(ExtractThreadFunc, this);

      if (pDoc->extractor.pcCvter.IsBothPixelCoordsSet())
        if (!pDoc->extractor.pcCvter.IsPointsProper())
          MessageBox(_T("Two points are too close vertically or horizontally."), _T("Points Pathological"));

      delete pdlg;
    }

    _tempPt[k] = CPoint(-1, -1);	// clear temp cross
    InvalidateRect(CRect(pt - r, pt + r));

    operation = NONE;
    break;
  case ERASING:
  case PEN:
    DrawAt(point);
    this->Invalidate(false);
    pDoc->UpdateAllViews(this);
    //if (!pDoc->extractor.IsExtracted())
    //	_pExtractThread = AfxBeginThread(ExtractThreadFunc, this);
    break;
  case CHOOSING_CURVE:
    if (!pDoc->extractor.IsRunning())
    {
      pDoc->extractor.choosedPoint = point;
      _pChooseConnectCurveThread = AfxBeginThread(ChooseCurveThreadFunc, this);
    }
    break;
  case CHOOSING_RECT:
    pTracker = new CRectTracker();
    pTracker->TrackRubberBand(this, point, true);
    pTracker->GetTrueRect(&rect);
    delete pTracker;

    if (pDoc->extractor.ChooseRectCurve(rect))
    {
      InvalidateRect(&rect, FALSE);
      pDoc->UpdateAllViews(this);

      if (!pDoc->extractor.IsExtracted())
        _pExtractThread = AfxBeginThread(ExtractThreadFunc, this);
    }
    break;
  case CHOOSING_COLOR:
    if (pDoc->extractor.ChooseColorCurve(point,
          ((CMainFrame*)AfxGetMainWnd())->m_wndToolOptions.GetColorThreshold()))
    {
      Invalidate(FALSE);
      pDoc->UpdateAllViews(this);
      if (!pDoc->extractor.IsExtracted())
        _pExtractThread = AfxBeginThread(ExtractThreadFunc, this);
    }
    break;
  case CHOOSING_POINTS:
    if (pDoc->extractor.ChoosePoint(point))
    {
      Invalidate(FALSE);
      pDoc->UpdateAllViews(this);
      if (!pDoc->extractor.IsExtracted())
        _pExtractThread = AfxBeginThread(ExtractThreadFunc, this);
    }
    break;
  }

  CView::OnLButtonDown(nFlags, point);
}

void CCurveSnapView::OnCurveChooseConnectCurve()
{
  operation = (operation == CHOOSING_CURVE)? NONE : CHOOSING_CURVE;
}

void CCurveSnapView::OnUpdateCurveChooseConnectCurve(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(operation == CHOOSING_CURVE);
}

void CCurveSnapView::OnCurveChooserectangle()
{
  operation = (operation == CHOOSING_RECT)? NONE : CHOOSING_RECT;
}

void CCurveSnapView::OnUpdateCurveChooserectangle(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(operation == CHOOSING_RECT);
}

void CCurveSnapView::OnCurveChoosecolor()
{
  operation = (operation == CHOOSING_COLOR)? NONE : CHOOSING_COLOR;
}

void CCurveSnapView::OnUpdateCurveChoosecolor(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(operation == CHOOSING_COLOR);
}

void CCurveSnapView::OnCurveChoosepoints()
{
  operation = (operation == CHOOSING_POINTS)? NONE : CHOOSING_POINTS;
}

void CCurveSnapView::OnUpdateCurveChoosepoints(CCmdUI *pCmdUI)
{
  pCmdUI->SetCheck(operation == CHOOSING_POINTS);
}

BOOL CCurveSnapView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
  // TODO: Add your message handler code here and/or call default
  switch (operation)
  {
  case SETPOINT1:
  case SETPOINT2:
  case CHOOSING_RECT:
    ::SetCursor(_cursorCross);   
    break;
  case ERASING:
    SetEraserCursor();
    break;
  case PEN:
    ::SetCursor(_cursorPen);
    break;
  case CHOOSING_CURVE:
    if (_pChooseConnectCurveThread)
      ::SetCursor(_cursorWait);
    else
      ::SetCursor(_cursorChoose);
    break;
  case CHOOSING_COLOR:
    ::SetCursor(_cursorChoose);
    break;
  case CHOOSING_POINTS:
    ::SetCursor(_cursorCross);
    break;
  case NONE:
  default:
    return CView::OnSetCursor(pWnd, nHitTest, message);
  }

  return TRUE;
}
