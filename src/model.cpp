//
// Created by fedor on 24/01/16.
//

#include "model.h"
#include <map>
#include <tuple>
#include <string.h>
#include <logging/easylogging++.h>
#include "measure.h"
#include "pdrh_config.h"
#include "box_factory.h"

using namespace std;
using namespace capd;

pdrh::type pdrh::model_type;
std::map<std::string, std::tuple<std::string, capd::interval, double>> pdrh::rv_map;
std::map<std::string, std::string> pdrh::rv_type_map;
std::map<std::string, std::map<capd::interval, capd::interval>> pdrh::dd_map;
std::map<std::string, capd::interval> pdrh::var_map;
std::map<std::string, capd::interval> pdrh::par_map;
std::map<std::string, capd::interval> pdrh::syn_map;
capd::interval pdrh::time;
std::vector<pdrh::mode> pdrh::modes;
std::vector<pdrh::state> pdrh::init;
std::vector<pdrh::state> pdrh::goal;

std::map<std::string, std::pair<capd::interval, capd::interval>> pdrh::distribution::uniform;
std::map<std::string, std::pair<capd::interval, capd::interval>> pdrh::distribution::normal;
std::map<std::string, std::pair<capd::interval, capd::interval>> pdrh::distribution::gamma;
std::map<std::string, capd::interval> pdrh::distribution::exp;

// adding a variable
void pdrh::push_var(std::string var, capd::interval domain)
{
    if(capd::intervals::width(domain) < 0)
    {
        std::ostringstream s;
        s << "invalid domain " << domain << " for variable \"" << var << "\"";
        throw std::invalid_argument(s.str());
    }
    if(pdrh::var_map.find(var) != pdrh::var_map.cend())
    {
        std::stringstream s;
        s << "multiple declaration of \"" << var << "\"";
        throw std::invalid_argument(s.str());
    }
    else
    {
        pdrh::var_map.insert(make_pair(var, domain));
    }
}

// adding time bounds
void pdrh::push_time_bounds(capd::interval domain)
{
    if(capd::intervals::width(domain) < 0)
    {
        std::ostringstream s;
        s << "invalid time domain " << domain;
        throw std::invalid_argument(s.str());
    }
    pdrh::time = domain;
}

// adding invariant
void pdrh::push_invt(pdrh::mode& m, pdrh::node* invt)
{
    m.invts.push_back(invt);
}

// adding mode
void pdrh::push_mode(pdrh::mode m)
{
    std::vector<std::string> extra_vars = pdrh::get_keys_diff(pdrh::var_map, m.flow_map);
    for(std::string var : extra_vars)
    {
        m.flow_map.insert(make_pair(var, pdrh::var_map[var]));
        m.odes.insert(make_pair(var, pdrh::push_terminal_node("0")));
        // adding this variable to the list of parameters if it is not there yet,
        // if it is not a continuous or discrete random variable and
        // if its domain is an interval of length greater than 0
        if(pdrh::par_map.find(var) == pdrh::par_map.cend() &&
                pdrh::rv_map.find(var) == pdrh::rv_map.cend() &&
                    pdrh::dd_map.find(var) == pdrh::dd_map.cend() &&
                        capd::intervals::width(pdrh::var_map[var]) > 0)
        {
            pdrh::par_map.insert(make_pair(var, pdrh::var_map[var]));
        }
    }
    pdrh::modes.push_back(m);
}

void pdrh::push_ode(pdrh::mode& m, std::string var, pdrh::node* ode)
{
    if(pdrh::var_map.find(var) != pdrh::var_map.cend())
    {
        if(m.flow_map.find(var) == m.flow_map.cend())
        {
            m.flow_map.insert(make_pair(var, pdrh::var_map[var]));
            m.odes.insert(make_pair(var, ode));
            // removing a variable from the parameter list if there is an ode defined for it
            if(pdrh::par_map.find(var) != pdrh::par_map.cend())
            {
                pdrh::par_map.erase(var);
            }
        }
        else
        {
            std::stringstream s;
            s << "ode for the variable \"" << var << "\" was already declared above";
            throw std::invalid_argument(s.str());
        }
    }
    else
    {
        std::stringstream s;
        s << "variable \"" << var << "\" appears in the flow but it was not declared";
        throw std::invalid_argument(s.str());
    }
}

void pdrh::push_reset(pdrh::mode& m, pdrh::mode::jump& j, std::string var, pdrh::node* expr)
{
    // implement error check
    j.reset.insert(make_pair(var, expr));
}

void pdrh::push_jump(pdrh::mode& m, mode::jump j)
{
    m.jumps.push_back(j);
}

void pdrh::push_init(std::vector<pdrh::state> s)
{
    pdrh::init = s;
}

void pdrh::push_goal(std::vector<pdrh::state> s)
{
    pdrh::goal = s;
}

void pdrh::push_syn_pair(std::string var, capd::interval e)
{
    pdrh::syn_map.insert(make_pair(var, e));
}

void pdrh::push_rv(std::string var, std::string pdf, capd::interval domain, double start)
{
    pdrh::rv_map.insert(make_pair(var, std::make_tuple(pdf, domain, start)));
}

void pdrh::push_rv_type(std::string var, std::string type)
{
    pdrh::rv_type_map.insert(make_pair(var, type));
}

void pdrh::push_dd(std::string var, std::map<capd::interval, capd::interval> m)
{
    pdrh::dd_map.insert(make_pair(var, m));
}

bool pdrh::var_exists(std::string var)
{
    return (pdrh::var_map.find(var) != pdrh::var_map.cend());
}

pdrh::mode* pdrh::get_mode(int id)
{
    for(int i = 0; i < pdrh::modes.size(); i++)
    {
        if(pdrh::modes.at(i).id == id)
        {
            return &pdrh::modes.at(i);
        }
    }
    return NULL;
}

