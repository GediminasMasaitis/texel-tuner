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
constexpr double preferred_k = 1.8;
constexpr int32_t max_epoch = 5001;
constexpr bool retune_from_zero = true;
constexpr bool enable_qsearch = true;
constexpr bool print_data_entries = false;
constexpr int32_t data_load_print_interval = 10000;

#endif // !CONFIG_H
