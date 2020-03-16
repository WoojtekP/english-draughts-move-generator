#include "reasoner.hpp"

namespace reasoner {
    int game_state::get_current_player(void) const {
        return current_player;
    }

    int game_state::get_player_score(int player_id) const {
        return variables[player_id-1];
    }

    void game_state::apply_move(const move& mv) {
        obligatory_jumper = 0;
        uint32_t movemask = mv.mr;
        uint32_t current = pieces[current_player - 1].first;
        uint32_t opponent = pieces[current_player & 1].first;
        uint32_t last_row = pieces[current_player - 1].second;

        if ((movemask & current & kings) && !(movemask & opponent)) {
            king_moves++;
        }
        else {
            king_moves = 0;
        }
        auto old_opponent = opponent;
        auto old_current = current;
        bool promotion = movemask & current & ~kings;

        current ^= movemask & ~opponent;
        opponent ^= movemask & opponent;

        if (old_current & movemask & kings) {
            kings |= movemask & current;
        }
        kings |= current & last_row;
        kings &= current | opponent;
        empty = ~(current | opponent);
        
        promotion = promotion && (current & movemask & kings);

        pieces[current_player - 1].first = current;
        pieces[current_player & 1].first = opponent;

        if (current_player == WHITE) {
            if ((opponent != old_opponent) && !promotion) {
                obligatory_jumper = get_jumpers_up(current & movemask, opponent);
                obligatory_jumper |= get_jumpers_down(current & movemask & kings, opponent);
            }
            if (!obligatory_jumper) {
                current_player = BLACK;
            }
        }
        else {
            if ((opponent != old_opponent) && !promotion) {
                obligatory_jumper = get_jumpers_down(current & movemask, opponent);
                obligatory_jumper |= get_jumpers_up(current & movemask & kings, opponent);
            }
            if (!obligatory_jumper) {
                current_player = WHITE;
            }
        }
    }

    std::vector<move> game_state::get_all_moves(resettable_bitarray_stack& cache) {
        std::vector<move> result;
        result.reserve(100);
        get_all_moves(cache, result);
        return result;
    }

    void game_state::get_all_moves(resettable_bitarray_stack&, std::vector<move>& moves) {
        moves.clear();
        if (king_moves >= 20) {
            variables[0] = variables[1] = 50;
            return;
        }
        
        uint32_t enemies = pieces[current_player & 1].first;
        uint32_t piecesU = pieces[0].first;
        uint32_t piecesD = piecesU & kings;
        if (current_player == BLACK) {
            piecesD = pieces[1].first;
            piecesU = piecesD & kings;
        }
        
        if (obligatory_jumper) {
            piecesU &= obligatory_jumper;
            piecesD &= obligatory_jumper;
        }
        uint32_t jumpersUp = get_jumpers_up(piecesU, enemies);
        uint32_t jumpersDown = get_jumpers_down(piecesD, enemies);

        if (jumpersUp || jumpersDown) {
            get_jump_list_up(jumpersUp, enemies, moves);
            get_jump_list_down(jumpersDown, enemies, moves);
        }
        else {
            get_move_list_up(get_movers_up(piecesU), moves);
            get_move_list_down(get_movers_down(piecesD), moves);
        }
        if (moves.size() == 0) {
            variables[current_player - 1] = 0;
            variables[current_player & 1] = 100;
        }
    }

    bool game_state::apply_any_move(resettable_bitarray_stack&) {
        return false;
    }

    int game_state::get_monotonicity_class(void) {
        return -1;
    }

    bool game_state::is_legal([[maybe_unused]] const move& m) const {
        return false;
    }

    inline uint32_t game_state::get_movers_up(const uint32_t& pieces) const {
        if (pieces == 0)
            return 0;
        uint32_t movers = (empty >> 4) & pieces;
        movers |= (empty >> 3) & pieces & 0x0E0E0E0E;
        movers |= (empty >> 5) & pieces & 0x00707070;
        return movers;
    }
    
    inline uint32_t game_state::get_movers_down(const uint32_t& pieces) const {
        if (pieces == 0)
            return 0;
        uint32_t movers = (empty << 4) & pieces;
        movers |= (empty << 3) & pieces & 0x70707070;
        movers |= (empty << 5) & pieces & 0x0E0E0E00;
        return movers;
    }

