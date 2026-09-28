// ToolOptionsBar.cpp : implementation file
//

#include "stdafx.h"
#include "CurveSnap.h"
#include "ToolOptionsBar.h"

#include "CurveSnapDoc.h"
#include "CurveSnapView.h"


static const UINT eraserCtrls[] = {
	IDC_LABEL_ERASER_SIZE, IDC_EDIT_ERASER_SIZE, IDC_SPIN_ERASER_SIZE,
	IDC_LABEL_ERASER_PX, IDC_LABEL_ERASER_SHAPE,
	IDC_RADIO_ERASER_ROUND, IDC_RADIO_ERASER_SQUARE
};

static const UINT colorCtrls[] = {
	IDC_LABEL_COLOR_THRESHOLD, IDC_EDIT_COLOR_THRESHOLD, IDC_SPIN_COLOR_THRESHOLD,
	IDC_LABEL_COLOR_HINT, IDC_CHECK_COLOR_CONNECTED
};

// CToolOptionsBar

IMPLEMENT_DYNAMIC(CToolOptionsBar, CDialogBar)

CToolOptionsBar::CToolOptionsBar()
{
	_initialized = false;
	_tool = TOOL_NONE;

	_eraserSize = 18;
	_eraserSquare = false;
	_colorThreshold = 25;
	_colorConnected = true;
}

CToolOptionsBar::~CToolOptionsBar()
{
}


BEGIN_MESSAGE_MAP(CToolOptionsBar, CDialogBar)
	ON_EN_CHANGE(IDC_EDIT_ERASER_SIZE, &CToolOptionsBar::OnEnChangeEraserSize)
	ON_BN_CLICKED(IDC_RADIO_ERASER_ROUND, &CToolOptionsBar::OnBnClickedEraserShape)
	ON_BN_CLICKED(IDC_RADIO_ERASER_SQUARE, &CToolOptionsBar::OnBnClickedEraserShape)
	ON_EN_CHANGE(IDC_EDIT_COLOR_THRESHOLD, &CToolOptionsBar::OnEnChangeColorThreshold)
	ON_BN_CLICKED(IDC_CHECK_COLOR_CONNECTED, &CToolOptionsBar::OnBnClickedColorConnected)
END_MESSAGE_MAP()


BOOL CToolOptionsBar::Create(CWnd* pParentWnd)
{
	if (!CDialogBar::Create(pParentWnd, IDD_TOOL_OPTIONS, CBRS_TOP, AFX_IDW_DIALOGBAR))
		return FALSE;

	// The spins write their position (0, then 1 after SetRange32) to the edits
	// while the bar is created and set up. _initialized keeps the resulting
	// EN_CHANGEs from overwriting the defaults. The spins (UDS_SETBUDDYINT)
	// take their position from the edit text.
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_ERASER_SIZE))->SetRange32(1, 500);
	((CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_COLOR_THRESHOLD))->SetRange32(1, 200);
	SetDlgItemInt(IDC_EDIT_ERASER_SIZE, _eraserSize);
	SetDlgItemInt(IDC_EDIT_COLOR_THRESHOLD, (int)_colorThreshold);
	CheckRadioButton(IDC_RADIO_ERASER_ROUND, IDC_RADIO_ERASER_SQUARE,
		_eraserSquare ? IDC_RADIO_ERASER_SQUARE : IDC_RADIO_ERASER_ROUND);
	CheckDlgButton(IDC_CHECK_COLOR_CONNECTED, _colorConnected ? BST_CHECKED : BST_UNCHECKED);

	_tool = TOOL_ERASER;	// force ShowTool to update
	ShowTool(TOOL_NONE);

	_initialized = true;
	return TRUE;
}

// Called on idle. Follow the tool selected in the image view.
// CDialogBar's default implementation would disable the radio buttons,
// since no command handlers exist for them in the frame.
void CToolOptionsBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL /*bDisableIfNoHndler*/)
{
	Tool tool = TOOL_NONE;

	CCurveSnapDoc* pDoc = (CCurveSnapDoc*)pTarget->GetActiveDocument();
	if (pDoc)
	{
		CCurveSnapView* pView = (CCurveSnapView*)pDoc->GetView(RUNTIME_CLASS(CCurveSnapView));
		if (pView)
		{
			switch (pView->GetOperation())
			{
			case CCurveSnapView::ERASING:
				tool = TOOL_ERASER;
				break;
			case CCurveSnapView::CHOOSING_COLOR:
				tool = TOOL_CHOOSE_COLOR;
				break;
			}
		}
	}

	ShowTool(tool);
}

void CToolOptionsBar::ShowTool(Tool tool)
{
	if (tool == _tool)
		return;
	_tool = tool;

	for (size_t i = 0; i < _countof(eraserCtrls); i++)
		GetDlgItem(eraserCtrls[i])->ShowWindow(tool == TOOL_ERASER ? SW_SHOW : SW_HIDE);
	for (size_t i = 0; i < _countof(colorCtrls); i++)
		GetDlgItem(colorCtrls[i])->ShowWindow(tool == TOOL_CHOOSE_COLOR ? SW_SHOW : SW_HIDE);

	switch (tool)
	{
	case TOOL_ERASER:
		SetDlgItemText(IDC_LABEL_TOOL_NAME, _T("Eraser:"));
		break;
	case TOOL_CHOOSE_COLOR:
		SetDlgItemText(IDC_LABEL_TOOL_NAME, _T("Choose by color:"));
		break;
	default:
		SetDlgItemText(IDC_LABEL_TOOL_NAME, _T("No options for the current tool."));
	}
}

// Returns the edit's value clamped to [minVal, maxVal], or current if it is empty/invalid
int CToolOptionsBar::ReadInt(int idEdit, int minVal, int maxVal, int current)
{
	BOOL ok = FALSE;
	int val = (int)GetDlgItemInt(idEdit, &ok, FALSE);
	if (!ok)
		return current;

	if (val < minVal)
		val = minVal;
	if (val > maxVal)
		val = maxVal;
	return val;
}

// CToolOptionsBar message handlers

void CToolOptionsBar::OnEnChangeEraserSize()
{
	if (!_initialized)
		return;
	_eraserSize = ReadInt(IDC_EDIT_ERASER_SIZE, 1, 500, _eraserSize);
}

void CToolOptionsBar::OnBnClickedEraserShape()
{
	_eraserSquare = IsDlgButtonChecked(IDC_RADIO_ERASER_SQUARE) != 0;
}

void CToolOptionsBar::OnEnChangeColorThreshold()
{
	if (!_initialized)
		return;
	_colorThreshold = ReadInt(IDC_EDIT_COLOR_THRESHOLD, 1, 200, (int)_colorThreshold);
}

void CToolOptionsBar::OnBnClickedColorConnected()
{
	_colorConnected = IsDlgButtonChecked(IDC_CHECK_COLOR_CONNECTED) != 0;
}
