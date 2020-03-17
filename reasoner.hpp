#ifndef REASONER
#define REASONER

#include <cstdint>
#include <vector>

namespace reasoner {
    constexpr int NUMBER_OF_PLAYERS = 3;
    constexpr int MONOTONIC_CLASSES = 0;

    class resettable_bitarray_stack {};

    typedef uint32_t move_representation;
    struct move {
        move_representation mr;
        move(const move_representation& mv) : mr(mv) {};
        move(void) = default;
        bool operator==(const move& rhs) const {
            return mr == rhs.mr;
        }
    };
    enum player : int {
        WHITE = 1,
        BLACK = 2
    };
    struct jump_mask {
        uint32_t mask;
        uint8_t empty;
        uint8_t enemy;
    };
    class game_state {
        public:
            int get_current_player(void) const;
            int get_player_score(int player_id) const;
            void apply_move(const move& m);
            void get_all_moves(resettable_bitarray_stack&, std::vector<move>& moves);
            bool apply_any_move(resettable_bitarray_stack&);
            int get_monotonicity_class(void);
            bool is_legal(const move& m) const;
        private:
            inline uint32_t get_movers_up(const uint32_t&) const;
            inline uint32_t get_movers_down(const uint32_t&) const;
            inline uint32_t get_jumpers_up(const uint32_t&, const uint32_t&) const;
            inline uint32_t get_jumpers_down(const uint32_t&, const uint32_t&) const;
            inline void get_move_list_up(uint32_t, std::vector<move>&);
            inline void get_move_list_down(uint32_t, std::vector<move>&);
            inline void get_jump_list_up(uint32_t, const uint32_t&, std::vector<move>&);
            inline void get_jump_list_down(uint32_t, const uint32_t&, std::vector<move>&);
            inline uint32_t msb(const uint32_t&) const;
            std::pair<uint32_t, const uint32_t> pieces[2] = {
                { 0x00000FFF, 0xF0000000 },  // white, last row
                { 0xFFF00000, 0x0000000F }  // black, last row
            };
            uint32_t kings = 0x00000000;
            uint32_t empty = 0x000FF000;
            uint32_t obligatory_jumper = 0x0;
            int king_moves = 0;
            player current_player = BLACK;
            int variables[2] = {0, 0};
    };
}

#endif