    inline uint32_t game_state::get_jumpers_down(const uint32_t& pieces, const uint32_t& enemies) const {
        if (pieces == 0)
            return 0;
        uint32_t jumps = pieces & 0x07070700 & (empty << 7) & (enemies << 4);  // odd -> right
        jumps |= pieces & 0x0E0E0E00 & (empty << 9) & (enemies << 5);  // odd -> left
        jumps |= pieces & 0x70707000 & (empty << 7) & (enemies << 3);  // even -> right
        jumps |= pieces & 0xE0E0E000 & (empty << 9) & (enemies << 4);  // even -> left
        return jumps;
    }

    inline uint32_t game_state::get_jumpers_up(const uint32_t& pieces, const uint32_t& enemies) const {
        if (pieces == 0)
            return 0;
        uint32_t jumps = pieces & 0x00070707 & (empty >> 9) & (enemies >> 4);  // odd -> right
        jumps |= pieces & 0x000E0E0E & (empty >> 7) & (enemies >> 3);  // odd -> left
        jumps |= pieces & 0x00707070 & (empty >> 9) & (enemies >> 5);  // even -> right
        jumps |= pieces & 0x00E0E0E0 & (empty >> 7) & (enemies >> 4);  // even -> left
        return jumps;
    }

    inline void game_state::get_move_list_up(uint32_t pieces, std::vector<move>& moves) {
        while(pieces) {
            uint32_t piece = msb(pieces);
            bool mv = (empty >> 4) & piece;
            if (mv)
                moves.push_back(piece | (piece << 4));
            
            mv = (empty >> 3) & piece & 0x0E0E0E0E;
            if (mv) 
                moves.push_back(piece | (piece << 3));
            
            mv = (empty >> 5) & piece & 0x00707070;
            if (mv)
                moves.push_back(piece | (piece << 5));

            pieces ^= piece;
        }
    }

    inline void game_state::get_move_list_down(uint32_t pieces, std::vector<move>& moves) {
        while(pieces) {
            uint32_t piece = msb(pieces);
            bool mv = (empty << 4) & piece;
            if (mv)
                moves.push_back(piece | (piece >> 4));
            
            mv = (empty << 3) & piece & 0x70707070;
            if (mv)
                moves.push_back(piece | (piece >> 3)); 
            
            mv = (empty << 5) & piece & 0x0E0E0E00;
            if (mv)
                moves.push_back(piece | (piece >> 5));
            
            pieces ^= piece;
        }
    }

    inline void game_state::get_jump_list_up(uint32_t jumpers, const uint32_t& enemies, std::vector<move>& moves) {
        static constexpr jump_mask bits[4] = {
            {0x00707070, 9, 5}, // even rows
            {0x00E0E0E0, 7, 4}, // even rows
            {0x00070707, 9, 4}, // odd rows
            {0x000E0E0E, 7, 3}  // odd rows
        };
        while(jumpers) {
            uint32_t jumper = msb(jumpers);
            int offset = (jumper & 0x000F0F0F) ? 2 : 0;
            for (int i = offset; i < offset + 2; ++i) {
                bool jump = bits[i].mask & jumper & (empty >> bits[i].empty) & (enemies >> bits[i].enemy);
                if (jump)
                    moves.push_back(jumper | (jumper << bits[i].empty) | (jumper << bits[i].enemy));
            }
            jumpers ^= jumper;
        }
    }

    inline void game_state::get_jump_list_down(uint32_t jumpers, const uint32_t& enemies, std::vector<move>& moves) {
        static constexpr jump_mask bits[4] = {
            {0x70707000, 7, 3}, // even rows
            {0xE0E0E000, 9, 4}, // even rows
            {0x07070700, 7, 4}, // odd rows
            {0x0E0E0E00, 9, 5}  // odd rows
        };
        while(jumpers) {
            uint32_t jumper = msb(jumpers);
            int offset = (jumper & 0x0F0F0F00) ? 2 : 0;
            for (int i = offset; i < offset + 2; ++i) {
                bool jump = bits[i].mask & jumper & (empty << bits[i].empty) & (enemies << bits[i].enemy);
                if (jump)
                    moves.push_back(jumper | (jumper >> bits[i].empty) | (jumper >> bits[i].enemy));
            }
            jumpers ^= jumper;
        }
    }

    inline uint32_t game_state::msb(const uint32_t& pieces) const {
        auto k = 31 - __builtin_clz(pieces);
        return 1u << k;
    }
}
