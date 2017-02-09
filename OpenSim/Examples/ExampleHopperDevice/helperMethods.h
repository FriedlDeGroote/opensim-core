#ifndef _helperMethods_h_
#define _helperMethods_h_
/* -------------------------------------------------------------------------- *
 *                         OpenSim:  helperMethods.h                          *
 * -------------------------------------------------------------------------- *
 * The OpenSim API is a toolkit for musculoskeletal modeling and simulation.  *
 * See http://opensim.stanford.edu and the NOTICE file for more information.  *
 * OpenSim is developed at Stanford University and supported by the US        *
 * National Institutes of Health (U54 GM072970, R24 HD065690) and by DARPA    *
 * through the Warrior Web program.                                           *
 *                                                                            *
 * Copyright (c) 2005-2017 Stanford University and the Authors                *
 * Author(s): Chris Dembia, Shrinidhi K. Lakshmikanth, Ajay Seth,             *
 *            Thomas Uchida                                                   *
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

/* Helper methods to take care of some mundane tasks. You don't need to add
anything in this file, but you should know what each of these methods does. */

#include <OpenSim/OpenSim.h>
#include <simbody/internal/Visualizer_InputListener.h>

namespace OpenSim {


//------------------------------------------------------------------------------
// Display the class name and absolute path name for each of the given
// Component's descendants (children, grandchildren, etc.).
//
// Examples:
//   showSubcomponentInfo(myComponent);         //show all descendant Components
//   showSubcomponentInfo<Joint>(myComponent);  //show Joint descendants only
//------------------------------------------------------------------------------
template <class C = Component>
inline void showSubcomponentInfo(const Component& comp);


//------------------------------------------------------------------------------
// Display the name of each output generated by a Component; include outputs
// generated by all the Component's descendants (children, grandchildren, etc.)
// by default (set includeDescendants=false to suppress).
//------------------------------------------------------------------------------
inline void showAllOutputs(const Component& comp, bool includeDescendants=true);


//------------------------------------------------------------------------------
// Simulate a model from an initial state. The user is repeatedly prompted to
// either begin simulating or quit. The provided state is updated (returns the
// state at the end of the simulation). Set saveStatesFile=true to save the
// states to a storage file.
//------------------------------------------------------------------------------
inline void simulate(Model& model,
                     SimTK::State& state,
                     bool simulateOnce,
                     bool saveStatesFile = false);


//------------------------------------------------------------------------------
// Build a testbed for testing the device before attaching it to the hopper. We
// will attach one end of the device to ground ("/testbed/ground") and the other
// end to a sprung load ("/testbed/load").
//------------------------------------------------------------------------------
inline Model buildTestbed(bool showVisualizer);


//------------------------------------------------------------------------------
// SignalGenerator is a type of Component with no inputs and only one output.
// This Component evaluates an OpenSim::Function (stored in its "function"
// property) as a function of time. We can use a SignalGenerator to design
// time-varying control inputs for testing the device.
//------------------------------------------------------------------------------
class SignalGenerator : public Component {
    OpenSim_DECLARE_CONCRETE_OBJECT(SignalGenerator, Component);

public:
    OpenSim_DECLARE_PROPERTY(function, Function,
        "Function used to generate the signal (a function of time)");
    OpenSim_DECLARE_OUTPUT(signal, double, getSignal, SimTK::Stage::Time);

    SignalGenerator() { constructProperties(); }

