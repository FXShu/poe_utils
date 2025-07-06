#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <opencv4/opencv2/opencv.hpp>
#include <iostream>

int main(int argc, char **argv) {
	// load image
	cv::Mat img = cv::imread("image.png");
	if (img.empty()) {
		std::cout << "Failed to load image." << std::endl;
		return -1;
	}

	/* Crop region (adjust based-on red box location) */
	cv::Rect roi(840, 320, 180, 40);
	cv::Mat cropped = img(roi);

	// Optional: Draw rectangle for visualization
	//cv::rectangle(img, roi, cv::Scalar(0, 255, 0), 2);
	cv::imwrite("debug_rect.png", cropped);
	// Convert to grayscale
	cv::Mat gray;
	cv::cvtColor(cropped, gray, cv::COLOR_BGR2GRAY);

	// Threshold for better contrast;
	cv::Mat thresh;
	cv::threshold(gray, thresh, 130, 255, cv::THRESH_BINARY);

	// Initialize Tesseract API
	tesseract::TessBaseAPI tess;
	if (tess.Init("./tessdata", "eng", tesseract::OEM_LSTM_ONLY)) {
		std::cout << "Could not initialize Tesseract" << std::endl;
		return -1;
	}

	tess.SetVariable("tessedit_char_whitelist", "0123456789");
	tess.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	tess.SetImage(thresh.data, thresh.cols, thresh.rows, 1, thresh.step);

	/* Get OCR result */
	std::string result = tess.GetUTF8Text();
	std::cout << "Detected digits :[" << result << "]" << std::endl;

	return 0;  
}
