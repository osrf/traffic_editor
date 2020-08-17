#ifndef CROWD_SIM_IMPL__H
#define CROWD_SIM_IMPL__H

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

#include <yaml-cpp/yaml.h>

namespace crowd_sim {
class State;
class GoalSet;
class Condition;
class ConditionAND;
class ConditionOR;
class ConditionNOT;
class ConditionGOAL;
class ConditionTIMER;

using StatePtr = std::shared_ptr<State>;
using GoalSetPtr = std::shared_ptr<GoalSet>;
using ConditionPtr = std::shared_ptr<Condition>;
using ConditionAndPtr = std::shared_ptr<ConditionAND>;
using ConditionOrPtr = std::shared_ptr<ConditionOR>;
using ConditionNotPtr = std::shared_ptr<ConditionNOT>;
using ConditionGoalPtr = std::shared_ptr<ConditionGOAL>;
using ConditionTimerPtr = std::shared_ptr<ConditionTIMER>;

//=========================================================
class State
{
public:
    State(std::string state_name) 
        : name(state_name),
        navmesh_file_name(""),
        is_final_state(true),
        goal_set_id(-1)
    {}
    ~State() {}

    void setNavmeshFile(std::string file_name) {this->navmesh_file_name = file_name;}
    void setFinalState(bool is_final) { this->is_final_state = is_final; }
    void setGoalSetId(size_t goal_set_id) { this->goal_set_id = static_cast<int>(goal_set_id); }
    void setName(std::string name) { this->name = name; }

    bool isValid() const;
    std::string getName() const {return this->name;}
    std::string getNavmeshFileName() const {return this->navmesh_file_name;}
    bool getFinalState() const {return this->is_final_state;}
    int getGoalSetId() const {return this->goal_set_id;}

    YAML::Node to_yaml() const;

private:
    std::string name;
    std::string navmesh_file_name;
    bool is_final_state;
    int goal_set_id;
};

//=========================================================
class GoalSet
{
public:
    GoalSet(size_t goal_id) 
        : id(goal_id),
        capacity(1),
        goal_area_contained({})
    {}
    ~GoalSet() {}

    void addGoalArea(std::string goal_area_name);
    void setCapacity(size_t capacity) { this->capacity = capacity; }
    
    std::set<std::string> getGoalAreas() const { return this->goal_area_contained; }
    YAML::Node getGoalAreasToYaml() const;
    size_t getGoalSetId() const { return this->id; }
    size_t getCapacity() const {return this->capacity; }

    YAML::Node to_yaml() const;

private:
    size_t id;
    size_t capacity;
    std::set<std::string> goal_area_contained;
};

//=========================================================
class AgentProfile
{
public:
    AgentProfile(std::string profile_name) : profile_name(profile_name) {}
    ~AgentProfile() {}

    std::string profile_name = "new_profile";
    // default external_agent profile setup
    size_t profile_class = 1;
    double max_accel = 0.0;
    double max_angle_vel = 0.0;
    size_t max_neighbors = 10;
    double max_speed = 0.0;
    double neighbor_dist = 5.0;
    size_t obstacle_set = 1;
    double pref_speed = 0.0;
    double r = 0.25;
    double ORCA_tau = 1.0;
    double ORCA_tauObst = 0.4;
};

//=========================================================
class Condition
{
public:
    enum TYPE {
        BASE,
        GOAL,
        TIMER,
        AND,
        OR,
        NOT
    };

    Condition() : name("base_condition"), type(BASE) 
    {}
    Condition(std::string name, TYPE type) : name(name), type(type)
    {}
    virtual ~Condition() {}

    TYPE type;
    std::string name;

    virtual std::string getConditionName() const { return name; }
    virtual TYPE getType() const { return type; }
    virtual bool isValid() const { return false; }

};

class ConditionGOAL : public Condition 
{
public:
    ConditionGOAL() : Condition("goal_reached", GOAL)
    {}
    ~ConditionGOAL() {}

    void setValue(double distance) {
        this->distance = distance;
    }

    double getValue() {
        return distance;
    }

    bool isValid() const override { return true; }

private:
    double distance = 0.1;
};

class ConditionTIMER : public Condition {
public:
    ConditionTIMER() : Condition("timer", TIMER) 
    {}
    ~ConditionTIMER() {}

    void setValue(double value) {
        this->duration = value;
    }

    double getValue() {
        return this->duration;
    }

    std::string getTimerDistribution() {
        return this->distribution;
    }

    bool isValid() const override { return true; }

private:
    //currently only provides const value distribution for timer
    std::string distribution = "c";
    double duration = 30.0;
};

class ConditionAND : public Condition 
{
public:
    ConditionAND() 
        : Condition("and", AND), 
        condition1(std::make_shared<Condition>()), 
        condition2(std::make_shared<Condition>())
    {}
    ~ConditionAND() {}

    void setCondition(ConditionPtr condition, int condition_index){
        if (condition_index == 1) {
            this->condition1 = condition;
        } 
        if (condition_index == 2) {
            this->condition2 = condition;
        }
    }

    ConditionPtr getCondition(int condition_index) {
        if (condition_index == 1) {
            return this->condition1;
        } 
        if (condition_index == 2) {
            return this->condition2;
        }
    }

    bool isValid() const override {
        if(condition1 && condition2 && condition1->isValid() && condition2->isValid()) {
            return true;
        }
        std::cout << "Invalid and condition" << std::endl;
        return false;
    }

private:
    ConditionPtr condition1;
    ConditionPtr condition2;
};

class ConditionOR : public Condition 
{
public:
    ConditionOR() {
        type = OR;
        name = "or";
    }
    ~ConditionOR() {}

