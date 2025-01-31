/*---------------------------------------------------------------------------------*/
/*  NOMAD - Nonlinear Optimization by Mesh Adaptive Direct Search -                */
/*                                                                                 */
/*  NOMAD - Version 4 has been created and developed by                            */
/*                 Viviane Rochon Montplaisir  - Polytechnique Montreal            */
/*                 Christophe Tribes           - Polytechnique Montreal            */
/*                                                                                 */
/*  The copyright of NOMAD - version 4 is owned by                                 */
/*                 Charles Audet               - Polytechnique Montreal            */
/*                 Sebastien Le Digabel        - Polytechnique Montreal            */
/*                 Viviane Rochon Montplaisir  - Polytechnique Montreal            */
/*                 Christophe Tribes           - Polytechnique Montreal            */
/*                                                                                 */
/*  NOMAD 4 has been funded by Rio Tinto, Hydro-Québec, Huawei-Canada,             */
/*  NSERC (Natural Sciences and Engineering Research Council of Canada),           */
/*  InnovÉÉ (Innovation en Énergie Électrique) and IVADO (The Institute            */
/*  for Data Valorization)                                                         */
/*                                                                                 */
/*  NOMAD v3 was created and developed by Charles Audet, Sebastien Le Digabel,     */
/*  Christophe Tribes and Viviane Rochon Montplaisir and was funded by AFOSR       */
/*  and Exxon Mobil.                                                               */
/*                                                                                 */
/*  NOMAD v1 and v2 were created and developed by Mark Abramson, Charles Audet,    */
/*  Gilles Couture, and John E. Dennis Jr., and were funded by AFOSR and           */
/*  Exxon Mobil.                                                                   */
/*                                                                                 */
/*  Contact information:                                                           */
/*    Polytechnique Montreal - GERAD                                               */
/*    C.P. 6079, Succ. Centre-ville, Montreal (Quebec) H3C 3A7 Canada              */
/*    e-mail: nomad@gerad.ca                                                       */
/*                                                                                 */
/*  This program is free software: you can redistribute it and/or modify it        */
/*  under the terms of the GNU Lesser General Public License as published by       */
/*  the Free Software Foundation, either version 3 of the License, or (at your     */
/*  option) any later version.                                                     */
/*                                                                                 */
/*  This program is distributed in the hope that it will be useful, but WITHOUT    */
/*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or          */
/*  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License    */
/*  for more details.                                                              */
/*                                                                                 */
/*  You should have received a copy of the GNU Lesser General Public License       */
/*  along with this program. If not, see <http://www.gnu.org/licenses/>.           */
/*                                                                                 */
/*  You can find information on the NOMAD software at www.gerad.ca/nomad           */
/*---------------------------------------------------------------------------------*/

#include "../Param/DisplayParameters.hpp"
#include "../Util/fileutils.hpp"

/*----------------------------------------*/
/*         initializations (private)      */
/*----------------------------------------*/
void NOMAD::DisplayParameters::init()
{
    _typeName = "Display";

    try
    {
        #include "../Attribute/displayAttributesDefinition.hpp"
        registerAttributes( _definition );

        // Note: we cannot call checkAndComply() here, the default values
        // are not valid, for instance DIMENSION, X0, etc.

    }
    catch (NOMAD::Exception& e)
    {
        std::string errorMsg = "Attribute registration failed: ";
        errorMsg += e.what();
        throw NOMAD::Exception(__FILE__,__LINE__, errorMsg);
    }

}

