#pragma once

#include <cstddef>
#include <cstring>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <zmq.hpp>

struct ImageStructure
{
    ImageStructure(cv::Mat& m, size_t id = 0) :m_(m), id(id){};

    size_t id;
    cv::Mat& m_;

    std::string serialize()
    {
        // Фиксированная структура байтов
        cv::Mat fixed;

        // if (m_.channels() == 1)
        // {
            // cv::cvtColor(m_, m_, cv::COLOR_GRAY2BGR);
        // }
        if (m_.type() != CV_8UC3) {
            m_.convertTo(fixed, CV_8UC3);
        } else {
            fixed = m_; // zero-copy
        }

        size_t size = fixed.total() * fixed.elemSize();
        size_t rows = fixed.rows;
        size_t cols = fixed.cols;
        
        std::stringstream ss;

        ss.write((char*)&id, sizeof(id));
        ss.write((char*)&size, sizeof(size));
        ss.write((char*)&rows, sizeof(rows));
        ss.write((char*)&cols, sizeof(cols));
        ss.write((char*)fixed.data, size);

        return ss.str();
    }

    void deserialize(const std::string& image)
    {
        std::stringstream ss(image);
        
        size_t size; 
        size_t rows; 
        size_t cols;
        
        ss.read((char*)&id, sizeof(id));
        ss.read((char*)&size, sizeof(size));
        ss.read((char*)&rows, sizeof(rows));
        ss.read((char*)&cols, sizeof(cols));

        m_ = cv::Mat(cv::Size(cols,rows),CV_8UC3);
        ss.read((char*)m_.data, size);
    }

    void deserialize(zmq::message_t image)
    {
        std::string im_str((char*)image.data(),image.size());
        deserialize(std::move(im_str));
    }
};