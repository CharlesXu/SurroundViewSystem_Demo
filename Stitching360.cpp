#include "Stitching360.h"
#define front 0
#define back 1
#define left 2
#define right 3

Stitching360::Stitching360():m_sImageRoot(".//CaliImg//"), m_sLastName(".png"), m_sCaliResult("result.txt"), m_szBoard(cv::Size(7, 7)), m_nImageCount(15), m_nSuccessImageNum(0)
{
}

Stitching360::~Stitching360() 
{
    cv::destroyAllWindows();
}

int Stitching360::Init(int nSrcHeight, int nSrcWidth)
{
	bool isExist = std::experimental::filesystem::exists(m_sCaliResult);
	m_szImage = cv::Size(nSrcHeight, nSrcWidth);
	if (!isExist) 
	{
		int count = findCorners();
		cameraCalibrate(count);
		savePara();
	}
	else
	{
		std::ifstream fin(m_sCaliResult);
		float d;
		int i = 0;
		// ���ﻹҪ����һ�¶���ķ�ʽ
		while (fin >> d) {
			if (i <= 8)
				m_mIntrinsicMatrix(i / 3, i % 3) = d;
			else if (i <= 12)
				m_vDistortionCoeffs(i - 9) = d;
			else
				m_mNewIntrinsicMat((i - 13) / 3, (i - 13) % 3) = d;
			i++;
		}
	}
	cv::Mat map1, map2;
	cv::fisheye::initUndistortRectifyMap(m_mIntrinsicMatrix, m_vDistortionCoeffs, cv::Matx33d::eye(), m_mNewIntrinsicMat, m_szImage + cv::Size(200,200), CV_32FC1, map1, map2);
	m_cmMap1.upload(map1);
	m_cmMap2.upload(map2);
    return 1;
}

int Stitching360::findCorners() 
{
	int count = 0;
	for (int i = 0; i != m_nImageCount; i++)
	{
        std::cout << "Frame #" << i + 1 << "..." << std::endl;
        std::string imageFileName;
		std::stringstream StrStm;
		StrStm << i + 1;
		StrStm >> imageFileName;
		imageFileName += m_sLastName;
		cv::Mat image = cv::imread(m_sImageRoot + imageFileName);
		/* ��ȡ�ǵ� */
		cv::Mat imageGray;
		cvtColor(image, imageGray, CV_RGB2GRAY);
		/*����ͼ�񣬽ǵ�������⵽�Ľǵ㣬Ѱ�ҽǵ�ǰ��ͼ�����ĵ���*/
		bool patternfound = cv::findChessboardCorners(image, m_szBoard, n_vCorners, cv::CALIB_CB_ADAPTIVE_THRESH + cv::CALIB_CB_NORMALIZE_IMAGE +
			cv::CALIB_CB_FAST_CHECK);

		if (!patternfound)
		{
            std::cout << "�Ҳ����ǵ㣬��ɾ��ͼƬ�ļ�" << imageFileName << "���������ļ������ٴα궨" << std::endl;
			getchar();
			exit(1);
		}
		else
		{
			/* �����ؾ�ȷ��,�Լ�⵽����������ǵ㾫ȷ������ȷ����ĵ����corners�У� ��С���˵���100�Σ������0.001*/
			cornerSubPix(imageGray, n_vCorners, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 100, 0.001));
            std::cout << "Frame corner#" << i + 1 << "...end" << std::endl;

			count = count + n_vCorners.size();
			m_nSuccessImageNum = m_nSuccessImageNum + 1;
			m_vCornersSeq.push_back(n_vCorners);
		}
		m_vImageSeq.push_back(image);
	}
	return count;
    std::cout << "�ǵ���ȡ��ɣ�\n";
    return 1;
}

