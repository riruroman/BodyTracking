/********************************************************************************
 * file name: main.cpp
 * authors: Markus Stabel, Marco Fendrich
 * last changes: 22.03.2018
 * content: automatically being executed as main script of the program; calls all
 relevant other scripts and calculates a new HMM either based on user input or
 variables introduced below. Can alternatively be used to calculate the
 probability of a new dataset based on existing HMMs by using the corresponding
 variabels below.
 ********************************************************************************/

#include "Matrix.h"
#include "Markov.h"
#include "kMeans.h"
#include <numeric>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <time.h>
#include <vector>
#include <thread>
#include <cmath>
#include <cfloat>
using namespace std;

// declaration above main for function to be found instead of creating header file
int getFullTrainingNumber(string trainingFilePath, string trainingFileName);
void updateFilePaths();
void multiThreadOptimisation(int lrDepth, int numStates, int numEmissions, int trainingNumber, vector<vector<vector<int>>> sequence, int hmmTries);
void multiThreadHMMCreation(int lrDepth, int numStates, int numEmissions, int trainingNumber, vector<vector<vector<int>>> sequence, int threadNumber);
vector<double> calculateProbability(HMMModel models[6]);

/// ***** ***** ***** Settings to be changed by user ***** ***** ***** ///
/// ***** Choose operational mode ***** ///
// Create new HMM based on the all the training file
bool createHMM = false;
// Create HMMs using 4 thread and keep on calculating new HMMs and replacing the old ones if those are better,only end when hmmtries reach max.
bool optimiseInfiniteHMM = false;
// Try all the combination of parameters( include numStates,numEmissions,lrDepths,tracker files),outputting table of probabilities in overview file.
bool optimiseMovementRecognition = true;
// Calculating the probability for a data set based on an already existing HMM.
bool calculateSingleProbability = false;
// Show debug messages on console
bool debug = false;

/// ***** Set paths and HMM parameters ***** ///
// File name of the data set to be used used for single probability calculation
string currentMovement = "Yoga_Krieger5";
// Path for the source files
// NOTE: change working directory if necessary
string trainingFilePath = "../Training/";
// Base file name in the format "<trainingFileName>_<number>.txt" (only trainingFileName required)
string trainingFileName = "Yoga_Krieger_";
// Path for HMM and clusters to be saved in
string writeFilePath = "../Tracking/";
// Base file name for files to be created (ending either in _<number>_HMM or _<number>_cluster)
string writeFileName = "Yoga_Krieger";
// Number of hidden states used for calculating the HMM (standard is 6)
int numStates = 6;
// Number of clusters used for the data set taken as input for the HMM (standard is 8)
int numEmissions = 8;
// Number of times an HMM is created per tracker before using the one with the best threshold
int hmmTries = 100;
// Left to right depth of HMM; 0 leaves the start points to be random
int lrDepth = 2;

/// ***** General variables used by system ***** ///
static const int num_threads = 4; // number of thread used for extensive calculations as well as amount of lrDepths to be tested; should under normal circumstances be left at 4
int trainingNumber;
vector<vector<vector<int>>> sequence;
vector<KMeans> kmeans;
vector<vector<vector<int>>> sequenceX;
vector<KMeans> kmeansX;
ofstream file;

/********************************************************************************
 * method: main
 * description: see file description
 * parameters: none
 * return value: 0
 ********************************************************************************/
