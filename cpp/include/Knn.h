//
// Created by Vivek Kumar Bagaria on 2/3/18.
//
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <thread>
#include <numeric>
#include <map>
#include <dlib/image_io.h>
#include <dlib/image_transforms.h>
#include "Points.h"
#include "Arms.h"
#include "UCB_dynamic.h"
#include <stdexcept>
#include "utils.h"

/*
 * Finds the nearest k neighbhours of pointsVectorLeft in pointsVectorRight
 */
template <class templatePoint>
class Knn{
public:
    std::vector<templatePoint> pointsVectorLeft;
    std::vector<templatePoint> pointsVectorRight;
    unsigned k; // Number of nearest neighbhours
    unsigned numberOfInitialPulls; // UCB Parameters
    float delta; // UCB Parameters
    unsigned int sampleSize;

    //Outputs
    std::unordered_map<unsigned long, unsigned long> finalNumberOfPulls;
    std::vector<unsigned long> finalSortedOrder;
    std::vector<ArmKNN<templatePoint>>nearestNeighbourIndex;
    std::vector<ArmKNN<templatePoint>> nearestNeighbourIndexBrute;
    std::vector< std::vector<long >> allNearestNeighbhourIds;
    float initTime;
    float runTime;
    std::string saveFolderPath;

    std::vector<float> avgNumberOfPulls; //Statistics
    bool leftEqualsRight = false; // True when left and right points are the same

    Knn( const std::vector<templatePoint> &pVecL, const std::vector<templatePoint> &pVecR,
         unsigned NumberOfNeighbours, unsigned noOfInitialPulls, float deltaAccuracy, unsigned int sSize,
         std::string sFolderPath) {
        pointsVectorLeft = pVecL;
        pointsVectorRight = pVecR;
        sampleSize = sSize;
        saveFolderPath = sFolderPath;
        initialiseKNN(NumberOfNeighbours, noOfInitialPulls,  deltaAccuracy );
    }

    Knn( const std::vector<templatePoint> &pVecL,
         unsigned NumberOfNeighbours, unsigned noOfInitialPulls, float deltaAccuracy , unsigned int sSize,
         std::string sFolderPath) {
        pointsVectorLeft = pVecL;
        pointsVectorRight = pVecL;
        leftEqualsRight = true;
        sampleSize = sSize;
        saveFolderPath = sFolderPath;
        initialiseKNN(NumberOfNeighbours, noOfInitialPulls,  deltaAccuracy );
    }



    void initialiseKNN(unsigned NumberOfNeighbhours, unsigned noOfInitialPulls, float deltaAccuracy ){
        k = NumberOfNeighbhours;
        numberOfInitialPulls = noOfInitialPulls;
        delta = deltaAccuracy;
        for (unsigned long i(0); i< pointsVectorLeft.size(); i++){
            avgNumberOfPulls.push_back(0);
        }

        nearestNeighbourIndex.reserve(pointsVectorLeft.size());
        nearestNeighbourIndexBrute.reserve(pointsVectorLeft.size());

    }

    void run(const std::vector<unsigned long> &indices){

        for (unsigned long i = 0; i < indices.size(); i++){
            unsigned long index = indices[i];

            std::vector<ArmKNN<templatePoint> > armsVec;
            for (unsigned long j(0); j < pointsVectorRight.size(); j++) {
                if ( (j == index) && (leftEqualsRight) ) // Skip the point itself
                    continue;
                ArmKNN<templatePoint> tmpArm(j, pointsVectorRight[j], pointsVectorLeft[index]);
                armsVec.push_back(tmpArm);
            }

            UCBDynamic<ArmKNN<templatePoint> > UCB1(armsVec, delta, k, 4*k, sampleSize);
            std::chrono::system_clock::time_point timeStart = std::chrono::system_clock::now();
            UCB1.initialise(numberOfInitialPulls);
            std::chrono::system_clock::time_point timeRunStart = std::chrono::system_clock::now();
            UCB1.runUCB(20000*pointsVectorRight.size());
            std::chrono::system_clock::time_point timeMABEnd = std::chrono::system_clock::now();
// Stats
//#define Brute
#ifdef Brute
//            std::cout << "Running Brute" << std::endl;
            UCB1.armsKeepFromArmsContainerBrute();
#endif
            std::chrono::system_clock::time_point timeBruteEnd = std::chrono::system_clock::now();

            initTime = std::chrono::duration_cast<std::chrono::milliseconds>
                    (timeRunStart - timeStart).count();
            runTime = std::chrono::duration_cast<std::chrono::milliseconds>
                    (timeMABEnd - timeRunStart).count();
            float bruteTime = std::chrono::duration_cast<std::chrono::milliseconds>
                    (timeBruteEnd - timeMABEnd).count();


            UCB1.storeExtraTopArms();
            avgNumberOfPulls[index] = UCB1.globalNumberOfPulls/UCB1.numberOfArms;
            if (index%1 == 0){
                std::cout << "index " << indices[i] << " Avg Pulls " <<  avgNumberOfPulls[index]
                          << " init time " << initTime << " ms"
                        << " run time " << runTime << " ms"
                        << " total mab time " << runTime +initTime<< " ms"
                        << " brute time " << bruteTime << " ms"
                          << std::endl;

            }
            nearestNeighbourIndex = UCB1.topKArms;
            finalSortedOrder = UCB1.finalSortedOrder;
            finalNumberOfPulls = UCB1.finalNumberOfPulls;
#ifdef Brute
            nearestNeighbourIndexBrute = UCB1.bruteBestArms();
#endif

            std::vector<long> ids;
            for (unsigned int j(0); j < k ; j++){
                ids.push_back(nearestNeighbourIndex[0].id);
            }
            allNearestNeighbhourIds.push_back(ids);
            saveAnswers(index);
        }
    }