    double getSignal(const SimTK::State& s) const {
        return get_function().calcValue(SimTK::Vector(1, s.getTime())); }

private:
    void constructProperties() { constructProperty_function(Constant(0.)); }

}; // end of SignalGenerator


//==============================================================================
//                               IMPLEMENTATIONS
//==============================================================================
template <class C>
inline void showSubcomponentInfo(const Component& comp)
{
    using std::cout; using std::endl;

    std::string className = SimTK::NiceTypeName<C>::namestr();
    const std::size_t colonPos = className.rfind(":");
    if (colonPos != std::string::npos)
        className = className.substr(colonPos+1, className.length()-colonPos);

    cout << "Class name and absolute path name for descendants of '"
         << comp.getName() << "' that are of type " << className << ":\n"
         << endl;

    ComponentList<const C> compList = comp.getComponentList<C>();

    // Step through compList once to find the longest concrete class name.
    unsigned maxlen = 0;
    for (const C& thisComp : compList) {
        auto len = thisComp.getConcreteClassName().length();
        maxlen = std::max(maxlen, static_cast<unsigned>(len));
    }
    maxlen += 4; //padding

    // Step through compList again to print.
    for (const C& thisComp : compList) {
        const std::string thisClass = thisComp.getConcreteClassName();
        cout << std::string(maxlen-thisClass.length(), ' ') << "[" << thisClass
             << "]  " << thisComp.getAbsolutePathName() << endl;
    }
    cout << endl;
}

inline void showAllOutputs(const Component& comp, bool includeDescendants)
{
    using std::cout; using std::endl;

    // Do not display header for Components with no outputs.
    if (comp.getNumOutputs() > 0) {
        const std::string msg = "Outputs from " + comp.getAbsolutePathName();
        cout << msg << "\n" << std::string(msg.size(), '=') << endl;

        std::vector<std::string> outputNames = comp.getOutputNames();
        for (auto thisName : outputNames) { cout << "  " << thisName << endl; }
        cout << endl;
    }

    if (includeDescendants) {
        ComponentList<const Component> compList =
            comp.getComponentList<Component>();
        for (const Component& thisComp : compList) {
            // compList (comp's ComponentList) includes all descendants (i.e.,
            // children, grandchildren, etc.) so set includeDescendants=false
            // when calling on thisComp.
            showAllOutputs(thisComp, false);
        }
    }
}

inline void simulate(Model& model,
                     SimTK::State& state,
                     const bool simulateOnce,
                     const bool saveStatesFile) {
    SimTK::State initState = state;
    SimTK::Visualizer::InputSilo* silo;

    // Configure the visualizer.
    if (model.getUseVisualizer()) {
        SimTK::Visualizer& viz = model.updVisualizer().updSimbodyVisualizer();
        // We use the input silo to get key presses.
        silo = &model.updVisualizer().updInputSilo();

        SimTK::DecorativeText help("Press any key to start a new simulation; "
                                   "ESC to quit.");
        help.setIsScreenText(true);
        viz.addDecoration(SimTK::MobilizedBodyIndex(0), SimTK::Vec3(0), help);

        viz.setBackgroundType(viz.GroundAndSky).setShowSimTime(true);
        viz.drawFrameNow(state);
        std::cout << "A visualizer window has opened." << std::endl;
    }

    // Simulate until the user presses ESC (or enters 'q' if visualization has
    // been disabled).
    do {
        if (model.getUseVisualizer()) {
            // Get a key press.
            silo->clear(); // Ignore any previous key presses.
            unsigned key, modifiers;
            silo->waitForKeyHit(key, modifiers);
            if (key == SimTK::Visualizer::InputListener::KeyEsc) { break; }
        } else if (!simulateOnce) {
            std::cout << "Press <Enter> to begin simulating, or 'q' followed "
                      << "by <Enter> to quit . . . " << std::endl;
            if (std::cin.get() == 'q') { break; }
        }

        // Set up manager and simulate.
        state = initState;
        SimTK::RungeKuttaMersonIntegrator integrator(model.getSystem());
        Manager manager(model, integrator);
        state.setTime(0.0);
        manager.integrate(state, 5.0);

        // Save the states to a storage file (if requested).
        if (saveStatesFile) {
            manager.getStateStorage().print("hopperStates.sto");
        }
    } while(!simulateOnce);
}

inline Model buildTestbed(bool showVisualizer)
{
    using SimTK::Vec3;
    using SimTK::Inertia;

    // Create a new OpenSim model.
    auto testbed = Model();
    testbed.setName("testbed");
    if (showVisualizer)
        testbed.setUseVisualizer(true);
    testbed.setGravity(Vec3(0));

    // Create a 2500 kg load and add geometry for visualization.
    auto load = new Body("load", 2500., Vec3(0), Inertia(1.));
    auto sphere = new Sphere(0.02);
    sphere->setFrame(*load);
    sphere->setOpacity(0.5);
    sphere->setColor(SimTK::Blue);
    load->attachGeometry(sphere);
    testbed.addBody(load);

    // Attach the load to ground with a FreeJoint and set the location of the
    // load to (1,0,0).
    auto gndToLoad = new FreeJoint("gndToLoad", testbed.getGround(), *load);
    gndToLoad->updCoordinate(FreeJoint::Coord::TranslationX).setDefaultValue(1.0);
    testbed.addJoint(gndToLoad);

    // Add a spring between the ground's origin and the load.
    auto spring = new PointToPointSpring(
        testbed.getGround(), Vec3(0),   //frame G and location in G of point 1
        *load, Vec3(0),                 //frame F and location in F of point 2
        5000., 1.);                     //stiffness and rest length
    testbed.addForce(spring);

    return testbed;
}

} // end of namespace OpenSim

#endif // _helperMethods_h_