/*----------------------------------------*/
/*            check the parameters        */
/*----------------------------------------*/
void NOMAD::DisplayParameters::checkAndComply(
                    const std::shared_ptr<NOMAD::RunParameters> &runParams,
                    const std::shared_ptr<NOMAD::PbParameters> &pbParams)
{

    checkInfo();

    if (!toBeChecked())
    {
        // Early out
        return;
    }
    
    if (getAttributeValueProtected<int>("DISPLAY_DEGREE",false) >= 3)
    {
        // High display degree. Force display all eval.
        setAttributeValue("DISPLAY_ALL_EVAL", true);
    }

    // If display all eval, force change to dependant parameters
    if(getAttributeValueProtected<bool>("DISPLAY_ALL_EVAL",false))
    {
        // Force display all eval.
        setAttributeValue("DISPLAY_INFEASIBLE", true);
        setAttributeValue("DISPLAY_UNSUCCESSFUL", true);
    }
    
    // Verify DISPLAY_HEADER is positive
    if (0 == getAttributeValueProtected<size_t>("DISPLAY_HEADER",false))
    {
        throw NOMAD::InvalidParameter(__FILE__,__LINE__, "DISPLAY_HEADER must be positive. To disable headers, set DISPLAY_HEADER to INF.");
    }

    // Pb params must be checked before accessing its value
    size_t n = pbParams->getAttributeValue<size_t>("DIMENSION");
    if (n == 0)
    {
        throw NOMAD::InvalidParameter(__FILE__,__LINE__, "Parameters check: DIMENSION must be positive" );
    }


    // SOL_FORMAT (internal)
    auto solFormat = getAttributeValueProtected<NOMAD::ArrayOfDouble>("SOL_FORMAT",false);
    if ( !solFormat.isDefined() )
    {
        solFormat.reset(n, NOMAD::DISPLAY_PRECISION_STD);
        setAttributeValue("SOL_FORMAT", solFormat);
    }

    if ( ! pbParams->isAttributeDefaultValue<NOMAD::ArrayOfDouble>("GRANULARITY") )
    {

        // Update SOL_FORMAT.
        // We want to remember the number of decimals in
        // GRANULARITY arguments, to be used later for formatting.
        //
        // Default is DISPLAY_PRECISION_STD.

        auto newSolFormat = setFormatFromGranularity( pbParams->getAttributeValue<NOMAD::ArrayOfDouble>("GRANULARITY") );
        setAttributeValue("SOL_FORMAT", newSolFormat);
    }

    // The default value is empty: set a basic display stats: BBE OBJ
    auto displayStats = getAttributeValueProtected<NOMAD::ArrayOfString>("DISPLAY_STATS",false);
    if ( displayStats.empty() )
    {
        NOMAD::ArrayOfString aos("BBE OBJ");
        setAttributeValue("DISPLAY_STATS", aos );
    }


    /*------------------------------------------------------*/
    /* Stats files                                          */
    /*------------------------------------------------------*/

    auto statsFileParam = getAttributeValueProtected<NOMAD::ArrayOfString>("STATS_FILE",false) ;
    std::string statsFileName;
    if (!statsFileParam.empty())
    {
        statsFileName = statsFileParam[0];
        if (statsFileParam.size() == 1)
        {
            // Default stats: BBE OBJ.
            statsFileParam.add("BBE");
            statsFileParam.add("OBJ");
        }
    }


    // Update stats file name
    auto addSeedToFileNames = runParams->getAttributeValue<bool>("ADD_SEED_TO_FILE_NAMES");
    auto problemDir = runParams->getAttributeValue<std::string>("PROBLEM_DIR");
    if (!statsFileName.empty())
    {
        auto seed = runParams->getAttributeValue<int>("SEED");
        NOMAD::completeFileName(statsFileName, problemDir, addSeedToFileNames, seed);
        statsFileParam.replace(0, statsFileName);
        setAttributeValue("STATS_FILE", statsFileParam);
    }

    auto evalStatsFileName = getAttributeValueProtected<std::string>("EVAL_STATS_FILE",false) ;
    if (!evalStatsFileName.empty() && evalStatsFileName.compare("-") != 0  )
    {
        auto seed = runParams->getAttributeValue<int>("SEED");
        NOMAD::completeFileName(evalStatsFileName, problemDir, addSeedToFileNames, seed);
        setAttributeValue("EVAL_STATS_FILE", evalStatsFileName);
    }

    /*------------------------------------------------------*/
    /* History file                                           */
    /*------------------------------------------------------*/
    auto historyFileName = getAttributeValueProtected<std::string>("HISTORY_FILE",false) ;
    if (!historyFileName.empty())
    {
        auto seed = runParams->getAttributeValue<int>("SEED");
        NOMAD::completeFileName(historyFileName, problemDir, addSeedToFileNames, seed);
        setAttributeValue("HISTORY_FILE", historyFileName);
    }

    /*------------------------------------------------------*/
    /* Solution file                                        */
    /*------------------------------------------------------*/
    auto solutionFileName = getAttributeValueProtected<std::string>("SOLUTION_FILE",false) ;
    if (!historyFileName.empty())
    {
        auto seed = runParams->getAttributeValue<int>("SEED");
        NOMAD::completeFileName(solutionFileName, problemDir, addSeedToFileNames, seed);
        setAttributeValue("SOLUTION_FILE", solutionFileName);
    }
    
    bool solutionFileFinal = getAttributeValueProtected<bool>("SOLUTION_FILE_FINAL",false) ;
    if(solutionFileName.empty() && solutionFileFinal)
    {
        throw NOMAD::InvalidParameter(__FILE__,__LINE__, "SOLUTION_FILE_FINAL must be enabled only with SOLUTION_FILE properly set.");
    }

    _toBeChecked = false;

}
// End checkAndComply()




NOMAD::ArrayOfDouble NOMAD::DisplayParameters::setFormatFromGranularity( const NOMAD::ArrayOfDouble & aod )
{
    size_t n = aod.size();
    NOMAD::ArrayOfDouble solFormat(n, NOMAD::DISPLAY_PRECISION_STD);

    // Use GRANULARITY as an ArrayOfDouble.
    size_t nbDecimals;
    for ( size_t i=0 ; i < n ; i++ )
    {
        if ( aod[i] > 0 )
        {
            nbDecimals = aod[i].nbDecimals( );
            solFormat.set(i, (double)nbDecimals);
        }
    }
    return solFormat;

}