    void run(){
        std::vector<unsigned long> indices(pointsVectorLeft.size());
        std::iota(indices.begin(), indices.end(), 0);
//        std::cout << "size of indices" << indices.size() << std::endl;
        run(indices);

    }



    std::vector<std::vector<ArmKNN<templatePoint>> > get(std::vector<unsigned long> indices){
        std::vector<std::vector<ArmKNN<templatePoint>> > answers;
        for (unsigned long i = 0; i< indices.size(); i++) {
            unsigned long index = indices[i];
            answers.push_back(nearestNeighbourIndex);
        }
        return answers;

    }


    void saveAnswers(unsigned long index ){
        //Variables
        std::string  n = std::to_string(pointsVectorLeft.size());
        std::string  d = std::to_string(nearestNeighbourIndex[0].dimension);
        std::string saveFilePath = saveFolderPath+"n_"+n+"_d_"+d+"_k_"+std::to_string(k)+"_index_"+std::to_string(index);
        std::ofstream saveFile;
        saveFile.open (saveFilePath, std::ofstream::out | std::ofstream::trunc);
        std::vector<ArmKNN<templatePoint>> topKArms = nearestNeighbourIndex;
#ifdef Brute
        std::vector<ArmKNN<templatePoint>> topKArmsBrute = nearestNeighbourIndexBrute;
#endif
        std::vector<float> topKArmsTrueMean(k*5);
        std::vector<float> topKArmsTrueMeanBrute(k*5);
        std::vector<int> topKArmsArgSort(k*5);
        std::iota(topKArmsArgSort.begin(), topKArmsArgSort.end(), 0);
        auto comparator = [&topKArmsTrueMean](int a, int b){ return topKArmsTrueMean[a] < topKArmsTrueMean[b]; };

        //Save single stats
        saveFile << "AveragePulls," << avgNumberOfPulls[index] << "\n";
        saveFile << "InitTime," << initTime << "\n";
        saveFile << "RunTime," << runTime << "\n";
        saveFile << "NumberOfInitialPulls," << numberOfInitialPulls << "\n";
        saveFile << "SampleSize," << sampleSize << "\n";
        saveFile << "n," << n << "\n";
        saveFile << "d," << d << "\n";
        saveFile << "k," << k << "\n";

        // Saving k+4k nearest neighbhours
        saveFile << "Answer," << std::endl;
        for (unsigned i = 0; i < k*5; i++) {
            saveFile << topKArms[i].id << ",";
#ifdef Brute
            topKArmsTrueMeanBrute[i] = topKArmsBrute[i].trueMean();
#endif
            topKArmsTrueMean[i] = topKArms[i].trueMean();
        }
#ifndef Brute
        saveFile << std::endl;

        // Saving the local true order of k+4k points
        std::sort(topKArmsArgSort.begin(), topKArmsArgSort.end(), comparator);
        saveFile << "Position";
        for (unsigned i = 0; i < k*5; i++) {
            saveFile <<  "," << topKArmsArgSort[i];
        }
        saveFile << std::endl;

        // Saving stats for all the arms
        saveFile << "AllPullsNumber";
        for (unsigned i = 0; i < pointsVectorRight.size(); i++) {
            saveFile <<  "," << finalNumberOfPulls[i];
        }
        saveFile << std::endl;

        saveFile << "AllPullsIndex";
        for (unsigned i = 0; i < pointsVectorRight.size(); i++) {
            saveFile <<  "," << finalSortedOrder[i];
        }
        saveFile << std::endl;
//        std::cout << "\nPoint number " << index  << " Av:" << avgNumberOfPulls[index] << "\n";

#endif
#ifdef Brute
        bool flag = true;
        unsigned badboy = 0;
        for (unsigned i = 0; i < k; i++) {
            if (topKArms[i].id!=topKArmsBrute[i].id)
                flag = false;
        }
//        std::cout << "index " << index << ": ";

        std::cout << "Verdict: " << flag << "\n";
        if (flag == false){
            std::cout << "\nUCB: " ;
            for (unsigned i = 0; i < 2*k; i++) {
                std::cout << topKArms[i].id << " ";
            }

            std::cout << "\nBrute: " ;
            for (unsigned i = 0; i < 2*k; i++) {
                std::cout << topKArmsBrute[i].id << " ";
            }

            for (unsigned i = 0; i < 2*k; i++) {
                std::cout << topKArmsTrueMean[i] << " ";
            }
            std::cout << "\nBrute: " ;
            for (unsigned i = 0; i < 2*k; i++) {
                std::cout << topKArmsTrueMeanBrute[i] << " ";
            }
        }
#endif
    }
};