std::vector<pdrh::mode*> pdrh::get_shortest_path(pdrh::mode* begin, pdrh::mode* end)
{
    // initializing the set of paths
    std::vector<std::vector<pdrh::mode*>> paths;
    std::vector<pdrh::mode*> path;
    // checking if the initial state is the end state
    if(begin == end)
    {
        // pushing the initial mode to the initial path
        path.push_back(begin);
        return path;
    }
    else
    {
        // pushing the initial mode to the initial path
        path.push_back(begin);
        // pushing the initial path to the set of paths
        paths.push_back(path);
        while(!paths.empty())
        {
            // getting the first path in the set of paths
            path = paths.front();
            paths.erase(paths.cbegin());
            // getting the mode in the path
            pdrh::mode* cur_mode = path.back();
            std::vector<pdrh::mode*> successors = pdrh::get_successors(cur_mode);
            // proceeding if the current mode has successors
            if(!successors.empty())
            {
                // checking if one of the successors is the end mode
                if (std::find(successors.cbegin(), successors.cend(), end) != successors.cend())
                {
                    path.push_back(end);
                    paths.clear();
                    return path;
                }
                else
                {
                    // iterating through the successors of the current mode
                    for (pdrh::mode *suc_mode : successors)
                    {
                        // checking if a successor does not appear in the current path
                        if (std::find(path.cbegin(), path.cend(), suc_mode) == path.cend())
                        {
                            std::vector<pdrh::mode*> tmp_path = path;
                            tmp_path.push_back(suc_mode);
                            paths.push_back(tmp_path);
                        }
                    }
                }
            }
        }
    }
    path.clear();
    return path;
}

std::vector<std::vector<pdrh::mode*>> pdrh::get_paths(pdrh::mode* begin, pdrh::mode* end, int path_length)
{
    // initializing the set of paths
    std::vector<std::vector<pdrh::mode*>> paths;
    std::vector<pdrh::mode*> path;
    path.push_back(begin);
    // initializing the stack
    std::vector<std::vector<pdrh::mode*>> stack;
    stack.push_back(path);
    while(!stack.empty())
    {
        // getting the first paths from the set of paths
        path = stack.front();
        stack.erase(stack.cbegin());
        // checking if the correct path of the required length is found
        if((path.back() == end) && (path.size() == path_length + 1))
        {
            paths.push_back(path);
        }
        // proceeding only if the length of the current path is smaller then the required length
        else if(path.size() < path_length + 1)
        {
            // getting the last mode in the path
            pdrh::mode* cur_mode = path.back();
            // getting the successors of the mode
            std::vector<pdrh::mode*> successors = pdrh::get_successors(cur_mode);
            for(pdrh::mode* suc_mode : successors)
            {
                // appending the successor the current paths
                std::vector<pdrh::mode*> new_path = path;
                new_path.push_back(suc_mode);
                // pushing the new path to the set of the paths
                stack.push_back(new_path);
            }
        }
    }
    return paths;
}

std::vector<std::vector<pdrh::mode*>> pdrh::get_all_paths(int path_length)
{
    std::vector<std::vector<pdrh::mode*>> res;
    for(pdrh::state i : pdrh::init)
    {
        for(pdrh::state g : pdrh::goal)
        {
            std::vector<std::vector<pdrh::mode*>> paths = pdrh::get_paths(pdrh::get_mode(i.id), pdrh::get_mode(g.id), path_length);
            res.insert(res.end(), paths.begin(), paths.end());
        }
    }
    return res;
}

std::vector<pdrh::mode*> pdrh::get_successors(pdrh::mode* m)
{
    std::vector<pdrh::mode*> res;
    for(pdrh::mode::jump j : m->jumps)
    {
        pdrh::mode *tmp = pdrh::get_mode(j.next_id);
        if(tmp != NULL)
        {
            res.push_back(tmp);
        }
        else
        {
            std::ostringstream s;
            s << "mode \"" << j.next_id << "\" is not defined but appears in the jump: " << pdrh::print_jump(j) << std::endl;
            throw std::invalid_argument(s.str());
        }
    }
    return res;
}

std::vector<pdrh::mode*> pdrh::get_init_modes()
{
    std::vector<pdrh::mode*> res;
    for(pdrh::state st : pdrh::init)
    {
        pdrh::mode *tmp = pdrh::get_mode(st.id);
        if(tmp != NULL)
        {
            res.push_back(tmp);
        }
        else
        {
            std::ostringstream s;
            s << "mode \"" << st.id << "\" is not defined but appears in the init" << std::endl;
            throw std::invalid_argument(s.str());
        }
    }
    return res;
}

std::vector<pdrh::mode*> pdrh::get_goal_modes()
{
    std::vector<pdrh::mode*> res;
    for(pdrh::state st : pdrh::goal)
    {
        pdrh::mode *tmp = pdrh::get_mode(st.id);
        if(tmp != NULL)
        {
            res.push_back(tmp);
        }
        else
        {
            std::ostringstream s;
            s << "mode \"" << st.id << "\" is not defined but appears in the goal" << std::endl;
            throw std::invalid_argument(s.str());
        }
    }
    return res;
}

std::vector<std::string> pdrh::get_keys_diff(std::map<std::string, capd::interval> left, std::map<std::string, capd::interval> right)
{
    std::vector<std::string> res;
    for(auto it = left.cbegin(); it != left.cend(); it++)
    {
        if(right.find(it->first) == right.cend())
        {
            res.push_back(it->first);
        }
    }
    return res;
}

std::string pdrh::model_to_string()
{
    std::stringstream out;
    out << "MODEL TYPE: " << pdrh::model_type << std::endl;
    out << "VARIABLES:" << std::endl;
    for(auto it = pdrh::var_map.cbegin(); it != pdrh::var_map.cend(); it++)
    {
        out << "|   " << it->first << " " << it->second << std::endl;
    }
    out << "CONTINUOUS RANDOM VARIABLES:" << std::endl;
    for(auto it = pdrh::rv_map.cbegin(); it != pdrh::rv_map.cend(); it++)
    {
        out << "|   pdf(" << it->first << ") = " << std::get<0>(it->second) << "  | " << std::get<1>(it->second) << " |   " << std::get<2>(it->second) << std::endl;
    }
    out << "DISCRETE RANDOM VARIABLES:" << std::endl;
    for(auto it = pdrh::dd_map.cbegin(); it != pdrh::dd_map.cend(); it++)
    {
        out << "|   dd(" << it->first << ") = (";
        for(auto it2 = it->second.cbegin(); it2 != it->second.cend(); it2++)
        {
            out << it2->first << " : " << it2->second << ", ";
        }
        out << ")" << std::endl;
    }
    out << "TIME DOMAIN:" << std::endl;
    out << "|   " << pdrh::time << std::endl;
    out << "MODES:" << std::endl;
    for(pdrh::mode m : pdrh::modes)
    {
        out << "|   MODE: " << m.id << ";" << std::endl;
        out << "|   INVARIANTS:" << std::endl;
        for(pdrh::node* n : m.invts)
        {
            out << "|   |   " << pdrh::node_to_string_prefix(n) << std::endl;
        }
        out << "|   FLOW_MAP:" << std::endl;
        for(auto it = m.flow_map.cbegin(); it != m.flow_map.cend(); it++)
        {
            out << "|   |   " << it->first << " " << it->second << std::endl;
        }
        out << "|   ODES:" << std::endl;
        for(auto it = m.odes.cbegin(); it != m.odes.cend(); it++)
        {
            out << "|   |   d[" << it->first << "]/dt = " << pdrh::node_to_string_prefix(it->second) << std::endl;
        }
        out << "|   JUMPS:" << std::endl;
        for(pdrh::mode::jump j : m.jumps)
        {
            out << "|   |   GUARD: " << pdrh::node_to_string_prefix(j.guard) << std::endl;
            out << "|   |   SUCCESSOR: " << j.next_id << std::endl;
            out << "|   |   RESETS:" << std::endl;
            for(auto it = j.reset.cbegin(); it != j.reset.cend(); it++)
            {
                out << "|   |   |   " << it->first << " := " << pdrh::node_to_string_prefix(it->second) << std::endl;
            }
        }
    }
    out << "INIT:" << std::endl;
    for(pdrh::state s : pdrh::init)
    {
        out << "|   MODE: " << s.id << std::endl;
        out << "|   PROPOSITION: " << pdrh::node_to_string_prefix(s.prop) << std::endl;
    }
    if(pdrh::goal.size() > 0)
    {
        out << "GOAL:" << std::endl;
        for(pdrh::state s : pdrh::goal)
        {
            out << "|   MODE: " << s.id << std::endl;
            out << "|   PROPOSITION: " << pdrh::node_to_string_prefix(s.prop) << std::endl;
        }
    }
    else
    {
        out << "SYNTHESIZE:" << std::endl;
        for(auto it = pdrh::syn_map.cbegin(); it != pdrh::syn_map.cend(); it++)
        {
            out << "|   " << it->first << " : " << it->second << std::endl;
        }
    }

    return out.str();
}

