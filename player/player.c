#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#ifdef _MSVC
    #include <process.h>
#else
    #include <unistd.h>
#endif

/* Compiler-specific optimizations */

#ifdef _MSVC
    #define PACKED
    #define INLINE
#else
    /* GCC 2.95 */
    #define PACKED __attribute__((packed))
    #define INLINE inline
#endif


/* Game parameters */

#define BOARD_ROWS    (7)
#define BOARD_COLS    (7)
#define BOARD_FIELDS  (BOARD_ROWS*BOARD_COLS)
#define MAX_TURNS     (400)
#define MAX_MOVES     (80)


/* Algorithm parameters */



/* MIN/MAX */
#define MIN(a,b) ((a)<=(b)?(a):(b))
#define MAX(a,b) ((a)>=(b)?(a):(b))


/* Game types */

typedef enum bool {
    FALSE, TRUE
} PACKED bool_t;


typedef enum role {
    V, B, G, O, L, M, R, S, ROLES
} PACKED role_t;


typedef enum player {
    RED, BLUE, PLAYERS
} PACKED player_t;


typedef struct field {
    unsigned occupied   : 1;    /* TRUE or FALSE */
    unsigned owner      : 1;    /* RED or BLUE (valid if occupied == TRUE) */
    unsigned moved      : 1;    /* TRUE or FALSE (valid if occupied == TRUE) */
    unsigned role       : 3;    /* any role; 0 means unassigned */
} PACKED field_t;


typedef struct move
{
    unsigned char from, to;
} move_t;


typedef struct turn
{
    move_t move;
    role_t attacker, defender;  /* attacker == V means: no attack */
} turn_t;


typedef union board
{
    field_t fields[BOARD_FIELDS];
    field_t coords[BOARD_ROWS][BOARD_COLS];
} board_t;


typedef struct state
{
    board_t   board;
    player_t  current_player;
    char      possible_bombs[PLAYERS];
    char      undeclared[PLAYERS][ROLES];
} PACKED state_t;

typedef short value_t;


/* Game constants and look-up tables */

static const char capture_table[ROLES][ROLES] = {
    /* V  B  G  O  L  M  R  S  def    att */
    {  0, 0, 0, 0, 0, 0, 0, 0, },   /* V */
    {  0, 0, 0, 0, 0, 0, 0, 0, },   /* B */
    { +1,-1, 0,+1,+1,+1,+1,+1, },   /* G */
    { +1,-1,-1, 0,+1,+1,+1,+1, },   /* O */
    { +1,-1,-1,-1, 0,+1,+1,+1, },   /* L */
    { +1,+1,-1,-1,-1, 0,+1,+1, },   /* M */
    { +1,-1,-1,-1,-1,-1, 0,+1, },   /* R */
    { +1,-1,+1,-1,-1,-1,-1, 0  } }; /* S */

static const unsigned char move_table[BOARD_FIELDS][5] = {
    { 7, 1,-1},       { 8, 0, 2,-1},    { 9, 1, 3,-1},    {10, 2, 4,-1},
    {11, 3, 5,-1},    {12, 4, 6,-1},    {13, 5,-1},       { 0,14, 8,-1},
    { 1,15, 7, 9,-1}, { 2,16, 8,10,-1}, { 3,17, 9,11,-1}, { 4,18,10,12,-1},
    { 5,19,11,13,-1}, { 6,20,12,-1},    { 7,21,15,-1},    { 8,14,16,-1},
    { 9,23,15,17,-1}, {10,24,16,18,-1}, {11,25,17,19,-1}, {12,18,20,-1},
    {13,27,19,-1},    {14,28,-1},       {-1},             {16,30,24,-1},
    {17,31,23,25,-1}, {18,32,24,-1},    {-1},             {20,34,-1},
    {21,35,29,-1},    {36,28,30,-1},    {23,37,29,31,-1}, {24,38,30,32,-1},
    {25,39,31,33,-1}, {40,32,34,-1},    {27,41,33,-1},    {28,42,36,-1},
    {29,43,35,37,-1}, {30,44,36,38,-1}, {31,45,37,39,-1}, {32,46,38,40,-1},
    {33,47,39,41,-1}, {34,48,40,-1},    {35,43,-1},       {36,42,44,-1},
    {37,43,45,-1},    {38,44,46,-1},    {39,45,47,-1},    {40,46,48,-1},
    {41,47,-1} };

static const char role_symbol_table[ROLES] = "VBGOLMRS";

static const value_t min_value  = -32767,
                     max_value  =  32767,
                     win_value  =  30000,
                     loss_value = -30000,
                     null_value = -32768,
                     tie_value  =      0;



