#include <memory>
#include <cxxtest/TestSuite.h>

#include <FGFDMExec.h>
#include <models/FGExternalReactions.h>
#include <models/FGExternalForce.h>
#include "TestUtilities.h"

using namespace JSBSim;

// A force whose value is set from code, standing in for a host application
// that computes its own forces and hands them to JSBSim.
class HostForce : public FGExternalForce
{
public:
  static int liveCount;

  HostForce(FGFDMExec* fdmex, const FGColumnVector3& F)
    : FGExternalForce(fdmex), force(F)
  { ++liveCount; }

  ~HostForce() override { --liveCount; }

  const FGColumnVector3& GetBodyForces(void) override {
    vFn = force;
    return FGForce::GetBodyForces();
  }

private:
  FGColumnVector3 force;
};

int HostForce::liveCount = 0;

class FGExternalReactionsTest : public CxxTest::TestSuite
{
public:
  ~FGExternalReactionsTest() {
    // Avoid constructing `FGLogging` instances in a static instance of
    // FGExternalReactionsTest. This is due to the destruction of thread local
    // storage *before* the destruction of static globals.
    JSBSim::FGJSBBase::debug_lvl = 0;
  }

  void testPropertiesTiedWithoutLoad() {
    FGFDMExec fdmex;
    auto pm = fdmex.GetPropertyManager();

    // No aircraft has been loaded, so Load() has never been called, yet the
    // totals must be available to a host that adds forces from code.
    TS_ASSERT(pm->HasNode("forces/fbx-external-lbs"));
    TS_ASSERT(pm->HasNode("forces/fby-external-lbs"));
    TS_ASSERT(pm->HasNode("forces/fbz-external-lbs"));
    TS_ASSERT(pm->HasNode("moments/l-external-lbsft"));
    TS_ASSERT(pm->HasNode("moments/m-external-lbsft"));
    TS_ASSERT(pm->HasNode("moments/n-external-lbsft"));
    TS_ASSERT_EQUALS(pm->GetNode("forces/fbx-external-lbs")->getDoubleValue(), 0.0);
  }

  void testAddedForcesAreSummed() {
    FGFDMExec fdmex;
    auto er = fdmex.GetExternalReactions();

    er->Add(std::make_unique<HostForce>(&fdmex, FGColumnVector3(1.0, 2.0, 3.0)));
    er->Add(std::make_unique<HostForce>(&fdmex, FGColumnVector3(10.0, 20.0, 30.0)));

    TS_ASSERT(er->InitModel());
    er->Run(false);

    TS_ASSERT_EQUALS(er->GetForces(FGJSBBase::eX), 11.0);
    TS_ASSERT_EQUALS(er->GetForces(FGJSBBase::eY), 22.0);
    TS_ASSERT_EQUALS(er->GetForces(FGJSBBase::eZ), 33.0);

    // Both forces act at the CG, so there is no moment.
    TS_ASSERT_EQUALS(er->GetMoments(FGJSBBase::eX), 0.0);
    TS_ASSERT_EQUALS(er->GetMoments(FGJSBBase::eY), 0.0);
    TS_ASSERT_EQUALS(er->GetMoments(FGJSBBase::eZ), 0.0);

    auto pm = fdmex.GetPropertyManager();
    TS_ASSERT_EQUALS(pm->GetNode("forces/fbx-external-lbs")->getDoubleValue(), 11.0);
    TS_ASSERT_EQUALS(pm->GetNode("forces/fby-external-lbs")->getDoubleValue(), 22.0);
    TS_ASSERT_EQUALS(pm->GetNode("forces/fbz-external-lbs")->getDoubleValue(), 33.0);
  }

  void testAddedForceOffTheCGGivesAMoment() {
    FGFDMExec fdmex;
    auto er = fdmex.GetExternalReactions();

    // 100 lbs downward, 12 inches aft of the CG in the structural frame, which
    // is 1 foot behind the CG along the body X axis.
    auto force = std::make_unique<HostForce>(&fdmex, FGColumnVector3(0.0, 0.0, 100.0));
    force->SetLocation(12.0, 0.0, 0.0);
    er->Add(std::move(force));

    TS_ASSERT(er->InitModel());
    er->Run(false);

    TS_ASSERT_EQUALS(er->GetForces(FGJSBBase::eZ), 100.0);
    TS_ASSERT_DELTA(er->GetMoments(FGJSBBase::eX), 0.0, 1E-9);
    TS_ASSERT_DELTA(er->GetMoments(FGJSBBase::eY), 100.0, 1E-9);
    TS_ASSERT_DELTA(er->GetMoments(FGJSBBase::eZ), 0.0, 1E-9);

    auto pm = fdmex.GetPropertyManager();
    TS_ASSERT_DELTA(pm->GetNode("moments/m-external-lbsft")->getDoubleValue(), 100.0, 1E-9);
  }

  void testNullIsIgnored() {
    FGFDMExec fdmex;
    auto er = fdmex.GetExternalReactions();

    er->Add(nullptr);

    TS_ASSERT(er->InitModel());
    er->Run(false);

    TS_ASSERT_EQUALS(er->GetForces(FGJSBBase::eX), 0.0);
    TS_ASSERT_EQUALS(er->GetForces(FGJSBBase::eY), 0.0);
    TS_ASSERT_EQUALS(er->GetForces(FGJSBBase::eZ), 0.0);
  }

  void testCodeForcesAddToThoseLoadedFromXML() {
    FGFDMExec fdmex;
    auto er = fdmex.GetExternalReactions();
    auto pm = fdmex.GetPropertyManager();

    Element_ptr el = readFromXML("<?xml version=\"1.0\"?>"
                                 "<external_reactions>"
                                 "  <force name=\"pull\" frame=\"BODY\">"
                                 "    <location unit=\"IN\">"
                                 "      <x>0</x><y>0</y><z>0</z>"
                                 "    </location>"
                                 "    <direction>"
                                 "      <x>1</x><y>0</y><z>0</z>"
                                 "    </direction>"
                                 "  </force>"
                                 "</external_reactions>");
    TS_ASSERT(er->Load(el));
    pm->GetNode("external_reactions/pull/magnitude", true)->setDoubleValue(10.0);

    // A host adds its own force alongside the one declared in the aircraft.
    er->Add(std::make_unique<HostForce>(&fdmex, FGColumnVector3(0.0, 5.0, 0.0)));

    TS_ASSERT(er->InitModel());
    er->Run(false);

    TS_ASSERT_DELTA(er->GetForces(FGJSBBase::eX), 10.0, 1E-9);
    TS_ASSERT_DELTA(er->GetForces(FGJSBBase::eY), 5.0, 1E-9);
    TS_ASSERT_DELTA(er->GetForces(FGJSBBase::eZ), 0.0, 1E-9);
    TS_ASSERT_DELTA(pm->GetNode("forces/fbx-external-lbs")->getDoubleValue(), 10.0, 1E-9);
    TS_ASSERT_DELTA(pm->GetNode("forces/fby-external-lbs")->getDoubleValue(), 5.0, 1E-9);
  }

  void testAddedForceIsOwnedByTheModel() {
    HostForce::liveCount = 0;
    {
      FGFDMExec fdmex;
      auto er = fdmex.GetExternalReactions();
      er->Add(std::make_unique<HostForce>(&fdmex, FGColumnVector3(1.0, 0.0, 0.0)));
      TS_ASSERT_EQUALS(HostForce::liveCount, 1);
    }
    // The derived object is destroyed with the model, through the virtual
    // destructor of FGExternalForce.
    TS_ASSERT_EQUALS(HostForce::liveCount, 0);
  }
};