int Stitching360::cameraCalibrate(int count) {
    std::cout << "��ʼ���ꡭ����������" << std::endl;
	cv::Size square_size = cv::Size(20, 20); /**** ÿһ��������20m*20mm ****/
    std::vector<std::vector<cv::Point3f>>  object_Points;        /****  ���涨����Ͻǵ����ά����   ****/

	cv::Mat image_points = cv::Mat(1, count, CV_32FC2, cv::Scalar::all(0));  /*****   ������ȡ�����нǵ�   *****/
    std::vector<int>  point_counts;
	/* ��ʼ��������Ͻǵ����ά���� */
	for (int t = 0; t<m_nSuccessImageNum; t++)
	{
        std::vector<cv::Point3f> tempPointSet;
		for (int i = 0; i<m_szBoard.height; i++)
		{
			for (int j = 0; j<m_szBoard.width; j++)
			{
				/* ���趨��������������ϵ��z=0��ƽ���� */
				cv::Point3f tempPoint;
				tempPoint.x = i*square_size.width;
				tempPoint.y = j*square_size.height;
				tempPoint.z = 0;
				tempPointSet.push_back(tempPoint);
			}
		}
		object_Points.push_back(tempPointSet);
	}
	for (int i = 0; i< m_nSuccessImageNum; i++)
	{
		point_counts.push_back(m_szBoard.width*m_szBoard.height);
	}
	/* ��ʼ���� */
	cv::Size image_size = m_vImageSeq[0].size();
	int flags = 0;
	flags |= cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC;
	flags |= cv::fisheye::CALIB_CHECK_COND;
	flags |= cv::fisheye::CALIB_FIX_SKEW;
	/* �ǵ��ڱ궨���ڵ����꣬ �ǵ���ͼ���е����꣬ ͼƬ��С�� �ڲξ��� �ĸ���������� �������ת������ �����ƽ�������� �������� ���*/
	cv::fisheye::calibrate(object_Points, m_vCornersSeq, image_size, m_mIntrinsicMatrix, m_vDistortionCoeffs, m_vRotationVectors, m_vTranslationVectors, flags, cv::TermCriteria(3, 20, 1e-6));
	cv::fisheye::estimateNewCameraMatrixForUndistortRectify(m_mIntrinsicMatrix, m_vDistortionCoeffs, image_size, cv::noArray(), m_mNewIntrinsicMat, 0.8, image_size, 1.0);
    std::cout << "������ɣ�\n";
    return 1;
}

int Stitching360::savePara() {
    std::cout << "��ʼ���涨����������������" << std::endl;
    std::ofstream fout(m_sCaliResult);
	/*����ڲ�������*/
    fout << m_mIntrinsicMatrix(0,0) << ' ' << m_mIntrinsicMatrix(0, 1) << ' ' << m_mIntrinsicMatrix(0, 2) << ' ' << m_mIntrinsicMatrix(1, 0) << ' ' << m_mIntrinsicMatrix(1, 1)
		<< ' ' << m_mIntrinsicMatrix(1, 2) << ' ' << m_mIntrinsicMatrix(2, 0) << ' ' << m_mIntrinsicMatrix(2, 1) << ' ' << m_mIntrinsicMatrix(2, 2) << std::endl;
	/*����ϵ��*/
	fout << m_vDistortionCoeffs(0) << ' '<< m_vDistortionCoeffs(1) <<' '<< m_vDistortionCoeffs(2) << ' ' << m_vDistortionCoeffs(3) << std::endl;
	/*�����ڲξ���*/
	fout << m_mNewIntrinsicMat(0, 0) << ' ' << m_mNewIntrinsicMat(0, 1) << ' ' << m_mNewIntrinsicMat(0, 2) << ' ' << m_mNewIntrinsicMat(1, 0) << ' ' << m_mNewIntrinsicMat(1, 1)
		<< ' ' << m_mNewIntrinsicMat(1, 2) << ' ' << m_mNewIntrinsicMat(2, 0) << ' ' << m_mNewIntrinsicMat(2, 1) << ' ' << m_mNewIntrinsicMat(2, 2) << std::endl;
    std::cout << "��ɱ���" << std::endl;
	fout << std::endl;
    return 1;
}

cv::cuda::GpuMat Stitching360::Undistort(cv::cuda::GpuMat &mSrcImg)
{
	//cv::Mat map1, map2;
	//fisheye::initUndistortRectifyMap();
	
	//cv::remap();
	cv::cuda::GpuMat mDstImg;
	//cv::remap(mSrcImg, mDstImg, map1, map2, INTER_LINEAR, BORDER_CONSTANT);
	cv::cuda::remap(mSrcImg, mDstImg, m_cmMap1, m_cmMap2, cv::INTER_LINEAR, cv::BORDER_CONSTANT);
	return mDstImg;
}

cv::Mat Stitching360::PerspectiveTransform(cv::InputArray aInput, cv::Point2f *pSrcPoints, cv::Point2f *pDstPoints, cv::Size sOutputSize, int nOrientation) 
{
    cv::Mat mPerspective = cv::getPerspectiveTransform(pSrcPoints, pDstPoints);
    cv::Mat mPerspectiveImg;
    cv::warpPerspective(aInput, mPerspectiveImg, mPerspective, sOutputSize, cv::INTER_LINEAR);
    if (nOrientation == front)
    {

    }
    else if (nOrientation == back)
    {
        cv::flip(mPerspectiveImg, mPerspectiveImg, -1);
    }
    else if (nOrientation == left)
    {
        cv::transpose(mPerspectiveImg, mPerspectiveImg);
        cv::flip(mPerspectiveImg, mPerspectiveImg, 0);
    }
    else if (nOrientation == right)
    {
        cv::transpose(mPerspectiveImg, mPerspectiveImg);
        cv::flip(mPerspectiveImg, mPerspectiveImg, 1);
    }

    return mPerspectiveImg;
}