/* Global variables */

static state_t  state;
static player_t myself;
static unsigned history_size;
static turn_t   moves_history[MAX_TURNS];
static state_t  states_history[MAX_TURNS];

#define CP (state.current_player)
#define OP (!state.current_player)

/* Function declarations */

int main(int argc, char *argv[]);
static void comment(const char *fmt, ...);

static INLINE char defeats(role_t attacker, role_t defender);

static void initialize_state();

static int generate_moves(move_t *moves);

static void execute(const turn_t *turn);
static INLINE void undo();

static value_t evaluate();
static value_t evaluate_piece(int row, int col);

value_t select_move(turn_t *best_turn);
value_t select_roles(turn_t *best_turn);

/* Function definitions */

int main(int argc, char *argv[])
{
    turn_t   turn;
    move_t   moves[MAX_MOVES];
    int      moves_size;
    char     buffer[32];

    /* Diagnostic: report sizes of games types */
    comment("role_t     size: %d", sizeof(role_t));
    comment("player_t   size: %d", sizeof(player_t));
    comment("field_t    size: %d", sizeof(field_t));
    comment("board_t    size: %d", sizeof(board_t));
    comment("move_t     size: %d", sizeof(move_t));
    comment("turn_t     size: %d", sizeof(turn_t));
    comment("state_t    size: %d", sizeof(state_t));


    /* Initialize. */
    initialize_state();

    myself = (fscanf(stdin, "S%s", buffer) == 1) ? RED : BLUE;

    while((moves_size = generate_moves(moves)))
    {
        if(CP == myself)
        {
            select_move(&turn);

            /* Print move */
            fprintf( stdout, "%c%c-%c%c\n",
                     'a' + (turn.move.from % 7), '1' + (turn.move.from / 7), 
                     'a' + (turn.move.to   % 7), '1' + (turn.move.to   / 7) );
            fflush(stdout);

            if(state.board.fields[turn.move.to].occupied)
            {
                /* Print attacking role */
                fprintf(stdout, "%c\n", role_symbol_table[turn.attacker]);
                fflush(stdout);

                /* Read defending role */
                if(scanf("%s", buffer) != 1)
                    break;
                turn.defender = 0;
                while(role_symbol_table[turn.defender] != buffer[0])
                    ++turn.defender;
            }
            else
            {
                turn.attacker = 0;
            }
        }
        else
        {
            /* Read move */
            if(scanf("%s", buffer) != 1)
                break;
            turn.move.from = (buffer[0] - 'a') + 7*(buffer[1] - '1');
            turn.move.to   = (buffer[3] - 'a') + 7*(buffer[4] - '1');

            if(state.board.fields[turn.move.to].occupied)
            {
                /* Pick the best role for the defender */
                select_roles(&turn);

                /* Print defending role */
                fprintf(stdout, "%c\n", role_symbol_table[turn.defender]);
                fflush(stdout);

                /* Read attacking role */
                if(scanf("%s", buffer) != 1)
                    break;
                turn.attacker = 0;
                while(role_symbol_table[turn.attacker] != buffer[0])
                    ++turn.attacker;
            }
            else
            {
                turn.attacker = 0;
            }
        }

        execute(&turn);

        /*
        comment("%c%c-%c%c",
                'a' + (turn.move.from % 7), '1' + (turn.move.from / 7), 
                'a' + (turn.move.to   % 7), '1' + (turn.move.to   / 7) );
        */
    
    }

    return 0;
}

void comment(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    fflush(stderr); 
}

static INLINE char defeats(role_t attacker, role_t defender)
{
    return capture_table[attacker][defender];
}

void initialize_state()
{
    int row, col;

    memset(&state.board, 0, sizeof(board_t));
    for(row = 0; row < 3; ++row)
        for(col = 0; col < BOARD_COLS; ++col)
        {
            state.board.coords[row][col].occupied = TRUE;
            state.board.coords[row][col].owner    = RED;
            state.board.coords[row][col].role     = V;     /* == unassigned */
        }
    for(row = BOARD_ROWS - 3; row < BOARD_ROWS; ++row)
        for(col = 0; col < BOARD_COLS; ++col)
        {
            state.board.coords[row][col].occupied = TRUE;
            state.board.coords[row][col].owner    = BLUE;
            state.board.coords[row][col].role     = V;     /* == unassigned */
        }

    state.undeclared[RED][V] = state.undeclared[BLUE][V] = 1;
    state.undeclared[RED][B] = state.undeclared[BLUE][B] = 4;
    state.undeclared[RED][G] = state.undeclared[BLUE][G] = 1;
    state.undeclared[RED][O] = state.undeclared[BLUE][O] = 2;
    state.undeclared[RED][L] = state.undeclared[BLUE][L] = 3;
    state.undeclared[RED][M] = state.undeclared[BLUE][M] = 4;
    state.undeclared[RED][R] = state.undeclared[BLUE][R] = 5;
    state.undeclared[RED][S] = state.undeclared[BLUE][S] = 1;

    state.possible_bombs[RED] = state.possible_bombs[BLUE] = 20;

    history_size = 0;
}

