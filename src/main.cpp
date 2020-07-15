#include <string.h>
#include <fstream>
#include <vector>
#include <ctime>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/features2d.hpp>

#include "magsac_utils.h"
#include "utils.h"
#include "magsac.h"

#include "uniform_sampler.h"
#include "flann_neighborhood_graph.h"
#include "fundamental_estimator.h"
#include "homography_estimator.h"
#include "types.h"
#include "model.h"
#include "estimators.h"

enum SceneType { FundamentalMatrixScene, HomographyScene, EssentialMatrixScene };
enum Dataset { kusvod2, extremeview, homogr, adelaidermf, multih, strecha };

// A method applying MAGSAC for fundamental matrix estimation to one of the built-in scenes
void testFundamentalMatrixFitting(
	double ransac_confidence_,
	double maximum_threshold_,
	std::string test_scene_,
	bool draw_results_ = false,
	double drawing_threshold_ = 2);

// A method applying MAGSAC for essential matrix estimation to one of the built-in scenes
void testEssentialMatrixFitting(
	double ransac_confidence_,
	double maximum_threshold_,
	const std::string &test_scene_,
	bool draw_results_ = false,
	double drawing_threshold_ = 2);

// A method applying MAGSAC for homography estimation to one of the built-in scenes
void testHomographyFitting(
	double ransac_confidence_,
	double maximum_threshold_,
	std::string test_scene_,
	bool draw_results_ = false,
	double drawing_threshold_ = 2);

// A method applying OpenCV for homography estimation to one of the built-in scenes
void opencvHomographyFitting(
	double ransac_confidence_,
	double threshold_,
	std::string test_scene_,
	bool draw_results_ = false,
	const bool with_magsac_post_processing_ = true);

// A method applying OpenCV for essential matrix estimation to one of the built-in scenes
void opencvEssentialMatrixFitting(
	double ransac_confidence_,
	double threshold_,
	const std::string &test_scene_,
	bool draw_results_ = false);

// A method applying OpenCV for fundamental matrix estimation to one of the built-in scenes
void opencvFundamentalMatrixFitting(
	double ransac_confidence_,
	double threshold_,
	std::string test_scene_,
	bool draw_results_ = false,
	const bool with_magsac_post_processing_ = true);

// The names of built-in scenes
std::vector<std::string> getAvailableTestScenes(
	const SceneType scene_type_,
	const Dataset dataset_);

// Running tests on the selected dataset
void runTest(SceneType scene_type_,
	Dataset dataset_,
	const double ransac_confidence_,
	const bool draw_results_,
	const double drawing_threshold_);

// Returns the name of the selected dataset
std::string dataset2str(Dataset dataset_);

int main(int argc, const char* argv[])
{
	/*
		This is an example showing how MAGSAC or MAGSAC++ is applied to homography or fundamental matrix estimation tasks.
		This implementation is not the one used in the experiments of the paper.
		If you use this method, please cite:
		(1) Barath, Daniel, Jana Noskova, and Jiri Matas. "MAGSAC: marginalizing sample consensus.", Proceedings of the IEEE Conference on Computer Vision and Pattern Recognition. 2019.
		(2) Barath, Daniel, Jana Noskova, Maksym Ivashechkin, and Jiri Matas. "MAGSAC++, a fast, reliable and accurate robust estimator", Arxiv preprint:1912.05909. 2019.
	*/
	const double ransac_confidence = 0.99; // The required confidence in the results
	const bool draw_results = true; // A flag to draw and show the results 
	// The inlier threshold for visualization. This threshold is not used by the algorithm,
	// it is simply for selecting the inliers to be drawn after MAGSAC finished.
	const double drawing_threshold_essential_matrix = 3.00;
	const double drawing_threshold_fundamental_matrix = 1.00;
	const double drawing_threshold_homography = 1.00;

	// Run homography estimation on the EVD dataset
	runTest(SceneType::HomographyScene, Dataset::extremeview, ransac_confidence, draw_results, drawing_threshold_homography);

	// Run homography estimation on the homogr dataset
	runTest(SceneType::HomographyScene, Dataset::homogr, ransac_confidence, draw_results, drawing_threshold_homography);

	// Run fundamental matrix estimation on the kusvod2 dataset
	runTest(SceneType::FundamentalMatrixScene, Dataset::kusvod2, ransac_confidence, draw_results, drawing_threshold_fundamental_matrix);

	// Run fundamental matrix estimation on the AdelaideRMF dataset
	runTest(SceneType::FundamentalMatrixScene, Dataset::adelaidermf, ransac_confidence, draw_results, drawing_threshold_fundamental_matrix);

	// Run fundamental matrix estimation on the Multi-H dataset
	runTest(SceneType::FundamentalMatrixScene, Dataset::multih, ransac_confidence, draw_results, drawing_threshold_fundamental_matrix);

	// Run essential matrix estimation on a scene from the strecha dataset
	runTest(SceneType::EssentialMatrixScene, Dataset::strecha, ransac_confidence, draw_results, drawing_threshold_essential_matrix);

	return 0;
}

