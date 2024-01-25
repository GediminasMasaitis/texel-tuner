#ifndef CONFIG_H
#define CONFIG_H 1

#include<cstdint>
//#include "engines/toy.h"
//#include "engines/toy_tapered.h"
#include "engines/fourku.h"

#define TAPERED 1
//using TuneEval = Toy::ToyEval;
//using TuneEval = Toy::ToyEvalTapered;
using TuneEval = Fourku::FourkuEval;
constexpr int32_t data_load_thread_count = 4;
constexpr int32_t thread_count = 12;
constexpr tune_t preferred_k = 2.1;
constexpr int32_t max_epoch = 5001;
constexpr bool retune_from_zero = true;
constexpr bool enable_qsearch = false;
constexpr bool filter_in_check = false;
constexpr tune_t initial_learning_rate = 1;
constexpr int32_t learning_rate_drop_interval = 10000;
constexpr tune_t learning_rate_drop_ratio = 1;
constexpr bool print_data_entries = false;
constexpr int32_t data_load_print_interval = 10000;

#endif // !CONFIG_H
