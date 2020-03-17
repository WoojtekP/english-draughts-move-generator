#include <iostream>
#include <random>
#include <chrono>
#include <optional>
#include "reasoner.hpp"

typedef unsigned int uint;
typedef unsigned long ulong;
constexpr int KEEPER = 0;

std::mt19937 random_generator(1);
inline uint random_choice(const uint bound) {
    return std::uniform_int_distribution<uint>(0,bound)(random_generator);
}

ulong goals_avg[reasoner::NUMBER_OF_PLAYERS] = {};
int goals_min[reasoner::NUMBER_OF_PLAYERS] = {};
int goals_max[reasoner::NUMBER_OF_PLAYERS] = {};
ulong states_count = 0, moves_count = 0;
uint depth_min = std::numeric_limits<uint>::max();
uint depth_max = std::numeric_limits<uint>::min();
reasoner::resettable_bitarray_stack cache;
reasoner::game_state initial_state;

void initialize_goals_arrays(void){
    for(uint i=0;i<reasoner::NUMBER_OF_PLAYERS;++i){
        goals_min[i] = std::numeric_limits<int>::max();
        goals_max[i] = std::numeric_limits<int>::min();
    }
}

void count_terminal(const reasoner::game_state state, uint depth){
	states_count += depth;
    if(depth < depth_min)
        depth_min = depth;
    if(depth > depth_max)
        depth_max = depth;
    for(uint i=1;i<reasoner::NUMBER_OF_PLAYERS;++i){
        goals_avg[i] += state.get_player_score(i);
        if(state.get_player_score(i) < goals_min[i])
            goals_min[i] = state.get_player_score(i);
        if(state.get_player_score(i) > goals_max[i])
            goals_max[i] = state.get_player_score(i);
    }
}

std::optional<uint> try_to_choose_random_from_monotonics(std::vector<reasoner::move>& monotonic_moves, const reasoner::game_state& state){
    uint chosen_move = random_choice(monotonic_moves.size()-1);
    if(state.is_legal(monotonic_moves[chosen_move]))
        return chosen_move;
    else{
        monotonic_moves[chosen_move] = monotonic_moves.back();
        monotonic_moves.pop_back();
        return std::nullopt;
    }
}

std::optional<uint> choose_random_from_monotonics_or_leave_empty(std::vector<reasoner::move>& monotonic_moves, const reasoner::game_state& state){
    while(not monotonic_moves.empty()){
        if(auto potential_result = try_to_choose_random_from_monotonics(monotonic_moves, state))
            return potential_result;
    }
    return std::nullopt;
}

template<int MONOTONIC_CLASSES>
void random_simulation_with_monotonic_moves(){
    static std::vector<reasoner::move> legal_moves;
    static std::vector<reasoner::move> monotonic_moves[MONOTONIC_CLASSES];
    for(uint i=0;i<MONOTONIC_CLASSES;++i)
        monotonic_moves[i].clear();
    reasoner::game_state state = initial_state;
    uint depth = 0;
    while(true){
        auto monotonicity_class = state.get_monotonicity_class();
        if(monotonicity_class >= 0){
            if(monotonic_moves[monotonicity_class].empty()) {
                state.get_all_moves(cache, monotonic_moves[monotonicity_class]);
                moves_count += monotonic_moves[monotonicity_class].size();
            }
            if(auto chosen_move = choose_random_from_monotonics_or_leave_empty(monotonic_moves[monotonicity_class], state)){
                depth++;
                state.apply_move(monotonic_moves[monotonicity_class][*chosen_move]);
            }
            else{
                count_terminal(state, depth);
                return;
            }
        }
        else{
            state.get_all_moves(cache, legal_moves);
            if(legal_moves.empty()){
                count_terminal(state, depth);
                return;
            }
            else{
                depth++;
                moves_count += legal_moves.size();
                uint chosen_move = random_choice(legal_moves.size()-1);
                state.apply_move(legal_moves[chosen_move]);
            }
        }
        while(state.get_current_player() == KEEPER){
            auto any_move = state.apply_any_move(cache);
            if(not any_move){
                count_terminal(state, depth);
                return;
            }
        }
    }
}

void random_simulation(){
    static std::vector<reasoner::move> legal_moves;
    reasoner::game_state state = initial_state;
    uint depth = 0;
    while(true){
        state.get_all_moves(cache, legal_moves);
        if(legal_moves.empty()){
            count_terminal(state, depth);
            return;
        }
        else{
            depth++;
            moves_count += legal_moves.size();
            uint chosen_move = random_choice(legal_moves.size()-1);
            state.apply_move(legal_moves[chosen_move]);
        }
        while(state.get_current_player() == KEEPER){
            auto any_move = state.apply_any_move(cache);
            if(not any_move){
                count_terminal(state, depth);
                return;
            }
        }
    }
}

double count_per_sec(ulong count, ulong ms){
    return static_cast<long double>(count)/ms*1000.0;
}

int main(int argv, char** argc){
    if(argv != 2){
        std::cout << "Number of simulations unspecified. Exitting..." << std::endl;
        return 1;
    }
    std::cout << "Monotonic classes: " << reasoner::MONOTONIC_CLASSES << std::endl;
    initialize_goals_arrays();
    while(initial_state.get_current_player() == KEEPER){
        auto any_move = initial_state.apply_any_move(cache);
        if(not any_move)
            return 2;
    }
    ulong simulations_count = std::stoi(argc[1]);

    std::chrono::steady_clock::time_point start_time(std::chrono::steady_clock::now());
    for(ulong i = 0; i < simulations_count; ++i){
        if constexpr (reasoner::MONOTONIC_CLASSES)
            random_simulation_with_monotonic_moves<reasoner::MONOTONIC_CLASSES>();
        else
            random_simulation();
    }
    std::chrono::steady_clock::time_point end_time(std::chrono::steady_clock::now());

    ulong ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time-start_time).count();
    std::cout << "time: " << ms << " ms" << std::endl;
    std::cout << "number of plays: " << simulations_count << " (" << std::fixed << count_per_sec(simulations_count, ms) << " plays/sec)" << std::endl;
    std::cout << "number of states: " << states_count << " (" << std::fixed << count_per_sec(states_count, ms) << " states/sec)" << std::endl;
    std::cout << "number of moves: " << moves_count << " (" << std::fixed << count_per_sec(moves_count, ms) << " moves/sec)" << std::endl;
    std::cout << "depth: avg " << static_cast<long double>(states_count)/simulations_count << " min " << depth_min << " max " << depth_max << std::endl;
    for(uint i=1;i<reasoner::NUMBER_OF_PLAYERS;++i)
        std::cout << "goal of player " << i << ": avg " << static_cast<long double>(goals_avg[i])/simulations_count << " min " << goals_min[i] << " max " << goals_max[i] << std::endl;
    return 0;
}