std::string pdrh::print_jump(mode::jump j)
{
    std::stringstream out;
    out << j.guard << " ==>  @" << j.next_id << std::endl;
    return out.str();
}

pdrh::node* pdrh::push_terminal_node(std::string value)
{
    pdrh::node* n;
    n = new pdrh::node;
    n->value = value;
    return n;
}

pdrh::node* pdrh::push_operation_node(std::string value, std::vector<pdrh::node*> operands)
{
    pdrh::node* n;
    n = new pdrh::node;
    n->value = value;
    n->operands = operands;
    return n;
}

std::string pdrh::node_to_string_prefix(pdrh::node* n)
{
    std::stringstream s;
    // checking whether n is an operation node
    if(n->operands.size() > 0)
    {
        s << "(" << n->value;
        for(pdrh::node* op : n->operands)
        {
            s << pdrh::node_to_string_prefix(op);
        }
        s << ")";
    }
    else
    {
        s  << " " << n->value;
    }
    return s.str();
}

std::string pdrh::node_fix_index(pdrh::node* n, int step, std::string index)
{
    std::stringstream s;
    // checking whether n is an operation node
    if(n->operands.size() > 0)
    {
        s << "(" << n->value;
        for(pdrh::node* op : n->operands)
        {
            s << pdrh::node_fix_index(op, step, index);
        }
        s << ")";
    }
    else
    {
        s  << " " << n->value;
        if(pdrh::var_exists(n->value))
        {
            s  << "_" << step << "_" << index;
        }
    }
    return s.str();
}

// NEED TO FIX
std::string pdrh::node_to_string_infix(pdrh::node* n)
{
    std::stringstream s;
    // checking whether n is an operation node
    if(n->operands.size() > 0)
    {
        s << "(";
        for(int i = 0; i < n->operands.size() - 1; i++)
        {
            s << pdrh::node_to_string_infix(n->operands.at(i));
            s << n->value;
        }
        s << pdrh::node_to_string_infix(n->operands.at(n->operands.size() - 1));
        s << ")";
    }
    else
    {
        s << n->value;
    }
    return s.str();
}

std::string pdrh::reach_to_smt2(std::vector<pdrh::mode*> path, std::vector<box> boxes)
{
    std::stringstream s;
    // setting logic
    s << "(set-logic QF_NRA_ODE)" << std::endl;
    // declaring variables and defining bounds
    for(auto it = pdrh::var_map.cbegin(); it != pdrh::var_map.cend(); it++)
    {
        s << "(declare-fun " << it->first << " () Real)" << std::endl;
        for(int i = 0; i < path.size(); i++)
        {
            s << "(declare-fun " << it->first << "_" << i << "_0 () Real)" << std::endl;
            s << "(declare-fun " << it->first << "_" << i << "_t () Real)" << std::endl;
            if(it->second.leftBound() != -std::numeric_limits<double>::infinity())
            {
                s << "(assert (>= " << it->first << "_" << i << "_0 " << it->second.leftBound() << "))" << std::endl;
                s << "(assert (>= " << it->first << "_" << i << "_t " << it->second.leftBound() << "))" << std::endl;
            }
            if(it->second.rightBound() != std::numeric_limits<double>::infinity())
            {
                s << "(assert (<= " << it->first << "_" << i << "_0 " << it->second.rightBound() << "))" << std::endl;
                s << "(assert (<= " << it->first << "_" << i << "_t " << it->second.rightBound() << "))" << std::endl;
            }
        }
    }
    // declaring time
    for(int i = 0; i < path.size(); i++)
    {
        s << "(declare-fun time_" << i << " () Real)" << std::endl;
        s << "(assert (>= time_" << i << " " << pdrh::time.leftBound() << "))" << std::endl;
        s << "(assert (<= time_" << i << " " << pdrh::time.rightBound() << "))" << std::endl;
    }
    // defining odes
    for(auto path_it = path.cbegin(); path_it != path.cend(); path_it++)
    {
        if(std::find(path.cbegin(), path_it, *path_it) == path_it)
        {
            s << "(define-ode flow_" << (*path_it)->id << " (";
            for(auto ode_it = (*path_it)->odes.cbegin(); ode_it != (*path_it)->odes.cend(); ode_it++)
            {
                s << "(= d/dt[" << ode_it->first << "] " << pdrh::node_to_string_prefix(ode_it->second) << ")";
            }
            s << "))" << std::endl;
        }
    }
    // defining the reachability formula
    s << "(assert (and " << std::endl;
    // defining initial states
    s << "(or ";
    for(pdrh::state st : pdrh::init)
    {
        if(path.front()->id == st.id)
        {
            s << pdrh::node_fix_index(st.prop, 0, "0");
        }
    }
    s << ")" << std::endl;
    // defining boxes bounds
    for(box b : boxes)
    {
        std::map<std::string, capd::interval> m = b.get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining trajectory
    int step = 0;
    for(pdrh::mode* m : path)
    {
        // defining integrals
        s << "(= [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_t ";
        }
        s << "] (integral 0.0 time_" << step << " [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_0 ";
        }
        s << "] flow_" << m->id << "))" << std::endl;
        // defining invariants
        for(pdrh::node* invt : m->invts)
        {
            s << "(forall_t " << m->id << " [0.0 time_" << step << "] " << pdrh::node_fix_index(invt, step, "t") << ")" << std::endl;
        }
        // checking the current depth
        if(step < path.size() - 1)
        {
            // defining jumps
            for (pdrh::mode::jump j : m->jumps)
            {
                s << pdrh::node_fix_index(j.guard, step, "t") << std::endl;
                for (auto reset_it = j.reset.cbegin(); reset_it != j.reset.cend(); reset_it++)
                {
                    s << "(= " << reset_it->first << "_" << step + 1 << "_0 " <<
                    pdrh::node_fix_index(reset_it->second, step, "t") << ")";
                }
            }
        }
        step++;
    }
    // defining goal
    s << "(or ";
    for(pdrh::state st : pdrh::goal)
    {
        if(path.back()->id == st.id)
        {
            s << pdrh::node_fix_index(st.prop, path.size() - 1, "t");
        }
    }
    s << ")))" << std::endl;
    // final statements
    s << "(check-sat)" << std::endl;
    s << "(exit)" << std::endl;
    return s.str();
}

