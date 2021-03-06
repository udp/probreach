//
// Created by fedor on 26/02/16.
//

#include <unistd.h>
#include <logging/easylogging++.h>
#include <omp.h>
#include "decision_procedure.h"
#include "solver/dreal_wrapper.h"
#include "pdrh_config.h"

using namespace std;

int decision_procedure::evaluate(std::vector<pdrh::mode *> path, std::vector<box> boxes)
{
    // default value for the thread number
    int thread_num = 0;
    #ifdef _OPENMP
        thread_num = omp_get_thread_num();
    #endif
    // getting raw filename here
    string filename = string(global_config.model_filename);
    size_t ext_index = filename.find_last_of('.');
    string raw_filename = filename.substr(0, ext_index);
    // creating a name for the smt2 file
    stringstream f_stream;
    f_stream << raw_filename << "_" << path.size() - 1 << "_0_" << thread_num << ".smt2";
    string smt_filename = f_stream.str();
    // writing to the file
    ofstream smt_file;
    smt_file.open(smt_filename.c_str());
    smt_file << pdrh::reach_to_smt2(path, boxes);
    //cout << "FIRST FORMULA" << endl;
    //cout << pdrh::reach_to_smt2(path, boxes) << endl;
    smt_file.close();
    // calling dreal here
    int first_res = dreal::execute(global_config.solver_bin, smt_filename, global_config.solver_opt);
    if(first_res == -1)
    {
        return decision_procedure::ERROR;
    }
    else if(first_res == 1)
    {
        if((remove(smt_filename.c_str()) == 0) &&
                (remove(std::string(smt_filename + ".output").c_str()) == 0))
        {
            //LOG(DEBUG) << "Removed auxiliary files";
            return decision_procedure::UNSAT;
        }
        else
        {
            CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (UNSAT)";
            return decision_procedure::ERROR;
        }
    }
    else
    {
        // removing auxiliary files for the first formula
        if((remove(smt_filename.c_str()) != 0) ||
            (remove(std::string(smt_filename + ".output").c_str()) != 0))
        {
            CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (DELTA-SAT)";
            return decision_procedure::ERROR;
        }
        // going through the formulas psi_i_pi
        for(int i = 0; i < path.size(); i++)
        {
            // the complement formula
            f_stream.str("");
            f_stream << raw_filename << "_" << i << "_" << path.size() - 1 << "_0_" << thread_num << ".c.smt2";
            string smt_c_filename = f_stream.str();
            // writing to the file
            ofstream smt_c_file;
            smt_c_file.open(smt_c_filename.c_str());
            smt_c_file << pdrh::reach_c_to_smt2(i, path, boxes);
            cout << "SECOND FORMULA(" << i << "):" << endl;
            cout << pdrh::reach_c_to_smt2(i, path, boxes) << endl;
            smt_c_file.close();
            // calling dreal here
            int second_res = dreal::execute(global_config.solver_bin, smt_c_filename, global_config.solver_opt);
            //cout << "RESULT: " << second_res << endl;
            if(second_res == -1)
            {
                return decision_procedure::ERROR;
            }
            else if(second_res == 1)
            {
                if((remove(smt_c_filename.c_str()) != 0) ||
                    (remove(std::string(smt_c_filename + ".output").c_str()) != 0))
                {
                    CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (SAT)";
                    return decision_procedure::ERROR;
                }
            }
            else
            {
                if((remove(smt_c_filename.c_str()) != 0) ||
                    (remove(std::string(smt_c_filename + ".output").c_str()) != 0))
                {
                    CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (UNDET)";
                    return decision_procedure::ERROR;
                }
                else
                {
                    return decision_procedure::UNDET;
                }
            }
        }
        return decision_procedure::SAT;
    }
}

