#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Load source image and template image in grayscale
    cv::Mat source = cv::imread("source.jpg", cv::IMREAD_GRAYSCALE);
    cv::Mat pattern = cv::imread("template.jpg", cv::IMREAD_GRAYSCALE);

    if (source.empty() || pattern.empty()) {
        std::cout << "Error loading images!" << std::endl;
        return -1;
    }

    // Result matrix size
    int result_cols = source.cols - pattern.cols + 1;
    int result_rows = source.rows - pattern.rows + 1;
    cv::Mat result(result_rows, result_cols, CV_32FC1);

    // Match template
    cv::matchTemplate(source, pattern, result, cv::TM_CCOEFF_NORMED);
    cv::normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

    // Localize the best match
    double minVal, maxVal;
    cv::Point minLoc, maxLoc, matchLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

    // Use max location for TM_CCOEFF_NORMED
    matchLoc = maxLoc;

    std::cout << "Top-left corner of matched region: (" 
              << matchLoc.x << ", " << matchLoc.y << ")" << std::endl;

    // Optional: Draw rectangle on the source image
    cv::rectangle(source, matchLoc, 
                  cv::Point(matchLoc.x + pattern.cols, matchLoc.y + pattern.rows), 
                  cv::Scalar(255, 0, 0), 2);

    // Save or display result (if you have GUI support)
    cv::imwrite("matched_result.jpg", source);

    return 0;
}

