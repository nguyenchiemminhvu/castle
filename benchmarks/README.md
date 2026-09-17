```
====================
Running benchmarks...
====================

----------------------------------------
Running castle_benchmark_algorithms
----------------------------------------
2026-09-17T15:48:23+09:00
Running /home/worker/study_space/castle/build/bin/castle_benchmark_algorithms
Run on (16 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 22528 KiB (x0)
Load Average: 11.25, 22.16, 30.68
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------------------------------
Benchmark                             Time             CPU   Iterations UserCounters...
---------------------------------------------------------------------------------------
algorithm_find/castle        2.7626e+16 ns      6413457 ns            1 items_per_second=39.7601k/s
algorithm_find/stl           2.7626e+16 ns      6479168 ns            1 items_per_second=39.3569k/s
algorithm_sort/castle        2.7626e+16 ns      6531569 ns            1 items_per_second=39.0412k/s
algorithm_sort/stl           2.7626e+16 ns      6550286 ns            1 items_per_second=38.9296k/s
algorithm_transform/castle   2.7626e+16 ns      6554429 ns            1 items_per_second=38.905k/s
algorithm_transform/stl      2.7626e+16 ns      6568616 ns            1 items_per_second=38.821k/s
algorithm_lower_bound/castle 2.7626e+16 ns      6583913 ns            1 items_per_second=151.885/s
algorithm_lower_bound/stl    2.7626e+16 ns      6598571 ns            1 items_per_second=151.548/s

----------------------------------------
Running castle_benchmark_callbacks
----------------------------------------
2026-09-17T15:48:23+09:00
Running /home/worker/study_space/castle/build/bin/castle_benchmark_callbacks
Run on (16 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 22528 KiB (x0)
Load Average: 11.25, 22.16, 30.68
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
--------------------------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------
function_free_call/castle              0.362 ns        0.362 ns   1943163549 items_per_second=2.76526G/s
function_free_call/stl                  2.94 ns         2.93 ns    238478647 items_per_second=340.782M/s
function_compile_time_call/castle      0.361 ns        0.361 ns   1948244484 items_per_second=2.7728G/s
function_compile_time_call/stl         0.361 ns        0.361 ns   1941308606 items_per_second=2.77224G/s
function_functor_call/castle           0.361 ns        0.361 ns   1941033988 items_per_second=2.76962G/s
function_functor_call/stl               2.21 ns         2.20 ns    314687109 items_per_second=453.539M/s
function_member_call/castle            0.360 ns        0.359 ns   1939948210 items_per_second=2.78198G/s
function_member_call/stl               0.377 ns        0.376 ns   1948674844 items_per_second=2.65743G/s
inplace_function_call/castle            1.83 ns         1.83 ns    388930474 items_per_second=546.22M/s
inplace_function_call/stl               1.82 ns         1.82 ns    390125254 items_per_second=550.958M/s
inplace_function_construct/castle       2.21 ns         2.21 ns    306261939 items_per_second=451.889M/s
inplace_function_construct/stl          2.17 ns         2.17 ns    310136773 items_per_second=461.624M/s

----------------------------------------
Running castle_benchmark_chrono
----------------------------------------
2026-09-17T15:48:35+09:00
Running /home/worker/study_space/castle/build/bin/castle_benchmark_chrono
Run on (16 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 22528 KiB (x0)
Load Average: 15.41, 22.48, 30.64
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------------------------------
Benchmark                             Time             CPU   Iterations UserCounters...
---------------------------------------------------------------------------------------
duration_add/castle               0.361 ns        0.361 ns   1951001941 items_per_second=2.76841G/s
duration_add/stl                  0.362 ns        0.362 ns   1941170127 items_per_second=2.76362G/s
duration_cast/castle              0.359 ns        0.359 ns   1951941102 items_per_second=2.78504G/s
duration_cast/stl                 0.364 ns        0.363 ns   1944669886 items_per_second=2.75141G/s
duration_update/castle       2.7626e+16 ns   4420247042 ns            1 items_per_second=0.452463/s
duration_update/stl          2.7626e+16 ns   4420267747 ns            1 items_per_second=0.452461/s
time_point_add/castle             0.364 ns        0.364 ns   1923717563 items_per_second=2.7481G/s
time_point_add/stl                0.370 ns        0.370 ns   1900282177 items_per_second=2.70599G/s
time_point_compare/castle         0.362 ns        0.361 ns   1929899537 items_per_second=2.76661G/s
time_point_compare/stl            0.366 ns        0.366 ns   1934118861 items_per_second=2.73462G/s
time_point_difference/castle      0.366 ns        0.366 ns   1936683599 items_per_second=2.73257G/s
time_point_difference/stl         0.365 ns        0.364 ns   1917652595 items_per_second=2.74414G/s
steady_clock_now/castle            21.2 ns         21.2 ns     33134350 items_per_second=47.2674M/s
steady_clock_now/stl               22.9 ns         22.9 ns     30689668 items_per_second=43.6351M/s
system_clock_now/castle            21.5 ns         21.5 ns     32944892 items_per_second=46.4342M/s
system_clock_now/stl               22.4 ns         22.4 ns     31014287 items_per_second=44.7186M/s

----------------------------------------
Running castle_benchmark_containers
----------------------------------------
2026-09-17T15:48:50+09:00
Running /home/worker/study_space/castle/build/bin/castle_benchmark_containers
Run on (16 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 22528 KiB (x0)
Load Average: 15.49, 22.15, 30.40
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
--------------------------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------
array_construct/castle                 0.361 ns        0.361 ns   1941884119 items_per_second=707.256G/s
array_construct/stl                    0.359 ns        0.359 ns   1950236778 items_per_second=709.657G/s
array_access/castle                     60.6 ns         60.6 ns      9796423 items_per_second=4.20908G/s
array_access/stl                        61.7 ns         61.7 ns     11134443 items_per_second=4.13466G/s
vector_construct/castle                0.564 ns        0.564 ns   1276472629 items_per_second=452.165G/s
vector_construct/stl                    19.4 ns         19.4 ns     36935049 items_per_second=13.1428G/s
vector_push_back/castle                  759 ns          766 ns       903729 items_per_second=332.766M/s
vector_push_back/stl                     751 ns          755 ns       905050 items_per_second=337.626M/s
vector_emplace_back/castle               753 ns          762 ns       918086 items_per_second=334.798M/s
vector_emplace_back/stl                  740 ns          748 ns       912999 items_per_second=341.041M/s
vector_pop_back/castle                   680 ns          686 ns       998408 items_per_second=371.707M/s
vector_pop_back/stl                      654 ns          661 ns      1028948 items_per_second=385.567M/s
vector_clear/castle                      511 ns          520 ns      1355814 items_per_second=490.011M/s
vector_clear/stl                         519 ns          524 ns      1317879 items_per_second=486.93M/s
hash_map_insert/castle                   940 ns          946 ns       707006 items_per_second=66.5951M/s
hash_map_insert/stl                     2718 ns         2724 ns       263130 items_per_second=23.1246M/s
hash_map_find_hit/castle          2.7627e+16 ns   2.0597e+10 ns            1 items_per_second=0.0485514/s
hash_map_find_hit/stl             2.7627e+16 ns   2.0597e+10 ns            1 items_per_second=0.0485514/s
hash_map_find_miss/castle         2.7627e+16 ns   2.0597e+10 ns            1 items_per_second=0.0485514/s
hash_map_find_miss/stl            2.7627e+16 ns   2.0597e+10 ns            1 items_per_second=0.0485513/s
hash_map_erase/castle                    954 ns          959 ns       708057 items_per_second=65.7223M/s
hash_map_erase/stl                      2251 ns         2253 ns       316255 items_per_second=27.9687M/s
hash_set_insert/castle                   953 ns          960 ns       720041 items_per_second=65.6522M/s
hash_set_insert/stl                     3160 ns         3168 ns       234256 items_per_second=19.8883M/s
hash_set_contains/castle          2.7627e+16 ns   2.7372e+10 ns            1 items_per_second=0.0365331/s
hash_set_contains/stl             2.7627e+16 ns   2.7372e+10 ns            1 items_per_second=0.036533/s
hash_set_erase/castle                    926 ns          930 ns       753331 items_per_second=67.7261M/s
hash_set_erase/stl                      2318 ns         2321 ns       304469 items_per_second=27.1405M/s
tree_map_insert/castle                  2180 ns         2191 ns       326264 items_per_second=28.7605M/s
tree_map_insert/stl                     2672 ns         2681 ns       263124 items_per_second=23.4947M/s
tree_map_find/castle              2.7627e+16 ns   3.4491e+10 ns            1 items_per_second=0.0289933/s
tree_map_find/stl                 2.7627e+16 ns   3.4491e+10 ns            1 items_per_second=0.0289933/s
tree_map_lower_bound/castle       2.7627e+16 ns   3.4491e+10 ns            1 items_per_second=0.0289933/s
tree_map_lower_bound/stl          2.7627e+16 ns   3.4491e+10 ns            1 items_per_second=0.0289933/s
tree_map_erase/castle                   1593 ns         1593 ns       453230 items_per_second=39.5511M/s
tree_map_erase/stl                      2892 ns         2890 ns       231994 items_per_second=21.7992M/s
tree_set_insert/castle                  2441 ns         2449 ns       284195 items_per_second=25.7219M/s
tree_set_insert/stl                     2671 ns         2676 ns       263196 items_per_second=23.5426M/s
tree_set_find/castle              2.7627e+16 ns   4.2170e+10 ns            1 items_per_second=0.0237136/s
tree_set_find/stl                 2.7627e+16 ns   4.2170e+10 ns            1 items_per_second=0.0237135/s
tree_set_lower_bound/castle       2.7627e+16 ns   4.2170e+10 ns            1 items_per_second=0.0237135/s
tree_set_lower_bound/stl          2.7627e+16 ns   4.2170e+10 ns            1 items_per_second=0.0237135/s
tree_set_erase/castle                   1629 ns         1631 ns       438926 items_per_second=38.6274M/s
tree_set_erase/stl                      2905 ns         2911 ns       242242 items_per_second=21.6414M/s
string_construct/castle                 59.4 ns         59.4 ns     12013526 items_per_second=16.835M/s
string_construct/stl                    47.9 ns         47.9 ns     14226329 items_per_second=20.8873M/s
string_append/castle                     522 ns          530 ns      1351004 items_per_second=124.508M/s
string_append/stl                        532 ns          537 ns      1348239 items_per_second=122.811M/s
string_compare_equal/castle             6.77 ns         6.77 ns    103375329 items_per_second=9.75532G/s
string_compare_equal/stl                6.59 ns         6.58 ns    104150657 items_per_second=10.0249G/s
string_find/castle                      11.4 ns         11.4 ns     61623165 items_per_second=5.79897G/s
string_find/stl                         23.9 ns         23.9 ns     30692552 items_per_second=2.76469G/s
string_view_construct/castle           0.783 ns        0.783 ns    884372536 items_per_second=1.27769G/s
string_view_construct/stl              0.806 ns        0.805 ns    966691472 items_per_second=1.24163G/s
string_view_find/castle                0.363 ns        0.363 ns   1940102543 items_per_second=193.022G/s
string_view_find/stl                   0.359 ns        0.359 ns   1939943253 items_per_second=194.816G/s
string_view_find_substring/castle       11.5 ns         11.5 ns     62309907 items_per_second=6.10712G/s
string_view_find_substring/stl          11.2 ns         11.2 ns     65247109 items_per_second=6.23813G/s
string_view_compare/castle             0.362 ns        0.362 ns   1897582794 items_per_second=193.522G/s
string_view_compare/stl                0.365 ns        0.365 ns   1844398200 items_per_second=191.748G/s
stack_push_pop/castle                   1227 ns         1238 ns       590710 items_per_second=411.916M/s
stack_push_pop/stl                      1130 ns         1139 ns       629343 items_per_second=447.93M/s
stack_top/castle                  2.7627e+16 ns   6.5324e+10 ns            1 items_per_second=0.0153082/s
stack_top/stl                     2.7627e+16 ns   6.5324e+10 ns            1 items_per_second=0.0153082/s
queue_push_pop/castle                   1518 ns         1523 ns       459853 items_per_second=334.96M/s
queue_push_pop/stl                      1241 ns         1249 ns       564807 items_per_second=408.272M/s
queue_front/castle                2.7627e+16 ns   6.7732e+10 ns            1 items_per_second=0.0147641/s
queue_front/stl                   2.7627e+16 ns   6.7732e+10 ns            1 items_per_second=0.014764/s

----------------------------------------
Running castle_benchmark_events
----------------------------------------
2026-09-17T15:49:58+09:00
Running /home/worker/study_space/castle/build/bin/castle_benchmark_events
Run on (16 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 22528 KiB (x0)
Load Average: 17.11, 21.26, 29.53
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-------------------------------------------------------------------------------------------
Benchmark                                 Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------
event_dispatch/castle            2.7627e+16 ns      4498932 ns            1 items_per_second=889.1/s
event_dispatch/stl               2.7627e+16 ns      4538780 ns            1 items_per_second=881.294/s
event_register/castle                   534 ns          538 ns      1315437 items_per_second=1.86007M/s
event_register/stl                      543 ns          548 ns      1159328 items_per_second=1.82388M/s
inplace_event_dispatch/castle    2.7627e+16 ns   2958296309 ns            1 items_per_second=1.35213/s
inplace_event_dispatch/stl       2.7627e+16 ns   2958321994 ns            1 items_per_second=1.35212/s
inplace_event_register/castle           548 ns          551 ns      1315269 items_per_second=1.81556M/s
inplace_event_register/stl              542 ns          546 ns      1269768 items_per_second=1.8304M/s
signal_emit/castle               2.7627e+16 ns   6068972904 ns            1 items_per_second=0.65909/s
signal_emit/stl                  2.7627e+16 ns   6068988295 ns            1 items_per_second=0.659088/s
signal_connect_disconnect/castle        559 ns          565 ns      1187578 items_per_second=3.53797M/s
signal_connect_disconnect/stl           550 ns          552 ns      1255584 items_per_second=3.62291M/s

----------------------------------------
Running castle_benchmark_iterator
----------------------------------------
2026-09-17T15:50:07+09:00
Running /home/worker/study_space/castle/build/bin/castle_benchmark_iterator
Run on (16 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 22528 KiB (x0)
Load Average: 17.17, 21.14, 29.40
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------
iterator_distance/castle 2.7627e+16 ns      4493786 ns            1 items_per_second=222.53/s
iterator_distance/stl    2.7627e+16 ns      4534352 ns            1 items_per_second=220.539/s
iterator_advance/castle  2.7627e+16 ns      4547325 ns            1 items_per_second=219.91/s
iterator_advance/stl     2.7627e+16 ns      4558131 ns            1 items_per_second=219.388/s
iterator_next/castle     2.7627e+16 ns      4568239 ns            1 items_per_second=218.903/s
iterator_next/stl        2.7627e+16 ns      4578219 ns            1 items_per_second=218.426/s

----------------------------------------
Running castle_benchmark_math
----------------------------------------
2026-09-17T15:50:07+09:00
Running /home/worker/study_space/castle/build/bin/castle_benchmark_math
Run on (16 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 22528 KiB (x0)
Load Average: 17.17, 21.14, 29.40
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
--------------------------------------------------------------------------------------
Benchmark                            Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------
math_abs/castle                  0.419 ns        0.418 ns   1718850434 items_per_second=2.39142G/s
math_abs/stl                     0.373 ns        0.373 ns   1907468657 items_per_second=2.68231G/s
math_sqrt/castle                 0.373 ns        0.373 ns   1789841363 items_per_second=2.67908G/s
math_sqrt/stl                    0.372 ns        0.372 ns   1846177235 items_per_second=2.6882G/s
math_clamp/castle                0.360 ns        0.359 ns   1942081519 items_per_second=2.78222G/s
math_clamp/stl                   0.362 ns        0.361 ns   1931168279 items_per_second=2.76748G/s
math_is_power_of_two/castle      0.437 ns        0.437 ns   1818109486 items_per_second=2.28978G/s
math_is_power_of_two/stl         0.404 ns        0.404 ns   1749100110 items_per_second=2.47562G/s
math_fib/castle                  0.361 ns        0.361 ns   1943133051 items_per_second=2.77304G/s
math_fib/stl                      11.6 ns         11.5 ns     62672079 items_per_second=86.5892M/s
math_mean/castle                 0.361 ns        0.360 ns   1942878091 items_per_second=22.1963G/s
math_mean/stl                    0.393 ns        0.393 ns   1755028156 items_per_second=20.3544G/s

----------------------------------------
Running castle_benchmark_utility
----------------------------------------
2026-09-17T15:50:20+09:00
Running /home/worker/study_space/castle/build/bin/castle_benchmark_utility
Run on (16 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 22528 KiB (x0)
Load Average: 17.25, 20.96, 29.21
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------
bitset_set_reset/castle        1.93 ns         1.93 ns    363407409 items_per_second=1.03682G/s
bitset_set_reset/stl           1.91 ns         1.91 ns    368845071 items_per_second=1.04756G/s
bitset_count/castle           0.365 ns        0.364 ns   1926531220 items_per_second=351.409G/s
bitset_count/stl              0.364 ns        0.363 ns   1899450535 items_per_second=352.675G/s
bitset_test/castle            0.451 ns        0.362 ns   1915103264 items_per_second=2.76563G/s
bitset_test/stl               0.365 ns        0.365 ns   1934786105 items_per_second=2.74319G/s
bitset_to_ullong/castle       0.466 ns        0.365 ns   1938657192 items_per_second=2.73936G/s
bitset_to_ullong/stl          0.367 ns        0.367 ns   1939672548 items_per_second=2.72439G/s
bitset_to_string/castle         155 ns          154 ns      4480556 items_per_second=828.943M/s
bitset_to_string/stl            197 ns          197 ns      3711523 items_per_second=649.754M/s
pair_construct/castle         0.757 ns        0.757 ns    973969889 items_per_second=1.32163G/s
pair_construct/stl            0.771 ns        0.771 ns    929774146 items_per_second=1.29706G/s
pair_get/castle               0.371 ns        0.369 ns   1923716506 items_per_second=2.70697G/s
pair_get/stl                  0.369 ns        0.369 ns   1921419456 items_per_second=2.7104G/s
pair_compare/castle           0.362 ns        0.362 ns   1933441241 items_per_second=2.76398G/s
pair_compare/stl              0.364 ns        0.364 ns   1928299043 items_per_second=2.74803G/s
variant_construct/castle      0.735 ns        0.735 ns    923266540 items_per_second=1.36105G/s
variant_construct/stl         0.734 ns        0.734 ns    914171614 items_per_second=1.36283G/s
variant_emplace/castle   2.7627e+16 ns   1.7848e+10 ns            1 items_per_second=0.0560275/s
variant_emplace/stl      2.7627e+16 ns   1.7848e+10 ns            1 items_per_second=0.0560274/s
variant_get/castle            0.363 ns        0.363 ns   1913203704 items_per_second=2.75755G/s
variant_get/stl               0.364 ns        0.363 ns   1932105130 items_per_second=2.75188G/s
variant_visit/castle          0.362 ns        0.362 ns   1908697004 items_per_second=2.76328G/s
variant_visit/stl             0.360 ns        0.360 ns   1932699948 items_per_second=2.77618G/s

All benchmarks completed.
```