std::string pdrh::reach_c_to_smt2(std::vector<mode*> path, std::vector<box> boxes)
{
    std::stringstream s;
    // setting logic
    s << "(set-logic QF_NRA_ODE)" << std::endl;
    // declaring variables and defining bounds
    for(auto it = pdrh::var_map.cbegin(); it != pdrh::var_map.cend(); it++)
    {
        s << "(declare-fun " << it->first << " () Real)" << std::endl;
        for(int i = 0; i < path.size(); i++)
        {
            s << "(declare-fun " << it->first << "_" << i << "_0 () Real)" << std::endl;
            s << "(declare-fun " << it->first << "_" << i << "_t () Real)" << std::endl;
            if(it->second.leftBound() != -std::numeric_limits<double>::infinity())
            {
                s << "(assert (>= " << it->first << "_" << i << "_0 " << it->second.leftBound() << "))" << std::endl;
                s << "(assert (>= " << it->first << "_" << i << "_t " << it->second.leftBound() << "))" << std::endl;
            }
            if(it->second.rightBound() != std::numeric_limits<double>::infinity())
            {
                s << "(assert (<= " << it->first << "_" << i << "_0 " << it->second.rightBound() << "))" << std::endl;
                s << "(assert (<= " << it->first << "_" << i << "_t " << it->second.rightBound() << "))" << std::endl;
            }
        }
    }
    // declaring time
    for(int i = 0; i < path.size(); i++)
    {
        s << "(declare-fun time_" << i << " () Real)" << std::endl;
        s << "(assert (>= time_" << i << " " << pdrh::time.leftBound() << "))" << std::endl;
        s << "(assert (<= time_" << i << " " << pdrh::time.rightBound() << "))" << std::endl;
    }
    // defining odes
    for(auto path_it = path.cbegin(); path_it != path.cend(); path_it++)
    {
        if(std::find(path.cbegin(), path_it, *path_it) == path_it)
        {
            s << "(define-ode flow_" << (*path_it)->id << " (";
            for(auto ode_it = (*path_it)->odes.cbegin(); ode_it != (*path_it)->odes.cend(); ode_it++)
            {
                s << "(= d/dt[" << ode_it->first << "] " << pdrh::node_to_string_prefix(ode_it->second) << ")";
            }
            s << "))" << std::endl;
        }
    }
    // defining the negated reachability formula
    s << "(assert (and (and " << std::endl;
    // defining initial states
    s << "(or ";
    for(pdrh::state st : pdrh::init)
    {
        if(path.front()->id == st.id)
        {
            s << pdrh::node_fix_index(st.prop, 0, "0");
        }
    }
    s << ")" << std::endl;
    // defining boxes bounds
    for(box b : boxes)
    {
        std::map<std::string, capd::interval> m = b.get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining trajectory
    int step = 0;
    for(pdrh::mode* m : path)
    {
        // defining integrals
        s << "(= [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_t ";
        }
        s << "] (integral 0.0 time_" << step << " [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_0 ";
        }
        s << "] flow_" << m->id << "))" << std::endl;
        // defining invariants
        for(pdrh::node* invt : m->invts)
        {
            s << "(forall_t " << m->id << " [0.0 time_" << step << "] " << pdrh::node_fix_index(invt, step, "t") << ")" << std::endl;
        }
        // checking the current depth
        //if(step < path.size() - 1)
        //{
            // defining jumps
            for (pdrh::mode::jump j : m->jumps)
            {
                s << pdrh::node_fix_index(j.guard, step, "t") << std::endl;
                if(step < path.size() - 1)
                {
                    for (auto reset_it = j.reset.cbegin(); reset_it != j.reset.cend(); reset_it++)
                    {
                        s << "(= " << reset_it->first << "_" << step + 1 << "_0 " <<
                        pdrh::node_fix_index(reset_it->second, step, "t") << ")";
                    }
                }
            }
        //}
        step++;
    }
    s << ")";
    // defining goal
    s << "(and ";
    for(pdrh::state st : pdrh::goal)
    {
        if(path.back()->id == st.id)
        {
            s << "(forall_t " << st.id << " [0 time_" << path.size() - 1 << "] (not " << pdrh::node_fix_index(st.prop, path.size() - 1, "t") << "))";
        }
    }
    s << ")))" << std::endl;
    // final statements
    s << "(check-sat)" << std::endl;
    s << "(exit)" << std::endl;
    return s.str();
}

