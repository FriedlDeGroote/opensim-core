/* -------------------------------------------------------------------------- *
 *                         OpenSim:  testStorage.cpp                          *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2017 Stanford University and the Authors                *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0.         *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include <fstream>
#include <OpenSim/Common/Storage.h>
#include <OpenSim/Auxiliary/auxiliaryTestFunctions.h>

using namespace OpenSim;
using namespace std;


void testStorageLoadingFromFile(const std::string& fileName, const int ncols);

void testStorageLegacy() {
    // Create a storage from a std file "std_storage.sto"
    //ofstream checkRunDirFile("rundir.txt");
    //checkRunDirFile << "Run from here:\n\n";
    //checkRunDirFile.close();
    string stdLabels[] = { "time", "v1", "v2" };
    std::unique_ptr<Storage> st(new Storage("test.sto"));
    // time[\t]v1[\t]v2
    // 1.[\t]   10.0[Space]20
    // 2.[\t\t] 20.0[\t]40
    ASSERT(st->getSize() == 2);
    const Array<std::string> &lbls = st->getColumnLabels();
    ASSERT(lbls.getSize() == 3);
    int i = 0;
    for (i = 0; i<lbls.getSize(); i++) {
        ASSERT(lbls[i] == stdLabels[i]);
    }

    double val;
    for (i = 0; i<st->getSize(); i++) {
        StateVector& row = (*st->getStateVector(i));
        ASSERT(row.getTime() == i + 1);
        ASSERT(row.getData()[0] == row.getTime()*10.0);
        row.getDataValue(0, val);
        ASSERT(val == row.getTime()*10.0);
        ASSERT(row.getData()[0] == row.getTime()*10.0);
        ASSERT(row.getData()[1] == row.getTime()*20.0);
    }
    int ncol = st->getSmallestNumberOfStates();
    ASSERT(ncol == 2);
    Array<double> col(SimTK::CNT<SimTK::Real>::getNaN(), 4);
    st->getDataColumn(1, col);
    ASSERT(col[0] == 20.);
    ASSERT(col[1] == 40.0);

    ASSERT(st->getStateIndex("v2") == 1);

    Storage st2("testDiff.sto");
    // Test Comparison
    double diff = st->compareColumn(st2, stdLabels[1], 0.);
    ASSERT(fabs(diff) < 1E-7);
    diff = st->compareColumn(st2, stdLabels[2], 0.);
    ASSERT(fabs(diff) < 1E-7);

    // Loading version 2 storage file with Storage class.
    auto table = st->exportToTable();
    STOFileAdapter::write(table, "testStorage_version2.sto");
    {
        // Now read using Storage() constructor.
        Storage stVersion2("testStorage_version2.sto");
        // Compare with st.
        SimTK_TEST_EQ(table.getMatrix(),
            stVersion2.exportToTable().getMatrix());
    }

    // The version 2 storage file does not require nRows and nColumns
    // metadata (Issue #2120).
    {
        table.removeTableMetaDataKey("nRows");
        table.removeTableMetaDataKey("nColumns");
        STOFileAdapter::write(table,
            "testStorage_version2_short_header.sto");
        Storage stVersion2("testStorage_version2_short_header.sto");
        SimTK_TEST_EQ(table.getMatrix(),
            stVersion2.exportToTable().getMatrix());
    }
}

int main() {
    SimTK_START_TEST("testStorage");

        // Verify the loading scalar Outputs (2) from ..sto into a Storage
        SimTK_SUBTEST2(testStorageLoadingFromFile, "sampleOutputs.sto", 2+1);

        // Verify the loading Vec3 Outputs (2) from .sto into a Storage
        SimTK_SUBTEST2(testStorageLoadingFromFile, "sampleOutputsVec3.sto", 2*3+1);

        // Verify the loading SpatialVec Outputs (2) from .sto into a Storage
        SimTK_SUBTEST2(testStorageLoadingFromFile, "sampleOutputsSpatialVec.sto", 2*6+1);

        // Verify the loading of marker data (14) from .trc into a Storage
        SimTK_SUBTEST2(testStorageLoadingFromFile, "TRCFileWithNANs.trc", 43);

        // Verify the loading of forces from .c3d into a Storage. Includes 2
        // force-plates with force, point, moment vectors (Vec3 flattened)
        SimTK_SUBTEST2(testStorageLoadingFromFile, "walking2.c3d", 3*6+1);

        SimTK_SUBTEST(testStorageLegacy);
    SimTK_END_TEST();
}

void testStorageLoadingFromFile(const std::string& fileName, const int numCols)
{
    Storage storage{ fileName};
    // remove extension
    auto ix = fileName.rfind(".");
    if (ix == std::string::npos) {
        throw Exception("File name for Storage loading '" + fileName +
            "' must have a valid extension.");
    }
    std::string name = fileName.substr(0, ix-1);
    Array<std::string> labels = storage.getColumnLabels();

    storage.print("test_" + name + ".sto");

    ASSERT(numCols == labels.size());
}
