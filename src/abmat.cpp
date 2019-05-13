/*************************************************************

	LSD 7.1 - December 2018
	written by Marco Valente, Universita' dell'Aquila
	and by Marcelo Pereira, University of Campinas

	This file: Frederik Schaff, Ruhr-University Bochum

	This file: Frederik Schaff, Ruhr-University Bochum
	LSD is distributed under the GNU General Public License

 *************************************************************/

/****************************************************************************************
    ABMAT.CPP
	Some tools for automated summary analysis of simulation runs.

    Similar to the blueprint objects there exist abmat objects.
    Each object corresponds to a variable with the same name in the real model.
    There are 4 basic kinds of abmat objects / statistics:

      Micro : This implies a variable number of objects with this variable.
        Distributions statistics are saved for each timestep.
      Macro : This implies a unique (single) object with this variable.
        Distribution statistics are saved at the end of the timestep.
        Additionally, time-series statistics are saved.
      Comp  : A set of two macro variables. The subject and its correspondance.
        Associative statistics are taken (Distances, Correlation, Association)
      Cond  : A set of two micro variables. The subject and a factorial
        indicator (that basically subsets the micro variables). Distribution
        statistics are saved for each timestep for ich subset.

    Macro and Comp variables need to survive the complete simulation.
    Micro and Cond variables may "die"

    Technically the following tree structure is given:

    abmat --|-- *micro --|--- micro_var_1
            |            |--- micro_var_*
            |
            |-- *macro --|--- macro_var_1
            |            |--- macro_var_*
            |
            |-- *comp  --|--- subject --|--- corr_sub_1 (hooked to corr_assoc_1)
            |            |              |--- corr_sub_*
            |            |
            |            |--- associate --|-- corr_ass_1 (hooked to corr_sub_1)
            |            |                |-- corr_ass_*
            |
            |-- *cond  --|--- subject --|--- cond_sub_1 (hooked to cond_assoc_1)
            |            |              |--- cond_sub_*
            |            |
            |            |--- associate --|-- cond_ass_1 (hooked to cond_sub_1)
            |            |                |-- cond_ass_*

    Basically shadow objects are created for each variable. This comes at the
        cost of some overhead, but allows to handle the statistical variables
        as regular LSD variables w.r.t. the saving of the data and also
        reflection in LSD internal analysis tool.


    The overhead of adding the macro variables is justified by the fact that
    the user can in principle (and may have reason to) modify past values of
    any LSD variable. Thus we completely separate the statistical items from
    the model itself.

****************************************************************************************/
#include "decl.h"
#ifdef CPP11

/********************************************
    ABMAT_STATS
    Produce advanced distribution statistics

    We allow only variable names that are short enough, in total max 31 chars
    Elements are:
    [1..6] variable name
    -- in case of a conditional subsetting
    [3] conditional indicator "_c_"
    [1..6] variable 2 name
    [1] "=" (condition)
    [3] 3-char number (the value of the conditional variable)
    -- in case of comparring two variables
    [3] comparison indicator "_v_"
    [1..6] variable 2 name
    --
    [4] stat type (cross-section timeseries)
    [3] "_In" the interval number. The interval info is saved separately.
    [4] stat type (cross time)

    Thus there are 18 chars gone for the specifiers, leaving 13 for the variable names.
    Thus, we need to shorten the variables to 6 chars to allow the full potential.
    For simplicity, any variable longer than 6 chars is shortened to 3 chars and
    ammended with a unique number from 0 to 999, so we allow for maximum 1000
    variables.

    The information of this mapping is later storred in an elements table txt file
    of the format: shortname longname.

    e.g. "var1" + {"_c_" + "var2" + "=" "nnn" +} {"_v_" + "var2" +} "_min"
        + "_In" + "_min"
    to do so, we build a map of the real names for var1 and var2
    and translate them to shortened names (storred in a text file)


    For conditional variables there is an additional field "shr" for percentage
    share of subset in the total set.

    Note: Some usual LSD things, like the search, do not work here as there
    may be more than one object with the same name in one brotherhood chain.

********************************************/

//global variables
std::map <const char*, const char*> m_abmat_varnames; //map variable names to shortened ones.
int i_abmat_varnames; //simple counter for up to 3 digits

const char* lfirst = "first";
const char* lsecond = "second";

const char* abmat_varname_convert(const char* lab)
{
    //std::string s_lab = std::string(lab);
    //check if exists, else create
    if (m_abmat_varnames.count(lab) == 0 ) {
        std::string s_short = std::string(lab);
        if (s_short.length() > 6) {
            s_short.resize(3); //drop last chars
            s_short.append( std::to_string(i_abmat_varnames) );
            if (++i_abmat_varnames > 999) {
                sprintf( msg, "error in '%s'.", __func__);
                error_hard( msg, "too many variables to be shortened",
                            "If you need more than 1000, please contact the developer.",
                            true );
                return "";
            }
        }
        m_abmat_varnames[lab] = s_short.c_str();
    }
    return m_abmat_varnames.at(lab);
}



