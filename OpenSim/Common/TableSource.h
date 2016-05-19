/* -------------------------------------------------------------------------- *
 *                            OpenSim:  TableSource.h                         *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2015 Stanford University and the Authors                *
 * Authors:                                                                   *
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

#ifndef OPENSIM_TABLE_SOURCE_H_
#define OPENSIM_TABLE_SOURCE_H_

#include "TimeSeriesTable.h"
#include "Component.h"

namespace OpenSim {

class TimeOutOfRange : public InvalidTimestamp {
public:
    TimeOutOfRange(const std::string& file,
                   size_t line,
                   const std::string& func,
                   double timestamp,
                   double minTimestamp,
                   double maxTimestamp) :
        InvalidTimestamp(file, line, func) {
        std::string msg = "min = " + std::to_string(minTimestamp);
        msg += " max = " + std::to_string(maxTimestamp);
        msg += " timestamp = " + std::to_string(timestamp);

        addMessage(msg);
    }
}; // class TimeOutOfRange


/** Component representing a source of data from a TimeSeriesTable_.

This Component has two outputs:
- A list output with one channel per column of the TimeSeriesTable_.
- A non-list output for a row of the TimeSeriesTable_.

Construct this Component by giving it a TimeSeriesTable_. Then use it by 
connecting its output to the Input of another Component that accepts compatible
type of Input. Make sure to populate the column-labels of the TimeSeriesTable_ 
before connecting this Component to the input of another Component.    

\tparam ET Type of each element of the TimeSeriesTable_ this Component 
           represents.                                                        */
template<typename ET>
class TableSource_ : public Component {
    OpenSim_DECLARE_CONCRETE_OBJECT_T(TableSource_, ET, Component);

public:
    /** Type of the TimeSeriesTable_ this Component will hold.                */
    typedef TimeSeriesTable_<ET> Table;
    /** Type of the 'row' Output of this Component.                           */
    typedef SimTK::Vector_<ET>   Vector;

    OpenSim_DECLARE_OUTPUT(all_columns, Vector, getRowAtTime, 
                           SimTK::Stage::Time);
    OpenSim_DECLARE_LIST_OUTPUT(column, ET, getColumnAtTime, 
                                SimTK::Stage::Time);

    TableSource_()                               = default;
    TableSource_(const TableSource_&)            = default;
    TableSource_(TableSource_&&)                 = default;
    TableSource_& operator=(const TableSource_&) = default;
    TableSource_& operator=(TableSource_&&)      = default;
    
    /** Construct the TableSource_ by giving it a TimeSeriesTable_ to hold.   */
    TableSource_(const Table& table) : 
        _table{table} 
    {}

    /// \name Get/Set underlying TimeSeriesTable_
    /// @{

    /** Get a read-only reference to the TimeSeriesTable_ this TableSource_ 
    currently holds.                                                          */
    const Table& getTable() const {
        return _table;
    }

    /** Replace the existing TimeSeriesTable_ this TableSource_ currently 
    holds.                                                                    

    \throws KeyNotFound If table provided does not have column-labels.        */
    void setTable(const Table& table) {
        _table = table;
        auto& columnOutput = updOutput("column");
        columnOutput.clearChannels();
        for(const auto& columnLabel : _table.getColumnLabels())
            columnOutput.addChannel(columnLabel);
    }

