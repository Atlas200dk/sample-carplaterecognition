/**
 * ============================================================================
 *
 * Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#include "video_analysis_post.h"
#include <memory>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <regex>
#include "hiaiengine/log.h"

using namespace std;
using namespace ascend::presenter;
using namespace std::__cxx11;


// register data type
//HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
//HIAI_REGISTER_DATA_TYPE("VideoImageParaT", VideoImageParaT);
//HIAI_REGISTER_DATA_TYPE("ObjectInfoT", ObjectInfoT);
//HIAI_REGISTER_DATA_TYPE("ObjectImageParaT", ObjectImageParaT);
//HIAI_REGISTER_DATA_TYPE("VideoDetectionImageParaT", VideoDetectionImageParaT);
//HIAI_REGISTER_DATA_TYPE("CarInfoT", CarInfoT);
//HIAI_REGISTER_DATA_TYPE("BatchCarInfoT", BatchCarInfoT);
//HIAI_REGISTER_DATA_TYPE("OutputT", OutputT);
//HIAI_REGISTER_DATA_TYPE("DetectionEngineTransT", DetectionEngineTransT);

VideoAnalysisPost::~VideoAnalysisPost() {
  if (agent_channel_ != nullptr) {
    delete agent_channel_;
  }
}

bool VideoAnalysisPost::IsInvalidIp(const std::string &ip) {
  regex re(kIpRegularExpression);
  smatch sm;
  return !regex_match(ip, sm, re);
}

bool VideoAnalysisPost::IsInvalidConfidence(float confidence) {
  return (confidence <= 0.0) || (confidence > 1.0);
}

bool VideoAnalysisPost::IsInvalidPort(const string &port) {
  if (port == "") {
    return true;
  }

  // check the input port is valid, value range:'0' ~ '9'
  for (auto &c : port) {
    if (c >= '0' && c <= '9') {
      continue;
    } else {
      return true;
    }
  }

  int port_tmp = atoi(port.data());
  return (port_tmp < kPortMinNumber) || (port_tmp > kPortMaxNumber);
}

bool VideoAnalysisPost::IsInvalidAppName(const std::string &app_name) {
  regex re(kAppNameRegularExpression);
  smatch sm;
  return !regex_match(app_name, sm, re);
}

HIAI_StatusT VideoAnalysisPost::Init(
    const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc) {
  if (app_config_ == nullptr) {
    app_config_ = make_shared<RegisterAppParam>();
  }

  // get engine config and save to app_config_
  for (int index = 0; index < config.items_size(); index++) {
    const ::hiai::AIConfigItem& item = config.items(index);
    string name = item.name();
    string value = item.value();

    if (name == kPresenterServerIP) {
      // validate presenter server IP
      if (IsInvalidIp(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "host_ip = %s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      app_config_->host_ip = value;
    } else if (name == kPresenterServerPort) {
      // validate presenter server port
      if (IsInvalidPort(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "port = %s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      app_config_->port = atoi(value.data());
    } else if (name == kAppName) {
      // validate app name
      if (IsInvalidAppName(value)) {
        HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                        "app_name = %s which configured is invalid.",
                        value.c_str());
        return HIAI_ERROR;
      }
      app_config_->app_name = value;
    } else {
      HIAI_ENGINE_LOG("unused config name: %s", name.c_str());
    }
  }


  uint16_t u_port = static_cast<uint16_t>(app_config_->port);
  OpenChannelParam channel_param = { app_config_->host_ip,
      app_config_->port, app_config_->app_name, ContentType::kVideo };
  Channel *chan = nullptr;
  PresenterErrorCode err_code = OpenChannel(chan, channel_param);
  // open channel failed
  if (err_code != PresenterErrorCode::kNone) {
    HIAI_ENGINE_LOG(HIAI_GRAPH_INIT_FAILED,
                    "Open presenter channel failed, error code=%d", err_code);
    return HIAI_ERROR;
  }

  presenter_channel_.reset(chan);
  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO, "End initialize!");
  return HIAI_OK;
}

bool VideoAnalysisPost::IsInvalidResults(float score,
												const Point &point_lt,
												const Point &point_rb) {
	
	// confidence check
	if ((score < 0.9)
		|| IsInvalidConfidence(score)) {
	  return true;
	}
	
	// rectangle position is a point or not: lt == rb
	if ((point_lt.x == point_rb.x) && (point_lt.y == point_rb.y)) {
	  return true;
	}
	return false;
}


int32_t VideoAnalysisPost::SendImage(uint32_t height, uint32_t width,
											uint32_t size, u_int8_t *data, std::vector<DetectionResult>& detection_results) {
	int32_t status = 0;
	// parameter
	ImageFrame image_frame_para;
	image_frame_para.format = ImageFormat::kJpeg;
	image_frame_para.width = width;
	image_frame_para.height = height;
	image_frame_para.size = size;
	image_frame_para.data = data;
	image_frame_para.detection_results = detection_results;
	
	PresenterErrorCode p_ret = PresentImage(presenter_channel_.get(),
											image_frame_para);
	// send to presenter failed
	if (p_ret != PresenterErrorCode::kNone) {
	  HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
						"Send JPEG image to presenter failed, error code=%d",
						p_ret);
	  status = -1;
	}
	
	return status;
}



HIAI_StatusT VideoAnalysisPost::HandleOriginalImage(
	const std::shared_ptr<BatchCarInfoT> &inference_res) {
	HIAI_StatusT status = HIAI_OK;
	uint32_t width = inference_res->image.img.width;
	uint32_t height = inference_res->image.img.height;
	uint32_t size = inference_res->image.img.size;
	
	// call SendImage
	// 1. call DVPP to change YUV420SP image to JPEG
	// 2. send image to presenter
	vector<DetectionResult> detection_results;
	int32_t ret = SendImage(height, width, size, inference_res->image.img.data.get(), detection_results);
	if (ret == -1) {
	  status = HIAI_ERROR;
	//  continue;
	}
	//}
	return status;
}

HIAI_StatusT VideoAnalysisPost::HandleResults(
	const std::shared_ptr<BatchCarInfoT> &inference_res) {
	HIAI_StatusT status = HIAI_OK;
	  uint32_t width = inference_res->image.img.width;
	  uint32_t height = inference_res->image.img.height;
	  uint32_t img_size = inference_res->image.img.size;
	
	  // every inference result needs 8 float
	  // loop the result for every inference result
	  std::vector<DetectionResult> detection_results;
	  //float *ptr = result;
	  int image_number = inference_res->car_infos.size();
	  for (int32_t k = 0; k < image_number; k ++) {

		  // confidence
		  float score = inference_res->car_infos[k].confidence;
	
		  //Detection result
		  DetectionResult one_result;
		  // left top
		  Point point_lt, point_rb;
		  point_lt.x = inference_res->car_infos[k].box.lt_x;
		  point_lt.y = inference_res->car_infos[k].box.lt_y;
		  // right bottom
		  point_rb.x = inference_res->car_infos[k].box.rb_x;
		  point_rb.y = inference_res->car_infos[k].box.rb_y;
	
		  one_result.lt = point_lt;
		  one_result.rb = point_rb;
	    printf("point:%d %d %d %d\n",point_lt.x,point_lt.y,point_rb.x,point_rb.y);
      printf("score=%f\n",score);
      printf("inference_result=%s\n",inference_res->car_infos[k].inference_result.c_str());
      printf("width=%d  height=%d\n",width,height);
		  // check results is valid
		  if (IsInvalidResults(score, point_lt, point_rb)) {
		    continue;
		  }
		  HIAI_ENGINE_LOG(HIAI_DEBUG_INFO,
					    "score=%f, lt.x=%d, lt.y=%d, rb.x=%d, rb.y=%d", score,
						point_lt.x, point_lt.y, point_rb.x, point_rb.y);
		  int32_t score_percent =  score * 100;
		  one_result.result_text.append(inference_res->car_infos[k].inference_result);
		  one_result.result_text.append(":");
		  one_result.result_text.append(to_string(score_percent));
		  one_result.result_text.append("%");
	
		  // push back
		  detection_results.emplace_back(one_result);
	  }
	
	  int32_t ret;
	  ret = SendImage(height, width, img_size, inference_res->image.img.data.get(), detection_results);

	  // check send result
	  if (ret == -1) {
		status = HIAI_ERROR;
	  }
	//}
	return status;
}

HIAI_IMPL_ENGINE_PROCESS("video_analysis_post", VideoAnalysisPost, INPUT_SIZE) {

  if (arg0 == nullptr) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "Failed to process invalid message.");
    return HIAI_ERROR;
  }
  std::shared_ptr<BatchCarInfoT> inference_res = std::static_pointer_cast<
      BatchCarInfoT>(arg0);
  if (inference_res == nullptr) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "Failed to process invalid message, original image is null.");
    return HIAI_ERROR;
  }

  // inference failed, dealing original images
  if (!inference_res->status) {
    HIAI_ENGINE_LOG(HIAI_OK, "will handle original image.");
    printf("[video_analysis_post] inference failed,handle original image\n");
    return HandleOriginalImage(inference_res);
  }
  printf("[video_analysis_post] inference successed,handle Results\n");
  return HandleResults(inference_res);
}
