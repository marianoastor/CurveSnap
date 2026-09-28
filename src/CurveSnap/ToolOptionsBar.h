#pragma once


// CToolOptionsBar
// Bar under the main toolbar that shows the options of the current tool.

class CToolOptionsBar : public CDialogBar
{
	DECLARE_DYNAMIC(CToolOptionsBar)

public:
	enum Tool
	{
		TOOL_NONE,
		TOOL_ERASER,
		TOOL_CHOOSE_COLOR,
	};

public:
	CToolOptionsBar();
	virtual ~CToolOptionsBar();

	BOOL Create(CWnd* pParentWnd);

public:
	int GetEraserSize() { return _eraserSize; }	// diameter in screen pixels
	bool IsEraserSquare() { return _eraserSquare; }
	double GetColorThreshold() { return _colorThreshold; }
	bool IsColorConnectedOnly() { return _colorConnected; }

protected:
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

private:
	void ShowTool(Tool tool);
	int ReadInt(int idEdit, int minVal, int maxVal, int current);

private:
	bool _initialized;	// ignore control notifications until Create is done
	Tool _tool;

	int _eraserSize;
	bool _eraserSquare;
	double _colorThreshold;
	bool _colorConnected;

protected:
	afx_msg void OnEnChangeEraserSize();
	afx_msg void OnBnClickedEraserShape();
	afx_msg void OnEnChangeColorThreshold();
	afx_msg void OnBnClickedColorConnected();
	DECLARE_MESSAGE_MAP()
};
