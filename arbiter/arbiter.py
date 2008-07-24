#!/usr/bin/env python

from os     import popen3
from sys    import argv, stdout, stderr, exit
from thread import start_new_thread

#
#  Game constants
#

roles_initial = {
    'flag':       1,    'bomb':       4,    'general':    1,    'colonel':    2,
    'lieutenant': 3,    'minor':      4,    'rider':      5,    'spy':        1 }

symbols_roles = {
    'V': 'flag',        'B': 'bomb',        'G': 'general',     'O': 'colonel', 
    'L': 'lieutenant',  'M': 'minor',       'R': 'rider',       'S': 'spy' }

roles_symbols = {
    'flag': 'V',        'bomb': 'B',        'general': 'G',     'colonel': 'O', 
    'lieutenant': 'L',  'minor': 'M',       'rider': 'R',       'spy': 'S' }
                           
ranking = [
    'bomb', 'general', 'colonel', 'lieutenant',
    'minor', 'rider', 'spy', 'flag' ]

movable = [
    'general', 'colonel', 'lieutenant', 'minor', 'rider', 'spy' ]

#
#  Exceptions
#

class PlayerException(Exception):
    def __init__(self, value, player=None):
        self.value  = value
        self.player = None
        
    def __str__(self):
        if self.player:
            return '%s caused exception: %s.' % (self.player, self.description())
        else:
            return str(self.value)


class InvalidMove(PlayerException):
    "Player performed an invalid move."
    
    def __init__(self, value, move, player=None):
        self.value  = value
        self.move   = move
        self.player = player

    def description(self):
        return 'invalid move; %s (declared move was: %s)' % (self.value, format_move(self.move))

class InvalidRole(PlayerException):
    "Player chose an invalid role."
    
    def __init__(self, value, role, move, player=None):
        self.value  = value
        self.role   = role
        self.move   = move
        self.player = player

    def description(self):
        return 'invalid role; %s (declared role was: %s; move was: %s)' % \
               (self.value, self.role, format_move(self.move))
        
    
class InvalidSyntax(PlayerException):
    "Syntax error in player communication"
    
    def description(self):
        return 'invalid syntax; %s' % self.value


class NotResponding(PlayerException):
    "Pipe to player process died or timed out"
    
    def description(self):
        return 'process not responding; %s' % self.value


def defeats(attacker, defender):
    """Determines wether an attacker defeats a defender. 
       Both attacker and defender must be legal role identifiers.
       Returns 1 if the attacker defeats the defender, 0 if the attacker and
       defender are tied, and -1 if the defender defeats the attacker."""

    if attacker == defender:
        return 0
    if attacker == 'spy' and defender == 'general':
        return 1
    if attacker == 'minor' and defender == 'bomb':
        return 1
    if ranking.index(attacker) < ranking.index(defender):
        return 1
    else:
        return -1
        

def parse_role(str):
    """Parses a role string (consisting of only a role symbol) and returns the
       respective role identifier."""

    if str in symbols_roles:
        return symbols_roles[str]
    else:
        raise InvalidSyntax('expected a role symbol, but got "%s"' % str)
        

def format_role(role):
    if not role:
        return '-'
    else:
       return roles_symbols[role]


def parse_move(str):
    """Parses a move string (for example "a1-b1") and returns the move a tuple
       consisting of the source and destination fields, each of which is
       described by a tuple containing the 0-based row and column indices."""
     
    if len(str) <> 5:
        raise InvalidSyntax('expected a move, but got "%s"' % str)
    c1, r1, dash, c2, r2 = str
    if not c1 in "abcdefg":
        raise InvalidSyntax('expected a column identifier, but got "%s" (move was "%s")' % (c1, str))
    if not r1 in "1234567":
        raise InvalidSyntax('expected a row identifier, but got "%s" (move was "%s")' % (r2, str))
    if dash <> '-':
        raise InvalidSyntax('expected a dash, but got "%s" (move was "%s")' % (dash, str))
    if not c2 in "abcdefg":
        raise InvalidSyntax('expected a column identifier, but got "%s" (move was "%s")' % (c2, str))
    if not r2 in "1234567":
        raise InvalidSyntax('expected a row identifier, but got "%s" (move was "%s")' % (r2, str))
    r1 = ord(r1) - ord('1')
    c1 = ord(c1) - ord('a') 
    r2 = ord(r2) - ord('1')
    c2 = ord(c2) - ord('a')
    return (r1, c1), (r2, c2)


def format_move(move):
    """Returns the canonical string format for the given move."""
    
    (r1, c1), (r2, c2) = move
    
    return chr(ord('a') + c1) + chr(ord('1') + r1) + '-' + chr(ord('a') + c2) + chr(ord('1') + r2) 