std::string pdrh::reach_to_smt2(std::vector<mode *> path, rv_box* b1, dd_box* b2, nd_box* b3)
{
    std::stringstream s;
    // setting logic
    s << "(set-logic QF_NRA_ODE)" << std::endl;
    // declaring variables and defining bounds
    for(auto it = pdrh::var_map.cbegin(); it != pdrh::var_map.cend(); it++)
    {
        s << "(declare-fun " << it->first << " () Real)" << std::endl;
        for(int i = 0; i < path.size(); i++)
        {
            s << "(declare-fun " << it->first << "_" << i << "_0 () Real)" << std::endl;
            s << "(declare-fun " << it->first << "_" << i << "_t () Real)" << std::endl;
            if(it->second.leftBound() != -std::numeric_limits<double>::infinity())
            {
                s << "(assert (>= " << it->first << "_" << i << "_0 " << it->second.leftBound() << "))" << std::endl;
                s << "(assert (>= " << it->first << "_" << i << "_t " << it->second.leftBound() << "))" << std::endl;
            }
            if(it->second.rightBound() != std::numeric_limits<double>::infinity())
            {
                s << "(assert (<= " << it->first << "_" << i << "_0 " << it->second.rightBound() << "))" << std::endl;
                s << "(assert (<= " << it->first << "_" << i << "_t " << it->second.rightBound() << "))" << std::endl;
            }
        }
    }
    // declaring time
    for(int i = 0; i < path.size(); i++)
    {
        s << "(declare-fun time_" << i << " () Real)" << std::endl;
        s << "(assert (>= time_" << i << " " << pdrh::time.leftBound() << "))" << std::endl;
        s << "(assert (<= time_" << i << " " << pdrh::time.rightBound() << "))" << std::endl;
    }
    // defining odes
    for(auto path_it = path.cbegin(); path_it != path.cend(); path_it++)
    {
        if(std::find(path.cbegin(), path_it, *path_it) == path_it)
        {
            s << "(define-ode flow_" << (*path_it)->id << " (";
            for(auto ode_it = (*path_it)->odes.cbegin(); ode_it != (*path_it)->odes.cend(); ode_it++)
            {
                s << "(= d/dt[" << ode_it->first << "] " << pdrh::node_to_string_prefix(ode_it->second) << ")";
            }
            s << "))" << std::endl;
        }
    }
    // defining the reachability formula
    s << "(assert (and " << std::endl;
    // defining initial states
    s << "(or ";
    for(pdrh::state st : pdrh::init)
    {
        if(path.front()->id == st.id)
        {
            s << pdrh::node_fix_index(st.prop, 0, "0");
        }
    }
    s << ")" << std::endl;
    // defining rv box
    if(b1 != NULL)
    {
        std::map<std::string, capd::interval> m = b1->get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining dd box
    if(b2 != NULL)
    {
        std::map<std::string, capd::interval> m = b2->get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining nd box
    if(b3 != NULL)
    {
        std::map<std::string, capd::interval> m = b3->get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining trajectory
    int step = 0;
    for(pdrh::mode* m : path)
    {
        // defining integrals
        s << "(= [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_t ";
        }
        s << "] (integral 0.0 time_" << step << " [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_0 ";
        }
        s << "] flow_" << m->id << "))" << std::endl;
        // defining invariants
        for(pdrh::node* invt : m->invts)
        {
            s << "(forall_t " << m->id << " [0.0 time_" << step << "] " << pdrh::node_fix_index(invt, step, "t") << ")" << std::endl;
        }
        // checking the current depth
        if(step < path.size() - 1)
        {
            // defining jumps
            for (pdrh::mode::jump j : m->jumps)
            {
                s << pdrh::node_fix_index(j.guard, step, "t") << std::endl;
                for (auto reset_it = j.reset.cbegin(); reset_it != j.reset.cend(); reset_it++)
                {
                    s << "(= " << reset_it->first << "_" << step + 1 << "_0 " <<
                    pdrh::node_fix_index(reset_it->second, step, "t") << ")";
                }
            }
        }
        step++;
    }
    // defining goal
    s << "(or ";
    for(pdrh::state st : pdrh::goal)
    {
        if(path.back()->id == st.id)
        {
            s << pdrh::node_fix_index(st.prop, path.size() - 1, "t");
        }
    }
    s << ")))" << std::endl;
    // final statements
    s << "(check-sat)" << std::endl;
    s << "(exit)" << std::endl;
    return s.str();
}

std::string pdrh::reach_c_to_smt2(std::vector<mode *> path, rv_box* b1, dd_box* b2, nd_box* b3)
{
    std::stringstream s;
    // setting logic
    s << "(set-logic QF_NRA_ODE)" << std::endl;
    // declaring variables and defining bounds
    for(auto it = pdrh::var_map.cbegin(); it != pdrh::var_map.cend(); it++)
    {
        s << "(declare-fun " << it->first << " () Real)" << std::endl;
        for(int i = 0; i < path.size(); i++)
        {
            s << "(declare-fun " << it->first << "_" << i << "_0 () Real)" << std::endl;
            s << "(declare-fun " << it->first << "_" << i << "_t () Real)" << std::endl;
            if(it->second.leftBound() != -std::numeric_limits<double>::infinity())
            {
                s << "(assert (>= " << it->first << "_" << i << "_0 " << it->second.leftBound() << "))" << std::endl;
                s << "(assert (>= " << it->first << "_" << i << "_t " << it->second.leftBound() << "))" << std::endl;
            }
            if(it->second.rightBound() != std::numeric_limits<double>::infinity())
            {
                s << "(assert (<= " << it->first << "_" << i << "_0 " << it->second.rightBound() << "))" << std::endl;
                s << "(assert (<= " << it->first << "_" << i << "_t " << it->second.rightBound() << "))" << std::endl;
            }
        }
    }
    // declaring time
    for(int i = 0; i < path.size(); i++)
    {
        s << "(declare-fun time_" << i << " () Real)" << std::endl;
        s << "(assert (>= time_" << i << " " << pdrh::time.leftBound() << "))" << std::endl;
        s << "(assert (<= time_" << i << " " << pdrh::time.rightBound() << "))" << std::endl;
    }
    // defining odes
    for(auto path_it = path.cbegin(); path_it != path.cend(); path_it++)
    {
        if(std::find(path.cbegin(), path_it, *path_it) == path_it)
        {
            s << "(define-ode flow_" << (*path_it)->id << " (";
            for(auto ode_it = (*path_it)->odes.cbegin(); ode_it != (*path_it)->odes.cend(); ode_it++)
            {
                s << "(= d/dt[" << ode_it->first << "] " << pdrh::node_to_string_prefix(ode_it->second) << ")";
            }
            s << "))" << std::endl;
        }
    }
    // defining the negated reachability formula
    s << "(assert (and (and " << std::endl;
    // defining initial states
    s << "(or ";
    for(pdrh::state st : pdrh::init)
    {
        if(path.front()->id == st.id)
        {
            s << pdrh::node_fix_index(st.prop, 0, "0");
        }
    }
    s << ")" << std::endl;
    // defining rv box
    if(b1 != NULL)
    {
        std::map<std::string, capd::interval> m = b1->get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining dd box
    if(b2 != NULL)
    {
        std::map<std::string, capd::interval> m = b2->get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining nd box
    if(b3 != NULL)
    {
        std::map<std::string, capd::interval> m = b3->get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining trajectory
    int step = 0;
    for(pdrh::mode* m : path)
    {
        // defining integrals
        s << "(= [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_t ";
        }
        s << "] (integral 0.0 time_" << step << " [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_0 ";
        }
        s << "] flow_" << m->id << "))" << std::endl;
        // defining invariants
        for(pdrh::node* invt : m->invts)
        {
            s << "(forall_t " << m->id << " [0.0 time_" << step << "] " << pdrh::node_fix_index(invt, step, "t") << ")" << std::endl;
        }
        // checking the current depth
        //if(step < path.size() - 1)
        //{
        // defining jumps
        for (pdrh::mode::jump j : m->jumps)
        {
            s << pdrh::node_fix_index(j.guard, step, "t") << std::endl;
            if(step < path.size() - 1)
            {
                for (auto reset_it = j.reset.cbegin(); reset_it != j.reset.cend(); reset_it++)
                {
                    s << "(= " << reset_it->first << "_" << step + 1 << "_0 " <<
                    pdrh::node_fix_index(reset_it->second, step, "t") << ")";
                }
            }
        }
        //}
        step++;
    }
    s << ")";
    // defining goal
    s << "(and ";
    for(pdrh::state st : pdrh::goal)
    {
        if(path.back()->id == st.id)
        {
            s << "(forall_t " << st.id << " [0 time_" << path.size() - 1 << "] (not " << pdrh::node_fix_index(st.prop, path.size() - 1, "t") << "))";
        }
    }
    s << ")))" << std::endl;
    // final statements
    s << "(check-sat)" << std::endl;
    s << "(exit)" << std::endl;
    return s.str();
}

std::string pdrh::reach_to_smt2(pdrh::state init, pdrh::state goal, std::vector<pdrh::mode *> path,
                                std::vector<box> boxes)
{
    std::stringstream s;
    // setting logic
    s << "(set-logic QF_NRA_ODE)" << std::endl;
    // declaring variables and defining bounds
    for(auto it = pdrh::var_map.cbegin(); it != pdrh::var_map.cend(); it++)
    {
        s << "(declare-fun " << it->first << " () Real)" << std::endl;
        for(int i = 0; i < path.size(); i++)
        {
            s << "(declare-fun " << it->first << "_" << i << "_0 () Real)" << std::endl;
            s << "(declare-fun " << it->first << "_" << i << "_t () Real)" << std::endl;
            if(it->second.leftBound() != -std::numeric_limits<double>::infinity())
            {
                s << "(assert (>= " << it->first << "_" << i << "_0 " << it->second.leftBound() << "))" << std::endl;
                s << "(assert (>= " << it->first << "_" << i << "_t " << it->second.leftBound() << "))" << std::endl;
            }
            if(it->second.rightBound() != std::numeric_limits<double>::infinity())
            {
                s << "(assert (<= " << it->first << "_" << i << "_0 " << it->second.rightBound() << "))" << std::endl;
                s << "(assert (<= " << it->first << "_" << i << "_t " << it->second.rightBound() << "))" << std::endl;
            }
        }
    }
    // declaring time
    for(int i = 0; i < path.size(); i++)
    {
        s << "(declare-fun time_" << i << " () Real)" << std::endl;
        s << "(assert (>= time_" << i << " " << pdrh::time.leftBound() << "))" << std::endl;
        s << "(assert (<= time_" << i << " " << pdrh::time.rightBound() << "))" << std::endl;
    }
    // defining odes
    for(auto path_it = path.cbegin(); path_it != path.cend(); path_it++)
    {
        if(std::find(path.cbegin(), path_it, *path_it) == path_it)
        {
            s << "(define-ode flow_" << (*path_it)->id << " (";
            for(auto ode_it = (*path_it)->odes.cbegin(); ode_it != (*path_it)->odes.cend(); ode_it++)
            {
                s << "(= d/dt[" << ode_it->first << "] " << pdrh::node_to_string_prefix(ode_it->second) << ")";
            }
            s << "))" << std::endl;
        }
    }
    // defining the reachability formula
    s << "(assert (and " << std::endl;
    // defining initial state
    s << pdrh::node_fix_index(init.prop, 0, "0") << std::endl;
    // defining boxes bounds
    for(box b : boxes)
    {
        std::map<std::string, capd::interval> m = b.get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining trajectory
    int step = 0;
    for(pdrh::mode* m : path)
    {
        // defining integrals
        s << "(= [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_t ";
        }
        s << "] (integral 0.0 time_" << step << " [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_0 ";
        }
        s << "] flow_" << m->id << "))" << std::endl;
        // defining invariants
        for(pdrh::node* invt : m->invts)
        {
            s << "(forall_t " << m->id << " [0.0 time_" << step << "] " << pdrh::node_fix_index(invt, step, "t") << ")" << std::endl;
        }
        // checking the current depth
        if(step < path.size() - 1)
        {
            // defining jumps
            for (pdrh::mode::jump j : m->jumps)
            {
                s << pdrh::node_fix_index(j.guard, step, "t") << std::endl;
                for (auto reset_it = j.reset.cbegin(); reset_it != j.reset.cend(); reset_it++)
                {
                    s << "(= " << reset_it->first << "_" << step + 1 << "_0 " <<
                    pdrh::node_fix_index(reset_it->second, step, "t") << ")";
                }
            }
        }
        step++;
    }
    // defining goal
    s << pdrh::node_fix_index(goal.prop, path.size() - 1, "t") << "))" << std::endl;
    // final statements
    s << "(check-sat)" << std::endl;
    s << "(exit)" << std::endl;
    return s.str();
}

std::string pdrh::reach_c_to_smt2(pdrh::state init, pdrh::state goal, std::vector<pdrh::mode *> path,
                                  std::vector<box> boxes)
{
    std::stringstream s;
    // setting logic
    s << "(set-logic QF_NRA_ODE)" << std::endl;
    // declaring variables and defining bounds
    for(auto it = pdrh::var_map.cbegin(); it != pdrh::var_map.cend(); it++)
    {
        s << "(declare-fun " << it->first << " () Real)" << std::endl;
        for(int i = 0; i < path.size(); i++)
        {
            s << "(declare-fun " << it->first << "_" << i << "_0 () Real)" << std::endl;
            s << "(declare-fun " << it->first << "_" << i << "_t () Real)" << std::endl;
            if(it->second.leftBound() != -std::numeric_limits<double>::infinity())
            {
                s << "(assert (>= " << it->first << "_" << i << "_0 " << it->second.leftBound() << "))" << std::endl;
                s << "(assert (>= " << it->first << "_" << i << "_t " << it->second.leftBound() << "))" << std::endl;
            }
            if(it->second.rightBound() != std::numeric_limits<double>::infinity())
            {
                s << "(assert (<= " << it->first << "_" << i << "_0 " << it->second.rightBound() << "))" << std::endl;
                s << "(assert (<= " << it->first << "_" << i << "_t " << it->second.rightBound() << "))" << std::endl;
            }
        }
    }
    // declaring time
    for(int i = 0; i < path.size(); i++)
    {
        s << "(declare-fun time_" << i << " () Real)" << std::endl;
        s << "(assert (>= time_" << i << " " << pdrh::time.leftBound() << "))" << std::endl;
        s << "(assert (<= time_" << i << " " << pdrh::time.rightBound() << "))" << std::endl;
    }
    // defining odes
    for(auto path_it = path.cbegin(); path_it != path.cend(); path_it++)
    {
        if(std::find(path.cbegin(), path_it, *path_it) == path_it)
        {
            s << "(define-ode flow_" << (*path_it)->id << " (";
            for(auto ode_it = (*path_it)->odes.cbegin(); ode_it != (*path_it)->odes.cend(); ode_it++)
            {
                s << "(= d/dt[" << ode_it->first << "] " << pdrh::node_to_string_prefix(ode_it->second) << ")";
            }
            s << "))" << std::endl;
        }
    }
    // defining the negated reachability formula
    s << "(assert (and (and " << std::endl;
    // defining the initial state
    s << pdrh::node_fix_index(init.prop, 0, "0") << std::endl;
    // defining boxes bounds
    for(box b : boxes)
    {
        std::map<std::string, capd::interval> m = b.get_map();
        for(auto it = m.cbegin(); it != m.cend(); it++)
        {
            s << "(>= " << it->first << "_0_0 " << it->second.leftBound() << ")" << std::endl;
            s << "(<= " << it->first << "_0_0 " << it->second.rightBound() << ")" << std::endl;
        }
    }
    // defining trajectory
    int step = 0;
    for(pdrh::mode* m : path)
    {
        // defining integrals
        s << "(= [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_t ";
        }
        s << "] (integral 0.0 time_" << step << " [";
        for(auto ode_it = m->odes.cbegin(); ode_it != m->odes.cend(); ode_it++)
        {
            s << ode_it->first << "_" << step << "_0 ";
        }
        s << "] flow_" << m->id << "))" << std::endl;
        // defining invariants
        for(pdrh::node* invt : m->invts)
        {
            s << "(forall_t " << m->id << " [0.0 time_" << step << "] " << pdrh::node_fix_index(invt, step, "t") << ")" << std::endl;
        }
        // checking the current depth
        //if(step < path.size() - 1)
        //{
        // defining jumps
        for (pdrh::mode::jump j : m->jumps)
        {
            s << pdrh::node_fix_index(j.guard, step, "t") << std::endl;
            if(step < path.size() - 1)
            {
                for (auto reset_it = j.reset.cbegin(); reset_it != j.reset.cend(); reset_it++)
                {
                    s << "(= " << reset_it->first << "_" << step + 1 << "_0 " <<
                    pdrh::node_fix_index(reset_it->second, step, "t") << ")";
                }
            }
        }
        //}
        step++;
    }
    s << ")";
    // defining goal
    s << "(forall_t " << goal.id << " [0 time_" << path.size() - 1 << "] (not " << pdrh::node_fix_index(goal.prop, path.size() - 1, "t") << "))";
    s << "))" << std::endl;
    // final statements
    s << "(check-sat)" << std::endl;
    s << "(exit)" << std::endl;
    return s.str();
}