int generate_moves(move_t *moves)
{
    int moves_size;
    unsigned char src;
    const unsigned char *dst;
    bool_t bombs_fixed;

    if( history_size == MAX_TURNS ||
        state.undeclared[RED ][V] == 0 ||
        state.undeclared[BLUE][V] == 0 )
    {
        return 0;
    }

    bombs_fixed = state.possible_bombs[CP] ==
                    state.undeclared[CP][B];

    moves_size = 0;
    for(src = 0; src < BOARD_FIELDS; ++src)
        if( state.board.fields[src].occupied &&
            state.board.fields[src].owner == CP )
        {
            if(state.board.fields[src].role == B)
                continue;

            if( bombs_fixed && !state.board.fields[src].role &&
                !state.board.fields[src].moved )
                continue;

            for(dst = move_table[src]; *dst != (unsigned char)-1; ++dst)
            {

                if(state.board.fields[*dst].occupied)
                {
                    if(state.board.fields[*dst].owner == CP)
                    {
                        /* May not move to a field containing my own piece */
                        continue;
                    }
                }
                else
                {
                    if( history_size >= 2 &&
                        !moves_history[history_size - 2].attacker &&
                        moves_history[history_size - 2].move.from == *dst &&
                        moves_history[history_size - 2].move.to == src )
                    {
                        /* Skip forbidden move */
                        continue;
                    }
                }

                /* Add a new move */
                moves[moves_size].from = src;
                moves[moves_size].to   = *dst;
                ++moves_size;
            }
        }

    return moves_size;
}

void execute(const turn_t *turn)
{
    /* Update history */
    moves_history[history_size] = *turn;
    states_history[history_size] = state;
    ++history_size;

    if( !state.board.fields[turn->move.from].moved &&
        !state.board.fields[turn->move.from].role )
    {
        /* Update number of possible bombs for current player */
        --state.possible_bombs[CP];
    }

    state.board.fields[turn->move.from].moved = TRUE;

    if(turn->attacker)
    {
        /* Move and attack */
        char d;

        if( !state.board.fields[turn->move.to].moved &&
            !state.board.fields[turn->move.to].role )
        {
            /* Update number of possible bombs for other player */
            --state.possible_bombs[OP];
        }

        /* Assign roles */
        if(!state.board.fields[turn->move.from].role)
        {
            state.board.fields[turn->move.from].role = turn->attacker;
            --state.undeclared[CP][turn->attacker];
        }
        if(!state.board.fields[turn->move.to].role)
        {
            state.board.fields[turn->move.to].role = turn->defender;
            --state.undeclared[OP][turn->defender];
        }

        /* Determine who wins */
        d = defeats(turn->attacker, turn->defender);

        if(d > 0)
        {
            /* Attacker wins; replace defending piece by attacking piece */
            state.board.fields[turn->move.to  ] =
                state.board.fields[turn->move.from];
            state.board.fields[turn->move.from].occupied = FALSE;
        }
        else
        if(d < 0)
        {
            /* Attacker loses; remove attacking piece */
            state.board.fields[turn->move.from].occupied = FALSE;
        }
        else
        {
            /* It's a tie; remove both pieces */
            state.board.fields[turn->move.from].occupied = FALSE;
            state.board.fields[turn->move.to  ].occupied = FALSE;
        }
    }
    else
    {
        /* Move only */
        state.board.fields[turn->move.to  ] =
            state.board.fields[turn->move.from];
        state.board.fields[turn->move.from].occupied = FALSE;
    }

    CP = OP;
}

void INLINE undo()
{
    /* Restore last state from history */
    state = states_history[--history_size];
}

value_t evaluate()
{
    value_t value;
    move_t  moves[MAX_MOVES];
    int     moves_size, r, c;
    
    if((moves_size = generate_moves(moves)) == 0)
        return history_size == MAX_TURNS ? tie_value : loss_value;
        
    value = 0;
    for(r = 0; r < BOARD_ROWS; ++r)
        for(c = 0; c < BOARD_COLS; ++c)
        {
            if(state.board.coords[r][c].occupied)
            {
                value += ((state.board.coords[r][c].owner == CP) ? +1 : -1) *
                         evaluate_piece(r, c);
            }
        }
    
    return value;
}


