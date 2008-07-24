#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum player {
	red, blue, empty, blocked
} player_t;

typedef enum role {
	bomb, general, colonel, lieutenant, minor, rider, spy, flag, roles, unassigned
} role_t;

typedef struct pos {
	int row, col;
} pos_t;

typedef struct move {
	pos_t from, to;
    role_t attacker, defender;
} move_t;

typedef struct piece {
	player_t owner;
	role_t role;
    int moved;
} piece_t;

static const int movable[roles] = {
	0, 1, 1, 1, 1, 1, 1, 0 };
static int undeclared[2][roles] = {
	{ 4, 1, 2, 3, 4, 5, 1, 1 },
	{ 4, 1, 2, 3, 4, 5, 1, 1 } };
static char symbol[roles] = {
    'B', 'G', 'O', 'L', 'M', 'R', 'S', 'V' };

static int my_player, current_player = red;
static unsigned moves_played = 0;
static move_t history[400];

static move_t moves[21*4*roles];
static int moves_size;

static piece_t field[7][7] = {
	{ { red, unassigned, 0 }, { red, unassigned, 0 }, { red, unassigned, 0 },
	  { red, unassigned, 0 }, { red, unassigned, 0 }, { red, unassigned, 0 },
	  { red, unassigned, 0 } },
	{ { red, unassigned, 0 }, { red, unassigned, 0 }, { red, unassigned, 0 },
	  { red, unassigned, 0 }, { red, unassigned, 0 }, { red, unassigned, 0 },
	  { red, unassigned, 0 } },
	{ { red, unassigned, 0 }, { red, unassigned, 0 }, { red, unassigned, 0 },
	  { red, unassigned, 0 }, { red, unassigned, 0 }, { red, unassigned, 0 },
      { red, unassigned, 0 } },
    { { empty }, { blocked }, { empty },  { empty },
      { empty }, { blocked }, { empty } },
	{ { blue, unassigned, 0 }, { blue, unassigned }, { blue, unassigned },
	  { blue, unassigned, 0 }, { blue, unassigned }, { blue, unassigned },
	  { blue, unassigned, 0 } },
	{ { blue, unassigned, 0 }, { blue, unassigned }, { blue, unassigned },
	  { blue, unassigned, 0 }, { blue, unassigned }, { blue, unassigned },
	  { blue, unassigned, 0 } },
	{ { blue, unassigned, 0 }, { blue, unassigned }, { blue, unassigned },
	  { blue, unassigned, 0 }, { blue, unassigned }, { blue, unassigned },
	  { blue, unassigned, 0 } } };

