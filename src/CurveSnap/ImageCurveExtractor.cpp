#include "StdAfx.h"
#include "ImageCurveExtractor.h"

#include <math.h>
#include <numeric>

ImageCurveExtractor::ImageCurveExtractor(void)
{
  eraserColor = cv::Scalar(255,255,255);
	_choosedShowerBgBlack = false;
	Clear();
}

ImageCurveExtractor::~ImageCurveExtractor(void)
{
}

void ImageCurveExtractor::SetInput(cv::Mat image)
{
  this->imgInput = image.clone();

  imgProcess = imgInput.clone();

	cv::cvtColor(imgInput, imgInputLab, CV_BGR2Lab);

	_matCurve = cv::Mat(imgInput.size(), CV_8U, cv::Scalar(0));

	UpdateShowers();
}

CString ImageCurveExtractor::GetInputSizeString()
{
    CString str;
    str.Format(_T("%dx%d"), imgInput.cols, imgInput.rows);

    return str;
}

bool ImageCurveExtractor::GetInputSize(int& width, int& height)
{
	if (imgInput.empty())
	{
		return false;
	}
	else
	{
		width = imgInput.cols;
		height = imgInput.rows;
		return true;
	}
}

void ImageCurveExtractor::SetEraserColor(COLORREF color)
{
    eraserColor = cv::Scalar(GetBValue(color),
                            GetGValue(color),GetRValue(color));
}

// This function should be optimized.
// Draw the circle in WINDOW directly, and update extractor in the end
void ImageCurveExtractor::Draw(POINT p, double radius, bool square)
{
	// Take care of the ::Invalidate to avoid hight CPU
    if (!IsImageSet())
        return;

    cv::Point v = cvx::round(shower.Show2Original(p));

	int r = cvx::round(radius/shower.GetScale());

	if (square)
	{
		cv::Point d(r, r);
		cv::rectangle(imgProcess, v - d, v + d, eraserColor, -1);
		cv::rectangle(_matCurve, v - d, v + d, cv::Scalar(0), -1);
	}
	else
	{
		cv::circle(imgProcess, v, r, eraserColor, -1);
		cv::circle(_matCurve, v, r, cv::Scalar(0), -1);
	}

	// for efficiency comment following codes, for this function is called in loop

	//if (cvx::IsSame(matCurveOld, _matCurve))
	//{
		imgShow = imgProcess.clone();
		imgShow.setTo(cv::Scalar(0,0,255), _matCurve);
		shower.Set(imgShow);
	//}
	//else
	//{
	//	CheckCurve();
		_extracted = false;
		//if (_choosedShowerBgBlack)
		//{
		//	choosedShower.Set(_matCurve);
		//}
		//else
		//{
		//	cv::Mat mat(_matCurve.size(), CV_8UC3, cv::Scalar::all(255));
		//	mat.setTo(cv::Scalar(0,0,255), _matCurve);
		//	choosedShower.Set(mat);
		//}
	//}

	if (!singlePoints.empty())
	{
		double r = radius;
		if (r<5)
			r=5;
		for (list<cv::Point2d>::iterator it = singlePoints.begin();
				it != singlePoints.end();)
		{
			cv::Point2d d = shower.Show2Original(p) - *it;
			double dist = square ? std::max(fabs(d.x), fabs(d.y)) : cv::norm(d);
			if (dist <= r)
			{
				cv::Point v = cvx::round(*it);
				_matCurve.at<uchar>(v.y,v.x) = 0;

				singlePoints.erase(it++);
			}
			else
				it++;
		}
	}


    erased = true;
}

void ImageCurveExtractor::ResetImage()
{
    if (!IsImageSet())
        return;

    imgProcess = imgInput.clone();
    erased = false;

    imgShow = imgProcess.clone();

    //shower.Set(imgShow);

	UpdateShowers();
}

void ImageCurveExtractor::Clear()
{
    imgInput = cv::Mat();
    imgProcess = cv::Mat();
    imgShow = cv::Mat();
	_matCurve = cv::Mat();
	singlePoints.clear();
	_x.clear();
	_y.clear();

	_curveChoosed = false;
	_extracted = false;

    shower.Clear();

    erased = false;

    running = false;

    this->pcCvter.Clear();
}

