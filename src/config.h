#ifndef CONFIG_H
#define CONFIG_H 1

#include<cstdint>

//#include "engines/toy.h"
//#include "engines/toy_tapered.h"
//#include "engines/fourku.h"
#include "engines/fourkdotcpp.h"

//using TuneEval = Toy::ToyEval;
//using TuneEval = Toy::ToyEvalTapered;
using TuneEval = Fourkdotcpp::FourkdotcppEval;
constexpr int32_t data_load_thread_count = 4;
constexpr int32_t thread_count = 12;
constexpr static bool print_data_entries = false;
constexpr static int32_t data_load_print_interval = 10000;


#endif // !CONFIG_H