/********************************************
    ABMAT_STATS
    Produce advanced distribution statistics
********************************************/


m_statsT abmat_stats(std::vector<double>& Data )
{
    m_statsT stats;

    stats["n"]; //number of items

    // O-Stats
    stats["min"];
    stats["p05"]; //lower
    stats["p25"]; //lower
    stats["p50"]; //interpolate
    stats["p75"]; //higher
    stats["p95"]; //higher
    stats["max"];

    // C-Stats
    stats["avg"];
    stats["sd"];
    stats["mae"];

    // L-Moments
    stats["Lcv"];
    stats["Lsk"];
    stats["Lku"];

    const int len_data = Data.size();
    const double rlen_data = static_cast<double>( len_data );

    if (len_data >= 1) {

        //O-Stats

        std::sort(Data.begin(), Data.end());
        stats["min"] = Data[0];
        stats["max"] = Data[len_data - 1];
        int index = static_cast<int>( (len_data / 4) );
        stats["p25"] = Data[index];
        index = static_cast<int>( std::ceil( rlen_data * 3.0 / 4.0 ) );
        stats["p75"] = Data[index];
        index = static_cast<int>( (len_data * 1 / 20) );
        stats["p05"] = Data[index];
        index = static_cast<int>( std::ceil( rlen_data * 19.0 / 20.0 ) );
        stats["p95"] = Data[index];

        if (len_data % 2 == 0) {
            index = static_cast<int>( len_data / 2 - 1 );
            stats["p50"] = (Data[index] + Data[index + 1]) / 2;
        }
        else {
            stats["p50"] = Data[ static_cast<int>( (len_data - 1) / 2 ) ];
        }

        //L-Moments and mean
        //Wang, Q. J. (1996): Direct Sample Estimators of L Moments.
        //In Water Resour. Res. 32 (12), pp. 3617–3619. DOI: 10.1029/96WR02675.
        //Fortran Routine and article unclear about casting. Here all real.
        //intermediates for the L-Moments
        double L1, L2, L3, L4, CL1, CL2, CL3, CR1, CR2, CR3;
        L1 = L2 = L3 = L4 = CL1 = CL2 = CL3 = CR1 = CR2 = CR3 = 0.0;

        //L1 == mean
        for (int i = 1; i <= len_data; i++) {
            double ri = static_cast<double>( i );
            CL1 = ri - 1;
            CL2 = CL1 * (ri - 1.0 - 1.0) / 2.0;
            CL3 = CL2 * (ri - 1.0 - 2.0) / 3.0;
            CR1 = (rlen_data - ri); // KLO:buggy! FS:??
            CR2 = CR1 * (rlen_data - ri - 1.0) / 2.0;
            CR3 = CR2 * (rlen_data - ri - 2.0) / 3.0;
            L1 += Data[i - 1];
            L2 += (CL1 - CR1) * Data[i - 1];
            L3 += (CL2 - 2.0 * CL1 * CR1 + CR2) * Data[i - 1];
            L4 += (CL3 - 3.0 * CL2 * CR1 + 3.0 * CL1 * CR2 - CR3) * Data[i - 1];
        }
        const double C1 = rlen_data; //just for readability
        double C2 = C1 * (rlen_data - 1.0) / 2.0;
        double C3 = C2 * (rlen_data - 2.0) / 3.0;
        double C4 = C3 * (rlen_data - 3.0) / 4.0;
        L1 = L1 / C1;
        stats["avg"] = L1;
        L2 = L2 / C2 / 2.0;
        L3 = L3 / C3 / 3.0;
        L4 = L4 / C4 / 4.0;

        stats["Lcv"] = (L1 == 0.0) ? 0.0 : L2 / L1; // L-cv
        stats["Lsk"] = (L2 == 0.0) ? 0.0 : L3 / L2;     // L-Skewness
        stats["Lku"] = (L2 == 0.0) ? 0.0 : L4 / L2;     // L-Kurtosis

        double MAE = 0.0;
        double SD = 0.0;
        for (int i = 0; i < len_data; i++) {
            MAE += std::abs(Data[i] - L1);
            SD += std::pow((Data[i] - L1), 2);
        }
        stats["mae"] = MAE / rlen_data;
        SD /= rlen_data;
        stats["sd"] = SD > 0 ? sqrt(SD) : 0.0;
    }
    else {
        for(auto& elem : stats) {
            elem.second = NAN;
        }
    }
    stats["n"] = rlen_data; //only one never NAN
    return stats;
}

