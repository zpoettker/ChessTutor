Readme · MD
# Chess Move Tutor (working title)
 
A browser-based chess app that reviews each move as you play: after you move, it shows what the best move was and explains why. Nearly all of the logic is written in C++ and compiled to WebAssembly, so the app runs entirely in the visitor's browser and can be hosted for free as a static site.
 
This document is the project brief. If you are an AI coding assistant helping build this, read the whole file before writing code, and follow the section "Instructions for the AI assistant" at the end.
 
## Goals
 
1. **Portfolio project.** The author is a senior Computer Science student applying for software engineering roles. The project should show real C++ skill: clean design, tests, and measured performance.
2. **Live demo.** A recruiter can open a link and play immediately, with nothing to install.
3. **Mostly C++.** Chess rules, search, evaluation, and explanation logic live in C++. JavaScript/TypeScript is limited to drawing the board and glue code.
4. **Useful explanations.** The app does not only name the best move; it gives a concrete reason whenever it can find one.
## Non-goals
 
- Beating Stockfish. The custom engine only needs to be a reasonable opponent.
- Accounts, online multiplayer, or any backend server.
- Explaining deep positional ideas. Explanations start with concrete tactics.
## How the pieces fit together
 
There are three parts, with different jobs:
 
| Part | Written by | Job |
|---|---|---|
| Chess core (C++) | This project | Board, rules, legal moves, game status |
| Custom engine (C++) | This project | The opponent the user plays against |
| Stockfish (WebAssembly build) | Third party | The judge: finds the true best move and scores moves |
 
Stockfish is used as the judge because the custom engine will not be strong enough to be trusted for "best move" feedback.
 
```
Browser main thread
  TypeScript UI  ──  chessground board, move list, explanation panel
       │
       ├── calls ──►  Chess core + explanation logic   (C++ → WebAssembly)
       │
       ├── messages ──►  Web Worker: custom engine     (C++ → WebAssembly)
       │
       └── messages ──►  Web Worker: Stockfish         (prebuilt WebAssembly, UCI text protocol)
```
 
Both engines run in Web Workers so that searching never freezes the page.
 
## Tech stack
 
| Area | Choice | Notes |
|---|---|---|
| Language | C++20 | |
| Build | CMake | One native target, one Emscripten target |
| WebAssembly toolchain | Emscripten, with Embind for bindings | |
| Tests | Catch2 (or GoogleTest) | Run natively |
| Frontend | TypeScript + Vite | Keep it thin |
| Board UI | chessground | Drag and drop, highlights, arrows |
| Stockfish | stockfish.js (`stockfish` npm package) | Start with a single-threaded build |
| Hosting | Any static host | Cloudflare Pages or Netlify if custom headers are needed later |
| License | GPL-3.0 | See "Licensing" |
 
Before installing anything, check the current documentation for Emscripten, chessground, and stockfish.js, because build names and APIs change between versions.
 
## Repository layout
 
```
/core          C++ chess core: board, moves, move generation, FEN, game status
/engine        C++ search and evaluation
/explain       C++ explanation logic (tactical detectors, move classification)
/bindings      Embind wrappers; the only C++ that includes Emscripten headers
/tests         Native unit tests and perft tests
/bench         Native benchmarks (perft speed, nodes per second)
/web           TypeScript app: UI, workers, Stockfish wrapper
CMakeLists.txt
```
 
**Hard rule:** `/core`, `/engine`, and `/explain` must not include any Emscripten or browser-specific code. They must compile and run natively so tests and benchmarks can run from the command line.
 
## Build phases
 
Build in this order. Each phase has acceptance criteria; do not start the next phase until they pass.
 
### Phase 0: Scaffolding
 
- CMake project with a native build and an Emscripten build.
- Test framework wired up with one passing placeholder test.
- Vite app that loads a trivial WebAssembly function and shows its result.
**Done when:** `cmake --build` works natively, the tests run, and the web page calls a C++ function.
 