int main() {
	// set file paths in kMeans to input above
	updateFilePaths();
	srand((unsigned int)time(NULL)); // randomises random numbers
	
	/// ***** ***** ***** Creating a new HMM ***** ***** ***** ///
	if (createHMM) {
		cout << "<Automatic execution>\n" << "Using predefined variables for execution.\n\n";
		trainingNumber = getFullTrainingNumber(trainingFilePath, trainingFileName);
		cout << "Create HMM using " << trainingNumber << " files.\n";
		
		/// ***** ***** Actual creation of HMM ***** ***** ///
		cout << "Training HMM with " << numStates << " hidden states and " << numEmissions << " possible emissions using " << trainingNumber << " sets of training data. \n\n";
		
		// read data and use k-Means algorithm for clustering
		cout << "Clustering data points using k-Means algorithm. \n";
		vector<KMeans> kmeans = calculateClusters(0, trainingNumber, numEmissions, 7, 1000);
		cout << "Matching data points of current data sets to clusters. ";
		vector<vector<vector<int>>> sequence = sortDataToClusters(trainingFileName, trainingNumber, kmeans);
		
		cout << "Single data sets clustered successfully. \n\n";
		
		if (debug) { // console output of clusters
			for (int kk = 0; kk < 6; kk++) {
				if (!sequence.at(kk).empty()) {
					cout << trackerNames[kk] << " clusters: \n";
					for (int ii = 0; ii < sequence.at(kk).size(); ii++) {
						cout << "File #" << ii + 1 << ": \n";
						for (int &singleClusterNumber : sequence.at(kk).at(ii)) {
							cout << singleClusterNumber << " ";
						}
						cout << "\n";
					}
					cout << "\n";
				}
			}
		}
		
		// train HMM per tracker and calculate individual probability thresholds of trackers 
		vector<double> probabilities(6, 0);
		for (int ii = 0; ii < 6; ii++) {
			if (!sequence.at(ii).empty()) {
				cout << "Training HMM for " + trackerNames[ii] + " using the Baum-Welch algorithm with " << hmmTries << " executes. Converged after iteration: ";
				for (int jj = 0; jj < hmmTries; jj++) {
					// use Baum-Welch-Algorithm to train HMM and write it to a file 
					HMMModel model(numStates, numEmissions, lrDepth);
					model.trainHMM(sequence.at(ii));
					//if (jj < hmmTries - 1) cout << ", ";
					if (isnan(probabilities.at(ii))|| probabilities.at(ii) == 0 || model.getProbabilityThreshold() > probabilities.at(ii)) {
						model.writeHMM(writeFilePath, writeFileName + "_" + to_string(ii));
						probabilities.at(ii) = (model.getProbabilityThreshold());
					}
					cout << jj << ", ";
				}
				cout << ".\n" << "HMM #" << ii << " has been trained and saved to " << writeFilePath + writeFileName + "_HMM_" + to_string(ii) + ".txt. " << "The log probability threshold is " << probabilities.at(ii) << ".\n\n";
			}
		}
		
		bool emptyTracker = false;
		for (int ii = 0; ii < 6; ii++) {
			if (sequence.at(ii).empty()) {
				if (!emptyTracker) {
					cout << "Some trackers were not found in the given data set: \n \{ ";
					emptyTracker = true;
				} else {
					cout << ", ";
				}
				cout << trackerNames[ii];
			}
		}
		if (emptyTracker) {
			cout << " \n\nThose trackers have been skipped and no corresponding HMM was created. \n\n";
		}
		
		for (int ii = 0; ii < 6; ii++) {
			if (!sequence.at(ii).empty()) {
				cout << "The log probability threshold of " << trackerNames[ii] << " is " << probabilities.at(ii) << ". \n";
			}
		}
	}
	
	/// ***** ***** ***** Calculating probability for data set ***** ***** ***** ///
	else if (calculateSingleProbability) {
		cout << "<Calculating probability for data set>\n" << "Using predefined variables for execution." "\n\n";
		
		cout << "Loading cluster coordinates.\n";
		vector<KMeans> kmeanVector(6);
		bool trackersPresent[6]; // stores which trackers are present in HMMs/clusters
		for (int ii = 0; ii < 6; ii++) {
			try {
				KMeans kmeans(writeFilePath, writeFileName + "_" + to_string(ii));
				kmeanVector.at(ii) = kmeans;
				cout << "Cluster coordinates for " << trackerNames[ii] << " found. \n";
				trackersPresent[ii] = true;
			} catch (invalid_argument) {
				trackersPresent[ii] = false;
			}
		}
		cout << "\n";
		
		cout << "Sorting new data sets into clusters. ";
		vector<vector<vector<int>>> dataClusters = sortDataToClusters(currentMovement, 1, kmeanVector);
		cout << "Normalised data sets clustered. \n";
		
		if (debug) { // console output of clusters
			for (int kk = 0; kk < 6; kk++) {
				cout << "\n";
				if (!dataClusters.at(kk).empty()) {
					cout << trackerNames[kk] << " clusters: \n";
					for (int &singleClusterNumber : dataClusters.at(kk).at(0)) {
						cout << singleClusterNumber << " ";
					}
					cout << "\n";
				}
			}
		}
		
		for (int ii = 0; ii < 6; ii++) {
			if (trackersPresent[ii]) {
				cout << "Calculating probability for " << trackerNames[ii] << " based on given HMM: ";
				HMMModel model(writeFilePath, writeFileName + "_" + to_string(ii));
				cout << model.calculateProbability(dataClusters.at(ii).at(0)) << "\n";
			}
		}
	}
	
	// Optimise movement recognition manually by outputting table of probabilities (currently only debug functionality)
	else if (optimiseMovementRecognition) {
		// Open threads
		thread t[num_threads];
		
		// Creates file for data and writes first row giving information about the data to come
		file.open(writeFilePath + writeFileName + "_Overview.txt", ios::out /*| ios::trunc*/);
		file << "Number of states" << "; " << "Number of emissions" << "; " << "LR Depth" << "; " << "Probability" << ";\n";
		
		// Variables to be used in training
		trainingNumber = getFullTrainingNumber(trainingFilePath, trainingFileName);
		if (trainingNumber > 10) trainingNumber = 10; // Limit training to ten files to see whether other correct files are being correctly recognized as such
		int emissionIterations[9] = { 8, 10, 12, 16, 20, 30, 40, 50, 100 };
		
		// Actual calculations
		for (int &numEmissions : emissionIterations) {
			kmeans = calculateClusters(0, trainingNumber, numEmissions, 7, 1000);
			sequence = sortDataToClusters(trainingFileName, trainingNumber, kmeans);
			for (numStates = 6; numStates <= 16; numStates += 2) {
				cout << "Splitting threads**********************************************************************\n";
				cout << "Training HMM with a left to right depth of " << lrDepth << ", " << numStates << " hidden states and " << numEmissions << " possible emissions using " << trainingNumber << " sets of training data. \n\n";
				
				// Uses threadIteration for lrDepth as well
				for (int threadIteration = 0; threadIteration < num_threads; threadIteration++) {
					cout << "Launched from thread " << threadIteration << "\n";
					t[threadIteration] = thread(multiThreadOptimisation, threadIteration, numStates, numEmissions, trainingNumber, sequence, hmmTries);
				}
				for (thread &singleThread : t) {
					singleThread.join();
				}
				cout << "\nRejoined threads**********************************************************************\n\n";
				
			}
		}
	}
	
	// Optimise a single HMM by indefinitely calculating new HMMs and replacing the old ones if those are better
	else if (optimiseInfiniteHMM) {
		// Open threads
		thread t[num_threads];
		trainingNumber = getFullTrainingNumber(trainingFilePath, trainingFileName);
		kmeans = calculateClusters(0, trainingNumber, numEmissions, 7, 1000);
		sequence = sortDataToClusters(trainingFileName, trainingNumber, kmeans);
		cout << "Training HMM with a left to right depth of " << lrDepth << ", " << numStates << " hidden states and " << numEmissions << " possible emissions using " << trainingNumber << " sets of training data. \n\n";
		cout << "Splitting threads**********************************************************************\n";
		for (int threadIteration = 0; threadIteration < num_threads; threadIteration++) {
			cout << "Launched from thread " << threadIteration << "\n";
			t[threadIteration] = thread(multiThreadHMMCreation, threadIteration, numStates, numEmissions, trainingNumber, sequence, threadIteration);
		}
		for (thread &singleThread : t) {
			singleThread.join();
		}
	}
	cout << "\n";
	return 0;
}