    /// @}

protected:
    /** Retrieve value of a column at a given time(implicit in the State 
    provided). Linear interpolation is performed if the TimeSeriesTable_ does
    not contain an entry for the time mentioned by the state.

    \throws EmptyTable If the TimeSeriesTable_ this TableSource_ holds is 
                       currently empty.
    \throws TimeOutOfRange If the time specified by the State is either less 
                           than the smallest timestamp or greater than the 
                           largest timestamp in the TimeSeriesTable_.
    \throws KeyNotFound If TimeSeriesTable_ does not have column-labels.      */
    ET getColumnAtTime(const SimTK::State& state, 
                       const std::string& columnLabel) const {
        OPENSIM_THROW_IF(_table.getNumRows() == 0, EmptyTable);
        const auto& timeCol = _table.getIndependentColumn();
        const auto time = state.getTime();
        OPENSIM_THROW_IF(time < timeCol.front() ||
                         time > timeCol.back(),
                         TimeOutOfRange, 
                         time, timeCol.front(), timeCol.back());

        const auto colInd = _table.getColumnIndex(columnLabel);
        auto lb = std::lower_bound(timeCol.begin(), timeCol.end(), time);
        if(lb == timeCol.begin())
            return _table.getMatrix().getElt(0, colInd);
        else if(lb == timeCol.end())
            return _table.getMatrix().getElt(timeCol.size() - 1, colInd);
        else if(*lb == time)
            return _table.getMatrix().getElt(lb - timeCol.begin(), colInd);
        else {
            auto prevTime = *(lb - 1);
            auto nextTime = *lb;
            auto prevElt = _table.getMatrix().getElt(lb - 1 - timeCol.begin(), 
                                                     colInd);
            auto nextElt = _table.getMatrix().getElt(lb - timeCol.begin(), 
                                                     colInd);
            auto elt = ((time - prevTime) / (nextTime - prevTime)) * 
                       (nextElt - prevElt) + prevElt;
            return elt;
        }
    }

    /** Retrieve a row of the TimeSeriesTable_ at a given time (specified by the
    state). Linear interpolation is performed if the TimeSeriesTable_ does not
    have an entry for the time mentioned by the state.

    \throws EmptyTable If the TimeSeriesTable_ this TableSource_ holds is 
                       currently empty.
    \throws TimeOutOfRange If the time specified by the State is either less 
                           than the smallest timestamp or greater than the 
                           largest timestamp in the TimeSeriesTable_.         */
    Vector getRowAtTime(const SimTK::State& state) const {
        OPENSIM_THROW_IF(_table.getNumRows() == 0, EmptyTable);
        const auto& timeCol = _table.getIndependentColumn();
        const auto time = state.getTime();
        OPENSIM_THROW_IF(time < timeCol.front() ||
                         time > timeCol.back(),
                         TimeOutOfRange, 
                         time, timeCol.front(), timeCol.back());

        auto lb = std::lower_bound(timeCol.begin(), timeCol.end(), time);
        if(lb == timeCol.begin())
            return _table.getRowAtIndex(0).getAsVector();
        else if(lb == timeCol.end())
            return _table.getRowAtIndex(timeCol.size() - 1).getAsVector();
        else if(*lb == time)
            return _table.getRowAtIndex(lb - timeCol.begin()).getAsVector();
        else {
            auto prevTime = *(lb - 1);
            auto nextTime = *lb;
            auto prevRow = _table.getRowAtIndex(lb - 1 - timeCol.begin());
            auto nextRow = _table.getRowAtIndex(lb - timeCol.begin());
            auto row = ((time - prevTime) / (nextTime - prevTime)) * 
                       (nextRow - prevRow) + prevRow;
            return row.getAsVector();
        }
    }

    void extendFinalizeFromProperties() override {
        Super::extendFinalizeFromProperties();
        auto& columnOutput = updOutput("column");
        for(const auto& columnLabel : _table.getColumnLabels())
            columnOutput.addChannel(columnLabel);
    }

private:
    Table _table;
}; // class TableSource_


/** This TableSource_ can hold a TimeSeriesTable_<SimTK::Real> and so its 
list-output 'column' will have channels of type SimTK::Real (double). Its other
output 'row' will be of type SimTK::Vector_<SimTK::Real>.                     */
typedef TableSource_<SimTK::Real> TableSource;

/** This TableSource_ can hold a TimeSeriesTable_<SimTK::Vec3> and so its 
list-output 'column' will have channels of type SimTK::Vec3. Its other output
'row' will be of type SimTK::Vector_<SimTK::Vec3>.                            */
typedef TableSource_<SimTK::Vec3> TableSourceVec3;

} // namespace Opensim

#endif // OPENSIM_TABLE_SOURCE_H_