value_t evaluate_piece(int r, int c)
{
    value_t value;
    field_t *field = &state.board.coords[r][c];
    int dr, dc, empty, friendly, enemy;
    
    return 1;
    
    /*
    value = 10;
    
    empty = friendly = enemy = 0;
    for(dr = -1; dr <= 1; ++dr)
        for(dc = -1; dc <= 1; ++dc)
        {
            field_t *neighbour;
            
            if( (dr == 0 && dr == 0) ||
                r + dr < 0 || r + dr >= BOARD_ROWS ||
                c + dc < 0 || c + dc >= BOARD_COLS )
            {
                continue;
            }
            
            neighbour = &state.board.coords[r +dr][c + dc];
            
            if(neighbour->occupied)
            {
                if(field->owner == neighbour->owner)
                {
                    ++value;
                }
                else
                {
                    value -= 5;
                }
            }
        }
    
    return value;
    */
}


value_t select_move(turn_t *best_turn)
{
    turn_t  turn;
    move_t  moves[MAX_MOVES];
    int     moves_size, m;
    value_t value, best_value;

    best_value = min_value;

    moves_size = generate_moves(moves);
    for(m = 0; m < moves_size; ++m)
    {
        turn.move = moves[m];
        
        if(state.board.fields[turn.move.to].occupied)
        {   
            value = select_roles(&turn);
        }
        else
        {
            turn.attacker = 0;
            execute(&turn);
            value = -evaluate();
            undo();
        
            comment( "select_move():  %c%c-%c%c      -- value: %4d",
                    'a' + (turn.move.from % 7), '1' + (turn.move.from / 7), 
                    'a' + (turn.move.to   % 7), '1' + (turn.move.to   / 7),
                    value );
        }
        
        
        if(value > best_value)
        {
            *best_turn = turn;
            best_value = value;
        }
    }
    
    return best_value;
}

value_t select_roles(turn_t *turn)
{
    value_t value[ROLES][ROLES], worst_value, best_value;
    role_t a, d, best_a, best_d;
    
    for(a = 0; a < ROLES; ++a)
        for(d = 0; d < ROLES; ++d)
        {
            if( /* Conditions for invalid attacker */
                state.board.fields[turn->move.from].role &&
                    state.board.fields[turn->move.from].role != a ||
                !state.board.fields[turn->move.from].role &&
                    state.undeclared[CP][a] == 0 ||
                a < G ||
                
                /* Conditions for invalid defender */
                state.board.fields[turn->move.to].role &&
                    state.board.fields[turn->move.to].role != d ||
                !state.board.fields[turn->move.to].role &&
                    state.undeclared[OP][d] == 0 ||
                state.board.fields[turn->move.to].moved && d < G ||
                state.possible_bombs[OP] == 0 && d != V ||
                state.possible_bombs[OP] == state.undeclared[OP][B]
                    && d != B )
            {
                /* Impossible attacker/defender pair. */
                value[a][d] = null_value;
            }
            else
            {
                turn->attacker = a;
                turn->defender = d;
                execute(turn);
                value[a][d] = -evaluate();
                undo(turn);
            
                comment( "select_roles(): %c%c-%c%c  %c %c -- value: %4d",
                        'a' + (turn->move.from % 7), '1' + (turn->move.from / 7), 
                        'a' + (turn->move.to   % 7), '1' + (turn->move.to   / 7),
                        role_symbol_table[a], role_symbol_table[d],
                        value[a][d] );
            
            }
        }

    /* Determine safest defender */
    best_value = min_value;
    for(d = 0; d < ROLES; ++d)
    {
        worst_value = max_value;
        for(a = 0; a < ROLES; ++a)
            if(value[a][d] != null_value && -value[a][d] < worst_value)
                worst_value = -value[a][d];
        if(worst_value != max_value && worst_value > best_value)
        {
            best_d = d;
            best_value = worst_value;
        }
    }
    comment( "--> best_value: %d", best_value);

    /* Determine safest attacker */
    best_value = min_value;
    for(a = 0; a < ROLES; ++a)
    {
        worst_value = max_value;
        for(d = 0; d < ROLES; ++d)
            if(value[a][d] != null_value && value[a][d] < worst_value)
                worst_value   = value[a][d];
        if(worst_value != max_value && worst_value > best_value)
        {
            best_a = a;
            best_value = worst_value;
        }
    }
    comment( "--> best_value: %d", best_value);
    
    turn->attacker = best_a;
    turn->defender = best_d;
    
    return best_value;
}