/********************************************************************************
 * method:		multiThreadOptimisation
 * description:	Creates an HMM based on a sequence of clusters and the necessary
 parameters, calculate the probability of the given data sets and
 writes them into a collective file for later evaluation. Used for
 call of single threads while multithreading to save computation
 time.
 * parameters:	lrDepth is the left to right depth of the HMMs to be created
 numStates is the number of hidden states of the wanted HMMs
 numEmissions is the number of clusters to be used in the HMMs
 trainingNumber is the amount of data sets to be taken into account
 sequence are the cluster numbers of the data sets used for creation
 HMMtries is the number of HMMs created of which the best is taken
 * return value: none
 ********************************************************************************/
void multiThreadOptimisation(int lrDepth, int numStates, int numEmissions, int trainingNumber, vector<vector<vector<int>>> sequence, int hmmTries) {
	HMMModel finalModels[6];
	vector<double> probabilities(6, 0);
	
	for (int ii = 0; ii < 6; ii++) {
		if (!sequence.at(ii).empty()) {
			for (int jj = 0; jj < hmmTries; jj++) {
				/* use Baum-Welch-Algorithm to train HMM and write it to a file */
				HMMModel model(numStates, numEmissions, lrDepth);
				model.trainHMM(sequence.at(ii));
				if (jj < hmmTries - 1) cout << ", ";
				if (probabilities.at(ii) == 0 || model.getProbabilityThreshold() > probabilities.at(ii)) {
					finalModels[ii] = model;
					probabilities.at(ii) = (model.getProbabilityThreshold());
				}
			}
		}
	}
	
	vector<double> probabilityTracker;
    vector<double> probabilityFile;
    for (int fileNumber = 0; fileNumber < getFullTrainingNumber(trainingFilePath, trainingFileName); fileNumber++) {
        currentMovement = trainingFileName + to_string(fileNumber);
        probabilityTracker = calculateProbability(finalModels);
        //ignore NAN
        auto iter = probabilityTracker.begin();
        while (iter != probabilityTracker.end()) {
            if (isnan(*iter)) {
                iter = probabilityTracker.erase(iter);
            }
            else {
                ++iter;
            }
        }
        double average = accumulate( probabilityTracker.begin(), probabilityTracker.end(), 0.0)/probabilityTracker.size();
        probabilityFile.push_back(average);
    }
    auto iter = probabilityFile.begin();
    while (iter != probabilityFile.end()) {
        if (isnan(*iter)) {
            iter = probabilityFile.erase(iter);
        }
        else {
            ++iter;
        }
    }
    double averageFile = accumulate( probabilityFile.begin(), probabilityFile.end(), 0.0)/probabilityFile.size();
    file << numStates << "; " << numEmissions << "; " << lrDepth << "; "<< averageFile << ";\n";

}

