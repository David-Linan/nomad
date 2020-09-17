#ifndef __NOMAD400_SGTELIB_MODEL__
#define __NOMAD400_SGTELIB_MODEL__

#include "../../Algos/AlgoStopReasons.hpp"
#include "../../Algos/Algorithm.hpp"
#include "../../Algos/EvcInterface.hpp"
#include "../../Type/SgtelibModelFeasibilityType.hpp"
#include "../../Type/SgtelibModelFormulationType.hpp"
#include "../../../ext/sgtelib/src/Surrogate.hpp"
#include "../../../ext/sgtelib/src/TrainingSet.hpp"

#include "../../nomad_nsbegin.hpp"

/// Class for implementation algorithms using Bastien Talgorn's sgtelib.
/**
 * When used as an algorithm by itself:
 * 1- Best points (with respect to blackbox evaluation) in the cache are found.
 *    - If the cache is empty, X0 points are used.
 * 2- These points are used to build a surrogate model.
 * 3- The model is optimized. This gives oracle points.
 * 4- The oracle points are evaluated by the blackbox.
 * 5- As long as new oracle points are found, the process is repeated.
 *
 * When used by Mads SearchMethod SgtelibSearchMethod:
 * - Steps 1, 2 and 3 are the same (X0 is never used: always the cache).
 * - The oracle points are send back to SgtelibSearchMethod, which takes care
 *   of projecting them to mesh and evaluate them.
 *
 * Reference: File Sgtelib_Model_Manager.cpp in NOMAD 3.9.1
 * Author: Bastien Talgorn
 */

class SgtelibModel: public Algorithm
{
private:
    // Barrier from upper step, if it exists
    std::shared_ptr<Barrier>                _barrierForX0s;

    std::shared_ptr<SGTELIB::TrainingSet>   _trainingSet;
    std::shared_ptr<SGTELIB::Surrogate>     _model;

    size_t _nbModels;   ///> The number of models. Depends on parameter SGTELIB_MODEL_FEASIBILITY.

    mutable bool _ready;
    bool _foundFeasible; ///> True if a feasible point has been found

    ArrayOfDouble _modelLowerBound; ///> Lower bound
    ArrayOfDouble _modelUpperBound; ///> Upper bound

    std::shared_ptr<MeshBase> _mesh; ///> Useful for sizes if a mesh is available.

public:
    /// Constructor
    explicit SgtelibModel(const Step* parentStep,
                          std::shared_ptr<AlgoStopReasons<ModelStopType>> stopReasons,
                          std::shared_ptr<Barrier> barrier,
                          const std::shared_ptr<RunParameters>& runParams,
                          const std::shared_ptr<PbParameters>& pbParams,
                          const std::shared_ptr<MeshBase>& mesh)
      : Algorithm(parentStep, stopReasons, runParams, pbParams),
        _barrierForX0s(barrier),
        _trainingSet(nullptr),
        _model(nullptr),
        _nbModels(0),
        _ready(false),
        _foundFeasible(false),
        _modelLowerBound(pbParams->getAttributeValue<size_t>("DIMENSION"), Double()),
        _modelUpperBound(pbParams->getAttributeValue<size_t>("DIMENSION"), Double()),
        _mesh(mesh)
    {
        init();
    }

    virtual ~SgtelibModel();

    // Get/Set
    // Return hMax from _barrierForX0s.
    // It is used for the sub-Mads initialization.
    Double getHMax() const { return _barrierForX0s->getHMax(); }

    std::shared_ptr<SGTELIB::TrainingSet> getTrainingSet() const { return _trainingSet; }
    std::shared_ptr<SGTELIB::Surrogate> getModel() const { return _model; }

    void setReady(const bool ready) { _ready = ready; }

    bool getFoundFeasible() const { return _foundFeasible; }
    void setFoundFeasible(const bool foundFeasible) { _foundFeasible = foundFeasible; }

    ArrayOfDouble getExtendedLowerBound() const;
    ArrayOfDouble getExtendedUpperBound() const;
    Double getFMin() const;
    const SgtelibModelFormulationType getFormulation() const;

    std::shared_ptr<MeshBase> getMesh() const { return _mesh; }
    Double getDeltaMNorm() const;


    // Utility function to get BB_OUTPUT_TYPE parameter, which is buried in Evaluator.
    static BBOutputTypeList getBBOutputType()
    {
        if (nullptr == EvcInterface::getEvaluatorControl()
            || nullptr == EvcInterface::getEvaluatorControl()->getEvalParams())
        {
            throw Exception(__FILE__, __LINE__, "Error in SgtelibModel::getBBOutputType()");
        }
        return EvcInterface::getEvaluatorControl()->getEvalParams()->getAttributeValue<BBOutputTypeList>("BB_OUTPUT_TYPE");
    }


    // Basic methods
    bool isReady() const;
    void update();
    void reset();
    void info();

    void checkHF(EvalPoint& x) const;

    static size_t getNbModels(const SgtelibModelFeasibilityType modelFeasibility,
                              const size_t nbConstraints);

    void setModelBounds(std::shared_ptr<SGTELIB::Matrix> X);

    void readInformationForHotRestart() override {}

    // Generate points that are interesting to evaluate,
    // based on the Sgtelib model.
    // Do not perform blackbox evaluation.
    // This method is used by SgtelibSearchMethod.
    EvalPointSet createOraclePoints();

    // Return X0s' from _barrierForX0s.
    // They are used for the sub-Mads initialization.
    std::vector<EvalPoint> getX0s() const;

private:
    void init();

    void startImp() override;
    bool runImp() override;
    void endImp() override;

};

#include "../../nomad_nsend.hpp"

#endif // __NOMAD400_SGTELIB_MODEL__

