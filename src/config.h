#ifndef CONFIG_H
#define CONFIG_H 1

#include<cstdint>
#include "engines/fourku.h"
#include "engines/toy.h"
#include "engines/toy_tapered.h"

#define TAPERED 1
//using TuneEval = Toy::ToyEval;
//using TuneEval = Toy::ToyEvalTapered;
using TuneEval = Fourku::FourkuEval;
constexpr int32_t thread_count = 12;
constexpr double preferred_k = 0;
constexpr int32_t max_epoch = 50000;
constexpr bool retune_from_zero = true;
constexpr bool enable_qsearch = false;
constexpr bool print_data_entries = false;
constexpr int32_t data_load_print_interval = 100000;

#endif // !CONFIG_H