/********************************************************************************
 * method:		multiThreadHMMCreation
 * description:	Creates an HMM based on a sequence of clusters and the necessary
 parameters, calculate the probability of the given data sets and
 writes them into a collective file for later evaluation. Used for
 call of single threads while multithreading to save computation
 time.
 * parameters:	lrDepth is the left to right depth of the HMMs to be created
 numStates is the number of hidden states of the wanted HMMs
 numEmissions is the number of clusters to be used in the HMMs
 trainingNumber is the amount of data sets to be taken into account
 sequence are the cluster numbers of the data sets used for creation
 HMMtries is the number of HMMs created of which the best is taken
 * return value: none
 ********************************************************************************/
void multiThreadHMMCreation(int lrDepth, int numStates, int numEmissions, int trainingNumber, vector<vector<vector<int>>> sequence, int threadNumber) {
	HMMModel finalModels[6];
	vector<double> probabilities(6, 0);

    for (int ii = 0; ii < 6; ii++) {
        if (!sequence.at(ii).empty()) {
            cout << "Training HMM for " + trackerNames[ii] + " using the Baum-Welch algorithm with " << hmmTries << " executes. Converged after iteration: ";
            for (int jj = 0; jj < hmmTries; jj++) {
                // use Baum-Welch-Algorithm to train HMM and write it to a file
                HMMModel model(numStates, numEmissions, lrDepth);
                model.trainHMM(sequence.at(ii));
                //if (jj < hmmTries - 1) cout << ", ";
                if (isnan(probabilities.at(ii))||probabilities.at(ii) == 0 || model.getProbabilityThreshold() > probabilities.at(ii)) {
                    finalModels[ii] = model;
                    model.writeHMM(writeFilePath, writeFileName + "_" + to_string(ii) + "_t" + to_string(threadNumber));
                    probabilities.at(ii) = (model.getProbabilityThreshold());
                }
                cout << jj << ", ";
            }
            cout << ".\n" << "HMM #" << ii << " has been trained and saved to " << writeFilePath + writeFileName + "_HMM_" + to_string(ii) + "_t" + to_string(threadNumber)+ ".txt. " << "The log probability threshold is " << probabilities.at(ii) << ".\n\n";
            
        }
    }
	
}


/********************************************************************************
 * method:		calculateProbability
 * description:	Calculates the probability of whether any given tracker (set via
 currentMovement externally) followed the same movement as it did
 in the HMM given as input
 * parameters:	models is an array of HMMs for the six trackers
 * return value: vector of the probability of each tracker
 ********************************************************************************/
vector<double> calculateProbability(HMMModel models[6]) {
	vector<double> returnVector(6);
	
	vector<vector<vector<int>>> dataClusters = sortDataToClusters(currentMovement, 1, kmeans);
	
	for (int ii = 0; ii < 6; ii++) {
		if (!dataClusters.at(ii).empty()) {
			returnVector.at(ii) = models[ii].calculateProbability(dataClusters.at(ii).at(0));
		}
	}
	return returnVector;
}

/********************************************************************************
 * method:		getFullTrainingNumber
 * description:	Outputs total amount of files present in folder that correspond
 to the naming convention when used with the input
 * parameters:	trainingFilePath is the path as string to the files to be read
 trainingFileName is the base name of the files to be read
 * return value: the number of files corresponding to the input given
 ********************************************************************************/
int getFullTrainingNumber(string trainingFilePath, string trainingFileName) {
	int trainingNumber = 0;
	while (ifstream(trainingFilePath + trainingFileName + to_string(trainingNumber) + ".csv")) {
		trainingNumber++;
	}
	return trainingNumber;
}


/********************************************************************************
 * method:		updateFilePaths
 * description:	updates the file paths in kMeans with the ones in main
 * parameters:	none
 * return value: none
 ********************************************************************************/
void updateFilePaths() {
	setTrainingFilePath(trainingFilePath);
	setTrainingFileName(trainingFileName);
	setWriteFilePath(writeFilePath);
	setWriteFileName(writeFileName);
}