/********************************************
    ABMAT_COMPARE
    Produce advanced comparative statistics
    between two variables
********************************************/


m_statsT abmat_compare(std::vector<double>& Data, std::vector<double>& Data2 )
{
    //checks

    if (Data.size() != Data2.size()) {
        sprintf( msg, "error in '%s'. ", __func__ );
        error_hard( msg, "Data sizes are different",
                    "Please contact the developer.",
                    true );
    }

    m_statsT compare;

    compare["n"]; //length of the timeseries

    //association, i.e. direction without magnitude
    compare["gamma"]; //gamme correlation
    compare["tauA"];
    compare["tauB"];
    compare["tauC"];

    //standard product moment correlation
    compare["corr"];

    //Differences L-Norms
    compare["L1"]; //difference in means
    compare["L2"]; //difference as RMSE

    const int len_data = Data.size();
    const double rlen_data = static_cast<double>( len_data );

    if (len_data >= 1) {

        //Norms possible

        if (len_data >= 2) {
            //other stuff possible
        }

    }
    else {
        for(auto& elem : compare) {
            elem.second = NAN;
        }
    }
    compare["n"] = rlen_data; //only one never NAN
    return compare;
}

bool abmat_linked_vars_exists_not(object* oFirst, const char* lVar1, const char* lVar2)
{
    //search in all existing abmat variables of current category for the link
    if ( oFirst == NULL ) {
        sprintf( msg, "error in '%s'. ", __func__ );
        error_hard( msg, "no oFirst NULL",
                    "Please contact the developer.",
                    true );
    }
    if (oFirst->b == NULL)
        return true; //no desc. objects yet.

    object* cur = oFirst->b->head;
    while (cur != NULL) {
        if ( cur->hook == NULL ) {
            sprintf( msg, "error in '%s'. ", __func__ );
            error_hard( msg, "Hook is null",
                        "Please contact the developer.",
                        true );
        }
        if ( strcmp(cur->label, lVar1) == 0 && strcmp(cur->hook->label, lVar2) == 0 ) {
            return false; //link exists
        }
        cur = cur->next;
    }

    return true;
}

/********************************************
    GET_ABMAT_VARNAME
    produce the varname as a function of
    - stattype
    - var1lab
    - stat
    - var2lab
    -
********************************************/

const char* get_abmat_varname(Tabmat stattype, const char* var1lab, const char* statname, const char* var2lab, const int condVal)
{
    std::string varname( abmat_varname_convert(var1lab) );
    switch (stattype) {
        case a_micro: //nothing to add
        case a_macro: //nothing to add
            break;
        case a_cond:
            varname.append("_c_");
            varname.append( abmat_varname_convert(var2lab) );
            varname.append("=");
            if (condVal < 0 || condVal > 999) {
                sprintf( msg, "error in '%s'.", __func__);
                error_hard( msg, "conditional value is wrong",
                            "Control that it is in 0..999!",
                            true );
                return "";
            }
            varname.append( std::to_string( condVal ) );
            break;
        case a_comp:
            varname.append("_v_");
            varname.append( abmat_varname_convert(var2lab) );
            break;
        default: //irrelevant
            sprintf( msg, "error in '%s'.", __func__);
            error_hard( msg, "defaulting should not happen",
                        "contact the developer.",
                        true );
    }
    varname.append("_");
    varname.append(statname);
    return varname.c_str();
}