// domain of nondeterministic parameters
box pdrh::get_nondet_domain()
{
    std::map<std::string, capd::interval> m;
    for(auto it = pdrh::par_map.cbegin(); it != pdrh::par_map.cend(); it++)
    {
        m.insert(make_pair(it->first, it->second));
    }
    return box(m);
}

box pdrh::get_psy_domain()
{
    std::map<std::string, capd::interval> m;
    for(auto it = pdrh::syn_map.cbegin(); it != pdrh::syn_map.cend(); it++)
    {
        m.insert(make_pair(it->first, pdrh::var_map[it->first]));
    }
    return box(m);
}

void pdrh::push_psy_goal(int mode_id, box b)
{
    pdrh::state st;
    st.id = mode_id;
    std::map<std::string, capd::interval> m = b.get_map();
    std::vector<pdrh::node*> operands;
    for(auto it = m.cbegin(); it != m.cend(); it++)
    {
        pdrh::node* var = pdrh::push_terminal_node(it->first);
        std::stringstream s;
        s << it->second.leftBound();
        pdrh::node* left_bound = pdrh::push_terminal_node(s.str());
        s.str("");
        s << it->second.rightBound();
        pdrh::node* right_bound = pdrh::push_terminal_node(s.str());
        s.str("");
        std::vector<pdrh::node*> tmp;
        tmp.push_back(var);
        tmp.push_back(left_bound);
        pdrh::node* left_constraint = pdrh::push_operation_node(">=", tmp);
        operands.push_back(left_constraint);
        tmp.clear();
        tmp.push_back(var);
        tmp.push_back(right_bound);
        pdrh::node* right_constraint = pdrh::push_operation_node("<=", tmp);
        operands.push_back(right_constraint);
        tmp.clear();
    }
    st.prop = pdrh::push_operation_node("and", operands);
    pdrh::goal = std::vector<pdrh::state>{ st };
    // updating time bounds
    pdrh::var_map["tau"] = capd::interval(0, m["tau"].leftBound());
    pdrh::push_time_bounds(capd::interval(0, m["tau"].leftBound()));
}