void runTest(SceneType scene_type_, // The type of the fitting problem
	Dataset dataset_, // The dataset currently used for the evaluation
	const double ransac_confidence_, // The confidence required in the results
	const bool draw_results_, // A flag determining if the results should be drawn
	const double drawing_threshold_) // The threshold used for selecting the inliers when they are drawn
{
	// Store the name of the current problem to be solved
	const std::string dataset_name = dataset2str(dataset_);
	std::string problem_name = "Homography";
	if (scene_type_ == SceneType::FundamentalMatrixScene)
		problem_name = "Fundamental matrix";
	else if (scene_type_ == SceneType::EssentialMatrixScene)
		problem_name = "Essential matrix";

	// Test scenes for homography estimation
	for (const auto& scene : getAvailableTestScenes(scene_type_, dataset_))
	{
		// Close all opened windows
		cv::destroyAllWindows();

		printf("--------------------------------------------------------------\n");
		printf("%s estimation on scene \"%s\" from dataset \"%s\".\n",
			problem_name.c_str(), scene.c_str(), dataset_name.c_str());
		printf("--------------------------------------------------------------\n");

		// Run this if the task is homography estimation
		if (scene_type_ == SceneType::HomographyScene)
		{
			// Apply the homography estimation method built into OpenCV
			printf("1. Running OpenCV's RANSAC with threshold %f px\n", drawing_threshold_);
			opencvHomographyFitting(ransac_confidence_,
				drawing_threshold_, // The maximum sigma value allowed in MAGSAC
				scene, // The scene type
				false, // A flag to draw and show the results
				false); // A flag to apply the MAGSAC post-processing to the OpenCV's output

			// Apply MAGSAC with maximum threshold set to a fairly high value
			printf("\n2. Running MAGSAC with fairly high maximum threshold (%f px)\n", 50.0);
			testHomographyFitting(ransac_confidence_,
				50.0, // The maximum sigma value allowed in MAGSAC
				scene, // The scene type
				false, // A flag to draw and show the results  
				2.5); // The inlier threshold for visualization.
		}
		else if (scene_type_ == SceneType::FundamentalMatrixScene)
		{
			// Apply the homography estimation method built into OpenCV
			printf("1. Running OpenCV's RANSAC with threshold %f px\n", drawing_threshold_);
			opencvFundamentalMatrixFitting(ransac_confidence_,
				drawing_threshold_, // The maximum sigma value allowed in MAGSAC
				scene, // The scene type
				false, // A flag to draw and show the results
				false); // A flag to apply the MAGSAC post-processing to the OpenCV's output

			// Apply MAGSAC with fairly high maximum threshold
			printf("\n2. Running MAGSAC with fairly high maximum threshold (%f px)\n", 5.0);
			testFundamentalMatrixFitting(ransac_confidence_, // The required confidence in the results
				5.0, // The maximum sigma value allowed in MAGSAC
				scene, // The scene type
				draw_results_, // A flag to draw and show the results 
				drawing_threshold_); // The inlier threshold for visualization.
		// Run this part of the code if the problem is essential matrix fitting
		}
		else if (scene_type_ == SceneType::EssentialMatrixScene)
		{
			// Apply the homography estimation method built into OpenCV
			printf("1. Running OpenCV's RANSAC with threshold %f px\n", drawing_threshold_);
			opencvEssentialMatrixFitting(ransac_confidence_,
				drawing_threshold_, // The maximum sigma value allowed in MAGSAC
				scene, // The scene type
				false); // A flag to draw and show the results

			// Apply MAGSAC with a reasonably set maximum threshold
			printf("\n2. Running MAGSAC with reasonably set maximum threshold (%f px)\n", 5.0);
			testEssentialMatrixFitting(ransac_confidence_, // The required confidence in the results
				5.0, // The maximum sigma value allowed in MAGSAC
				scene, // The scene type
				false, // A flag to draw and show the results 
				drawing_threshold_); // The inlier threshold for visualization.
		}

		printf("\nPress a button to continue.\n\n");
		cv::waitKey(0);
	}
}

std::string dataset2str(Dataset dataset_)
{
	switch (dataset_)
	{
	case Dataset::strecha:
		return "strecha";
	case Dataset::homogr:
		return "homogr";
	case Dataset::extremeview:
		return "extremeview";
	case Dataset::kusvod2:
		return "kusvod2";
	case Dataset::adelaidermf:
		return "adelaidermf";
	case Dataset::multih:
		return "multih";
	default:
		return "unknown";
	}
}

std::vector<std::string> getAvailableTestScenes(
	const SceneType scene_type_,
	const Dataset dataset_)
{
	switch (scene_type_)
	{
	case SceneType::EssentialMatrixScene: // Available test scenes for homography estimation
		switch (dataset_)
		{
		case Dataset::strecha:
			return { "fountain" };
		}

	case SceneType::HomographyScene: // Available test scenes for homography estimation
		switch (dataset_)
		{
		case Dataset::homogr:
			return { "LePoint1", "LePoint2", "LePoint3", // "homogr" dataset
				"graf", "ExtremeZoom", "city",
				"CapitalRegion", "BruggeTower", "BruggeSquare",
				"BostonLib", "boat", "adam",
				"WhiteBoard", "Eiffel", "Brussels",
				"Boston" };
		case Dataset::extremeview:
			return { "extremeview/adam", "extremeview/cafe", "extremeview/cat", // "EVD" (i.e. extremeview) dataset
				"extremeview/dum", "extremeview/face", "extremeview/fox",
				"extremeview/girl", "extremeview/graf", "extremeview/grand",
				"extremeview/index", "extremeview/mag", "extremeview/pkk",
				"extremeview/shop", "extremeview/there", "extremeview/vin" };

		default:
			return std::vector<std::string>();
		}

	case SceneType::FundamentalMatrixScene:
		switch (dataset_)
		{
		case Dataset::kusvod2:
			return { "corr", "booksh", "box",
				"castle", "graff", "head",
				"kampa", "leafs", "plant",
				"rotunda", "shout", "valbonne",
				"wall", "wash", "zoom",
				"Kyoto" };
		case Dataset::adelaidermf:
			return { "barrsmith", "bonhall", "bonython",
				"elderhalla", "elderhallb",
				"hartley", "johnssonb", "ladysymon",
				"library", "napiera", "napierb",
				"nese", "oldclassicswing", "physics",
				"sene", "unihouse", "unionhouse" };
		case Dataset::multih:
			return { "boxesandbooks", "glasscaseb", "stairs" };
		default:
			return std::vector<std::string>();
		}
	default:
		return std::vector<std::string>();
	}
}