cv::Mat ImageCurveExtractor::Threshold(cv::Mat img)
{
	cv::Mat imgBin;
    cv::cvtColor(img, imgBin, CV_BGR2GRAY);

    // draw hist

    //cv::imshow("gray", imgBin);
    //cv::imshow("hist", cvx::imhist(imgBin));

    cv::Scalar mean = cv::mean(imgBin);

    //afxDump << mean[0] << " ";

    if (mean[0] < 128)
    {
        imgBin = ~imgBin;
        mean[0] = 255 - mean[0];
    }

    cv::threshold(imgBin, imgBin, mean[0]-20, 255, CV_THRESH_BINARY_INV);
    //cv::threshold(imgBin, imgBin, 128, 255, CV_THRESH_BINARY_INV);

	return imgBin;
}

// Pixels of bin reachable from seed by steps that move one pixel up or down,
// or one column in direction dx (-1: left, +1: right), possibly also one row.
// Same result as repeating a one-sided 3x3 dilation masked by bin until it
// stops changing, but each pixel is visited once instead of once per step.
static cv::Mat GrowCurve(const cv::Mat& seed, const cv::Mat& bin, int dx)
{
	const int rows = bin.rows, cols = bin.cols;
	cv::Mat result = seed & bin;
	std::vector<int> stack;		// pixels to expand, as y*cols+x

	for (int y = 0; y < rows; y++)
	{
		const uchar* s = seed.ptr<uchar>(y);
		for (int x = 0; x < cols; x++)
			if (s[x])
				stack.push_back(y*cols + x);
	}

	const int steps[5][2] = {{0,-1}, {0,1}, {dx,-1}, {dx,0}, {dx,1}};	// {x,y}
	while (!stack.empty())
	{
		int i = stack.back();
		stack.pop_back();
		int y = i / cols, x = i % cols;

		for (int k = 0; k < 5; k++)
		{
			int qx = x + steps[k][0], qy = y + steps[k][1];
			if (qx < 0 || qx >= cols || qy < 0 || qy >= rows)
				continue;
			uchar& r = result.at<uchar>(qy, qx);
			if (!r && bin.at<uchar>(qy, qx))
			{
				r = 255;
				stack.push_back(qy*cols + qx);
			}
		}
	}

	return result;
}

void ImageCurveExtractor::ChooseConnectCurve()
{
	cv::Point v = cvx::round(shower.Show2Original(choosedPoint));

	if (_matCurve.at<uchar>(v.y,v.x) > 0)
	{
		::MessageBox(shower.GetHwnd(), "Already choosed.", "Try again", MB_OK);
		running = false;
		return;
	}

    cv::Mat imgBin = Threshold(imgProcess);

    //cv::threshold(imgBin, imgBin, 128, 255, CV_THRESH_BINARY);
    //imgBin = ~imgBin;
    //cv::erode(imgBin, imgBin, cv::Mat());
    //cv::morphologyEx(imgBin, imgBin, cv::MORPH_OPEN, cv::Mat());

    //cv::medianBlur(imgBin, imgBin, 3);

    //cv::imshow("threshold", imgBin);
    //cv::waitKey();

	cv::Mat matCurveStart = cvx::FindClosedArea(imgBin, v);


    if (matCurveStart.empty())
    {
		::MessageBox(shower.GetHwnd(), "No curve found under click point.", "Try again", MB_OK);
        running = false;		// for safety
        return;
    }
	else if(cvx::IsContainedBin(_matCurve, matCurveStart))
	{
		::MessageBox(shower.GetHwnd(), "Already choosed.", "Try again", MB_OK);
		running = false;
		return;
	}

	imgBin &= ~_matCurve;	// only choose in the non-chosen part

    // Curve was found: follow it to the left, then to the right

	running = true;
	for (int dx = -1; dx <= 1; dx += 2)
	{
		_matCurve |= GrowCurve(matCurveStart, imgBin, dx);
		CheckCurve();
		UpdateShowers();
		shower.InvalidateRect(NULL, FALSE);
		choosedShower.InvalidateRect(NULL, FALSE);
	}

	_extracted = false;

    running = false;
}