def run_error_thread(name, error):
    while True:
        line = error.readline()
        if line:
            print '%s> %s' % (name.rjust(6), line.rstrip())
        else:
            break    


class Player:
    """Represents a player in the game and provides methods for communicating
       with the player."""

    def __init__(self, name, input, output, error, pieces = 21):
        self.name = name
        self.input = input
        self.output = output
        self.unassigned = dict(roles_initial)
        self.uu_roles = 0
        for role, count in self.unassigned.items():
            if role not in movable:
                self.uu_roles = self.uu_roles + count
        self.uu_pieces = pieces
        start_new_thread(run_error_thread, (name, error))
        
    def __str__(self):
        return self.name

    def next_line(self):
        while True:
            line = self.input.readline()
            if line.endswith('\n'):
                return line.rstrip('\n')
            else:
                return None

    def next_word(self):
        line = self.next_line()
        if line == None:
            raise NotResponding('unexpected end of input', self)
        else:
            return line

    def write(self, data):
        self.output.write(data)
        self.output.flush()

    def start(self):
        try:
            self.write('Start\n')
        except IOError, e:
            raise NotResponding(e, self)

    def cancel(self):
        try:
            self.write('X')
        except IOError, e:
            pass
    
    def write_move(self, move):
        try:
            self.write('%s\n' % format_move(move))
        except IOError, e:
            raise NotResponding(e, self)
       
    def write_role(self, role):
        try:
            self.write('%s\n' % format_role(role))
        except IOError, e:
            raise NotResponding(e, self)
        
    def read_move(self):
        try:
            return parse_move(self.next_word())
        except IOError, e:
            raise NotResponding(e, self)
        except PlayerException, e:
            e.player = self
            raise e

    def read_role(self):
        try:
            return parse_role(self.next_word())
        except IOError, e:
            raise NotResponding(e, self)
        except PlayerException, e:
            e.player = self
            raise e
            
    def close(self):
        self.next_line()
    
    def excess_uu(self):
        return self.uu_pieces > self.uu_roles

    

class Piece:
    def __init__(self, owner):
        self.owner = owner
        self.role  = None
        self.moved = False