int decision_procedure::evaluate_delta_sat(std::vector<pdrh::mode *> path, std::vector<box> boxes)
{
    // default value for the thread number
    int thread_num = 0;
    #ifdef _OPENMP
        thread_num = omp_get_thread_num();
    #endif
    // getting raw filename here
    std::string filename = std::string(global_config.model_filename);
    size_t ext_index = filename.find_last_of('.');
    std::string raw_filename = filename.substr(0, ext_index);
    // creating a name for the smt2 file
    std::stringstream f_stream;
    f_stream << raw_filename << "_" << path.size() - 1 << "_0_" << thread_num << ".smt2";
    std::string smt_filename = f_stream.str();
    // writing to the file
    std::ofstream smt_file;
    smt_file.open(smt_filename.c_str());
    smt_file << pdrh::reach_to_smt2(path, boxes);
    smt_file.close();
    // calling dreal here
    int first_res = dreal::execute(global_config.solver_bin, smt_filename, global_config.solver_opt);

    if(first_res == -1)
    {
        return decision_procedure::ERROR;
    }
    else if(first_res == 1)
    {
        if((std::remove(smt_filename.c_str()) == 0) &&
           (std::remove(std::string(smt_filename + ".output").c_str()) == 0))
        {
            //LOG(DEBUG) << "Removed auxiliary files";
            return decision_procedure::UNSAT;
        }
        else
        {
            CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (UNSAT)";
            return decision_procedure::ERROR;
        }
    }
    else
    {
        if((std::remove(smt_filename.c_str()) == 0) &&
           (std::remove(std::string(smt_filename + ".output").c_str()) == 0))
        {
            //LOG(DEBUG) << "Removed auxiliary files";
            return decision_procedure::SAT;
        }
        else
        {
            CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (DELTA-SAT)";
            return decision_procedure::ERROR;
        }
    }
}

/*
int decision_procedure::synthesize(pdrh::state init, std::vector<pdrh::mode *> path, box psy_box, int mode_id, box goal_box)
{
    // default value for the thread number
    int thread_num = 0;
    #ifdef _OPENMP
        thread_num = omp_get_thread_num();
    #endif
    // getting raw filename here
    std::string filename = std::string(global_config.model_filename);
    size_t ext_index = filename.find_last_of('.');
    std::string raw_filename = filename.substr(0, ext_index);
    // creating a name for the smt2 file
    std::stringstream f_stream;
    f_stream << raw_filename << "_" << path.size() - 1 << "_0_" << thread_num << ".smt2";
    std::string smt_filename = f_stream.str();
    // writing to the file
    std::ofstream smt_file;
    smt_file.open(smt_filename.c_str());
    // pushing the goal here
    pdrh::push_psy_goal(mode_id, goal_box);
    // generating the reachability formula
    smt_file << pdrh::reach_to_smt2(path, std::vector<box>{psy_box});
    smt_file.close();
    // resetting the goal
    pdrh::goal.clear();
    // calling dreal here
    int first_res = dreal::execute(global_config.solver_bin, smt_filename, global_config.solver_opt);

    if(first_res == -1)
    {
        return decision_procedure::ERROR;
    }
    else if(first_res == 1)
    {
        if((std::remove(smt_filename.c_str()) == 0) &&
           (std::remove(std::string(smt_filename + ".output").c_str()) == 0))
        {
            //LOG(DEBUG) << "Removed auxiliary files";
            return decision_procedure::UNSAT;
        }
        else
        {
            CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (UNSAT)";
            return decision_procedure::ERROR;
        }
    }
    else
    {
        // the complement formula
        f_stream.str("");
        f_stream << raw_filename << "_" << path.size() - 1 << "_0_" << thread_num << ".c.smt2";
        std::string smt_c_filename = f_stream.str();
        // writing to the file
        std::ofstream smt_c_file;
        smt_c_file.open(smt_c_filename.c_str());
        // pushing the negated goal here
        pdrh::push_psy_c_goal(mode_id, goal_box);
        // generating the reachability formula
        smt_c_file << pdrh::reach_to_smt2(path, std::vector<box>{psy_box});
        smt_c_file.close();
        //resetting the goal
        pdrh::goal.clear();
        int second_res = dreal::execute(global_config.solver_bin, smt_c_filename, global_config.solver_opt);

        if(second_res == -1)
        {
            return decision_procedure::ERROR;
        }
        else if(second_res == 1)
        {
            if((std::remove(smt_c_filename.c_str()) == 0) &&
               (std::remove(std::string(smt_c_filename + ".output").c_str()) == 0) &&
               (std::remove(smt_filename.c_str()) == 0) &&
               (std::remove(std::string(smt_filename + ".output").c_str()) == 0))
            {
                //LOG(DEBUG) << "Removed auxiliary files";
                return decision_procedure::SAT;
            }
            else
            {
                CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (SAT)";
                return decision_procedure::ERROR;
            }
        }
        else
        {
            if((std::remove(smt_c_filename.c_str()) == 0) &&
               (std::remove(std::string(smt_c_filename + ".output").c_str()) == 0) &&
               (std::remove(smt_filename.c_str()) == 0) &&
               (std::remove(std::string(smt_filename + ".output").c_str()) == 0))
            {
                //LOG(DEBUG) << "Removed auxiliary files";
                return decision_procedure::UNDET;
            }
            else
            {
                CLOG(ERROR, "solver") << "Problem occurred while removing one of auxiliary files (UNDET)";
                return decision_procedure::ERROR;
            }
        }
    }
}
*/