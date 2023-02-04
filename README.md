# Texel tuner

This project is based on the linear evaluation ideas described in https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf.

## Usage:
To add your own evaluation it is required to transform your evaluation into a linear system.

For each position in the training dataset, the evaluation should count the occurances of each evaluation term, and return a `coefficients_t` object where each entry is the count oftimes an evaluation term has been userd per-side.

For a new engine it's required to implement a class with 3 functions:

```cpp
    class YourEval
    {
    public:
        static parameters_t get_initial_parameters();
        static coefficients_t get_fen_coefficients(const std::string& fen);
        static void print_parameters(const parameters_t& parameters);
    };
```

### get_initial_parameters
This function retrieves the initial parameters of the evaluation in a vector form. Each parameter is an entry in `parameters_t`.

If the ealuation used is tapered, each entry is an `std::array<double, 2>`, where the first value is the midgame value and the second value is the endgame value.

If the evaluationis not tapered, the enty is just the plain value of the parameter used in the evaluation.

### get_fen_coefficients
This function gets the linear coefficients for each parameter given a position in a FEN form. Instead of counting a score, count how many times an evaluation term was used for each side in a position in the data set.

The input is a FEN string because it's unreasonable to expect each engine to have the same structure for a board representation, so FEN parsing is left to the engine implementation.

### print_parameters
This function prints the results of the tuning, the input is given as a vector of the tuned parameters, and it's up to the engine to ptint it as as it desires.

### Plugging in your evaluation
Edit`config.h` to point `TuneEval` to your evaluation class. Edit thread_count to be equivalent to what you're comfortable with. If you're using a tapered evaluation, set `#define TAPERED 1` in both `base.h` and `config.h`, otherwhise set both to `#define TAPERED 0`.