class Game:

    def __init__(self, p1, p2):
        self.board = [
            [ Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1) ],
            [ Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1) ],
            [ Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1), Piece(p1) ],
            [ None,      Piece(None), None,    None,      None,      Piece(None), None    ],
            [ Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2) ],
            [ Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2) ],
            [ Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2), Piece(p2) ] ]
        self.history = [ ]
        self.current = p1
        self.other   = p2

    def over(self):
    
        # If your flag has been captured, you lose the game.
        if self.current.unassigned['flag'] == 0:
            self.winner = self.other
            self.loser  = self.current
            print >> stderr, "%s's flag was taken." % self.current
            return True
        if self.other.unassigned['flag'] == 0:
            self.winner = self.current
            self.loser  = self.other
            print >> stderr, "%s's flag was taken." % self.other
            return True
            
        # After 200 turns for each player, it is a tie.
        if len(self.history) >= 400:
            self.loser = self.winner = None            
            print >> stderr, 'Each player has played 200 turns.'
            return True
       
        for row in range(7):
            for col in range(7):
                src = row, col
                src_piece = self.get_piece(src)
                if src_piece and src_piece.owner == self.current and \
                   ( src_piece.role in movable or src_piece.moved or
                     (not src_piece.role and self.current.excess_uu()) ):

                    for dst in [ (row - 1, col), (row, col - 1), (row + 1, col), (row, col + 1) ]:
                        row, col = dst
                        if row >= 0 and row < 7 and col >= 0 and col < 7:
                            dst_piece = self.get_piece(dst)
                            if not dst_piece or dst_piece.owner == self.other:
                                # valid move found!
                                return False

        # If you have no valid moves left, you lose.
        self.winner = self.other
        self.loser  = self.current
        print >> stderr, '%s has no valid moves left.' % self.current
        return True
               
        
    def get_piece(self, pos):
        row, col = pos
        return self.board[row][col]

    def set_piece(self, pos, piece):
        row, col = pos
        self.board[row][col] = piece        

    def run(self):
        self.current.start()
        try:

            while not self.over():
           
                # Read next move
                move = src, dst = self.current.read_move()
                
                # Check fields
                if abs(src[0] - dst[0]) + abs(src[1] - dst[1]) <> 1:
                    raise InvalidMove('source and destination fields are not adjacent', move, self.current)

                # Check source piece
                src_piece = self.get_piece(src)
                if not src_piece or not src_piece.owner:
                    raise InvalidMove('may not move from an empty field', move, self.current)
                if src_piece.owner <> self.current:
                    raise InvalidMove('may not move with an enemy piece', move, self.current)
                if src_piece.role and src_piece.role not in movable:
                    raise InvalidMove('%s may not move' % src_piece.role, move, self.current)
                if not self.current.excess_uu() and not src_piece.role and not src_piece.moved:
                    print 'uu_pieces', self.current.uu_pieces
                    print 'uu_roles',  self.current.uu_roles
                    raise InvalidMove( 'may not move a piece that must be assigned an unmovable role',
                                       move, self.current )

                # Update state for moved piece
                if not src_piece.moved and not src_piece.role:
                    self.current.uu_pieces = self.current.uu_pieces - 1
                src_piece.moved = True

                # Check destination piece
                dst_piece = self.get_piece(dst)
                if dst_piece and dst_piece.owner == None:
                    raise InvalidMove('may not attack a neutral piece', move, self.current)
                if dst_piece and dst_piece.owner == self.current:
                    raise InvalidMove('may not attack own pieces', move, self.current)
               
                # Determine if fighting occurs
                if dst_piece:
                
                    # Inform other player of move
                    self.other.write_move(move)

                    # Read and check attacking role
                    attacker = self.current.read_role()
                    if attacker not in movable:
                        raise InvalidRole('attacker must be movable', attacker, move, self.current)
                    if src_piece.role:
                        if src_piece.role <> attacker:
                            raise InvalidRole( 'may not change role for attacking %s' %
                                               src_piece.role, attacker, move, self.current )
                    else:
                        if self.current.unassigned[attacker] == 0:
                            raise InvalidRole('role unavailable', attacker, move, self.current)
                        else:
                            src_piece.role = attacker
                            self.current.unassigned[attacker] = self.current.unassigned[attacker] - 1
                        
                        
                    # Read and check defending role
                    defender = self.other.read_role()
                    if dst_piece.moved and defender not in movable:
                        raise InvalidRole( 'may not assign unmovable role to defender that has already moved',
                                           defender, move, self.other )
                    if dst_piece.role:
                        if dst_piece.role <> defender:
                            raise InvalidRole( 'may not change role for defending %s' %
                                               dst_piece.role, defender, move, self.other )
                    else:
                        if not dst_piece.moved:
                            if defender in movable:
                                if not self.other.excess_uu():
                                    raise InvalidRole( 'may not assign a movable role to this piece',
                                                        defender, move, self.other )
                            else:
                                self.other.uu_roles = self.other.uu_roles - 1
                            self.other.uu_pieces = self.other.uu_pieces - 1
                            
                        if self.other.unassigned[defender] == 0:
                            raise InvalidRole('role unavailable', defender, move, self.other)
                        else:
                            dst_piece.role = defender
                            self.other.unassigned[defender] = self.other.unassigned[defender] - 1
                            
                        
                    # Inform other players of roles chosen
                    self.current.write_role(defender)
                    self.other.write_role(attacker)

                    # Fight!
                    d = defeats(attacker, defender)
                    if d > 0:
                        self.set_piece(src, None)
                        self.set_piece(dst, src_piece)
                    elif d < 0:
                        self.set_piece(src, None)
                    else:
                        self.set_piece(src, None)
                        self.set_piece(dst, None)

                else:
                    # Check if this move reverses last move for this player
                    if len(self.history) >= 2 and \
                       self.history[len(self.history) - 2] == ((dst, src), None, None):
                       
                        raise InvalidMove('may not do the reverse of your previous move', move, self.current)

                    # Inform other player of move
                    self.other.write_move(move)

                    attacker, defender = None, None
                    self.set_piece(src, None)
                    self.set_piece(dst, src_piece)

                # Print out turn (consisting of select move and assigned roles)
                print >> stdout, format_move(move), format_role(attacker), format_role(defender)
                stdout.flush()

                # Add turn to game history
                self.history.append((move, attacker, defender))
        
                # Swap players
                next_player  = self.current
                self.current = self.other
                self.other   = next_player

            player1.close()
            player2.close()

        except PlayerException, e:
            print >> stderr, e
            self.current.cancel()
            self.other.cancel()
            
            if e.player == self.current:
                self.winner = self.other
            else:
                self.winner = self.current

        if self.winner:
            print >> stderr, '%s won.' % self.winner
        else:
            print >> stderr, "It's a tie."
        stderr.flush()


if __name__ == '__main__':
    if len(argv) <> 3:
        print 'Usage: %s <command1> <command2>' % argv[0]
        exit()
    
    input, output, error = popen3(argv[1])
    player1 = Player('Red', output, input, error)
    input, output, error = popen3(argv[2])
    player2 = Player('Blue', output, input, error)
    game = Game(player1, player2)
    game.run()
