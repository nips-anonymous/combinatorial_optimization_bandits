//
// Created by Vivek Kumar Bagaria on 2/11/18.
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
#include "../deprecated/UCB.h"
#include <stdexcept>
#include "utils.h"

#ifndef COMBINATORIAL_MAB_MEDOID_H
#define COMBINATORIAL_MAB_MEDOID_H



template <class templatePoint>
class Medoid{
public:
    std::vector<templatePoint> pointsVector;
    std::vector<ArmMedoid<templatePoint> > topMedoids;
    unsigned numberOfTopMedoids;

    unsigned numberOfInitialPulls; // UCB Parameters
    float delta; // UCB Parameters

    Medoid( const std::vector<templatePoint> &pVec, unsigned noOfTopMedoids, unsigned noOfInitialPulls, float deltaAccuracy ) {
        pointsVector = pVec;
        numberOfTopMedoids = noOfTopMedoids;
        numberOfInitialPulls = noOfInitialPulls;
        delta = deltaAccuracy;
    };

    void run(){
        unsigned numberOfPoints = pointsVector.size();
        std::vector<ArmMedoid<templatePoint> > armsVec;

        // Creating Arms from points
        for (unsigned i(0); i < numberOfPoints; i++) {
            ArmMedoid<templatePoint> tmpArm(i, pointsVector[i], pointsVector);
            armsVec.push_back(tmpArm);
        }

        std::cout << "Total points = " << numberOfPoints << " Dimension = " << armsVec[0].point->vecSize << std::endl;
        //UCB
        UCB<ArmMedoid<templatePoint> > UCB1(armsVec, delta, numberOfTopMedoids);

        // std::cout << "best arm = " << UCB1.bestArm().id <<std::endl;

        // UCB: Initialization
        std::cout << "UCB: Initializing with " << numberOfInitialPulls << " points" << std::endl;
        std::chrono::system_clock::time_point loopTimeStart = std::chrono::system_clock::now();
        UCB1.initialise(numberOfInitialPulls);
        std::chrono::system_clock::time_point loopTimeEnd = std::chrono::system_clock::now();
        std::cout<< "UCB: Finished initialising. ";
        std::cout << "Time taken (s) "
                  << std::chrono::duration_cast<std::chrono::seconds>(loopTimeEnd - loopTimeStart).count() << std::endl;

        std::cout << "UCB: Iterating" << std::endl;
        loopTimeStart = std::chrono::system_clock::now();
        UCB1.runUCB(100000*pointsVector.size());
        loopTimeEnd = std::chrono::system_clock::now();
        std::cout << "UCB: Iterating done. ";
        std::cout << "Time taken (s) "
                  << std::chrono::duration_cast<std::chrono::seconds>(loopTimeEnd - loopTimeStart).count() << std::endl;

        //Print Result
        std::cout << "Total number of pulls " << UCB1.globalNumberOfPulls << std::endl;
        std::cout << "Number of points " << UCB1.numberOfArms << std::endl;
//        std::cout << "Dimension of each point" << shapeData[0] << std::endl;
        std::cout << "Average number of paired distance evaluated (taking sparsity into account)" <<
                  UCB1.globalNumberOfPulls/(UCB1.numberOfArms*armsVec[0].point->vecSize*0.1) << std::endl;//0.1=sparsity
        std::cout << "Global Sigma = " << UCB1.globalSigma << std::endl;

        topMedoids = UCB1.topKArms;
    }

    void printOrder(){

        // True Means
        std::vector<float> topKArmsTrueMean(numberOfTopMedoids*3);
        std::vector<int> topKArmsArgSort(numberOfTopMedoids*3);
        std::cout << "Calculating true mean of top " << numberOfTopMedoids*3 << " medoids." << std::endl;
        for (unsigned i = 0; i < numberOfTopMedoids*3; i++)
            topKArmsTrueMean[i] = topMedoids[i].trueMean();

        std::iota(topKArmsArgSort.begin(), topKArmsArgSort.end(), 0);
        auto comparator = [&topKArmsTrueMean](int a, int b){ return topKArmsTrueMean[a] < topKArmsTrueMean[b]; };
        std::sort(topKArmsArgSort.begin(), topKArmsArgSort.end(), comparator);

        std::cout<<"Rank\tId\tTr Mean\t\tEs Mean\t\tLCB\t\t\tUCB\t\t\tNoP"<<std::endl;
        for (unsigned i = 0; i < numberOfTopMedoids*3; i++) {
            std::cout << std::setprecision (15) << topKArmsArgSort[i]+1
                      << "\t" << topMedoids[i].id
                      << "\t" << topKArmsTrueMean[i]
                      << "\t" << topMedoids[i].estimateOfMean
                      << "\t" << topMedoids[i].lowerConfidenceBound
                      << "\t" << topMedoids[i].upperConfidenceBound
                      << "\t" << topMedoids[i].numberOfPulls << std::endl;
        }

    }
};

#endif //COMBINATORIAL_MAB_MEDOID_H
