#ifndef CONFIG_H
#define CONFIG_H 1

#include<cstdint>
#include "engines/toy.h"
#include "engines/toy_tapered.h"

#define TAPERED 1
using TuneEval = Toy::ToyEvalTapered;
constexpr int32_t thread_count = 12;

#endif // !CONFIG_H