void ImageCurveExtractor::ResetCurve()
{
	if (imgInput.empty())
		throw "Input not set.";

	_matCurve = cv::Mat();
	_matCurve = cv::Mat(imgInput.size(), CV_8U, cv::Scalar(0));
	singlePoints.clear();
	_x.clear();
	_y.clear();

	CheckCurve();
	UpdateShowers();
}

void ImageCurveExtractor::ChooseAll()
{
	cv::Mat mat = Threshold(imgProcess);

	if (cv::countNonZero(mat)==0)
		return;

	_matCurve = mat;

	//cv::imshow("",_matCurve);

    cv::Mat white(_matCurve.size(), _matCurve.type(), cv::Scalar(255));

	cv::Mat mats[3] = { ~_matCurve, ~_matCurve, white};
	cv::merge(mats, 3, imgShow);

	CheckCurve();
	_extracted = false;
	//Extract();
	UpdateShowers();
}

bool ImageCurveExtractor::ChooseRectCurve(RECT rc)
{
	POINT topLeft = {rc.left, rc.top};
	POINT bottomRight = {rc.right, rc.bottom};
	return ChooseRectCurve(topLeft, bottomRight);
}

void SaturatePointBoundary(cv::Point& pt, int rows, int cols)
{
	if (pt.x < 0)
		pt.x = 0;
	if (pt.y < 0)
		pt.y = 0;

	if (pt.x >= cols)
		pt.x = cols - 1;
	if (pt.y >= rows)
		pt.y = rows - 1;
}

bool ImageCurveExtractor::ChooseRectCurve(POINT topLeft, POINT bottomRight)
{
	cv::Point p1 = cvx::round(shower.Show2Original(topLeft));
	cv::Point p2 = cvx::round(shower.Show2Original(bottomRight));

	// check boundary
	SaturatePointBoundary(p1, _matCurve.rows, _matCurve.cols);
	SaturatePointBoundary(p2, _matCurve.rows, _matCurve.cols);

	cv::Rect roi(p1,p2);
	cv::Mat mat = Threshold(imgProcess);

	if (cv::countNonZero(mat(roi))==0)
		return false;

	_matCurve(roi) |= mat(roi);

	CheckCurve();
	_extracted = false;

	UpdateShowers();

	return true;
}

void ImageCurveExtractor::Extract()
{
	_x.clear();
	_y.clear();

	if (!IsCurveChoosed())
		return;

	bool calibrated = this->pcCvter.IsCalibrated();

	double x,y;
	for (int i = 0; i < _matCurve.cols; i++)
	{
		int cNonZero = cv::countNonZero(_matCurve.col(i));
		//afxDump << "col: " << i << ", Nozero: " << cNonZero << " ";

		if (cNonZero ==0 || cNonZero > _matCurve.cols/2)
			continue;	// no point, skip
		

		vector<double> vy;
		for (int j=0; j < _matCurve.rows; j++)
		{
			if (_matCurve.at<uchar>(j,i) == 0)
				continue;

			if (calibrated)
			{
				pcCvter.Pixel2Coord(i, j, x, y);

				vy.push_back(y);
			}
			else	// use pixel
			{
				x = i;
				vy.push_back(_matCurve.rows - j);
			}
		}

		double ym = std::accumulate(vy.begin(), vy.end(), 0.0) / vy.size();

		//afxDump << " (" << x << "," << ym << ") \n";

		_x.push_back(x);
		_y.push_back(ym);
	}
	_extracted = true;

	// test smoothing
	//if (_x.size() >=3)
	//{
	//	vector<double> xs, ys;
	//	for (int i = 1; i < _x.size()-1; i++)
	//	{
	//		xs.push_back(_x[i]);
	//		ys.push_back((_y[i-1]+_y[i]+_y[i+1])/3);
	//	}
	//	_x = xs;
	//	_y = ys;
	//	::MessageBeep(0);
	//}
}

