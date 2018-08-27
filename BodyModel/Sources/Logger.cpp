#include "pch.h"
#include "Logger.h"

#include <Kore/Log.h>
#include <ctime>

Logger::~Logger() {
	positionDataOutputFile.close();
	positionDataOutputFile.close();
}

void Logger::startLogger() {
	time_t t = time(0);   // Get time now
	
	positionDataPath.str(std::string());
	positionDataPath << positionData << "_" << t << ".csv";
	
	positionDataOutputFile.open(positionDataPath.str(), std::ios::app); // Append to the end
	positionDataOutputFile << "rawPosX;rawPosY;rawPosZ;rawRotX;rawRotY;rawRotZ;rawRotW;scale\n";
	positionDataOutputFile.flush();
	
	log(Kore::Info, "start logging!");
}

void Logger::endLogger() {
	positionDataOutputFile.close();
    prevScale = 0;
	
	log(Kore::Info, "stop logging!");
}

void Logger::startEvaluationLogger() {
	time_t t = time(0);   // Get time now
	
	evaluationDataPath.str(std::string());
	evaluationConfigPath.str(std::string());
	evaluationDataPath << "eval/" << evaluationDataFilename << "_" << t << ".csv";
	evaluationConfigPath << "eval/" << evaluationConfigFilename << "_" << t << ".csv";
	
	evaluationConfigOutputFile.open(evaluationConfigPath.str(), std::ios::app);
	evaluationConfigOutputFile << "IK Mode;with Orientation;File;lambda;Error Pos Max;Error Rot Max;dMax Arms;dMax Legs;Steps Max\n";
	evaluationConfigOutputFile << ikMode << ";" << withOrientation << ";" << currentGroup[currentFile] << ";" << lambda[ikMode] << ";" << errorMaxPos[ikMode] << ";" << errorMaxRot[ikMode] << ";" << dMaxArms[ikMode] << ";" << dMaxLegs[ikMode] << ";" << maxSteps[ikMode] << "\n";
	evaluationConfigOutputFile.flush();
	evaluationConfigOutputFile.close();
	
	evaluationDataOutputFile.open(evaluationDataPath.str(), std::ios::app);
	evaluationDataOutputFile << "Iterations;Error Pos;Error Rot;Error;Time [us];Time/Iteration [us];";
	evaluationDataOutputFile << "Iterations Min;Error Pos Min;Error Rot Min;Error Min;Time [us] Min;Time/Iteration [us] Min;";
	evaluationDataOutputFile << "Iterations Max;Error Pos Max;Error Rot Max;Error Max;Time [us] Max;Time/Iteration [us] Max;";
	evaluationDataOutputFile << "Reached [%];Stucked [%]\n";
	evaluationDataOutputFile.flush();
	
	log(Kore::Info, "start eval-logging!");
}

void Logger::endEvaluationLogger() {
	evaluationDataOutputFile.close();
	
	log(Kore::Info, "stop eval-logging!");
}

void Logger::saveData(Kore::vec3 rawPos, Kore::Quaternion rawRot, float scale) {
	// Save positional and rotation data
	if (scale != prevScale) {
		positionDataOutputFile << rawPos.x() << ";" << rawPos.y() << ";" << rawPos.z() << ";" << rawRot.x << ";" << rawRot.y << ";" << rawRot.z << ";" << rawRot.w << ";" << scale << "\n";
		prevScale = scale;
	} else
		positionDataOutputFile << rawPos.x() << ";" << rawPos.y() << ";" << rawPos.z() << ";" << rawRot.x << ";" << rawRot.y << ";" << rawRot.z << ";" << rawRot.w << "\n";
	positionDataOutputFile.flush();
}

void Logger::saveEvaluationData(Avatar *avatar) {
	float* iterations = avatar->getIterations();
	float* errorPos = avatar->getErrorPos();
	float* errorRot = avatar->getErrorRot();
	float* time = avatar->getTime();
	float* timeIteration = avatar->getTimeIteration();
	
	// Save datas
	for (int i = 0; i < 3; ++i) {
		float error = sqrtf(Square(*(errorPos + i)) + Square(*(errorRot + i)));
		
		evaluationDataOutputFile << *(iterations + i) << ";";
		evaluationDataOutputFile << *(errorPos + i) << ";";
		evaluationDataOutputFile << *(errorRot + i) << ";";
		evaluationDataOutputFile << error << ";";
		evaluationDataOutputFile << *(time + i) << ";";
		evaluationDataOutputFile << *(timeIteration + i) << ";";
	}
	evaluationDataOutputFile << avatar->getReached() << ";" << avatar->getStucked() << "\n";
	evaluationDataOutputFile.flush();
}

bool Logger::readLine(std::string str, Kore::vec3* rawPos, Kore::Quaternion* rawRot) {
	int column = 0;
	
	if (std::getline(positionDataInputFile, str, '\n')) {
		std::stringstream ss;
		ss.str(str);
		std::string item;
		
		while(std::getline(ss, item, ';')) {
			float num = std::stof(item);
			
			if (column == 0) rawPos->x() = num;
			else if (column == 1) rawPos->y() = num;
			else if (column == 2) rawPos->z() = num;
			else if (column == 3) rawRot->x = num;
			else if (column == 4) rawRot->y = num;
			else if (column == 5) rawRot->z = num;
			else if (column == 6) rawRot->w = num;
			
			if (currLineNumber == 1 && column == 7)
				scale = num;
			
			++column;
		}
		
		return true;
	}
	
	return false;
}

bool Logger::readData(int line, const int numOfEndEffectors, const char* filename, Kore::vec3* rawPos, Kore::Quaternion* rawRot) {
	std::string str;
	bool success = false;
	
	if (!positionDataInputFile.is_open())
		positionDataInputFile.open(filename);
	
	// Skip lines
	while(line > currLineNumber - 1) {
		std::getline(positionDataInputFile, str, '\n');
		++currLineNumber;
	}
	
	// Read line
	for (int i = 0; i < numOfEndEffectors; ++i) {
		Kore::vec3 pos = Kore::vec3(0, 0, 0);
		Kore::Quaternion rot = Kore::Quaternion(0, 0, 0, 1);
		
		success = readLine(str, &pos, &rot);
		if (success) {
			++currLineNumber;
			rawPos[i] = pos;
			rawRot[i] = rot;
		}
	}
	
	if (positionDataInputFile.eof()) {
		positionDataInputFile.close();
		currLineNumber = 0;
	}
	
	return success;
}
