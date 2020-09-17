#ifndef __NOMAD400_SGTELIB_MODEL_FILTER_CACHE__
#define __NOMAD400_SGTELIB_MODEL_FILTER_CACHE__

#include "../../Algos/SgtelibModel/SgtelibModel.hpp"
#include "../../Output/OutputInfo.hpp"  // for OutputLevel

#include "../../nomad_nsbegin.hpp"

// Methods available for greedy selection
enum class FilterSelectionMethod
{
    METHOD_BEST = 0,            // Method 0: Select the best point
    METHOD_MOST_DISTANT,        // Method 1: Select the most distant point
    METHOD_BEST_MIN_DIST,       // Method 2: Select the best point but with a minimum distance to points already selected
    METHOD_BEST_GOOD_HMAX,      // Method 3: Select the best points with a good enough value of hmax
    METHOD_HIGHEST_ISOLATION,   // Method 4: Select point with highest isolation number
    METHOD_HIGHEST_DENSITY,     // Method 5: Select point with highest density number
    NB_METHODS
};

// Dictionary for outputs
const std::map<FilterSelectionMethod, std::string> FilterSelectionMethodDict =
{
    {FilterSelectionMethod::METHOD_BEST,                "Select the best point"},
    {FilterSelectionMethod::METHOD_MOST_DISTANT,        "Select the most distant point"},
    {FilterSelectionMethod::METHOD_BEST_MIN_DIST,       "Select the best point but with a minimum distance to points already selected"},
    {FilterSelectionMethod::METHOD_BEST_GOOD_HMAX,      "Select the best points with a good enough value of hmax"},
    {FilterSelectionMethod::METHOD_HIGHEST_ISOLATION,   "Select point with highest isolation number"},
    {FilterSelectionMethod::METHOD_HIGHEST_DENSITY,     "Select point with highest density number"}
};



class SgtelibModelFilterCache : public Step
{
private:
    const SgtelibModel*     _modelAlgo;
    const size_t            _nbCandidates;
    EvalPointSet            _oraclePoints;  // Out - points generated by selection methods
    OutputLevel             _displayLevel;

    // Vector of EvalPoints which have a Sgte eval
    std::vector<EvalPoint>  _cacheSgte;

    // Structures used for filtering computations

    // Objective function (prediction)
    std::vector<double> _f;
    // Aggregate constraint (prediction)
    std::vector<double> _h;
    // Feasibility value (max of cj)
    std::vector<double> _hmax;
    // Distance to main cache.
    std::vector<double> _DX;
    // Distance between each pair of points
    std::vector<std::vector<double>> _DSS;
    // Initial isolation distances
    std::vector<double> _distIsolation;

    // Values for greedy selection
    std::vector<bool>   _keep;
    std::vector<double> _DT;
    std::vector<double> _DTX;
    std::vector<int>    _nIsolation;
    std::vector<int>    _nDensity;
    double              _hmaxThreshold;


public:
    explicit SgtelibModelFilterCache(const SgtelibModel* modelAlgo,
                                     const size_t nbCandidates)
      : Step(modelAlgo),
        _modelAlgo(modelAlgo),
        _nbCandidates(nbCandidates),
        _oraclePoints(),
        _displayLevel(OutputLevel::LEVEL_INFO),
        _cacheSgte(0),
        _f(),
        _h(),
        _hmax(),
        _DX(),
        _DSS(),
        _distIsolation(),
        _keep(),
        _DT(),
        _DTX(),
        _nIsolation(),
        _nDensity(),
        _hmaxThreshold(-INF)
    {
        init();
    }

    virtual ~SgtelibModelFilterCache();

    // Get / Set
    EvalPointSet getOraclePoints() const { return _oraclePoints; }

private:
    void init();

    virtual void startImp() override;
    virtual bool runImp() override;
    virtual void endImp() override;

    void computeInitialValues();
    int applyMethod(FilterSelectionMethod method);
    void freeSpace();


};

#include "../../nomad_nsend.hpp"

#endif // __NOMAD400_SGTELIB_MODEL_FILTER_CACHE__