### Phase 1: Chess core
 
- Board representation using bitboards (one 64-bit integer per piece type and color).
- FEN parsing and output.
- Legal move generation, including castling, en passant, and promotion.
- Make and unmake move.
- Game status: checkmate, stalemate, fifty-move rule, threefold repetition, insufficient material.
- A `perft` function (counts leaf positions to a given depth).
Start with simple sliding-piece attack generation. Magic bitboards are an optional later optimization, to be done only with a benchmark showing the before and after.
 
**Done when:** every perft total below matches exactly.
 
| Position (FEN) | Depth | Expected nodes |
|---|---|---|
| Start position | 1 | 20 |
| | 2 | 400 |
| | 3 | 8,902 |
| | 4 | 197,281 |
| | 5 | 4,865,609 |
| `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1` | 1 | 48 |
| | 2 | 2,039 |
| | 3 | 97,862 |
| | 4 | 4,085,603 |
| `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1` | 1 | 14 |
| | 2 | 191 |
| | 3 | 2,812 |
| | 4 | 43,238 |
| | 5 | 674,624 |
| `r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1` | 1 | 6 |
| | 2 | 264 |
| | 3 | 9,467 |
| | 4 | 422,333 |
| `rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8` | 1 | 44 |
| | 2 | 1,486 |
| | 3 | 62,379 |
| | 4 | 2,103,487 |
 
These are the standard perft test positions; confirm the totals against the Chess Programming Wiki "Perft Results" page when setting up the tests.
 
### Phase 2: Playable web UI
 
- Embind bindings exposing a `Game` object (sketch below).
- chessground board that only allows legal moves.
- Promotion picker, move list, undo, new game, flip board.
- Game-over messages.
**Done when:** two humans can play a full legal game in the browser on one screen.
 
Binding sketch (adjust as needed):
 
```cpp
class Game {
public:
    bool        loadFen(const std::string& fen);
    std::string fen() const;
    std::vector<std::string> legalMoves() const;   // UCI strings, e.g. "e2e4", "e7e8q"
    bool        makeMove(const std::string& uci);
    bool        undo();
    std::string status() const;                    // "ongoing", "checkmate", "stalemate", ...
};
```
 
### Phase 3: Stockfish and move review
 
- Load Stockfish in a Web Worker and wrap it in a small TypeScript class that sends UCI commands and parses replies.
- After each user move, analyse the position **before** the move:
  1. `position fen <fen>` then `go depth <N>` to get the best move, its score, and the principal variation (the expected line).
  2. `go depth <N> searchmoves <user move>` to get the score of the move actually played, from the same side's point of view.
- Classify the move by how much evaluation it lost.
- Show: the classification, the best move as an arrow on the board, an evaluation bar, and both lines.
Starting thresholds, in centipawns (hundredths of a pawn) lost; keep them configurable:
 
| Loss | Label |
|---|---|
| Under 50 | Good |
| 50 to 99 | Inaccuracy |
| 100 to 199 | Mistake |
| 200 or more | Blunder |
 
Mate scores need separate handling (missing a forced mate, or allowing one, is always at least a mistake).
 
**Done when:** after every move, the app shows a label, the best move, and the evaluation, without freezing the page.
 
### Phase 4: Explanations
 
The explanation logic is C++ in `/explain`. It takes the position, the move played, the best move, and both principal variations, and returns structured reasons that the UI turns into sentences.
 
```cpp
struct Explanation {
    std::string classification;        // "good", "inaccuracy", "mistake", "blunder"
    int         centipawnLoss;
    std::string bestMove;              // UCI
    std::vector<std::string> bestLine;
    std::vector<std::string> playedLine;
    std::vector<Reason>      reasons;  // ordered, most important first
};
```
 
Detectors, in rough priority order:
 
