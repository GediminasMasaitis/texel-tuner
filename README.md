# Texel tuner

This project is based on the linear evaluation ideas described in https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf.

## Plugging in your evaluation
To add your own evaluation it is required to transform your evaluation into a linear system.

For each position in the training dataset, the evaluation should count the occurances of each evaluation term, and return a `coefficients_t` object where each entry is the count oftimes an evaluation term has been userd per-side.

For a new engine it's required to implement a class with 3 functions:

```cpp
    class YourEval
    {
    public:
        static parameters_t get_initial_parameters();
        static EvalResult get_fen_eval_result(const std::string& fen);
        static void print_parameters(const parameters_t& parameters);
    };
```

### get_initial_parameters
This function retrieves the initial parameters of the evaluation in a vector form. Each parameter is an entry in `parameters_t`.

If the ealuation used is tapered, each entry is an `std::array<double, 2>`, where the first value is the midgame value and the second value is the endgame value.

If the evaluationis not tapered, the enty is just the plain value of the parameter used in the evaluation.

### get_fen_eval_result
This function gets the linear coefficients for each parameter given a position in a FEN form. Instead of counting a score, count how many times an evaluation term was used for each side in a position in the data set.

The input is a FEN string because it's unreasonable to expect each engine to have the same structure for a board representation, so FEN parsing is left to the engine implementation.

Additionaly, you may return the score, this is used to tune around other existing parameters. 

### print_parameters
This function prints the results of the tuning, the input is given as a vector of the tuned parameters, and it's up to the engine to ptint it as as it desires.

### Adding your engine
Edit `config.h` to point `TuneEval` to your evaluation class. Edit thread_count to be equivalent to what you're comfortable with. If you're using a tapered evaluation, set `#define TAPERED 1` in both `base.h` and `config.h`, otherwhise set both to `#define TAPERED 0`.

## Build
Cmake / make // TODO


## Data sources
This tuner does not provide data sources. Own data source must be used.
Each data source is a file where each line is a FEN and a WDL. (WDL = Win/Draw/Loss).

Supported formats:
1. Line containing WDL. Markers are `1.0`, `0.5`, and `0.0` for win, draw loss. Example:
```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1; [1.0]
```
2. Line containing match result. Markers are `1-0`, `1/2-1/2`, and `0-1` for win, draw loss. Example:
```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1; [1-0]
```
3. Line containing probability to win. Pretty much equivalent to option nr.1, except it doesn't need to be stricly win/draw/loss scores . Example:
```
rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1; [0.6]
```

The brackets are not necessary, the WDL only has to be found somewhere in the line.

## Usage
Create a csv formatted file with data sources. `#` marks a comment line.

Columns:
1. Path to data file.
2. Whether or not the WDL is from the side playing. 1 = yes, 0 = no,
3. Limit of how may FENs to load from this data source (sequentially). 0 = unlimited

Example:
```
# Path, WDL from side playing, position limit
C:\Data1.epd,0,0
C:\Data2.epd,0,900000
```

Build the project and run `tuner.exe sources.csv` where sources.csv is the data source file mentioned previously.