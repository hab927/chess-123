# Chess Movement

## Intro

For this assignment, I began by rewatching the Sebastian Lague Chess AI video. Specifically, I rewatched the part where he went over the offsets for the king pieces.

The second thing I did was use the provided Bitboard.h file to create bitboards for each piece on the board, and also separate ones for white pieces, black pieces, occupied, and empty squares. This helped me generate all of the moves for the three required pieces (pawns, knights, and kings). 

## Pawn Movement

Pawns were the first movement I tried. First I attempted to use a similar tactic to how the `Checkers.cpp` file approached it where it made use of the `getN()`, `getS()`, etc. directional helpers to get the correct squares. But that wasn't helpful because it would have made the bitboards useless. 

So I then got the idea to create new functions in the bitboard class. However, that was making my logic much more complicated than it needed to be; I could instead just store different values and unify everything instead of having a lot of branching `if` statements. 

So the final design came where I just used offsets and checked against empty squares and enemy pieces. For pawns, since they can only move forward, the directions they can move are affected by who they're being played by. Captures can only happen in that direction too, so I subtract/add accordingly.

## Knights and Kings Movement

The knights and kings followed very simply afterward, since they have offsets and only have to be checked against the empty pieces or squares occupied by enemy pieces.

## Rooks, Bishops, and Queens

For this assignment, I just rewatched the lectures covering Magic Bitboards. I kind of needed to remind myself how they worked again, but it was pretty much handled for me in the `MagicBitboards.h` file. Not wanting to just blindly plug in functions and have them magically work out for me, I wanted to get an understanding of the inner workings and made sure I knew what each one did first. So after that, I simply implemented it under my `canBitMoveFromTo` function, and now the moves are implemented alongside the above pieces.