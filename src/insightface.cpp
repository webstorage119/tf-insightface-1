#include <insightface.hpp>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <tensorflow/c/c_api.h>
#include <opencv2/opencv.hpp>
#include <time.h>

static void dummy_deallocator(void* data, size_t len, void* arg) {

}

int InsightFace::setup() {
  m_iopts = TF_NewImportGraphDefOptions();
  m_sopts = TF_NewSessionOptions();
  m_status = TF_NewStatus();
  m_graph = TF_NewGraph();
  m_sess = this->loadGraph(m_modelf);
  if (m_sess == NULL) {
    std::cerr << "load graph fail" << std::endl;
    return -1;
  }
  m_iops.resize(m_inputs.size());
  m_oops.resize(m_outputs.size());
  m_ivals.resize(m_inputs.size());
  m_ovals.resize(m_outputs.size());
  for (unsigned int i = 0; i < m_inputs.size(); ++i) {
    m_iops[i] = {TF_GraphOperationByName(m_graph, m_inputs[i].c_str()), 0};
    if (m_iops[i].oper == nullptr) {
      std::cerr << "input operation not found: " << m_inputs[i] << std::endl;
      return -1;
    }
  }
  for (unsigned int i = 0; i < m_outputs.size(); ++i) {
    m_oops[i] = {TF_GraphOperationByName(m_graph, m_outputs[i].c_str()), 0};
    if (m_oops[i].oper == nullptr) {
      std::cerr << "output operation not found: " << m_outputs[i] << std::endl;
      return -1;
    }
  }
  //int64_t dim[1] = {1};
  //m_ivals[1] = TF_NewTensor(TF_FLOAT, dim, 1, &m_dropout, sizeof(float), dummy_deallocator, nullptr); 
  return 0;
}


void InsightFace::preprocess(cv::Mat& image, std::vector<float>& landmark, cv::Mat& face) {  
  //float dst[10] = {38.2946, 73.5318, 56.0252, 41.5493, 70.7299,
  //                   51.6963, 51.5014, 71.7366, 92.3655, 92.2041};
  //int radius = 3;
  char imagename[50];
  //cv::Mat croped;
  //imcrop(image, rect, m_margin, croped);
  //cv::resize(croped, croped, cv::Size(m_width, m_height));
  cv::Mat aligned;
  align(image, landmark, aligned);
  //sprintf(imagename, "aligned_%d.jpg", time(NULL));

  //for (int i = 0; i < 5; ++i) {
  //  cv::Point center(dst[i], dst[i+5]);
  //  cv::circle(aligned, center, radius, cv::Scalar(0, 0, 225), -1);
  //}
  //cv::imwrite(imagename, aligned);
  //cv::resize(aligned, croped, cv::Size(m_width, m_height));
  //sprintf(imagename, "frame_%d.jpg", time(NULL));
  //for (int i = 0; i < 5; ++i) {
  //  cv::Point center(landmark[2*i], landmark[2*i+1]);
  //  cv::circle(image, center, radius, cv::Scalar(0, 0, 225), -1); 
  //}
  //cv::imwrite(imagename, image);
  
  aligned.convertTo(face, CV_32FC3);
}

int InsightFace::extract(cv::Mat& image, cv::Rect& rect, std::vector<float>& landmark, std::vector<float>& feat) {
  feat.clear();
  if (image.empty() || image.channels() != 3) {
    std::cerr << "input image not valid" << std::endl;
    return -1;
  }
  // pose filter
  if (!isPoseProper(landmark)) {
    return -1;
  }
  cv::Mat resized;
  preprocess(image, landmark, resized);
  std::cout << "save aligned face ......" << std::endl;
  const int64_t dim[4] = {1, m_height, m_width, 3};
  m_ivals[0] = TF_NewTensor(TF_FLOAT, dim, 4, resized.ptr<float>(), sizeof(float) * m_width * m_height * 3, dummy_deallocator, nullptr);
  TF_SessionRun(m_sess, nullptr, m_iops.data(), m_ivals.data(), m_iops.size(), m_oops.data(), m_ovals.data(), m_oops.size(), nullptr, 0, nullptr, m_status);
  if (TF_GetCode(m_status) != TF_OK) {
    std::cerr << "session run fail" << std::endl;
    return -1;
  }
  feat.resize(TF_Dim(m_ovals[0], 1));
  const float* raw = (const float*)TF_TensorData(m_ovals[0]);
  memcpy((unsigned char*)&feat[0], raw, feat.size()*sizeof(float));
  norm(feat);
  //for (unsigned int i = 0; i < feat.size(); ++i) {
  //  feat[i] = raw[i];
  //}
  for (unsigned int i = 0; i < m_ovals.size(); ++i) {
    TF_DeleteTensor(m_ovals[i]);
    m_ovals[i] = nullptr;
  }
  TF_DeleteTensor(m_ivals[0]);
  m_ivals[0] = nullptr;
  return 0;
}