/********************************************
    create_abmat_object
    Add the abmat objects for the variable varlab of type type. if type is
    cond or comp, var2lab is the reference variable (in the same object).

    issue: Map for objects (no more unique labs!)

    for macro and comp
********************************************/
void add_abmat_object(std::string abmat_type, char const* varlab, char const* var2lab)
{
    //check if the abmat object associated to the variable
    //exists. Note: It may be included in multiple categories
    //so we check on category level.

    //first check if the variable varLab exists in the model.
    if (root->search_var(root, varlab) == NULL) {
        sprintf( msg, "error in '%s'. Variable %s is not in the model.", __func__, varlab );
        error_hard( msg, "Wrong variable name",
                    "Check your code to prevent this error.",
                    true );
    }

    object* parent = NULL;
    const size_t s_typeLab = 10;
    char typeLab[s_typeLab];
    Tabmat type;
    if (abmat_type.find("micro") != std::string::npos ) {
        snprintf(typeLab, sizeof(char)*s_typeLab, "*micro");
        type = a_micro;
    }
    else if (abmat_type.find("macro") != std::string::npos ) {
        snprintf(typeLab, sizeof(char)*s_typeLab, "*macro");
        type = a_macro;
    }
    else if (abmat_type.find("cond") != std::string::npos ) {
        snprintf(typeLab, sizeof(char)*s_typeLab, "*cond");
        type = a_cond;
    }
    else if (abmat_type.find("comp") != std::string::npos ) {
        snprintf(typeLab, sizeof(char)*s_typeLab, "*comp");
        type = a_comp;
    }
    else {
        sprintf( msg, "error in '%s'. Type %s is not valid.", __func__, abmat_type.c_str() );
        error_hard( msg, "wrong type",
                    "Check your code to prevent this error.",
                    true );
        return;
    }

    //get the parent for the type, create if it exists not yet
    parent = abmat->search(typeLab);
    object* oVar = NULL;
    object* oVar2 = NULL;

    if (parent == NULL) {
        abmat->add_obj(typeLab, 1, false );
        parent = abmat->search(typeLab);
    }

    if (type == a_micro || type == a_macro) {
        //Add the variable as an object to the category
        oVar = parent->search(varlab);
        if (oVar == NULL) {
            parent->add_obj(varlab, 1, false);
            oVar = parent->search(varlab);
        }
    }
    else if (type == a_cond || type == a_comp) {
        //In case we have type cond or comp, we add the variables within the
        //respective subgroups first and second and hook them with each other.
        //We make sure that each unique combination is only created once.


        //get variable aka "first" and conditional aka "second"
        object* oFirst = parent->search(lfirst);
        if (oFirst == NULL ) {
            parent->add_obj(lfirst, 1, false );
            oFirst = parent->search(lfirst);
        }
        object* oSecond = parent->search(lsecond);
        if (oSecond == NULL) {
            parent->add_obj(lsecond, 1, false );
            oSecond = parent->search(lsecond);
        }

        //check if the unique link exists already
        //Careful: Search does not work here!
        if ( oFirst->b != NULL )
            oVar = oFirst->b->head; //first descending object

        if (oVar == NULL || abmat_linked_vars_exists_not(oFirst, varlab, var2lab) ) {
            oFirst->add_obj(varlab, 1, false);
            //move to last variable - search does not work!
            oVar = oFirst->b->head; //first variable
            while (oVar->next != NULL)
                oVar = oVar->next;

            oSecond->add_obj(var2lab, 1, false);
            oVar2 = oSecond->b->head;
            while (oVar2->next != NULL)
                oVar2 = oVar2->next;

            //link them
            oVar->hook = oVar2;
            oVar2->hook = oVar;
        }
    }

    // Add the variables
    switch (type) {
        case a_micro: {
                std::vector<double> dummy;
                auto stats_template = abmat_stats( dummy ); //retrieve map of stats
                //create a variable for each statistic
                for (auto& elem : stats_template) {
                    const char* varLab = get_abmat_varname ( type, varLab, elem.first);
                    oVar->add_var(varLab, -1, NULL, true); //no lags, no values, save
                    variable* var = oVar->search_var(oVar, varLab, true, true, oVar);
                    var->param = 0; //0 is parameter. Other fields are not used.
                }
            }
            break;

        case a_macro: {
                //a macro object holds a variable with the same name, maybe shortened.
                const char* varLab = get_abmat_varname(type, oVar->label); //name is same as original, shortened
                oVar->add_var(varLab, -1, NULL, true); //no lags, no values --> only data
                variable* var = oVar->search_var(oVar, varLab, true, true, oVar);
                var->param = 0; //0 is parameter. Other fields are not used.
            }
            break;


        case a_cond: {
                //the top objects for the unique pair variable and conditioning
                //variable exist. The rest is dynamically checked/produced.
            }
            break;

        case a_comp: {
                //the comparative ("versus")
                //to do!
                std::vector<double> dummy;
                auto stats_template = abmat_compare( dummy, dummy ); //retrieve map of stats
                //create a variable for each statistic
                for (auto& elem : stats_template) {
                    const char* varLab = get_abmat_varname ( type, varLab, elem.first, var2lab);
                    oVar->add_var(varLab, -1, NULL, true); //no lags, no values, save
                    variable* var = oVar->search_var(oVar, varLab, true, true, oVar);
                    var->param = 0; //0 is parameter. Other fields are not used.
                }
            }
            break;

    }

    //Add ,,,
}

void update_abmat_vars()
{

    //for type cond:
    //the cond variable holds a set of micro stats for each unique
    //value that ever existed in the conditioning variable
    //initially there are none. This is instead checked each time
    //data is saved.


    //2) create the second,

    //check conditioning variable:
    //a) All are implicit integer?!
    //b) All are in same object as conditional variable?


    //create a map of the conditioning values that currently exist

    //compare with parameter labels of conditioning variable
    //if it does not yet exist, add the variable with the value
    //and add all the new parameters to the conditional variable,
    //initialise the prior data to NAN

    //gather the stats for each subset and write them to the params

}

#endif
