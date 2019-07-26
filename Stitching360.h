#pragma once
#include <opencv2\opencv.hpp>
#include <experimental/filesystem>
#include <fstream>


class Stitching360 {
private:
	std::string                             m_sImageRoot;	/* ͼƬ�ļ��� */
    std::string                             m_sLastName;    /* ͼƬ��׺�� */
    std::string                             m_sCaliResult; /* ��궨���ݵ��ļ���*/
    cv::Size                                m_szImage;
    cv::Size                                m_szBoard;	/****    �������ÿ�С��еĽǵ���       ****/
	int                                     m_nImageCount;	/****    �궨ͼ������     ****/
	int                                     m_nSuccessImageNum;                /****   �ɹ���ȡ�ǵ������ͼ����    ****/
	cv::Matx33d                             m_mIntrinsicMatrix;    /*****    ������ڲ�������    ****/
	cv::Matx33d                             m_mNewIntrinsicMat;   /** ����ͷ�µ��ڲ����ڽ��� **/
	cv::Vec4d                               m_vDistortionCoeffs;     /* �������4������ϵ����k1,k2,k3,k4*/
	std::vector<cv::Mat>                    m_vImageSeq;					/* ����ͼ�� */
    std::vector<std::vector<cv::Point2f>>   m_vCornersSeq;    /****  �����⵽�����нǵ�       ****/
    std::vector<cv::Point2f>                n_vCorners;                  /****    ����ÿ��ͼ���ϼ�⵽�Ľǵ�       ****/
	std::vector<cv::Vec3d>                  m_vRotationVectors;                           /* ÿ��ͼ�����ת���� */
	std::vector<cv::Vec3d>                  m_vTranslationVectors;                        /* ÿ��ͼ���ƽ������ */
    cv::cuda::GpuMat                        m_cmMap1; /* ���ս�����ӳ��� */
    cv::cuda::GpuMat                        m_cmMap2; /* ���ս�����ӳ��� */

	int findCorners();
	int cameraCalibrate(int count);
	int savePara();
	

public:
	Stitching360();
    ~Stitching360();
    /************************����궨�Լ�����****************************/
	int Init(int nSrcHeight, int nSrcWidth);
	cv::cuda::GpuMat Undistort(cv::cuda::GpuMat &mSrcImg);

    /************************��ͶӰ�任*******************************/
    cv::Mat PerspectiveTransform(cv::InputArray aInput, cv::Point2f *pSrcPoints, cv::Point2f *pDstPoints, cv::Size sOutputSize, int nOrientation);

    /*************************ͼ��ƴ��**********************************/
    cv::Mat ImageStitching(cv::Mat aInputLeft, cv::Mat aInputRight, cv::Mat aInputFront, cv::Mat aInputBack);

};