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
	IDC_LABEL_COLOR_HINT
};

// CToolOptionsBar

IMPLEMENT_DYNAMIC(CToolOptionsBar, CDialogBar)

CToolOptionsBar::CToolOptionsBar()
{
	_tool = TOOL_NONE;

	_eraserSize = 18;
	_eraserSquare = false;
	_colorThreshold = 25;
}

CToolOptionsBar::~CToolOptionsBar()
{
}


BEGIN_MESSAGE_MAP(CToolOptionsBar, CDialogBar)
	ON_EN_CHANGE(IDC_EDIT_ERASER_SIZE, &CToolOptionsBar::OnEnChangeEraserSize)
	ON_BN_CLICKED(IDC_RADIO_ERASER_ROUND, &CToolOptionsBar::OnBnClickedEraserShape)
	ON_BN_CLICKED(IDC_RADIO_ERASER_SQUARE, &CToolOptionsBar::OnBnClickedEraserShape)
	ON_EN_CHANGE(IDC_EDIT_COLOR_THRESHOLD, &CToolOptionsBar::OnEnChangeColorThreshold)
END_MESSAGE_MAP()


BOOL CToolOptionsBar::Create(CWnd* pParentWnd)
{
	if (!CDialogBar::Create(pParentWnd, IDD_TOOL_OPTIONS, CBRS_TOP, AFX_IDW_DIALOGBAR))
		return FALSE;

	// Setting a range makes the spin write its clamped position (1) to the
	// edit, and EN_CHANGE would overwrite the defaults: keep them aside.
	int eraserSize = _eraserSize;
	int colorThreshold = (int)_colorThreshold;

	CSpinButtonCtrl* pSpinEraser = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_ERASER_SIZE);
	CSpinButtonCtrl* pSpinColor = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_COLOR_THRESHOLD);
	pSpinEraser->SetRange32(1, 500);
	pSpinColor->SetRange32(1, 200);
	pSpinEraser->SetPos32(eraserSize);
	pSpinColor->SetPos32(colorThreshold);

	_eraserSize = eraserSize;
	_colorThreshold = colorThreshold;
	CheckRadioButton(IDC_RADIO_ERASER_ROUND, IDC_RADIO_ERASER_SQUARE,
		_eraserSquare ? IDC_RADIO_ERASER_SQUARE : IDC_RADIO_ERASER_ROUND);

	_tool = TOOL_ERASER;	// force ShowTool to update
	ShowTool(TOOL_NONE);

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
	_eraserSize = ReadInt(IDC_EDIT_ERASER_SIZE, 1, 500, _eraserSize);
}

void CToolOptionsBar::OnBnClickedEraserShape()
{
	_eraserSquare = IsDlgButtonChecked(IDC_RADIO_ERASER_SQUARE) != 0;
}

void CToolOptionsBar::OnEnChangeColorThreshold()
{
	_colorThreshold = ReadInt(IDC_EDIT_COLOR_THRESHOLD, 1, 200, (int)_colorThreshold);
}
