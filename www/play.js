var hist = document.getElementById('history');
var mesg = document.getElementById('messages');

var players = new Array('R', 'B');
var roles   = new Array('B', 'G', 'O', 'L', 'M', 'R', 'S', 'V');

var last_state;

function Piece(owner, role, moved)
{
    this.owner = owner;
    this.role  = role;
    this.moved = moved ? moved : 0;
}

function defeats(attacker, defender)
{
    if(attacker == defender)
        return 0;
    if(attacker == 'S' && defender == 'G')
        return 1;
    if(attacker == 'M' && defender == 'B')
        return 1;     
    for(var r in roles)
    {
        if(roles[r] == attacker)
            return 1;
        if(roles[r] == defender)
            return -1;
    }
}

function State(state, r1, c1, r2, c2, attacker, defender)
{
    if(state)
    {
        this.board = new Array();
        this.board.length = state.board.length;
        for(var r = 0; r < state.board.length; ++r)
        {
            this.board[r] = new Array();
            this.board[r].length = state.board[r].length;
            for(var c = 0; c < state.board.length; ++c)
                this.board[r][c] = new Piece(
                    state.board[r][c].owner, state.board[r][c].role, state.board[r][c].moved );
        }
        
        this.roles = new Object();
        this.roles['R'] = new Object();
        this.roles['B'] = new Object();
        for(var r in state.roles['R'])
            this.roles['R'][r] = state.roles['R'][r];
        for(var r in state.roles['B'])
            this.roles['B'][r] = state.roles['B'][r];

        ++this.board[r1][c1].moved;
        if(attacker && defender)
        {
            if(!this.board[r1][c1].role)
            {
                --this.roles[this.board[r1][c1].owner][attacker];
                this.board[r1][c1].role = attacker;
            }
            if(!this.board[r2][c2].role)
            {
                --this.roles[this.board[r2][c2].owner][defender];
                this.board[r2][c2].role = defender;
            }

            var d = defeats(attacker, defender);
            if(d > 0)
            {
                this.board[r2][c2] = this.board[r1][c1];
                this.board[r1][c1] = new Piece();
            }
            if(d < 0)
            {
                this.board[r1][c1] = new Piece();
            }
            if(d == 0)
            {
                this.board[r1][c1] = new Piece();
                this.board[r2][c2] = new Piece();
            }
        }
        else
        {
            this.board[r2][c2] = this.board[r1][c1];
            this.board[r1][c1] = new Piece();
        }
    }
    else
    {
        this.board = new Array(
            new Array( new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R') ),
            new Array( new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R') ),
            new Array( new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R'), new Piece('R') ),
            new Array( new Piece(),    new Piece('N'), new Piece(),    new Piece(),    new Piece(),    new Piece('N'), new Piece()    ),
            new Array( new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B') ),
            new Array( new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B') ),
            new Array( new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B'), new Piece('B') ) );
    
        this.roles = new Object();
        this.roles['R'] = new Object(); this.roles['B'] = new Object();
        this.roles['R']['B'] = 4; this.roles['B']['B'] = 4;
        this.roles['R']['G'] = 1; this.roles['B']['G'] = 1;
        this.roles['R']['O'] = 2; this.roles['B']['O'] = 2;
        this.roles['R']['L'] = 3; this.roles['B']['L'] = 3;
        this.roles['R']['M'] = 4; this.roles['B']['M'] = 4;
        this.roles['R']['R'] = 5; this.roles['B']['R'] = 5;
        this.roles['R']['S'] = 1; this.roles['B']['S'] = 1;
        this.roles['R']['V'] = 1; this.roles['B']['V'] = 1;
    }

    this.apply = function() {
        for(p = 0; p < players.length; ++p)
            for(r = 0; r < roles.length; ++r)
            {
                player = players[p];
                role   = roles[r];
                document.getElementById(player + role + '_left').innerHTML =  this.roles[player][role];
            }
            
        for(r = 0; r < this.board.length; ++r)
            for(c = 0; c < this.board[r].length; ++c)
            {
                if(this.board[r][c].owner)
                {
                    if(this.board[r][c].owner == 'N')
                    {
                        type = 'N';
                    }
                    else
                    {
                        type = this.board[r][c].owner;
                        if(this.board[r][c].role)
                            type += this.board[r][c].role;
                        if(!this.board[r][c].moved)
                            type += 'x';
                    }
                }
                else
                {
                    type = 'E';
                }
                document.getElementById('field_' + r + c).src = 'img/' + type + '.png';
            }
    }
    
}

function add_to_history(label, state)
{
    var index = hist.length;
    var suffix = ' [' + ((index + index % 2) / 2) + ']';
    hist.options[index] = new Option(label + suffix);
    hist.options[index].state = last_state = state;
    state.apply()
}

function execute(move, r1, c1, r2, c2, role1, role2)
{
    var new_state  = new State(last_state, r1, c1, r2, c2, role1, role2);
    add_to_history(move, new_state);
}

function message(line)
{
    mesg.innerHTML += line + "\n";
}

function comment(source, line)
{
    var last_state = hist.options[hist.length - 1].state;
    document.getElementById(source[0] + '_comments').value += line + "\n";
}

hist.onchange = function() {
    if(hist.selectedIndex >= 0)
        hist.options[hist.selectedIndex].state.apply();
}

hist.length = 0;
add_to_history('Start', new State());