bool ImageCurveExtractor::ChooseColorCurve(POINT p, double threshold)
{
	// 1. color sample area
	cv::Point v = cvx::round(shower.Show2Original(p));
	SaturatePointBoundary(v, _matCurve.rows, _matCurve.cols);

    cv::Mat imgBin = Threshold(imgProcess);
	cv::Mat img;
	cv::cvtColor(imgProcess, img, CV_BGR2Lab);

	cv::Mat matCurveStart = cvx::FindClosedArea(imgBin, v);

    if (matCurveStart.empty())
    {
		AfxMessageBox("No curve found near click point.");
        return false;
    }

    for (int k = 0; k < 5; k++)
    {
		cv::dilate(matCurveStart, matCurveStart, cv::Mat());
        matCurveStart = matCurveStart & imgBin;
	}

	// Get Hist of start area
	//cv::Mat hist;
	//int histSize[] = {255, 255};				// {256, 256, 256} for 3 channels
	//float hranges[] = {0.0, 255};
	//const float* ranges[] = {hranges ,hranges};	// {hranges, hranges, hranges} for 3 channels
	//int channels[] = {1,2};				// {0, 1, 2} for 3 channel
	//cv::calcHist(&img, 1,channels, matCurveStart, hist, 2, histSize, ranges);	// dims equal to channels

	//// back projection
	//cv::Mat back;
	//cv::calcBackProject(&img, 1, channels , hist, back, ranges);

	//back = back>0;
	////cv::imshow("back", back);

	//
	
	// Keep this in 8-bit Lab and compute the color difference per pixel:
	// full-size CV_64FC3 temporaries (24 bytes/pixel each) exhaust the
	// 32-bit address space on large images and crash the program.
	cv::Scalar meanLab = cv::mean(img, matCurveStart);

	cv::Mat devBin(img.size(), CV_8U, cv::Scalar(0));
	for (int i = 0; i < img.rows; i++)
	{
		const cv::Vec3b* lab = img.ptr<cv::Vec3b>(i);
		const uchar* bin = imgBin.ptr<uchar>(i);
		uchar* out = devBin.ptr<uchar>(i);
		for (int j = 0; j < img.cols; j++)
		{
			if (!bin[j])
				continue;

			double dL = lab[j][0] - meanLab[0];
			double da = lab[j][1] - meanLab[1];
			double db = lab[j][2] - meanLab[2];

			// CIE76 color difference (8-bit L is scaled by 2.55). JND:2.3
			if (dL*dL/(2.55*2.55) + da*da + db*db < threshold*threshold)
				out[j] = 255;
		}
	}

	if (cv::countNonZero(devBin) > 0)
	{
		_matCurve |= devBin;
		CheckCurve();
		_extracted = false;
		UpdateShowers();
		return true;
	}
	else
		return false;
}

bool ImageCurveExtractor::ChoosePoint(POINT p)
{
	cv::Point v = cvx::round(shower.Show2Original(p));

	if (_matCurve.at<uchar>(v.y,v.x) > 0)
	{
		::MessageBox(shower.GetHwnd(), "Already choosed.", "Try again", MB_OK);
		return false;
	}

	singlePoints.push_back(shower.Show2Original(p));
	
	_matCurve.at<uchar>(v.y,v.x) = 255;
	CheckCurve();
	_extracted = false;
	UpdateShowers();

	return true;
}

void ImageCurveExtractor::CheckCurve() 
{
	_curveChoosed = cv::countNonZero(_matCurve) >0;
}

void ImageCurveExtractor::UpdateShowers()
{
	imgShow = imgProcess.clone();
	imgShow.setTo(cv::Scalar(0,0,255), _matCurve);
	shower.Set(imgShow);

	if (_choosedShowerBgBlack)
	{
		choosedShower.Set(_matCurve);
	}
	else
	{
		cv::Mat mat(_matCurve.size(), CV_8UC3, cv::Scalar::all(255));
		mat.setTo(cv::Scalar(0,0,255), _matCurve);
		choosedShower.Set(mat);
	}
}
