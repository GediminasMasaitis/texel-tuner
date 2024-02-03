#ifndef CONFIG_H
#define CONFIG_H 1

#include<cstdint>

//#include "engines/toy.h"
//#include "engines/toy_tapered.h"
#include "engines/fourku.h"

//using TuneEval = Toy::ToyEval;
//using TuneEval = Toy::ToyEvalTapered;
using TuneEval = Fourku::FourkuEval;
constexpr int32_t data_load_thread_count = 2;
constexpr int32_t thread_count = 4;


#endif // !CONFIG_H