// A method applying MAGSAC for essential matrix estimation to one of the built-in scenes
void testEssentialMatrixFitting(
	double ransac_confidence_,
	double maximum_threshold_,
	const std::string &test_scene_,
	bool draw_results_,
	double drawing_threshold_)
{
	printf("\tProcessed scene = '%s'.\n", test_scene_.c_str());

	// Load the images of the current test scene
	cv::Mat image1 = cv::imread("data/essential_matrix/" + test_scene_ + "1.png");
	cv::Mat image2 = cv::imread("data/essential_matrix/" + test_scene_ + "2.png");
	if (image1.cols == 0)
	{
		image1 = cv::imread("data/essential_matrix/" + test_scene_ + "1.jpg");
		image2 = cv::imread("data/essential_matrix/" + test_scene_ + "2.jpg");
	}

	if (image1.cols == 0)
	{
		fprintf(stderr, "A problem occured when loading the images for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	cv::Mat points; // The point correspondences, each is of format x1 y1 x2 y2
	Eigen::Matrix3d intrinsics_source, // The intrinsic parameters of the source camera
		intrinsics_destination; // The intrinsic parameters of the destination camera

	// A function loading the points from files
	readPoints<4>("data/essential_matrix/" + test_scene_ + "_pts.txt",
		points);

	// Loading the intrinsic camera matrices
	static const std::string source_intrinsics_path = "data/essential_matrix/" + test_scene_ + "1.K";
	if (!gcransac::utils::loadMatrix<double, 3, 3>(source_intrinsics_path,
		intrinsics_source))
	{
		printf("An error occured when loading the intrinsics camera matrix from '%s'\n",
			source_intrinsics_path.c_str());
		return;
	}

	static const std::string destination_intrinsics_path = "data/essential_matrix/" + test_scene_ + "2.K";
	if (!gcransac::utils::loadMatrix<double, 3, 3>(destination_intrinsics_path,
		intrinsics_destination))
	{
		printf("An error occured when loading the intrinsics camera matrix from '%s'\n",
			destination_intrinsics_path.c_str());
		return;
	}

	// Normalize the point coordinates by the intrinsic matrices
	cv::Mat normalized_points(points.size(), CV_64F);
	gcransac::utils::normalizeCorrespondences(points,
		intrinsics_source,
		intrinsics_destination,
		normalized_points);

	// Normalize the threshold by the average of the focal lengths
	const double normalizing_multiplier = 1.0 / ((intrinsics_source(0, 0) + intrinsics_source(1, 1) +
		intrinsics_destination(0, 0) + intrinsics_destination(1, 1)) / 4.0);
	const double normalized_maximum_threshold =
		maximum_threshold_ * normalizing_multiplier;
	const double normalized_drawing_threshold =
		drawing_threshold_ * normalizing_multiplier;

	// The number of points in the datasets
	const size_t N = points.rows; // The number of points in the scene

	if (N == 0) // If there are no points, return
	{
		fprintf(stderr, "A problem occured when loading the annotated points for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	// The robust homography estimator class containing the function for the fitting and residual calculation
	magsac::utils::DefaultEssentialMatrixEstimator estimator(
		intrinsics_source,
		intrinsics_destination,
		0.0);
	gcransac::EssentialMatrix model; // The estimated model

	printf("\tEstimated model = '%s'.\n", "essential matrix");
	printf("\tNumber of correspondences loaded = %d.\n", static_cast<int>(N));

	// Initialize the sampler used for selecting minimal samples
	gcransac::sampler::UniformSampler main_sampler(&normalized_points);

	MAGSAC<cv::Mat, magsac::utils::DefaultEssentialMatrixEstimator> magsac;
	magsac.setMaximumThreshold(normalized_maximum_threshold); // The maximum noise scale sigma allowed
	magsac.setReferenceThreshold(magsac.getReferenceThreshold() * normalizing_multiplier); // The reference threshold inside MAGSAC++ should also be normalized.
	magsac.setIterationLimit(1e4); // Iteration limit to interrupt the cases when the algorithm run too long.

	int iteration_number = 0; // Number of iterations required
	ModelScore score; // The model score

	std::chrono::time_point<std::chrono::system_clock> end,
		start = std::chrono::system_clock::now();
	magsac.run(normalized_points, // The data points
		ransac_confidence_, // The required confidence in the results
		estimator, // The used estimator
		main_sampler, // The sampler used for selecting minimal samples in each iteration
		model, // The estimated model
		iteration_number, // The number of iterations
		score); // The score of the estimated model
	end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);

	printf("\tActual number of iterations drawn by MAGSAC at %.2f confidence: %d\n", ransac_confidence_, iteration_number);
	printf("\tElapsed time: %f secs\n", elapsed_seconds.count());


	std::vector<bool> inliers_mask;
	double inlier_thd = maximum_threshold_;
	magsac.getModelInliersMask(points, model, estimator, inlier_thd, inliers_mask);
	uint inliers_cnt = std::count(inliers_mask.begin(), inliers_mask.end(), true);
	printf("\tNumber of inliers for threshold %2.1f: %d\n", inlier_thd, inliers_cnt);

	// Visualization part.
	// Inliers are selected using threshold and the estimated model. 
	// This part is not necessary and is only for visualization purposes. 
	std::vector<int> obtained_labeling(points.rows, 0);
	size_t inlier_number = 0;

	for (auto pt_idx = 0; pt_idx < points.rows; ++pt_idx)
	{
		// Computing the residual of the point given the estimated model
		auto residual = estimator.residual(normalized_points.row(pt_idx),
			model.descriptor);

		// Change the label to 'inlier' if the residual is smaller than the threshold
		if (normalized_drawing_threshold >= residual)
		{
			obtained_labeling[pt_idx] = 1;
			++inlier_number;
		}
	}

	printf("\tNumber of points closer than %f is %d\n",
		drawing_threshold_, inlier_number);

	if (draw_results_)
	{
		// Draw the matches to the images
		cv::Mat out_image;
		drawMatches<double, int>(points, obtained_labeling, image1, image2, out_image);

		// Show the matches
		std::string window_name = "Visualization with threshold = " + std::to_string(drawing_threshold_) + " px; Maximum threshold is = " + std::to_string(maximum_threshold_);
		showImage(out_image,
			window_name,
			1600,
			900);
		out_image.release();
	}

	// Clean up the memory occupied by the images
	image1.release();
	image2.release();
}

void testFundamentalMatrixFitting(
	double ransac_confidence_,
	double maximum_threshold_,
	std::string test_scene_,
	bool draw_results_,
	double drawing_threshold_)
{
	printf("\tProcessed scene = '%s'.\n", test_scene_.c_str());

	// Load the images of the current test scene
	cv::Mat image1 = cv::imread("data/fundamental_matrix/" + test_scene_ + "A.png");
	cv::Mat image2 = cv::imread("data/fundamental_matrix/" + test_scene_ + "B.png");
	if (image1.cols == 0)
	{
		image1 = cv::imread("data/fundamental_matrix/" + test_scene_ + "A.jpg");
		image2 = cv::imread("data/fundamental_matrix/" + test_scene_ + "B.jpg");
	}

	if (image1.cols == 0)
	{
		fprintf(stderr, "A problem occured when loading the images for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	cv::Mat points; // The point correspondences, each is of format x1 y1 x2 y2
	std::vector<int> ground_truth_labels; // The ground truth labeling provided in the dataset

	// A function loading the points from files
	readAnnotatedPoints("data/fundamental_matrix/" + test_scene_ + "_pts.txt",
		points,
		ground_truth_labels);

	// The number of points in the datasets
	const size_t N = points.rows; // The number of points in the scene

	if (N == 0) // If there are no points, return
	{
		fprintf(stderr, "A problem occured when loading the annotated points for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	magsac::utils::DefaultFundamentalMatrixEstimator estimator(maximum_threshold_); // The robust homography estimator class containing the function for the fitting and residual calculation
	gcransac::FundamentalMatrix model; // The estimated model

	// In this used datasets, the manually selected inliers are not all inliers but a subset of them.
	// Therefore, the manually selected inliers are augmented as follows: 
	// (i) First, the implied model is estimated from the manually selected inliers.
	// (ii) Second, the inliers of the ground truth model are selected.
	std::vector<int> refined_labels = ground_truth_labels;
	refineManualLabeling<gcransac::FundamentalMatrix, magsac::utils::DefaultFundamentalMatrixEstimator>(
		points,
		refined_labels,
		estimator,
		0.35); // Threshold value from the LO*-RANSAC paper

	// Select the inliers from the labeling
	std::vector<int> ground_truth_inliers = getSubsetFromLabeling(ground_truth_labels, 1),
		refined_inliers = getSubsetFromLabeling(refined_labels, 1);
	if (refined_inliers.size() > ground_truth_inliers.size())
		refined_inliers.swap(ground_truth_inliers);
	const size_t inlier_number = static_cast<double>(ground_truth_inliers.size());

	printf("\tEstimated model = '%s'.\n", "fundamental matrix");
	printf("\tNumber of correspondences loaded = %d.\n", static_cast<int>(N));
	printf("\tNumber of ground truth inliers = %d.\n", static_cast<int>(inlier_number));
	printf("\tTheoretical RANSAC iteration number at %.2f confidence = %d.\n",
		ransac_confidence_, static_cast<int>(log(1.0 - ransac_confidence_) / log(1.0 - pow(static_cast<double>(inlier_number) / static_cast<double>(N), 4))));

	// Initialize the sampler used for selecting minimal samples
	gcransac::sampler::UniformSampler main_sampler(&points);

	MAGSAC<cv::Mat, magsac::utils::DefaultFundamentalMatrixEstimator> magsac;
	magsac.setMaximumThreshold(maximum_threshold_); // The maximum noise scale sigma allowed
	magsac.setIterationLimit(1e4); // Iteration limit to interrupt the cases when the algorithm run too long.

	int iteration_number = 0; // Number of iterations required
	ModelScore score; // The model score

	std::chrono::time_point<std::chrono::system_clock> end,
		start = std::chrono::system_clock::now();
	const bool success = magsac.run(points, // The data points
		ransac_confidence_, // The required confidence in the results
		estimator, // The used estimator
		main_sampler, // The sampler used for selecting minimal samples in each iteration
		model, // The estimated model
		iteration_number, // The number of iterations
		score); // The score of the estimated model
	end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;

	printf("\tActual number of iterations drawn by MAGSAC at %.2f confidence: %d\n", ransac_confidence_, iteration_number);
	printf("\tElapsed time: %f secs\n", elapsed_seconds.count());

	if (!success)
	{
		printf("No reasonable model has been found.\n");
		return;
	}

	// Compute the RMSE given the ground truth inliers
	double rmse = 0, error;
	for (const auto &inlier_idx : ground_truth_inliers)
		rmse += estimator.squaredResidual(points.row(inlier_idx), model);
	rmse = sqrt(rmse / static_cast<double>(inlier_number));
	printf("\tRMSE error: %f px\n", rmse);

	std::vector<bool> inliers_mask;
	double inlier_thd = maximum_threshold_;
	magsac.getModelInliersMask(points, model, estimator, inlier_thd, inliers_mask);
	uint inliers_cnt = std::count(inliers_mask.begin(), inliers_mask.end(), true);
	printf("\tNumber of inliers for threshold %2.1f: %d\n", inlier_thd, inliers_cnt);

	// Visualization part.
	// Inliers are selected using threshold and the estimated model. 
	// This part is not necessary and is only for visualization purposes. 
	if (draw_results_)
	{
		std::vector<int> obtained_labeling(points.rows, 0);

		for (auto pt_idx = 0; pt_idx < points.rows; ++pt_idx)
		{
			// Computing the residual of the point given the estimated model
			auto residual = estimator.residual(points.row(pt_idx),
				model.descriptor);

			// Change the label to 'inlier' if the residual is smaller than the threshold
			if (drawing_threshold_ >= residual)
				obtained_labeling[pt_idx] = 1;
		}

		// Draw the matches to the images
		cv::Mat out_image;
		drawMatches<double, int>(points, obtained_labeling, image1, image2, out_image);

		// Show the matches
		std::string window_name = "Visualization with threshold = " + std::to_string(drawing_threshold_) + " px; Maximum threshold is = " + std::to_string(maximum_threshold_);
		showImage(out_image,
			window_name,
			1600,
			900);
		out_image.release();
	}

	// Clean up the memory occupied by the images
	image1.release();
	image2.release();
}

void testHomographyFitting(
	double ransac_confidence_, // The confidence required
	double maximum_threshold_, // The maximum threshold value
	std::string test_scene_, // The name of the current test scene
	bool draw_results_, // A flag determining if the results should be visualized
	double drawing_threshold_) // The threshold used for visualizing the results
{
	// Print the name of the current test scene
	printf("\tProcessed scene = '%s'.\n", test_scene_.c_str());

	// Load the images of the current test scene
	cv::Mat image1 = cv::imread("data/homography/" + test_scene_ + "A.png");
	cv::Mat image2 = cv::imread("data/homography/" + test_scene_ + "B.png");
	if (image1.cols == 0 || image2.cols == 0) // If the images have not been loaded, try to load them as jpg files.
	{
		image1 = cv::imread("data/homography/" + test_scene_ + "A.jpg");
		image2 = cv::imread("data/homography/" + test_scene_ + "B.jpg");
	}

	// If the images have not been loaded, return
	if (image1.cols == 0 || image2.cols == 0)
	{
		fprintf(stderr, "A problem occured when loading the images for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	cv::Mat points; // The point correspondences, each is of format "x1 y1 x2 y2"
	std::vector<int> ground_truth_labels; // The ground truth labeling provided in the dataset

	// A function loading the points from files
	readAnnotatedPoints("data/homography/" + test_scene_ + "_pts.txt", // The path where the reference labeling and the points are found
		points, // All data points 
		ground_truth_labels); // The reference labeling

	// The number of points in the datasets
	const size_t point_number = points.rows; // The number of points in the scene

	if (point_number == 0) // If there are no points, return
	{
		fprintf(stderr, "A problem occured when loading the annotated points for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	magsac::utils::DefaultHomographyEstimator estimator; // The robust homography estimator class containing the function for the fitting and residual calculation
	gcransac::Homography model; // The estimated model

	// In this used datasets, the manually selected inliers are not all inliers but a subset of them.
	// Therefore, the manually selected inliers are augmented as follows: 
	// (i) First, the implied model is estimated from the manually selected inliers.
	// (ii) Second, the inliers of the ground truth model are selected.
	std::vector<int> refined_labels = ground_truth_labels;
	refineManualLabeling<gcransac::Homography, magsac::utils::DefaultHomographyEstimator>(
		points, // The data points
		refined_labels, // The refined labeling
		estimator, // The model estimator
		2.0); // The used threshold in pixels

	// Select the inliers from the labeling
	std::vector<int> ground_truth_inliers = getSubsetFromLabeling(ground_truth_labels, 1),
		refined_inliers = getSubsetFromLabeling(refined_labels, 1);
	if (ground_truth_inliers.size() < refined_inliers.size())
		ground_truth_inliers.swap(refined_inliers);

	const size_t reference_inlier_number = ground_truth_inliers.size();

	printf("\tEstimated model = 'homography'.\n");
	printf("\tNumber of correspondences loaded = %d.\n", static_cast<int>(point_number));
	printf("\tNumber of ground truth inliers = %d.\n", static_cast<int>(reference_inlier_number));
	printf("\tTheoretical RANSAC iteration number at %.2f confidence = %d.\n",
		ransac_confidence_, static_cast<int>(log(1.0 - ransac_confidence_) / log(1.0 - pow(static_cast<double>(reference_inlier_number) / static_cast<double>(point_number), 4))));

	// Initialize the sampler used for selecting minimal samples
	gcransac::sampler::UniformSampler main_sampler(&points);

	MAGSAC<cv::Mat, magsac::utils::DefaultHomographyEstimator> magsac;
	magsac.setMaximumThreshold(maximum_threshold_); // The maximum noise scale sigma allowed
	magsac.setIterationLimit(1e4); // Iteration limit to interrupt the cases when the algorithm run too long.
	magsac.setReferenceThreshold(2.0);

	int iteration_number = 0; // Number of iterations required
	ModelScore score; // The model score

	std::chrono::time_point<std::chrono::system_clock> end,
		start = std::chrono::system_clock::now();
	magsac.run(points, // The data points
		ransac_confidence_, // The required confidence in the results
		estimator, // The used estimator
		main_sampler, // The sampler used for selecting minimal samples in each iteration
		model, // The estimated model
		iteration_number, // The number of iterations
		score); // The score of the estimated model
	end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end - start;
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);

	printf("\tActual number of iterations drawn by MAGSAC at %.2f confidence: %d\n", ransac_confidence_, iteration_number);
	printf("\tElapsed time: %f secs\n", elapsed_seconds.count());

	if (model.descriptor.size() == 0)
	{
		// Clean up the memory occupied by the images
		image1.release();
		image2.release();
		return;
	}

	// Compute the root mean square error (RMSE) using the ground truth inliers
	double rmse = 0; // The RMSE error
	// Iterate through all inliers and calculate the error
	for (const auto& inlier_idx : ground_truth_inliers)
		rmse += estimator.squaredResidual(points.row(inlier_idx), model);
	rmse = sqrt(rmse / static_cast<double>(reference_inlier_number));
	printf("\tRMSE error: %f px\n", rmse);

	std::vector<bool> inliers_mask;
	double inlier_thd = maximum_threshold_;
	magsac.getModelInliersMask(points, model, estimator, inlier_thd, inliers_mask);
	uint inliers_cnt = std::count(inliers_mask.begin(), inliers_mask.end(), true);
	printf("\tNumber of inliers for threshold %2.1f: %d\n", inlier_thd, inliers_cnt);

	// Visualization part.
	// Inliers are selected using threshold and the estimated model. 
	// This part is not necessary and is only for visualization purposes. 
	if (draw_results_)
	{
		// The labeling implied by the estimated model and the drawing threshold
		std::vector<int> obtained_labeling(points.rows, 0);

		for (size_t point_idx = 0; point_idx < points.rows; ++point_idx)
		{
			// Computing the residual of the point given the estimated model
			auto residual = sqrt(estimator.residual(points.row(point_idx),
				model));

			// Change the label to 'inlier' if the residual is smaller than the threshold
			if (drawing_threshold_ >= residual)
				obtained_labeling[point_idx] = 1;
		}

		cv::Mat out_image;

		// Draw the matches to the images
		drawMatches<double, int>(points, // All points 
			obtained_labeling, // The labeling obtained by OpenCV
			image1, // The source image
			image2, // The destination image
			out_image); // The image with the matches drawn

		// Show the matches
		std::string window_name = "Visualization with threshold = " + std::to_string(drawing_threshold_) + " px; Maximum threshold is = " + std::to_string(maximum_threshold_);
		showImage(out_image, // The image with the matches drawn
			window_name, // The name of the window
			1600, // The width of the window
			900); // The height of the window
		out_image.release(); // Clean up the memory
	}

	// Clean up the memory occupied by the images
	image1.release();
	image2.release();
}

void opencvHomographyFitting(
	double ransac_confidence_,
	double threshold_,
	std::string test_scene_,
	bool draw_results_,
	const bool with_magsac_post_processing_)
{
	// Print the name of the current scene
	printf("\tProcessed scene = '%s'.\n", test_scene_.c_str());

	// Load the images of the current test scene
	cv::Mat image1 = cv::imread("data/homography/" + test_scene_ + "A.png");
	cv::Mat image2 = cv::imread("data/homography/" + test_scene_ + "B.png");
	if (image1.cols == 0 || image2.cols == 0) // If the images have not been loaded, try to load them as jpg files.
	{
		image1 = cv::imread("data/homography/" + test_scene_ + "A.jpg");
		image2 = cv::imread("data/homography/" + test_scene_ + "B.jpg");
	}

	// If the images have not been loaded, return
	if (image1.cols == 0 || image2.cols == 0)
	{
		fprintf(stderr, "A problem occured when loading the images for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	cv::Mat points; // The point correspondences, each is of format "x1 y1 x2 y2"
	std::vector<int> ground_truth_labels; // The ground truth labeling provided in the dataset

	// A function loading the points from files
	readAnnotatedPoints("data/homography/" + test_scene_ + "_pts.txt", // The path where the reference labeling and the points are found
		points, // All data points 
		ground_truth_labels); // The reference labeling

	// The number of points in the scene
	const size_t point_number = points.rows;

	if (point_number == 0) // If there are no points, return
	{
		fprintf(stderr, "A problem occured when loading the annotated points for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	magsac::utils::DefaultHomographyEstimator estimator; // The robust homography estimator class containing the function for the fitting and residual calculation
	gcransac::Homography model; // The estimated model

	// In this used datasets, the manually selected inliers are not all inliers but a subset of them.
	// Therefore, the manually selected inliers are augmented as follows: 
	// (i) First, the implied model is estimated from the manually selected inliers.
	// (ii) Second, the inliers of the ground truth model are selected.
	std::vector<int> refined_labels = ground_truth_labels;
	refineManualLabeling<gcransac::Homography, magsac::utils::DefaultHomographyEstimator>(
		points, // The data points
		refined_labels, // The refined labeling
		estimator, // The model estimator
		2.0); // The used threshold in pixels

	// Select the inliers from the labeling
	std::vector<int> ground_truth_inliers = getSubsetFromLabeling(ground_truth_labels, 1), // The inlier indices from the reference labeling
		refined_inliers = getSubsetFromLabeling(refined_labels, 1); // The inlier indices the refined labeling

	// If there are more inliers in the refined labeling, use them.
	if (ground_truth_inliers.size() < refined_inliers.size())
		ground_truth_inliers.swap(refined_inliers);

	// The number of reference inliers
	const size_t reference_inlier_number = ground_truth_inliers.size();

	printf("\tEstimated model = 'homography'.\n");
	printf("\tNumber of correspondences loaded = %d.\n", static_cast<int>(point_number));
	printf("\tNumber of ground truth inliers = %d.\n", static_cast<int>(reference_inlier_number));

	// Define location of sub matrices in data matrix
	cv::Rect roi1(0, 0, 2, point_number); // The ROI of the points in the source image
	cv::Rect roi2(2, 0, 2, point_number); // The ROI of the points in the destination image

	// The labeling obtained by OpenCV
	std::vector<int> obtained_labeling(points.rows, 0);

	// Variables to measure time
	std::chrono::time_point<std::chrono::system_clock> end,
		start = std::chrono::system_clock::now();

	// Estimating the homography matrix by OpenCV's RANSAC
	cv::Mat cv_homography = cv::findHomography(cv::Mat(points, roi1), // The points in the first image
		cv::Mat(points, roi2), // The points in the second image
		cv::RANSAC, // The method used for the fitting
		threshold_, // The inlier-outlier threshold
		obtained_labeling); // The obtained labeling

	// Convert cv::Mat to Eigen::Matrix3d 
	Eigen::Matrix3d homography =
		Eigen::Map<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>>(cv_homography.ptr<double>(), 3, 3);

	end = std::chrono::system_clock::now();

	// Calculate the processing time of OpenCV
	std::chrono::duration<double> elapsed_seconds = end - start;

	// Print the processing time
	printf("\tElapsed time: %f secs\n", elapsed_seconds.count());

	// Applying the MAGSAC post-processing step using the OpenCV's output
	// as the input.
	if (with_magsac_post_processing_)
	{
		fprintf(stderr, "The MAGSAC post-processing is not implemented yet.\n");
	}

	// Compute the root mean square error (RMSE) using the ground truth inliers
	double rmse = 0; // The RMSE error
	// Iterate through all inliers and calculate the error
	for (const auto& inlier_idx : ground_truth_inliers)
		rmse += estimator.squaredResidual(points.row(inlier_idx), homography);
	// Divide by the inlier number and get the square root
	rmse = std::sqrt(rmse / reference_inlier_number);

	// Print the RMSE error
	printf("\tRMSE error: %f px\n", rmse);

	// Visualization part.
	// Inliers are selected using threshold and the estimated model. 
	// This part is not necessary and is only for visualization purposes. 
	if (draw_results_)
	{
		// Draw the matches to the images
		cv::Mat out_image;

		// Draw the inlier matches 
		drawMatches<double, int>(points, // All points 
			obtained_labeling, // The labeling obtained by OpenCV
			image1, // The source image
			image2, // The destination image
			out_image); // The image with the matches drawn

		// Show the matches
		std::string window_name = "OpenCV's RANSAC";
		showImage(out_image, // The image with the matches drawn
			window_name, // The name of the window
			1600, // The width of the window
			900); // The height of the window
		out_image.release(); // Clean up the memory
	}

	// Clean up the memory occupied by the images
	image1.release();
	image2.release();
}

void opencvFundamentalMatrixFitting(
	double ransac_confidence_,
	double threshold_,
	std::string test_scene_,
	bool draw_results_,
	const bool with_magsac_post_processing_)
{
	// Print the name of the current test scene
	printf("\tProcessed scene = '%s'.\n", test_scene_.c_str());

	// Load the images of the current test scene
	cv::Mat image1 = cv::imread("data/fundamental_matrix/" + test_scene_ + "A.png");
	cv::Mat image2 = cv::imread("data/fundamental_matrix/" + test_scene_ + "B.png");
	if (image1.cols == 0 || image2.cols == 0) // Try to load jpg files if there are no pngs
	{
		image1 = cv::imread("data/fundamental_matrix/" + test_scene_ + "A.jpg");
		image2 = cv::imread("data/fundamental_matrix/" + test_scene_ + "B.jpg");
	}

	// If the images are not loaded, return
	if (image1.cols == 0 || image2.cols == 0)
	{
		fprintf(stderr, "A problem occured when loading the images for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	cv::Mat points; // The point correspondences, each is of format x1 y1 x2 y2
	std::vector<int> ground_truth_labels; // The ground truth labeling provided in the dataset

	// A function loading the points from files
	readAnnotatedPoints("data/fundamental_matrix/" + test_scene_ + "_pts.txt", // The path to the labels and points
		points, // The container for the loaded points
		ground_truth_labels); // The ground thruth labeling

	// The number of points in the datasets
	const size_t point_number = points.rows; // The number of points in the scene

	if (point_number == 0) // If there are no points, return
	{
		fprintf(stderr, "A problem occured when loading the annotated points for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	gcransac::utils::DefaultFundamentalMatrixEstimator estimator; // The robust homography estimator class containing the function for the fitting and residual calculation
	gcransac::FundamentalMatrix model; // The estimated model parameters

	// In this used datasets, the manually selected inliers are not all inliers but a subset of them.
	// Therefore, the manually selected inliers are augmented as follows: 
	// (i) First, the implied model is estimated from the manually selected inliers.
	// (ii) Second, the inliers of the ground truth model are selected.
	std::vector<int> refined_labels = ground_truth_labels;
	refineManualLabeling<gcransac::FundamentalMatrix, gcransac::utils::DefaultFundamentalMatrixEstimator>(
		points, // All data points
		ground_truth_labels, // The refined labeling
		estimator, // The estimator used for determining the underlying model
		0.35); // Threshold value from the LO*-RANSAC paper

	// Select the inliers from the labeling
	std::vector<int> ground_truth_inliers = getSubsetFromLabeling(ground_truth_labels, 1), // The indices of the inliers from the reference labeling
		refined_inliers = getSubsetFromLabeling(refined_labels, 1); // The indices of the inlier from the refined labeling

 // If the refined labeling has more inliers than the original one, use the refined.
 // It can happen that the model fit to the inliers of the reference labeling is close to being degenerate.
 // In those cases, enforcing, e.g., the rank-two constraint leads to a model which selects fewer inliers than the original one. 
	if (refined_inliers.size() > ground_truth_inliers.size())
		refined_inliers.swap(ground_truth_inliers);

	// Number of inliers in the reference labeling
	const size_t reference_inlier_number = ground_truth_inliers.size();

	printf("\tEstimated model = 'fundamental matrix'.\n");
	printf("\tNumber of correspondences loaded = %d.\n", static_cast<int>(point_number));
	printf("\tNumber of ground truth inliers = %d.\n", static_cast<int>(reference_inlier_number));

	// Define location of sub matrices in data matrix
	cv::Rect roi1(0, 0, 2, point_number); // The ROI of the points in the source image
	cv::Rect roi2(2, 0, 2, point_number); // The ROI of the points in the destination image

	// The labeling obtained by OpenCV
	std::vector<uchar> obtained_labeling(points.rows, 0);

	// Variables to measure the time
	std::chrono::time_point<std::chrono::system_clock> end,
		start = std::chrono::system_clock::now();

	// Fundamental matrix estimation using the OpenCV's function
	cv::Mat cv_fundamental_matrix = cv::findFundamentalMat(cv::Mat(points, roi1), // The points in the source image
		cv::Mat(points, roi2), // The points in the destination image
		cv::RANSAC,
		threshold_,
		ransac_confidence_,
		obtained_labeling);
	end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	std::time_t end_time = std::chrono::system_clock::to_time_t(end);

	// Convert cv::Mat to Eigen::Matrix3d 
	Eigen::Matrix3d fundamental_matrix =
		Eigen::Map<Eigen::Matrix<double, 3, 3, Eigen::RowMajor>>(cv_fundamental_matrix.ptr<double>(), 3, 3);

	printf("\tElapsed time: %f secs\n", elapsed_seconds.count());

	// Applying the MAGSAC post-processing step using the OpenCV's output
	// as the input.
	if (with_magsac_post_processing_)
	{
		fprintf(stderr, "\tPost-processing is not implemented yet.\n");
	}

	// Compute the RMSE given the ground truth inliers
	double rmse = 0, error;
	size_t inlier_number = 0;
	for (const auto& inlier_idx : ground_truth_inliers)
	{
		error = estimator.residual(points.row(inlier_idx),
			fundamental_matrix);
		rmse += error;
	}

	rmse = sqrt(rmse / static_cast<double>(reference_inlier_number));
	printf("\tRMSE error: %f px\n", rmse);

	// Visualization part.
	// Inliers are selected using threshold and the estimated model. 
	// This part is not necessary and is only for visualization purposes. 
	if (draw_results_)
	{
		// Draw the matches to the images
		cv::Mat out_image;
		drawMatches<double, uchar>(points, obtained_labeling, image1, image2, out_image);

		// Show the matches
		std::string window_name = "OpenCV's RANSAC";
		showImage(out_image,
			window_name,
			1600,
			900);
		out_image.release();
	}

	// Clean up the memory occupied by the images
	image1.release();
	image2.release();
}

// A method applying OpenCV for essential matrix estimation to one of the built-in scenes
void opencvEssentialMatrixFitting(
	double ransac_confidence_,
	double threshold_,
	const std::string &test_scene_,
	bool draw_results_)
{
	printf("\tProcessed scene = '%s'.\n", test_scene_.c_str());

	// Load the images of the current test scene
	cv::Mat image1 = cv::imread("data/essential_matrix/" + test_scene_ + "1.png");
	cv::Mat image2 = cv::imread("data/essential_matrix/" + test_scene_ + "2.png");
	if (image1.cols == 0)
	{
		image1 = cv::imread("data/essential_matrix/" + test_scene_ + "1.jpg");
		image2 = cv::imread("data/essential_matrix/" + test_scene_ + "2.jpg");
	}

	if (image1.cols == 0)
	{
		fprintf(stderr, "A problem occured when loading the images for test scene '%s'\n", test_scene_.c_str());
		return;
	}

	cv::Mat points; // The point correspondences, each is of format x1 y1 x2 y2
	Eigen::Matrix3d intrinsics_source, // The intrinsic parameters of the source camera
		intrinsics_destination; // The intrinsic parameters of the destination camera

	// A function loading the points from files
	readPoints<4>("data/essential_matrix/" + test_scene_ + "_pts.txt",
		points);

	// Loading the intrinsic camera matrices
	static const std::string source_intrinsics_path = "data/essential_matrix/" + test_scene_ + "1.K";
	if (!gcransac::utils::loadMatrix<double, 3, 3>(source_intrinsics_path,
		intrinsics_source))
	{
		printf("An error occured when loading the intrinsics camera matrix from '%s'\n",
			source_intrinsics_path.c_str());
		return;
	}

	static const std::string destination_intrinsics_path = "data/essential_matrix/" + test_scene_ + "2.K";
	if (!gcransac::utils::loadMatrix<double, 3, 3>(destination_intrinsics_path,
		intrinsics_destination))
	{
		printf("An error occured when loading the intrinsics camera matrix from '%s'\n",
			destination_intrinsics_path.c_str());
		return;
	}

	// Normalize the point coordinates by the intrinsic matrices
	cv::Mat normalized_points(points.size(), CV_64F);
	gcransac::utils::normalizeCorrespondences(points,
		intrinsics_source,
		intrinsics_destination,
		normalized_points);

	cv::Mat cv_intrinsics_source(3, 3, CV_64F);
	cv_intrinsics_source.at<double>(0, 0) = intrinsics_source(0, 0);
	cv_intrinsics_source.at<double>(0, 1) = intrinsics_source(0, 1);
	cv_intrinsics_source.at<double>(0, 2) = intrinsics_source(0, 2);
	cv_intrinsics_source.at<double>(1, 0) = intrinsics_source(1, 0);
	cv_intrinsics_source.at<double>(1, 1) = intrinsics_source(1, 1);
	cv_intrinsics_source.at<double>(1, 2) = intrinsics_source(1, 2);
	cv_intrinsics_source.at<double>(2, 0) = intrinsics_source(2, 0);
	cv_intrinsics_source.at<double>(2, 1) = intrinsics_source(2, 1);
	cv_intrinsics_source.at<double>(2, 2) = intrinsics_source(2, 2);

	const size_t N = points.rows;

	const double normalized_threshold =
		threshold_ / ((intrinsics_source(0, 0) + intrinsics_source(1, 1) +
			intrinsics_destination(0, 0) + intrinsics_destination(1, 1)) / 4.0);

	// Define location of sub matrices in data matrix
	cv::Rect roi1(0, 0, 2, N);
	cv::Rect roi2(2, 0, 2, N);

	std::vector<uchar> obtained_labeling(points.rows, 0);
	std::chrono::time_point<std::chrono::system_clock> end,
		start = std::chrono::system_clock::now();

	// Estimating the homography matrix by OpenCV's RANSAC
	cv::Mat cv_essential_matrix = cv::findEssentialMat(cv::Mat(normalized_points, roi1), // The points in the first image
		cv::Mat(normalized_points, roi2), // The points in the second image
		cv::Mat::eye(3, 3, CV_64F), // The intrinsic camera matrix of the source image
		cv::RANSAC, // The method used for the fitting
		ransac_confidence_, // The RANSAC confidence
		normalized_threshold, // The inlier-outlier threshold
		obtained_labeling); // The obtained labeling

	// Convert cv::Mat to Eigen::Matrix3d 
	Eigen::Matrix3d essential_matrix =
		Eigen::Map<Eigen::Matrix3d>(cv_essential_matrix.ptr<double>(), 3, 3);

	end = std::chrono::system_clock::now();

	// Calculate the processing time of OpenCV
	std::chrono::duration<double> elapsed_seconds = end - start;

	printf("\tElapsed time: %f secs\n", elapsed_seconds.count());

	size_t inlier_number = 0;

	// Visualization part.
	for (auto pt_idx = 0; pt_idx < points.rows; ++pt_idx)
	{
		// Change the label to 'inlier' if the residual is smaller than the threshold
		if (obtained_labeling[pt_idx])
			++inlier_number;
	}

	printf("\tNumber of points closer than %f = %d\n",
		threshold_, inlier_number);

	if (draw_results_)
	{
		// Draw the matches to the images
		cv::Mat out_image;
		drawMatches<double, uchar>(points, obtained_labeling, image1, image2, out_image);

		// Show the matches
		std::string window_name = "Threshold = " + std::to_string(threshold_) + " px";
		showImage(out_image,
			window_name,
			1600,
			900);
		out_image.release();
	}

	// Clean up the memory occupied by the images
	image1.release();
	image2.release();
}