    void setCondition(ConditionPtr condition, int condition_index){
        if (condition_index == 1) {
            this->condition1 = condition;
        } 
        if (condition_index == 2) {
            this->condition2 = condition;
        }
    }

    ConditionPtr getCondition(int condition_index) {
        if (condition_index == 1) {
            return this->condition1;
        } 
        if (condition_index == 2) {
            return this->condition2;
        }
    }

    bool isValid() const override {
        if(condition1 && condition2 && condition1->isValid() && condition2->isValid()) {
            return true;
        }
        std::cout << "Invalid or condition" << std::endl;
        return false;
    }

private:
    ConditionPtr condition1;
    ConditionPtr condition2;
};

class ConditionNOT : public Condition
{
public:
    ConditionNOT() {
        type = NOT;
        name = "not";
    }
    ~ConditionNOT() {}

    void setCondition(ConditionPtr condition1) {
        this->condition1 = condition1;
    } 

    ConditionPtr getCondition() {
        return this->condition1;
    }

    bool isValid() const override {
        if(condition1 && condition1->isValid()) {
            return true;
        }
        std::cout << "Invalid not condition" << std::endl;
        return false;
    }

private:
    ConditionPtr condition1;
};

//=========================================================
class Transition
{
using StateName = std::string;
using ToStateType = std::map<StateName, double>;
public:
    Transition(StateName from_state_name) {
        this->from_state_name = from_state_name;
        this->condition = std::make_shared<Condition>();
    }
    ~Transition() {}

    void setFromState(StateName state_name) {
        this->from_state_name = state_name;
    }
    std::string getFromState() const {
        return this->from_state_name;
    }

    void addToState(StateName state_name, double weight = 1.0) {
        this->to_state_name.insert(std::make_pair(state_name, weight) );
    }
    void deleteToState(StateName state_name) {
        this->to_state_name.erase(state_name);
    }
    ToStateType getToState() const {
        return this->to_state_name;
    }
    void clearToState() {
        this->to_state_name.clear();
    }

    void setCondition(ConditionPtr condition) {
        this->condition = condition;
    }

    ConditionPtr getCondition() const {
        return condition;
    }

    bool isValid() {
        if (condition && condition->isValid() && to_state_name.size() > 0 && from_state_name.size() > 0) {
            return true;
        } 
        std::cout << "Invalid transition" << std::endl;
        return false;
    }

private:
    StateName from_state_name = "";
    //output states with weights
    ToStateType to_state_name;
    ConditionPtr condition;
};

//=========================================================
class AgentGroup
{
public:
    AgentGroup(size_t group_id, bool is_external_group = false) 
        : group_id(group_id), is_external_group(is_external_group)
    {}
    ~AgentGroup() {}

    bool isValid() {
        return agent_profile.size() > 0 && initial_state.size() > 0;
    }

    bool isExternalGroup() {
        return is_external_group;
    }

    size_t getGroupId() {
        return group_id;
    }

    void setSpawnPoint(double x, double y) {
        spawn_point_x = x;
        spawn_point_y = y;
    }
    std::pair<double, double> getSpawnPoint() {
        return std::pair<double, double>(spawn_point_x, spawn_point_y);
    }

    void setExternalAgentName(std::vector<std::string>& external_name) {
        external_agent_name.clear();
        for (auto name : external_name) {
            external_agent_name.emplace_back(name);
        }
        spawn_number = static_cast<int>(external_agent_name.size());
    }
    std::vector<std::string> getExternalAgentName() {
        return external_agent_name;
    }

    void setSpawnNumber(int number) {
        spawn_number = number;
    }
    int getSpawnNumber() {
        return spawn_number;
    }

    void setAgentProfile(std::string profile) {
        agent_profile = profile;
    }
    std::string getAgentProfile() {
        return agent_profile;
    }

    void setInitialState(std::string state) {
        initial_state = state;
    }
    std::string getInitialState() {
        return initial_state;
    }

private:
    size_t group_id;
    bool is_external_group = false;
    double spawn_point_x = 0.0;
    double spawn_point_y = 0.0;
    int spawn_number = 0;
    std::vector<std::string> external_agent_name = {};

    std::string agent_profile = "";
    std::string initial_state = "";
};

//=========================================================
class CrowdSimImplementation
{
public:
    CrowdSimImplementation() 
        : enable_crowd_sim(false)
    {
        initializeState();
        initializeAgentProfile();
        initializeAgentGroup();
    }
    ~CrowdSimImplementation() {}

    std::vector<std::string> getGoalAreas();
    std::vector<std::string> getNavmeshFileName();
    void clearAgentProfile();

    YAML::Node to_yaml();

    // update from project.building in crowd_sim_table
    std::set<std::string> goal_areas;
    std::vector<std::string> navmesh_filename_list;
    
    // real configurations
    bool enable_crowd_sim;
    std::vector<State> states;
    std::vector<GoalSet> goal_sets;
    std::vector<Transition> transitions;
    std::vector<AgentProfile> agent_profiles;
    std::vector<AgentGroup> agent_groups;
    // std::vector<ModelType> model_types;


private:
    void initializeState();
    void initializeAgentProfile();
    void initializeAgentGroup();
};

} //namespace crowd_sim
using CrowdSimImplPtr = std::shared_ptr<crowd_sim::CrowdSimImplementation>;

#endif