void pdrh::push_psy_c_goal(int mode_id, box b)
{
    pdrh::state st;
    st.id = mode_id;
    std::map<std::string, capd::interval> m = b.get_map();
    std::vector<pdrh::node*> operands;
    for(auto it = m.cbegin(); it != m.cend(); it++)
    {
        // using everything except for time tau
        if(strcmp(it->first.c_str(), "tau") != 0)
        {
            pdrh::node *var = pdrh::push_terminal_node(it->first);
            std::stringstream s;
            s << it->second.leftBound();
            pdrh::node *left_bound = pdrh::push_terminal_node(s.str());
            s.str("");
            s << it->second.rightBound();
            pdrh::node *right_bound = pdrh::push_terminal_node(s.str());
            s.str("");
            std::vector<pdrh::node *> tmp;
            tmp.push_back(var);
            tmp.push_back(left_bound);
            pdrh::node *left_constraint = pdrh::push_operation_node("<", tmp);
            operands.push_back(left_constraint);
            tmp.clear();
            tmp.push_back(var);
            tmp.push_back(right_bound);
            pdrh::node *right_constraint = pdrh::push_operation_node(">", tmp);
            operands.push_back(right_constraint);
            tmp.clear();
        }
    }
    pdrh::node* or_node = pdrh::push_operation_node("or", operands);
    // adding time as a constraint
    std::stringstream s;
    s << m["tau"].leftBound();
    pdrh::node *left_time_bound = pdrh::push_operation_node("=", std::vector<pdrh::node*>{ pdrh::push_terminal_node("tau"), pdrh::push_terminal_node(s.str())});
    s.str("");
    //s << m["tau"].rightBound();
    //pdrh::node *right_time_bound = pdrh::push_operation_node("<=", std::vector<pdrh::node*>{ pdrh::push_terminal_node("tau"), pdrh::push_terminal_node(s.str())});
    //st.prop = pdrh::push_operation_node("and", std::vector<pdrh::node*>{left_time_bound, right_time_bound, or_node});
    st.prop = pdrh::push_operation_node("and", std::vector<pdrh::node*>{left_time_bound, or_node});
    pdrh::goal = std::vector<pdrh::state>{ st };
    // updating time bounds
    pdrh::var_map["tau"] = capd::interval(0, m["tau"].leftBound());
    pdrh::push_time_bounds(capd::interval(0, m["tau"].leftBound()));
}