1. **Missed mate**: the best line is a forced mate.
2. **Allowed mate**: the played line lets the opponent force mate.
3. **Hung piece**: after the played move, a piece can be captured for a net material loss. Use static exchange evaluation (adding up the captures on one square) to decide.
4. **Missed free material**: the best move wins material that the played move does not.
5. **Missed fork, pin, or skewer**: the best move attacks two or more valuable targets.
6. **Walked into a fork, pin, or skewer**: the opponent's reply in the played line does so.
7. **Material by the end of the line**: play out both lines and compare material.
8. **Fallback**: when nothing concrete is found, show the two lines and the evaluation difference, and say plainly that the reason is positional.
Each detector gets unit tests built from hand-picked FEN positions.
 
**Done when:** the common tactical cases produce a correct, specific sentence, and the fallback never states a reason that is not supported by the lines.
 
### Phase 5: Custom engine
 
- Evaluation: material plus piece-square tables first; then mobility, king safety, and pawn structure.
- Search: iterative deepening, alpha-beta, move ordering (captures first, then killer moves), quiescence search, and a transposition table keyed by Zobrist hashes.
- Time management: search for a fixed time per move.
- Runs in its own Web Worker.
- Difficulty levels by limiting depth or time.
**Done when:** the user can play a full game against the engine, it never makes an illegal move, and it finds mate-in-two test positions.
 
Stretch: add a native UCI command-line front end so the engine can be tested against other engines.
 
### Phase 6: Polish and deploy
 
- Benchmarks in `/bench`: perft nodes per second and search nodes per second, with results recorded in this README.
- A public-facing README section: screenshots, design decisions, and what each optimization gained.
- Deploy as a static site and link it from the portfolio.
- Mobile-friendly layout.
## Stockfish in the browser: things to know
 
- A browser cannot launch a separate program, so Stockfish runs as a WebAssembly module in a Web Worker. Communication is still UCI text, sent as worker messages.
- Start with a **single-threaded** build. Multi-threaded builds need `SharedArrayBuffer`, which requires the `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` headers. Some free hosts do not allow custom headers.
- Engine files with the neural network can be tens of megabytes. Prefer a smaller build if one is available, load it after the page is interactive, and show a loading state.
- Use a modest fixed depth so results arrive in about a second on an ordinary laptop.
## Licensing
 
Stockfish and chessground are both licensed under GPL-3.0. Distributing them as part of this app most likely requires the whole project to be released under a GPL-compatible license, so this project uses GPL-3.0 and keeps its source public. Confirm the terms of each dependency before publishing. Do not copy code from Stockfish or any other engine into `/core`, `/engine`, or `/explain`; that code must be original.
 
## Instructions for the AI assistant
 
1. **Follow the phase order.** Finish and verify one phase before starting the next.
2. **Work in small steps.** Propose a short plan for each step, then implement it. Avoid dumping large amounts of code at once.
3. **Explain the reasoning.** The author needs to understand every part well enough to discuss it in an interview. Briefly explain each design decision and any non-obvious C++.
4. **Test as you go.** Add or update tests with every change to the core, engine, or explanation code. Run the tests and report the actual results.
5. **Measure before optimizing.** Do not add an optimization without a benchmark showing the result before and after.
6. **Ask before adding dependencies** beyond those listed in "Tech stack".
7. **Keep the core portable.** No Emscripten or browser code outside `/bindings` and `/web`.
8. **Check current docs** for Emscripten, chessground, and stockfish.js rather than relying on memory for build names, flags, or APIs.
9. **Say so when unsure.** If a perft total is wrong or a detector is unreliable, report it plainly instead of working around it.
## Status
 
- [ ] Phase 0: Scaffolding
- [ ] Phase 1: Chess core
- [ ] Phase 2: Playable web UI
- [ ] Phase 3: Stockfish and move review
- [ ] Phase 4: Explanations
- [ ] Phase 5: Custom engine
- [ ] Phase 6: Polish and deploy
 

