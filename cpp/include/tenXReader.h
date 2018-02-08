//
// Created by Govinda Kamath on 2/7/18.
//

#ifndef COMBINATORIAL_MAB_TENXREADER_H
#define COMBINATORIAL_MAB_TENXREADER_H

#endif //COMBINATORIAL_MAB_TENXREADER_H
#include <unordered_map>
namespace tenXReader{
    void get10xMatrixSize(std::string fileName, std::vector<int> &sizeVect) ;
    void get10xMatrix(std::string fileName, std::vector<std::vector<int> > &denseDataMatrix) ;
    void get10xSparseMatrix(std::string fileName, std::vector<std::unordered_map<unsigned long, int> >
    &sparseDataMatrix);
}