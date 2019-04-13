
# Real-time body reconstruction and recognition in virtual reality using Vive Trackers and Controllers

This project implements an accurate full-body recognition approach for VR-based applications using HTC Vive Trackers and Controllers. Motion recognition is based on Hidden Markov Model. Currently, a Yoga Warrior I pose can be recognized with an accuracy of 88%. However, more models can be trained. Furthermore, we implemented an Inverse Kinematics solver to animate the motions of the avatar as smoothly, rapidly and as accurately as possible. Using a HTC Vive headset, two Vive Controllers and three Vive Trackers, attached to the hands and feet, it is possible to create immersive VR experiences, where the user is able to perceive the avatar as her/his own body. The user can see her/his avatar from the first-person perspective and in a virtual mirror.

This fork uses the body tracking framework to record and identify exercises through the Weka machine learning framework. The training and testing of models, as well as the communication between both frameworks, is handled by the [Weka Helper](https://github.com/riruroman/WekaHelper). Setup and usage instructions for this use case can be found further below.

## Requirements:
- node.js
- VisualStudio or Xcode

## Clone the project:
git clone --recursive git@github.com:CatCuddler/BodyTracking.git <br />
cd BodyTracking/BodyModel/Kore <br />
git checkout master <br />
git submodule update --init --recursive <br />
cd .. <br />

### Compile
node Kore/make --vr steamvr

#### Only for Mac User:
Currently Metal does not work. Use Opengl. <br />
node Kore/make -g opengl

### Open VS or XCode
Open VisualStudio or Xcode project in BodyModel/build <br />
Change to "Release" mode. <br />
Change working directory in Xcode: Edit Scheme -> Use custom working directory -> choose deployment directory


### Avatar Calibration
1. Strap one Vive Tracker on your left foot and another one on your right foot (above ankles)
2. Strap the third Vive tracker on your waist
3. Hold the Vive Controller in your hands ;)
4. Start the project. You will see an avatar, standing in a T-Pose.
5. Press the "grip button" to set the size of the avatar (you must look straight ahead)
6. Go to where the avatar stands and put your feet and hands in the same position.
7. Make sure that the green arrows for the controllers are pointing straight ahead.
8. Press the "menu button" to calibrate the avatar.

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/x4SS8_-XY38/0.jpg)](https://youtu.be/x4SS8_-XY38)

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/F6yFdpnhhoo/0.jpg)](https://youtu.be/F6yFdpnhhoo)

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/q-yKLtrTodA/0.jpg)](https://youtu.be/q-yKLtrTodA)


# Weka based motion recognition

## Setup

### Java

The Weka machine learning framework and the Weka Helper run within a Java Virtual Machine. Since the body Tracking Framework does not currently feature 64-bit support, live classification requires a 32-bit Java Development Kit. However, Oracle does not provide 32-bit JDKs for Java 9 and upwards. The Body Tracking Framework and Weka Helper have been tested with Zulu 10.3, a 32-bit JDK 10 distribution provided by [Azul](https://www.azul.com/downloads/zulu/).

After installing / unpacking the JDK, add the following environment variables to your system "Path" variable:

- (JDK installation directory) / bin
- (JDK installation directory) / bin / server

Afterwards, restart your computer.

### IDE

- Add (JDK installation directory)\include and (JDK installation directory)\include\win32 to the include directories of your compiler
(for Visual Studio 2017 [15.8.6], right-click on project in Solution Explorer -> Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories)
- Add (JDK installation directory)\lib\jvm.lib to the linker input files as additional dependencies
(for Visual Studio 2017 [15.8.6], right-click on project in Solution Explorer -> Properties -> Configuration Properties -> Linker -> Input -> Additional Dependencies)

## Usage

### Recording

All settings are made within the MachineLearningMotionRecognition.h file. To record new movement data, first set the "operatingMode" to "RecordMovements". You can also change the name of the recorded subject, the session ID, an optional file tag, and the names of the tasks that you want to record. The recording process is controlled through the following keys:

- [H] : set avatar size 
- [K] : Calibrate controllers and trackers (while subject is in T-Pose)
- [B] : Block all gamepad inputs, to prevent user from accidentally recalibrating (also blocked automatically while recording)
- [Numpad 0 - 9] : Start recording the previously defined tasks
- [Space] : Stop the recording currently in progress

The recorded sensor data for each task is saved in 
(Project Folder) \ MachineLearningMotionRecognition \ Recordings.

### Model training and testing
The [Weka Helper](https://github.com/riruroman/WekaHelper) is compatible with the data produced by the Body Tracking Framework, and can be used to train and evaluate models for different sensor subsets and classifiers.

### Live Classification
The Weka Helper can also be used to predict the currently performed exercise based on the incoming sensor data. After training a model and packaging the Weka Helper according to its documentation, place the jar and model files in (Project Folder) \ MachineLearningMotionRecognition \ Weka. The jar file should be named "WekaMotionRecognitionForCpp-1.0-jar-with-dependencies.jar", and the model "wekaModel.model". Afterwards, set the "operatingMode" within MachineLearningMotionRecognition.h to "RecognizeMovements". The classification process is started by pressing [Space] after the player avatar has been calibrated. 

When changing to another model, you need to make sure that the Weka Helper jar was compiled with the same settings. Also note that "processMovementData" method in MachineLearningMotionRecognition.cpp currently contains a list of allowed sensor positions for the live classifications, which needs to be modified if other sensors are to be used. It currently contains the positions rForeArm, lLeg, lForearm, rLeg, hip, lHand, rHand and head.