// mode, step, box
std::vector<std::tuple<int, box>> pdrh::series_to_boxes(std::map<std::string, std::vector<capd::interval>> time_series)
{
    std::vector<std::tuple<int, box>> res;
    for(int i = 0; i < time_series.cbegin()->second.size(); i++)
    {
        std::map<std::string, capd::interval> m;
        for(auto it = time_series.cbegin(); it != time_series.cend(); it++)
        {
            if((strcmp(it->first.c_str(), "Mode") != 0) && (strcmp(it->first.c_str(), "Step") != 0))
            {
                if(strcmp(it->first.c_str(), "Time") == 0)
                {
                    m.insert(std::make_pair("tau", it->second.at(i)));
                }
                else
                {
                    m.insert(std::make_pair(it->first, it->second.at(i)));
                }
            }
        }
        res.push_back(std::make_tuple((int) time_series["Mode"].at(i).leftBound(), box(m)));
    }
    return res;
}

std::vector<pdrh::mode*> pdrh::get_psy_path(std::map<std::string, std::vector<capd::interval>> time_series)
{
    std::vector<pdrh::mode*> path;
    path.push_back(pdrh::get_mode(pdrh::init.front().id));
    for(int i = 1; i < time_series.cbegin()->second.size(); i++)
    {
        if(time_series["Mode"].at(i).leftBound() != time_series["Mode"].at(i - 1).leftBound())
        {
            path.push_back(pdrh::get_mode((int) time_series["Mode"].at(i).leftBound()));
        }
    }
    return path;
}

// throws exception in case if one of the terminal modes is not a number
interval pdrh::evaluate_node_value(node *expr)
{
    if(expr->operands.size() == 0)
    {
        return interval(expr->value, expr->value);
    }
    else if(expr->operands.size() > 2)
    {
        CLOG(ERROR, "model") << "The number of operands can't be greater than 2";
        exit(EXIT_FAILURE);
    }
    else
    {
        if(strcmp(expr->value.c_str(), "+") == 0)
        {
            if(expr->operands.size() == 1)
            {
                return evaluate_node_value(expr->operands.front());
            }
            else if(expr->operands.size() == 2)
            {
                return evaluate_node_value(expr->operands.front()) + evaluate_node_value(expr->operands.back());
            }
        }
        else if(strcmp(expr->value.c_str(), "-") == 0)
        {
            if(expr->operands.size() == 1)
            {
                return interval(-1.0) * evaluate_node_value(expr->operands.front());
            }
            else if(expr->operands.size() == 2)
            {
                return evaluate_node_value(expr->operands.front()) - evaluate_node_value(expr->operands.back());
            }
        }
        else if(strcmp(expr->value.c_str(), "*") == 0)
        {
            return evaluate_node_value(expr->operands.front()) * evaluate_node_value(expr->operands.back());
        }
        else if(strcmp(expr->value.c_str(), "/") == 0)
        {
            return evaluate_node_value(expr->operands.front()) / evaluate_node_value(expr->operands.back());
        }
        else if(strcmp(expr->value.c_str(), "^") == 0)
        {
            return intervals::power(evaluate_node_value(expr->operands.front()), evaluate_node_value(expr->operands.back()));
        }
        else if(strcmp(expr->value.c_str(), "sqrt") == 0)
        {
            return intervals::sqrt(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "abs") == 0)
        {
            return intervals::iabs(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "exp") == 0)
        {
            return intervals::exp(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "log") == 0)
        {
            return intervals::log(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "sin") == 0)
        {
            return intervals::sin(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "cos") == 0)
        {
            return intervals::cos(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "tan") == 0)
        {
            return intervals::tan(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "asin") == 0)
        {
            return intervals::asin(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "acos") == 0)
        {
            return intervals::acos(evaluate_node_value(expr->operands.front()));
        }
        else if(strcmp(expr->value.c_str(), "atan") == 0)
        {
            return intervals::atan(evaluate_node_value(expr->operands.front()));
        }
        else
        {
            CLOG(ERROR, "model") << "Unknown function \"" << expr->value << "\"";
            exit(EXIT_FAILURE);
        }
    }
}

void pdrh::distribution::push_uniform(std::string var, capd::interval a, capd::interval b)
{
    pdrh::distribution::uniform.insert(make_pair(var, std::make_pair(a, b)));
}

void pdrh::distribution::push_normal(std::string var, capd::interval mu, capd::interval sigma)
{
    pdrh::distribution::normal.insert(make_pair(var, std::make_pair(mu, sigma)));
}

void pdrh::distribution::push_gamma(std::string var, capd::interval a, capd::interval b)
{
    pdrh::distribution::gamma.insert(std::make_pair(var, std::make_pair(a, b)));
}

void pdrh::distribution::push_exp(std::string var, capd::interval lambda)
{
    pdrh::distribution::exp.insert(std::make_pair(var, lambda));
}