cv::Mat Stitching360::ImageStitching(cv::Mat mInputLeft, cv::Mat mInputRight, cv::Mat mInputFront, cv::Mat mInputBack)
{
    cv::Mat mCombine = cv::Mat::zeros(1600, 1600, mInputRight.type());
    cv::Mat mRoiInputRight = cv::Mat::zeros(mInputRight.size(), CV_8U);
    cv::Mat mRoiInputLeft = cv::Mat::zeros(mInputLeft.size(), CV_8U);
    cv::Mat mRoiInputFront = cv::Mat::zeros(mInputFront.size(), CV_8U);
    cv::Mat mRoiInputBack = cv::Mat::zeros(mInputBack.size(), CV_8U);

    std::vector<std::vector<cv::Point>> vContourInputRight;
    std::vector<cv::Point> vPtsInputRight;
    std::vector<std::vector<cv::Point>> vContourInputLeft;
    std::vector<cv::Point> vPtsInputLeft;
    std::vector<std::vector<cv::Point>> vContourInputFront;
    std::vector<cv::Point> vPtsInputFront;
    std::vector<std::vector<cv::Point>> vContourInputBack;
    std::vector<cv::Point> vPtsInputBack;

    vPtsInputRight.push_back(cv::Point(432, 1080));
    vPtsInputRight.push_back(cv::Point(0, 648));
    vPtsInputRight.push_back(cv::Point(0, 0));
    vPtsInputRight.push_back(cv::Point(500, 0));
    vPtsInputRight.push_back(cv::Point(500, 1080));
    vContourInputRight.push_back(vPtsInputRight);
    drawContours(mRoiInputRight, vContourInputRight, 0, cv::Scalar::all(255), -1);
    cv::Mat mImgRoiInputRight = mCombine(cv::Rect(1000, 100, 500, 1080));

    vPtsInputLeft.push_back(cv::Point(500, 765 + 397 - 500));
    vPtsInputLeft.push_back(cv::Point(0, 765+397));
    vPtsInputLeft.push_back(cv::Point(0, 1080));
    vPtsInputLeft.push_back(cv::Point(0, 0));
    vPtsInputLeft.push_back(cv::Point(500, 0));
    vContourInputLeft.push_back(vPtsInputLeft);
    drawContours(mRoiInputLeft, vContourInputLeft, 0, cv::Scalar::all(255), -1);
    cv::Mat mImgRoiInputLeft = mCombine(cv::Rect(266, 84, 500, 1080));
    
    vPtsInputBack.push_back(cv::Point(560, 0));
    vPtsInputBack.push_back(cv::Point(1060, 500));
    vPtsInputBack.push_back(cv::Point(1080, 500));
    vPtsInputBack.push_back(cv::Point(0, 500));
    vPtsInputBack.push_back(cv::Point(0, 0));
    vContourInputBack.push_back(vPtsInputBack);
    drawContours(mRoiInputBack, vContourInputBack, 0, cv::Scalar::all(255), -1);
    cv::Mat mImgRoiInputBack = mCombine(cv::Rect(440, 747, 1080, 500));

    vPtsInputFront.push_back(cv::Point(423-396, 0));
    vPtsInputFront.push_back(cv::Point(1080, 0));
    vPtsInputFront.push_back(cv::Point(1080, 397 - 1080 + 857 ));
    vPtsInputFront.push_back(cv::Point(857 - 500 + 397, 500));
    vPtsInputFront.push_back(cv::Point(423 + 500 - 396, 500));
    vContourInputFront.push_back(vPtsInputFront);
    drawContours(mRoiInputFront, vContourInputFront, 0, cv::Scalar::all(255), -1);
    cv::Mat mImgRoiInputFront = mCombine(cv::Rect(665-423, 413-396, 1080, 500));
    
    // �������˳���ܱ䡣��Ϊright,back,left��ֻ����һ���ߣ�ֱ�ӵ�����ȥ�ġ�
    // ��left����backǰһ��ִ�У�leftͼ�񽫱�back���ǡ�
    mInputRight.copyTo(mImgRoiInputRight, mRoiInputRight);
    mInputBack.copyTo(mImgRoiInputBack, mRoiInputBack);
    mInputLeft.copyTo(mImgRoiInputLeft, mRoiInputLeft);
    mInputFront.copyTo(mImgRoiInputFront, mRoiInputFront);

    mCombine = mCombine(cv::Rect(443, 175, 900, 910));
    return mCombine;
}