void comment(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fputc('\t', stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    fflush(stderr);
}

void execute(move_t *move)
{
    if( field[move->to.row][move->to.col].owner != empty )
    {
        /* Assign roles to pieces, if they were unassigned. */
        if(field[move->from.row][move->from.col].role == unassigned)
        {
            field[move->from.row][move->from.col].role = move->attacker;
            --undeclared[current_player][move->attacker];
        }
        if(field[move->to.row][move->to.col].role == unassigned)
        {
            field[move->to.row][move->to.col].role = move->defender;
            --undeclared[1 - current_player][move->defender];
        }
    }

    ++(field[move->from.row][move->from.col].moved);
    
    if( field[move->to.row][move->to.col].owner == empty ||
        move->attacker < move->defender || 
        (move->attacker == minor && move->defender == bomb) ||
        (move->attacker == spy && move->defender == general) )
    {
        /* Attacker takes field. */
        field[move->to.row][move->to.col] =
            field[move->from.row][move->from.col];
    }
    else
    if( move->attacker == move->defender )
    {
        /* Both players lose. */
        field[move->to.row][move->to.col].owner = empty;
    }

    field[move->from.row][move->from.col].owner = empty;

	history[moves_played++] = *move;
    current_player = 1 - current_player;
}

const char *next_word()
{
    static char word[8];
	scanf("%7s", word);
	if(strcmp(word, "X") == 0)
		exit(0);
	return word;
}

void parse_move(const char *word, move_t *move)
{
    char fc, fr, tc, tr;
    sscanf(word, "%c%c-%c%c", &fc, &fr, &tc, &tr);
    move->from.row = fr - '1';
    move->from.col = fc - 'a';
    move->to.row = tr - '1';
    move->to.col = tc - 'a';
}

role_t parse_role(const char *word)
{
    char r;
    role_t result = 0;
    sscanf(word, "%c", &r);
    while(symbol[result] != r)
        ++result;
    return result;
}

void parse_attacker(const char *word, move_t *move)
{
    move->attacker = parse_role(word);
}

void parse_defender(const char *word, move_t *move)
{
    move->defender = parse_role(word);
}

int generate_moves()
{
    int dir, unmovable_roles_left = 0, unmoved_undeclared_left = 0;
    move_t move;

    moves_size = 0;

    /* Check if we may move additional undeclared unmoved pieces. */
    {
        int row, col;
        role_t role;
        for(role = 0; role < roles; ++role)
            if(!movable[role])
                unmovable_roles_left += undeclared[current_player][role];
        for(row = 0; row < 7; ++row)
            for(col = 0; col < 7; ++col)
                if( field[row][col].owner == current_player && 
                    field[row][col].moved == 0 &&
                    field[row][col].role == unassigned )
                {
                    ++unmoved_undeclared_left;
                }
    }

    /* Generate all moves. */
    for(move.from.row = 0; move.from.row < 7; ++move.from.row)
        for(move.from.col = 0; move.from.col < 7; ++move.from.col)
        {
            if( field[move.from.row][move.from.col].owner == current_player &&
                ( field[move.from.row][move.from.col].role == unassigned ||
                  movable[field[move.from.row][move.from.col].role] ) &&
                ( field[move.from.row][move.from.col].role != unassigned ||
                  unmoved_undeclared_left > unmovable_roles_left ||
                  field[move.from.row][move.from.col].moved > 0) )
            {
                for(dir = 0; dir < 4; ++dir)
                {
                    switch(dir)
                    {
                    case 0:
                        move.to.col = move.from.col;
                        move.to.row = move.from.row + 1;
                        break;
                    case 1:
                        move.to.col = move.from.col + 1;
                        move.to.row = move.from.row;
                        break;
                    case 2:
                        move.to.col = move.from.col - 1;
                        move.to.row = move.from.row;
                        break;
                    case 3:
                        move.to.col = move.from.col;
                        move.to.row = move.from.row - 1;
                        break;
                    }

                    if( move.to.col >= 0 && move.to.col < 7 &&
                        move.to.row >= 0 && move.to.row < 7)
                    {
                        if(field[move.to.row][move.to.col].owner == empty)
                        {
                            move.attacker = move.defender = unassigned;
                            moves[moves_size++] = move;
                        }
                        else
                        if(field[move.to.row][move.to.col].owner == 1 - current_player)
                        {
                            if(field[move.from.row][move.from.col].role == unassigned)
                            {
                                for(move.attacker = 0; move.attacker < roles; ++move.attacker)
                                    if(movable[move.attacker] && undeclared[current_player][move.attacker] > 0)
                                        moves[moves_size++] = move;
                            }
                            else
                            {
                                move.attacker = field[move.from.row][move.from.col].role;
                                moves[moves_size++] = move;
                            }
                        }
                    }
                }
            }
        }

    if(moves_played >= 2 && history[moves_played - 2].attacker == unassigned)
    {
        int m;

        if(current_player == my_player)
            comment("Forbidden move: %c%c-%c%c",
                'a' + history[moves_played - 2].to.col,   '1' + history[moves_played - 2].to.row,
                'a' + history[moves_played - 2].from.col, '1' + history[moves_played - 2].from.row );

        for(m = 0; m < moves_size; ++m)
            if( moves[m].attacker == unassigned &&
                moves[m].from.col == history[moves_played - 2].to.col &&
                moves[m].from.row == history[moves_played - 2].to.row && 
                moves[m].to.col == history[moves_played - 2].from.col && 
                moves[m].to.row == history[moves_played - 2].from.row )
            {
                moves[m] = moves[--moves_size];
                --m;
            }
    }

    return moves_size;
}

void select_move(move_t *move)
{
    *move = moves[rand() % moves_size];
}

void select_role(move_t *move)
{
    /* Choose a role for the defender of this move. */
    piece_t *p = &field[move->to.row][move->to.col];
    if(p->role != unassigned)
    {
        move->defender = p->role;
    }
    else
    {
        move->defender = 0;
        while( undeclared[1 - current_player][move->defender] == 0 ||
               (!movable[move->defender] && p->moved > 0) )
        {
            ++(move->defender);
        }
    }
}

int game_over()
{
	return 
        generate_moves() == 0 ||
		undeclared[red ][flag] == 0 ||
		undeclared[blue][flag] == 0 ||
		moves_played == 2*200;
}

void comment_board()
{
    int r, c;

    char line[] = " . . . . . . . ";
    for(r = 6; r >= 0; --r)
    {
        for(c = 0; c < 7; ++c)
        {
            if(field[r][c].owner == empty || field[r][c].owner == blocked)
            {
                if(field[r][c].owner == empty)
                {
                    line[2*c + 1] = '.';
                    line[2*c + 2] = ' ';
                }
                else
                {
                    line[2*c + 1] = '[';
                    line[2*c + 2] = ']';
                }            }
            else
            {
                line[2*c + 1] = field[r][c].owner == red ? 'R' : 'B';
                line[2*c + 2] = field[r][c].role == unassigned ?
                               '?' : symbol[field[r][c].role];
            }
        }
        comment(line);
    }

}

int main()
{
    const char *word;
    
    srand((unsigned)time(NULL));

    word = next_word();
	if(strcmp(word, "Start") == 0)
    {
    	/* I get to move first. */
		my_player = red;
    }
	else
    {
   	    /* My opponent moves first. */
        move_t move;
		my_player = blue;
        parse_move(word, &move);
        execute(&move);
    }

	/* Run game loop. */
	while(!game_over())
	{
        move_t move;

        if(current_player == my_player)
		{
            comment_board();
            comment("I must select a move.");
            select_move(&move);
            fprintf( stdout, "%c%c-%c%c\n",
                move.from.col + 'a', move.from.row + '1',
                move.to.col   + 'a', move.to.row   + '1' );
            fflush(stdout);
            if( field[move.to.row][move.to.col].owner != empty )
            {
                fprintf(stdout, "%c\n", symbol[move.attacker]);
                fflush(stdout);
                parse_defender(next_word(), &move);
            }
		}
		else
		{
            parse_move(next_word(), &move);
            if( field[move.to.row][move.to.col].owner != empty )
            {
                comment("I must select a role for the defender.");
                select_role(&move);
                fprintf(stdout, "%c\n", symbol[move.defender]);
                fflush(stdout);
                parse_attacker(next_word(), &move);
            }
		}

        execute(&move);
    }
    comment("Game has ended.");
 
    return 0;
}