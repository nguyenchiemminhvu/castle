```
====================
Running benchmarks...
====================

----------------------------------------
Running castle_benchmark_algorithms
----------------------------------------
2026-09-17T18:56:40+07:00
Running /home/ncmv/work_space/castle/build/bin/castle_benchmark_algorithms
Run on (16 X 1996.27 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.57, 0.36, 0.11
---------------------------------------------------------------------------------------
Benchmark                             Time             CPU   Iterations UserCounters...
---------------------------------------------------------------------------------------
algorithm_find/castle        1.4183e+11 ns      2280295 ns            1 items_per_second=111.828k/s
algorithm_find/stl           1.4183e+11 ns      2330118 ns            1 items_per_second=109.437k/s
algorithm_sort/castle        1.4183e+11 ns      2355425 ns            1 items_per_second=108.261k/s
algorithm_sort/stl           1.4183e+11 ns      2386553 ns            1 items_per_second=106.849k/s
algorithm_transform/castle   1.4183e+11 ns      2414625 ns            1 items_per_second=105.606k/s
algorithm_transform/stl      1.4183e+11 ns      2464859 ns            1 items_per_second=103.454k/s
algorithm_lower_bound/castle 1.4183e+11 ns      2500997 ns            1 items_per_second=399.841/s
algorithm_lower_bound/stl    1.4183e+11 ns      2525663 ns            1 items_per_second=395.936/s

----------------------------------------
Running castle_benchmark_callbacks
----------------------------------------
2026-09-17T18:56:40+07:00
Running /home/ncmv/work_space/castle/build/bin/castle_benchmark_callbacks
Run on (16 X 1996.27 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.57, 0.36, 0.11
--------------------------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------
function_free_call/castle              0.181 ns        0.197 ns   3520434255 items_per_second=5.07788G/s
function_free_call/stl                 0.928 ns         1.01 ns    689306621 items_per_second=987.628M/s
function_compile_time_call/castle      0.181 ns        0.197 ns   3553340527 items_per_second=5.07722G/s
function_compile_time_call/stl         0.187 ns        0.197 ns   3549575789 items_per_second=5.07125G/s
function_functor_call/castle           0.190 ns        0.197 ns   3554491781 items_per_second=5.07617G/s
function_functor_call/stl              0.834 ns        0.866 ns    791405088 items_per_second=1.15488G/s
function_member_call/castle            0.190 ns        0.197 ns   3560751386 items_per_second=5.07254G/s
function_member_call/stl               0.190 ns        0.197 ns   3555253131 items_per_second=5.07778G/s
inplace_function_call/castle           0.838 ns        0.870 ns    803575378 items_per_second=1.14944G/s
inplace_function_call/stl              0.858 ns        0.892 ns    783264187 items_per_second=1.1216G/s
inplace_function_construct/castle      0.974 ns         1.01 ns    685375620 items_per_second=988.727M/s
inplace_function_construct/stl         0.982 ns         1.01 ns    691536281 items_per_second=986.361M/s

----------------------------------------
Running castle_benchmark_chrono
----------------------------------------
2026-09-17T18:56:53+07:00
Running /home/ncmv/work_space/castle/build/bin/castle_benchmark_chrono
Run on (16 X 1996.27 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.48, 0.38, 0.12
---------------------------------------------------------------------------------------
Benchmark                             Time             CPU   Iterations UserCounters...
---------------------------------------------------------------------------------------
duration_add/castle               0.190 ns        0.197 ns   3545815212 items_per_second=5.07485G/s
duration_add/stl                  0.187 ns        0.197 ns   3559766486 items_per_second=5.07424G/s
duration_cast/castle              0.186 ns        0.197 ns   3560604226 items_per_second=5.06965G/s
duration_cast/stl                 0.186 ns        0.197 ns   3561465771 items_per_second=5.0742G/s
duration_update/castle       1.5514e+11 ns   3682662282 ns            1 items_per_second=0.543085/s
duration_update/stl          1.5514e+11 ns   3682676639 ns            1 items_per_second=0.543083/s
time_point_add/castle             0.186 ns        0.197 ns   3539107205 items_per_second=5.08154G/s
time_point_add/stl                0.186 ns        0.197 ns   3557853002 items_per_second=5.08778G/s
time_point_compare/castle         0.186 ns        0.197 ns   3528828784 items_per_second=5.08103G/s
time_point_compare/stl            0.186 ns        0.197 ns   3553393468 items_per_second=5.07546G/s
time_point_difference/castle      0.186 ns        0.197 ns   3559615588 items_per_second=5.07932G/s
time_point_difference/stl         0.186 ns        0.197 ns   3560793662 items_per_second=5.07575G/s
steady_clock_now/castle            15.7 ns         17.0 ns     41361648 items_per_second=58.8798M/s
steady_clock_now/stl               16.4 ns         17.8 ns     39828818 items_per_second=56.0649M/s
system_clock_now/castle            15.6 ns         17.1 ns     39015918 items_per_second=58.6208M/s
system_clock_now/stl               16.1 ns         17.5 ns     40193720 items_per_second=57.204M/s

----------------------------------------
Running castle_benchmark_containers
----------------------------------------
2026-09-17T18:57:05+07:00
Running /home/ncmv/work_space/castle/build/bin/castle_benchmark_containers
Run on (16 X 1996.27 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.41, 0.40, 0.13
--------------------------------------------------------------------------------------------
Benchmark                                  Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------------
array_construct/castle                 0.181 ns        0.197 ns   3553662399 items_per_second=1.294T/s
array_construct/stl                    0.182 ns        0.197 ns   3559433155 items_per_second=1.29532T/s
array_access/castle                     17.5 ns         19.1 ns     36786483 items_per_second=13.3358G/s
array_access/stl                        23.0 ns         25.1 ns     27908801 items_per_second=10.1791G/s
vector_construct/castle                0.180 ns        0.197 ns   3531022130 items_per_second=1.29559T/s
vector_construct/stl                    4.97 ns         5.42 ns    128493110 items_per_second=47.0264G/s
vector_push_back/castle                  727 ns          794 ns       811247 items_per_second=321.036M/s
vector_push_back/stl                     727 ns          794 ns       871334 items_per_second=321.084M/s
vector_emplace_back/castle               728 ns          795 ns       879715 items_per_second=320.581M/s
vector_emplace_back/stl                  749 ns          815 ns       859762 items_per_second=312.861M/s
vector_pop_back/castle                   722 ns          789 ns       881937 items_per_second=323.19M/s
vector_pop_back/stl                      739 ns          808 ns       874557 items_per_second=315.721M/s
vector_clear/castle                      658 ns          719 ns       971771 items_per_second=354.647M/s
vector_clear/stl                         661 ns          722 ns       966583 items_per_second=353.299M/s
hash_map_insert/castle                   791 ns          864 ns       812582 items_per_second=72.9091M/s
hash_map_insert/stl                     1040 ns         1085 ns       640294 items_per_second=58.0485M/s
hash_map_find_hit/castle          1.8276e+11 ns   2.0788e+10 ns            1 items_per_second=0.0481038/s
hash_map_find_hit/stl             1.8276e+11 ns   2.0788e+10 ns            1 items_per_second=0.0481038/s
hash_map_find_miss/castle         1.8276e+11 ns   2.0788e+10 ns            1 items_per_second=0.0481038/s
hash_map_find_miss/stl            1.8276e+11 ns   2.0788e+10 ns            1 items_per_second=0.0481037/s
hash_map_erase/castle                    855 ns          857 ns       821830 items_per_second=73.4982M/s
hash_map_erase/stl                       993 ns         1080 ns       653158 items_per_second=58.3556M/s
hash_set_insert/castle                   793 ns          865 ns       800584 items_per_second=72.7931M/s
hash_set_insert/stl                     1047 ns         1141 ns       613974 items_per_second=55.223M/s
hash_set_contains/castle          1.8874e+11 ns   2.7139e+10 ns            1 items_per_second=0.0368467/s
hash_set_contains/stl             1.8874e+11 ns   2.7140e+10 ns            1 items_per_second=0.0368466/s
hash_set_erase/castle                    783 ns          855 ns       815352 items_per_second=73.6415M/s
hash_set_erase/stl                       982 ns         1072 ns       638520 items_per_second=58.7525M/s
tree_map_insert/castle                  1196 ns         1306 ns       535713 items_per_second=48.2464M/s
tree_map_insert/stl                     1412 ns         1546 ns       450890 items_per_second=40.7521M/s
tree_map_find/castle              1.9445e+11 ns   3.3366e+10 ns            1 items_per_second=0.0299704/s
tree_map_find/stl                 1.9445e+11 ns   3.3366e+10 ns            1 items_per_second=0.0299704/s
tree_map_lower_bound/castle       1.9445e+11 ns   3.3366e+10 ns            1 items_per_second=0.0299704/s
tree_map_lower_bound/stl          1.9445e+11 ns   3.3366e+10 ns            1 items_per_second=0.0299703/s
tree_map_erase/castle                   1017 ns         1110 ns       630074 items_per_second=56.7391M/s
tree_map_erase/stl                      1308 ns         1425 ns       490433 items_per_second=44.2025M/s
tree_set_insert/castle                  1174 ns         1283 ns       543331 items_per_second=49.1207M/s
tree_set_insert/stl                     1375 ns         1505 ns       466280 items_per_second=41.8483M/s
tree_set_find/castle              2.0051e+11 ns   3.9973e+10 ns            1 items_per_second=0.025017/s
tree_set_find/stl                 2.0051e+11 ns   3.9973e+10 ns            1 items_per_second=0.025017/s
tree_set_lower_bound/castle       2.0051e+11 ns   3.9973e+10 ns            1 items_per_second=0.025017/s
tree_set_lower_bound/stl          2.0051e+11 ns   3.9973e+10 ns            1 items_per_second=0.025017/s
tree_set_erase/castle                   1023 ns         1114 ns       629437 items_per_second=56.5556M/s
tree_set_erase/stl                      1307 ns         1427 ns       486564 items_per_second=44.1488M/s
string_construct/castle                 13.5 ns         14.7 ns     47545984 items_per_second=67.8096M/s
string_construct/stl                    12.6 ns         13.8 ns     51634467 items_per_second=72.6983M/s
string_append/castle                     662 ns          723 ns       965883 items_per_second=91.2498M/s
string_append/stl                        670 ns          732 ns       968330 items_per_second=90.1105M/s
string_compare_equal/castle             1.49 ns         1.63 ns    426734549 items_per_second=40.5982G/s
string_compare_equal/stl                1.50 ns         1.63 ns    427483065 items_per_second=40.4342G/s
string_find/castle                      3.68 ns         4.00 ns    175222285 items_per_second=16.4866G/s
string_find/stl                         7.73 ns         8.43 ns     83288131 items_per_second=7.82537G/s
string_view_construct/castle           0.182 ns        0.198 ns   3540033531 items_per_second=5.05218G/s
string_view_construct/stl              0.182 ns        0.198 ns   3551610692 items_per_second=5.03844G/s
string_view_find/castle                0.181 ns        0.197 ns   3519984908 items_per_second=355.436G/s
string_view_find/stl                   0.193 ns        0.197 ns   3558239847 items_per_second=355.601G/s
string_view_find_substring/castle       3.84 ns         3.84 ns    175504159 items_per_second=18.2488G/s
string_view_find_substring/stl          3.46 ns         3.74 ns    188164589 items_per_second=18.7032G/s
string_view_compare/castle             0.181 ns        0.197 ns   3548917696 items_per_second=355.378G/s
string_view_compare/stl                0.181 ns        0.197 ns   3557203822 items_per_second=354.915G/s
stack_push_pop/castle                    850 ns          931 ns       753148 items_per_second=547.779M/s
stack_push_pop/stl                       854 ns          935 ns       748958 items_per_second=545.44M/s
stack_top/castle                  2.2167e+11 ns   6.2858e+10 ns            1 items_per_second=0.0159089/s
stack_top/stl                     2.2167e+11 ns   6.2858e+10 ns            1 items_per_second=0.0159089/s
queue_push_pop/castle                    847 ns          926 ns       751907 items_per_second=550.597M/s
queue_push_pop/stl                       804 ns          879 ns       796675 items_per_second=579.96M/s
queue_front/castle                2.2434e+11 ns   6.5763e+10 ns            1 items_per_second=0.0152061/s
queue_front/stl                   2.2434e+11 ns   6.5763e+10 ns            1 items_per_second=0.0152061/s

----------------------------------------
Running castle_benchmark_events
----------------------------------------
2026-09-17T18:58:11+07:00
Running /home/ncmv/work_space/castle/build/bin/castle_benchmark_events
Run on (16 X 1996.27 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.18, 0.52, 0.20
-------------------------------------------------------------------------------------------
Benchmark                                 Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------
event_dispatch/castle            2.2434e+11 ns      2667434 ns            1 items_per_second=1.49957k/s
event_dispatch/stl               2.2434e+11 ns      2770425 ns            1 items_per_second=1.44382k/s
event_register/castle                   651 ns          712 ns       984234 items_per_second=1.40427M/s
event_register/stl                      670 ns          731 ns       965929 items_per_second=1.36845M/s
inplace_event_dispatch/castle    2.2722e+11 ns   3138153280 ns            1 items_per_second=1.27463/s
inplace_event_dispatch/stl       2.2722e+11 ns   3138167959 ns            1 items_per_second=1.27463/s
inplace_event_register/castle           653 ns          717 ns       992010 items_per_second=1.39429M/s
inplace_event_register/stl              658 ns          720 ns       984460 items_per_second=1.38887M/s
signal_emit/castle               2.3013e+11 ns   6317832594 ns            1 items_per_second=0.633129/s
signal_emit/stl                  2.3013e+11 ns   6317847931 ns            1 items_per_second=0.633127/s
signal_connect_disconnect/castle        651 ns          714 ns       981502 items_per_second=2.80171M/s
signal_connect_disconnect/stl           669 ns          731 ns       963535 items_per_second=2.73518M/s

----------------------------------------
Running castle_benchmark_iterator
----------------------------------------
2026-09-17T18:58:20+07:00
Running /home/ncmv/work_space/castle/build/bin/castle_benchmark_iterator
Run on (16 X 1996.27 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.15, 0.54, 0.20
-----------------------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------
iterator_distance/castle 2.3302e+11 ns      2730594 ns            1 items_per_second=366.221/s
iterator_distance/stl    2.3302e+11 ns      2759599 ns            1 items_per_second=362.371/s
iterator_advance/castle  2.3302e+11 ns      2767484 ns            1 items_per_second=361.339/s
iterator_advance/stl     2.3302e+11 ns      2774666 ns            1 items_per_second=360.404/s
iterator_next/castle     2.3302e+11 ns      2813789 ns            1 items_per_second=355.393/s
iterator_next/stl        2.3302e+11 ns      2826884 ns            1 items_per_second=353.746/s

----------------------------------------
Running castle_benchmark_math
----------------------------------------
2026-09-17T18:58:20+07:00
Running /home/ncmv/work_space/castle/build/bin/castle_benchmark_math
Run on (16 X 1996.27 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.15, 0.54, 0.20
--------------------------------------------------------------------------------------
Benchmark                            Time             CPU   Iterations UserCounters...
--------------------------------------------------------------------------------------
math_abs/castle                  0.181 ns        0.197 ns   3541078995 items_per_second=5.06622G/s
math_abs/stl                     0.180 ns        0.197 ns   3554074551 items_per_second=5.08346G/s
math_sqrt/castle                 0.181 ns        0.197 ns   3557978216 items_per_second=5.07455G/s
math_sqrt/stl                    0.181 ns        0.197 ns   3549089696 items_per_second=5.06985G/s
math_clamp/castle                0.181 ns        0.197 ns   3561221620 items_per_second=5.07716G/s
math_clamp/stl                   0.180 ns        0.197 ns   3561416938 items_per_second=5.0804G/s
math_is_power_of_two/castle      0.180 ns        0.197 ns   3556616949 items_per_second=5.07947G/s
math_is_power_of_two/stl         0.180 ns        0.197 ns   3546821412 items_per_second=5.07923G/s
math_fib/castle                  0.181 ns        0.197 ns   3561366657 items_per_second=5.07222G/s
math_fib/stl                      3.88 ns         4.24 ns    169080371 items_per_second=236.036M/s
math_mean/castle                 0.181 ns        0.197 ns   3552696400 items_per_second=40.5895G/s
math_mean/stl                    0.180 ns        0.197 ns   3548539370 items_per_second=40.6383G/s

----------------------------------------
Running castle_benchmark_utility
----------------------------------------
2026-09-17T18:58:30+07:00
Running /home/ncmv/work_space/castle/build/bin/castle_benchmark_utility
Run on (16 X 1996.27 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.13, 0.56, 0.21
-----------------------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------------
bitset_set_reset/castle       0.181 ns        0.197 ns   3555084524 items_per_second=10.1421G/s
bitset_set_reset/stl          0.181 ns        0.197 ns   3554275403 items_per_second=10.1517G/s
bitset_count/castle           0.180 ns        0.197 ns   3558218829 items_per_second=650.623G/s
bitset_count/stl              0.192 ns        0.197 ns   3559120389 items_per_second=649.452G/s
bitset_test/castle            0.196 ns        0.196 ns   3547045277 items_per_second=5.08945G/s
bitset_test/stl               0.197 ns        0.197 ns   3550251430 items_per_second=5.08247G/s
bitset_to_ullong/castle       0.180 ns        0.197 ns   3552667316 items_per_second=5.08138G/s
bitset_to_ullong/stl          0.181 ns        0.197 ns   3558435941 items_per_second=5.07579G/s
bitset_to_string/castle        46.0 ns         50.2 ns     13876652 items_per_second=2.55225G/s
bitset_to_string/stl           57.9 ns         63.1 ns     11257686 items_per_second=2.02758G/s
pair_construct/castle         0.181 ns        0.197 ns   3527381546 items_per_second=5.07385G/s
pair_construct/stl            0.181 ns        0.198 ns   3551407421 items_per_second=5.06088G/s
pair_get/castle               0.180 ns        0.197 ns   3557254708 items_per_second=5.07861G/s
pair_get/stl                  0.180 ns        0.197 ns   3534683213 items_per_second=5.07882G/s
pair_compare/castle           0.181 ns        0.197 ns   3557364639 items_per_second=5.07394G/s
pair_compare/stl              0.180 ns        0.197 ns   3559170063 items_per_second=5.08197G/s
variant_construct/castle      0.180 ns        0.197 ns   3517717583 items_per_second=5.08117G/s
variant_construct/stl         0.181 ns        0.197 ns   3552741568 items_per_second=5.06918G/s
variant_emplace/castle   2.5845e+11 ns   1.6229e+10 ns            1 items_per_second=0.0616167/s
variant_emplace/stl      2.5845e+11 ns   1.6229e+10 ns            1 items_per_second=0.0616166/s
variant_get/castle            0.181 ns        0.197 ns   3552342740 items_per_second=5.07509G/s
variant_get/stl               0.181 ns        0.197 ns   3559572236 items_per_second=5.06666G/s
variant_visit/castle          0.181 ns        0.197 ns   3556029928 items_per_second=5.06585G/s
variant_visit/stl             0.180 ns        0.197 ns   3506262571 items_per_second=5.07863G/s

All benchmarks completed.
```